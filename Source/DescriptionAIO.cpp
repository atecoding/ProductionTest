#include "base.h"
#include "Description.h"
#include "DescriptionAIO.h"
#include "AcousticIO.h"
#include "AIOAModule.h"
#include "AIOSModule.h"

DescriptionAIO::DescriptionAIO(uint8 moduleTypes_, uint16 const bcdVersion_) :
Description(moduleTypes_, bcdVersion_)
{
}


int DescriptionAIO::getNumInputs() const
{
	return 8;
}

int DescriptionAIO::getNumOutputs() const
{
	return 4;
}

String DescriptionAIO::getInputName(const int input) const
{
	int inputType = getInputType(input);
	switch (inputType)
	{
	case SPKRMON_VOLTAGE_IN:
		return "Voltage " + String(input + 1);

	case SPKRMON_CURRENT_IN:
		return "Current " + String(input + 1);
	}

	return analogString + String(input + 1);
}

String DescriptionAIO::getOutputName(const int output) const
{
	return analogString + String(output + 1);
}

int DescriptionAIO::getInputType(int const input) const
{
	int module = getModuleForInput(input);
	int moduleType = getModuleType(module);
	int moduleInput;

	switch (moduleType)
	{
	case ACOUSTICIO_ANALOG_MODULE:
		break;

	case ACOUSTICIO_SPKRMON_MODULE:
		moduleInput = input % 4;
		switch (moduleInput)
		{
		case 0:
		case 1:
			break;

		case 2:
			return SPKRMON_VOLTAGE_IN;

		case 3:
			return SPKRMON_CURRENT_IN;
		}
		break;
	}

	return TEDS_IN;
}

int DescriptionAIO::getOutputType(int const /*output*/) const
{
	return AMP_OUT;
}

int DescriptionAIO::getModuleForInput(int const input) const
{
    jassert(input >= 0 && input < 8);

	if (input < 0 || input >= 8)
		return -1;

	return input / 4;
}

int DescriptionAIO::getModuleForOutput(int const output) const
{
    jassert(output >= 0 && output < 4);

	if (output < 0 || output >= 8)
		return -1;

	return output / 2;
}

#if JUCE_MAC
String DescriptionAIO::getCoreAudioName() const
{
    return "EchoAIO";
}
#endif
