#include "base.h"
#include "ehw.h"
#include "TestPrompt.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "MeterWindow.h"

TestPrompt::TestPrompt(XmlElement *xe,int input_,int output_,bool &ok) :
	input (input_),
	output (output_),
    num_channels (1),
    _dc_offset(0.0f),
    _sawtooth(0),
    _pulsate(0)
{
	XmlElement *temp;

	ok = false;

	wait_for_user = 0;
	start_group = 0;
	stop_group = 0;

	temp = xe->getFirstChildElement();
    if (temp && temp->isTextElement())
	{
		title = temp->getText();
		title = title.trim();

		text = getStringValue(xe,"text");
		if (text.isEmpty())
			text = title;

		getIntValue(xe,T("input"),input);
		getIntValue(xe,T("output"),output);
		getIntValue(xe,T("num_channels"),num_channels);
		ok = getIntValue(xe,T("sample_rate"),sample_rate);
		ok &= getFloatValue(xe,T("output_amplitude_db"),output_amplitude_db);
		ok &= getFloatValue(xe,T("min_input_db"),min_input_db);
		ok &= getFloatValue(xe,T("max_input_db"),max_input_db);
		getFloatValue(xe,T("dc_offset"),_dc_offset);
		getIntValue(xe, T("wait_for_user"), wait_for_user);
		getIntValue(xe, T("start_group"), start_group);
		getIntValue(xe, T("stop_group"), stop_group);
	}
}


TestPrompt::~TestPrompt()
{
}
	

void TestPrompt::Setup(int samples_per_block,ToneGeneratorAudioSource &tone,uint32 &active_outputs,float &dc_offset,int &sawtooth,int &pulsate)
{
	float amp;

/*
	uint32 mask = (1 << num_channels) - 1;
	active_outputs = mask << output;
*/
	active_outputs = 0x03 << (output & 0xfffe);	// always play a stereo pair

	tone.prepareToPlay(samples_per_block,sample_rate);
	tone.setFrequency( 1000.0 );	
	amp = pow(10.0f,output_amplitude_db/20.0f);
	tone.setAmplitude( amp );
	dc_offset = _dc_offset;
	sawtooth = _sawtooth;
	pulsate = _pulsate;

}



bool TestPrompt::ShowMeterWindow(Component *parent,ehw *dev,volatile float *peaks)
{
	int rval;
	MeterWindow mw(parent,dev->getcaps(),*this,peaks);

	parent->addAndMakeVisible(&mw);
    rval = mw.runModalLoop();
	parent->removeChildComponent(&mw);
	
	return rval != 0;
}
