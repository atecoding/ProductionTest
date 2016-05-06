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
            
        case STAGE_CALIBRATE_AIOS_VMON_INPUT:
            return prepareStageCalibrateVmonInput();
            
        case STAGE_CALIBRATE_AIOS_VOUT:
            return prepareStageCalibrateVout();
            
        case STAGE_CALIBRATE_AIOS_IMON_INPUT:
            return prepareStageCalibrateImon();
            
        case STAGE_CALIBRATE_AIOS_MIC_INPUTS:
            return prepareStageCalibrateMics();
            
        case STAGE_CALIBRATE_AIOS_AMP_OUTPUTS:
            return prepareStageCalibrateAmps();
            
        default:
            break;
    }
    
    return Result::fail("Invalid stage");
}


Result CalibrationProcedureAIOS::prepareStageCalibrateVmonInput()
{
    DBG("CalibrationProcedureAIOS::prepareStageCalibrateVmonInput()");
    
    SquareWaveAudioSource::Configuration squareWaveConfiguration;
    squareWaveConfiguration.frequency = 500.0f;
    squareWaveConfiguration.minAmplitude = 0.0f;
    squareWaveConfiguration.maxAmplitude = 0.5f;
    squareWaveSource.setConfiguration(squareWaveConfiguration);
    
    Result result(writeTestAdapter(ADAPTER_TEST_INPUTS));
    if (result.wasOk())
    {
        result = calibrationUnit->aioUSBDevice->setCalibrationReferenceVoltage(module->getModuleNumber(), true);
    }
    
    return result;
}

Result CalibrationProcedureAIOS::prepareStageCalibrateVout()
{
    DBG("CalibrationProcedureAIOS::prepareStageCalibrateVout()");
    
    SquareWaveAudioSource::Configuration squareWaveConfiguration;
    squareWaveConfiguration.frequency = 500.0f;
    squareWaveConfiguration.minAmplitude = -0.25f;
    squareWaveConfiguration.maxAmplitude = 0.25f;
    squareWaveSource.setConfiguration(squareWaveConfiguration);
    
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
    
    SquareWaveAudioSource::Configuration squareWaveConfiguration;
    squareWaveConfiguration.frequency = 500.0f;
    squareWaveConfiguration.minAmplitude = -0.25f;
    squareWaveConfiguration.maxAmplitude = 0.25f;
    squareWaveSource.setConfiguration(squareWaveConfiguration);
    
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
            AudioBuffer<float> fillBuffer(channelPointers + module->getFirstOutput() + AIOS_AMP_OUTPUT_CHANNEL,
                                          1,
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);
            break;
        }
            
            
        //-------------------------------------------------------------------------------------------------------
        //
        // Play a square wave out of the voltage output
        //
        case STAGE_CALIBRATE_AIOS_VMON_INPUT:
        case STAGE_CALIBRATE_AIOS_IMON_INPUT:
        case STAGE_CALIBRATE_AIOS_VOUT:
        {
            float** channelPointers = outputBuffer.getArrayOfWritePointers();
            AudioBuffer<float> fillBuffer(channelPointers + module->getFirstOutput() + VOUT_CHANNEL,
                                          1,
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);

            break;
        }
            
        //-------------------------------------------------------------------------------------------------------
        //
        // Fill output buffer with silence because the test adapter is generating the
        // reference signal
        //
        case STAGE_CALIBRATE_AIOS_MIC_INPUTS:
        {
            break;
        }
            
        //-------------------------------------------------------------------------------------------------------
        //
        // Play out a square wave out of the amps for this module
        //
        case STAGE_CALIBRATE_AIOS_AMP_OUTPUTS:
        {
            float** channelPointers = outputBuffer.getArrayOfWritePointers();
            AudioBuffer<float> fillBuffer(channelPointers + module->getFirstOutput() + AIOS_AMP_OUTPUT_CHANNEL,
                                          1,
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);
            break;
        }

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
        // For AIO-S, only check MIC1, MIC2, and AMP1
        //
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            return checkAmpConnection(recordBuffer, calibrationData,
                                      module->getFirstInput() + FIRST_MIC_INPUT_CHANNEL,
                                      module->getFirstInput() + FIRST_MIC_INPUT_CHANNEL + 1);
            
        //
        // Calibrate the microphone & VMON inputs for this module
        //
        case STAGE_CALIBRATE_AIOS_VMON_INPUT:
            return calibrateVmonInput(recordBuffer, calibrationData);
            
        //
        // Calibrate the amp outputs for this module
        //
        case STAGE_CALIBRATE_AIOS_VOUT:
            return calibrateVout(recordBuffer, calibrationData);
            
        //
        // Calibrate the IMON input for this module
        //
        case STAGE_CALIBRATE_AIOS_IMON_INPUT:
            return calibrateImonInput(recordBuffer, calibrationData);
            
        //
        // Calibrate the Mic inputs for this module
        //
        case STAGE_CALIBRATE_AIOS_MIC_INPUTS:
            return calibrateMicInputs(recordBuffer, calibrationData,
                                      module->getFirstInput() + FIRST_MIC_INPUT_CHANNEL,
                                      module->getFirstInput() + FIRST_MIC_INPUT_CHANNEL + 1);
            
        //
        // Calibrate the Amp outputs for the module
        //
        case STAGE_CALIBRATE_AIOS_AMP_OUTPUTS:
            return calibrateAmpOutputs(recordBuffer, calibrationData,
                                       module->getFirstOutput() + AIOS_AMP_OUTPUT_CHANNEL,
                                       module->getFirstOutput() + AIOS_AMP_OUTPUT_CHANNEL,
                                       module->getFirstInput() + FIRST_MIC_INPUT_CHANNEL);
    }
    
    return invalidStageResult;
}

Result CalibrationProcedureAIOS::calibrateVmonInput(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    DBG("CalibrationProcedureAIOS::calibrateVmonInput");
    
#if 0
    String filename("VMON " + Time::getCurrentTime().toString(true, true));
    filename = File::createLegalFileName(filename);
    WriteWaveFile(filename, recordBuffer, getSampleRate(), recordBuffer.getNumSamples(), 0,3);
#endif
    
    int vmonInput = module->getFirstInput() + VMON_INPUT_CHANNEL;
    Result result(calibrateMicInputs(recordBuffer, calibrationData, vmonInput, vmonInput));
    if (result.failed())
        return result;
    
    return Result::ok();
}

Result CalibrationProcedureAIOS::calibrateVout(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    SquareWaveAudioSource::Configuration const &squareWaveConfiguration( squareWaveSource.getConfiguration() );
    
    DBG("CalibrationProcedureAIOS::calibrateVout");
    
#if 0
    String filename("VOUT " + Time::getCurrentTime().toString(true, true));
    filename = File::createLegalFileName(filename);
    WriteWaveFile(filename, recordBuffer, getSampleRate(), recordBuffer.getNumSamples(), 0,2);
#endif
    
    updateUncalibratedOutputLimits();
    
    //
    // Analyze the square wave for the VMON input (not differential mode)
    //
    float inputLevel = 0.0f;
    int input = module->getFirstInput() + VMON_INPUT_CHANNEL;
    int output = module->getFirstOutput() + VOUT_CHANNEL;
    
    Result analysisResult(Result::ok());
    analysisResult = analyze( "Voltage out",
            recordBuffer.getWritePointer(input),
            nullptr,
            recordBuffer.getNumSamples(),
            squareWaveConfiguration.frequency,
            inputLevel,
            uncalibratedOutputLimits);
    if (analysisResult.failed())
    {
        return Result::fail("Failed to calibrate voltage output - " + analysisResult.getErrorMessage());
    }
    
    storeOutputCalibration(output, inputLevel, calibrationData);
    
    DBG("CalibrationProcedureAIOS::calibrateAmpOutputs (after) -- " + calibrationData.toString());
    
    return Result::ok();
}

Result CalibrationProcedureAIOS::calibrateImonInput(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    SquareWaveAudioSource::Configuration const &squareWaveConfiguration( squareWaveSource.getConfiguration() );

    DBG("CalibrationProcedureAIOS::calibrateImonInput");
    
    updateUncalibratedOutputLimits();
    
    //
    // Analyze the square wave for the VMON input
    //
    float vmon = 0.0f;
    int input = module->getFirstInput() + VMON_INPUT_CHANNEL;
    int output = module->getFirstOutput() + VOUT_CHANNEL;
    Result analysisResult( analyze( "VMON input",
                                   recordBuffer.getWritePointer(input),
                                   nullptr,
                                   recordBuffer.getNumSamples(),
                                   squareWaveConfiguration.frequency,
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
    input = module->getFirstInput() + IMON_INPUT_CHANNEL;
    output = module->getFirstOutput() + VOUT_CHANNEL;
    analysisResult = analyze( "IMON input",
                                   recordBuffer.getWritePointer(input),
                                   nullptr,
                                   recordBuffer.getNumSamples(),
                                   squareWaveConfiguration.frequency,
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

Result CalibrationProcedureAIOS::finishStage()
{
    DBG("CalibrationProcedureAIOS::finishStage()  stage:" << stage);
    
    switch (stage)
    {
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            stage = STAGE_CALIBRATE_AIOS_VMON_INPUT;
            break;
            
        case STAGE_CALIBRATE_AIOS_VMON_INPUT:
            stage = STAGE_CALIBRATE_AIOS_VOUT;
            break;
            
        case STAGE_CALIBRATE_AIOS_VOUT:
            stage = STAGE_CALIBRATE_AIOS_IMON_INPUT;
            break;

        case STAGE_CALIBRATE_AIOS_IMON_INPUT:
            if (module->getInterfaceModuleVersion() < ECHOAIO_INTERFACE_MODULE_REV2)
                stage = STAGE_MODULE_CALIBRATION_DONE;
            else
                stage = STAGE_CALIBRATE_AIOS_MIC_INPUTS;
            break;
            
        case STAGE_CALIBRATE_AIOS_MIC_INPUTS:
            stage = STAGE_CALIBRATE_AIOS_AMP_OUTPUTS;
            break;
            
        case STAGE_CALIBRATE_AIOS_AMP_OUTPUTS:
            stage = STAGE_MODULE_CALIBRATION_DONE;
            break;
    }
    
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
    
    return result;
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
        case STAGE_CALIBRATE_AIOS_VMON_INPUT:
        {
            return "Calibrating VMON";
        }
            
        case STAGE_CALIBRATE_AIOS_VOUT:
        {
            return "Calibrating VOUT";
        }
            
        case STAGE_CALIBRATE_AIOS_IMON_INPUT:
        {
            return "Calibrating IMON";
        }
            
        case STAGE_CALIBRATE_AIOS_MIC_INPUTS:
        {
            return "Calibrating MIC1 and MIC2";
        }
            
        case STAGE_CALIBRATE_AIOS_AMP_OUTPUTS:
        {
            return "Calibrating AMP1";
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
