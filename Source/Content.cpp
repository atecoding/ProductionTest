#include "base.h"
#include "Content.h"
#include "ehw.h"
#include "ehwlist.h"
#include "hwcaps.h"
#include "ProductionUnit.h"

#define NUM_COLUMNS	7
#define COLUMN_W	80
#define ROW_H		20
#define BUTTON_H	23
#define SLIDER_H	23
#define MARGIN		20

#define SLIDER_MIN	0.0
#define SLIDER_MAX	2.0

void colorlabel(Label *lbl)
{
	lbl->setColour(Label::textColourId,Colours::black);
}

//
// c-tor
//
Content::Content(ehwlist *devlist,const StringArray &hardwareInstances_) :
	_unit(NULL),
	hardwareInstances (hardwareInstances_)
{
	Font f;

	_dev_listener._content = this;

	_devlist = devlist;
	_devlist->RegisterMessageListener(&_dev_listener);

	_logfile = File::getSpecialLocation(File::currentExecutableFile ).getParentDirectory();
	_logfile = _logfile.getChildFile("Echo production test log.txt");

	_log_stream = new FileOutputStream (_logfile);

	setName("content");
	setOpaque(true);

	_f.setTypefaceName("Arial");
	_f.setHeight(15);
	_f.setStyleFlags(Font::bold);

	_log = new TextEditor;
	_log->setMultiLine(true);
	f = _log->getFont();
	f.setBold(true);
	_log->setFont(f);
	_log->setReadOnly(true);
	_log->setCaretVisible(false);
	addAndMakeVisible(_log);
	//log(String("Production test (built ") << __DATE__ << " at " << __TIME__ << ")");

	_start_button = new TextButton(T("Start"));
	addAndMakeVisible(_start_button);
	_start_button->addListener(this);
	_start_button->addShortcut(KeyPress(KeyPress::returnKey));

	//
	// Check that only one device is connected
	//
	if (devlist->GetNumDevices() > 1)
	{
		AlertWindow::showNativeDialogBox(	"Production Test",
											"Please ensure that only one device is connected.",
											false);
		JUCEApplication::quit();
		return;
	}

	//
	// Create the ProductionUnit
	//
#ifndef PCI_BUILD
	//if (devlist->GetNumDevices() == 1)
	//{
	//	DevArrived(devlist->GetNthDevice(0));
	//}
	//else
	{
		_start_button->setEnabled(false);
	}
#endif

	triggerAsyncUpdate();
}

void Content::Setup(AppWindow *window)
{
}

Content::~Content()
{
	DBG("~Content");

	//Reset();

	deleteAllChildren();

	if (_audio_devices)
		_audio_devices->closeAudioDevice();

	DBG("~Content done");
}

//
// Draw the most recent polled stuff
//
void Content::handleCommandMessage(int commandId)
{
}

void Content::paint(Graphics &g)
{
	int x,y,w,h,inset,result_w;
	Font f;
	static const String pass_txt("PASS");
	static const String skip_txt("SKIP");
	static const String fail_txt("FAIL");

	f.setBold(true);
	f.setHeight(14.0f);
	g.setFont(f);
	g.setColour(Colours::black);

	g.fillAll(Colour(239,235,222));

	g.setColour(Colours::black);
	g.drawRect(0,0,getWidth(),getHeight());

	x = roundFloatToInt(getWidth() * 0.1f);
	y = roundFloatToInt(getHeight() * 0.025f);
	w = _log->getX() - x*2;
	h = 24;
	g.setColour(Colours::white);
	g.fillRect(x,y,w,h);
	g.setColour(Colours::black);
	g.drawRect(x-1,y-1,w+2,h+2);
	if (_unit)
		g.drawText(_unit_name,x,y,w,h,Justification::centred,false);
#if ACOUSTICIO_BUILD
	else
		g.drawText("Acoustic AIO", x, y, w, h, Justification::centred, false);
#endif

	result_w = roundFloatToInt(getWidth() * 0.05f);

	if (_unit) // && (false == _unit->_skipped))
	{
		x = roundFloatToInt(getWidth() * 0.05f);
		y = roundFloatToInt(getHeight() * 0.1f);
		w = roundFloatToInt(_log->getX() * 0.6f);
		h = 24;
		inset = roundFloatToInt(w * 0.02f);
		for (int i = 0; i < _group_names.size(); i++)
		{
			g.setColour(Colours::white);
			g.fillRect(x,y,w,h);

			g.setColour(Colours::black);
			g.fillRect(x + w + inset,y,result_w,h);

			g.drawText(_group_names[i],
				x+inset,y,w,h,Justification::centredLeft,false);

			if (_results[i] == 1)
			{
				g.setColour(Colours::limegreen);
				g.drawText(pass_txt,
							x + w + inset,y,result_w,h,
							Justification::centred,false);
			}
			else if (_results[i] == 2)
			{
				g.setColour(Colours::yellow);
				g.drawText(skip_txt,
							x + w + inset,y,result_w,h,
							Justification::centred,false);
			}
			else
			{
				g.setColour(Colours::red);
				g.drawText(fail_txt,
							x + w + inset,y,result_w,h,
							Justification::centred,false);
			}

			y += h + 8;
		}
	}

	if (finalResult.isNotEmpty())
	{
		juce::Rectangle<int> r(x, y + 10, w + inset + result_w, h * 4);
		g.setFont(Font(24.0f, Font::bold));
		g.setColour(Colours::darkgrey);
		g.drawRect(r, 4);
		g.setColour(Colours::black);
		g.fillRect(r);
		g.setColour(finalResultColour);
		g.drawFittedText(finalResult, r, Justification::centred, 2);
	}
}

void Content::buttonClicked(Button *button)
{
	if (button == _start_button)
	{
#ifdef PCI_BUILD
		bool result = false;
		StringArray DisableAllPCIDevices();
		void EnableDevices(const StringArray &instances);

		DBG("Content::buttonClicked _start_button");

		_start_button->setEnabled(false);

		_devlist->Cleanup();

		DisableAllPCIDevices();
		if (hardwareInstances.size())
		{
			result = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,"PCI Production Test","Connect the breakout box to the PCI card","Done", "Cancel");
		}
		else
		{
			AlertWindow::showMessageBox(AlertWindow::NoIcon,"PCI Production Test","No PCI cards found.","Done");
		}

		if (result)
		{
			EnableDevices(hardwareInstances);
		}
		else
		{
			DBG("no hardware instances or user canceled");
			_start_button->setEnabled(true);
			_start_button->grabKeyboardFocus();
		}
#else

		if (_unit)
		{

			if (false == _unit->_running)
			{

#if ACOUSTICIO_BUILD
				int adapterFound = aioTestAdapter.open();
				if (0 == adapterFound)
				{
					AlertWindow::showMessageBox(AlertWindow::NoIcon, "Production Test", "Please connect the AcousticIO test adapter to this computer.", "Close");
					return;
				}
#endif

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
				//
				// Unregister to handle the Echo2 power test, which generates PnP arrival & removal messages
				//
				_devlist->UnregisterMessageListener(&_dev_listener);
#endif

				_start_button->setButtonText("Stop");
				_unit->RunTests();
			}
			else // stop button pressed
			{
				_start_button->setButtonText("Start");
				_start_button->setEnabled(false);
				_unit->_running = false;
			}
		}
#endif
	}
}

bool Content::keyPressed(const KeyPress &key)
{
	switch (key.getKeyCode())
	{
		case VK_ESCAPE :
			JUCEApplication::quit();
			break;
	}

	return false;
}

void Content::sliderValueChanged(Slider *s)
{
}

void Content::log(String msg)
{
	const char * cr = "\n";

	if (nullptr != _log)
	{
		_log->setCaretPosition(INT_MAX);
		_log->insertTextAtCaret(msg);
		_log->insertTextAtCaret(cr);

		//_logfile.appendText(msg);
		//const char * lfcr = "\r\n";
		//_logfile.appendText(lfcr);
		_log_stream->writeText( msg + cr, false, false );
	}
}

void Content::resized()
{
	int x,y,w,h;
	float split = 0.45f;

	x = roundFloatToInt(getWidth() * split);
	y = 4;
	w = getWidth() - x - 4;
	h = getHeight() - 8;
	_log->setBounds(x,y,w,h);

	_start_button->setSize(80,30);
	_start_button->setCentreRelative(split * 0.5f,0.9f);
}

void Content::AddResult(String &name,int pass)
{
	_group_names.add(name);
	_results.add(pass);
}

void Content::FinishTests(bool pass,bool skipped)
{
	_log_stream->flush();

	_unit->_running = false;
	_start_button->setButtonText("Start");
	_start_button->setEnabled(true);
	_start_button->grabKeyboardFocus();

#ifdef PCI_BUILD
	_unit = nullptr;
	_devlist->Cleanup();

	if (_audio_devices)
	{
		_audio_devices->closeAudioDevice();
		_audio_devices = nullptr;
	}

	StringArray DisableAllPCIDevices();
	DisableAllPCIDevices();

	AlertWindow::showMessageBox(AlertWindow::NoIcon,"PCI Production Test","Disconnect the breakout box from the PCI card","Done");

#endif

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
	if (_audio_devices)
	{
		_audio_devices->closeAudioDevice();
		_audio_devices = nullptr;
	}

	//
	// Handle the Echo2 power test, which generates PnP arrival & removal messages
	//
	// Register to once again receive notifications and rebuild the device list
	//
	_unit = nullptr;
	_devlist->BuildDeviceList(nullptr);
	if (_devlist->GetNumDevices())
	{
		DevArrived(_devlist->GetNthDevice(0));
	}
	_devlist->RegisterMessageListener(&_dev_listener);
#endif

#if ACOUSTICIO_BUILD
	aioTestAdapter.close();
	if (_audio_devices)
	{
		_audio_devices->closeAudioDevice();
		_audio_devices = nullptr;
	}
#endif
}

void Content::Reset()
{
	_results.clear();
	_group_names.clear();
}

void Content::DevArrived(ehw *dev)
{
	if (nullptr == dev)
		return;

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
	extern Result CheckUSBFirmwareVersion(ehw *dev);

	Result firmware_result(CheckUSBFirmwareVersion(dev));
	if (firmware_result.failed())
		return;
#endif

	_unit = new ProductionUnit(dev,_devlist,this);

	_unit_name = dev->getcaps()->BoxTypeName();

#ifdef PCI_BUILD
	_unit->RunTests();
#else
	DBG("Content::DevArrived - button enabled");

	_start_button->setEnabled(true);
	_start_button->grabKeyboardFocus();
#endif

	repaint();
}

void Content::DevRemoved(ehw *dev)
{
	_unit = nullptr;
	_unit_name = String::empty;
#ifndef PCI_BUILD
	_start_button->setEnabled(false);
#endif

#if 0 // def ECHOUSB
	if (_audio_devices)
	{
		_audio_devices->closeAudioDevice();
		_audio_devices = nullptr;
	}
#endif
}

void DevChangeListener::handleMessage(const Message &message)
{
#if 1
	ehw *dev;
	DeviceChangeMessage const*const deviceChangeMessage = dynamic_cast <DeviceChangeMessage const*const>(&message);
	if (nullptr == deviceChangeMessage)
	{
		return;
	}

	DBG("DevChangeListener::handleMessage " + String::toHexString(deviceChangeMessage->intParameter1) + String::toHexString((int)deviceChangeMessage->pointerParameter));

	switch (deviceChangeMessage->intParameter1)
	{
		case EHW_DEVICE_ARRIVAL :
#ifdef ECHOUSB
			_content->_devlist->Cleanup();
			_content->_devlist->BuildDeviceList(nullptr);
			dev = _content->_devlist->GetNthDevice(0);
#else
			dev = (ehw *) message.pointerParameter;
#endif
			_content->DevArrived(dev);
			break;

		case EHW_DEVICE_REMOVAL :
			dev = (ehw *)deviceChangeMessage->pointerParameter;
			_content->DevRemoved(dev);
			break;
	}
#endif
}

AudioDeviceManager *Content::GetAudioDeviceManager()
{
	if (nullptr == _audio_devices)
	{
		_audio_devices = new AudioDeviceManager;
		_audio_devices->initialise(2, 2, NULL, false);
	}

	return _audio_devices;
}

void Content::handleAsyncUpdate()
{
	if (_devlist->GetNumDevices())
	{
		getTopLevelComponent()->toFront(true);
		DevArrived(_devlist->GetNthDevice(0));
	}
}

void Content::setFinalResult(String text,Colour color)
{
	finalResult = text;
	finalResultColour = color;
	repaint();
}

