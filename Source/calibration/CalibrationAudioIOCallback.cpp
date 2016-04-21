#if 0
#include "../base.h"
#include "CalibrationAudioIOCallback.h"

static const float squareWaveFrequency = 500.0f;
static const int audioBufferLengthSamples = 48000 * 10;

CalibrationAudioIOCallback::CalibrationAudioIOCallback() :
playbackBuffer(8, audioBufferLengthSamples),
recordBuffer(8, audioBufferLengthSamples),
squareWaveMinAmplitude(0.0f),
squareWaveMaxAmplitude(0.5f),
callbackCount(0)
{
}

CalibrationAudioIOCallback::~CalibrationAudioIOCallback()
{
}

void CalibrationAudioIOCallback::audioDeviceAboutToStart(AudioIODevice* device)
{
    DBG("CalibrationAudioIOCallback::audioDeviceAboutToStart recordBuffer:" << recordBuffer.getNumSamples());
    
    callbackCount = 0;
    
	squareWavePeriodSamples = roundDoubleToInt(device->getCurrentSampleRate() / squareWaveFrequency);
	squareWavePosition = 0;
	
	playbackPosition = 0;
	recordPosition = 0;
}

void CalibrationAudioIOCallback::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples)
{
	++callbackCount;

	AudioSampleBuffer outputBuffer(outputChannelData, numOutputChannels, numSamples);
	fillSquareWave(outputBuffer);

	recordInputs(numSamples, numInputChannels, inputChannelData);
}

void CalibrationAudioIOCallback::audioDeviceStopped()
{
    DBG("CalibrationAudioIOCallback::audioDeviceStopped() callbackCount:" << callbackCount << " recordPosition:" << recordPosition);
}

void CalibrationAudioIOCallback::fillSquareWave(AudioSampleBuffer &outputBuffer)
{
	int squareWavePositionThisChannel = squareWavePosition;
	int halfPeriodSamples = squareWavePeriodSamples >> 1;

	for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
	{
		squareWavePositionThisChannel = squareWavePosition;
		float* destination = outputBuffer.getWritePointer(channel);
		if (nullptr == destination)
			continue;

		int bufferSamplesRemaining = outputBuffer.getNumSamples();
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

void CalibrationAudioIOCallback::recordInputs(int numSamples, int numInputChannels, const float** inputChannelData)
{
	int remainingRecordSpace = recordBuffer.getNumSamples() - recordPosition;
	if (remainingRecordSpace > 0)
	{
		int copyCount = jmin(numSamples, remainingRecordSpace);
		for (int input = 0; input < numInputChannels; ++input)
		{
			if (nullptr == inputChannelData[input])
				continue;

			recordBuffer.copyFrom(input, recordPosition, inputChannelData[input], copyCount);
		}

		recordPosition += copyCount;
	}
}

float CalibrationAudioIOCallback::getSquareWaveFrequency() const
{
	return squareWaveFrequency;
}
#endif
