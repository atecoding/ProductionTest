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
                 String &displayedChannel,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes,
                 ValueTree &unitTree)
{
	int input;
	uint8 channel;
	uint8 data[ACOUSTICIO_TEDS_DATA_BYTES];
	uint8 expectedValue;
	bool ok = true, chan_ok;

	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_TEDS_test missing 'input' setting", false);
		return false;
	}

	input = element->getIntAttribute("input", -1);
	if (input < 0 || input > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_TEDS_test - input " + String(input) + " out of range", false);
		return false;
	}
    
    int numInputs = element->getIntAttribute("num_channels", AIOTestAdapter::NUM_INPUTS_PER_ADAPTER);
	if (true == element->hasAttribute("short"))
		numInputs = 2;

    displayedChannel = String::formatted("%d-%d", input + 1, input + numInputs);
    
	for (int j = 0; j < numInputs; j++)
	{
		chan_ok = true;
		channel = (uint8)(j + input);

        int retries = 3;
        while (retries > 0)
        {
            Result status(dev->readTEDSData(channel, data, sizeof(data)));
            if (status.failed())
            {
                msg = "Could not read TEDS data for input " + String(channel + 1) + "\n" +
                    status.getErrorMessage();
                errorCodes.add(ErrorCodes::TEDS, channel + 1);
                
                return false;
            }
            
            if (0xff != data[0])
            {
                break;
            }
            
            DBG("Read 0xff for TEDS channel " << (channel + 1));
            retries--;
        }

		expectedValue = (channel % 4) + 1;
		expectedValue |= expectedValue << 4;

		for (int i = 0; i < ACOUSTICIO_TEDS_DATA_BYTES - 1; i++)
		{
			if (data[i] != expectedValue)
			{
				msg = "*** Read unexpected TEDS data for input " + String(channel + 1) + " (value of 0x" + String::toHexString(data[i]) + " at offset " + String(i) + ")";
				ok = false;
                errorCodes.add(ErrorCodes::TEDS, channel + 1);
                chan_ok = false;
                
				break;
			}
		}

		if (chan_ok)
			msg = "Read expected TEDS data for input " + String(channel + 1);
		if (((numInputs == 2) && (j < 1)) || ((numInputs == 4) && (j < 3)))
			content->log(msg);
	}
    
    if (!ok)
		Thread::sleep(50);		// delay so things don't get hosed
	return ok;
}

#endif