#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

#define UPSAMPLE_FACTOR 4

FrequencySweepResponseTest::FrequencySweepResponseTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
Test(xe,ok,unit_),
sweep_time_seconds(2.0f),
sweep_delay_seconds(0.5f),
sweep_fadein_seconds(0.3f),
sweep_record_seconds(2.9f),
sweep_fadeout_seconds(0.1f),
upsampler(sample_rate, sample_rate * 4)
{
	ok &= getFloatValue(xe, "pass_threshold_db", pass_threshold_db);
	ok &= getFloatValue(xe, "output_frequency"
                        , output_frequency);
    ok &= getFloatValue(xe, "sweep_time_seconds", sweep_time_seconds);
    ok &= getFloatValue(xe, "sweep_delay_seconds", sweep_delay_seconds);
	ok &= getFloatValue(xe, "sweep_fadein_seconds", sweep_fadein_seconds);
	ok &= getFloatValue(xe, "sweep_record_seconds", sweep_record_seconds);
	ok &= getFloatValue(xe, "sweep_fadeout_seconds", sweep_fadeout_seconds);

	upsampler.setOutputBufferSize(sweep_record_seconds);
}


FrequencySweepResponseTest::~FrequencySweepResponseTest()
{
    sweepGenerator.releaseResources();
}

int FrequencySweepResponseTest::getSamplesRequired()
{
    return roundFloatToInt(sample_rate * (sweep_delay_seconds + sweep_fadein_seconds + sweep_time_seconds + sweep_fadeout_seconds));
}
                                 
void FrequencySweepResponseTest::Setup(int samples_per_block,                                          ToneGeneratorAudioSource &tone,
    uint32 &active_outputs)
{
    Test::Setup(samples_per_block, tone, active_outputs);
    
    sweepGenerator.setSweepTime(sweep_delay_seconds, sweep_fadein_seconds, sweep_fadeout_seconds, sweep_time_seconds);
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

		upsampler.upsample(buffs[physicalInput]);

		double peak = 0.0;
		for (int i = 0; i < upsampler.outputSampleCount; ++i)
		{
			double sample = abs( upsampler.outputBuffer[i] );
			if (sample > peak)
				peak = sample;
		}
        
#if WRITE_WAVE_FILES
        {
            String name;
            
            name = String::formatted("Frequency sweep out%02d-in%02d at %d Hz.wav", output, physicalInput, sample_rate);
            WriteWaveFile(unit, name, sample_rate, buffs[physicalInput], getSamplesRequired());

			name = String::formatted("Upsampled out%02d-in%02d at %d Hz.wav", output, physicalInput, sample_rate * UPSAMPLE_FACTOR);
			int upsampleWaveFileCount = jmin(upsampler.outputSampleCount, getSamplesRequired() * UPSAMPLE_FACTOR);
			WriteWaveFile(unit, name, sample_rate * UPSAMPLE_FACTOR, upsampler.outputBuffer, upsampleWaveFileCount);
        }
#endif
    }
    
    return true;
}

