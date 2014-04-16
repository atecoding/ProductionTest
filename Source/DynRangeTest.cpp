#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"


DynRangeTest::DynRangeTest(XmlElement *xe,bool &ok) :
	Test(xe,ok)
{
	ok &= getFloatValue(xe, T("pass_threshold_db"), pass_threshold_db);
}


DynRangeTest::~DynRangeTest()
{
}


bool DynRangeTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg)
{
	int channel;
	double result;
	bool pass = true;

	msg = "Dynamic range at ";
	msg += MsgSampleRate();
	msg += ": ";
	for (channel = 0; channel < num_channels; channel++)
	{
		result = computeTHDN(buffs[input + channel]->getSampleData(0),sample_rate);
		result = (-1.0 * result) + 60.0;

		msg += String::formatted(T("  %.1f dB"),result);

	#ifdef WRITE_WAVE_FILES
		String name;

		name = String::formatted("Dynamic range  out%02d-in%02d ",output + channel,input + channel);
		name += MsgSampleRate();
		name += ".wav";		
		WriteWaveFile(name,sample_rate,buffs[input + channel]);
	#endif

		pass &= result >= pass_threshold_db;
	}

	return pass;
}

