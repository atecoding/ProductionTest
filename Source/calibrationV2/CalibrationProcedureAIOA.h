#pragma once

#include "CalibrationProcedure.h"

class CalibrationProcedureAIOA : public CalibrationProcedure
{
public:
    CalibrationProcedureAIOA(CalibrationUnit * const calibrationUnit_, AIOModule * const module_);
    
    virtual double getRecordLengthSeconds() const override;
    
    virtual int getNumCalibrationStages() const override
    {
        return STAGE_CALIBRATE_AMP_OUTPUTS + 1;
    }
    
    virtual Result prepareModuleForCalibration() override;
    
    virtual void fillOutputBuffer(AudioBuffer<float> outputBuffer) override;
    
    virtual Result analyzeRecording(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData) override;

    virtual Result finishModuleCalibration() override;
    
    virtual String getConnectPrompt() const override;
    
    virtual String getProgressLabelText() const override;
    
    void cancelCalibration() override;
    
protected:
    enum
    {
        STAGE_CHECK_ADAPTER_CONNECTIONS = 0,
        STAGE_CALIBRATE_MIC_INPUTS,
        STAGE_CALIBRATE_AMP_OUTPUTS,
        
        STAGE_MODULE_CALIBRATION_DONE = 1000000
    };
    
    void updateUncalibratedOutputLimits();
    Result setGains();
    Result prepareStageCheckAdapterConnections();
    Result prepareStageCalibrateMics();
    Result prepareStageCalibrateAmps();
    Result checkAmpConnection(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, Range<int> inputs);
    Result calibrateMicInputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, Range<int> inputs);
    Result calibrateAmpOutputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData);
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
