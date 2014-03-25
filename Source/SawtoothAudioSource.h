#pragma once

class SawtoothAudioSource :
	public AudioSource
{
public:
	SawtoothAudioSource(void);
	virtual ~SawtoothAudioSource(void);
	virtual void 	prepareToPlay (int samplesPerBlockExpected, double sampleRate);
	virtual void 	releaseResources ();
	virtual void 	getNextAudioBlock (const AudioSourceChannelInfo &bufferToFill);
};
