#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"

bool RunTEDSTest(XmlElement const *element,
                 ehw *dev,
                 String &msg,
                 int &displayedInput,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes,
                 ValueTree &unitTree)
{
	int attribute;
	uint8 channel;
	uint8 data[ACOUSTICIO_TEDS_DATA_BYTES];
	uint8 expectedValue;
	bool ok = true, chan_ok;

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
    
    int numInputs = element->getIntAttribute("num_chanels", AIOTestAdapter::NUM_INPUTS);
	if (true == element->hasAttribute("short"))
		numInputs = 2;

	for (int j = 0; j < numInputs; j++)
	{
		chan_ok = true;
		channel = (uint8)(j + attribute);
		displayedInput = channel + 1;

        int retries = 3;
        while (retries > 0)
        {
            Result status(dev->readTEDSData(channel, data, sizeof(data)));
            if (status.failed())
            {
                msg = "Could not read TEDS data for input " + String(displayedInput) + "\n" +
                    status.getErrorMessage();
                errorCodes.add(ErrorCodes::TEDS, displayedInput);
                
                return false;
            }
            
            if (0xff != data[0])
            {
                break;
            }
            
            DBG("Read 0xff for TEDS channel " << displayedInput);
            retries--;
        }

		expectedValue = (channel % 4) + 1;
		expectedValue |= expectedValue << 4;

		for (int i = 0; i < ACOUSTICIO_TEDS_DATA_BYTES - 1; i++)
		{
			if (data[i] != expectedValue)
			{
				msg = "*** Read unexpected TEDS data for input " + String(displayedInput) + " (value of 0x" + String::toHexString(data[i]) + " at offset " + String(i) + ")";
				ok = false;
                errorCodes.add(ErrorCodes::TEDS, displayedInput);
                chan_ok = false;
                
				break;
			}
		}

		if (chan_ok)
			msg = "Read expected TEDS data for input " + String(displayedInput);
		if (((numInputs == 2) && (j < 1)) || ((numInputs == 4) && (j < 3)))
			content->log(msg);
	}
    
	if (numInputs == 2)
		displayedInput = -255;
	else
		displayedInput -= 3;

	if (!ok)
		Thread::sleep(50);		// delay so things don't get hosed
	return ok;
}

#endif