#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

LevelCheckTest::LevelCheckTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_),
    peakRejectThresholdDb(-80) // Reject any peaks below -80 dB by default
{
	ok &= getFloatValue(xe, T("min_level_db"), min_level_db);
	ok &= getFloatValue(xe, T("max_level_db"), max_level_db);
    getFloatValue(xe, "Glitch_threshold", glitchThreshold);
    getFloatValue(xe, "output_frequency", output_frequency);
    getFloatValue(xe, "peak_reject_threshold_db", peakRejectThresholdDb);
}


LevelCheckTest::~LevelCheckTest()
{
}


bool LevelCheckTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
	int channel;
    int idx, num_samples;
	float samples_per_cycle;
	float peak, max_db, max_delta, max_delta_level, max_level_linear;
	bool pass = true;

	msg = String::formatted(T("Level check at "));
	msg += MsgSampleRate();
	msg += ": ";

	// calculate maximum delta between samples to look for glitches
	max_level_linear = pow(10.0f, max_level_db * 0.05f);
	samples_per_cycle = sample_rate / output_frequency;
    max_delta_level = max_level_linear * sin(float_Pi / samples_per_cycle) * glitchThreshold;

	for (channel = 0; channel < num_channels; channel++)
	{
		num_samples = getSamplesRequired();
		float const *data = buffs[input + channel]->getReadPointer(0);

		peak = 0.0f;
		max_delta = 0.0f;
		for (idx = num_samples / 16; idx < 15 * num_samples / 16; idx++)
		{
			float sample = fabs(data[idx]);
			peak = jmax(peak, sample);
			float delta = fabs(data[idx] - data[idx-1]);
			if (delta > max_delta)
				max_delta = delta;
//            max_delta = jmax(max_delta, delta);
		}

        max_db = Decibels::gainToDecibels(peak, -144.0f);

		if (max_db < peakRejectThresholdDb)
		{
			msg = String::formatted(T("Level check at "));
			msg += MsgSampleRate();
			msg += String::formatted(T(": level too low (peak %f)"), peak);

			max_db = -144.0f;
		}

		if (max_delta > max_delta_level)	// glitch?
		{
			max_db = max_level_db + 0.5f;	// force failure
			msg += String::formatted(T("  GLITCH"));
		}
		else
		{
			msg += String::formatted(T("  level %.1f dB"), max_db);
		}

        bool channelOK = (max_db >= min_level_db) && (max_db <= max_level_db);
		pass &= channelOK;
		if (false == channelOK)	// only write wave files on failure
		{
            errorCodes.add(ErrorCodes::LEVEL, input + channel + 1);
            
#if WRITE_WAVE_FILES
            {
                String name(title);
                
                name += String::formatted(" (out%02d-in%02d).wav", output, input + channel);
                WriteWaveFile(unit, name, sample_rate, buffs[input + channel], getSamplesRequired());
            }
#endif
        }
	}

	return pass;
}

