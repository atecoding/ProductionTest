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

void FrequencySweepAudioSource::setSweepTime(double initialDelaySeconds_, double fadeInSeconds_, double fadeOutSeconds_, double sweepLengthSeconds_ )
{
	initialDelaySeconds = initialDelaySeconds_;
	fadeInSeconds = fadeInSeconds_;
	fadeOutSeconds = fadeOutSeconds_;
	sweepLengthSeconds = sweepLengthSeconds_;
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
    finalPhasePerSample = double_Pi * 2.0 / (sampleRate / finalFrequency);
    double totalSamples = sweepLengthSeconds * sampleRate;
    double ratio = finalPhasePerSample / startPhasePerSample;
    
    phasePerSampleStep = pow( ratio, 1.0 / totalSamples);
    
    DBG(startPhasePerSample << " " << finalPhasePerSample << " " << totalSamples << " " << phasePerSampleStep);
    
    phasePerSample = startPhasePerSample;
    
	initialDelaySamples = roundDoubleToInt(initialDelaySeconds * sampleRate);
	fadeInSamples = fadeInCount = roundDoubleToInt(fadeInSeconds * sampleRate);
	fadeOutSamples = fadeOutCount = roundDoubleToInt(fadeOutSeconds * sampleRate);
	
}

void FrequencySweepAudioSource::releaseResources()
{
}

void FrequencySweepAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
    for (int i = 0; i < info.numSamples; ++i)
    {
        float sample = 0.0f;
        
        if (initialDelaySamples > 0)
        {
            initialDelaySamples--;
        }
        
		if (fadeInCount > 0 && 0 == initialDelaySamples)
		{
			fadeInCount--;
			fadeGain = 1.0f - (float)fadeInCount / (float)fadeInSamples;
			fadeGain *= fadeGain;
			sample = amplitude * (float)std::sin(currentPhase) * fadeGain;
			currentPhase += phasePerSample;
		}
		
		if (phasePerSample <= finalPhasePerSample && 0 == fadeInCount)
		{
			sample = amplitude * (float)std::sin(currentPhase);
			currentPhase += phasePerSample;
			phasePerSample *= phasePerSampleStep;
		}

		if (fadeOutCount > 0 && phasePerSample > finalPhasePerSample)
		{
			fadeOutCount--;
			fadeGain = (float)fadeOutCount / (float)fadeInSamples;
			fadeGain *= fadeGain;
			sample = amplitude * (float)std::sin(currentPhase) * fadeGain;
			currentPhase += phasePerSample;
		}

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
    
    WriteWaveFile("sweep.wav", 48000, &buffer, buffer.getNumSamples());
}

