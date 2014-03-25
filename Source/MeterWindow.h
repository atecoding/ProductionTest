#pragma once

class MeterWindow;
class Meter;
class hwcaps;

#include "TestPrompt.h"

class MeterWindowContent :
	public Component, public ButtonListener, public Timer
{
public:
	MeterWindowContent(MeterWindow *parent,hwcaps *caps,TestPrompt &prompt,volatile float *peaks);
	virtual ~MeterWindowContent();

	void resized();
	void paint(Graphics &g);
	
	void buttonClicked(Button *button);

	void timerCallback();

protected:
	MeterWindow *_parent;
	TextButton *_ok_button;
	TextButton *_skip_button;

	Array<Meter *> _meters;
	hwcaps *_caps;
	TestPrompt &_prompt;
	volatile float *_peaks;
};


class MeterWindow :
	public DialogWindow
{
public:
	MeterWindow(Component *parent,hwcaps *caps,TestPrompt &prompt,volatile float *peaks);
	virtual ~MeterWindow(void);

	void closeButtonPressed();

};
