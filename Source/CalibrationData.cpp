#include "base.h"
#include "AcousticIO.h"
#include "CalibrationData.h"
#include "crc32.h"

const Time AIOSCalibrationData::epoch(2015, 3, 1, 0, 0);	// April 1, 2015 0:00

AIOSCalibrationData::AIOSCalibrationData()
{
	reset();
}

bool AIOSCalibrationData::isChecksumOK() const
{
	return checksumOK;
}

String AIOSCalibrationData::getDate() const
{
	if (0 == data.time)
	{
		return "Date not set";
	}

	Time time(epoch);
	time += RelativeTime(data.time * 60.0);
	return time.toString(true, true, false, true);
}

uint32 AIOSCalibrationData::calculateChecksum() const
{
	// skip last uint32 - don't calculate the checksum of the checksum
	return CRC32Block((uint32 const *)&data, (sizeof(data) / sizeof(uint32)) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);
}


void AIOSCalibrationData::updateChecksum()
{
	data.checksum = calculateChecksum();
	checksumOK = true;
}

void AIOSCalibrationData::validateChecksum()
{
	checksumOK = calculateChecksum() == data.checksum;
}

void AIOSCalibrationData::updateDate()
{
	Time now(Time::getCurrentTime());
	RelativeTime elapsed(now - epoch);
	data.time = roundDoubleToInt(elapsed.inMinutes());
}

void AIOSCalibrationData::reset()
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

String AIOSCalibrationData::toString() const
{
	String text(getDate() + newLine);
	text += "Voltage input: " + printValue(data.inputGains[AIOS_VOLTAGE_INPUT_CHANNEL]) + newLine;
	text += "Current input: " + printValue(data.inputGains[AIOS_CURRENT_INPUT_CHANNEL]) + newLine;
	text += "Voltage output: " + printValue(data.outputGains[AIOS_VOLTAGE_OUTPUT_CHANNEL]) + newLine;
	return text;
}

String AIOSCalibrationData::printValue(uint16 const value)
{
	float percent = (uint32(value) / 32768.0f) * 100.0f;
	percent -= 100.0f;
	String text("0x" + String::toHexString((int32)value) + "  (");
	if (percent > 0.0f)
		text += "+";
	text += String(percent, 4) + "%)";
	return text;
}

ExternalSpeakerMonitorCalibrationData::ExternalSpeakerMonitorCalibrationData() :
voltageInputGain(1.0f),
currentInputGain(1.0f)
{

}

String ExternalSpeakerMonitorCalibrationData::printValue(float const value)
{
	float percent = value * 100.0f;
	percent -= 100.0f;
	String text(String(value, 6) + "  (");
	if (percent > 0.0f)
		text += "+";
	text += String(percent, 4) + "%)";
	return text;
}
