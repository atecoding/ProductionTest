#include "base.h"
#include "Description.h"
#include "DescriptionAM1.h"

DescriptionAM1::DescriptionAM1(uint8 moduleTypes_, uint16 const bcdVersion_) :
Description(moduleTypes_, bcdVersion_)
{

}

int DescriptionAM1::getNumInputs() const
{
	return 14;
}

int DescriptionAM1::getNumOutputs() const
{
	return 12;
}

String DescriptionAM1::getInputName(const int input) const
{
	if (input < 4)
		return analogString + String(input + 1);

	return mbString + String(input - 3);
}

String DescriptionAM1::getOutputName(const int output) const
{
	if (output < 2)
		return analogString + String(output + 1);

	return mbString + String(output - 1);
}

int DescriptionAM1::getInputType(int const input) const
{
	if (input < 4)
		return TEDS_IN;

	return MIKEYBUS_IN;
}

int DescriptionAM1::getOutputType(int const output) const
{
	if (output < 2)
		return AMP_OUT;

	return MIKEYBUS_OUT;
}

int DescriptionAM1::getModuleForInput(int const input) const
{
	if (input < 4)
		return 0;

	return 1;
}

int DescriptionAM1::getModuleForOutput(int const output) const
{
	if (output < 2)
		return 0;

	return 1;
}

#if JUCE_MAC
String DescriptionAM1::getCoreAudioName() const
{
    return "EchoAIO-M1";
}
#endif
