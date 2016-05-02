#include "base.h"
#include "Description.h"
#include "DescriptionAM2.h"

DescriptionAM2::DescriptionAM2(uint8 moduleTypes_, uint16 productID_, uint16 const bcdVersion_) :
Description(moduleTypes_, bcdVersion_),
productID(productID_)
{

}

int DescriptionAM2::getNumInputs() const
{
	return 20;
}

int DescriptionAM2::getNumOutputs() const
{
	return 20;
}

String DescriptionAM2::getInputName(const int input) const
{
	if (input < 10)
		return mbString + " 1-" + String(input + 1);

	return mbString + " 2-" + String(input - 9);
}

String DescriptionAM2::getOutputName(const int output) const
{
	if (output < 10)
		return mbString + " 1-" + String(output + 1);

	return mbString + " 2-" + String(output - 9);
}

int DescriptionAM2::getInputType(int const input) const
{
	return MIKEYBUS_IN;
}

int DescriptionAM2::getOutputType(int const output) const
{
	return MIKEYBUS_OUT;
}

int DescriptionAM2::getModuleForInput(int const input) const
{
	if (input < 10)
		return 0;

	return 1;
}

int DescriptionAM2::getModuleForOutput(int const output) const
{
	if (output < 10)
		return 0;

	return 1;
}

bool DescriptionAM2::supportsPeakMeters() const
{
    return getInterfaceModuleVersion() >= ECHOAIO_INTERFACE_MODULE_REV2;
}

#if JUCE_MAC
String DescriptionAM2::getCoreAudioName() const
{
    return "EchoAIO-M2";
}
#endif

