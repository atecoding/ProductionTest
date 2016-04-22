#pragma once
#include "CalibrationComponentV2.h"
#include "SquareWaveAnalysisResult.h"
#include "CalibrationDataV2.h"
#include "CalibrationUnit.h"
#include "CalibrationAudioIO.h"
#include "CalibrationProcedure.h"

class CalibrationManagerV2
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
    
    void addStateListener(Value::Listener* listener_)
    {
        state.addListener(listener_);
    }
    
    void removeStateListener(Value::Listener* listener_)
    {
        state.removeListener(listener_);
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

	bool isUnitCalibrated() const
	{
		return unit.isCalibrated();
	}

	void setDevices(ReferenceCountedObjectPtr<USBDevice> aioUSBDevice_, AudioIODevice* audioIODevice_);
    
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
    
    Value state;
    Result result;
    
    CalibrationUnit unit;
    CalibrationAudioIO audioIO;
};
