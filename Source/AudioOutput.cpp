#include "base.h"
#include "ehw.h"
#include "AudioOutput.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"

AudioOutput::AudioOutput(XmlElement *xe,bool &ok)
{
	ok = true;

	ok &= getIntValue(xe,T("output"),output);
	ok &= getIntValue(xe,T("sample_rate"),sample_rate);
	ok &= getFloatValue(xe,T("output_amplitude_db"),output_amplitude_db);
	ok &= getIntValue(xe,T("num_channels"),num_channels);
}


AudioOutput::~AudioOutput()
{
}
	

void AudioOutput::Setup
(
	int samples_per_block,
	ToneGeneratorAudioSource &tone,
	uint32 &active_outputs)
{
	float amp;

/*
	uint32 mask = (1 << num_channels) - 1;
	active_outputs = mask << output;
*/
	active_outputs = 0x03 << (output & 0xfffe);	// always play a stereo pair
	tone.prepareToPlay(samples_per_block,sample_rate);
	tone.setFrequency( 1000.0 );	
	amp = pow(10.0f,output_amplitude_db/20.0f);
	tone.setAmplitude( amp );
}



