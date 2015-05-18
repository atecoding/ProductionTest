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
    glitchThreshold(4.0f),
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

	output_frequency = 1000.0f;
}

Test::~Test()
{
}

void Test::Setup
(
	int samples_per_block,
	ToneGeneratorAudioSource &tone,
	uint32 &active_outputs
)
{
	float amp;

	tone.prepareToPlay(samples_per_block,sample_rate);
	tone.setFrequency(output_frequency);
	amp = pow(10.0f,output_amplitude_db/20.0f);
	tone.setAmplitude( amp );
}

Test *Test::Create(XmlElement *xe, int input, int output, bool &ok, ProductionUnit *unit_)
{
	XmlElement *typeElement;
	Test *test = nullptr;

	typeElement = xe->getChildByName("type");
	if (typeElement)
	{
        String typeString(typeElement->getAllSubText());
        
		if (typeString == "THD+N")
			test = new ThdnTest(xe, ok, unit_);

		if (typeString == "Differential THD+N")
			test = new DiffThdnTest(xe, ok, unit_);

		if (typeString == "Dynamic range")
			test = new DynRangeTest(xe, ok, unit_);

		if (typeString == "Differential Dynamic range")
			test = new DiffDynRangeTest(xe, ok, unit_);

		if (typeString == "Frequency response")
			test = new FrequencyResponseTest(xe, ok, unit_);

		if (typeString == "Level check")
			test = new LevelCheckTest(xe, ok, unit_);
        
        if (typeString == "Relative level check")
            test = new RelativeLevelTest(xe, ok, unit_);

#if ECHO1394
		if (typeString == "Guitar hex input crosstalk")
			test = new HexInputCrosstalkTest(xe,ok);

		if (typeString == "Saturation")
			test = new SaturationTest(xe,ok);
#endif

		if (typeString == "Phase")
			test = new PhaseTest(xe, ok, unit_);
        
        if (typeString == "AIOS Reference voltage check")
            test = new AIOSReferenceVoltageTest(xe, ok, unit_);
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

void Test::fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone)
{
    AudioSourceChannelInfo asci;
    asci.buffer = &buffer;
    asci.numSamples = buffer.getNumSamples();
    asci.startSample = 0;
    tone.getNextAudioBlock(asci);
}
