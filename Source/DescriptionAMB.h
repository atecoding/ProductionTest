#pragma once

class DescriptionAMB : public Description
{
public:
	DescriptionAMB(uint8 moduleTypes_);

	virtual int getNumInputs() const;
	virtual int getNumOutputs() const;
	virtual String getInputName(int const input) const;
	virtual String getOutputName(int const output) const;
	virtual int getInputType(int const input) const;
	virtual int getOutputType(int const output) const;
	virtual int getModuleForInput(int const input) const;
	virtual int getModuleForOutput(int const output) const;
};
