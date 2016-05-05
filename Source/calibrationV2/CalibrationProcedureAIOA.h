#pragma once

#include "CalibrationProcedure.h"

class CalibrationProcedureAIOA : public CalibrationProcedure
{
public:
    CalibrationProcedureAIOA(CalibrationUnit * const calibrationUnit_, AIOModule * const module_);
    
    virtual double getRecordLengthSeconds() const override;
    
    virtual bool isDone() const override
    {
        return STAGE_MODULE_CALIBRATION_DONE == stage;
    }
    
    virtual int getNumCalibrationStages() const override
    {
        return STAGE_CALIBRATE_AIOA_AMP_OUTPUTS - STAGE_CHECK_ADAPTER_CONNECTIONS + 1;
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
        STAGE_CHECK_ADAPTER_CONNECTIONS = 200,
        STAGE_CALIBRATE_AIOA_MIC_INPUTS,
        STAGE_CALIBRATE_AIOA_AMP_OUTPUTS,
        
        STAGE_MODULE_CALIBRATION_DONE = 1000000,
        
        FIRST_MIC_INPUT_CHANNEL = 0
    };
    
    void updateUncalibratedOutputLimits();
    Result setGains();
    Result prepareStageCheckAdapterConnections();
    Result prepareStageCalibrateMics();
    Result prepareStageCalibrateAmps();
    Result checkAmpConnection(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, int const firstInput, int const lastInput);
    Result calibrateMicInputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, int const firstInput, int const lastInput);
    Result calibrateAmpOutputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, int const firstOutput, int const lastOutput, int const firstInput);
    void storeOutputCalibration(int const output, float const inputLevel, CalibrationDataV2& calibrationData);
    Result writeTestAdapter(uint8 byte);
    
    const static float AIOReferencePeakVolts;
    const static float inputPeakVolts;
    const static float outputPeakVolts; 
    const static float expectedInputReferenceSignalResult;
    const static float maxInputVariationPercent;
    const static float maxOutputVariationPercent;
    
    Range<float> uncalibratedOutputLimits;
    float expectedOutputReferenceSignalResult;
};
