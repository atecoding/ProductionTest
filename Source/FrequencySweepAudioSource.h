#pragma once

class FrequencySweepAudioSource  : public AudioSource
{
public:
    FrequencySweepAudioSource();
    ~FrequencySweepAudioSource();
    
    void setSweepTime(double initialDelaySeconds_, double sweepFadeInSeconds_, double sweepFadeOutSeconds_, double sweepLengthSeconds_);
    void setAmplitude (float newAmplitude);
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const AudioSourceChannelInfo&) override;
    
    static void test();
    
private:
    double initialDelaySeconds, fadeInSeconds, fadeOutSeconds;
    double sampleRate, startFrequency, finalFrequency, sweepLengthSeconds;
    double currentPhase, phasePerSample, phasePerSampleStep, finalPhasePerSample;
    float amplitude;
    int initialDelaySamples, fadeInSamples, fadeOutSamples, fadeInCount, fadeOutCount;

	typedef enum
	{
		START,
		FADE_IN,
		SWEEP,
		FADE_OUT,
		FINISH
	} State;
	State state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencySweepAudioSource)
};