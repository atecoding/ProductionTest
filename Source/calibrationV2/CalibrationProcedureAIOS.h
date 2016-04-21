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
        return STAGE_CALIBRATE_IMON_INPUT + 1;
    }
    
    virtual Result prepareModuleForCalibration() override;
    
    virtual void fillOutputBuffer(AudioBuffer<float> outputBuffer) override;
    
    virtual Result analyzeRecording(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData) override;
    
    Result finishModuleCalibration() override;
    
    virtual String getConnectPrompt() const override;
    
    virtual String getProgressLabelText() const override;
    
    void cancelCalibration() override;
    
protected:
    enum
    {
        STAGE_CALIBRATE_MIC_AND_VMON_INPUTS = STAGE_CALIBRATE_MIC_INPUTS,
        STAGE_CALIBRATE_AMP_AND_VOUT_OUTPUTS = STAGE_CALIBRATE_AMP_OUTPUTS,
        STAGE_CALIBRATE_IMON_INPUT,
        
        VMON_INPUT_CHANNEL = 2,
        IMON_INPUT_CHANNEL,
        VOLTAGE_OUTPUT_CHANNEL = 0,
        AMP_OUTPUT_CHANNEL
    };
    
    Result prepareStageCalibrateMicAndVmonInputs();
    Result prepareStageCalibrateAmpAndVout();
    Result prepareStageCalibrateImon();
    Result calibrateMicAndVmonInputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
    Result calibrateAmpOutputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
    Result calibrateImonInput(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
};
