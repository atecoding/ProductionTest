#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"


ThdnTest::ThdnTest(XmlElement *xe,bool &ok) :
	Test(xe,ok)
{
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
		result = computeTHDN(buffs[input + channel]->getSampleData(0),sample_rate);
		msg += String::formatted(T("  %.1f dB"),result);

	#ifdef WRITE_WAVE_FILES
		String name;

		name = String::formatted("THDN out%02d-in%02d ",output + channel,input + channel);
		name += MsgSampleRate();
		name += ".wav";
		WriteWaveFile(name,sample_rate,buffs[input + channel]);
	#endif

		pass &= result <= pass_threshold_db;
	}

	return pass;
}

