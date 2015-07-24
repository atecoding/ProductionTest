#pragma once

bool getIntValue(XmlElement const *parent,String tagname,int &val);
bool getFloatValue(XmlElement const *parent,String tagname,float &val);
bool getHexValue(XmlElement const *parent,String tagname,uint32 &val);
String getStringValue(XmlElement const *parent,String tagname);