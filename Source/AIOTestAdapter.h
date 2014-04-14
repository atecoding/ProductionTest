
#pragma once


class AIOTestAdapter
{
public:
	AIOTestAdapter();
	~AIOTestAdapter();

	bool open();
	void close();
	int write(uint8 byte);
	int read(uint16 data[4]);

protected:
	HANDLE deviceHandle;
	HANDLE readHandle;
	HANDLE writeHandle;

	enum
	{
		ECHO_VENDOR_ID = 0x40f,
		ECHO_HID_TESTER_PRODUCT_ID = 0x200
	};
};

