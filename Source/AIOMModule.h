#pragma once

#include "AIOModule.h"

class AIOMModule : public AIOModule
{
public:
    AIOMModule(int const moduleNumber_, Range<int> &inputChannels_, Range<int> &outputChannels_, uint16 const interfaceModuleVersion_) :
    AIOModule(moduleNumber_, interfaceModuleVersion_)
    {
        int numInputs = 10;
        int numOutputs = 10;
        
        inputChannels_.setLength(numInputs);
        outputChannels_.setLength(numOutputs);
        inputChannels = inputChannels_;
        outputChannels = outputChannels_;
        inputChannels_ += numInputs;
        outputChannels_ += numOutputs;
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
