#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "errorbits.h"
//#include "AIODevice.h"

#define CUR 1

bool RunInputTest(XmlElement const *element, ehw *dev, String &msg, int &displayedInput, Content *content)
{
	int attribute;
	uint8 channel;
	uint8 data[ACOUSTICIO_TEDS_DATA_BYTES];
	uint8 expectedValue;
	bool ok = true, test_ok = true;
//	XmlElement cc_off = " "

	displayedInput = -1;

	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_input_test missing 'input' setting", false);
		return false;
	}

	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_input_test - input " + String(attribute) + " out of range", false);
		return false;
	}
	channel = (uint8)attribute;
	displayedInput = channel + 1;

	dev->setConstantCurrent(channel, true);	// turn on constant current for channel
	ok = AlertWindow::showOkCancelBox(AlertWindow::NoIcon, "Production Test", "Is the LED on at the correct brightness?\n\nOptional: VDC approx 21.5V, Current approx 4mA", T("Yes"), T("No"));
	if (ok)
		msg = "Constant Current on input " + String(displayedInput) + ": ok";
	else
		msg = "Constant Current on input " + String(displayedInput) + ": failed";
	content->log(msg);
	test_ok &= ok;

	dev->setConstantCurrent(channel, false);	// turn off constant current for channel
	ok = AlertWindow::showOkCancelBox(AlertWindow::NoIcon, "Production Test", "Is the LED off?", T("Yes"), T("No"));
	if (ok)
		msg = "Constant Current off input " + String(displayedInput) + ": ok";
	else
		msg = "Constant Current off input " + String(displayedInput) + ": failed";
	content->log(msg);
	test_ok &= ok;

	ok = true;
	
#if 0
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
#else
    Result status(dev->readTEDSData(channel, data, sizeof(data)));
    if (status.failed())
    {
        msg = "Could not read TEDS data for input " + String(displayedInput) + "\n" +
        status.getErrorMessage();
        return false;
    }
#endif

	expectedValue = 0x22;

	for (int i = 0; i < ACOUSTICIO_TEDS_DATA_BYTES - 1; i++)
	{
		if (data[i] != expectedValue)
		{
			msg = "Read unexpected TEDS data for input " + String(displayedInput) + " (value of 0x" + String::toHexString(data[i]) + " at offset " + String(i) + ")";
			ok = false;
			break;
		}
	}

	if (ok)
		msg = "Read expected TEDS data for input " + String(displayedInput);
	if (!ok)
		Thread::sleep(50);		// delay so things don't get hosed
	test_ok &= ok;
	return test_ok;
}

bool RunTEDSTest(XmlElement const *element, ehw *dev, String &msg, int &displayedInput, Content *content, int &errorBit)
{
	int attribute;
	uint8 channel;
	uint8 data[ACOUSTICIO_TEDS_DATA_BYTES];
	uint8 expectedValue;
	bool ok = true;

	errorBit = 0;

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
	for (int j = 0; j < 4; j++)
	{
		ok = true;
		channel = (uint8)(j + attribute);
		displayedInput = channel + 1;

        Result status(dev->readTEDSData(channel,data, sizeof(data)));
        if (status.failed())
        {
            msg = "Could not read TEDS data for input " + String(displayedInput) + "\n" +
                status.getErrorMessage();
			errorBit |= TEDS_ERROR_INDEX << channel;
            return false;
        }

		expectedValue = (channel % 4) + 1;
		expectedValue |= expectedValue << 4;

		for (int i = 0; i < ACOUSTICIO_TEDS_DATA_BYTES - 1; i++)
		{
			if (data[i] != expectedValue)
			{
				msg = "*** Read unexpected TEDS data for input " + String(displayedInput) + " (value of 0x" + String::toHexString(data[i]) + " at offset " + String(i) + ")";
				ok = false;
				errorBit |= TEDS_ERROR_INDEX << channel;
				break;
			}
		}

		if (ok)
			msg = "Read expected TEDS data for input " + String(displayedInput);
		if (j < 3)
			content->log(msg);
	}
    
    ok = errorBit == 0;
	if (!ok)
		Thread::sleep(50);		// delay so things don't get hosed
	return ok;
}

#endif