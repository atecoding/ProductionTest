#pragma once

#include "AIOModule.h"

class AIOAModule : public AIOModule
{
public:
    AIOAModule(int const moduleNumber_, uint16 const interfaceModuleVersion_,
               int &firstInput_, int &firstOutput_) :
        AIOModule(moduleNumber_, interfaceModuleVersion_,
                  firstInput_, 4,
                  firstOutput_, 2)
    {
    }
    
    virtual uint8 getType() const override
    {
        return ACOUSTICIO_ANALOG_MODULE;
    }
    
    String const getName() const override
    {
        return "AIO-A";
    }
    
    virtual bool supportsCalibration() const override
    {
        return interfaceModuleVersion >= ECHOAIO_INTERFACE_MODULE_REV2;
    }
    
protected:
};
