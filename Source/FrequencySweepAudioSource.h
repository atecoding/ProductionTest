#pragma once

class FrequencySweepAudioSource  : public AudioSource
{
public:
    FrequencySweepAudioSource();
    ~FrequencySweepAudioSource();
    
    void setAmplitude (float newAmplitude);
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const AudioSourceChannelInfo&) override;
    
    static void test();
    
private:
    double sampleRate, startFrequency, finalFrequency, sweepLengthSeconds;
    double currentPhase, phasePerSample, phasePerSampleStep;
    float amplitude;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencySweepAudioSource)
};