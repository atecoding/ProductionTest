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
    AudioSampleBuffer highPassOutput;
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
		bool channelOK = false;

#if 0
		
         
		highPassOutput.setSize(1, num_samples);
        highPassOutput.copyFrom(0, 0, *sourceBuffer, 0, 0, num_samples);
        float *data = highPassOutput.getWritePointer(0);
        
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

#endif

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
                
                name += String::formatted(" highpass (out%02d-in%02d).wav", output, physicalInput);
                WriteWaveFile(unit, name, sample_rate, &highPassOutput, getSamplesRequired());
            }
        }
#endif
	}
   
	return pass;
}

