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
    AudioSampleBuffer highPassOutput;
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
		int num_samples = sourceBuffer->getNumSamples();
        bool channelOK = false;
         
        highPassOutput.setSize(1, num_samples);
        highPassOutput.copyFrom(0, 0, *sourceBuffer, 0, 0, num_samples);
        float *data = highPassOutput.getWritePointer(0);
        
        highPassFilter.processSamples(data, num_samples);
        
        float accum = 0.0f;
        while (num_samples > 0)
        {
            float sample = fabs(*data);
            
            accum += sample;
            
            num_samples--;
            data++;
        }
        
        msg += "    Channel " + String(physicalInput + 1) + ": ";
        
        float average = 0.0f;
        float db = minusInfinity;
        if (highPassOutput.getNumSamples() != 0)
        {
            average = accum / highPassOutput.getNumSamples();
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
                WriteWaveFile(unit, name, sample_rate, sourceBuffer);
            }
            
            {
                String name(title);
                
                name += String::formatted(" highpass (out%02d-in%02d).wav", output, physicalInput);
                WriteWaveFile(unit, name, sample_rate, &highPassOutput);
            }
        }
#endif
	}
   
	return pass;
}

