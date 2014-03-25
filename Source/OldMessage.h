#pragma once

class OldMessage : public Message
{
public:
	OldMessage(int intParameter1_, int intParameter2_, int intParameter3_, void* pointerParameter_):
		intParameter1(intParameter1_),
		intParameter2(intParameter2_),
		intParameter3(intParameter3_),
		pointerParameter(pointerParameter_)
		{
		}
	int intParameter1;
	int intParameter2;
	int intParameter3;
	void* pointerParameter;
};
