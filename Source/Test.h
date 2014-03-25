#pragma once

class ehw;

class Test
{
protected:
	Test(XmlElement *xe,bool &ok);

public:
	static Test *Create(XmlElement *xe,int input, int output, bool &ok);
	~Test();

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
	float _dc_offset;
	int _sawtooth;
	int _pulsate;

protected:
	String MsgSampleRate();
};



class ThdnTest : public Test
{
public:
	ThdnTest(XmlElement *xe,bool &ok);
	~ThdnTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg);
};


class DynRangeTest : public Test
{
public:
	DynRangeTest(XmlElement *xe,bool &ok);
	~DynRangeTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg);
};


class FrequencyResponseTest : public Test
{
public:
	FrequencyResponseTest(XmlElement *xe,bool &ok);
	~FrequencyResponseTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg);

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
