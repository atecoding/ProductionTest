#pragma once

bool getIntValue(XmlElement *parent,String tagname,int &val);
bool getFloatValue(XmlElement *parent,String tagname,float &val);
bool getHexValue(XmlElement *parent,String tagname,uint32 &val);
String getStringValue(XmlElement *parent,String tagname);