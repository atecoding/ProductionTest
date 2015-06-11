#pragma once

#include "Test.h"
#include "MIDILoopTest.h"
#include "ErrorCodes.h"

#if ACOUSTICIO_BUILD
#include "AIOTestAdapter.h"
#include "calibration/CalibrationManager.h"
class Content;
typedef bool (*AIOTestVector)(XmlElement const *element, ehw *dev, String &msg, int &displayedInput, AIOTestAdapter &testAdapter, Content *content, ErrorCodes &errorCodes);
#endif

class ehw;
class ehwlist;
class Content;

class ProductionUnit : public AudioIODeviceCallback, public MessageListener
{
public:
	ProductionUnit(ehw *dev,ehwlist *devlist,Content *content);
	~ProductionUnit(void);

	bool status();
	void RunTests(String const serialNumber_);

	void audioDeviceAboutToStart(AudioIODevice *device);
	void audioDeviceIOCallback(const float **inputChannelData, int numInputChannels, float **outputChannelData, int numOutputChannels, int numSamples);
	void audioDeviceStopped();

	void handleMessage(const Message &message);

	int	_num_tests;
    ErrorCodes errorCodes;
	bool _unit_passed;
	bool _skipped;
	bool _running;

	FileOutputStream *getLogStream() const
	{
		return _log_stream;
	}

	File getOutputFolder();

	String getSerialNumber() const
	{
		return _serial_number;
	}
    
    ValueTree tree;

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
	ehw *_dev;
	ehwlist *_devlist;
	Content *_content;
	ScopedPointer<AudioIODevice> _asio;
	ToneGeneratorAudioSource _tone;
	
	uint32 active_outputs;
	Atomic<int32> blocks_recorded;
	OwnedArray<AudioSampleBuffer> _inbuffs;

	ScopedPointer<XmlElement> _root;
	XmlElement *_script;
	Atomic<int32> new_test;
	int callback_samples;
	bool record_done;

	ScopedPointer<Test> _test;
	MIDILoopTest midiLoopTest;

	int _input;
	int _output;

	Atomic<int> timestampCount;
	int64 timestamps[MAX_TIMESTAMPS];

	String _channel_group_name;
	int _channel_group_passed;

	String _serial_number;
	File _logfile;
	ScopedPointer <FileOutputStream> _log_stream;

	void CreateLogFile();
	void Cleanup();
	bool ShowMeterWindow(Test &test);
	void ParseScript();
	void FinishGroup();
	bool OpenASIO(int sample_rate);
	bool CreateASIO(XmlElement *script);
	void clockDetectTest();

	Result CheckSampleRate();

#if ACOUSTICIO_BUILD
	AIOTestAdapter aioTestAdapter;
    CalibrationManager calibrationManager;
    
    void runAIOTest(AIOTestVector function, String const groupName);
    void finishAIOSCalibration();
#endif
};
