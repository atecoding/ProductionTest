#pragma once

#include "AIOModule.h"

class AIOSModule : public AIOModule
{
public:
    AIOSModule(int const moduleNumber_, uint16 const interfaceModuleVersion_,
               int &firstInput_, int &firstOutput_) :
        AIOModule(moduleNumber_, interfaceModuleVersion_,
                  firstInput_, 4,
                  firstOutput_, 2)
    {
    }
    
    virtual uint8 getType() const override
    {
        return ACOUSTICIO_SPKRMON_MODULE;
    }
    
    String const getName() const override
    {
        return "AIO-S";
    }

	virtual bool supportsCalibration() const override
	{
		return true;
	}
    
protected:
};
