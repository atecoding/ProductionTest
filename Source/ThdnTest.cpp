#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "errorbits.h"


ThdnTest::ThdnTest(XmlElement *xe,bool &ok) :
	Test(xe,ok)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


ThdnTest::~ThdnTest()
{
}


bool ThdnTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg)
{
	int channel;
	double result;
	bool pass = true;

	msg = "THD+N at ";
	msg += MsgSampleRate();
	msg += ": ";
	for (channel = 0; channel < num_channels; channel++)
	{
		result = computeTHDN(buffs[input + channel]->getReadPointer(0),sample_rate);
		msg += String::formatted(T("  %.1f dB"),result);

	#if WRITE_WAVE_FILES
		if (result > pass_threshold_db)	// only write wave file on failure
		{
			String name;

			name = String::formatted("THDN out%02d-in%02d ", output + channel, input + channel);
			name += MsgSampleRate();
			name += ".wav";
			WriteWaveFile(name, sample_rate, buffs[input + channel]);
		}
	#endif

		pass &= result <= pass_threshold_db;
	}

	return pass;
}

DiffThdnTest::DiffThdnTest(XmlElement *xe, bool &ok) :
Test(xe, ok)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


DiffThdnTest::~DiffThdnTest()
{
}


bool DiffThdnTest::calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg)
{
	double result;
	//bool pass = true;

	msg = "Differential THD+N at ";
	msg += MsgSampleRate();
	msg += ": ";
	result = computeDiffTHDN(buffs[input]->getReadPointer(0), buffs[input+1]->getReadPointer(0), sample_rate);
	msg += String::formatted(T("  %.1f dB"), result);

#if WRITE_WAVE_FILES
	if (result > pass_threshold_db)	// only write wave file on failure
	{
		String name;

		name = String::formatted("Differential THDN out%d-in%d-in%d ", output, input, input + 1);
		name += MsgSampleRate();
		name += ".wav";
		WriteWaveFile(name, sample_rate, buffs[input]);
	}
#endif

	if (result > pass_threshold_db)
	{
		errorBit |= THDN_ERROR_INDEX << input;
		errorBit |= THDN_ERROR_INDEX << (input + 1);

	}

	return result <= pass_threshold_db;
}

