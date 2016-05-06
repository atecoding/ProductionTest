#pragma once
#include "CalibrationComponentV2.h"
#include "SquareWaveAnalysisResult.h"
#include "CalibrationDataV2.h"
#include "CalibrationUnit.h"
#include "CalibrationAudioIO.h"
#include "CalibrationProcedure.h"

struct CalibrationManagerConfiguration
{
    ReferenceCountedObjectPtr<USBDevice> aioUSBDevice;
    AudioIODevice* audioIODevice;
    int firstModule;
    int numModulesToCalibrate;
    bool writeToFlash;
};

class CalibrationManagerV2 : public AsyncUpdater
{
public:
	CalibrationManagerV2(USBDevices* devices_);
	virtual ~CalibrationManagerV2();

    void aioChanged();
	Result userAction(int const action);
    
    void audioRecordDone();

	typedef enum
	{
		STATE_IDLE,
        STATE_SHOW_ACTIVE_CALIBRATION,
        STATE_PREPARING,
        STATE_MODULE_READY,
        STATE_RECORDING,
        STATE_ANALYZING,
        STATE_ERROR,
        
        NUM_STATES
	} State;
	
	enum
	{
		ACTION_CANCEL,
		ACTION_CALIBRATE,
		ACTION_MEASURE,
		ACTION_CONTINUE,
		ACTION_RESTART_MODULE_CALIBRATION,
		ACTION_RESET_RAM_DATA,
		ACTION_ERASE_FLASH_DATA
	};

	State getState() const
	{
        int value = (int)state.getValue();
        jassert(0 <= value && value < NUM_STATES);
        return (State)value;
	}
    
    Value const getStateValue() const
    {
        return state;
    }
    
	double &getRecordProgress();

	String getHistory();

	CalibrationProcedure * const getProcedure() const
    {
        return unit.getProcedure();
    }
    
    CalibrationDataV2 const &getData() const
    {
        return unit.calibrationData;
    }
    
    Result const getResult() const
    {
        return result;
    }

	bool isUnitDone() const
	{
		return unit.isDone();
	}

	void setDevices(ReferenceCountedObjectPtr<USBDevice> aioUSBDevice_, AudioIODevice* audioIODevice_);

    void configure(CalibrationManagerConfiguration& configuration);
    
protected:
    Result userActionCancel();
    Result userActionCalibrate();
    Result userActionRestartModuleCalibration();
    Result userActionResetRAMData();
    Result userActionEraseFlashData();
    Result startUnitCalibration();
    Result runModuleProcedureStage();
    Result startAudioIO();
    Result restartModuleCalibration();
    void finishUnitModule();
    
    virtual void handleAsyncUpdate() override;
    
    Value state;
    Result result;
    
    CalibrationUnit unit;
    CalibrationAudioIO audioIO;
};
