#pragma once

class AIOModule
{
public:
    AIOModule(int const moduleNumber_, uint16 const interfaceModuleVersion_) :
    moduleNumber(moduleNumber_),
    interfaceModuleVersion(interfaceModuleVersion_)
    {
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
    
    Range<int> const getInputs() const
    {
        return inputChannels;
    }
    
    Range<int> const getOutputs() const
    {
        return outputChannels;
    }
    
    virtual bool supportsCalibration() const
    {
        return false;
    }
    
protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIOModule)
    
    int const moduleNumber;
    uint16 const interfaceModuleVersion;
    Range<int> inputChannels;
    Range<int> outputChannels;
};
