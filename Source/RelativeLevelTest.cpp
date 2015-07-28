#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

const Identifier RelativeLevelTest::relativeLevelResults("relativeLevelResults");
static const float minusInfinity = -144.0f;

Identifier getChannelID(int const channel)
{
    return Identifier("channel" + String(channel));
}

RelativeLevelTest::RelativeLevelTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
    DBG(title);
    
    min_level_db = minusInfinity;
    max_level_db = 0.0f;
	bool minLevelFound = getFloatValue(xe, "min_level_db", min_level_db);
	bool maxLevelFound = getFloatValue(xe, "max_level_db", max_level_db);
    autoPass = !minLevelFound && !maxLevelFound;
}


RelativeLevelTest::~RelativeLevelTest()
{
}


bool RelativeLevelTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
	AudioSampleBuffer testInput, testOutput;
	IIRCoefficients highPassCoefficients(IIRCoefficients::makeHighPass(sample_rate, 500.0));
    IIRFilter highPassFilter;
	bool pass = true;
    ValueTree unitTree(unit->tree);
    ValueTree previousResultsTree(unitTree.getOrCreateChildWithName(relativeLevelResults, nullptr));
    
    msg = String::empty;
    
    highPassFilter.setCoefficients(highPassCoefficients);
    
	for (int channel = 0; channel < num_channels; channel++)
	{
        int physicalInput = input + channel;
        Identifier channelID(getChannelID(physicalInput));
        AudioSampleBuffer *sourceBuffer = buffs[physicalInput];
		int num_samples = getSamplesRequired();
        bool channelOK = false;
		int transitions = 0, lowCount = 0, highCount = 0;
        int lowSamples = 0;//, highSamples = 0;
		float totalAve = 0.0, runningAve = 0.0, lowAve = 0.0, tops = 0.0, max = 0.0; // highAve = 0.0

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
			tops *= 0.99f;
			if (indata[i] > tops)
				tops = indata[i];
			runningAve *= 0.96f;
			//			runningAve += fabs(indata[i]) * 0.04;
			runningAve += tops * 0.04f;
			//			if (0.1 * (runningAve / totalAve) < 0.1)
			//			if (tops < max/2)
			if (runningAve < max / 2)
			{
				highCount = 0;
				if (0 == lowCount)
					transitions++;
				lowAve += fabs(indata[i]);
				lowSamples++;
				lowCount++;
				outdata[i] = 0;
			}
			else
			{
				lowCount = 0;
				if (0 == highCount)
					transitions++;
//				highAve += fabs(indata[i]);
//				highSamples++;
				highCount++;
				outdata[i] = max;
			}
		}

		lowAve /= (float)lowSamples;
//		highAve /= (float)highSamples;

		msg += "    Channel " + String(physicalInput + 1) + ": ";
        
        float average = 0.0f;
        float db = minusInfinity;
        if (testInput.getNumSamples() != 0)
        {
			if (transitions > 50)
				average = totalAve;
			else
				average = lowAve;
            db = Decibels::gainToDecibels(average, minusInfinity);
        }
        
        if (autoPass)
        {
            msg += Decibels::toString(db, 1, minusInfinity);
            channelOK = true;
            pass = true;
        }
        else
        {
            float delta = db - (float)(double)previousResultsTree[channelID];
            
            msg += Decibels::toString(db, 1, minusInfinity);
            msg += " (" + Decibels::toString(delta, 1, minusInfinity) + ")";
            
            DBG("db:" << db << "  previous:" << (double)previousResultsTree[channelID] << " delta:" << delta);
            
            channelOK = min_level_db <= delta && delta <= max_level_db;
            if (channelOK)
            {
                msg += " OK";
            }
            else
            {
                msg += " FAIL";
                
                errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
            }
            pass &= channelOK;
        }
        
        previousResultsTree.setProperty(channelID, db, nullptr);
        
        if (channel < (num_channels - 1))
        {
            msg += "\n";
        }

#if WRITE_WAVE_FILES
		if (false == channelOK)
        {
            {
                String name(title);

                name += String::formatted(" (out%02d-in%02d).wav", output, physicalInput);
                WriteWaveFile(unit, name, sample_rate, sourceBuffer, getSamplesRequired());
            }
            
            {
                String name(title);
                
                name += String::formatted(" transitions (out%02d-in%02d).wav", output, physicalInput);
				WriteWaveFile(unit, name, sample_rate, &testOutput, getSamplesRequired());
            }
        }
#endif
	}
   
	return pass;
}

