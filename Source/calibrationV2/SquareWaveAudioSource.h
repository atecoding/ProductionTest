#pragma once

class SquareWaveAudioSource : public AudioSource
{
public:
    SquareWaveAudioSource();
    virtual ~SquareWaveAudioSource();
    
    struct Configuration
    {
        Configuration()
        {
            frequency = 0.0f;
            minAmplitude = -FLT_MAX;
            maxAmplitude = FLT_MAX;
        }
        
        float const getAmplitude() const
        {
            return fabs(maxAmplitude - minAmplitude);
        }
        
        float frequency;
        float minAmplitude;
        float maxAmplitude;
    };
    
    void setConfiguration(Configuration& configuration_);
    Configuration const & getConfiguration() const
    {
        return configuration;
    }
    
    virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    virtual void releaseResources();
    virtual void getNextAudioBlock (const AudioSourceChannelInfo &bufferToFill);
    
protected:
    Configuration configuration;
    int squareWavePeriodSamples;
    int squareWavePosition;
};