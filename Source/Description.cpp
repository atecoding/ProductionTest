#include "base.h"
#include "Description.h"
#include "AcousticIO.h"

String const Description::analogString("Analog ");
String const Description::mbString("MB ");

bool Description::isInputPresent(int const input) const
{
	int module = getModuleForInput(input);
	uint8 moduleType = getModuleType(module);
	return ACOUSTICIO_MODULE_NOT_PRESENT != moduleType;
}

bool Description::isOutputPresent(int const output) const
{
	int module = getModuleForOutput(output);
	uint8 moduleType = getModuleType(module);
	return ACOUSTICIO_MODULE_NOT_PRESENT != moduleType;
}
