#pragma once

#include "Test.h"
#include "MIDILoopTest.h"

class ehw;
class ehwlist;
class Content;

class ProductionUnit : public AudioIODeviceCallback, public MessageListener
{
public:
	ProductionUnit(ehw *dev,ehwlist *devlist,Content *content);
	~ProductionUnit(void);

	bool status();
	void RunTests();

	void audioDeviceAboutToStart(AudioIODevice *device);
	void audioDeviceIOCallback(const float **inputChannelData, int numInputChannels, float **outputChannelData, int numOutputChannels, int numSamples);
	void audioDeviceStopped();

	void handleMessage(const Message &message);

	int	_num_tests;
	bool _unit_passed;
	bool _skipped;

protected:
	enum
	{
		device_buffer_size_samples = 512,
		callback_skip_count = 128,

		max_inputs = 64,

		timer_msec = 100
	};

	float input_meters[max_inputs];

	bool _ok;
	ehw *_dev;
	ehwlist *_devlist;
	Content *_content;
	AudioIODevice *_asio;
	ToneGeneratorAudioSource _tone;
	float _dc_offset;
	int _sawtooth;
	int _pulsate;
	
	uint32 active_outputs;
	volatile LONG blocks_recorded;
	OwnedArray<AudioSampleBuffer> _inbuffs;

	ScopedPointer<XmlElement> _root;
	XmlElement *_script;
	LONG new_test;
	int callback_count;
	bool record_done;

	ScopedPointer<Test> _test;
	MIDILoopTest midiLoopTest;

	int _input;
	int _output;

	LARGE_INTEGER _average_callback_interval;

	String _channel_group_name;
	int _channel_group_passed;

	void Cleanup();
	bool ShowMeterWindow(Test &test);
	void ParseScript();
	void FinishGroup();
	bool OpenASIO(int sample_rate);
	bool CreateASIO(XmlElement *script);
	void clockDetectTest();

	bool CheckSampleRate();
};
