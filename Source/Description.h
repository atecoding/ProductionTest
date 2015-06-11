#pragma once

class Description
{
public:
	Description(uint8 moduleTypes_)
	{
		moduleTypes[0] = moduleTypes_ & 0xf;
		moduleTypes[1] = moduleTypes_ >> 4;
        
        DBG("Description " << moduleTypes[0] << " " << moduleTypes[1]);
	}

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

    enum
    {
        TEDS_IN = 0,
        MIKEYBUS_IN,
		SPKRMON_VOLTAGE_IN,
		SPKRMON_CURRENT_IN,
        
        AMP_OUT,
        MIKEYBUS_OUT
    };

	uint8 getModuleType(int const module) const
	{
		return moduleTypes[module];
	}

	bool isInputPresent(int const input) const;
	bool isOutputPresent(int const output) const;

protected:
	uint8 moduleTypes[2];

	const static String analogString;
	const static String mbString;
};

#include "DescriptionAIO.h"
#include "DescriptionAMB.h"

