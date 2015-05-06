#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ehw.h"
#include "hwcaps.h"

Test::Test(XmlElement *xe,bool &ok, ProductionUnit* unit_) :
	input (-1),
	output (-1),
	num_channels(1),
    _dc_offset(0.0f),
    _sawtooth(0),
    _pulsate(0),
    glitchThreshold(4.0f),
	errorBit(0),
	unit(unit_)
{
	XmlElement *temp;

	temp = xe->getFirstChildElement();
    if (temp && temp->isTextElement())
	{
		title = temp->getText();
		title = title.trim();
	}

	getIntValue(xe,T("input"),input);
	getIntValue(xe,T("output"),output);
	getIntValue(xe,T("num_channels"),num_channels);

	ok = getIntValue(xe,T("sample_rate"),sample_rate);
	ok &= getFloatValue(xe,T("output_amplitude_db"),output_amplitude_db);

	minSampleRate = sample_rate * 0.96f;
	maxSampleRate = sample_rate * 1.04f;
	getFloatValue(xe, "min_sample_rate", minSampleRate);
	getFloatValue(xe, "max_sample_rate", maxSampleRate);

	getFloatValue(xe, T("dc_offset"), _dc_offset);
	getIntValue(xe,T("sawtooth"),_sawtooth);
	getIntValue(xe,T("pulsate"),_pulsate);

	output_frequency = 1000.0f;
}

Test::~Test()
{
}

void Test::Setup
(
	int samples_per_block,
	ToneGeneratorAudioSource &tone,
	uint32 &active_outputs,
	float &dc_offset,
	int &sawtooth,
	int &pulsate
)
{
	float amp;

	tone.prepareToPlay(samples_per_block,sample_rate);
	tone.setFrequency(output_frequency);
	amp = pow(10.0f,output_amplitude_db/20.0f);
	tone.setAmplitude( amp );
	dc_offset = _dc_offset;
	sawtooth = _sawtooth;
	pulsate = _pulsate;
}

Test *Test::Create(XmlElement *xe, int input, int output, bool &ok, ProductionUnit *unit_)
{
	XmlElement *type;
	Test *test = nullptr;

	type = xe->getChildByName("type");
	if (type)
	{
		if (String("THD+N") == type->getAllSubText())
			test = new ThdnTest(xe, ok, unit_);

		if (String("Differential THD+N") == type->getAllSubText())
			test = new DiffThdnTest(xe, ok, unit_);

		if (String("Dynamic range") == type->getAllSubText())
			test = new DynRangeTest(xe, ok, unit_);

		if (String("Differential Dynamic range") == type->getAllSubText())
			test = new DiffDynRangeTest(xe, ok, unit_);

		if (String("Frequency response") == type->getAllSubText())
			test = new FrequencyResponseTest(xe, ok, unit_);

		if (String("Level check") == type->getAllSubText())
			test = new LevelCheckTest(xe, ok, unit_);

#if ECHO1394
		if (String("Guitar hex input crosstalk") == type->getAllSubText())
			test = new HexInputCrosstalkTest(xe,ok);

		if (String("Saturation") == type->getAllSubText())
			test = new SaturationTest(xe,ok);
#endif

		if (String("Phase") == type->getAllSubText())
			test = new PhaseTest(xe, ok, unit_);
	}

	if (test)
	{
		if (test->input < 0)
			test->input = input;
		if (test->output < 0)
			test->output = input;
	}

	return test;
}

String Test::MsgSampleRate()
{
	if (0 == (sample_rate % 1000))
		return String::formatted(T("%d kHz"),sample_rate/1000);

	return String::formatted(T("%.1f kHz"),sample_rate * 0.001f);
}
