#pragma once

class Description
{
public:
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
        
        AMP_OUT,
        MIKEYBUS_OUT
    };
};

class DescriptionAIO : public Description
{
public:
    virtual int getNumInputs() const ;
    virtual int getNumOutputs() const ;
    virtual String getInputName(int const input) const;
    virtual String getOutputName(int const output) const;
    virtual int getInputType(int const input) const;
    virtual int getOutputType(int const output) const;
	virtual int getModuleForInput(int const input) const;
	virtual int getModuleForOutput(int const output) const;
};

class DescriptionAMB : public Description
{
public:
    virtual int getNumInputs() const;
    virtual int getNumOutputs() const;
    virtual String getInputName(int const input) const;
    virtual String getOutputName(int const output) const;
    virtual int getInputType(int const input) const;
    virtual int getOutputType(int const output) const;
	virtual int getModuleForInput(int const input) const;
	virtual int getModuleForOutput(int const output) const;
};