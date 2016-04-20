#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

USBSyncTest::USBSyncTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
	ok &= getFloatValue(xe, T("min_level_db"), min_level_db);
	ok &= getFloatValue(xe, T("max_level_db"), max_level_db);
}


USBSyncTest::~USBSyncTest()
{
}


bool USBSyncTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
	bool pass = true;

	msg = String::formatted("USB Sync Sample rate %d Hz", sample_rate_check);
	return pass;
}

