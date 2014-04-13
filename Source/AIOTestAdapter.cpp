#if ACOUSTICIO_BUILD

#include "base.h"
#include "AIOTestAdapter.h"

#pragma comment (lib, "UsbHidApi")

AIOTestAdapter::AIOTestAdapter() :
opened(0)
{
}

AIOTestAdapter::~AIOTestAdapter()
{
	close();
}


int AIOTestAdapter::open()
{
	opened = api.Open(ECHO_VENDOR_ID,
		ECHO_HID_TESTER_PRODUCT_ID,
		nullptr, nullptr, nullptr, 0);
	return opened;
}

void AIOTestAdapter::close()
{
	if (opened)
	{
		api.CloseRead();
		api.CloseWrite();
		opened = 0;
	}
}

int AIOTestAdapter::write(uint8 byte)
{
	uint8 data[2];
	data[0] = 0;
	data[1] = byte;
	return api.Write(data);
}

int AIOTestAdapter::read(uint16 data[4])
{
	uint8 temp[9];
	return api.Read(temp);
}

#endif