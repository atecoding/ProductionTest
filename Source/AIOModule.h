#pragma once

#include "USBDevice.h"

class AIOModule
{
public:
    AIOModule(int const moduleNumber_, uint16 const interfaceModuleVersion_,
              int &firstInput_, int const numInputs_,
              int &firstOutput_, int const numOutputs_) :
    moduleNumber(moduleNumber_),
    interfaceModuleVersion(interfaceModuleVersion_)
    {
        firstInput = firstInput_;
        numInputs = numInputs_;
        firstInput_ += numInputs_;
        
        firstOutput = firstOutput_;
        numOutputs = numOutputs_;
        firstOutput_ += numOutputs;
    }
    
    virtual ~AIOModule()
    {
    }
    
    int getModuleNumber() const
    {
        return moduleNumber;
    }
    
    virtual uint8 getType() const = 0;
    virtual String const getName() const = 0;
    
    int const getFirstInput() const
    {
        return firstInput;
    }
    
    int const getNumInputs() const
    {
        return numInputs;
    }
    
    int const getLastInput() const
    {
        return firstInput + numInputs - 1;
    }
    
    int const getFirstOutput() const
    {
        return firstOutput;
    }
    
    int const getNumOutputs() const
    {
        return numOutputs;
    }
    
    int const getLastOutput() const
    {
        return firstOutput + numOutputs - 1;
    }
    
    virtual bool supportsCalibration() const
    {
        return false;
    }
    
protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIOModule)
    
    int const moduleNumber;
    uint16 const interfaceModuleVersion;
    int firstInput;
    int numInputs;
    int firstOutput;
    int numOutputs;
};
