#if ACOUSTICIO_BUILD & defined(_WIN32)
extern "C"
{
#include <windows.h>
#include "hidsdi.h"
#include <setupapi.h>
#include <dbt.h>
}
#include "../base.h"
#include "../AIOTestAdapter.h"

#pragma comment (lib, "Hid")

AIOTestAdapter::AIOTestAdapter() :
deviceHandle(INVALID_HANDLE_VALUE),
readHandle(INVALID_HANDLE_VALUE),
writeHandle(INVALID_HANDLE_VALUE),
maxRequestTicks(0)
{
}

AIOTestAdapter::~AIOTestAdapter()
{
	close();
}


bool AIOTestAdapter::open()
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

			SetupDiGetDeviceInterfaceDetail
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
			SetupDiGetDeviceInterfaceDetail
				(hDevInfo,
				&devInfoData,
				detailData,
				detailDataBlock.getSize(),
				&bytesNeeded,
				NULL);

			deviceHandle = CreateFile
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
				BOOL attributes_result = HidD_GetAttributes(deviceHandle,&attributes);
				if (attributes_result && ECHO_VENDOR_ID == attributes.VendorID)
				{
					bool match = false;

					switch (attributes.ProductID)
					{
#if SUPPORT_TEST_ADAPTER_V200
					case ECHO_HID_TESTER_PRODUCT_ID_V100:
					case ECHO_HID_TESTER_PRODUCT_ID_V200:
#endif
					case ECHO_HID_TESTER_PRODUCT_ID_V210:
						DBG("Test adapter found");
						match = true;
						productIDs.add(attributes.ProductID);
						break;
					}

					if (match)
					{
						readHandle = CreateFile
							(detailData->DevicePath,
							GENERIC_READ,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							(LPSECURITY_ATTRIBUTES)NULL,
							OPEN_EXISTING,
							FILE_FLAG_OVERLAPPED,
							NULL);

						writeHandle = CreateFile
							(detailData->DevicePath,
							GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							(LPSECURITY_ATTRIBUTES)NULL,
							OPEN_EXISTING,
							FILE_FLAG_OVERLAPPED,
							NULL);
						break;
					}
				}

				CloseHandle(deviceHandle);
				deviceHandle = INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			done = true;
		}

		index++;
	} while (false == done);

	SetupDiDestroyDeviceInfoList(hDevInfo);

	return deviceHandle != INVALID_HANDLE_VALUE && readHandle != INVALID_HANDLE_VALUE && writeHandle != INVALID_HANDLE_VALUE;
}

void AIOTestAdapter::close()
{
	if (readHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(readHandle);
		readHandle = INVALID_HANDLE_VALUE;
	}

	if (writeHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(writeHandle);
		writeHandle = INVALID_HANDLE_VALUE;
	}

	if (deviceHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(deviceHandle);
		deviceHandle = INVALID_HANDLE_VALUE;
	}
}

int AIOTestAdapter::write(uint8 byte)
{
	uint8 data[2];
	data[0] = 0;
	data[1] = byte;

	if (INVALID_HANDLE_VALUE == writeHandle)
		return 0;

	HidD_SetOutputReport(writeHandle, data, sizeof(data));
	Thread::sleep(100);
	BOOLEAN result = HidD_SetOutputReport(writeHandle, data, sizeof(data));		// send twice because of buffering somewhere -- quien sabes?

	return result;
}

int AIOTestAdapter::read(Array<uint16> &data)
{
	uint8 temp[NUM_INPUTS_PER_ADAPTER * 2 + 1];

	data.clearQuick();

	if (INVALID_HANDLE_VALUE == readHandle)
		return 0;

	zerostruct(temp);
	BOOLEAN result = HidD_GetInputReport(readHandle, temp, sizeof(temp));
	if (0 == result)
	{
		return 0;
	}

	uint16 *source = (uint16 *)(temp + 1);
	for (int i = 0; i < NUM_INPUTS_PER_ADAPTER; ++i)
	{
		data.add(source[i]);
	}
	
	return NUM_INPUTS_PER_ADAPTER;
}

#endif