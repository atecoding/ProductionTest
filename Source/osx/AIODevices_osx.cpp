//
//  AIODevices.cpp
//  AppleBox
//
//  Created by Matthew Gonzalez on 11/6/13.
//
//

#if 0

#include "base.h"
#if JUCE_WIN32 && ACOUSTICIO_BUILD
#include "AIODevices.h"
#pragma comment (lib, "tusbaudioapi")

AIODevices::AIODevices()
{
    enumerate();
}

AIODevices::~AIODevices()
{
}

void AIODevices::enumerate()
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_EnumerateDevices();
	if (TSTATUS_SUCCESS != status)
		return;

	unsigned count = TUSBAUDIO_GetDeviceCount();

	for (unsigned index = 0; index < count; index++)
	{
		TUsbAudioHandle handle;

		status = TUSBAUDIO_OpenDeviceByIndex( index, &handle);
		if (TSTATUS_SUCCESS != status)
			continue;

		TUsbAudioDeviceRunMode mode;

		status = TUSBAUDIO_GetDeviceUsbMode(handle,&mode);
		if (TSTATUS_SUCCESS == status && DeviceRunMode_APP == mode)
		{
			devices.add(new AIODevice (handle));
		}
		else
		{
			TUSBAUDIO_CloseDevice(handle);
		}
	}
}
#endif

#endif

