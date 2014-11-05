#pragma once

class SampleRateBroadcaster : public Thread
{
public:
	SampleRateBroadcaster();
	~SampleRateBroadcaster();

	void initialize(TUsbAudioHandle deviceHandle);
	void shutdown();
	virtual void run();

	ChangeBroadcaster broadcaster;
	HANDLE handle;
};

