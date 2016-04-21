#include "../base.h"
#include "../AcousticIO.h"
#include "CalibrationDataV2.h"
#include "crc32.h"

const Time CalibrationDataV2::epoch(2015, 3, 1, 0, 0);	// April 1, 2015 0:00

CalibrationDataV2::CalibrationDataV2()
{
	reset();
}

bool CalibrationDataV2::isChecksumOK() const
{
	return checksumOK;
}

String CalibrationDataV2::getDate() const
{
	if (0 == data.time)
	{
		return "Date not set";
	}

	Time time(epoch);
	time += RelativeTime(data.time * 60.0);
	return time.toString(true, true, false, true);
}

uint32 CalibrationDataV2::calculateChecksum() const
{
	// skip last uint32 - don't calculate the checksum of the checksum
	return CRC32Block((uint32 const *)&data, (sizeof(data) / sizeof(uint32)) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);
}


void CalibrationDataV2::updateChecksum()
{
	data.checksum = calculateChecksum();
	checksumOK = true;
}

void CalibrationDataV2::validateChecksum()
{
	checksumOK = calculateChecksum() == data.checksum;
}

void CalibrationDataV2::updateDate()
{
	Time now(Time::getCurrentTime());
	RelativeTime elapsed(now - epoch);
	data.time = roundDoubleToInt(elapsed.inMinutes());
}

void CalibrationDataV2::reset()
{
	zerostruct(data);

	for (int i = 0; i < numElementsInArray(data.inputGains); ++i)
	{
		data.inputGains[i] = 0x8000;
	}

	for (int i = 0; i < numElementsInArray(data.outputGains); ++i)
	{
		data.outputGains[i] = 0x8000;
	}

	validateChecksum();
}

void CalibrationDataV2::reset(int moduleNumber)
{
    int numInputs = numElementsInArray(data.inputGains) / 2;
    for (int i = moduleNumber * numInputs; i < numInputs; ++i)
    {
        data.inputGains[i] = 0x8000;
    }
    
    int numOutputs = numElementsInArray(data.outputGains);
    for (int i = moduleNumber * numOutputs; i < numOutputs; ++i)
    {
        data.outputGains[i] = 0x8000;
    }
    
    validateChecksum();
}

String CalibrationDataV2::toString() const
{
    String text(getDate() + newLine);
    text += newLine;

    for (int input = 0; input < numElementsInArray(data.inputGains); ++input)
    {
        text += "Input " + String(input + 1) + ": " + printValue(data.inputGains[input]) + newLine;
    }
    for (int output = 0; output < numElementsInArray(data.outputGains); ++output)
    {
        text += "Output " + String(output + 1) + ": " + printValue(data.outputGains[output]) + newLine;
    }
    
    return text;
}

String CalibrationDataV2::printValue(uint16 const value)
{
	float percent = (uint32(value) / 32768.0f) * 100.0f;
	percent -= 100.0f;
	String text("0x" + String::toHexString((int32)value) + "  (");
	if (percent > 0.0f)
		text += "+";
	text += String(percent, 4) + "%)";
	return text;
}

