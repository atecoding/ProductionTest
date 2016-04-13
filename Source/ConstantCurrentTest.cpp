#if ACOUSTICIO_BUILD
#include "base.h"
#include "xml.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"

class ehw;
class Content;

bool RunCCVoltageTest(XmlElement const *element,
                      ehw *dev,
                      String &msg,
                      String &displayedChannel,
                      AIOTestAdapter &testAdapter,
                      Content *content,
                      ErrorCodes &errorCodes,
                      ValueTree &unitTree)
{
	int attribute;
    int numInputs;
	uint8 channel;
	float minimum, maximum, volts;
	bool ok;

	//
	// Read XML parameters
	//
	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_voltage_test missing 'input' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_voltage_test - input " + String(attribute) + " out of range", false);
		return false;
	}
	channel = (uint8)attribute;
	channel &= ~3;
	displayedChannel = String(channel + 1);
    
    numInputs = element->getIntAttribute("num_channels", AIOTestAdapter::NUM_INPUTS_PER_ADAPTER);
	if (true == element->hasAttribute("short"))
		numInputs = 2;

	ok = getFloatValue((XmlElement *)element, T("minimum"), minimum);
	if (!ok)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_voltage_test missing 'minimum' setting", false);
		return false;
	}
	if (minimum < 0.0f || minimum > 25.0f)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_voltage_test - minimum " + String(minimum,1) + " out of range", false);
		return false;
	}

	ok = getFloatValue((XmlElement *)element, T("maximum"), maximum);
	if (!ok)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_voltage_test missing 'maximum' setting", false);
		return false;
	}
	if (maximum < 0.0f || maximum > 25.0f)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_voltage_test - maximum " + String(maximum,1) + " out of range", false);
		return false;
	}

	//
	// Read values from test adapter
	//
    Array<uint16> values;

	Thread::sleep(10);		// wait for ADCs to convert
	testAdapter.read(values);

	//
	// Return the results
	//
	bool pass = true;

	if(maximum < 2.0f)
		msg = "Mic Supply off voltage test, channels " + String(channel + 1) + "-" + String(channel + 4) + "\n";
	else
		msg = "Mic Supply on voltage test, channels " + String(channel + 1) + "-" + String(channel + 4) + "\n";

	for (int i = 0; i < numInputs; i++)
	{
		uint16 value = values[i];
		volts = 0.6f + (float)value / 1966.0f;
		if (volts < minimum || volts > maximum)
		{
			msg += "    Channel " + String(i + channel + 1) + ": " + String(volts,1) + "V out of range\n";
			pass = false;
            
            errorCodes.add( ErrorCodes::MIC_SUPPLY_VOLTAGE, channel + 1 + i);
		}
		else
		{
			msg += "    Channel " + String(i + channel + 1) + ": " + String(volts,1) + "V OK\n";
		}
	}

	return pass;
}

bool RunCCCurrentTest(XmlElement const *element,
                      ehw *dev,
                      String &msg,
                      String &displayedChannel,
                      AIOTestAdapter &testAdapter,
                      Content *content,
                      ErrorCodes &errorCodes,
                      ValueTree &unitTree)
{
	int attribute;
	uint8 channel;
	float minimum, maximum, current;
	bool ok;

	//
	// Read XML parameters
	//
	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_test missing 'input' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_test - input " + String(attribute) + " out of range", false);
		return false;
	}
	channel = (uint8)attribute;
	channel &= ~3;
	displayedChannel = String(channel + 1);
    
    int numInputs = element->getIntAttribute("num_channels", AIOTestAdapter::NUM_INPUTS_PER_ADAPTER);
	if (true == element->hasAttribute("short"))
		numInputs = 2;

	ok = getFloatValue((XmlElement *)element, T("minimum"), minimum);
	if (!ok)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_test missing 'minimum' setting", false);
		return false;
	}
	if (minimum < 0 || minimum > 10)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_test - minimum " + String(minimum) + " out of range", false);
		return false;
	}

	ok = getFloatValue((XmlElement *)element, T("maximum"), maximum);
	if (!ok)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_test missing 'maximum' setting", false);
		return false;
	}
	if (maximum < 0 || maximum > 10)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_mic_supply_test - maximum " + String(maximum) + " out of range", false);
		return false;
	}

	//
	// Read values from test adapter
	//
	Array<uint16> values;

	Thread::sleep(10);		// wait for ADCs to convert
	testAdapter.read(values);

	//
	// Return the results
	//
	bool pass = true;

	msg = "Mic Supply current test, channels " + String(channel + 1) + "-" + String(channel + 4) + "\n";
	for (int i = 0; i < numInputs; i++)
	{
		uint16 value = values[i];
		current = (float)value / 3933.0f;

		if (current < minimum || current > maximum)
		{
			msg += "    Channel " + String(i + channel + 1) + ": " + String(current,1) + "ma out of range\n";
			pass = false;
			errorCodes.add( ErrorCodes::MIC_SUPPLY_CURRENT, channel + 1 + i);
		}
		else
		{
			msg += "    Channel " + String(i + channel + 1) + ": " + String(current,1) + "ma OK\n";
		}
	}

	return pass;
}

#endif