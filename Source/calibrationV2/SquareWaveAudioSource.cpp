#include "../base.h"
#include "SquareWaveAudioSource.h"

SquareWaveAudioSource::SquareWaveAudioSource()
{
    
}

SquareWaveAudioSource::~SquareWaveAudioSource()
{
    
}


void SquareWaveAudioSource::setConfiguration(Configuration& configuration_)
{
    jassert(configuration_.frequency > 0.0f);
    jassert(-1.0f <= configuration_.minAmplitude && configuration_.minAmplitude <= 0.0f);
    jassert(0.0f <= configuration_.maxAmplitude && configuration_.maxAmplitude <= 1.0f);
    configuration = configuration_;
}

void SquareWaveAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    jassert(configuration.frequency > 0.0f);
    jassert(-1.0f <= configuration.minAmplitude && configuration.minAmplitude <= 0.0f);
    jassert(0.0f <= configuration.maxAmplitude && configuration.maxAmplitude<= 1.0f);
    squareWavePosition = 0;
    squareWavePeriodSamples = roundDoubleToInt(sampleRate / configuration.frequency);
}

void SquareWaveAudioSource::releaseResources()
{
}

void SquareWaveAudioSource::getNextAudioBlock (const AudioSourceChannelInfo &bufferToFill)
{
    int squareWavePositionThisChannel = squareWavePosition;
    int halfPeriodSamples = squareWavePeriodSamples >> 1;
    
    for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        squareWavePositionThisChannel = squareWavePosition;
        float* destination = bufferToFill.buffer->getWritePointer(channel);
        if (nullptr == destination)
            continue;
        
        int bufferSamplesRemaining = bufferToFill.buffer->getNumSamples();
        while (bufferSamplesRemaining > 0)
        {
            if (squareWavePositionThisChannel < halfPeriodSamples)
                *destination = configuration.maxAmplitude;
            else
                *destination = configuration.minAmplitude;
            
            squareWavePositionThisChannel = (squareWavePositionThisChannel + 1) % squareWavePeriodSamples;
            
            ++destination;
            --bufferSamplesRemaining;
        }
    }
    
    squareWavePosition = squareWavePositionThisChannel;
}
