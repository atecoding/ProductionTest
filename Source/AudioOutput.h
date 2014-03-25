#pragma once

class AudioOutput
{
public:
	AudioOutput(XmlElement *xe,bool &ok);
	~AudioOutput();

	void Setup(int samples_per_block,ToneGeneratorAudioSource &tone,uint32 &active_outputs, float &dc_offset, int &sawtooth, int &pulsate);

	String title;
	int output;
	int num_channels;
	int sample_rate;
	float output_amplitude_db;
	float _dc_offset;
	int _sawtooth;
	int _pulsate;
};
