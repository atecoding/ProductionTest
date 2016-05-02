#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"

DynRangeTest::DynRangeTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


DynRangeTest::~DynRangeTest()
{
}


bool DynRangeTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
	int channel;
	double result;
	bool pass = true;

	msg = "Dynamic range at ";
	msg += MsgSampleRate();
	msg += ": ";
	for (channel = 0; channel < num_channels; channel++)
	{
		result = computeTHDN(buffs[input + channel]->getReadPointer(0), sample_rate, 1000.0);
		result = (-1.0 * result) + 60.0;

		msg += String::formatted(T("  %.1f dB"), result);

#if WRITE_WAVE_FILES
		if(result < pass_threshold_db)  // only write wave file on failure
		{
			String name;

			name = String::formatted("Dynamic range  out%02d-in%02d ",output + channel,input + channel);
			name += MsgSampleRate();
			name += ".wav";		
			WriteWaveFile(unit, name, sample_rate, buffs[input + channel], getSamplesRequired());
		}
	#endif

		pass &= result >= pass_threshold_db;
	}

	return pass;
}

DiffDynRangeTest::DiffDynRangeTest(XmlElement *xe, bool &ok, ProductionUnit *unit_) :
Test(xe, ok, unit_)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


DiffDynRangeTest::~DiffDynRangeTest()
{
}


bool DiffDynRangeTest::calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes)
{
	double result;

	msg = "Differential Dynamic Range at ";
	msg += MsgSampleRate();
	msg += ": ";
	result = computeDiffTHDN(buffs[input]->getReadPointer(0), buffs[input + 1]->getReadPointer(0), sample_rate, output_frequency);
	result = (-1.0 * result) + 60.0;
	msg += String::formatted(T("  %.1f dB"), result);

#if WRITE_WAVE_FILES
	if (result < pass_threshold_db)  // only write wave file on failure
	{
		String name;

		name = String::formatted("Differential dynamic range out%d-in%d-in%d ", output, input, input + 1);
		name += MsgSampleRate();
		name += ".wav";
		WriteWaveFile(unit, name, sample_rate, buffs[input], getSamplesRequired());
	}
#endif

	if (result < pass_threshold_db)
	{
        errorCodes.add(ErrorCodes::DNR, input + 1);
        errorCodes.add(ErrorCodes::DNR, input + 2);
	}

	return result >= pass_threshold_db;
}

