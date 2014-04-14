#if ACOUSTICIO_BUILD
#include "base.h"
#include "AIOTestAdapter.h"

bool RunConstantCurrentTest(XmlElement const *element, String &msg, int &displayedInput, AIOTestAdapter &testAdapter)
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
	if (attribute < 0 || attribute > 0xffff)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test - minimum " + String(attribute) + " out of range", false);
		return false;
	}
	minimum = (uint16)attribute;


	if (false == element->hasAttribute("maximum"))
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test missing 'maximum' setting", false);
		return false;
	}
	attribute = element->getIntAttribute("maximum", -1);
	if (attribute < 0 || attribute > 0xffff)
	{
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_constant_current_test - maximum " + String(attribute) + " out of range", false);
		return false;
	}
	maximum = (uint16)attribute;

	//
	// Read values from test adapter
	//
	uint16 values[4];

	testAdapter.read(values);
	testAdapter.read(values);
	testAdapter.read(values);

	//
	// Return the results
	//
	bool pass = true;

	msg = "Constant current test, channels " + String(displayedInput) + "-" + String(displayedInput + 3) + "\n";
	for (int i = 0; i < 4; i++)
	{
		uint16 value = values[i];
		if (value < minimum || value > maximum)
		{
			msg += "Channel " + String(i + displayedInput) + ": " + String::toHexString(value) + " out of range\n";
			pass = false;
		}
		else
		{
			msg += "Channel " + String(i + displayedInput) + ": " + String::toHexString(value) + " OK\n";
		}
	}
	
	return pass;
}

#endif