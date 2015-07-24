#include "base.h"

bool getIntValue(XmlElement const *parent,String tagname,int &val)
{
	XmlElement const *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		val = xe->getAllSubText().getIntValue();
		return true;
	}

	return false;
}

bool getFloatValue(XmlElement const *parent,String tagname,float &val)
{
	XmlElement const *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		val = (float) xe->getAllSubText().getDoubleValue();
		return true;
	}

	return false;
}


bool getHexValue(XmlElement const *parent,String tagname,uint32 &val)
{
	XmlElement const *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		val = xe->getAllSubText().getHexValue32();
		return true;
	}

	return false;
}


String getStringValue(XmlElement const *parent,String tagname)
{
	XmlElement const *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		return xe->getAllSubText().trim();
	}

	return String::empty;
}