#pragma once

#include "AIOModule.h"

class AIOAModule : public AIOModule
{
public:
    AIOAModule(int const moduleNumber_, Range<int> &inputChannels_, Range<int> &outputChannels_, uint16 const interfaceModuleVersion_) :
    AIOModule(moduleNumber_, interfaceModuleVersion_)
    {
        int numInputs = 4;
        int numOutputs = 2;
        
        inputChannels_.setLength(numInputs);
        outputChannels_.setLength(numOutputs);
        inputChannels = inputChannels_;
        outputChannels = outputChannels_;
        inputChannels_ += numInputs;
        outputChannels_ += numOutputs;
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
