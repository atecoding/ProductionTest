#pragma once

class CalibrationAudioIOCallback : public AudioIODeviceCallback
{
public:
	CalibrationAudioIOCallback();
	virtual ~CalibrationAudioIOCallback();

	virtual void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override;

	void recordInputs(int numSamples, int numInputChannels, const float** inputChannelData);

	virtual void audioDeviceAboutToStart(AudioIODevice* device) override;
	virtual void audioDeviceStopped() override;

	int playbackPosition;
	AudioSampleBuffer playbackBuffer;

	int recordPosition;
	AudioSampleBuffer recordBuffer;

	float getSquareWaveFrequency() const;

	float squareWaveMinAmplitude;
	float squareWaveMaxAmplitude;

protected:
	int callbackCount;
	int squareWavePeriodSamples;
	int squareWavePosition;

	void fillSquareWave(AudioSampleBuffer &outputBuffer);
};

