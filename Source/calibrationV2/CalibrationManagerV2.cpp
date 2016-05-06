#include "../base.h"
#include "CalibrationManagerV2.h"
#include "../USBDevice.h"
#include "../Description.h"

CalibrationManagerV2::CalibrationManagerV2(USBDevices* devices_) :
state(STATE_IDLE),
result(Result::ok()),
unit(devices_, *this),
audioIO(*this)
{
}

CalibrationManagerV2::~CalibrationManagerV2()
{
	//stopIODevice();
}

void CalibrationManagerV2::aioChanged()
{
    if (unit.aioUSBDevice)
    {
        state = STATE_SHOW_ACTIVE_CALIBRATION;
    }
    else
    {
        state = STATE_IDLE;
    }
    
#if JUCE_MAC
    audioIO.setCoreAudioName(unit.getCoreAudioName());
#endif
}


Result CalibrationManagerV2::userAction(int const action)
{
    switch (action)
    {
        case ACTION_CANCEL:
            return userActionCancel();
            
        case ACTION_CALIBRATE:
            return userActionCalibrate();
            
        case ACTION_RESTART_MODULE_CALIBRATION:
            return userActionRestartModuleCalibration();
            
        case ACTION_RESET_RAM_DATA:
            return userActionResetRAMData();
            
        case ACTION_ERASE_FLASH_DATA:
            return userActionEraseFlashData();
            
        default:
            break;
    }
    
    return Result::ok();
}

Result CalibrationManagerV2::userActionCancel()
{
    audioIO.stop();
    unit.cancelCalibration();
    
    state = STATE_SHOW_ACTIVE_CALIBRATION;
    
    return Result::ok();
}

Result CalibrationManagerV2::userActionCalibrate()
{
    switch ((int)state.getValue())
    {
        case STATE_SHOW_ACTIVE_CALIBRATION:
            return startUnitCalibration();
            
        case STATE_MODULE_READY:
            return runModuleProcedureStage();
            
        default:
            break;
    }
    
    return Result::ok();
}


Result CalibrationManagerV2::userActionRestartModuleCalibration()
{
    switch ((int)state.getValue())
    {
        case STATE_ERROR:
            return restartModuleCalibration();
            
        default:
            break;
    }
    
    return Result::ok();
}

Result CalibrationManagerV2::startUnitCalibration()
{
    DBG("CalibrationManagerV2::startUnitCalibration()");
    
    //
    // Call CalibrationUnit::prepareForCalibration to set up the entire unit
    //
    result = unit.prepareUnitForCalibration();
    if (result.wasOk())
    {
        //
        // Tell the unit to create the procedure for the first module
        //
        result = unit.createModuleProcedure();
        if (result.wasOk())
        {
            //
            // It's possible that this module does not need to be calibrated
            // and should be skipped
            //
            if (unit.isModuleProcedureDone())
            {
                finishUnitModule();
            }
            else
            {
                state = STATE_MODULE_READY;
            }
            return result;
        }
    }
    
    state = STATE_ERROR;
    return result;
}


Result CalibrationManagerV2::restartModuleCalibration()
{
    //
    // Tell the unit to re-create the procedure for the first module
    // to start over
    //
    result = unit.createModuleProcedure();
    if (result.wasOk())
    {
        state = STATE_MODULE_READY;
    }
    
    return result;
}

Result CalibrationManagerV2::runModuleProcedureStage()
{
    DBG("CalibrationManagerV2::runModuleProcedureStage()");
    
    //
    // Tell the unit to run one stage of the procedure
    //
    result = unit.runModuleProcedureStage();
    if (result.wasOk())
    {
        //
        // Start the audio I/O
        //
        result = startAudioIO();
        if (result.wasOk())
        {
            return result;
        }
    }
    
    state = STATE_ERROR;
    return result;
}


Result CalibrationManagerV2::startAudioIO()
{
    DBG("CalibrationManagerV2::startAudioIO()");
    
    result = audioIO.start(unit.getProcedure()->getSampleRate());
    if (result.wasOk())
    {
        state = STATE_RECORDING;
        return result;
    }
    
        DBG("CalibrationManagerV2::startAudioIO() - failed to start audio I/O");
        state = STATE_ERROR;
    
    return result;
}


void CalibrationManagerV2::audioRecordDone()
{
    DBG("CalibrationManagerV2::audioRecordDone()");
    triggerAsyncUpdate();
}

void CalibrationManagerV2::handleAsyncUpdate()
{
    DBG("CalibrationManagerV2::handleAsyncUpdate()");
    
    audioIO.stop();
    
    //
    // Recording OK?
    //
    result = audioIO.getResult();
    if (result.failed())
    {
        DBG("Error result from AudioIO object " + result.getErrorMessage());
        state = STATE_ERROR;
        return;
    }
    
    //
    // Analyze the results for this stage of the procedure
    //
    state = STATE_ANALYZING;
    
    result = unit.analyzeRecording(audioIO.recordBuffer);
    if (result.failed())
    {
        DBG("CalibrationManagerV2::audioRecordDone() - analyzeRecording failed");
        state = STATE_ERROR;
        return;
    }
    
    //
    // Tell the unit to finish the procedure
    //
    result = unit.finishModuleProcedureStage();
    if (result.failed())
    {
        DBG("CalibrationManagerV2::audioRecordDone() - unit.finishModuleProcedureStage() failed");
        state = STATE_ERROR;
        return;
    }
    
    //
    // Is this module done?
    //
    if (unit.isModuleProcedureDone())
    {
        finishUnitModule();
    }
    else
    {
        //
        // Continue running the procedure for this module
        //
        result = runModuleProcedureStage();
        if (result.wasOk())
        {
            return;
        }
    }
    
    DBG("Failed to continue or finish module procedure");
                state = STATE_ERROR;
}


void CalibrationManagerV2::finishUnitModule()
{
    //
    // Finish up this module, then move to the next module
    //
    result = unit.finishModuleCalibration();
    if (result.wasOk())
    {
        DBG("CalibrationManagerV2::finishUnitModule() - module is done   state:" << (int)state.getValue());
        if (unit.isDone())
        {
            state = STATE_SHOW_ACTIVE_CALIBRATION;
        }
        else
        {
            result = unit.createModuleProcedure();
            if (result.wasOk())
            {
                state = STATE_MODULE_READY;
            }
        }
    }
}


double & CalibrationManagerV2::getRecordProgress()
{
    return audioIO.getRecordProgress();
}

String CalibrationManagerV2::getHistory()
{
    return unit.getHistory();
}

void CalibrationManagerV2::setDevices(ReferenceCountedObjectPtr<USBDevice> aioUSBDevice_, AudioIODevice* audioIODevice_)
{
	unit.setAIOUSBDevice(aioUSBDevice_);
	audioIO.setDevice(audioIODevice_);
}

void CalibrationManagerV2::configure(CalibrationManagerConfiguration& configuration)
{
	unit.configure(configuration);
	audioIO.setDevice(configuration.audioIODevice);
}

Result CalibrationManagerV2::userActionResetRAMData()
{
    return unit.resetRAMCalibrationData();
}

Result CalibrationManagerV2::userActionEraseFlashData()
{
    return unit.eraseFlashCalibrationData();
}

