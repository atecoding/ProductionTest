#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"

#define CUR 1

bool RunTEDSTest(XmlElement const *element, ehw *dev, String &msg, int &displayedInput)
{
	int attribute;
	uint8 channel;
	uint8 data[ACOUSTICIO_TEDS_DATA_BYTES];
	uint8 expectedValue;
	
	displayedInput = -1;

	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_TEDS_test missing 'input' setting", false);
		return false;
	}

	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_TEDS_test - input " + String(attribute) + " out of range", false);
		return false;
	}
	channel = (uint8)attribute;
	displayedInput = attribute + 1;

	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestGet(dev->GetNativeHandle(),
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_TEDS_DATA_CONTROL,
		channel,
		(void *)data,
		sizeof(data),
		NULL,
		1000);
	if (TSTATUS_SUCCESS != status)
	{
		msg = "Could not read TEDS data for input " + String(displayedInput);
		return false;
	}

	expectedValue = (channel % 4) + 1;
	expectedValue |= expectedValue << 4;

	for (int i = 0; i < ACOUSTICIO_TEDS_DATA_BYTES - 1; i++)
	{
		if (data[i] != expectedValue)
		{
			msg = "Read unexpected TEDS data for input " + String(displayedInput) + " (value of 0x" + String::toHexString(data[i]) + " at offset " + String(i) + ")";
			return false;
		}
	}

	msg = "Read expected TEDS data for input " + String(displayedInput);
	return true;
}

#endif