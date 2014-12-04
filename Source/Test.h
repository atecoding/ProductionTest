#pragma once

class ehw;

class Test
{
protected:
	Test(XmlElement *xe,bool &ok);

public:
	static Test *Create(XmlElement *xe,int input, int output, bool &ok);
	virtual ~Test();

	void Setup( int samples_per_block,
				ToneGeneratorAudioSource &tone,
				uint32 &active_outputs,
				float &dc_offset,
				int &sawtooth,
				int &pulsate);
	virtual bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg) = 0;
	
	String title;
	int input;
	int output;
	int num_channels;
	int sample_rate;
	float output_amplitude_db;	
	float output_frequency;
	float pass_threshold_db;
	float min_level_db;
	float max_level_db;
	float _dc_offset;
	int _sawtooth;
	int _pulsate;
	float minSampleRate;
	float maxSampleRate;
	int errorBit;

protected:
	String MsgSampleRate();
};



class ThdnTest : public Test
{
public:
	ThdnTest(XmlElement *xe, bool &ok);
	~ThdnTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg);
};


class DiffThdnTest : public Test
{
public:
	DiffThdnTest(XmlElement *xe, bool &ok);
	~DiffThdnTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg);
};


class DynRangeTest : public Test
{
public:
	DynRangeTest(XmlElement *xe, bool &ok);
	~DynRangeTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg);
};


class DiffDynRangeTest : public Test
{
public:
	DiffDynRangeTest(XmlElement *xe, bool &ok);
	~DiffDynRangeTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg);
};


class FrequencyResponseTest : public Test
{
public:
	FrequencyResponseTest(XmlElement *xe, bool &ok);
	~FrequencyResponseTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg);

};


class LevelCheckTest : public Test
{
public:
	LevelCheckTest(XmlElement *xe, bool &ok);
	~LevelCheckTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg);

};


class HexInputCrosstalkTest : public Test
{
public:
	HexInputCrosstalkTest(XmlElement *xe,bool &ok);
	~HexInputCrosstalkTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg);
};

class SaturationTest : public Test
{
public:
	SaturationTest(XmlElement *xe,bool &ok);
	~SaturationTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg);

};

class PhaseTest : public Test
{
public:
	PhaseTest(XmlElement *xe,bool &ok);
	~PhaseTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg);

};
