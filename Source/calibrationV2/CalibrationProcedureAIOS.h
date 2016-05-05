#pragma once

#include "CalibrationProcedureAIOA.h"

class CalibrationProcedureAIOS : public CalibrationProcedureAIOA
{
public:
    CalibrationProcedureAIOS(CalibrationUnit * const calibrationUnit_, AIOModule * const module_) :
    CalibrationProcedureAIOA(calibrationUnit_, module_)
    {
        DBG("CalibrationProcedureAIOS::CalibrationProcedureAIOS");
    }
    
    virtual int getNumCalibrationStages() const override
    {
        //
        // Needs to include STAGE_CHECK_ADAPTER_CONNECTIONS, plus the other STAGE_CALIBRATE_AIOS_??? values below
        //
        return 6;
    }
    
    virtual Result prepareModuleForCalibration() override;
    
    virtual void fillOutputBuffer(AudioBuffer<float> outputBuffer) override;
    
    virtual Result analyzeRecording(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData) override;
    
    virtual Result finishStage() override;
    
    virtual Result finishModuleCalibration() override;
    
    virtual String getConnectPrompt() const override;
    
    virtual String getProgressLabelText() const override;
    
    void cancelCalibration() override;
    
protected:
    enum
    {
        STAGE_CALIBRATE_AIOS_VMON_INPUT = 300,
        STAGE_CALIBRATE_AIOS_VOUT,
        STAGE_CALIBRATE_AIOS_IMON_INPUT,
        STAGE_CALIBRATE_AIOS_MIC_INPUTS,
        STAGE_CALIBRATE_AIOS_AMP_OUTPUTS,
        
        VMON_INPUT_CHANNEL = 2,
        IMON_INPUT_CHANNEL,
        VOUT_CHANNEL = 0,
        AIOS_AMP_OUTPUT_CHANNEL
    };
    
    Result prepareStageCalibrateVmonInput();
    Result prepareStageCalibrateVout();
    Result prepareStageCalibrateImon();
    Result calibrateVmonInput(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
    Result calibrateVout(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
    Result calibrateImonInput(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
};
