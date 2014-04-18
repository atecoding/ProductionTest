#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"


LevelCheckTest::LevelCheckTest(XmlElement *xe,bool &ok) :
	Test(xe,ok)
{
	ok &= getFloatValue(xe, T("min_level_db"), min_level_db);
	ok &= getFloatValue(xe, T("max_level_db"), max_level_db);
}


LevelCheckTest::~LevelCheckTest()
{
}


bool LevelCheckTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg)
{
	int channel;
	int idx,zc,num_samples,temp;
	float last,*data,max,min,peak,s,max_db,min_db;//,rms;//,rms_db;
	bool pass = true;

	msg = String::formatted(T("Level check at "));
	msg += MsgSampleRate();
	msg += ": ";
	for (channel = 0; channel < num_channels; channel++)
	{

		num_samples = buffs[input]->getNumSamples();
		data = buffs[input]->getSampleData(0);

		peak = 0.0f;
		for (idx = num_samples/4; idx < 3*num_samples/4; idx++)
		{
			peak = jmax(peak,fabs(data[idx]));
		}
	
		if (peak < 0.0001)
		{
			msg = String::formatted(T("Level check at "));
			msg += MsgSampleRate();
			msg += String::formatted(T(": level too low (peak %f)"),peak);

			max_db = -144.0f;
		}
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

			msg += String::formatted(T("  level %.1f dB"),max_db);
		}

		pass &= (max_db >= min_level_db) && (max_db <= max_level_db);
	}

#ifdef WRITE_WAVE_FILES
	String name;

	name = String::formatted(T("Frequency response out%02d-in%02d at %.0f Hz.wav"),output,input,output_frequency);
	WriteWaveFile(name,sample_rate,buffs[input]);
#endif

	return pass;
}

