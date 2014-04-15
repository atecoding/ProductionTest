#if ACOUSTICIO_BUILD
#include "base.h"
#include "AIOTestAdapter.h"

bool RunCCVoltageTest(XmlElement const *element, String &msg, int &displayedInput, AIOTestAdapter &testAdapter)
{
	int attribute;
	float fattribute;
	String sattribute;
	uint8 channel;
	float minimum, maximum;

	//
	// Read XML parameters
	//
	displayedInput = -1;

	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_CC_voltage_test missing 'input' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_CC_voltage_test - input " + String(attribute) + " out of range", false);
		return false;
	}
	channel = (uint8)attribute;
	channel &= ~3;
	displayedInput = channel + 1;

	if (false == element->hasAttribute("minimum"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_CC_voltage_test missing 'minimum' setting", false);
		return false;
	}
//	attribute = element->getIntAttribute("minimum", -1);
	sattribute = element->getStringAttribute("minimum");
	fattribute = String::getFloatValue(sattribute);
	if (fattribute < 0.0f || fattribute > 25.0f)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_CC_voltage_test - minimum " + String(fattribute) + " out of range", false);
		return false;
	}
	minimum = fattribute;


	if (false == element->hasAttribute("maximum"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_CC_voltage_test missing 'maximum' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("maximum", -1);
	if (attribute < 0 || attribute > 25)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_CC_voltage_test - maximum " + String(attribute) + " out of range", false);
		return false;
	}
	maximum = (float)attribute;

	//
	// Read values from test adapter
	//
	uint16 values[4];

	Thread::sleep(10);		// wait for ADCs to convert
	testAdapter.read(values);

	//
	// Return the results
	//
	bool pass = true;

	if(maximum < 2.0f)
		msg = "Constant Current off voltage test, channels " + String(displayedInput) + "-" + String(displayedInput + 3) + "\n";
	else
		msg = "Constant Current on voltage test, channels " + String(displayedInput) + "-" + String(displayedInput + 3) + "\n";

	for (int i = 0; i < 4; i++)
	{
		uint16 value = values[i];
		float volts = 0.6f + (float)value / 1966.0f;
		if (volts < minimum || volts > maximum)
		{
			msg += "    Channel " + String(i + displayedInput) + ": " + String(volts,1) + " out of range\n";
			pass = false;
		}
		else
		{
			msg += "    Channel " + String(i + displayedInput) + ": " + String(volts,1) + " OK\n";
		}
	}

	return pass;
}

bool RunCCCurrentTest(XmlElement const *element, String &msg, int &displayedInput, AIOTestAdapter &testAdapter)
{
	int attribute;
	uint8 channel;
	uint16 minimum, maximum;

	//
	// Read XML parameters
	//
	displayedInput = -1;

	if (false == element->hasAttribute("input"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test missing 'input' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test - input " + String(attribute) + " out of range", false);
		return false;
	}
	channel = (uint8)attribute;
	channel &= ~3;
	displayedInput = channel + 1;

	if (false == element->hasAttribute("minimum"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test missing 'minimum' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("minimum", -1);
	if (attribute < 0 || attribute > 10)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test - minimum " + String(attribute) + " out of range", false);
		return false;
	}
	minimum = (float)attribute;


	if (false == element->hasAttribute("maximum"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test missing 'maximum' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("maximum", -1);
	if (attribute < 0 || attribute > 10)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test - maximum " + String(attribute) + " out of range", false);
		return false;
	}
	maximum = (float)attribute;

	//
	// Read values from test adapter
	//
	uint16 values[4];

	Thread::sleep(10);		// wait for ADCs to convert
	testAdapter.read(values);

	//
	// Return the results
	//
	bool pass = true;

	msg = "CC current test, channels " + String(displayedInput) + "-" + String(displayedInput + 3) + "\n";
	for (int i = 0; i < 4; i++)
	{
		uint16 value = values[i];
		float current = (float)value / 3933.0f;


		if (value < minimum || value > maximum)
		{
			msg += "    Channel " + String(i + displayedInput) + ": " + String(current,1) + " out of range\n";
			pass = false;
		}
		else
		{
			msg += "    Channel " + String(i + displayedInput) + ": " + String(current,1) + " OK\n";
		}
	}

	return pass;
}

#endif