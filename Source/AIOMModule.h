#pragma once

#include "AIOModule.h"

class AIOMModule : public AIOModule
{
public:
    AIOMModule(int const moduleNumber_, uint16 const interfaceModuleVersion_,
               int &firstInput_, int &firstOutput_) :
        AIOModule(moduleNumber_, interfaceModuleVersion_,
                  firstInput_, 10,
                  firstOutput_, 10)
    {
    }
    
    virtual uint8 getType() const override
    {
        return ACOUSTICIO_MIKEYBUS_MODULE;
    }
    
    String const getName() const override
    {
        return "AIO-MB";
    }
    
protected:
};
