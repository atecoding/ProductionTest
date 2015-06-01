#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

FrequencySweepResponseTest::FrequencySweepResponseTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
Test(xe,ok,unit_),
sweep_time_seconds(2.0f),
sweep_delay_seconds(0.2f),
sweep_record_seconds(2.5f)
{
	ok &= getFloatValue(xe, "pass_threshold_db", pass_threshold_db);
	ok &= getFloatValue(xe, "output_frequency"
                        , output_frequency);
    ok &= getFloatValue(xe, "sweep_time_seconds", sweep_time_seconds);
    ok &= getFloatValue(xe, "sweep_delay_seconds", sweep_delay_seconds);
    ok &= getFloatValue(xe, "sweep_record_seconds", sweep_record_seconds);
}


FrequencySweepResponseTest::~FrequencySweepResponseTest()
{
    sweepGenerator.releaseResources();
}

int FrequencySweepResponseTest::getSamplesRequired()
{
    return roundFloatToInt(sample_rate * sweep_record_seconds);
}
                                 
void FrequencySweepResponseTest::Setup(int samples_per_block,                                          ToneGeneratorAudioSource &tone,
    uint32 &active_outputs)
{
    Test::Setup(samples_per_block, tone, active_outputs);
    
    sweepGenerator.setSweepTime(sweep_delay_seconds, sweep_time_seconds);
    sweepGenerator.prepareToPlay( samples_per_block, sample_rate);
}

void FrequencySweepResponseTest::fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone)
{
    AudioSourceChannelInfo asci;
    asci.buffer = &buffer;
    asci.numSamples = buffer.getNumSamples();
    asci.startSample = 0;
    sweepGenerator.getNextAudioBlock(asci);
}

bool FrequencySweepResponseTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
    msg = "Frequency sweep at ";
    msg += String(sample_rate);
    msg += ": " + newLine;
    
    for (int channel = 0; channel < num_channels; channel++)
    {
        int physicalInput = input + channel;
        
        msg += "Input " + String(physicalInput + 1) + newLine;
        
#if WRITE_WAVE_FILES
        {
            String name;
            
            name = String::formatted(T("Frequency sweep out%02d-in%02d at %d Hz.wav"), output, physicalInput, sample_rate);
            WriteWaveFile(unit, name, sample_rate, buffs[physicalInput], getSamplesRequired());
        }
#endif
    }
    
    return true;
}

