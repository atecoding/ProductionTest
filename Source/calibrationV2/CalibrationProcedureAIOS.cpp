#include "../base.h"
#include "CalibrationProcedureAIOS.h"
#include "CalibrationUnit.h"
#include "../AIOModule.h"

static const float expectedVmonOverImon = 5.0f;

Result CalibrationProcedureAIOS::prepareModuleForCalibration()
{
    DBG("CalibrationProcedureAIOS::prepareModuleForCalibration()  stage:" << stage);
    
    switch (stage)
    {
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            return prepareStageCheckAdapterConnections();
            
        case STAGE_CALIBRATE_MIC_AND_VMON_INPUTS:
            return prepareStageCalibrateMicAndVmonInputs();
            
        case STAGE_CALIBRATE_AMP_AND_VOUT_OUTPUTS:
            return prepareStageCalibrateAmpAndVout();
            
        case STAGE_CALIBRATE_IMON_INPUT:
            return prepareStageCalibrateImon();
            
        default:
            break;
    }
    
    return Result::fail("Invalid stage");
}


Result CalibrationProcedureAIOS::prepareStageCalibrateMicAndVmonInputs()
{
    DBG("CalibrationProcedureAIOS::prepareStageCalibrateMicAndVmonInputs()");
    
    Result result(writeTestAdapter(ADAPTER_REFERENCE_VOLTAGE));
    if (result.wasOk())
    {
        result = calibrationUnit->aioUSBDevice->setCalibrationReferenceVoltage(module->getModuleNumber(), true);
    }
    
    return result;
}

Result CalibrationProcedureAIOS::prepareStageCalibrateAmpAndVout()
{
    DBG("CalibrationProcedureAIOS::prepareStageCalibrateAmpAndVout()");
    
    Result result(writeTestAdapter(ADAPTER_LOOP_OUTPUT_TO_INPUT));
    if (result.wasOk())
    {
        result = calibrationUnit->aioUSBDevice->setCalibrationReferenceVoltage(module->getModuleNumber(), false);
    }
    
    return result;
}

Result CalibrationProcedureAIOS::prepareStageCalibrateImon()
{
    DBG("CalibrationProcedureAIOS::prepareStageCalibrateImon()");
    return Result::ok();
}


void CalibrationProcedureAIOS::fillOutputBuffer(AudioBuffer<float> outputBuffer)
{
    switch (stage)
    {
        //-------------------------------------------------------------------------------------------------------
        //
        //  Just play the signal out of the first AMP output to make sure the cables are connected OK
        //
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
        {
            float** channelPointers = outputBuffer.getArrayOfWritePointers();
            Range<int> outputs(module->getOutputs());
            AudioBuffer<float> fillBuffer(channelPointers + outputs.getStart() + AMP_OUTPUT_CHANNEL,
                                          1,
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);
            break;
        }
            
            
        //-------------------------------------------------------------------------------------------------------
        //
        // Fill output channels 1 & 2 with silence because the test adapter is
        // generating the reference signal -or- the test adapter ref signal is not used
        //
        // Play a square wave out of the voltage output
        //
        case STAGE_CALIBRATE_MIC_AND_VMON_INPUTS:
        case STAGE_CALIBRATE_IMON_INPUT:
        {
            float** channelPointers = outputBuffer.getArrayOfWritePointers();
            Range<int> outputs(module->getOutputs());
            AudioBuffer<float> fillBuffer(channelPointers + outputs.getStart() + VOLTAGE_OUTPUT_CHANNEL,
                                          1,
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);

            break;
        } // STAGE_CALIBRATE_MIC_AND_VMON_INPUTS
            
            
        //-------------------------------------------------------------------------------------------------------
        //
        // Play out a square wave out the amps for this module
        //
        case STAGE_CALIBRATE_AMP_OUTPUTS:
        {
            float** channelPointers = outputBuffer.getArrayOfWritePointers();
            Range<int> outputs(module->getOutputs());
            AudioBuffer<float> fillBuffer(channelPointers + outputs.getStart(),
                                          outputs.getLength(),
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);
        } // STAGE_CALIBRATE_AMP_OUTPUTS
    }
}


Result CalibrationProcedureAIOS::analyzeRecording(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    DBG("CalibrationProcedureAIOS::analyzeRecording()  stage:" << stage);
    
    switch (stage)
    {
        //
        // Check that the amp plugs are connected correctly
        //
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            return checkAmpConnection(recordBuffer, calibrationData, module->getInputs());
            
        //
        // Calibrate the microphone & VMON inputs for this module
        //
        case STAGE_CALIBRATE_MIC_INPUTS:
            return calibrateMicAndVmonInputs(recordBuffer, calibrationData);
            
        //
        // Calibrate the amp outputs for this module
        //
        case STAGE_CALIBRATE_AMP_OUTPUTS:
            return calibrateAmpOutputs(recordBuffer, calibrationData);
            
        //
        // Calibrate the IMON input for this module
        //
        case STAGE_CALIBRATE_IMON_INPUT:
            return calibrateImonInput(recordBuffer, calibrationData);
    }
    
    return invalidStageResult;
}

Result CalibrationProcedureAIOS::calibrateMicAndVmonInputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    DBG("CalibrationProcedureAIOS::calibrateMicAndVmonInputs");
    
    Range<int> micInputs(module->getInputs());
    micInputs.setLength(VMON_INPUT_CHANNEL + 1);
    Result result(calibrateMicInputs(recordBuffer, calibrationData,micInputs));
    if (result.failed())
        return result;
    
    return Result::ok();
}

Result CalibrationProcedureAIOS::calibrateAmpOutputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    DBG("CalibrationProcedureAIOS::calibrateAmpOutputs");
    
    Range<int> inputs(module->getInputs());
    Range<int> outputs(module->getOutputs());
    
    updateUncalibratedOutputLimits();
    
    //
    // Analyze the square wave for the first two inputs (differential mode)
    //
    float inputLevel = 0.0f;
    int input = inputs.getStart();
    int output = outputs.getStart() + AMP_OUTPUT_CHANNEL;
    Result analysisResult( analyze( "AMP " + String(output + 1),
                                   recordBuffer.getWritePointer(input),
                                   recordBuffer.getWritePointer(input + 1),
                                   recordBuffer.getNumSamples(),
                                   testAdapterSquareWaveFrequency,
                                   inputLevel,
                                   uncalibratedOutputLimits));
    if (analysisResult.failed())
    {
        return Result::fail("Failed to calibrate AMP " + String(output + 1) + " - " + analysisResult.getErrorMessage());
    }
    
    storeOutputCalibration(output, inputLevel, calibrationData);
    
    DBG("CalibrationProcedureAIOS::calibrateAmpOutputs (1st) -- " + calibrationData.toString());
    
    
    //
    // Analyze the square wave for the VMON input (not differential mode)
    //
    output = outputs.getStart() + VOLTAGE_OUTPUT_CHANNEL;
    input = inputs.getStart() + VMON_INPUT_CHANNEL;
    analysisResult = analyze( "Voltage out",
            recordBuffer.getWritePointer(input),
            nullptr,
            recordBuffer.getNumSamples(),
            testAdapterSquareWaveFrequency,
            inputLevel,
            uncalibratedOutputLimits);
    if (analysisResult.failed())
    {
        return Result::fail("Failed to calibrate voltage output - " + analysisResult.getErrorMessage());
    }
    
    storeOutputCalibration(output, inputLevel, calibrationData);
    
    DBG("CalibrationProcedureAIOS::calibrateAmpOutputs (2nd) -- " + calibrationData.toString());
    
    return Result::ok();
}

Result CalibrationProcedureAIOS::calibrateImonInput(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    DBG("CalibrationProcedureAIOS::calibrateImonInput");
    
    Range<int> inputs(module->getInputs());
    Range<int> outputs(module->getOutputs());
    
    updateUncalibratedOutputLimits();
    
    //
    // Analyze the square wave for the VMON input
    //
    float vmon = 0.0f;
    int input = inputs.getStart() + VMON_INPUT_CHANNEL;
    int output = outputs.getStart() + VOLTAGE_OUTPUT_CHANNEL;
    Result analysisResult( analyze( "VMON input",
                                   recordBuffer.getWritePointer(input),
                                   nullptr,
                                   recordBuffer.getNumSamples(),
                                   testAdapterSquareWaveFrequency,
                                   vmon,
                                   uncalibratedOutputLimits));
    if (analysisResult.failed())
    {
        return Result::fail("Failed to calibrate VMON input - " + analysisResult.getErrorMessage());
    }
    

    //
    // Analyze the IMON input
    //
    Range<float> imonLimits( uncalibratedOutputLimits.getStart() / expectedVmonOverImon,
                            uncalibratedOutputLimits.getEnd() / expectedVmonOverImon);
    float imon = 0.0f;
    input = inputs.getStart() + IMON_INPUT_CHANNEL;
    output = outputs.getStart() + VOLTAGE_OUTPUT_CHANNEL;
    analysisResult = analyze( "IMON input",
                                   recordBuffer.getWritePointer(input),
                                   nullptr,
                                   recordBuffer.getNumSamples(),
                                   testAdapterSquareWaveFrequency,
                                   imon,
                                   imonLimits);
    if (analysisResult.failed())
    {
        return Result::fail("Failed to calibrate IMON input - " + analysisResult.getErrorMessage());
    }
    
    //
    // Use the VMON and IMON results 
    //
    float ratio = vmon / imon;
    ratio = ratio / expectedVmonOverImon;
    uint16 value = (uint16)(ratio * 32768.0f);
    calibrationData.data.inputGains[input] = value;
    
    return Result::ok();
}



Result CalibrationProcedureAIOS::finishModuleCalibration()
{
    ReferenceCountedObjectPtr<USBDevice> aioUSBDevice(calibrationUnit->aioUSBDevice);
    
    DBG("CalibrationProcedureAIOS::finishModuleCalibration()  stage:" << stage);
    
    Result result(writeTestAdapter(ADAPTER_LOOP_OUTPUT_TO_INPUT));
    if (result.failed())
        return result;
    
    result = aioUSBDevice->setCalibrationReferenceVoltage(module->getModuleNumber(), false);
    if (result.failed())
        return Result::fail("Could not disable AIO-S reference voltage - " + result.getErrorMessage());
    
    switch (stage)
    {
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            stage = STAGE_CALIBRATE_MIC_AND_VMON_INPUTS;
            break;
            
        case STAGE_CALIBRATE_MIC_AND_VMON_INPUTS:
            stage = STAGE_CALIBRATE_AMP_OUTPUTS;
            break;
            
        case STAGE_CALIBRATE_AMP_OUTPUTS:
            stage = STAGE_CALIBRATE_IMON_INPUT;
            break;
            
        case STAGE_CALIBRATE_IMON_INPUT:
            stage = STAGE_MODULE_CALIBRATION_DONE;
            break;
    }
    
    return Result::ok();
}


String CalibrationProcedureAIOS::getConnectPrompt() const
{
    if (0 == module->getModuleNumber())
        return "Connect the test adapter to MIC1, MIC2, SPKR, and AMP1";
    
    return CalibrationProcedureAIOA::getConnectPrompt();
}

String CalibrationProcedureAIOS::getProgressLabelText() const
{
    switch (stage)
    {
        case STAGE_CALIBRATE_MIC_AND_VMON_INPUTS:
        {
            Range<int> inputs(module->getInputs());
            int micInput = inputs.getStart();
            return "Calibrating MIC " + String(micInput + 1) + "/" + String(micInput + 2) + " and VMON";
        }
            
        case STAGE_CALIBRATE_AMP_OUTPUTS:
        {
            return "Calibrating AMP 1 and VOUT";
        }
            
        case STAGE_CALIBRATE_IMON_INPUT:
        {
            return "Calibrating IMON";
        }
    }
    
    return CalibrationProcedureAIOA::getProgressLabelText();
}


void CalibrationProcedureAIOS::cancelCalibration()
{
    ReferenceCountedObjectPtr<USBDevice> aioUSBDevice(calibrationUnit->aioUSBDevice);
    
    //
    // Turn off the external test adapter reference voltage just in case
    //
    if (aioUSBDevice)
    {
        aioUSBDevice->setCalibrationReferenceVoltage(module->getModuleNumber(), false);
    }
    
    CalibrationProcedureAIOA::cancelCalibration();
}
