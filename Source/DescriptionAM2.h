#pragma once

class DescriptionAM2 : public Description
{
public:
	DescriptionAM2(uint8 moduleTypes_, uint16 productID_, uint16 const bcdVersion_);

	virtual int getNumInputs() const;
	virtual int getNumOutputs() const;
	virtual String getInputName(int const input) const;
	virtual String getOutputName(int const output) const;
	virtual int getInputType(int const input) const;
	virtual int getOutputType(int const output) const;
	virtual int getModuleForInput(int const input) const;
	virtual int getModuleForOutput(int const output) const;
    virtual bool supportsPeakMeters() const;
    
#if JUCE_MAC
    virtual String getCoreAudioName() const;
#endif
    
private:
    uint16 productID;
};

