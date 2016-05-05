#include "base.h"
#include "Description.h"
#include "AcousticIO.h"
#include "AIOAModule.h"
#include "AIOSModule.h"
#include "AIOMModule.h"

String const Description::analogString("Analog ");
String const Description::mbString("MB ");

Description::Description(uint8 moduleTypes_, uint16 const bcdVersion_) :
bcdVersion(bcdVersion_)
{
    int firstInput = 0;
    int firstOutput = 0;
    int shift = 0;
    uint16 const interfaceModuleVersion = getInterfaceModuleVersion();
    for (int moduleNumber = 0; moduleNumber < Description::MAX_MODULES; ++moduleNumber)
    {
        uint8 moduleType = (moduleTypes_ >> shift) & 0xf;
        AIOModule* module = nullptr;
        
        switch (moduleType)
        {
            case ACOUSTICIO_ANALOG_MODULE:
                module = new AIOAModule(moduleNumber, interfaceModuleVersion, firstInput, firstOutput);
                break;
                
            case ACOUSTICIO_SPKRMON_MODULE:
                module = new AIOSModule(moduleNumber, interfaceModuleVersion, firstInput, firstOutput);
                break;
                
            case ACOUSTICIO_MIKEYBUS_MODULE:
                module = new AIOMModule(moduleNumber, interfaceModuleVersion, firstInput, firstOutput);
                break;
        }
        
        modules.add(module);
        
        shift += 4;
    }
}

uint16 const Description::getInterfaceModuleVersion() const
{
    uint16 upperBits = bcdVersion & ECHOAIO_INTERFACE_MODULE_BCDDEVICE_MASK;
    
    if (upperBits < ECHOAIO_INTERFACE_MODULE_REV2)
    {
        return ECHOAIO_INTERFACE_MODULE_REV1;
    }
    
    return upperBits;
}

bool Description::isInputPresent(int const input) const
{
    if (input < 0 || input >= getNumInputs())
        return false;
    
	int module = getModuleForInput(input);
	uint8 moduleType = getModuleType(module);
	return ACOUSTICIO_MODULE_NOT_PRESENT != moduleType;
}

bool Description::isOutputPresent(int const output) const
{
    if (output < 0 || output >= getNumOutputs())
        return false;
    
	int module = getModuleForOutput(output);
	uint8 moduleType = getModuleType(module);
	return ACOUSTICIO_MODULE_NOT_PRESENT != moduleType;
}

Array<AIOClockSource> Description::getSupportedClockSources() const
{
	Array<AIOClockSource> clockSources;

	clockSources.add(AIOClockSource(ACOUSTICIO_INTERNAL_CLOCK, "Internal clock"));

	switch (getInterfaceModuleVersion())
	{
		case ECHOAIO_INTERFACE_MODULE_REV1:
		default:
			break;

		case ECHOAIO_INTERFACE_MODULE_REV2:
			clockSources.add(AIOClockSource(ACOUSTICIO_USB_CLOCK, "USB clock"));
			break;
	}

	return clockSources;
}

uint8 Description::getModuleType(int const moduleNumber) const
{
    jassert((0 == moduleNumber) || (1 == moduleNumber));

	if (moduleNumber < 0 || moduleNumber >= MAX_MODULES)
		return 0;
    
    if (moduleNumber >= 0 && moduleNumber < modules.size() && modules[moduleNumber])
    {
        return modules[moduleNumber]->getType();
    }
    
    return 0;
}


AIOModule* Description::getModuleObject(int const moduleNumber) const
{
    jassert((0 == moduleNumber) || (1 == moduleNumber));
    
    return modules[moduleNumber];
}
