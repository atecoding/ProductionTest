#pragma once

struct AIOClockSource
{
	AIOClockSource() :
		controlValue(-1)
	{

	}
	AIOClockSource(int controlValue_, String name_) :
		controlValue(controlValue_),
		name(name_)
	{
	}

	int controlValue;
	String name;
};
