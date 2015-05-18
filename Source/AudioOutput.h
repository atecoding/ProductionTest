#pragma once

class AudioOutput
{
public:
	AudioOutput(XmlElement *xe,bool &ok);
	~AudioOutput();

	void Setup(int samples_per_block,ToneGeneratorAudioSource &tone,uint32 &active_outputs);

	String title;
	int output;
	int num_channels;
	int sample_rate;
	float output_amplitude_db;
};
