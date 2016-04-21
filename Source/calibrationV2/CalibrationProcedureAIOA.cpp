#include "../base.h"
#include "CalibrationProcedureAIOA.h"
#include "CalibrationUnit.h"
#include "../AIOModule.h"

const float CalibrationProcedureAIOA::AIOReferencePeakVolts = 2.5f;   // Reference signal is 0 to +5V, but AC coupled so -2.5V to +2.5V (for both signal from test adapter and AIO-S onboard signal)
const float CalibrationProcedureAIOA::inputPeakVolts = 8.75f;    // Max +8.75V, min -8.75V, common to AIO-2 and AIO-S
const float CalibrationProcedureAIOA::outputPeakVolts = 13.5f;   // Max +13.5V, min -13.5V, common to AIO-2 and AIO-S
const float CalibrationProcedureAIOA::expectedInputReferenceSignalResult = (AIOReferencePeakVolts * 2.0f) / inputPeakVolts;
const float CalibrationProcedureAIOA::maxInputVariationPercent = 6.0f; // Allow +/- 6% around the reference signal for inputs
const float CalibrationProcedureAIOA::maxOutputVariationPercent = 12.0f; // Allow +/- 12 % for outputs

CalibrationProcedureAIOA::CalibrationProcedureAIOA(CalibrationUnit * const calibrationUnit_, AIOModule * const module_) :
CalibrationProcedure(calibrationUnit_, module_),
expectedOutputReferenceSignalResult(0.0f)
{
    DBG("CalibrationProcedureAIOA::CalibrationProcedureAIOA");
}

double CalibrationProcedureAIOA::getRecordLengthSeconds() const
{
    if (STAGE_CHECK_ADAPTER_CONNECTIONS == stage)
        return 0.5;
    
    return 10.0;
}

Result CalibrationProcedureAIOA::prepareModuleForCalibration()
{
    DBG("CalibrationProcedureAIOA::prepareModuleForCalibration()  stage:" << stage);
    
    switch (stage)
    {
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            return prepareStageCheckAdapterConnections();
            
        case STAGE_CALIBRATE_MIC_INPUTS:
            return prepareStageCalibrateMics();
        
        case STAGE_CALIBRATE_AMP_OUTPUTS:
            return prepareStageCalibrateAmps();
        
        default:
            break;
    }
    
    return Result::fail("Invalid stage");
}

Result CalibrationProcedureAIOA::setGains()
{
    ReferenceCountedObjectPtr<USBDevice> aioUSBDevice(calibrationUnit->aioUSBDevice);
    
    DBG("CalibrationProcedureAIOA::setGains()");
    
    //
    // Set all microphone inputs to 1x gain and disable constant current
    //
    Range<int> inputChannels( module->getInputs() );
    Result usbResult(Result::ok());
    for (int input = inputChannels.getStart(); input < inputChannels.getEnd() && usbResult.wasOk(); ++input)
    {
        usbResult = aioUSBDevice->setConstantCurrent((uint8)input, false);
        if (usbResult.wasOk())
        {
            usbResult = aioUSBDevice->setMicGain((uint8)input, 1);
        }
    }
    
    if (usbResult.wasOk())
    {
        //
        // Set all amp outputs to full scale output
        //
        Range<int> outputChannels(module->getOutputs());
        for (int output = outputChannels.getStart(); output < outputChannels.getEnd() && usbResult.wasOk(); ++output)
        {
            usbResult = aioUSBDevice->setAmpGain((uint8)output, ACOUSTICIO_AMP_GAIN_10V_P2P);
        }
    }
    
    if (usbResult.failed())
    {
        return Result::fail("Could not configure AIO for calibration: " + usbResult.getErrorMessage());
    }
    
    return Result::ok();
}

Result CalibrationProcedureAIOA::prepareStageCheckAdapterConnections()
{
    DBG("CalibrationProcedureAIOA::prepareStageCheckAdapterConnections()");
    
    Result result(setGains());
    if (result.wasOk())
    {
        ReferenceCountedObjectPtr<USBDevice> aioUSBDevice(calibrationUnit->aioUSBDevice);
        Description const * const description = aioUSBDevice->getDescription();
        Range<int> inputs(module->getInputs());
        
        result = writeTestAdapter(ADAPTER_TEST_INPUTS);
        if (result.wasOk())
        {
            DBG("writeTestAdapter OK");
            
            for (int input = inputs.getStart(); input < inputs.getEnd(); ++input)
            {
                if (description->getInputType(input) == Description::TEDS_IN)
                {
                    uint8 data[ACOUSTICIO_TEDS_DATA_BYTES];
                    
                    result = aioUSBDevice->readTEDSData((uint8)input, data, sizeof(data));
                    if (result.wasOk())
                    {
                        int8 inputModulo = int8(input % AIOTestAdapter::NUM_INPUTS_PER_ADAPTER);
                        inputModulo += 1;
                        uint8 expectedValue = inputModulo | (inputModulo << 4);
                        
                        DBG(String::formatted("input %d  0x%02x", input, data[0]));
                        
                        if (expectedValue != data[0])
                        {
                            result = Result::fail(String::formatted("MIC %d not connected to test adapter.", input + 1));
                            break;
                        }
                    }
                }
            }
        }
    }
    
    if (result.wasOk())
    {
        result = writeTestAdapter(ADAPTER_LOOP_OUTPUT_TO_INPUT);
    }
    
    return result;
}

Result CalibrationProcedureAIOA::prepareStageCalibrateMics()
{
    DBG("CalibrationProcedureAIOA::prepareStageCalibrateMics()");
    
    //
    // Turn on test adapter reference signal
    //
    return writeTestAdapter(ADAPTER_REFERENCE_VOLTAGE);
}

Result CalibrationProcedureAIOA::prepareStageCalibrateAmps()
{
    DBG("CalibrationProcedureAIOA::prepareStageCalibrateAmps()");
    
    //
    // Set test adapter to loopback mode, disabling the reference voltage
    //
    return writeTestAdapter(ADAPTER_LOOP_OUTPUT_TO_INPUT);
}


void CalibrationProcedureAIOA::fillOutputBuffer(AudioBuffer<float> outputBuffer)
{
    switch (stage)
    {
        //-------------------------------------------------------------------------------------------------------
        //
        // Just play the signal out of the first AMP output to make sure the cables are connected OK
        //
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
        {
            float** channelPointers = outputBuffer.getArrayOfWritePointers();
            Range<int> outputs(module->getOutputs());
            AudioBuffer<float> fillBuffer(channelPointers + outputs.getStart(),
                                          1,
                                          outputBuffer.getNumSamples());
            AudioSourceChannelInfo info(fillBuffer);
            squareWaveSource.getNextAudioBlock(info);
            break;
        }
            
        // STAGE_CALIBRATE_MIC_INPUTS
        //-------------------------------------------------------------------------------------------------------
        //
        // Fill output buffer with silence because the test adapter is generating the
        // reference signal
        //
        case STAGE_CALIBRATE_MIC_INPUTS:
        {
            break;
        } // STAGE_CALIBRATE_MIC_INPUTS
            
        //-------------------------------------------------------------------------------------------------------
        //
        // Play out a square wave out of the amps for this module
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
            break;
        } // STAGE_CALIBRATE_AMP_OUTPUTS
    }
}


Result CalibrationProcedureAIOA::analyzeRecording(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    DBG("CalibrationProcedureAIOA::analyzeRecording()  stage:" << stage);

    switch (stage)
    {
        //
        // Check that the amp plugs are connected correctly
        //
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            return checkAmpConnection(recordBuffer, calibrationData, module->getInputs());
            
        //
        // Calibrate the microphone inputs for this module
        //
        case STAGE_CALIBRATE_MIC_INPUTS:
            return calibrateMicInputs(recordBuffer, calibrationData, module->getInputs());
            
        //
        // Calibrate the amp outputs for this module
        //
        case STAGE_CALIBRATE_AMP_OUTPUTS:
            return calibrateAmpOutputs(recordBuffer, calibrationData);
    }
    
    return invalidStageResult;
}


Result CalibrationProcedureAIOA::checkAmpConnection(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, Range<int> inputs)
{
    float threshold = (outputPeakVolts * squareWaveSource.squareWaveMaxAmplitude * 0.25f) / inputPeakVolts;
    
    bool pass = true;
    for (int input = inputs.getStart(); input < inputs.getEnd(); ++input)
    {
        float peak = recordBuffer.getMagnitude(input, 0, recordBuffer.getNumSamples());
        
        DBG("input " << input << "  peak " << peak);
        
        if (input < (inputs.getStart() + 2))
            pass &= peak >= threshold;
        else
            pass &= peak < threshold;
    }
    
    if (false == pass)
    {
        return Result::fail("Check AMP connections");
    }

    return Result::ok();
}


Result CalibrationProcedureAIOA::calibrateMicInputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData, Range<int> inputs)
{
    // Uncalibrated input limits - 5V ref signal into mic input
    // Nominal float sample value 0.571
    Range<float> uncalibratedInputLimits;
    float maxVariation = maxInputVariationPercent * 0.01f;
    uncalibratedInputLimits.setStart( expectedInputReferenceSignalResult * (1.0f - maxVariation));
    uncalibratedInputLimits.setEnd( expectedInputReferenceSignalResult * (1.0f + maxVariation));
    
    //
    // Analyze each input
    //
    for (int input = inputs.getStart(); input < inputs.getEnd(); ++input)
    {
        float inputLevel = 0.0f;
        
        //
        // Analyze the square wave for this input, get the average level in inputLevel
        //
        Result analysisResult( analyze( "MIC " + String(input + 1),
                recordBuffer.getWritePointer(input),
                nullptr,
                recordBuffer.getNumSamples(),
                testAdapterSquareWaveFrequency,
                inputLevel,
                uncalibratedInputLimits));
        if (analysisResult.failed())
        {
            return Result::fail("Failed to calibrate MIC " + String(input + 1) + " - " + analysisResult.getErrorMessage());
        }
        
        //
        // Compare inputLevel to expectedInputReferenceSignalResult, store result in calibrationData
        //
        float ratio = expectedInputReferenceSignalResult / inputLevel;
        uint16 value = (uint16)(ratio * 32768.0f);
        calibrationData.data.inputGains[input] = value;
    }
    
    DBG("CalibrationProcedureAIOA::calibrateMicInputs -- " + calibrationData.toString());
    
    return Result::ok();
}


void CalibrationProcedureAIOA::updateUncalibratedOutputLimits()
{
    // Uncalibrated output limits - square wave from amp outputs into mic inputs
    // Multiply squareWaveMaxAmplitude because the square wave output has a DC offset -> AC coupled input
    // Nominal float sample value 0.771
    expectedOutputReferenceSignalResult = (outputPeakVolts * squareWaveSource.squareWaveMaxAmplitude * 0.5f) / inputPeakVolts;
    float maxVariation = maxOutputVariationPercent * 0.01f;
    
    uncalibratedOutputLimits.setStart( expectedOutputReferenceSignalResult * (1.0f - maxVariation));
    uncalibratedOutputLimits.setEnd( expectedOutputReferenceSignalResult * (1.0f + maxVariation));
}

Result CalibrationProcedureAIOA::calibrateAmpOutputs(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
{
    Range<int> inputs(module->getInputs());
    Range<int> outputs(module->getOutputs());
    updateUncalibratedOutputLimits();
    
    int output = outputs.getStart();
    for (int input = inputs.getStart(); input < inputs.getEnd(); input += 2, ++output)
    {
        float inputLevel = 0.0f;
        
        //
        // Analyze the square wave for this input, get the average level in inputLevel
        //
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
    }
    
    DBG("CalibrationProcedureAIOA::calibrateAmpOutputs -- " + calibrationData.toString());
    
    return Result::ok();
}


void CalibrationProcedureAIOA::storeOutputCalibration(int const output, float const inputLevel, CalibrationDataV2& calibrationData)
{
    jassert(output >= 0 && output < numElementsInArray(calibrationData.data.outputGains));
    
    //
    // Compare inputLevel to expectedInputReferenceSignalResult, store result in calibrationData
    //
    float ratio = expectedOutputReferenceSignalResult / inputLevel;
    uint16 value = (uint16)(ratio * 32768.0f);
    calibrationData.data.outputGains[output] = value;
}

Result CalibrationProcedureAIOA::finishModuleCalibration()
{
    DBG("CalibrationProcedureAIOA::finishModuleCalibration()  stage:" << stage);
    
    Result result(writeTestAdapter(ADAPTER_LOOP_OUTPUT_TO_INPUT));
    if (result.failed())
        return result;
    
    switch (stage)
    {
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
            stage = STAGE_CALIBRATE_MIC_INPUTS;
            break;
            
        case STAGE_CALIBRATE_MIC_INPUTS:
            stage = STAGE_CALIBRATE_AMP_OUTPUTS;
            break;

        case STAGE_CALIBRATE_AMP_OUTPUTS:
            stage = STAGE_MODULE_CALIBRATION_DONE;
            break;
    }
    
    return Result::ok();
}


String CalibrationProcedureAIOA::getConnectPrompt() const
{
    switch (module->getModuleNumber())
    {
        case 0:
            return "Connect the test adapter to MIC1, MIC2, MIC3, MIC4, AMP1, and AMP2";
            
        case 1:
            return "Connect the test adapter to MIC5, MIC6, MIC7, MIC8, AMP3, and AMP4";
    }
    
    return CalibrationProcedure::getConnectPrompt();
}


String CalibrationProcedureAIOA::getProgressLabelText() const
{
    switch (stage)
    {
        case STAGE_CHECK_ADAPTER_CONNECTIONS:
        {
            return "Checking test adapter connections";
        }
            
        case STAGE_CALIBRATE_MIC_INPUTS:
        {
            Range<int> inputs(module->getInputs());
            return "Calibrating MIC " + String(inputs.getStart() + 1) + " through MIC " + String(inputs.getEnd());
        }
            
        case STAGE_CALIBRATE_AMP_OUTPUTS:
        {
            Range<int> outputs(module->getOutputs());
            return "Calibrating AMP " + String(outputs.getStart() + 1) + "/" + String(outputs.getEnd());
        }
    }
    
    return String::empty;
}


void CalibrationProcedureAIOA::cancelCalibration()
{
    //
    // Turn off the external test adapter reference voltage just in case
    //
    writeTestAdapter(ADAPTER_LOOP_OUTPUT_TO_INPUT);
    
    CalibrationProcedure::cancelCalibration();
}


Result CalibrationProcedureAIOA::writeTestAdapter(uint8 byte)
{
    if (nullptr == calibrationUnit->testAdapter)
    {
        return Result::fail("AIO test adapter not found");
    }
    
    bool testAdapterWriteOK = calibrationUnit->testAdapter->write(byte) != 0;
    if (false == testAdapterWriteOK)
        return Result::fail("Failed to enable test adapter reference voltage");
    
    return Result::ok();
}

