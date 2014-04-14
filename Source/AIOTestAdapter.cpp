#if ACOUSTICIO_BUILD

#include "base.h"
#include "AIOTestAdapter.h"

extern "C" 
{
#include "hidsdi.h"
#include <setupapi.h>
#include <dbt.h>
}
//#pragma comment (lib, "UsbHidApi")
#pragma comment (lib, "Hid")

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
	//opened = api.Open(ECHO_VENDOR_ID,
	//	ECHO_HID_TESTER_PRODUCT_ID,
	//	nullptr, nullptr, nullptr, 0);

	return 0;
}

void AIOTestAdapter::foo()
{
	GUID HidGuid;
	SP_DEVICE_INTERFACE_DATA devInfoData;
	int index = 0;
	BOOL result;
	bool done = false;
	MemoryBlock detailDataBlock(1024);

	HidD_GetHidGuid(&HidGuid);

	HANDLE hDevInfo = SetupDiGetClassDevs
		(&HidGuid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	devInfoData.cbSize = sizeof(devInfoData);

	do
	{
		result = SetupDiEnumDeviceInterfaces
			(hDevInfo,
			0,
			&HidGuid,
			index,
			&devInfoData);
		if (result)
		{ 
			PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
			ULONG bytesNeeded;

			BOOL r1 = SetupDiGetDeviceInterfaceDetail
				(hDevInfo,
				&devInfoData,
				NULL,
				0,
				&bytesNeeded,
				NULL);

			if (bytesNeeded > detailDataBlock.getSize())
			{
				detailDataBlock.setSize(bytesNeeded);
			}
			
			detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)detailDataBlock.getData();
			detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			BOOL r2 = SetupDiGetDeviceInterfaceDetail
				(hDevInfo,
				&devInfoData,
				detailData,
				detailDataBlock.getSize(),
				&bytesNeeded,
				NULL);

			HANDLE deviceHandle = CreateFile
				(detailData->DevicePath,
				0,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (deviceHandle != INVALID_HANDLE_VALUE)
			{
				HIDD_ATTRIBUTES attributes;

				zerostruct(attributes);
				attributes.Size = sizeof(attributes);
				BOOL r3 = HidD_GetAttributes(deviceHandle,&attributes);

				CloseHandle(deviceHandle);
			}
		}
		else
		{
			done = true;
		}

		index++;
	} while (false == done);

	SetupDiDestroyDeviceInfoList(hDevInfo);
}

void AIOTestAdapter::close()
{
	if (opened)
	{
		//api.CloseRead();
		//api.CloseWrite();
		opened = 0;
	}
}

int AIOTestAdapter::write(uint8 byte)
{
	//uint8 data[2];
	//data[0] = 0;
	//data[1] = byte;
	//return api.Write(data);
	return 0;
}

int AIOTestAdapter::read(uint16 data[4])
{
	//uint8 temp[9];
	//int count = api.Read(temp);
	//if (sizeof(temp) == count)
	//{
	//	uint16 *source = (uint16 *)temp + 1;
	//	for (int i = 0; i < 4; ++i)
	//	{
	//		data[i] = source[i];
	//	}
	//}
	//return count;

	return 0;
}

#endif