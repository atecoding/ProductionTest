#include "../base.h"
#include "CalibrationProcedure.h"
#include "../AIOModule.h"

static const double skipInitialDataSeconds = 8.0;
const float ratioTolerancePercent = 25.0f;
const float minDutyCycle = 0.45f;
const float maxDutyCycle = 0.55f;

const float CalibrationProcedure::testAdapterSquareWaveFrequency = 1000.0f;
const Result CalibrationProcedure::invalidProcedureResult(Result::fail("Invalid calibration procedure"));
const Result CalibrationProcedure::invalidStageResult(Result::fail("Invalid calibration stage"));

CalibrationProcedure::CalibrationProcedure(CalibrationUnit * const calibrationUnit_, AIOModule * const module_) :
calibrationUnit(calibrationUnit_),
module(module_),
stage(0)
{
}


Result CalibrationProcedure::analyze
(
    String const name,
    float *data,
    float *dataNegative,
    int numSamples,
    float const squareWaveFrequency,
    float &totalResult,
    Range<float> const range
)
{
	SquareWaveAnalysisResult positiveResult;
	SquareWaveAnalysisResult negativeResult;
	int length;
    int positiveLength = 0;
    int negativeLength = 0;
    int i;
    
    totalResult = 0.0f;
    
    // if negative data is supplied, convert differential data to equivalent single input data
    if (dataNegative)
    {
        for (i = 0; i < numSamples; i++)
        {
            data[i] -= dataNegative[i];
            data[i] *= 0.5;
        }
    }
    
    double const sampleRate = getSampleRate();
    int skipSamples = roundDoubleToInt(sampleRate * skipInitialDataSeconds);
    numSamples -= skipSamples;
    data += skipSamples;
    
    //
    // How many zero crossings should there be?  One for each phase of the square wave (*2.0), then times 1.5 for a fudge factor
    //
    int zeroCrossing1, zeroCrossing2;
    int zeroCrossingCount(1);
    int const maxExpectedZeroCrossings = roundDoubleToInt(1.5f * 2.0f * (double(numSamples) / sampleRate * squareWaveFrequency));
    
    findZeroCrossing(data, numSamples, 0, zeroCrossing1, squareWaveFrequency);
    if (zeroCrossing1 < 0)
        return Result::fail(name + " - No signal detected");
    data += zeroCrossing1;
    numSamples -= zeroCrossing1;
    
    int numPositiveCenterPoints = 0;
    int numNegativeCenterPoints = 0;
    
    positiveResult.clear(1.0f);
    negativeResult.clear(-1.0f);
    
    while (numSamples > 0)
    {
        findZeroCrossing(data, numSamples, zeroCrossing1, zeroCrossing2, squareWaveFrequency);
        if (zeroCrossing2 < 0)
            break;
        
        zeroCrossingCount++;
        if (zeroCrossingCount > maxExpectedZeroCrossings)
        {
            return Result::fail(name + " signal too noisy");
        }
        
        //DBG("Zero crossing at " << zeroCrossing2);
        
        length = zeroCrossing2 - zeroCrossing1;
        float sample = data[length / 2];
        
        //DBG("  Center sample " << sample);
        
        if (sample < 0.0f)
        {
            negativeResult.add(sample);
            numNegativeCenterPoints++;
            negativeLength += length;
        }
        else
        {
            positiveResult.add(sample);
            numPositiveCenterPoints++;
            positiveLength += length;
        }
        
        data += length;
        numSamples -= length;
        zeroCrossing1 = zeroCrossing2;
    }
    
    if (numNegativeCenterPoints)
    {
        negativeResult.average /= numNegativeCenterPoints;
    }
    if (numPositiveCenterPoints)
    {
        positiveResult.average /= numPositiveCenterPoints;
    }
    
    // make sure waveform is centered around ground (symmetrical)
    {
        if (0.0f == negativeResult.average)
        {
            return Result::fail(name + " negative result is zero");
        }
        
        float ratio = fabs(positiveResult.average / negativeResult.average);
        
        if (ratio < (1.0f - ratioTolerancePercent * 0.01f) || ratio > (1.0f + ratioTolerancePercent * 0.01f))
            return Result::fail(name + " ratio out of range " + String(ratio));
    }
    
    // make sure duty cycle is approximately 50%
    {
        int denominator = positiveLength + negativeLength;
        if (0 == denominator)
            return Result::fail(name + " failed duty cycle - zero length");
        
        float dutyCycle = float(positiveLength) / float(denominator);
        if (dutyCycle < minDutyCycle || dutyCycle > maxDutyCycle)
            return Result::fail(name + " invalid duty cycle " + String(dutyCycle));
    }
    
    totalResult = fabs(negativeResult.average) + positiveResult.average;
    if (range.contains(totalResult))
    {
        return Result::ok();
    }
    
    return Result::fail(name + " out of range (value " + String(totalResult,4) + ")");
}

void CalibrationProcedure::findZeroCrossing(const float * data, int numSamples, int startIndex, int &zeroCrossingIndex, double const squareWaveFrequency)
{
    int const periodThreshold = roundDoubleToInt( (getSampleRate() / squareWaveFrequency) * 0.4);
    
    if (numSamples < 2)
    {
        zeroCrossingIndex = -1;
        return;
    }
    
    float previousSample = *data;
    numSamples--;
    data++;
    
    zeroCrossingIndex = startIndex + 1;
    while (numSamples > 0)
    {
        float sample = *data;
        
        if (((sample < 0.0f && previousSample >= 0.0f) ||
             (previousSample < 0.0f && sample >= 0.0f)) && ((zeroCrossingIndex - startIndex) > periodThreshold))
        {
            return;
        }
        
        zeroCrossingIndex++;
        numSamples--;
        data++;
    }
    
    zeroCrossingIndex = -1;
}


Result CalibrationProcedure::finishStage()
{
    stage++;
    return Result::ok();
}
