#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

static const float minusInfinity = -144.0f;


NoiseSegmentTest::NoiseSegmentTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
}


NoiseSegmentTest::~NoiseSegmentTest()
{
}


bool NoiseSegmentTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
    AudioSampleBuffer testInput, testOutput;
    IIRCoefficients highPassCoefficients(IIRCoefficients::makeHighPass(sample_rate, 500.0));
    IIRFilter highPassFilter;
	bool pass = true;
    
    msg = String::empty;
    
    highPassFilter.setCoefficients(highPassCoefficients);
    
	for (int channel = 0; channel < num_channels; channel++)
	{
        int physicalInput = input + channel;
        AudioSampleBuffer *sourceBuffer = buffs[physicalInput];
		int num_samples = getSamplesRequired();
		int transitions = 0, lowCount = 0, highCount = 0;
		int lowSamples = 0, highSamples = 0;
		bool channelOK = false;
		float totalAve = 0.0, runningAve = 0.0, lowAve = 0.0, highAve = 0.0, tops = 0.0, max = 0.0;

		testInput.setSize(1, num_samples);
		testInput.copyFrom(0, 0, *sourceBuffer, 0, 0, num_samples);
		float *indata = testInput.getWritePointer(0);

		highPassFilter.processSamples(indata, num_samples);

		testOutput.setSize(1, num_samples);
        testOutput.copyFrom(0, 0, *sourceBuffer, 0, 0, num_samples);
        float *outdata = testOutput.getWritePointer(0);
        
		for (int i = 0; i < num_samples - 500; i++)
		{
			totalAve += fabs(indata[i]);
			if (indata[i] > max)
				max = indata[i];
		}
		totalAve /= num_samples;

		for (int i = 0; i < num_samples - 500; i++)
			{
			tops *= 0.99;
			if (indata[i] > tops)
				tops = indata[i];
			runningAve *= 0.96;
//			runningAve += fabs(indata[i]) * 0.04;
			runningAve += tops * 0.04;
			//			if (0.1 * (runningAve / totalAve) < 0.1)
//			if (tops < max/2)
			if (runningAve < max/2)
			{
				highCount = 0;
				if (0 == lowCount)
					transitions++;
				lowAve += fabs(indata[i]);
				lowSamples++;
				lowCount++;
				outdata[i] = indata[i];
			}
			else
			{
				lowCount = 0;
				if (0 == highCount)
					transitions++;
				highAve += fabs(indata[i]);
				highSamples++;
				highCount++;
				outdata[i] = 0;
			}
		}
		
		lowAve /= (float)lowSamples;
		highAve /= (float)highSamples;

		float percentHigh = (float)highSamples / (float)(highSamples + lowSamples);
		msg += String::formatted(T("Percent High = %2.0f"), percentHigh * 100.0f) + "%\n";

		msg += String::formatted(T("transitions = %d"), transitions) + "\n";
		float db = minusInfinity;
		db = Decibels::gainToDecibels(totalAve, minusInfinity);
		msg += "totalAve = " + Decibels::toString(db, 1, minusInfinity) + "\n";
		db = minusInfinity;
		db = Decibels::gainToDecibels(highAve, minusInfinity);
		msg += "highAve = " + Decibels::toString(db, 1, minusInfinity) + "\n";
		db = minusInfinity;
		db = Decibels::gainToDecibels(lowAve, minusInfinity);
		msg += "lowlAve = " + Decibels::toString(db, 1, minusInfinity) + "\n";

#if 0
		highPassFilter.processSamples(data, num_samples);
        
        float accum = 0.0f;
		for (int i = num_samples / 16; i < 15 * num_samples / 16; i++)
        {
            float sample = fabs(*data);
            
            accum += sample;
            
            i++;
            data++;
        }
        
        msg += "    Channel " + String(physicalInput + 1) + ": ";
        
        float average = 0.0f;
        float db = minusInfinity;
        if (testOutput.getNumSamples() != 0)
        {
            average = accum / testOutput.getNumSamples();
            db = Decibels::gainToDecibels(average, minusInfinity);
        }
#endif        

#if WRITE_WAVE_FILES
        if (1) // (false == channelOK)
        {
            
            {
                String name(title);
                
                name += String::formatted(" processed (out%02d-in%02d).wav", output, physicalInput);
                WriteWaveFile(unit, name, sample_rate, &testOutput, getSamplesRequired());
            }
        }
#endif
	}
   
	return pass;
}

