#include "base.h"

bool getIntValue(XmlElement *parent,String tagname,int &val)
{
	XmlElement *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		val = xe->getAllSubText().getIntValue();
		return true;
	}

	return false;
}

bool getFloatValue(XmlElement *parent,String tagname,float &val)
{
	XmlElement *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		val = (float) xe->getAllSubText().getDoubleValue();
		return true;
	}

	return false;
}


bool getHexValue(XmlElement *parent,String tagname,uint32 &val)
{
	XmlElement *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		val = xe->getAllSubText().getHexValue32();
		return true;
	}

	return false;
}


String getStringValue(XmlElement *parent,String tagname)
{
	XmlElement *xe;

	xe = parent->getChildByName(tagname);
	if (xe)
	{
		return xe->getAllSubText().trim();
	}

	return String::empty;
}