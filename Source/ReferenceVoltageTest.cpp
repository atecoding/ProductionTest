#include "base.h"
#include "Test.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"
#include "AcousticIO.h"

const float AIOSReferencePeakVolts = 2.5f;   // Reference signal is 0 to +5V, but AC coupled so -2.5V to +2.5V
const float voltageInputPeakVolts = 8.75f;    // Max +8.75V, min -8.75V, common to AIO-2 and AIO-S
const float expectedAIOSVoltageInputResult = (AIOSReferencePeakVolts * 2.0f) / voltageInputPeakVolts;
const float tolerancePercent = 6.0f;
const float ratioTolerancePercent = 25.0f;
const float minDutyCycle = 0.45f;
const float maxDutyCycle = 0.55f;

ReferenceVoltageTest::ReferenceVoltageTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
Test(xe,ok,unit_)
{
    DBG(title);
    
    squareWavePosition = 0;
    squareWaveMaxAmplitude = 0.5f;
    squareWaveMinAmplitude = -0.5f;
    
    squareWaveFrequency = 500.0f;
    getFloatValue(xe, "square_wave_frequency", squareWaveFrequency);
    
    squareWavePeriodSamples = roundDoubleToInt( double(sample_rate) / squareWaveFrequency);
}

int ReferenceVoltageTest::getSamplesRequired()
{
    return sample_rate * 2;
}

void ReferenceVoltageTest::fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone)
{
    int squareWavePositionThisChannel = squareWavePosition;
    int halfPeriodSamples = squareWavePeriodSamples >> 1;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        squareWavePositionThisChannel = squareWavePosition;
        float* destination = buffer.getWritePointer(channel);
        if (nullptr == destination)
            continue;
        
        int bufferSamplesRemaining = buffer.getNumSamples();
        while (bufferSamplesRemaining > 0)
        {
            if (squareWavePositionThisChannel < halfPeriodSamples)
                *destination = squareWaveMaxAmplitude;
            else
                *destination = squareWaveMinAmplitude;
            
            squareWavePositionThisChannel = (squareWavePositionThisChannel + 1) % squareWavePeriodSamples;
            
            ++destination;
            --bufferSamplesRemaining;
        }
    }
    
    squareWavePosition = squareWavePositionThisChannel;
}


bool ReferenceVoltageTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
    bool pass = true;
    
    for (int channel = 0; channel < num_channels; ++channel)
    {
        int physicalInput = input + channel;
        AudioSampleBuffer *sourceBuffer = buffs[physicalInput];
        Range<float> allowed(expectedAIOSVoltageInputResult * (1.0f - tolerancePercent * 0.01f),
                             expectedAIOSVoltageInputResult * (1.0f + tolerancePercent * 0.01f));
        
        float totalResult = 0.0f;
        Result rangeResult(analyze("Reference voltage",
                sourceBuffer->getReadPointer(0),
                sourceBuffer->getNumSamples(),
                totalResult, allowed));

        msg += String::formatted("     Input %d: ", physicalInput + 1);
        msg += String(totalResult * voltageInputPeakVolts, 2) + " V";
        if (rangeResult.failed())
        {
            msg += " FAIL - " + rangeResult.getErrorMessage();
            
            String filename(title);
            filename += String::formatted(" (in%02d).wav", output, physicalInput);
            WriteWaveFile(unit, filename, sample_rate, sourceBuffer, getSamplesRequired());
            
            errorCodes.add(ErrorCodes::AIOS_REFERENCE_VOLTAGE, channel);
        }
        else
        {
            msg += " OK";
        }
        
        msg += newLine;
        
        pass &= rangeResult.wasOk();
    }
    
    return pass;
}

Result ReferenceVoltageTest::analyze(
                                         String const name,
                                         const float *data,
                                         int numSamples,
                                         float &totalResult,
                                         Range<float> const range
                                         )
{
    SquareWaveAnalysisResult positiveResult,negativeResult;

    totalResult = 0.0f;
    
    int skipSamples = roundDoubleToInt( double(sample_rate) * 0.1);
    numSamples -= skipSamples;
    data += skipSamples;
    
    //
    // How many zero crossings should there be?  One for each phase of the square wave (*2.0), then times 1.5 for a fudge factor
    //
    int zeroCrossing1, zeroCrossing2;
    int zeroCrossingCount;
    int const maxExpectedZeroCrossings = roundDoubleToInt(1.5f * 2.0f * (double(numSamples) / double(sample_rate) * squareWaveFrequency));
    
    findZeroCrossing(data, numSamples, 0, zeroCrossing1);
    if (zeroCrossing1 < 0)
        return Result::fail(name + " - No signal detected");
    data += zeroCrossing1;
    numSamples -= zeroCrossing1;
    
    zeroCrossingCount = 1;
    
    int numPositiveCenterPoints = 0;
    int numNegativeCenterPoints = 0;
	int length;
	int positiveLength = 0;
	int negativeLength = 0;

    positiveResult.clear(1.0f);
    negativeResult.clear(-1.0f);
    
    while (numSamples > 0)
    {
        findZeroCrossing(data, numSamples, zeroCrossing1, zeroCrossing2);
        if (zeroCrossing2 < 0)
            break;
        
        zeroCrossingCount++;
        if (zeroCrossingCount > maxExpectedZeroCrossings)
        {
            return Result::fail(name + " signal too noisy");
        }
        
        DBG("Zero crossing at " << zeroCrossing2);
        
        length = zeroCrossing2 - zeroCrossing1;
        float sample = data[length / 2];
        
        DBG("  Center sample " << sample);
        
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

	totalResult = fabs(negativeResult.average) + positiveResult.average;

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

	if (range.contains(totalResult))
    {
        return Result::ok();
    }
    
    return Result::fail(name + " out of range (value " + String(totalResult,1) + ")");
}


void ReferenceVoltageTest::findZeroCrossing(const float * data, int numSamples, int startIndex, int &zeroCrossingIndex)
{
	int const periodThreshold = roundFloatToInt(squareWavePeriodSamples * 0.4f);

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

