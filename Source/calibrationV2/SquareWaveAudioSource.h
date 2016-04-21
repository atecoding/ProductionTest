#pragma once

class SquareWaveAudioSource : public AudioSource
{
public:
    SquareWaveAudioSource();
    virtual ~SquareWaveAudioSource();
    
    virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    virtual void releaseResources();
    virtual void getNextAudioBlock (const AudioSourceChannelInfo &bufferToFill);
    
    float squareWaveFrequency;
    float squareWaveMinAmplitude;
    float squareWaveMaxAmplitude;

protected:
    int squareWavePeriodSamples;
    int squareWavePosition;
};