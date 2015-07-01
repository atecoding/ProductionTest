#include "../base.h"
#include "CalibrationManager.h"
#include "ehw.h"
#include "crc32.h"
#include "../OldMessage.h"

#define SHOW_INTERMEDIATE_RESULTS 0
#define SKIP_AIO2_CALIBRATION 0

static const double sampleRate = 48000.0;
static const double skipInitialDataSeconds = 8.0;
static const double timeoutSeconds = 15.0;
static const float voltageOutputAmplitude = 0.5f;
static const float voltageInputOutputAmplitude = 0.5f;
static const float currentInputOutputAmplitude = 0.5f;
static const float referenceResistorOhms = 10.0f;
static const float externalSpeakerMonitorTolerance = 0.005f; // +/- 0.5%

const float CalibrationManager::AIOSReferencePeakVolts = 2.5f;   // Reference signal is 0 to +5V, but AC coupled so -2.5V to +2.5V
const float CalibrationManager::voltageInputPeakVolts = 8.75f;    // Max +8.75V, min -8.75V, common to AIO-2 and AIO-S
const float CalibrationManager::voltageOutputPeakVolts = 13.5f;   // Max +13.5V, min -13.5V, common to AIO-2 and AIO-S
const float CalibrationManager::expectedAIOSVoltageInputReferenceSignalResult = (AIOSReferencePeakVolts * 2.0f) / voltageInputPeakVolts;
const float CalibrationManager::expectedAIOSVoltageInputWithCalibratedOutputResult = (voltageOutputPeakVolts / voltageInputPeakVolts) * voltageOutputAmplitude;
const float CalibrationManager::expectedAIO2VoltageInputResult = (voltageOutputPeakVolts * voltageOutputAmplitude) / voltageInputPeakVolts;
const float CalibrationManager::expectedVoltageOutputResult = (voltageOutputPeakVolts / voltageInputPeakVolts) * voltageOutputAmplitude;
const float CalibrationManager::expectedVoltageOverCurrent = 5.0f;

const float CalibrationManager::voltMeterMax = 7.0f;
const float CalibrationManager::voltMeterMin = 6.0f;
const int CalibrationManager::serialNumberLength = 7;

const float ratioTolerancePercent = 25.0f;
const float minDutyCycle = 0.45f;
const float maxDutyCycle = 0.55f;

const String CalibrationManager::voltageInputName("Voltage input");
const String CalibrationManager::currentInputName("Current input");
const String CalibrationManager::voltageOutputName("Output voltage");

CalibrationManager::Limits CalibrationManager::limitsExternalSpeakerMonitor
(
	0.6f, // uncalibratedVoltageInputMin
	0.8f, // uncalibratedVoltageInputMax
	0.6f, // voltageOutputMin
	0.8f, // voltageOutputMax
	0.6f, // uncalibratedVoltageInputMin
	0.8f, // uncalibratedVoltageInputMax
	0.6f, // currentInputMin
	0.8f  // currentInputMax
);

CalibrationManager::Limits CalibrationManager::limitsAIOS
(
    // Uncalibrated voltage input - 5V ref signal into voltage input
	expectedAIOSVoltageInputReferenceSignalResult * 0.94f, // voltageInputMin - nominal 0.571
	expectedAIOSVoltageInputReferenceSignalResult * 1.06f, // voltageInputMax
 
    // Uncalibrated voltage output into calibrated voltage input
	expectedVoltageOutputResult * 0.88f, // voltageOutputMin - nominal 0.771
	expectedVoltageOutputResult * 1.12f, // voltageOutputMax

    // Calibrated voltage output into calibrated voltage input
	expectedAIOSVoltageInputWithCalibratedOutputResult * 0.94f, // voltageInputMin - nominal 0.771
	expectedAIOSVoltageInputWithCalibratedOutputResult * 1.06f, // voltageInputMax
 
    // Current input
    (expectedVoltageOutputResult / expectedVoltageOverCurrent) * 0.94f, // currentInputMin - nominal 0.154
	(expectedVoltageOutputResult / expectedVoltageOverCurrent) * 1.06f // currentInputMax
);

CalibrationManager::CalibrationManager(MessageListener *messageListener_) :
state(STATE_IDLE),
powerLED(false),
limits(nullptr),
messageListener(messageListener_)
{
	for (int i = 0; i < numElementsInArray(positiveCalibrationResults); ++i)
	{
		positiveCalibrationResults[i].clear(1.0f);
	}
	for (int i = 0; i < numElementsInArray(negativeCalibrationResults); ++i)
	{
		negativeCalibrationResults[i].clear(-1.0f);
	}

#if SKIP_AIO2_CALIBRATION
	//
	// Use kinda-sort accurate values
	//
  	calibrationDataExternal.currentInputGain = 0.909371f;
  	calibrationDataExternal.voltageInputGain = 0.901895f;
#endif
}

CalibrationManager::~CalibrationManager()
{
	closeCalibrationDialog();
	stopIODevice();
}

void CalibrationManager::startIntegratedSpeakerMonitorCalibration(ReferenceCountedObjectPtr<ehw> device_)
{
	voltageInputChannel = AIOS_VOLTAGE_INPUT_CHANNEL;
	currentInputChannel = AIOS_CURRENT_INPUT_CHANNEL;
	voltageOutputChannel = AIOS_VOLTAGE_OUTPUT_CHANNEL;
    
    results = String::empty;

	state = STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE;
	usbDevice = device_;

	limits = &limitsAIOS;

	execute();
}

void CalibrationManager::timerCallback()
{
	double recordPosition(audioIOCallback.recordPosition);
	int recordBufferNumSamples(audioIOCallback.recordBuffer.getNumSamples());
	
	recordProgress = recordPosition / recordBufferNumSamples;

	if (recordPosition < recordBufferNumSamples)
	{
		RelativeTime elapsed(Time::getCurrentTime() - recordStartTime);
		if (elapsed.inSeconds() >= timeoutSeconds)
		{
			stopTimer();

			state = STATE_TIMED_OUT;
			execute();
		}
		return;
	}

	stopTimer();

	switch (state)
	{
	case STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE:
		state = STATE_ANALYZE_VOLTAGE_INPUT;
		execute();
		break;

	case STATE_START_CALIBRATE_VOLTAGE_OUTPUT:
		state = STATE_ANALYZE_VOLTAGE_OUTPUT;
		execute();
		break;

	case STATE_START_CALIBRATE_CURRENT_INPUT:
		state = STATE_ANALYZE_CURRENT_INPUT;
		execute();
		break;

	case STATE_START_RESISTANCE_MEASUREMENT:
		state = STATE_ANALYZE_RESISTANCE_MEASUREMENT;
		execute();
		break;

	case STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK:
		state = STATE_ANALYZE_VOLTAGE_INPUT;
		execute();
		break;

	case STATE_START_EXTERNAL_SPEAKER_MONITOR_TEST:
		state = STATE_FINISH_EXTERNAL_SPEAKER_MONITOR_TEST;
		execute();
		break;

	default:
		break;
	}
}

Result CalibrationManager::createIODevice()
{
    DBG("CalibrationManager::createIODevice()");

#ifdef JUCE_WIN32
    String deviceName("ASIO Echo Acoustic AIO");

    ioDeviceType = AudioIODeviceType::createAudioIODeviceType_ASIO();
#endif
    
#ifdef JUCE_MAC
    String deviceName("EchoAIO");

    ioDeviceType = AudioIODeviceType::createAudioIODeviceType_CoreAudio();
#endif
    
    if (nullptr == ioDeviceType)
		return Result::fail("Could not create device enumerator");

	ioDeviceType->scanForDevices();

	ioDevice = ioDeviceType->createDevice(deviceName, String::empty);
	if (nullptr == ioDevice)
		return Result::fail("Could not create " + deviceName);

	DBG("ioDevice OK " + ioDevice->getName());

	return Result::ok();
}

Result CalibrationManager::startIODevice()
{
	DBG("CalibrationManager::startIODevice");

	if (nullptr == ioDevice)
		return Result::fail("No IO device");

	DBG("CalibrationManager::startIODevice " + ioDevice->getName());

	StringArray inputNames(ioDevice->getInputChannelNames());
	StringArray outputNames(ioDevice->getOutputChannelNames());
	BigInteger inputChannels, outputChannels;
	inputChannels.setRange(0, inputNames.size(), true);
	outputChannels.setRange(0, outputNames.size(), true);
	String openResult(ioDevice->open(inputChannels, outputChannels, sampleRate, ioDevice->getDefaultBufferSize()));
    DBG("inputChannels:" + inputChannels.toString(16) + " outputChannels:" + outputChannels.toString(16) +
        " sampleRate:" + String(sampleRate,0));
	if (openResult.isNotEmpty())
	{
		DBG("CalibrationManager::startIODevice open error " + openResult);
		return Result::fail(openResult);
	}

	DBG("CalibrationManager::startIODevice open OK");

	ioDevice->start(&audioIOCallback);
	recordProgress = 0.0;
	recordStartTime = Time::getCurrentTime();

	DBG("CalibrationManager::startIODevice started");

	return Result::ok();
}

Result CalibrationManager::stopIODevice()
{
	if (ioDevice)
	{
		DBG("CalibrationManager::stopIODevice stop");
		ioDevice->stop();
		DBG("CalibrationManager::stopIODevice close");
		ioDevice->close();
		DBG("CalibrationManager::stopIODevice done");
	}

	return Result::ok();
}

void CalibrationManager::writeWaveFile( String name, AudioSampleBuffer &buffer, int const count )
{
	if (0 == count)
		return;

	WavAudioFormat format;
	File file("c://tmp//" + name);
	file.deleteFile();
	FileOutputStream* outputStream = new FileOutputStream(file);
	ScopedPointer<AudioFormatWriter> writer = format.createWriterFor(outputStream,
		48000.0,
		buffer.getNumChannels(),
		16,
		StringPairArray(),
		0);
	writer->writeFromAudioSampleBuffer(buffer, 0, count);
}

Result CalibrationManager::analyze(
	String const name,
	const float *data,
	int numSamples,
	SquareWaveAnalysisResult &positiveResult,
	SquareWaveAnalysisResult &negativeResult,
	float &totalResult,
	Range<float> const range
)
{
    int length;
    int positiveLength = 0;
    int negativeLength = 0;
    
	totalResult = 0.0f;

	int skipSamples = roundDoubleToInt(sampleRate * skipInitialDataSeconds);
	numSamples -= skipSamples;
	data += skipSamples;

	//
	// How many zero crossings should there be?  One for each phase of the square wave (*2.0), then times 1.5 for a fudge factor
	//
	int zeroCrossing1, zeroCrossing2;
	int zeroCrossingCount;
	int const maxExpectedZeroCrossings = roundDoubleToInt(1.5f * 2.0f * (double(numSamples) / sampleRate * audioIOCallback.getSquareWaveFrequency()));

	findZeroCrossing(data, numSamples, 0, zeroCrossing1);
	if (zeroCrossing1 < 0)
		return Result::fail(name + " - No signal detected");
	data += zeroCrossing1;
	numSamples -= zeroCrossing1;

	zeroCrossingCount = 1;

	int numPositiveCenterPoints = 0;
	int numNegativeCenterPoints = 0;

	positiveResult.clear(1.0f);
	negativeResult.clear(-1.0f);

	while (numSamples > 0)
	{
		findZeroCrossing(data, numSamples, zeroCrossing1, zeroCrossing2);
		if (zeroCrossing2 < 0)
			break;
		
		zeroCrossingCount++;
		if (zeroCrossingCount > maxExpectedZeroCrossings)
		{
			return Result::fail(name + " signal too noisy");
		}

		//DBG("Zero crossing at " << zeroCrossing2);

		length = zeroCrossing2 - zeroCrossing1;
		float sample = data[length / 2];

		//DBG("  Center sample " << sample);

		if (sample < 0.0f)
		{
			negativeResult.add(sample);
			numNegativeCenterPoints++;
            negativeLength += length;
		}
		else
		{
			positiveResult.add(sample);
			numPositiveCenterPoints++;
            positiveLength += length;
		}

		data += length;
		numSamples -= length;
		zeroCrossing1 = zeroCrossing2;
	}

	if (numNegativeCenterPoints)
	{
		negativeResult.average /= numNegativeCenterPoints;
	}
	if (numPositiveCenterPoints)
	{
		positiveResult.average /= numPositiveCenterPoints;
	}
    
    // make sure waveform is centered around ground (symmetrical)
    {
        if (0.0f == negativeResult.average)
        {
            return Result::fail(name + " negative result is zero");
        }
        
        float ratio = positiveResult.average / negativeResult.average;
        
        if (ratio < (1.0f - ratioTolerancePercent * 0.01f) || ratio > (1.0f + ratioTolerancePercent * 0.01f))
            return Result::fail(name + " ratio out of range " + String(ratio));
    }
    
    // make sure duty cycle is approximately 50%
    {
        int denominator = positiveLength + negativeLength;
        if (0 == denominator)
            return Result::fail(name + " failed duty cycle - zero length");
        
        float dutyCycle = float(positiveLength) / float(denominator);
        if (dutyCycle < minDutyCycle || dutyCycle > maxDutyCycle)
            return Result::fail(name + " invalid duty cycle " + String(dutyCycle));
    }

	totalResult = fabs(negativeResult.average) + positiveResult.average;
	if (range.contains(totalResult))
	{
		return Result::ok();
	}

	return Result::fail(name + " out of range (value " + String(totalResult,4) + ")");
}

void CalibrationManager::findZeroCrossing(const float * data, int numSamples, int startIndex, int &zeroCrossingIndex)
{
    int const periodThreshold = roundFloatToInt( (sampleRate / audioIOCallback.getSquareWaveFrequency()) * 0.4f);
    
	if (numSamples < 2)
	{
		zeroCrossingIndex = -1;
		return;
	}

	float previousSample = *data;
	numSamples--;
	data++;

	zeroCrossingIndex = startIndex + 1;
	while (numSamples > 0)
	{
		float sample = *data;

        if (((sample < 0.0f && previousSample >= 0.0f) ||
             (previousSample < 0.0f && sample >= 0.0f)) && ((zeroCrossingIndex - startIndex) > periodThreshold))
        {
            return;
        }

		zeroCrossingIndex++;
		numSamples--;
		data++;
	}

	zeroCrossingIndex = -1;
}

void CalibrationManager::execute()
{
    DBG("CalibrationManager::execute() " << (int)state);
    
	switch (state)
	{
#if 0
	case STATE_SHOW_ACTIVE_AIOS_CALIBRATION:
		{
			showActiveCalibration();
			return;
		}
		break;
#endif

	case STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE:
		{
			startCalibrateVoltageInputWithReferenceVoltage();
			return;
		}
		break;

	case STATE_ANALYZE_VOLTAGE_INPUT:
		{
			analyzeVoltageInput();

		}
		break;

	case STATE_STORE_VOLTAGE_INPUT_RESULTS:
		{
			storeVoltageInputResults();
		}
		break;

	case STATE_START_CALIBRATE_VOLTAGE_OUTPUT:
		{
			startCalibrateVoltageOutput();
		}
		break;

	case STATE_ANALYZE_VOLTAGE_OUTPUT:
		{
			analyzeVoltageOutput();
		}
		break;

	case STATE_STORE_VOLTAGE_OUTPUT_RESULTS:
		{
			storeVoltageOutputCalibration();
		}
		break;

	case STATE_START_CALIBRATE_CURRENT_INPUT:
		{
			startCalibrateCurrentInput();
		}
		break;

	case STATE_ANALYZE_CURRENT_INPUT:
		{
			analyzeCurrentInput();
		}
		break;

	case STATE_STORE_CURRENT_INPUT_RESULTS:
		{
			storeCurrentInputResults();
		}
		break;

	case STATE_START_RESISTANCE_MEASUREMENT:
		{
			startResistanceMeasurement();
		}
		break;

	case STATE_ANALYZE_RESISTANCE_MEASUREMENT:
		{
			analyzeResistanceMeasurement();
		}
		break;

	case STATE_CONNECT_PRODUCTION_TEST_ADAPTER:
		{
			connectProductionTestAdapter();
		}
		break;

	case STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK:
		{
			startCalibrateVoltageInputWithLoopback();
		}
		break;

// 	case STATE_STORE_VOLTAGE_AND_CURRENT_INPUTS:
// 		{
// 			storeVoltageAndCurrentInputCalibration();
// 		}
// 		break;

	case STATE_EXTERNAL_SPEAKER_MONITOR_READY:
		{
			externalSpeakerMonitorReady();
		}
		break;

	case STATE_FINISH_EXTERNAL_SPEAKER_MONITOR_TEST:
		{
			finishExternalSpeakerMonitorTest();
		}
		break;
            
    case STATE_FINISH_INTEGRATED_SPEAKER_MONITOR_TEST:
        {
            DBG("case STATE_FINISH_INTEGRATED_SPEAKER_MONITOR_TEST");
            finish();
        }
        break;

	case STATE_TIMED_OUT:
		{
			AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), "No audio data recorded - unable to proceed.", "Close");

            finish();
		}
		break;

	case STATE_CANCELLED:
		{
            finish();
        }
		break;
            
    default:
        break;
	}
}

void CalibrationManager::finish()
{
    DBG("CalibrationManager::finish");
    
    closeCalibrationDialog();
    
    stopIODevice();
    
    if (usbDevice)
    {
        uint8 moduleType = usbDevice->getDescription()->getModuleType(0);
        if (ACOUSTICIO_SPKRMON_MODULE == moduleType)
        {
            usbDevice->setAIOSReferenceVoltage(0, false);
        }
        usbDevice = nullptr;
    }
    
#if SPEAKER_MONITOR_TEST
    JUCEApplication::quit();
#endif
    
    messageListener->postMessage(new OldMessage(MESSAGE_AIOS_CALIBRATION_DONE,0,0,nullptr));
}

Result CalibrationManager::resetCalibration()
{
	int answer = AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, JUCEApplication::getInstance()->getApplicationName(),
		"Temporarily reset RAM calibration settings?  Calibration data will be still be saved in flash.", "Reset", "Don't reset");
	if (0 == answer)
        return Result::ok();

    calibrationDataAIOS.reset();
    calibrationDataAIOS.updateDate();
    calibrationDataAIOS.updateChecksum();
    
	Result resetResult(usbDevice->setCalibrationData(&calibrationDataAIOS.data));
	if (resetResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), resetResult.getErrorMessage(), "Close");
	}
    
    return resetResult;
}

Result CalibrationManager::eraseCalibration()
{
    int answer = AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, JUCEApplication::getInstance()->getApplicationName(),
                                              "Permanently erase calibration settings?  Calibration data will be erased from flash.", "Erase", "Don't erase");
    if (0 == answer)
        return Result::ok();
    
    calibrationDataAIOS.reset();
    calibrationDataAIOS.updateDate();
    calibrationDataAIOS.updateChecksum();
    
    Result resetResult(usbDevice->setCalibrationData(&calibrationDataAIOS.data));
    if (resetResult.failed())
    {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), resetResult.getErrorMessage(), "Close");
        
        return resetResult;
    }
    
    return writeActiveCalibrationToFlash();
}

void CalibrationManager::ModalDialogCallback::modalStateFinished(int returnValue)
{
	calibrationManager->modalStateFinished(returnValue);
}

void CalibrationManager::modalStateFinished(int returnValue)
{
	calibrationDialog = nullptr;

	switch (returnValue)
	{
	case CalibrationDialogComponent::CANCEL:
        switch (state)
        {
            case STATE_FINISH_INTEGRATED_SPEAKER_MONITOR_TEST:
            case STATE_CANCELLED:
                return;
                
            default:
                state = STATE_CANCELLED;
                execute();
                return;
        }
        break;

	case CalibrationDialogComponent::CALIBRATE:
		switch (state)
		{
#if 0
		case STATE_SHOW_ACTIVE_AIOS_CALIBRATION:
			{
				int button = AlertWindow::showOkCancelBox(AlertWindow::InfoIcon, JUCEApplication::getInstance()->getApplicationName(),
					"Please connect the external 10 ohm reference resistor.",
					"Proceed", "Cancel");
				if (0 == button)
				{
					return;
				}

				state = STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE;
				execute();
				return;
			}
#endif

		case STATE_SHOW_VOLTAGE_INPUT_RESULTS:
			{
				state = STATE_STORE_VOLTAGE_INPUT_RESULTS;
				execute();
				return;
			}

		case STATE_SHOW_VOLTAGE_OUTPUT_RESULTS:
			{
				state = STATE_STORE_VOLTAGE_OUTPUT_RESULTS;
				execute();
				return;
			}

		case STATE_SHOW_CURRENT_INPUT_RESULTS:
			{
				state = STATE_STORE_CURRENT_INPUT_RESULTS;
				execute();
				return;
			}
                
// 		case STATE_SHOW_VOLTAGE_AND_CURRENT_INPUTS:
// 			{
// 				state = STATE_STORE_VOLTAGE_AND_CURRENT_INPUTS;
// 				execute();
// 				return;
// 			}

        default:
            break;
		}
		return;

	case CalibrationDialogComponent::MEASURE:
		state = STATE_START_RESISTANCE_MEASUREMENT;
		execute();
		return;

	case CalibrationDialogComponent::CONTINUE:
		{
			switch (state)
			{
			case STATE_CONNECT_PRODUCTION_TEST_ADAPTER:
				openTestAdapter();
				break;

			case STATE_PROMPT_CONNECT_LOOPBACK_MIC2:
				state = STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK;
				execute();
				break;
                    
            default:
                break;
			}
			return;
		}
	case CalibrationDialogComponent::START_EXTERNAL_SPKRMON_TEST:
		{
			startExternalSpeakerMonitorTest();
			return;
		}

	case CalibrationDialogComponent::EXTERNAL_SPEAKER_MONITOR_DONE:
		{
			state = STATE_EXTERNAL_SPEAKER_MONITOR_READY;
			execute();
			return;
		}
            
    case CalibrationDialogComponent::RESET_RAM_DATA:
        {
            if (resetCalibration().wasOk())
            {
                showCalibrationDialog();
            }
            return;
        }
        
    case CalibrationDialogComponent::ERASE_FLASH_DATA:
        {
            if (eraseCalibration().wasOk())
            {
                showCalibrationDialog();
            }
            return;
        }
	}

	state = STATE_CANCELLED;
	execute();
}

void CalibrationManager::getSquareWaveResults(int const input, SquareWaveAnalysisResult &positiveResult, SquareWaveAnalysisResult &negativeResult) const
{
	positiveResult = positiveCalibrationResults[input];
	negativeResult = negativeCalibrationResults[input];
}

void CalibrationManager::showCalibrationDialog()
{
	if (calibrationDialog != nullptr)
	{
		calibrationDialog->update();
		return;
	}

	calibrationDialog = new CalibrationDialogComponent(this, usbDevice);
	calibrationDialog->addToDesktop(ComponentPeer::windowHasTitleBar |
		ComponentPeer::windowHasCloseButton |
		ComponentPeer::windowAppearsOnTaskbar,
		nullptr);
	Component* window = TopLevelWindow::getActiveTopLevelWindow();
	if (window)
		calibrationDialog->setBounds(window->getScreenBounds().withSizeKeepingCentre(400, 300));
	else
		calibrationDialog->setBounds(Desktop::getInstance().getDisplays().getMainDisplay().userArea.withSizeKeepingCentre(800, 600));
	calibrationDialog->update();
	calibrationDialog->enterModalState(true, new ModalDialogCallback(this), true);
}

double & CalibrationManager::getRecordProgress()
{
	return recordProgress;
}

Result CalibrationManager::setActiveCalibration()
{
	calibrationDataAIOS.updateDate();
	calibrationDataAIOS.updateChecksum();

	//
	// Store calibration results in device RAM
	//
	return usbDevice->setCalibrationData(&calibrationDataAIOS.data);
}


Result CalibrationManager::writeActiveCalibrationToFlash()
{
	AcousticIOCalibrationIndex calibrationIndex;

	uint32 numCalibrations;
	bool checksumOK;
	Result result(getFlashIndex(calibrationIndex,checksumOK));
	if (result.failed())
		return result;

	if (checksumOK && calibrationIndex.numCalibrations != 0)
	{
		calibrationIndex.numCalibrations++;
	}
	else
	{
		calibrationIndex.numCalibrations = 1;
	}
	
	//
	// Update the index block
	// 
	numCalibrations = calibrationIndex.numCalibrations;
	zerostruct(calibrationIndex.reserved);
	calibrationIndex.checksum = CRC32Block((uint32 const *)&calibrationIndex, sizeof(calibrationIndex) / sizeof(uint32) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);

	//
	// Write the calibration data to flash
	//
	uint8 calibrationBlock = (uint8)((numCalibrations - 1) % ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES);
	
	result = usbDevice->writeFlashBlock(calibrationBlock, (uint8 * const)&calibrationDataAIOS.data, sizeof(calibrationDataAIOS.data));
	if (result.failed())
		return result;

	//
	// Write the updated index block to flash
	//
	result = usbDevice->writeFlashBlock(ACOUSTICIO_CALIBRATION_INDEX_BLOCK, (uint8 * const)&calibrationIndex, sizeof(calibrationIndex));
	if (result.failed())
		return result;

	//usbDevice->dumpFlash();

	return Result::ok();
}

void CalibrationManager::closeCalibrationDialog()
{
	calibrationDialog.deleteAndZero();
}


Result CalibrationManager::getFlashIndex(AcousticIOCalibrationIndex &calibrationIndex, bool &checksumOK)
{
    zerostruct(calibrationIndex);
    
	Result result(usbDevice->readFlashBlock(ACOUSTICIO_CALIBRATION_INDEX_BLOCK, (uint8 * const)&calibrationIndex, sizeof(calibrationIndex)));
	if (result.failed())
		return result;

	uint32 checksum = CRC32Block((uint32 const *)&calibrationIndex, sizeof(calibrationIndex) / sizeof(uint32) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);
	checksumOK = checksum == calibrationIndex.checksum;

	return Result::ok();
}


String CalibrationManager::getHistory()
{
	//
	// Get the index block
	//
	AcousticIOCalibrationIndex calibrationIndex;
	bool checksumOK;

	Result result(getFlashIndex(calibrationIndex,checksumOK));
	if (result.failed())
		return String::empty;

	if (false == checksumOK)
		return "No calibration history found";

	String text("Unit calibration count: " + String(calibrationIndex.numCalibrations) + newLine + newLine);

	uint32 count = calibrationIndex.numCalibrations;
	int8 block = (uint8)(calibrationIndex.numCalibrations - 1) % ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES;
    count = jmin(count, (uint32)ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES);
	while (count != 0)
	{
		AIOSCalibrationData entry;
		result = usbDevice->readFlashBlock(block, (uint8 * const)&entry.data, sizeof(entry.data));
		if (result.failed())
			return String::empty;

		entry.validateChecksum();
		if (false == entry.isChecksumOK())
		{
			text += "Found invalid checksum" + newLine;
			break;
		}
		if (0 == entry.data.time)
		{
			text += "Found invalid time" + newLine;
			break;
		}
        
        text += entry.toString();
        text += newLine;
        
		block--;
        if (block < 0)
            block += ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES;
		count--;
	}

	return text;
}


void CalibrationManager::startExternalSpeakerMonitorCalibration(ReferenceCountedObjectPtr<ehw> device_)
{
    DBG("CalibrationManager::startExternalSpeakerMonitorCalibration");
    
	voltageInputChannel = EXTERNAL_SPKRMON_VOLTAGE_INPUT;
	currentInputChannel = EXTERNAL_SPKRMON_CURRENT_INPUT;
	voltageOutputChannel = EXTERNAL_SPKRMON_VOLTAGE_OUTPUT;

	createExternalSpeakerMonitorLogFile();
	log(newLine + "Begin calibration for external speaker monitor test");
	log(Time::getCurrentTime().toString(true, true, false));

	//state = STATE_EXTERNAL_SPEAKER_MONITOR_READY;
	state = STATE_CONNECT_PRODUCTION_TEST_ADAPTER;
	usbDevice = device_;

	limits = &limitsExternalSpeakerMonitor;

	Result configureResult(configureAIO());
	if (configureResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), configureResult.getErrorMessage(), "Close");
		return;
	}

	Result audioDeviceResult(createIODevice());
	if (audioDeviceResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), audioDeviceResult.getErrorMessage(), "Close");
		return;
	}

	execute();
}

#if 0
void CalibrationManager::showActiveCalibration()
{
	//
	// Read the RAM calibration data from the unit
	//
	AcousticIOCalibrationData temp;
	Result readResult(usbDevice->getCalibrationData(&temp));
	if (readResult.wasOk())
	{
		calibrationDataAIOS.data = temp;
		calibrationDataAIOS.validateChecksum();
	}
	else
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), readResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Show the dialog
	//
	showCalibrationDialog();
}
#endif

void CalibrationManager::startCalibrateVoltageInputWithReferenceVoltage()
{
    DBG("CalibrationManager::startCalibrateVoltageInputWithReferenceVoltage()");
    
	Result audioDeviceResult(createIODevice());
	if (audioDeviceResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), audioDeviceResult.getErrorMessage(), "Close");
        state = STATE_CANCELLED;
		return;
	}

	Result setResult(usbDevice->setMicGain(AIOS_VOLTAGE_INPUT_CHANNEL, 1));
	if (setResult.wasOk())
	{
		setResult = usbDevice->setMicGain(AIOS_CURRENT_INPUT_CHANNEL, 1);
		if (setResult.wasOk())
		{
			setResult = usbDevice->setConstantCurrent(AIOS_VOLTAGE_INPUT_CHANNEL, false);
			if (setResult.wasOk())
			{
				setResult = usbDevice->setConstantCurrent(AIOS_CURRENT_INPUT_CHANNEL, false);
				if (setResult.failed())
				{
                    results += setResult.getErrorMessage() + newLine;
                    
					AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setResult.getErrorMessage(), "Close");
                    state = STATE_CANCELLED;
					return;
				}
			}
		}
	}
    
    if (setResult.failed())
    {
        results += setResult.getErrorMessage() + newLine;
        state = STATE_CANCELLED;
        return;
    }

	calibrationDataAIOS.reset();
	setResult = setActiveCalibration();
	if (setResult.failed())
	{
        results += setResult.getErrorMessage() + newLine;
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setResult.getErrorMessage(), "Close");
        state = STATE_CANCELLED;
		return;
	}

	//
	// Turn on calibration reference voltage
	//
	Result setCVResult(usbDevice->setAIOSReferenceVoltage(0, true));
	if (setCVResult.failed())
	{
        results += setCVResult.getErrorMessage() + newLine;
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setCVResult.getErrorMessage(), "Close");
        state = STATE_CANCELLED;
		return;
	}

	//
	// Start the audio
	//
	audioIOCallback.squareWaveMaxAmplitude = 0.5f;
	audioIOCallback.squareWaveMinAmplitude = 0.0f;
	Result startIOResult(startIODevice());
	if (startIOResult.failed())
	{
        results += startIOResult.getErrorMessage() + newLine;
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), startIOResult.getErrorMessage(), "Close");
        state = STATE_CANCELLED;
		return;
	}

	//
	// Show the dialog
	//
	showCalibrationDialog();

	startTimer(200);
}

void CalibrationManager::analyzeVoltageInput()
{
	stopIODevice();

	uint8 const moduleType = usbDevice->getDescription()->getModuleType(0);
	if (ACOUSTICIO_SPKRMON_MODULE == moduleType)
	{
		Result setCVResult(usbDevice->setAIOSReferenceVoltage(0, false));
		if (setCVResult.failed())
		{
            results += setCVResult.getErrorMessage() + newLine;
			AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setCVResult.getErrorMessage(), "Close");
			return;
		}
	}

	Result analysisResult(analyze(voltageInputName,
		audioIOCallback.recordBuffer.getReadPointer(voltageInputChannel),
		audioIOCallback.recordPosition,
		positiveCalibrationResults[voltageInputChannel],
		negativeCalibrationResults[voltageInputChannel],
		voltage,
		limits->uncalibratedVoltageInput));
    
    results += "     Uncalibrated voltage input ratio: " + String(voltage, 4);
    if (analysisResult.wasOk())
    {
        results += " OK" + newLine;
    }
    else
    {
        results += " FAIL" + newLine;
    }
    
	if (analysisResult.wasOk())
	{
#if SHOW_INTERMEDIATE_RESULTS
		state = STATE_SHOW_VOLTAGE_INPUT_RESULTS;
		showCalibrationDialog();
#else
		state = STATE_STORE_VOLTAGE_INPUT_RESULTS;
		execute();
#endif
	}
	else
	{
		switch (moduleType)
		{
		case ACOUSTICIO_SPKRMON_MODULE:
			restart(analysisResult.getErrorMessage(), STATE_CANCELLED);
			break;

		case ACOUSTICIO_ANALOG_MODULE:
			cancel(analysisResult.getErrorMessage());
			break;
		}
	}
}

void CalibrationManager::storeVoltageInputResults()
{
    DBG("CalibrationManager::storeVoltageInputResults()");
    
	switch (usbDevice->getDescription()->getModuleType(0))
	{
	case ACOUSTICIO_SPKRMON_MODULE:
		{
            float ratio = expectedAIOSVoltageInputReferenceSignalResult / voltage;
			uint16 value = (uint16)(ratio * 32768.0f);
			calibrationDataAIOS.data.inputGains[AIOS_VOLTAGE_INPUT_CHANNEL] = value;
            
            DBG(calibrationDataAIOS.toString());
            
			Result storeResult(setActiveCalibration());
            DBG("Result " + storeResult.getErrorMessage());
			if (storeResult.failed())
			{
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), storeResult.getErrorMessage(), "Close");
				state = STATE_CANCELLED;
			}
			else
			{
				state = STATE_START_CALIBRATE_VOLTAGE_OUTPUT;
			}
            DBG(" state now " << (int)state);
			execute();
		}
		break;

	case ACOUSTICIO_ANALOG_MODULE:
		{
            DBG("ACOUSTICIO_ANALOG_MODULE");
            
            float ratio = expectedAIO2VoltageInputResult / voltage;
			calibrationDataExternal.voltageInputGain = ratio;

			log("Voltage input gain: " + ExternalSpeakerMonitorCalibrationData::printValue(ratio));

			state = STATE_EXTERNAL_SPEAKER_MONITOR_READY;
			showCalibrationDialog();
		}
		break;
	}
}

void CalibrationManager::startCalibrateVoltageOutput()
{
    DBG("CalibrationManager::startCalibrateVoltageOutput()");
    
	uint8 moduleType = usbDevice->getDescription()->getModuleType(0);
	Result setResult(Result::ok());

	switch (moduleType)
	{
	case ACOUSTICIO_ANALOG_MODULE:
	{
		Result audioDeviceResult(createIODevice());
		if (audioDeviceResult.failed())
		{
			AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), audioDeviceResult.getErrorMessage(), "Close");
			return;
		}

		setResult = usbDevice->setMicGain(voltageInputChannel, 1);
		if (setResult.wasOk())
		{
			setResult = usbDevice->setMicGain(currentInputChannel, 1);
			if (setResult.wasOk())
			{
				setResult = usbDevice->setConstantCurrent(voltageInputChannel, false);
				if (setResult.wasOk())
				{
					setResult = usbDevice->setConstantCurrent(currentInputChannel, false);
					if (setResult.wasOk())
					{
						setResult = usbDevice->setAmpGain(EXTERNAL_SPKRMON_VOLTAGE_OUTPUT, ACOUSTICIO_AMP_GAIN_10V_P2P);
					}
				}
			}
		}
		break;
	}
	case ACOUSTICIO_SPKRMON_MODULE:
	{
		setResult = usbDevice->setAmpGain(AIOS_VOLTAGE_OUTPUT_CHANNEL, ACOUSTICIO_AMP_GAIN_10V_P2P);
		break;
	}
	default:
	{
		jassertfalse;
		return;
	}
	}
	
	if (setResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Start the audio
	//
	audioIOCallback.squareWaveMaxAmplitude = voltageOutputAmplitude;
	audioIOCallback.squareWaveMinAmplitude = -voltageOutputAmplitude;
	Result startIOResult(startIODevice());
	if (startIOResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), startIOResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Show the dialog
	//
	switch (moduleType)
	{
	case ACOUSTICIO_ANALOG_MODULE:
	{
		showCalibrationDialog();
		break;
	}
	case ACOUSTICIO_SPKRMON_MODULE:
	{
		showCalibrationDialog();
		startTimer(200);
		break;
	}
	default:
	{
		jassertfalse;
		return;
	}
	}
}

void CalibrationManager::analyzeVoltageOutput()
{
	stopIODevice();

	Result analysisResult(analyze(voltageOutputName,
		audioIOCallback.recordBuffer.getReadPointer(voltageInputChannel),
		audioIOCallback.recordPosition,
		positiveCalibrationResults[voltageInputChannel],
		negativeCalibrationResults[voltageInputChannel],
		voltage,
		limits->voltageOutput));
    
    results += "     Uncalibrated voltage output ratio: " + String(voltage, 4);
    if (analysisResult.wasOk())
    {
        results += " OK" + newLine;
    }
    else
    {
        results += " FAIL" + newLine;
    }
    
	if (analysisResult.wasOk())
	{
#if SHOW_INTERMEDIATE_RESULTS
		state = STATE_SHOW_VOLTAGE_OUTPUT_RESULTS;
		showCalibrationDialog();
#else
		state = STATE_STORE_VOLTAGE_OUTPUT_RESULTS;
		execute();
#endif
	}
	else
	{
		switch (usbDevice->getDescription()->getModuleType(0))
		{
		case ACOUSTICIO_SPKRMON_MODULE:
			restart(analysisResult.getErrorMessage(), STATE_CANCELLED);
			break;

		case ACOUSTICIO_ANALOG_MODULE:
			cancel(analysisResult.getErrorMessage());
			break;
		}
	}
}

void CalibrationManager::storeVoltageOutputCalibration()
{
	switch (usbDevice->getDescription()->getModuleType(0))
	{
	case ACOUSTICIO_SPKRMON_MODULE:
		{
			float ratio = expectedVoltageOutputResult / voltage;
			uint16 value = (uint16)(ratio * 32768.0f);
			calibrationDataAIOS.data.outputGains[AIOS_VOLTAGE_OUTPUT_CHANNEL] = value;
            
            DBG(calibrationDataAIOS.toString());

			Result storeResult(setActiveCalibration());
			if (storeResult.failed())
			{
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), storeResult.getErrorMessage(), "Close");
				state = STATE_CANCELLED;
			}
			else
			{
				state = STATE_START_CALIBRATE_CURRENT_INPUT;
			}
		}

		execute();
		break;
	}
}

void CalibrationManager::startCalibrateCurrentInput()
{
	//
	// Start the audio
	//
	audioIOCallback.squareWaveMaxAmplitude = currentInputOutputAmplitude;
	audioIOCallback.squareWaveMinAmplitude = -currentInputOutputAmplitude;
	Result startIOResult(startIODevice());
	if (startIOResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), startIOResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Show the dialog
	//
	showCalibrationDialog();

	startTimer(200);
}

void CalibrationManager::analyzeCurrentInput()
{
	stopIODevice();

	switch (usbDevice->getDescription()->getModuleType(0))
	{
	case ACOUSTICIO_SPKRMON_MODULE:
		{
			Result voltageAnalysisResult(analyze(voltageInputName,
				audioIOCallback.recordBuffer.getReadPointer(voltageInputChannel),
				audioIOCallback.recordPosition,
				positiveCalibrationResults[voltageInputChannel],
				negativeCalibrationResults[voltageInputChannel],
				voltage,
				limits->calibratedVoltageInput));
			Result currentAnalysisResult(analyze(currentInputName,
				audioIOCallback.recordBuffer.getReadPointer(currentInputChannel),
				audioIOCallback.recordPosition,
				positiveCalibrationResults[currentInputChannel],
				negativeCalibrationResults[currentInputChannel],
				current,
				limits->currentInput));
            
            results += "     Calibrated voltage input ratio: " + String(voltage, 4);
            if (voltageAnalysisResult.wasOk())
            {
                results += " OK" + newLine;
            }
            else
            {
                results += " FAIL" + newLine;
            }
            
            results += "     Uncalibrated current input ratio: " + String(current, 4);
            if (currentAnalysisResult.wasOk())
            {
                results += " OK" + newLine;
            }
            else
            {
                results += " FAIL" + newLine;
            }

			if (voltageAnalysisResult.wasOk() && currentAnalysisResult.wasOk())
			{
#if SHOW_INTERMEDIATE_RESULTS
				state = STATE_SHOW_CURRENT_INPUT_RESULTS;
				showCalibrationDialog();
#else
				state = STATE_STORE_CURRENT_INPUT_RESULTS;
				execute();
#endif
			}
			else
			{
				String error(currentAnalysisResult.getErrorMessage());
				if (currentAnalysisResult.wasOk())
				{
					error = voltageAnalysisResult.getErrorMessage();
				}

				restart(error, STATE_CANCELLED);
			}
		}
		break;

	case ACOUSTICIO_ANALOG_MODULE:
		{
			Result currentAnalysisResult(analyze(currentInputName,
				audioIOCallback.recordBuffer.getReadPointer(currentInputChannel),
				audioIOCallback.recordPosition,
				positiveCalibrationResults[currentInputChannel],
				negativeCalibrationResults[currentInputChannel],
				current,
				limits->currentInput));

			if (currentAnalysisResult.wasOk())
			{
#if SHOW_INTERMEDIATE_RESULTS
				state = STATE_SHOW_CURRENT_INPUT_RESULTS;
				showCalibrationDialog();
#else
				state = STATE_STORE_CURRENT_INPUT_RESULTS;
				execute();
#endif
			}
			else
			{
				cancel(currentAnalysisResult.getErrorMessage());
			}
		}
		break;
	}
	

}

void CalibrationManager::storeCurrentInputResults()
{
	switch (usbDevice->getDescription()->getModuleType(0))
	{
	case ACOUSTICIO_SPKRMON_MODULE:
		{
			float ratio = voltage / current;
			ratio = ratio / expectedVoltageOverCurrent;
			uint16 value = (uint16)(ratio * 32768.0f);
			calibrationDataAIOS.data.inputGains[currentInputChannel] = value;
            
            DBG(calibrationDataAIOS.toString());

			Result storeResult(setActiveCalibration());
			if (storeResult.failed())
			{
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), storeResult.getErrorMessage(), "Close");
				state = STATE_CANCELLED;
			}
			else
			{
				Result writeResult(writeActiveCalibrationToFlash());
				if (writeResult.failed())
				{
					AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), storeResult.getErrorMessage(), "Close");
					state = STATE_CANCELLED;
				}
                
                DBG("Setting STATE_FINISH_INTEGRATED_SPEAKER_MONITOR_TEST");
				state = STATE_FINISH_INTEGRATED_SPEAKER_MONITOR_TEST;
			}

			execute();
		}
		break;

	case ACOUSTICIO_ANALOG_MODULE:
		{
			float ratio = 0.5f / current;
			calibrationDataExternal.currentInputGain = ratio;

			log("Current input gain: " + ExternalSpeakerMonitorCalibrationData::printValue(ratio));

			state = STATE_PROMPT_CONNECT_LOOPBACK_MIC2;
			showCalibrationDialog();
		}
		break;
	}
}

void CalibrationManager::startResistanceMeasurement()
{
    DBG("CalibrationManager::startResistanceMeasurement()");
    
	Result setResult(usbDevice->setMicGain(AIOS_VOLTAGE_INPUT_CHANNEL, 1));
	if (setResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setResult.getErrorMessage(), "Close");
		return;
	}
	setResult = usbDevice->setMicGain(AIOS_CURRENT_INPUT_CHANNEL, 1);
	if (setResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setResult.getErrorMessage(), "Close");
		return;
	}

	usbDevice->setAmpGain(AIOS_VOLTAGE_OUTPUT_CHANNEL, ACOUSTICIO_AMP_GAIN_10V_P2P);

	Result audioDeviceResult(createIODevice());
	if (audioDeviceResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), audioDeviceResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Start the audio
	//
	audioIOCallback.squareWaveMaxAmplitude = voltageOutputAmplitude;
	audioIOCallback.squareWaveMinAmplitude = -voltageOutputAmplitude;
	Result startIOResult(startIODevice());
	if (startIOResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), startIOResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Show the dialog
	//
	showCalibrationDialog();
	startTimer(200);
}

void CalibrationManager::analyzeResistanceMeasurement()
{
	stopIODevice();

	analyze(voltageInputName,
		audioIOCallback.recordBuffer.getReadPointer(AIOS_VOLTAGE_INPUT_CHANNEL),
		audioIOCallback.recordPosition,
		positiveCalibrationResults[AIOS_VOLTAGE_INPUT_CHANNEL],
		negativeCalibrationResults[AIOS_VOLTAGE_INPUT_CHANNEL],
		voltage,
		limits->calibratedVoltageInput);
	analyze(currentInputName,
		audioIOCallback.recordBuffer.getReadPointer(AIOS_CURRENT_INPUT_CHANNEL),
		audioIOCallback.recordPosition,
		positiveCalibrationResults[AIOS_CURRENT_INPUT_CHANNEL],
		negativeCalibrationResults[AIOS_CURRENT_INPUT_CHANNEL],
		current,
		limits->currentInput);

	state = STATE_SHOW_RESISTANCE_MEASUREMENT;
	showCalibrationDialog();
}

void CalibrationManager::connectProductionTestAdapter()
{
	showCalibrationDialog();
}

void CalibrationManager::openTestAdapter()
{
	testAdapter = new AIOTestAdapter();
	bool success(testAdapter->open());
	if (success)
	{
		testAdapter->write(0x30); //set adapter for audio measurements

		state = STATE_START_CALIBRATE_CURRENT_INPUT;
		execute();
		return;
	}
	else
	{
		int button = AlertWindow::showOkCancelBox(AlertWindow::InfoIcon, JUCEApplication::getInstance()->getApplicationName(),
			"The production test adapter was not detected. Please connect the production test adapter.",
			"Proceed", "Cancel");
		if (0 == button)
		{
			state = STATE_CANCELLED;
			return;
		}
	}

	showCalibrationDialog();
}

void CalibrationManager::startCalibrateVoltageInputWithLoopback()
{
	//
	// Start the audio
	//
	audioIOCallback.squareWaveMaxAmplitude = voltageInputOutputAmplitude;
	audioIOCallback.squareWaveMinAmplitude = -voltageInputOutputAmplitude;
	Result startIOResult(startIODevice());
	if (startIOResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), startIOResult.getErrorMessage(), "Close");
        state = STATE_CANCELLED;
		return;
	}

	//
	// Show the dialog
	//
	showCalibrationDialog();
	startTimer(200);
}


void CalibrationManager::externalSpeakerMonitorReady()
{
	showCalibrationDialog();
}

void CalibrationManager::startExternalSpeakerMonitorTest()
{
    DBG("CalibrationManager::startExternalSpeakerMonitorTest()");
    
#if SKIP_AIO2_CALIBRATION
	testAdapter = new AIOTestAdapter();
	bool success(testAdapter->open());
	if (!success)
	{
		int button = AlertWindow::showOkCancelBox(AlertWindow::InfoIcon, JUCEApplication::getInstance()->getApplicationName(),
			"The production test adapter was not detected. Please connect the production test adapter.",
			"Proceed", "Cancel");
		if (0 == button)
		{
			state = STATE_CANCELLED;
			execute();
			return;
		}
	}

	Result audioDeviceResult(createIODevice());
	if (audioDeviceResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), audioDeviceResult.getErrorMessage(), "Close");
		return;
	}

	Result setResult(configureAIO());
	if (setResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), setResult.getErrorMessage(), "Close");
		return;
	}
#endif

	int button = AlertWindow::showOkCancelBox(AlertWindow::InfoIcon, "Speaker Monitor Test",
		"Attach the reference resistor adapter to SPKR\n"
		"Plug in the speaker monitor power adapter\n\n"
		"Connect the speaker monitor to the AIO:\n"
		"AMPS to MIC1\n"
		"VOLTS to MIC2\n"
		"AIO to AMP1\n",
		"Continue","Cancel");
	if (0 == button)
	{
		state = STATE_EXTERNAL_SPEAKER_MONITOR_READY;
		execute();
		return;
	}

	log("Begin speaker monitor test " + serialNumber);
	log(Time::getCurrentTime().toString(true, true, false));

	//
	// LED check
	//
	button = AlertWindow::showYesNoCancelBox(AlertWindow::QuestionIcon, "Speaker Monitor Test",
		"Is the speaker monitor green PWR LED glowing?",
		"Yes", "No", "Cancel");
	switch (button)
	{
	case 1:
		powerLED = true;
		log("Power LED is on");
		break;

	case 2:
		powerLED = false;
		log("Power LED is off");
		break;

	default:
		state = STATE_EXTERNAL_SPEAKER_MONITOR_READY;
		execute();
		return;
	}


	//
	// Start the audio
	//
	audioIOCallback.squareWaveMaxAmplitude = voltageOutputAmplitude;
	audioIOCallback.squareWaveMinAmplitude = -voltageOutputAmplitude;
	Result startIOResult(startIODevice());
	if (startIOResult.failed())
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), startIOResult.getErrorMessage(), "Close");
		return;
	}

	//
	// Show the dialog
	//
	state = STATE_START_EXTERNAL_SPEAKER_MONITOR_TEST;
	showCalibrationDialog();
	startTimer(200);
}

void CalibrationManager::finishExternalSpeakerMonitorTest()
{
	stopIODevice();

	Result voltageAnalysisResult(analyze(voltageInputName,
		audioIOCallback.recordBuffer.getReadPointer(voltageInputChannel),
		audioIOCallback.recordPosition,
		positiveCalibrationResults[voltageInputChannel],
		negativeCalibrationResults[voltageInputChannel],
		voltage,
		limits->calibratedVoltageInput));
	Result currentAnalysisResult(analyze(currentInputName,
		audioIOCallback.recordBuffer.getReadPointer(currentInputChannel),
		audioIOCallback.recordPosition,
		positiveCalibrationResults[currentInputChannel],
		negativeCalibrationResults[currentInputChannel],
		current,
		limits->currentInput));

	showCalibrationDialog();
}

void CalibrationManager::getExternalSpeakerMonitorTestResult(ExternalSpeakerMonitorTestResult &result)
{
	result.voltage = fabs(negativeCalibrationResults[voltageInputChannel].average) + fabs(positiveCalibrationResults[voltageInputChannel].average);
	result.current = fabs(negativeCalibrationResults[currentInputChannel].average) + fabs(positiveCalibrationResults[currentInputChannel].average);
	if (0.0f == result.current || 0.0f == result.voltage)
	{
		result.message = "Invalid test results - check cables and reference resistor adapter.";
		result.passed = false;
		result.resistance = 0.0f;
	}
	else
	{
		result.voltage *= calibrationDataExternal.voltageInputGain;
		result.current *= calibrationDataExternal.currentInputGain;
		float ratio = result.voltage / result.current;
		result.resistance = (ratio / expectedVoltageOverCurrent) * referenceResistorOhms;

		if (result.resistance <= referenceResistorOhms * (1.0f - externalSpeakerMonitorTolerance) || result.resistance >= referenceResistorOhms * (1.0f + externalSpeakerMonitorTolerance))
		{
			result.passed = false;
			result.message = "Out of range - resistance is: " + String(result.resistance, 4) + " ohms";
		}
		else
		{
			result.passed = true;
			result.message = "Voltage: " + String(result.voltage, 4) + " Current: " + String(result.current, 4);
		}
	}

	result.passed &= powerLED;
	if (false == powerLED)
		result.message += " (no power LED)";

	log(result.message);
	String text("Pass");
	if (false == result.passed)
		text = "Fail";

	log(text + " - measured resistance: " + String(result.resistance,4) + " ohms" + newLine);
}

Result CalibrationManager::setSerialNumber(String serialNumber_)
{
	if (serialNumber_.isEmpty())
		return Result::fail("Please enter the speaker monitor serial number");

	if (serialNumber_.length() != serialNumberLength ||
		false == serialNumber_.containsOnly("0123456789"))
	{
		return Result::fail("Invalid serial number " + serialNumber_);
	}

	serialNumber = serialNumber_;
	return Result::ok();
}

void CalibrationManager::createExternalSpeakerMonitorLogFile()
{
	File logfolder(getOutputFolder());

	logfile = logfolder.getChildFile("SpeakerMonitorTestLog.txt");
	logStream = new FileOutputStream(logfile);
}

File CalibrationManager::getOutputFolder()
{
	File logfolder;

#if JUCE_WIN32
	logfolder = File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
#endif
#if JUCE_MAC
	logfolder = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory();
#endif

	return logfolder;
}


void CalibrationManager::log(String const text)
{
	if (logStream)
	{
		logStream->writeText(text + newLine, false, false);
	}
}


void CalibrationManager::restart(String error, State newState)
{
#if 0
	error += ". Make sure the reference resistor adapter is attached.";
	AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), error, "Close");
	state = newState;
	showCalibrationDialog();
#else
    cancel(error);
#endif
}


void CalibrationManager::cancel(String error)
{
	error += ". Make sure the reference resistor adapter is attached.";
	AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), error, "Close");
	state = STATE_CANCELLED;
	execute();
}

Result CalibrationManager::configureAIO()
{
	Result setResult(usbDevice->setMicGain(voltageInputChannel, 1));
	if (setResult.wasOk())
	{
		setResult = usbDevice->setMicGain(currentInputChannel, 1);
		if (setResult.wasOk())
		{
			setResult = usbDevice->setConstantCurrent(voltageInputChannel, false);
			if (setResult.wasOk())
			{
				setResult = usbDevice->setConstantCurrent(currentInputChannel, false);
				if (setResult.wasOk())
				{
					setResult = usbDevice->setAmpGain(voltageOutputChannel, ACOUSTICIO_AMP_GAIN_10V_P2P);
				}
			}
		}
	}
    
    if (setResult.failed())
    {
        results += setResult.getErrorMessage() + newLine;
    }

	return setResult;
}

