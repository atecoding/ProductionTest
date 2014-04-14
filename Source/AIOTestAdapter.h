
#pragma once

//#include "UsbHidApi.h"


class AIOTestAdapter
{
public:
	AIOTestAdapter();
	~AIOTestAdapter();

	int open();
	void close();
	int write(uint8 byte);
	int read(uint16 data[4]);

	static void foo();

protected:
	/*CUsbHidApi api;*/
	int opened;

	enum
	{
		ECHO_VENDOR_ID = 0x40f,
		ECHO_HID_TESTER_PRODUCT_ID = 0x200
	};
};

