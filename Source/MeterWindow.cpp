#include "base.h"
#include "ehw.h"
#include "hwcaps.h"
#include "MeterWindow.h"

class Meter : public Component
{
public:
	enum
	{
		margin = 3
	};

	Meter()
	{
		setOpaque(false);
	}

	void paint(Graphics &g)
	{
		int w;
		Colour color = Colours::red;

		w = roundFloatToInt(getWidth() * 0.7f);

		g.setColour(Colours::black);
		g.fillRect(0,0,w,getHeight());

		if (peak >= 0.0001f)
		{
			String txt;
			int x;
			float db = 20.0f * log10(peak);
			float left_db, right_db, range_db, width_db;

			g.setFont(Font(14.0f, Font::bold));

			txt = String::formatted(T("%.1f dB"),db);
			color = Colours::limegreen;

			if(db > max_db)
			{
				//txt += T("<--");
				color = Colours::red;
			}
			else if(db < min_db)
			{
				//txt += T("-->");
				color = Colours::red;
			}
			g.drawText(txt,
						w+margin,0,getWidth()-w-margin,getHeight(),
						Justification::centred,false);
			range_db = max_db - min_db;
			left_db = min_db - range_db/2;
			right_db = max_db + range_db/2;
			if(db < left_db)
				db = left_db;
			if(db > right_db)
				db = right_db;
			width_db = (db - left_db)/(2 * range_db);

			x = roundFloatToInt((w - 2 * margin) * width_db);
			g.setColour(color);
			g.fillRect(margin,margin,x,getHeight()-margin*2);
		}
	}

	float peak;
	float min_db;
	float max_db;
};

MeterWindow::MeterWindow(Component *parent,hwcaps *caps,TestPrompt &prompt,volatile float *peaks) :
DialogWindow("Level check",Colours::grey,false,false)
{
	int x,y,w,h;

	x = parent->getX();
	y = parent->getY();
	w = roundFloatToInt(parent->getWidth() * 0.45f);
	h = parent->getHeight();
	setBounds(x,y,w,h);
	setTitleBarButtonsRequired(0,false);
	setTitleBarHeight(0);
	setContentOwned(new MeterWindowContent(this,caps,prompt,peaks),false);
}

MeterWindow::~MeterWindow(void)
{
}

void MeterWindow::closeButtonPressed()
{
	exitModalState(TestPrompt::skip);
}

MeterWindowContent::MeterWindowContent(MeterWindow *parent,hwcaps *caps,TestPrompt &prompt,volatile float *peaks) :
	_parent(parent),
	_caps(caps),
    _prompt(prompt),
    _peaks(peaks)
{
	_ok_button = new TextButton("OK");
	_ok_button->addShortcut(KeyPress(KeyPress::returnKey));
	_ok_button->addListener(this);
	addAndMakeVisible(_ok_button);

	if (0 == _prompt.wait_for_user)
		_ok_button->setVisible(false);

	_skip_button = new TextButton("Skip test");
	_skip_button->addShortcut(KeyPress(KeyPress::escapeKey));
	_skip_button->addListener(this);
	addAndMakeVisible(_skip_button);

	for (int i = 0; i < prompt.num_channels; i++)
	{
		_meters.add(new Meter);
		addAndMakeVisible(_meters[i]);
	}

	startTimer(100);
}

MeterWindowContent::~MeterWindowContent()
{
	deleteAllChildren();
}

void MeterWindowContent::paint(Graphics &g)
{
	int i,in,grp,y,w;
	String txt;
	Meter *meter;

	g.fillAll(Colours::white);

	g.setFont(Font(16.0f, Font::bold));
	g.setColour(Colours::black);

	y = roundFloatToInt(getHeight() * 0.1f) + 48;

	/*
	g.drawText("Please connect the following:",
		0,y,getWidth(),24,Justification::centred,false);

	y += 48;
	*/

	//if(_prompt._pulsate)
	{
		g.drawFittedText(_prompt.text,proportionOfWidth(0.1f),y,proportionOfWidth(0.8f),96,Justification::centred,8);

		y += 24;
	}
	//else
	//{
	//	for (i = 0; i < _prompt.num_channels; i++)
	//	{
	//		int out;
	//		out = _prompt.output + i;
	//		grp = _caps->MixOutGroup(out);
	//		txt = _caps->MixOutGroupName(grp);
	//		txt += String::formatted(T(" out %d"),_caps->MixOutGroupOffset(out) + 1);

	//		txt += T(" to ");

	//		in = _prompt.input + i;
	//		grp = _caps->MixInGroup(in);
	//		txt += String(_caps->MixInGroupName(grp));
	//		txt += String::formatted(T(" in %d"),_caps->MixInGroupOffset(in) + 1);

	//		g.drawText(txt,0,y,getWidth(),24,Justification::centred,false);

	//		y += 24;
	//	}
	//}

	for (i = 0; i < _meters.size(); i++)
	{
		meter = _meters[i];

		in = _prompt.input + i;
		grp = _caps->MixInGroup(in);
		txt = _caps->MixInGroupName(grp);
		txt += String::formatted(T(" in %d"),_caps->MixInGroupOffset(in) + 1);

		w = meter->getX() - getWidth()/32;

		g.drawFittedText(	txt,
							0,meter->getY(),w,meter->getHeight(),
							Justification::centredRight,1);
	}
}

void MeterWindowContent::resized()
{
	int i;

	for (i = 0; i < _meters.size(); i++)
	{
		_meters[i]->setBoundsRelative(0.3f,0.05f*i+0.5f,0.6f,0.04f);
	}
	_ok_button->setSize(80,30);
	_ok_button->setCentreRelative(0.35f,0.9f);
	_skip_button->setSize(80,30);
	_skip_button->setCentreRelative(0.65f,0.9f);
}

void MeterWindowContent::buttonClicked(Button *button)
{
	if (button == _ok_button)
	{
		_parent->exitModalState(TestPrompt::ok);
		return;
	}

	if (button == _skip_button)
	{
		_parent->exitModalState(TestPrompt::skip);
		return;
	}
}

void MeterWindowContent::timerCallback()
{
	int i,in;
	bool ok;

	in = _prompt.input;
	ok = true;
	for (i = 0; i < _meters.size(); i++)
	{
		float peak,db;

		peak = _peaks[in++];
		_meters[i]->peak = peak;
		_meters[i]->min_db = _prompt.min_input_db;
		_meters[i]->max_db = _prompt.max_input_db;
		_meters[i]->repaint();

		if (peak > 0.0f)
			db = 20.0f * log10(peak);
		else
			db = -144.0f;

		ok &= (db >= _prompt.min_input_db) && (db <= _prompt.max_input_db);
	}

	_ok_button->setEnabled(ok);

	//
	// Automatically unless "wait for user" is set
	//
	if (ok && (0 == _prompt.wait_for_user))
	{
		_parent->exitModalState(TestPrompt::ok);
	}
}