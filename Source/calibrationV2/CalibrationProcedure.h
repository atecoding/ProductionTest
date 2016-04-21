#pragma once

class CalibrationUnit;
class AIOModule;

#include "SquareWaveAnalysisResult.h"
#include "CalibrationDataV2.h"
#include "SquareWaveAudioSource.h"

class CalibrationProcedure
{
public:
    CalibrationProcedure(CalibrationUnit * const calibrationUnit_, AIOModule * const module_);

    virtual ~CalibrationProcedure()
    {
    }
    
    virtual double getSampleRate() const
    {
        return 48000.0;
    }
    
    virtual double getRecordLengthSeconds() const
    {
        return 0;
    }
    
    bool isDone() const
    {
        return stage >= getNumCalibrationStages();
    }
    
    virtual int getNumCalibrationStages() const = 0;
    
    virtual Result prepareModuleForCalibration()
    {
        return invalidProcedureResult;
    }
    
    virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        squareWaveSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
    virtual Result finishStage();
    
    virtual Result finishModuleCalibration()
    {
        return invalidProcedureResult;
    }
    
    virtual void cancelCalibration()
    {
        stage = -1;
    }
    
    virtual void fillOutputBuffer(AudioBuffer<float> outputBuffer)
    {
    }
    
    AIOModule * const getModule() const
    {
        return module;
    }
    
    virtual Result analyzeRecording(AudioBuffer<float> recordBuffer, CalibrationDataV2& calibrationData)
    {
        return invalidProcedureResult;
    }
    
    virtual String getConnectPrompt() const
    {
        return "Connect the test adapter to the module";
    }
    
    virtual String getProgressLabelText() const
    {
        return String::empty;
    }

protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalibrationProcedure)
    
    Result analyze(String const name,
                   float *data,
                   float *dataNegative,
                   int numSamples,
                   float const squareWaveFrequency,
                   float &totalResult,
                   Range<float> const range);
    void findZeroCrossing(const float * data, int numSamples, int startIndex, int &zeroCrossingIndex, double const squareWaveFrequency);
    
    CalibrationUnit * const calibrationUnit;
    AIOModule * const module;
    int stage;
    
    SquareWaveAudioSource squareWaveSource;
    
    static const float testAdapterSquareWaveFrequency;
    static const Result invalidProcedureResult;
    static const Result invalidStageResult;
};
