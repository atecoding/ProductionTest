#pragma once

class FrequencySweepAudioSource  : public AudioSource
{
public:
    FrequencySweepAudioSource();
    ~FrequencySweepAudioSource();
    
    void setSweepTime(double initialDelaySeconds_, double sweepLengthSeconds_);
    void setAmplitude (float newAmplitude);
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const AudioSourceChannelInfo&) override;
    
    static void test();
    
private:
    double initialDelaySeconds;
    double sampleRate, startFrequency, finalFrequency, sweepLengthSeconds;
    double currentPhase, phasePerSample, phasePerSampleStep, finalPhasePerSample;
    float amplitude;
    int initialDelaySamples;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencySweepAudioSource)
};