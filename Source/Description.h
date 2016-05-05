#pragma once

#include "AIOClockSource.h"
#include "AIOModule.h"

class Description
{
public:
    Description(uint8 moduleTypes_, uint16 const bcdVersion_);
    
    virtual ~Description()
    {
    }
    
    virtual int getNumInputs() const
    {
        return 0;
    }
    
    virtual int getNumOutputs() const
    {
        return 0;
    }
    
#if JUCE_MAC
    virtual String getCoreAudioName() const
    {
        return String::empty;
    }
#endif
    
    virtual String getInputName(int const /*input*/) const
    {
        return String::empty;
    }

    virtual String getOutputName(int const /*output*/) const
    {
        return String::empty;
    }
    
    virtual int getInputType(int const /*input*/) const
    {
        return -1;
    }
    
    virtual int getOutputType(int const /*output*/) const
    {
        return -1;
    }

	virtual int getModuleForInput(int const /*input*/) const
	{
		return 0;
	}
	
	virtual int getModuleForOutput(int const /*output*/) const
	{
		return 0;
	}
    
    virtual AIOModule* getModuleObject(int const moduleNumber) const;
    
    virtual bool supportsPeakMeters() const
    {
        return true;
    }

	uint16 const getInterfaceModuleVersion() const;

    enum
    {
        TEDS_IN = 0,
        MIKEYBUS_IN,
		SPKRMON_VOLTAGE_IN,
		SPKRMON_CURRENT_IN,
        
        AMP_OUT,
        MIKEYBUS_OUT
    };
    
    enum
    {
        MAX_MODULES = 2
    };

    uint8 getModuleTypes() const
    {
        return moduleTypes;
    }
    
    uint8 getModuleType(int const moduleNumber) const;

	bool isInputPresent(int const input) const;
	bool isOutputPresent(int const output) const;

	virtual Array<AIOClockSource> getSupportedClockSources() const;

protected:
    OwnedArray<AIOModule> modules;
    uint8 const moduleTypes;
	uint16 const bcdVersion;

	const static String analogString;
	const static String mbString;
};

