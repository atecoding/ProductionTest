#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"


PhaseTest::PhaseTest(XmlElement *xe,bool &ok, ProductionUnit *unit_) :
	Test(xe,ok,unit_)
{
}


PhaseTest::~PhaseTest()
{
}

bool PhaseTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
	int i,num_samples;
	float last,peak,jump;
    float const *data = buffs[input]->getReadPointer(0);
	num_samples = buffs[input]->getNumSamples();

	peak = 0.0f;
	jump = 0.0f;

	for (i = 0; i < num_samples; i++)
	{
        float sample = fabs(data[i]);
		peak = jmax(peak, sample);
	}
	
	if (peak < 0.008)
	{
		msg = String::formatted(T("Phase "));
		msg += String::formatted(T(": level too low (peak %f)"),peak);

	}
	else
	{
		last = 0.0f;
		for(i = 0; i < num_samples; i++)
		{
			if (fabs(data[i] - last) > fabs(jump))
				jump = data[i] - last;
			last = data[i];
		}
		if(jump < 0.0f)
			msg = String::formatted(T("Phase test : Normal"));
		else
			msg = String::formatted(T("Phase test : Inverted"));
	}

#if WRITE_WAVE_FILES
	String name;

	name = String::formatted(T("Phase out%02d-in%02d.wav"),output,input);
	WriteWaveFile(unit, name, sample_rate, buffs[input]);
#endif

	if(pass_threshold_db > 0.0f)
		return (jump < 0.0f);
	else
		return (jump > 0.0f);
}

