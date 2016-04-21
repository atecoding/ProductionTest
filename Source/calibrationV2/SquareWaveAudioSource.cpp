#include "../base.h"
#include "SquareWaveAudioSource.h"

SquareWaveAudioSource::SquareWaveAudioSource() :
squareWaveFrequency(1000.0f),
squareWaveMinAmplitude(0.0f),
squareWaveMaxAmplitude(0.5f)
{
    
}

SquareWaveAudioSource::~SquareWaveAudioSource()
{
    
}

void SquareWaveAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    squareWavePosition = 0;
    squareWavePeriodSamples = roundDoubleToInt(sampleRate / squareWaveFrequency);
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
                *destination = squareWaveMaxAmplitude;
            else
                *destination = squareWaveMinAmplitude;
            
            squareWavePositionThisChannel = (squareWavePositionThisChannel + 1) % squareWavePeriodSamples;
            
            ++destination;
            --bufferSamplesRemaining;
        }
    }
    
    squareWavePosition = squareWavePositionThisChannel;
}
