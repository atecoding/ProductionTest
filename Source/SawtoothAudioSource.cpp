#include "base.h"
#include "SawtoothAudioSource.h"

SawtoothAudioSource::SawtoothAudioSource(void)
{
}

SawtoothAudioSource::~SawtoothAudioSource(void)
{
}
void 	SawtoothAudioSource::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
}
void 	SawtoothAudioSource::releaseResources ()
{
}
void 	SawtoothAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
    for (int i = 0; i < info.numSamples; ++i)
    {
        const float sample = - 0.5f + (float) i / (float) info.numSamples;

        for (int j = info.buffer->getNumChannels(); --j >= 0;)
            *info.buffer->getWritePointer(j, info.startSample + i) = sample;
    }
}
