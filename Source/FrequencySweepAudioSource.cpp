#include "base.h"
#include "FrequencySweepAudioSource.h"
#include "wavefile.h"

FrequencySweepAudioSource::FrequencySweepAudioSource() :
sampleRate (48000.0),
startFrequency (20.0),
finalFrequency(20000.0),
sweepLengthSeconds(2.0),
currentPhase (0.0),
phasePerSample (0.0),
phasePerSampleStep(0.0),
amplitude (0.5f)
{
}

FrequencySweepAudioSource::~FrequencySweepAudioSource()
{
}

void FrequencySweepAudioSource::setAmplitude (const float newAmplitude)
{
    amplitude = newAmplitude;
}

void FrequencySweepAudioSource::prepareToPlay (int /*samplesPerBlockExpected*/, double rate)
{
    currentPhase = 0.0;
    sampleRate = rate;
    
    double startPhasePerSample = double_Pi * 2.0 / (sampleRate / startFrequency);
    double finalPhasePerSample = double_Pi * 2.0 / (sampleRate / finalFrequency);
    double totalSamples = sweepLengthSeconds * sampleRate;
    double ratio = finalPhasePerSample / startPhasePerSample;
    
    phasePerSampleStep = pow( ratio, 1.0 / totalSamples);
    
    DBG(startPhasePerSample << " " << finalPhasePerSample << " " << totalSamples << " " << phasePerSampleStep);
    
    phasePerSample = startPhasePerSample;
}

void FrequencySweepAudioSource::releaseResources()
{
}

void FrequencySweepAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
    for (int i = 0; i < info.numSamples; ++i)
    {
        const float sample = amplitude * (float) std::sin (currentPhase);
        currentPhase += phasePerSample;
        phasePerSample *= phasePerSampleStep;
        
        for (int j = info.buffer->getNumChannels(); --j >= 0;)
            info.buffer->setSample (j, info.startSample + i, sample);
    }
}

void FrequencySweepAudioSource::test()
{
    FrequencySweepAudioSource source;
    AudioSampleBuffer buffer(1, 48000 * 2);
    AudioSourceChannelInfo asi(&buffer, 0, buffer.getNumSamples());
    
    source.prepareToPlay(buffer.getNumSamples(), 48000.0);
    source.getNextAudioBlock(asi);
    source.releaseResources();
    
    WriteWaveFile("sweep.wav", 48000.0, &buffer);
}

