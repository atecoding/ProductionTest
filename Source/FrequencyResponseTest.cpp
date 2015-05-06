#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "errorbits.h"
#include "ProductionUnit.h"

FrequencyResponseTest::FrequencyResponseTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
	ok &= getFloatValue(xe, T("output_frequency"), output_frequency);
    getFloatValue(xe, "Glitch_threshold", glitchThreshold);
}


FrequencyResponseTest::~FrequencyResponseTest()
{
}


bool FrequencyResponseTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg)
{
	int channel;
    int idx, num_samples;
	float samples_per_cycle;
	float peak, max_db, max_delta, max_delta_level, max_level_linear;//,rms;//,rms_db;
	bool pass = true;

	msg = String::formatted(T("%.0f Hz freq. response at "),output_frequency);
	msg += MsgSampleRate();
	msg += ": ";

	// calculate maximum delta between samples to look for glitches
	max_level_linear = pow(10.0f, (pass_threshold_db + 2.0f) * 0.05f);
	samples_per_cycle = sample_rate / output_frequency;
    max_delta_level = max_level_linear * sin(float_Pi / samples_per_cycle) * glitchThreshold;

	for (channel = 0; channel < num_channels; channel++)
	{

		num_samples = buffs[input]->getNumSamples();
		float const *data = buffs[input + channel]->getReadPointer(0);

		peak = 0.0f;
		max_delta = 0.0f;
		for (idx = num_samples / 16; idx < 15 * num_samples / 16; idx++)
		{
            float sample = fabs(data[idx]);
			peak = jmax( peak, sample );
			float delta = fabs(data[idx] - data[idx - 1]);
			max_delta = jmax(max_delta, delta);
		}

		max_db = 20.0f * log10(peak);

		if (peak < 0.0001)
		{
			msg = String::formatted(T("%.0f Hz freq. response at "),output_frequency);
			msg += MsgSampleRate();
			msg += String::formatted(T(": level too low (peak %f)"),peak);

			max_db = -144.0f;
		}
#if 0
		else
		{

			max = 0.0f;
			min = 2.0f;
			max_db = -144.0f;
			min_db = -144.0f;
			//rms_db = -144.0f;

			idx = 0;
			while (idx < num_samples)
			{
				last = 0.0f;
				for (zc = idx; zc < num_samples; zc++)	
				{
					if (((data[zc] * last) < 0) || (0.0f == data[zc]))
					{
						if (idx != 0)
						{
							peak = 0.0f;
							for (temp = idx; temp < zc; temp++)
							{
								s = fabs(data[temp]);
								if (s > peak)
									peak = s;
							}

							if (peak > max)
								max = peak;

							if (peak < min)
								min = peak;
						}

						break;
					}

					last = data[zc];
				}

				if (zc == idx)
				{
					break;
				}

				idx = zc;
			}

			/*
			rms = 0.0f;
			for (idx = 0; idx < num_samples; idx++)
			{
				rms += data[idx] * data[idx];
			}
			rms /= num_samples;
			rms = sqrt(rms);
			*/

			if (max != 0.0f)
				max_db = 20.0f * log10(max);
		
			if (max != 0.0f)
				min_db = 20.0f * log10(min);

			/*
			if (rms != 0.0f)
				rms_db = 20.0f * log10(rms);
			*/

		}
#endif
		if (max_delta > max_delta_level)	// glitch?
		{
			max_db = max_level_db + 0.5f;	// force failure
			msg += String::formatted(T("  GLITCH"));
		}
		else
		{
			msg += String::formatted(T("  max %.1f dB"), max_db);
		}

		pass &= (max_db >= pass_threshold_db) && (max_db <= pass_threshold_db + 10.0f);
		if ((max_db <= pass_threshold_db) || (max_db >= pass_threshold_db + 10.0f))
			errorBit |= LEVEL_ERROR_INDEX << (input + channel);
	}

#if WRITE_WAVE_FILES
	if (false == pass)	// only write wave file on failure
	{
		String name;

		name = String::formatted(T("Frequency response out%02d-in%02d at %.0f Hz.wav"), output, input, output_frequency);
		WriteWaveFile(unit, name, sample_rate, buffs[input]);
	}
#endif

	return pass;
}

