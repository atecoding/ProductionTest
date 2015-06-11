#include "base.h"
#include "Description.h"
#include "DescriptionAMB.h"

DescriptionAMB::DescriptionAMB(uint8 moduleTypes_) :
Description(moduleTypes_)
{

}

int DescriptionAMB::getNumInputs() const
{
	return 14;
}

int DescriptionAMB::getNumOutputs() const
{
	return 12;
}

String DescriptionAMB::getInputName(const int input) const
{
	if (input < 4)
		return analogString + String(input + 1);

	return mbString + String(input - 3);
}

String DescriptionAMB::getOutputName(const int output) const
{
	if (output < 2)
		return analogString + String(output + 1);

	return mbString + String(output - 1);
}

int DescriptionAMB::getInputType(int const input) const
{
	if (input < 4)
		return TEDS_IN;

	return MIKEYBUS_IN;
}

int DescriptionAMB::getOutputType(int const output) const
{
	if (output < 2)
		return AMP_OUT;

	return MIKEYBUS_OUT;
}

int DescriptionAMB::getModuleForInput(int const input) const
{
	if (input < 4)
		return 0;

	return 1;
}

int DescriptionAMB::getModuleForOutput(int const output) const
{
	if (output < 2)
		return 0;

	return 1;
}

