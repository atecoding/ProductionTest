#include "../base.h"
#include "CalibrationAudioIO.h"
#include "CalibrationManagerV2.h"


CalibrationAudioIO::CalibrationAudioIO(CalibrationManagerV2& calibrationManager_) :
calibrationManager(calibrationManager_),
ioDevice(nullptr, true),
recordPosition(0),
recordSamplesNeeded(0),
callbackCount(0),
result(Result::ok()),
recordProgress(0.0)
{
}

CalibrationAudioIO::~CalibrationAudioIO()
{
}

void CalibrationAudioIO::audioDeviceAboutToStart(AudioIODevice* device)
{
    double recordLengthSeconds = 0.0;
    recordPosition = 0;
    callbackCount = 0;
    recordProgress = 0.0;
    
    CalibrationProcedure * const procedure = calibrationManager.getProcedure();
    if (procedure)
    {
        procedure->prepareToPlay(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());
        
        recordLengthSeconds = procedure->getRecordLengthSeconds();
        recordSamplesNeeded = roundDoubleToInt(recordLengthSeconds * device->getCurrentSampleRate());
        
        StringArray inputNames(device->getInputChannelNames());
        if (recordBuffer.getNumSamples() < recordSamplesNeeded ||
            recordBuffer.getNumChannels() < inputNames.size())
        {
            recordBuffer.setSize(inputNames.size(), recordSamplesNeeded);
        }
    }
    
    recordBuffer.clear();

    double timeoutSeconds = jmax( recordLengthSeconds * 1.5, 15.0);
    timeout = Time::getCurrentTime() + RelativeTime(timeoutSeconds);
    startTimer(100);
}

void CalibrationAudioIO::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples)
{
    ++callbackCount;
    
    CalibrationProcedure * const procedure = calibrationManager.getProcedure();
    
    AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    
    outputBuffer.clear();
    
    if (procedure)
    {
        procedure->fillOutputBuffer(outputBuffer);
        
        recordInputs(inputChannelData, numInputChannels, numSamples);
    }
}

void CalibrationAudioIO::audioDeviceStopped()
{
}


void CalibrationAudioIO::recordInputs(const float** inputChannelData, int numInputChannels, int numSamples)
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

void CalibrationAudioIO::setDevice(AudioIODevice* audioIODevice_)
{
	ioDevice.setNonOwned(audioIODevice_);
}


Result CalibrationAudioIO::start(double const sampleRate)
{
#ifdef JUCE_WIN32
    String deviceName("ASIO Echo AIO");
#endif
    
#ifdef JUCE_MAC
    String deviceName(ioDeviceName);
#endif
    
    DBG("CalibrationAudioIO::start()");
    if (ioDevice.willDeleteObject())
    {
		//
		// Create the AudioIODeviceType if it doesn't exist
		//
		if (nullptr == ioDeviceType)
		{
#ifdef JUCE_WIN32
			ioDeviceType = AudioIODeviceType::createAudioIODeviceType_ASIO();
#endif

#ifdef JUCE_MAC
			ioDeviceType = AudioIODeviceType::createAudioIODeviceType_CoreAudio();
#endif
		}

		if (nullptr == ioDeviceType)
		{
			result = Result::fail("Could not create audio device enumerator");
			return result;
		}

		ioDeviceType->scanForDevices();

		//
		// Create the AudioIODevice
		//
		ioDevice.setOwned(ioDeviceType->createDevice(deviceName, String::empty));
		if (nullptr == ioDevice)
		{
			result = Result::fail("Could not create " + deviceName);
			return result;
		}
    }

    DBG("CalibrationAudioIO::start -- ioDevice OK " + ioDevice->getName());

    //
    // Start the AudioIODevice
    //
    StringArray inputNames(ioDevice->getInputChannelNames());
    StringArray outputNames(ioDevice->getOutputChannelNames());
    BigInteger inputChannels, outputChannels;
    inputChannels.setRange(0, inputNames.size(), true);
    outputChannels.setRange(0, outputNames.size(), true);
    String openResult(ioDevice->open(inputChannels, outputChannels, sampleRate, ioDevice->getDefaultBufferSize()));
    if (openResult.isNotEmpty())
    {
        DBG("CalibrationAudioIO::start --  open error " + openResult);
        result = Result::fail(openResult);
        return result;
    }
    
    ioDevice->start(this);
    
    DBG("CalibrationAudioIO::start -- OK");
    
    result = Result::ok();
    return result;
}

void CalibrationAudioIO::stop()
{
    if (ioDevice)
    {
        ioDevice->stop();
        ioDevice->close();
    }
    
    stopTimer();
    
    //WriteWaveFile("calibration.wav", recordBuffer, ioDevice->getCurrentSampleRate(), recordPosition);
}

void CalibrationAudioIO::timerCallback()
{
    if (0 == recordBuffer.getNumSamples())
    {
        stopTimer();
        return;
    }
    
    recordProgress = double(recordPosition) / recordSamplesNeeded;
    
    if (recordPosition >= recordSamplesNeeded)
    {
        stopTimer();
        calibrationManager.audioRecordDone();
        return;
    }
    
    if (Time::getCurrentTime() >= timeout)
    {
        stopTimer();
        
        result = Result::fail("Could not record - audio callback timeout");
        
        calibrationManager.audioRecordDone();
        return;
    }
}


