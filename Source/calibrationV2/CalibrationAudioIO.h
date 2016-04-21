#pragma once

class CalibrationManagerV2;

class CalibrationAudioIO : public AudioIODeviceCallback, public Timer
{
public:
    CalibrationAudioIO(CalibrationManagerV2& calibrationManager_);
    virtual ~CalibrationAudioIO();
    
    Result start(double const sampleRate);
    void stop();
    
    double &getRecordProgress()
    {
        return recordProgress;
    }
    
    Result getResult() const
    {
        return result;
    }
    
    AudioBuffer<float> recordBuffer;
    
#if JUCE_MAC
    void setCoreAudioName(String const ioDeviceName_)
    {
        ioDeviceName = ioDeviceName_;
    }
#endif

	void setDevice(AudioIODevice* audioIODevice_);

    
private:
    virtual void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override;
    
    void recordInputs(const float** inputChannelData, int numInputChannels, int numSamples);
    
    virtual void audioDeviceAboutToStart(AudioIODevice* device) override;
    virtual void audioDeviceStopped() override;
    
    virtual void timerCallback() override;
    
    CalibrationManagerV2& calibrationManager;
    
#if JUCE_MAC
    String ioDeviceName;
#endif
    ScopedPointer<AudioIODeviceType> ioDeviceType;
    OptionalScopedPointer<AudioIODevice> ioDevice;
    
    int recordPosition;
    int recordSamplesNeeded;
    
    int callbackCount;
    
    Result result;
    
    double recordProgress;
    Time timeout;
};
