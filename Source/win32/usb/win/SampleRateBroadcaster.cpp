#include <windows.h>
#include "../../base.h"
#include "ehw.h"
#include "SampleRateBroadcaster.h"

SampleRateBroadcaster::SampleRateBroadcaster() :
	Thread("SampleRateBroadcaster"),
	handle(NULL)
{
}

SampleRateBroadcaster::~SampleRateBroadcaster()
{
	shutdown();
}

void SampleRateBroadcaster::initialize( TUsbAudioHandle deviceHandle )
{
	handle = CreateEvent(NULL, FALSE, FALSE, NULL);
	/*TUsbAudioStatus status = */TUSBAUDIO_RegisterDeviceNotification(deviceHandle, TUSBAUDIO_NOTIFY_CATEGORY_SAMPLE_RATE_CHANGE, handle, 0);

	startThread();
}

void SampleRateBroadcaster::shutdown()
{
	if (handle != NULL)
	{
		SetEvent(handle);
		stopThread(1000);
		CloseHandle(handle);
		handle = NULL;
	}
}

void SampleRateBroadcaster::run()
{
	while (! threadShouldExit())
	{
		DWORD result = WaitForSingleObject(handle,INFINITE);

		if (threadShouldExit())
			break;

		switch (result)
		{
		case WAIT_OBJECT_0:
			broadcaster.sendChangeMessage();
			break;
		}
	}
}