#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"


ThdnTest::ThdnTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


ThdnTest::~ThdnTest()
{
}


bool ThdnTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
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
			WriteWaveFile(unit, name, sample_rate, buffs[input + channel], getSamplesRequired());
		}
	#endif

		pass &= result <= pass_threshold_db;
	}

	return pass;
}

DiffThdnTest::DiffThdnTest(XmlElement *xe, bool &ok, ProductionUnit* unit_) :
Test(xe, ok, unit_)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


DiffThdnTest::~DiffThdnTest()
{
}


bool DiffThdnTest::calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes)
{
	double result;
	//bool pass = true;

	msg = "Differential THD+N at ";
	msg += String::formatted("%1.1f kHz", output_frequency/1000.0);
	msg += ": ";
	result = computeDiffTHDN(buffs[input]->getReadPointer(0), buffs[input+1]->getReadPointer(0), sample_rate);
	msg += String::formatted(T("  %.02f%%"), 100*pow(10,result/20));
	if (result > pass_threshold_db)
	{
		if (0 == input || 4 == input)
			msg += " (L201/L202)";
		else
			msg += " (L203/L204)";
	}

#if WRITE_WAVE_FILES
	if (result > pass_threshold_db)	// only write wave file on failure
	{
		String name;

		name = String::formatted("Differential THDN out%d-in%d-in%d ", output, input, input + 1);
		name += MsgSampleRate();
		name += ".wav";
		WriteWaveFile(unit, name, sample_rate, buffs[input], getSamplesRequired());
	}
#endif

	if (result > pass_threshold_db)
	{
        errorCodes.add(ErrorCodes::THDN, input + 1);
        errorCodes.add(ErrorCodes::THDN, input + 2);
	}

	return result <= pass_threshold_db;
}

