#include "base.h"
#include "Description.h"

String const static analogString("Analog ");
String const static mbString("MB ");

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
    return analogString + String(input + 1);
}

String DescriptionAIO::getOutputName(const int output) const
{
    return analogString + String(output + 1);
}

int DescriptionAIO::getInputType(int const /*input*/) const
{
    return TEDS_IN;
}

int DescriptionAIO::getOutputType(int const /*output*/) const
{
    return AMP_OUT;
}

int DescriptionAIO::getModuleForInput( int const input ) const
{
	return input / 4;
}

int DescriptionAIO::getModuleForOutput( int const output ) const
{
	return output / 2;
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

int DescriptionAMB::getModuleForInput( int const input ) const
{
	if (input < 4)
		return 0;

	return 1;
}

int DescriptionAMB::getModuleForOutput( int const output ) const
{
	if (output < 2)
		return 0;

	return 1;
}
