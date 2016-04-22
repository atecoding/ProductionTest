#pragma once

#include "Test.h"
#include "ErrorCodes.h"

#if ACOUSTICIO_BUILD
#include "AIOTestAdapter.h"
#include "calibrationV2/CalibrationManagerV2.h"
class Content;
typedef bool (*AIOTestVector)(  XmlElement const *element,
                                ehw *dev,
                                String &msg,
                                String &displayedChannel,
                                AIOTestAdapter &testAdapter,
                                Content *content,
                                ErrorCodes &errorCodes,
                                ValueTree &unitTree);
#endif

class ehw;
class ehwlist;
class Content;

class ProductionUnit : public AudioIODeviceCallback, public MessageListener, public Timer, public Value::Listener
{
public:
	ProductionUnit(ReferenceCountedObjectPtr<ehw> dev, ehwlist *devlist, Content *content, CalibrationManagerV2* calibrationManager_);
	~ProductionUnit(void);

	bool status();
	void RunTests(Time const testStartTime_);

	void audioDeviceAboutToStart(AudioIODevice *device) override;
	void audioDeviceIOCallback(const float **inputChannelData, int numInputChannels, float **outputChannelData, int numOutputChannels, int numSamples) override;
	void audioDeviceStopped() override;
    void timerCallback() override;
	virtual void valueChanged(Value& value) override;

	void handleMessage(const Message &message) override;
    void deviceRemoved();

	int	_num_tests;
    ErrorCodes errorCodes;
	bool _unit_passed;
	bool _skipped;
	bool _running;
    bool deviceAttached;

	File & getLogFile()
	{
		return logfile;
	}

	File getOutputFolder();

	String getSerialNumber() const
	{
		return _serial_number;
	}
    
    void setSerialNumber(String const serialNumber_);
    
    ValueTree unitTree;

protected:
	enum
	{
		callback_skip_samples = 32000,
        MAX_RECORD_BUFFER_SAMPLES = 4 * 96000,

		max_inputs = 64,
		MAX_TIMESTAMPS = 2048,

		timer_msec = 100
	};

	float input_meters[max_inputs];

	bool _ok;
	ReferenceCountedObjectPtr<ehw> _dev;
	ehwlist *_devlist;
	Content *_content;
	ScopedPointer<AudioIODevice> audioDevice;
	ToneGeneratorAudioSource _tone;
	
	uint32 active_outputs;
	Atomic<int32> blocks_recorded;
	OwnedArray<AudioSampleBuffer> _inbuffs;

	ScopedPointer<XmlElement> _root;
	XmlElement *_script;
	Atomic<int32> new_test;
    int audioCallbackCount;
	int totalAudioCallbackSamples;
    int timerIntervalMsec;
	bool record_done;
    int64 maxAudioDeviceCreateTicks;
    int64 maxAudioDeviceOpenTicks;
    int64 maxAudioDeviceStartTicks;
    
	ScopedPointer<Test> _test;

	int _input;
	int _output;

	Atomic<int> timestampCount;
	int64 timestamps[MAX_TIMESTAMPS];

	String _channel_group_name;
	int _channel_group_passed;

	String _serial_number;
    Time testStartTime;
	File logfile;

    void assignAutomaticSerialNumber();
	void CreateLogFile();
	void Cleanup();
	bool ShowMeterWindow(Test &test);
	void ParseScript();
	void FinishGroup();
    bool createAudioDevice(XmlElement *script);
	bool openAudioDevice(int sample_rate);
    bool startAudioDevice();
    void stopAudioDevice();
    void audioDeviceTimedOut();
	void clockDetectTest();
    void runOfflineTest(XmlElement *script);
    void logPerformanceInfo();

	Result CheckSampleRate();

#if ACOUSTICIO_BUILD
	AIOTestAdapter aioTestAdapter;
	CalibrationManagerV2* calibrationManager;
    Value calibrationStateValue;

    void runAIOTest(AIOTestVector function, String const groupName);
    void finishCalibration();
    void finishAIOSResistanceMeasurement();
    void printErrorCodes(XmlElement *xe);
#endif
};
