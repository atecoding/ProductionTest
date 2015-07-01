#include "base.h"
#include "Content.h"
#include "ehw.h"
#include "ehwlist.h"
#include "hwcaps.h"
#include "ProductionUnit.h"
#include "App.h"

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
	hardwareInstances (hardwareInstances_),
    _unit(NULL),
    startButton("Start"),
    stopButton("Stop")
{
	_dev_listener._content = this;

	_devlist = devlist;
	_devlist->RegisterMessageListener(&_dev_listener);

	setName("content");
	setOpaque(true);

	//_f.setTypefaceName("Arial");
	//_f.setHeight(15);
	//_f.setStyleFlags(Font::bold);

	logDisplay.setMultiLine(true);
    Font f(14.0f, Font::bold);
    logDisplay.setFont(f);
	logDisplay.setReadOnly(true);
	logDisplay.setCaretVisible(false);
	addAndMakeVisible(logDisplay);
	//log(String("Production test (built ") << __DATE__ << " at " << __TIME__ << ")");

	addAndMakeVisible(startButton);
    addAndMakeVisible(stopButton);
    startButton.addShortcut(KeyPress(KeyPress::returnKey));
	startButton.addListener(this);
    stopButton.addListener(this);
    stopButton.setEnabled(false);
    
    TestManager* testManager = application->testManager;
    for (int index = 0; index < testManager->getNumScripts(); ++index)
    {
        File file(testManager->getScript(index));
        scriptCombo.addItem(file.getFileNameWithoutExtension(), index + 1);
    }
    scriptCombo.setSelectedItemIndex(testManager->getCurrentScriptIndex());
    addAndMakeVisible(scriptCombo);
    scriptCombo.addListener(this);

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
		startButton.setEnabled(false);
        stopButton.setEnabled(false);
        scriptCombo.setEnabled(true);
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

    ModalComponentManager::getInstance()->cancelAllModalComponents();

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
	w = logDisplay.getX() - x*2;
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
	inset = roundFloatToInt(w * 0.02f);

	if (_unit) // && (false == _unit->_skipped))
	{
		x = roundFloatToInt(getWidth() * 0.05f);
		y = roundFloatToInt(getHeight() * 0.1f);
		w = roundFloatToInt(logDisplay.getX() * 0.6f);
		h = 24;
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
    DBG("Content::buttonClicked " + button->getButtonText());
    
	if (button == &startButton)
	{
#ifdef PCI_BUILD
		bool result = false;
		StringArray DisableAllPCIDevices();
		void EnableDevices(const StringArray &instances);

		DBG("Content::buttonClicked _start_button");

		_start_button->setEnabled(false);
        scriptCombo->setEnabled(false);

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
            scriptCombo->setEnabled(true);
			_start_button->grabKeyboardFocus();
		}
#else

		if (_unit)
		{

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
            //
            // Unregister to handle the Echo2 power test, which generates PnP arrival & removal messages
            //
            _devlist->UnregisterMessageListener(&_dev_listener);
#endif

            String serialNumber_(_unit->getSerialNumber());
            if (serialNumber_.isEmpty())
            {
                Result serialNumberResult(GetSerialNumber(serialNumber_));
                if (serialNumberResult.failed())
                    return;
            }

            finalResult = String::empty;
            repaint();
            startButton.setEnabled(false);
            stopButton.setEnabled(true);
            stopButton.setState(Button::buttonNormal);
            scriptCombo.setEnabled(false);
            stopButton.grabKeyboardFocus();
            _unit->RunTests(serialNumber_, Time::getCurrentTime());
		}
#endif
    
    return;
	}


    if (button == &stopButton)
    {
        stopButton.setEnabled(false);
        scriptCombo.setEnabled(true);
        _unit->_running = false;
        return;
    }
}

bool Content::keyPressed(KeyPress const & key)
{
    DBG("Content::keyPressed " + key.getTextDescription());
    
    if (key.isKeyCode (KeyPress::returnKey) && startButton.isEnabled())
    {
        startButton.triggerClick();
        return true;
    }
    
    if (key.isKeyCode (KeyPress::escapeKey) && stopButton.isEnabled())
    {
        stopButton.triggerClick();
        return true;
    }
    
    return false;
}

void Content::log(String msg)
{
	if (nullptr == _unit)
		return;

	const char * cr = "\n";

    logDisplay.setCaretPosition(INT_MAX);
    logDisplay.insertTextAtCaret(msg);
    logDisplay.insertTextAtCaret(cr);

    //_logfile.appendText(msg);
    //const char * lfcr = "\r\n";
    //_logfile.appendText(lfcr);
    FileOutputStream* logStream = _unit->getLogStream();
    if (logStream)
        logStream->writeText( msg + cr, false, false );
}

void Content::resized()
{
	int x,y,w,h;
	float split = 0.45f;
    
	x = proportionOfWidth(split);
	y = 26;
	w = getWidth() - x - 4;
	h = getHeight() - y;
	logDisplay.setBounds(x,y,w,h);
    
    w = 300;
    scriptCombo.setBounds((logDisplay.getWidth() - w)/2 + logDisplay.getX(),
                           3,
                           w,
                           20);

	startButton.setSize(80,30);
    stopButton.setSize(80,30);
	x = proportionOfWidth(split * 0.5f);
	startButton.setTopLeftPosition(x - startButton.getWidth() - 10, getHeight() - stopButton.getHeight() - 10);
	stopButton.setTopLeftPosition(x + 10, getHeight() - stopButton.getHeight() - 10);
}

void Content::AddResult(String const &name,int pass)
{
	if(name[0] != '%')
		_group_names.add(name);
	_results.add(pass);
}

void Content::FinishTests(bool pass,bool skipped)
{
	FileOutputStream* logStream = _unit->getLogStream();
	if (logStream)
		logStream->flush();

	_unit->_running = false;
	startButton.setEnabled(true);
    startButton.setState(Button::buttonNormal);
    stopButton.setEnabled(false);
    scriptCombo.setEnabled(true);
	startButton.grabKeyboardFocus();

#ifdef PCI_BUILD
	_unit = nullptr;
	_devlist->Cleanup();

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

	_unit = new ProductionUnit(dev, _devlist, this);

	_unit_name = dev->getcaps()->BoxTypeName();

#ifdef PCI_BUILD
	_unit->RunTests();
#else
	DBG("Content::DevArrived - button enabled");

	startButton.setEnabled(true);
    startButton.setState(Button::buttonNormal);
    stopButton.setEnabled(false);
    scriptCombo.setEnabled(true);
	startButton.grabKeyboardFocus();
#endif

	repaint();
}

void Content::DevRemoved(ehw *dev)
{
    _unit->deviceRemoved();
	//_unit = nullptr;

    _unit_name = String::empty;
#ifndef PCI_BUILD
    startButton.setEnabled(false);
    stopButton.setEnabled(false);
    scriptCombo.setEnabled(true);
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

	DBG("DevChangeListener::handleMessage " + String::toHexString(deviceChangeMessage->intParameter1) + String::toHexString((pointer_sized_int)deviceChangeMessage->pointerParameter));

	switch (deviceChangeMessage->intParameter1)
	{
		case (int)EHW_DEVICE_ARRIVAL :
#if defined(ECHOUSB) && defined(_WIN32)
			_content->_devlist->Cleanup();
			_content->_devlist->BuildDeviceList(nullptr);
			dev = _content->_devlist->GetNthDevice(0);
#else
			dev = (ehw *) deviceChangeMessage->pointerParameter;
#endif
			_content->DevArrived(dev);
			break;

		case (int)EHW_DEVICE_REMOVAL :
			dev = (ehw *)deviceChangeMessage->pointerParameter;
			_content->DevRemoved(dev);
			break;
	}
#endif
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

void Content::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    application->testManager->setCurrentScriptIndex(scriptCombo.getSelectedItemIndex());
}

Result Content::GetSerialNumber(String &serialNumber_)
{
	Time currentTime = Time::getCurrentTime();
	int dayOfYear = currentTime.getDayOfYear();
	int hour = currentTime.getHours();
	int minute = currentTime.getMinutes();
#if ACOUSTICIO_BUILD
	String const deviceName("AIO");
#endif

#if (0)
    bool ok;

    String text("Please enter the serial number for this " + deviceName);// String(_dev->getcaps()->BoxTypeName()));

	do
	{
		juce::AlertWindow dialog("Enter serial number", text, AlertWindow::QuestionIcon);
		String textEditorName("serialNumberEditor");
#if ACOUSTICIO_BUILD
		dialog.addTextEditor(textEditorName, "AIO", "Serial Number:");
#else
#pragma message("Not defined")
#endif

		TextEditor *editor = dialog.getTextEditor(textEditorName);
		editor->setSelectAllWhenFocused(false);

		dialog.addButton("OK", 1, KeyPress(KeyPress::returnKey));
		dialog.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));
		int button = dialog.runModalLoop();
		if (0 == button)
		{
			return Result::fail("User cancel");
		}

		String temp(dialog.getTextEditorContents(textEditorName));

		ok = true;
#if ACOUSTICIO_BUILD
		ok &= temp.startsWith("AIO");
		int length = temp.length();
		ok &= 10 == length;
		ok &= temp.substring(4, 6).containsOnly("0123456789");
#endif
		if (false == ok)
		{
			text = "Please enter a valid serial number";
		}
		else
		{
			serialNumber_ = temp;
		}
	} while (false == ok);
#endif
	serialNumber_ = String::formatted("AIO%03d%02d%02d", dayOfYear, hour, minute);
	return Result::ok();
}

