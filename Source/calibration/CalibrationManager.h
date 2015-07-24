#pragma once
#include "CalibrationDialogComponent.h"
#include "CalibrationAudioIOCallback.h"
#include "SquareWaveAnalysisResult.h"
#include "CalibrationData.h"
#include "../AIOTestAdapter.h"
#include "ExternalSpeakerMonitorTestResult.h"

class ehw;

class CalibrationManager : public Timer
{
public:
	CalibrationManager(MessageListener* messageListener_);
	virtual ~CalibrationManager();

	void startIntegratedSpeakerMonitorCalibration(ReferenceCountedObjectPtr<ehw> device_);
	void startExternalSpeakerMonitorCalibration(ReferenceCountedObjectPtr<ehw> device_);
    void startResistanceMeasurement(ReferenceCountedObjectPtr<ehw> device_);

	virtual void timerCallback() override;
	void modalStateFinished(int returnValue);

	void openTestAdapter();

	typedef enum
	{
		STATE_IDLE,

		//STATE_SHOW_ACTIVE_AIOS_CALIBRATION,
		
		STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE, 
		STATE_ANALYZE_VOLTAGE_INPUT,
		STATE_SHOW_VOLTAGE_INPUT_RESULTS,
		STATE_STORE_VOLTAGE_INPUT_RESULTS,

		STATE_START_CALIBRATE_VOLTAGE_OUTPUT,
		STATE_ANALYZE_VOLTAGE_OUTPUT,
		STATE_SHOW_VOLTAGE_OUTPUT_RESULTS,
		STATE_STORE_VOLTAGE_OUTPUT_RESULTS,

		STATE_START_CALIBRATE_CURRENT_INPUT,
		STATE_ANALYZE_CURRENT_INPUT,
		STATE_SHOW_CURRENT_INPUT_RESULTS,
		STATE_STORE_CURRENT_INPUT_RESULTS,

		STATE_START_RESISTANCE_MEASUREMENT,
		STATE_ANALYZE_RESISTANCE_MEASUREMENT,
        STATE_SHOW_RESISTANCE_MEASUREMENT,
		STATE_RESISTANCE_MEASUREMENT_DONE,

		STATE_CONNECT_PRODUCTION_TEST_ADAPTER,
		STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK,
		STATE_PROMPT_CONNECT_LOOPBACK_MIC2,
		STATE_EXTERNAL_SPEAKER_MONITOR_READY,
		STATE_START_EXTERNAL_SPEAKER_MONITOR_TEST,
		STATE_FINISH_EXTERNAL_SPEAKER_MONITOR_TEST,
        
        STATE_FINISH_INTEGRATED_SPEAKER_MONITOR_TEST,

		STATE_CANCELLED,
		STATE_TIMED_OUT
	} State;

	enum 
	{
		EXTERNAL_SPKRMON_CURRENT_INPUT = 0,
		EXTERNAL_SPKRMON_VOLTAGE_INPUT = 1,
		EXTERNAL_SPKRMON_VOLTAGE_OUTPUT = 0
	};

	State getState() const
	{
		return state;
	}

	double &getRecordProgress();
	void getSquareWaveResults(int const channel, SquareWaveAnalysisResult &positiveResult, SquareWaveAnalysisResult &negativeResult) const;
	void getExternalSpeakerMonitorTestResult(ExternalSpeakerMonitorTestResult &result);

	String getHistory();

	String getSerialNumber() const
	{
		return serialNumber;
	}
	Result setSerialNumber(String serialNumber_);
    
    String getResults(bool &pass_)
    {
        pass_ = pass;
        return results;
    }

	AIOSCalibrationData calibrationDataAIOS;
	ExternalSpeakerMonitorCalibrationData calibrationDataExternal;
    
    static const float AIOSReferencePeakVolts;
    static const float voltageInputPeakVolts;
    static const float voltageOutputPeakVolts;
    static const float expectedAIOSVoltageInputReferenceSignalResult;
    static const float expectedAIOSVoltageInputWithCalibratedOutputResult;
    static const float expectedAIO2VoltageInputResult;
    static const float expectedVoltageOutputResult;
    static const float expectedVoltageOverCurrent;
    
	static const float voltMeterMin;
	static const float voltMeterMax;
	static const int serialNumberLength;

	struct Limits
	{
		Limits( float const uncalibratedVoltageInputMin,
                float const uncalibratedVoltageInputMax,
                float const voltageOutputMin,
                float const voltageOutputMax,
                float const calibratedVoltageInputMin,
                float const calibratedVoltageInputMax,
                float const currentInputMin,
                float const currentInputMax) :
            uncalibratedVoltageInput(uncalibratedVoltageInputMin, uncalibratedVoltageInputMax),
			voltageOutput(voltageOutputMin, voltageOutputMax),
			calibratedVoltageInput(calibratedVoltageInputMin, calibratedVoltageInputMax),
			currentInput(currentInputMin, currentInputMax)
		{
		}

        Range<float> const uncalibratedVoltageInput;
		Range<float> const voltageOutput;
        Range<float> const calibratedVoltageInput;
		Range<float> const currentInput;

		JUCE_DECLARE_NON_COPYABLE(Limits);
	};

	static Limits limitsExternalSpeakerMonitor;
	static Limits limitsAIOS;

protected:
	State state;
	ReferenceCountedObjectPtr<ehw> usbDevice;
	Component::SafePointer<CalibrationDialogComponent> calibrationDialog;
	ScopedPointer<AudioIODeviceType> ioDeviceType;
	ScopedPointer<AudioIODevice> ioDevice;
	ScopedPointer<AIOTestAdapter> testAdapter;
    
    String results;
    bool pass;

	class ModalDialogCallback : public ModalComponentManager::Callback
	{
	public:
		ModalDialogCallback(CalibrationManager *calibrationManager_) :
			calibrationManager(calibrationManager_)
		{
		}
		CalibrationManager *calibrationManager;
		virtual void modalStateFinished(int returnValue) override;
	};

	void execute();
    void finish(int messageCode);

	//void showActiveCalibration();
	void startCalibrateVoltageInputWithReferenceVoltage();
	void analyzeVoltageInput();
	void storeVoltageInputResults();
	void startCalibrateVoltageOutput();
	void analyzeVoltageOutput();
	void storeVoltageOutputCalibration();
	void startCalibrateCurrentInput();
	void analyzeCurrentInput();
	void storeCurrentInputResults();
	void startResistanceMeasurement();
	void analyzeResistanceMeasurement();
	void connectProductionTestAdapter();
	void startCalibrateVoltageInputWithLoopback();

	void showCalibrationDialog();
	void closeCalibrationDialog();

	Result createIODevice();
	Result startIODevice();
	Result stopIODevice();
	Result analyze(String const name,
		const float *data, 
		int numSamples, 
		SquareWaveAnalysisResult &positiveResult, 
		SquareWaveAnalysisResult &negativeResult,
		float &totalResult,
		Range<float> const range);
	void writeWaveFile(String name, AudioSampleBuffer &buffer, int const count);
	void findZeroCrossing(const float * data, int numSamples, int startIndex, int &zeroCrossingIndex);
	void externalSpeakerMonitorReady();
	Result configureAIO();
	Result setActiveCalibration();
	Result writeActiveCalibrationToFlash();
	Result getFlashIndex(AcousticIOCalibrationIndex &calibrationIndex, bool &checksumOK);
	Result resetCalibration();
    Result eraseCalibration();
	void startExternalSpeakerMonitorTest();
	void finishExternalSpeakerMonitorTest();

	void createExternalSpeakerMonitorLogFile();
	void log(String const text);

	File getOutputFolder();
	void restart(String error, State newState);
	void cancel(String error);
	File logfile;
	ScopedPointer <FileOutputStream> logStream;

	CalibrationAudioIOCallback audioIOCallback;

	SquareWaveAnalysisResult negativeCalibrationResults[8];
	SquareWaveAnalysisResult positiveCalibrationResults[8];
	uint8 voltageInputChannel;
	uint8 currentInputChannel;
	uint8 voltageOutputChannel;
	float voltage;
	float current;
	bool powerLED;

	double recordProgress;
	Time recordStartTime;
	String serialNumber;
	Limits const *limits;
    MessageListener* messageListener;

	static const String voltageInputName;
	static const String voltageOutputName;
	static const String currentInputName;
};
