#include "base.h"
#include "ProductionUnit.h"
#include "ehw.h"
#include "ehwlist.h"
#include "hwcaps.h"
#include "Content.h"
#include "Analysis.h"
#include "wavefile.h"
#include "TestPrompt.h"
#include "AudioOutput.h"
#include "xml.h"
#if defined(ECHOUSB) && defined(ECHO2_BUILD)
#include "TouchPanelTestWindow.h"
#include "Echo2PowerTestWindow.h"
#endif
#include "OldMessage.h"
#include "App.h"
#include "TestManager.h"
#if JUCE_MAC
#include "osx/osx.h"
#endif

#ifdef ACOUSTICIO_BUILD
#include "printer/Printer.h"

bool RunTEDSTest(XmlElement const *element,
                 ehw *dev,
                 String &msg,
                 String &displayedChannel,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes,
                 ValueTree &unitTree);
bool RunCCVoltageTest(XmlElement const *element,
                      ehw *dev,
                      String &msg,
                      String &displayedChannel,
                      AIOTestAdapter &testAdapter,
                      Content *content,
                      ErrorCodes &errorCodes,
                      ValueTree &unitTree);
bool RunCCCurrentTest(XmlElement const *element,
                      ehw *dev,
                      String &msg,
                      String &displayedChannel,
                      AIOTestAdapter &testAdapter,
                      Content *content,
                      ErrorCodes &errorCodes,
                      ValueTree &unitTree);
bool RunUSBFirmwareVersionTest(XmlElement const *element,
                        ehw *dev,
                        String &msg,
                        String &displayedChannel,
                        AIOTestAdapter &testAdapter,
                        Content *content,
                        ErrorCodes &errorCodes,
                        ValueTree &unitTree);
bool RunFlashMemoryTest(XmlElement const *element,
                        ehw *dev,
                        String &msg,
                        String &displayedChannel,
                        AIOTestAdapter &testAdapter,
                        Content *content,
                        ErrorCodes &errorCodes,
                        ValueTree &unitTree);
bool RunModuleTypeTest(XmlElement const *element,
                       ehw *dev,
                       String &msg,
                       String &displayedChannel,
                       AIOTestAdapter &testAdapter,
                       Content *content,
                       ErrorCodes &errorCodes,
                       ValueTree &unitTree);
bool RunCalibrationVerificationTest(XmlElement const *element,
                        ehw *dev,
                        String &msg,
                        String &displayedChannel,
                        AIOTestAdapter &testAdapter,
                        Content *content,
                        ErrorCodes &errorCodes,
                        ValueTree &unitTree);
bool RunPowerSupplyResetTest(XmlElement const *element,
                                ehw *dev,
                                String &msg,
                                String &displayedChannel,
                                AIOTestAdapter &testAdapter,
                                Content *content,
                                ErrorCodes &errorCodes,
                                ValueTree &unitTree);
bool MikeyBusRegisters(XmlElement const *element,
                  ehw *dev,
                  String &msg,
                  String &displayedChannel,
                  AIOTestAdapter &testAdapter,
                  Content *content,
                  ErrorCodes &errorCodes,
                  ValueTree &unitTree);

#endif

//extern String ProductionTestsXmlFileName;

ProductionUnit::ProductionUnit(ReferenceCountedObjectPtr<ehw> dev, ehwlist *devlist, Content *content, CalibrationManagerV2* calibrationManager_) :
_num_tests(0),
_unit_passed(true),
_skipped(false),
_running(false),
deviceAttached(true),
unitTree("ProductionUnit"),
_ok(true),
_dev(dev),
_devlist(devlist),
_content(content),
audioDevice(nullptr),
active_outputs(0),
_script(NULL),
maxAudioDeviceCreateTicks(0),
maxAudioDeviceOpenTicks(0),
maxAudioDeviceStartTicks(0),
calibrationManager(calibrationManager_)
{
	zerostruct(input_meters);

	//_dev->incReferenceCount();
    
#if ACOUSTICIO_BUILD && 0 == USER_PROMPT_SERIAL_NUMBER
    assignAutomaticSerialNumber();
#endif

	//
	// Set up the mixer and buffer size
	//
	int in,rval;
	String msg;
	hwcaps *caps;

	dev->OpenDriver();

	caps = dev->getcaps();

	//rval = dev->setbuffsize(device_buffer_size_samples);
	//content->log(String::formatted(T("setbuffsize result %d"),rval));

	rval = 0;

#if defined(ECHOPCI) || defined(ECHO1394)
	int out;
	if (0 == rval)
	{
		for (in = 0; in < caps->numbusin(); in++)
		{
			for (out = 0; out < caps->numbusout(); out += 2)
			{
				if (0 == rval)
				{
					rval = dev->setmonmute(in,out,true);
					//content->log(String::formatted(T("setmonmute result %d"),rval));
					if (0 == rval)
					{
						rval = dev->setmongain(in,out,0.0f);
						//content->log(String::formatted(T("setmongain result %d"),rval));
					}
				}
			}
		}

		if (0 == rval)
		{
#if ECHO1394
			if (caps->HasOutputBussing())
			{
				efc_isoc_map map;

				rval = dev->getisocmap(&map);
				//content->log(String::formatted(T("getisocmap result %d"),rval));
				if (0 == rval)
				{
					for (out = 0; out < caps->numbusout(); out += 2)
						map.playmap[out/2] = out;
					rval = dev->setisocmap(&map);
					//content->log(String::formatted(T("setisocmap result %d"),rval));
				}
			}
			else
#endif
			{
				for (out = 0; out < caps->numplaychan(); out++)
				{
					if (0 == rval)
					{
						rval = dev->setplaygain(out,out,0.0f);
						//jassert(0 == rval);
						//content->log(String::formatted(T("setplaygain result %d"),rval));
						if (0 == rval)
						{
							rval = dev->setplaymute(out,out,0);
							//jassert(0 == rval);
							//content->log(String::formatted(T("setplaymute result %d"),rval));
						}
					}
				}
			}

			for (out = 0; out < caps->numbusout(); out++)
			{
				if (0 == rval)
				{
					rval = dev->setmastergain(out,0.0f);
					//jassert(0 == rval);
					//content->log(String::formatted(T("setmastergain result %d"),rval));
					if (0 == rval)
					{
						rval = dev->setmastermute(out,0);
						//jassert(0 == rval);
						//content->log(String::formatted(T("setmastermute result %d"),rval));
					}
				}
			}

			if (0 == rval)
			{
				rval = dev->setclock(ehw::clock_not_specified,hwcaps::internal_clock);
				//jassert(0 == rval);
				//content->log(String::formatted(T("setclock result %d"),rval));
				if (0 == rval)
				{
					dev->setphantom(0);
				}
			}
		}
	}
#endif

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
	extern Result ConfigureUSBDevice(ehw *dev);

	Result usb_result( ConfigureUSBDevice(dev) );
	if (usb_result.failed())
		rval = 1;
#endif

	if (0 != rval)
	{
		AlertWindow::showNativeDialogBox(	"Production Test",
											"Could not configure device.",
											false);
		JUCEApplication::quit();
		_ok = false;
		return;
	}

	for (in = 0; in < dev->getcaps()->numrecchan(); in++)
	{
		AudioSampleBuffer *buffer = new AudioSampleBuffer(1,MAX_RECORD_BUFFER_SAMPLES);
        buffer->clear();
		_inbuffs.add(buffer);
	}

    calibrationStateValue.referTo(calibrationManager_->getStateValue());
    calibrationStateValue.addListener(this);
}

ProductionUnit::~ProductionUnit(void)
{
	Cleanup();

#if ACOUSTICIO_BUILD
	aioTestAdapter.close();
#endif

	if (_dev)
	{
		_dev->CloseDriver();
		_dev = nullptr;
	}

	DBG("ProductionUnit::~ProductionUnit");
}

void ProductionUnit::Cleanup()
{
    DBG("ProductionUnit::Cleanup()");
    
    _running = false;
    
#if defined(ECHOUSB) && defined(ECHO2_BUILD)
    extern bool CloseUSBDevice(TUsbAudioHandle handle);
    CloseUSBDevice(_dev->GetNativeHandle());
#endif
    
    if (audioDevice)
    {
        stopAudioDevice();
        audioDevice->close();
        audioDevice = nullptr;
	}
}

bool ProductionUnit::status()
{
	return _ok;
}

#if ACOUSTICIO_BUILD && 0 == USER_PROMPT_SERIAL_NUMBER
void ProductionUnit::assignAutomaticSerialNumber()
{
    Time currentTime = Time::getCurrentTime();
    int dayOfYear = currentTime.getDayOfYear();
    int hour = currentTime.getHours();
    int minute = currentTime.getMinutes();
    String const deviceName("AIO");
    _serial_number = String::formatted("AIO%03d%02d%02d", dayOfYear, hour, minute);
}
#endif

void ProductionUnit::setSerialNumber(const String serialNumber_)
{
    _serial_number = serialNumber_;
}

void ProductionUnit::RunTests(Time const testStartTime_)
{
    DBG("ProductionUnit::RunTests");
    
    testStartTime = testStartTime_;
    
	_channel_group_name = String::empty;
	_content->Reset();
	_skipped = false;
	_unit_passed = true;
	_running = true;
    errorCodes.reset();
    unitTree.removeAllChildren(nullptr);
    unitTree.removeAllProperties(nullptr);

	//
	// Load the set of tests from XML
	//
	File f(application->testManager->getCurrentScript());
    
	XmlDocument xmlDocument(f);
	XmlElement *child;

	_root = xmlDocument.getDocumentElement();
	DBG("XML file " + f.getFullPathName());
	if (_root)
	{
		uint32 scriptUSBProductID = 0;
		child = _root->getFirstChildElement();
		while (child)
		{
			if (child->hasTagName("device"))
			{
				scriptUSBProductID = child->getStringAttribute("USB_product_ID").getHexValue32();

				if (scriptUSBProductID == _dev->GetBoxType())
				{
					_script = child;
					break;
				}
			}
			child = child->getNextElement();
		}

		if (nullptr == _script)
		{
			String errorString("Incorrect AIO type for " + f.getFullPathName() + newLine);
			errorString += "Detected " + String(_dev->getcaps()->BoxTypeName());
			errorString += String::formatted("(product ID 0x%04x)", _dev->GetBoxType());
			errorString += newLine;
			errorString += String::formatted("Test script requires product ID 0x%04x", scriptUSBProductID);
			AlertWindow::showNativeDialogBox("Production Test",
				errorString,
				false);
			JUCEApplication::quit();
			DBG("   RunTests exit");
			return;
		}
	}
	else
	{
		AlertWindow::showNativeDialogBox(	"Production Test",
											"Could not load " + f.getFullPathName(),
											false);
		JUCEApplication::quit();
		_ok = false;
        DBG("   RunTests exit");
		return;
	}

#if ACOUSTICIO_BUILD
	//
	// Look for the AIO test adapter (USB HID-class device)
	//
	{
		XmlElement *requireTestAdapterElement = _script->getChildByName("Require_AIO_Test_Adapter");
		if (requireTestAdapterElement != nullptr)
		{
			String text(requireTestAdapterElement->getAllSubText());
			if (text.compareIgnoreCase("true") == 0)
			{
				int adapterFound = aioTestAdapter.open();
				if (0 == adapterFound)
				{
					AlertWindow::showMessageBox(AlertWindow::NoIcon, "Production Test", "Please connect the AcousticIO test adapter to this computer and restart.", "Close");
                    DBG("   RunTests exit");
					return;
				}
			}

			//_script = requireTestAdapterElement->getNextElement();
		}
	}
#endif
    
    //
    // Prompt user for serial number?
    //
    {
        XmlElement* userPromptElement = _script->getChildByName("User_prompt_serial_number");
        if (userPromptElement && userPromptElement->getAllSubText().compareIgnoreCase("true") == 0)
        {
            Result result(_content->promptForSerialNumber(_serial_number));
            if (result.failed())
            {
                return;
            }
        }
    }
    
    CreateLogFile();

	//
	// Create the audio driver object
	//
	if (!createAudioDevice(_script))
    {
        DBG("   RunTests exit");
		return;
    }

	//
	// onscreen logging
	//
	String msg;

	msg = "Begin testing for ";
	msg += _dev->getcaps()->BoxTypeName();
	msg += " " + _serial_number;
    _content->log("-------------------------------------------");
    _content->log("Script: " + f.getFileNameWithoutExtension());
    _content->log("Test built " __DATE__);
	_content->log(msg);
    _content->log("Firmware version " + _dev->getFirmwareVersionString());
	_content->log(testStartTime_.toString(true,true));
#if JUCE_MAC
    _content->log(getMacModelID());
#endif
    _content->log(SystemStats::getOperatingSystemName());
	_content->log(String::empty);

    {
        int numLoops = application->testManager->getNumLoops();
        if (numLoops)
        {
            _content->log(String::formatted("Loop %d/%d", application->testManager->currentLoop + 1,
                                            numLoops));
        }
    }

	//
	// Set up the state machine
	//
	new_test = false;
	blocks_recorded = 0;
	record_done = true;

	_script = _script->getFirstChildElement();	// skip initial device name element
	if (_script)
	{
		_script = _script->getNextElement();
		ParseScript();
	}
    
    DBG("   RunTests exit");
}

void ProductionUnit::audioDeviceAboutToStart(AudioIODevice *device)
{
    //DBG("ProductionUnit::audioDeviceAboutToStart " << device->getName());

	for (int i = 0; i < _inbuffs.size(); ++i)
	{
		_inbuffs[i]->clear();
	}
    
    audioCallbackCount = 0;
    totalAudioCallbackSamples = 0;
    
    timerIntervalMsec = 2000;
    startTimer(timerIntervalMsec);
}

void ProductionUnit::audioDeviceStopped()
{
    stopTimer();
}

void ProductionUnit::audioDeviceIOCallback
(
	const float **inputChannelData,
	int numInputChannels,
	float **outputChannelData,
	int numOutputChannels,
	int numSamples
)
{
	int in,i;
	static int passes = 0;
    Test* test = _test;
    
    audioCallbackCount++;
    startTimer(timerIntervalMsec);

    int64 now = Time::getHighResolutionTicks();
	
	int timestampIndex = ++timestampCount - 1;
	if (timestampIndex < numElementsInArray(timestamps))
	{
		timestamps[timestampIndex] = now;
	}

	//
	// Update test state if necessary
	//
	if (new_test.exchange(0))
	{
		record_done = false;
		totalAudioCallbackSamples = 0;
		blocks_recorded.exchange(0);
		timestampCount = 0;
		zerostruct(timestamps);
	}
	else
	{
		totalAudioCallbackSamples += numSamples;
	}

	//
	// Input metering
	//
	for (in = 0; in < numInputChannels; in++)
	{
		float max,s;

		max = 0.0f;
		for (i = 0; i < numSamples; i++)
		{
			s = fabs(inputChannelData[in][i]);
			if (s > max)
				max = s;
		}

		input_meters[in] = input_meters[in]*0.9f + max*0.1f;
	}

	//
	// Record
	//
    if (test)
    {
        if (totalAudioCallbackSamples >= callback_skip_samples)
        {
            int count,temp;
            int const recordSamplesRequired = test->getSamplesRequired();

            temp = ++blocks_recorded;
            if (temp <= (recordSamplesRequired/numSamples))
            {
                temp = (temp-1)*numSamples;
                count = jmin(numSamples,recordSamplesRequired - temp);
                if (count)
                {
                    for (in = 0; in < numInputChannels; in++)
                    {
                        _inbuffs[in]->copyFrom(0,temp,inputChannelData[in],count);
                    }
                }
            }
            else
            {
                if (false == record_done)
                {
                    postMessage(new OldMessage(MESSAGE_AUDIO_TEST_DONE,0,0,nullptr));
                    record_done = true;
                }
                --blocks_recorded;
            }
        }
    }

	//
	// Playback
	//
	AudioSampleBuffer asb(outputChannelData, numOutputChannels, numSamples);
    if (test)
    {
        test->fillAudioOutputs(asb,_tone);
    }
    else
    {
        asb.clear();
    }
    
	passes++;
	if(passes >= 100)
		passes = 0;
}

void ProductionUnit::timerCallback()
{
    audioDeviceTimedOut();
}

void ProductionUnit::audioDeviceTimedOut()
{
    stopAudioDevice();
    
    _content->log("FAIL -- Audio driver timeout");
    _content->log(String::formatted("    Callbacks: %d    Samples recorded: %d", audioCallbackCount, totalAudioCallbackSamples));
    _content->log(String::empty);
    _unit_passed = false;
    
    ParseScript();
}

void ProductionUnit::handleMessage(const Message &message)
{
	OldMessage const* oldMessage = dynamic_cast<OldMessage const*>(&message);
	if (nullptr == oldMessage)
	{
		return;
	}
	switch (oldMessage->intParameter1)
	{
	case MESSAGE_AUDIO_TEST_DONE:
		{
			bool result;
			String msg;
            
            stopAudioDevice();

			Result sampleRateResult(CheckSampleRate());

			result = _test->calc(_inbuffs, msg, errorCodes);
			result &= sampleRateResult.wasOk();
			_unit_passed &= result;
            
			_channel_group_passed &= (int)result;

		#if 0
			if (false == result)
				_content->log(msg);
		#else
			if (false == result)
				msg = String("*** ") + msg;
			_content->log(msg);

			if (sampleRateResult.failed())
			{
				_content->log("*** Sample rate out of range");
				_content->log(sampleRateResult.getErrorMessage());

				/*
				if (timestampCount.get() != 0)
				{
					LARGE_INTEGER freq;
					QueryPerformanceFrequency(&freq);

					for (int i = 1; i < timestampCount.get(); ++i)
					{
						int64 diff = timestamps[i] - timestamps[i - 1];
						double msec = (diff * 1000.0) / freq.QuadPart;
						String line(String(i) + " " + String(msec, 1));
						_content->log(line);
					}
				}
				*/				
			}
		#endif

			ParseScript();
		}
		break;
            
        case MESSAGE_CALIBRATION_DONE:
        {
            finishCalibration();
        }
        break;
            
           
        case MESSAGE_AIOS_RESISTANCE_MEASUREMENT_DONE:
        {
            finishAIOSResistanceMeasurement();
        }
        break;
	}
}

void ProductionUnit::ParseScript()
{
	bool ok;
	bool quit = false;
	int rval = -1;

	while (_script && _running && deviceAttached)
	{
        DBG("ParseScript " << _script->getTagName());
        
		//-----------------------------------------------------------------------------
		//
		// Firmware version check?
		//
		//-----------------------------------------------------------------------------
	
#if 0
		if (_script->hasTagName("flash_update"))
		{
			bool FirmwareVersionCheck(ehw *dev,XmlElement *xe,String &msg);
			String msg;

			ok = FirmwareVersionCheck(_dev,_script,msg);
			_content->log(msg);
			_content->log(String::empty);
			if (false == ok)
			{
				_content->FinishTests(false,false);
				Cleanup();
				return;
			}

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Show message box?
		//
		//-----------------------------------------------------------------------------

		if (_script->hasTagName("message_box"))
		{
			XmlElement *text,*log_text;
            int yesOrNo = 0;
            int continueOrStop = 0;
            
			//
			// Look for the message box text
			//
			text = _script->getFirstChildElement();
			if (!text->isTextElement())
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No text for message box in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			//
			// get "yes/no" flag and "continue/stop" flag
			//
			getIntValue(_script,"show_yes_no",yesOrNo);
            getIntValue(_script,"continue_or_stop",continueOrStop);

			if (yesOrNo)
			{
				ok = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,"Production Test",text->getText(),"Yes","No");
				_unit_passed &= ok;

				log_text = _script->getChildByName("log");
				if (log_text)
				{
					String msg;

//					_content->log(String::empty);
					msg = log_text->getFirstChildElement()->getText();
					_content->AddResult(msg,(int)ok);
					if (!ok)
					{
						msg = "*** ";
						msg += log_text->getFirstChildElement()->getText();
                        
                        errorCodes.add(ErrorCodes::LED);
					}
					msg += ": ";
					msg += ok ? "ok" : "failed";
					_content->log(msg);
				}
			}
            else if (continueOrStop)
            {
                ok = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,"Production Test",text->getText(),"Continue","Stop");
                if (false == ok)
                {
                    _running = false;
                    _skipped = true;
                    
                    _content->FinishTests(false, true);
                    _script = _script->getNextElement();
                    break;
                }
            }
			else
			{
				AlertWindow::showMessageBox(AlertWindow::NoIcon,"Production Test",text->getText(),"OK");
			}

            _script = _script->getNextElement();
			continue;
		}


		//-----------------------------------------------------------------------------
		//
		// Show user prompt with audio meters?
		//
		//-----------------------------------------------------------------------------
		
		if (_script->hasTagName("prompt"))
		{
			XmlElement *text;

			//
			// Look for the test group name
			//
			text = _script->getFirstChildElement();
			if (!text->isTextElement())
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No title for prompt in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			//
			// Parse the rest of this child element
			//
			TestPrompt tp(_script,_input,_output,ok);
			if (!ok)
				return;

			if((tp.num_channels == 1) && (_channel_group_passed == 2))
			{
				//
				// Skip over second test of a pair that has been skipped
				//
				do
				{
					_script = _script->getNextElement();
					if (!_script)
						break;
				} while (_script->hasTagName("test"));

				continue;
			}

			if (1 == tp.stop_group)
			{
				//
				// Finish any previous test group
				//
				FinishGroup();
				_script = _script->getNextElement();
				continue;
			}
			
			if(tp.num_channels == 2)
			{
				//
				// Finish any previous test group
				//
				FinishGroup();

				//
				// Set up the channel group
				//
				_channel_group_name = text->getText().trim();
				_channel_group_passed = 1;
			}

			//
			// Set up the audio driver
			//
			ok = openAudioDevice(tp.sample_rate);
			if (!ok)
				return;

			//
			// show user prompt
			//
			tp.Setup(audioDevice->getCurrentBufferSizeSamples(),_tone,active_outputs);

			if (0 == tp.start_group)
			{
                startAudioDevice();
				rval = tp.ShowMeterWindow(_content, _dev, input_meters);
                stopAudioDevice();
				//DBG("audioDevice start and stop");
			}

			//
			// If the user selected "skip", move ahead to the next user prompt
			//
			if (TestPrompt::skip == rval)
			{
				_skipped = true;
				_channel_group_passed = 2;
				{
					_content->log(String::empty);
					_content->log(_channel_group_name + String(" skipped"));
				}

				//
				// Skip ahead
				//
				do
				{
					_script = _script->getNextElement();
					if (!_script)
						break;
				} while (_script->hasTagName("test") || _script->hasTagName("ignore"));

				continue;
			}

			_script = _script->getNextElement();
			continue;
		}

		//-----------------------------------------------------------------------------
		//
		// Run a test?
		//
		//-----------------------------------------------------------------------------

		if (_script->hasTagName("test"))
		{
			_test = Test::Create(_script,_input,_output,ok,this);

			if (!ok || (NULL == _test))
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Test object init failed; missing parameter?",
													false);
				return;
			}
            
            if (0 != _test->requiredTestAdapterProductId && false == aioTestAdapter.checkProductID( _test->requiredTestAdapterProductId))                
            {
                //
                // Ignore this test
                //
                _content->log(String::empty);
                _content->log("Not running test " + _test->title + " - not supported by this adapter");
                _script = _script->getNextElement();
                continue;
            }

			_num_tests++;

			if (_test->title.isNotEmpty())
			{
				_content->log(String::empty);
				_content->log(_test->title);
			}

			_test->Setup(audioDevice->getCurrentBufferSizeSamples(),_tone,active_outputs);

			ok = openAudioDevice(_test->sample_rate);
			if (!ok)
				return;

            startAudioDevice();
            Thread::sleep(20);  // prime the pump
			new_test.exchange(1);

			_script = _script->getNextElement();
			return;
		}


		//-----------------------------------------------------------------------------
		//
		// Audio output?
		//
		//-----------------------------------------------------------------------------

		if (_script->hasTagName("audio_output"))
		{
			AudioOutput ao(_script,ok);
			if (!ok)
				return;

			ao.Setup(audioDevice->getCurrentBufferSizeSamples(),_tone,active_outputs);

			//
			// Start the audio driver
			//
			ok = openAudioDevice(ao.sample_rate);
			if (!ok)
				return;

            startAudioDevice();

			_script = _script->getNextElement();
			continue;
		}

		//
		// Ignore ASIO_driver, CoreAudio_driver && Require_AIO_Test_Adapter
		//
		if (_script->hasTagName("ASIO_driver") ||
            _script->hasTagName("CoreAudio_driver") ||
            _script->hasTagName("Require_AIO_Test_Adapter") ||
            _script->hasTagName("User_prompt_serial_number"))
		{
			_script = _script->getNextElement();
			continue;
		}

		//-----------------------------------------------------------------------------
		//
		// Global input or output channel?
		//
		//-----------------------------------------------------------------------------

		if (_script->hasTagName("input_channel"))
		{
			_input = _script->getAllSubText().getIntValue();
			_script = _script->getNextElement();
			continue;
		}
		if (_script->hasTagName("output_channel"))
		{
			_output = _script->getAllSubText().getIntValue();
			_script = _script->getNextElement();
			continue;
		}


        //-----------------------------------------------------------------------------
		//
		// AcousticIO
		//
		//-----------------------------------------------------------------------------

#ifdef ACOUSTICIO_BUILD
		if (_script->hasTagName("AIO_set_mic_gain"))
		{
			Result result(_dev->setMicGain(_script));
            
            if (result.failed())
            {
                _content->log(String::empty);
                _content->log(result.getErrorMessage());
            }

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_set_amp_gain"))
		{
			Result result(_dev->setAmpGain(_script));
            
            if (result.failed())
            {
                _content->log(String::empty);
                _content->log(result.getErrorMessage());
            }

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_set_constant_current"))
		{
			Result result(_dev->setConstantCurrent(_script));

			if (result.failed())
			{
				_content->log(String::empty);
				_content->log(result.getErrorMessage());
			}

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_set_clock_source"))
		{
			Result result(_dev->setClockSource(_script));
			if (result.ok())
			{
				_content->log(String::empty);
				String tmp("Set Clock Source = ");
				tmp += _script->getAttributeValue(0);
				_content->log(tmp);
			}
			if (result.failed())
			{
				_content->log(String::empty);
				_content->log(result.getErrorMessage());
			}

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_set_USB_clock_rate"))
		{
			Result result(_dev->setUSBClockRate(_script));

			if (result.failed())
			{
				_content->log(String::empty);
				_content->log(result.getErrorMessage());
			}

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIOS_set_reference_voltage"))
        {
            Result result(_dev->setCalibrationReferenceVoltage(_script));
            
            if (result.failed())
            {
                _content->log(String::empty);
                _content->log(result.getErrorMessage());
            }
            
            _script = _script->getNextElement();
            continue;
        }

		if (_script->hasTagName("AIO_clear_RAM_calibration"))
		{
			Result result(_dev->clearRAMCalibrationData());

			if (result.failed())
			{
				_content->log(String::empty);
				_content->log(result.getErrorMessage());
			}

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_TEDS_test"))
		{
            runAIOTest(RunTEDSTest, "TEDS");
			continue;
		}

		if (_script->hasTagName("AIO_write_test_adapter"))
		{
			uint8 byte = (uint8)_script->getStringAttribute("byte").getHexValue32();

			/*int count =*/ aioTestAdapter.write(byte);
			// _content->log("HID write count:" + String(count) + " value:0x" + String::toHexString(byte));
			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_mic_supply_off_voltage_test"))
		{
            runAIOTest(RunCCVoltageTest, "Mic Supply off voltage");
			continue;
		}

		if (_script->hasTagName("AIO_mic_supply_on_voltage_test"))
		{
            runAIOTest(RunCCVoltageTest, "Mic Supply on voltage");
			continue;
		}

		if (_script->hasTagName("AIO_mic_supply_current_test"))
		{
            runAIOTest(RunCCCurrentTest, "Mic Supply current");
			continue;
		}
        
        if (_script->hasTagName("AIO_firmware_version_test"))
        {
            runAIOTest(RunUSBFirmwareVersionTest, "Firmware version");
            continue;
        }
        
        if (_script->hasTagName("AIO_flash_memory_test"))
        {
            runAIOTest(RunFlashMemoryTest, "Flash memory");
            continue;
        }
        
        if (_script->hasTagName("AIO_power_supply_reset_test"))
        {
            runAIOTest(RunPowerSupplyResetTest, "Power supply reset");
            if (false == _unit_passed)
            {
 //               break; fixme
            }
            continue;
        }
        
        if (_script->hasTagName("AIO_module_type_test"))
        {
            runAIOTest(RunModuleTypeTest, "Module type");
            if (false == _unit_passed)
            {
                break;
            }
            continue;
        }
        
        if (_script->hasTagName("AIO_calibration_verification_test"))
        {
            if (_unit_passed)
            {
                runAIOTest(RunCalibrationVerificationTest, "Calibration verification");
            }
            else
            {
                _script = _script->getNextElement();
            }
            continue;
        }
        
        if (_script->hasTagName("AIO_calibrate"))
        {
            int firstModule = -1;
            int writeToFlash = 0;
            
            if (false == getIntValue(_script, "module", firstModule))
            {
                _content->log("Missing module tag for AIO_calibrate");
            }
            
            getIntValue(_script, "write_to_flash", writeToFlash);
            
            _script = _script->getNextElement();

            if (_unit_passed && firstModule >= 0)
            {
                //
                // Destroy this object's AudioIODevice - this means that the
                // calibration has to be the last stage of the test
                //
                //audioDevice = nullptr;
                
                //
                // Start the calibration
                //
                CalibrationManagerConfiguration configuration;
                configuration.aioUSBDevice = _dev;
                configuration.audioIODevice = audioDevice;
                configuration.firstModule = firstModule;
                configuration.numModulesToCalibrate = 1;
                configuration.writeToFlash = writeToFlash != 0;
                calibrationManager->configure(configuration);
                calibrationManager->userAction(CalibrationManagerV2::ACTION_CALIBRATE);
				return;
            }
            
			continue;
        }
        
//         if (_script->hasTagName("AIOS_measure_resistance"))
//         {
//             _script = _script->getNextElement();
//             
//             if (_unit_passed)
//             {
//                 //
//                 // Destroy this object's AudioIODevice - this means that the
//                 // calibration has to be the last stage of the test
//                 //
//                 audioDevice = nullptr;
//                 
//                 //
//                 // Start the resistance measurement
//                 //
//                 calibrationManager.setSerialNumber(_serial_number);
//                 calibrationManager.startResistanceMeasurement(_dev);
//                 return;
//             }
//             
//             continue;
//         }
        
        if (_script->hasTagName("AIO_mikeybus"))
        {
            runAIOTest(MikeyBusRegisters, "MB registers");
            continue;
        }
    
        if (_script->hasTagName("Print_error_codes"))
        {
            printErrorCodes(_script);
            _script = _script->getNextElement();
            continue;
        }
#endif
        
        //-----------------------------------------------------------------------------
        //
        // Delay?
        //
        //-----------------------------------------------------------------------------
      
        if (_script->hasTagName("Delay_msec"))
        {
            Result runDelayTask(XmlElement *element, bool& running_);
            
            Result result(runDelayTask(_script, _running));
            
            _script = _script->getNextElement();
            continue;
        }
        
        //-----------------------------------------------------------------------------
        //
        // Finish this group of tests and display the pass/fail on the left?
        //
        //-----------------------------------------------------------------------------
        
        if (_script->hasTagName("Finish_group"))
        {
            FinishGroup();
            
            _script = _script->getNextElement();
            continue;
        }
        
        //-----------------------------------------------------------------------------
        //
        // Offline test?
        //
        //-----------------------------------------------------------------------------
        
        if (_script->hasTagName("Offline_test"))
        {
            runOfflineTest(_script);
            _script = _script->getNextElement();
            continue;
        }

		//-----------------------------------------------------------------------------
		//
		// Quit the application entirely?
		//
		//---------------------------------------------------------------------------- -

		if (_script->hasTagName("quit"))
		{
			_running = false;
			quit = true;
			break;
		}

		//-----------------------------------------------------------------------------
		//
		// Ignore this block?
		//
		//-----------------------------------------------------------------------------

		if (_script->hasTagName("ignore"))
		{
			_script = _script->getNextElement();
			continue;
		}

		//
		// Unknown tag name
		//
		_content->log(String("Unknown XML tag: ") + _script->getTagName());
		Cleanup();
		return;
	}
    
    if (false == deviceAttached)
    {
        DBG("Cancel script parsing - device no longer attached");
        return;
    }

	//
	// Finish the last group
	//
	FinishGroup();

	//
	// Pass or fail?
	//
	if (_num_tests)
    {
		String msg;
		String finalResult;
		Colour finalResultColor;
		int i;

		_content->log(String::empty);
		if (_unit_passed && !_skipped && _running)
		{
			msg = _serial_number + " passed.";
			finalResult = "UNIT PASSED";
			finalResultColor = Colours::limegreen;
		}
		else if (_unit_passed && _skipped)
		{
			msg = "*** " + _serial_number + " FAILED (skipped tests).";
			finalResult = "UNIT FAILED\n(skipped)";
			finalResultColor = Colours::red;
		}
		else if (_unit_passed && (!_running))
		{
			msg = "*** " + _serial_number + " FAILED(stopped).";
			finalResult = "UNIT FAILED\n(stopped)";
			finalResultColor = Colours::red;
			msg = _serial_number + " passed.";	//fixme
			finalResult = "UNIT PASSED";
			finalResultColor = Colours::limegreen;
		}
		else
		{
			msg = "*** " + _serial_number + " FAILED: ";
			finalResult = "UNIT FAILED";
			finalResultColor = Colours::red;
            
            int errorCodeCount = errorCodes.getCount();
            if (errorCodeCount != 0)
            {
                finalResult += "\n";
                
                for (i = 0; i < errorCodeCount; i++)
                {
                    String errorCode(errorCodes.getCodeAsString(i));
                    
                    errorCode += " ";
                    msg += errorCode;
                    finalResult += errorCode;
                }
            }
		}
		_content->log(msg);
		_content->log(String::empty);
		_content->setFinalResult(finalResult,finalResultColor);
	}
    
    logPerformanceInfo();
    
	_content->FinishTests(_unit_passed,_skipped);
	Cleanup();

	if (quit)
	{ 
		JUCEApplication::quit();
	}
}

static RelativeTime ticksToRelativeTime(int64 const ticks)
{
    return RelativeTime( double(ticks) / Time::getHighResolutionTicksPerSecond());
}

void ProductionUnit::logPerformanceInfo()
{
    String const msecString(" msec");
    
    {
        RelativeTime maxRequestTime(aioTestAdapter.getMaxRequestTime());
        double seconds = maxRequestTime.inSeconds();
        _content->log("Test adapter max HID request: " + String(seconds * 1000.0, 3) + msecString);
    }
    
    if (_dev)
    {
        RelativeTime maxRequestTime(ticksToRelativeTime(_dev->getMaxRequestTicks()));
        double seconds = maxRequestTime.inSeconds();
        _content->log("DUT max request: " + String(seconds * 1000.0, 3) + msecString);
    }
    
    {
        double seconds = ticksToRelativeTime(maxAudioDeviceCreateTicks).inSeconds();
        _content->log("Max audio device create: " + String(seconds * 1000.0, 3) + msecString);
    }
    
    {
        double seconds = ticksToRelativeTime(maxAudioDeviceOpenTicks).inSeconds();
        _content->log("Max audio device open: " + String(seconds * 1000.0, 3) + msecString);
    }
    
    {
        double seconds = ticksToRelativeTime(maxAudioDeviceStartTicks).inSeconds();
        _content->log("Max audio device start: " + String(seconds * 1000.0, 3) + msecString);
    }

    _content->log(String::empty);
}

void ProductionUnit::FinishGroup()
{
	if (_channel_group_name.isNotEmpty())
	{
		_content->AddResult(_channel_group_name,_channel_group_passed);
		_content->repaint();

		_channel_group_name = String::empty;
	}
}

bool ProductionUnit::createAudioDevice(XmlElement *script)
{
	//
	// Load the audio driver
	//
	String devicename;

	DBG("createAudioDevice " << (pointer_sized_int)audioDevice.get());
    
#if JUCE_WIN32
    String audioDriverTag("ASIO_driver");
#endif
#if JUCE_MAC
    String audioDriverTag("CoreAudio_driver");
#endif
    
	forEachXmlChildElementWithTagName(*script, child, audioDriverTag)
	{
		devicename = child->getAllSubText();
		if ((NULL == child) || devicename.isEmpty())
		{
			AlertWindow::showNativeDialogBox("Production Test",
				audioDriverTag + " XML tag missing.",
				false);
			return false;
		}

        int64 begin = Time::getHighResolutionTicks();
#if JUCE_WIN32
		ScopedPointer<AudioIODeviceType> type(AudioIODeviceType::createAudioIODeviceType_ASIO());
#endif
#if JUCE_MAC
		ScopedPointer<AudioIODeviceType> type(AudioIODeviceType::createAudioIODeviceType_CoreAudio());
#endif
		type->scanForDevices();
		StringArray deviceNames(type->getDeviceNames());  // This will now return a list of available devices of this type
		for (int j = 0; j < deviceNames.size(); ++j)
		{
			if (deviceNames[j] == devicename)
			{
				audioDevice = type->createDevice(devicename, devicename);
                
                int64 end = Time::getHighResolutionTicks();
                maxAudioDeviceCreateTicks = jmax(maxAudioDeviceCreateTicks, end - begin);
                
				DBG("createAudioDevice ok");
				return true;
			}
		}
	}

	AlertWindow::showNativeDialogBox(	"Production Test",
			"Audio driver not found: '" + devicename + "'",
										false);
	JUCEApplication::quit();
	_ok = false;
	return false;
}

bool ProductionUnit::openAudioDevice(int sample_rate)
{
	BitArray inputs,outputs;
	String err;
    int64 begin = Time::getHighResolutionTicks();

#ifdef PCI_BUILD
	int channels;

	channels = _dev->getcaps()->numrecchan();
	channels = jmin( 10, channels);
	inputs.setRange(0,channels,true);
	channels = _dev->getcaps()->numplaychan();
	channels = jmin( 10, channels);
	outputs.setRange(0,channels,true);
#else
	inputs.setRange(0,_dev->getcaps()->numrecchan(),true);
	outputs.setRange(0,_dev->getcaps()->numplaychan(),true);
#endif

	if (audioDevice->isOpen())
	{
		//DBG("audioDevice is open");
		if ((audioDevice->getCurrentSampleRate() != sample_rate) ||
			 (inputs != audioDevice->getActiveInputChannels ()) ||
			 (outputs != audioDevice->getActiveOutputChannels()))
		{
			audioDevice->close();
			DBG("audioDevice->close");
		}
	}

	if (false == audioDevice->isOpen())
	{
		DBG("configuring audioDevice");
        
        Array<int> availableBufferSizes(audioDevice->getAvailableBufferSizes());
        int bufferSize = availableBufferSizes.getLast();

		err = audioDevice->open(inputs,outputs,sample_rate,bufferSize);
		if (err.isNotEmpty())
		{
			AlertWindow::showNativeDialogBox(	"Production Test",
												err,
												false);
			return false;
		}
		DBG("audioDevice->open");
	}
    
    int64 end = Time::getHighResolutionTicks();
    maxAudioDeviceOpenTicks = jmax(maxAudioDeviceOpenTicks, end - begin);

	return true;
}

bool ProductionUnit::startAudioDevice()
{
    if (nullptr == audioDevice)
    {
        DBG("ProductionUnit::startAudioDevice - audio device not created");
        return false;
    }
 
    int64 begin = Time::getHighResolutionTicks();
    
    //Process::setPriority(Process::RealtimePriority);
    audioDevice->start(this);
    
    int64 end = Time::getHighResolutionTicks();
    maxAudioDeviceStartTicks = jmax(maxAudioDeviceStartTicks, end - begin);
    
    return true;
}

void ProductionUnit::stopAudioDevice()
{
    if (audioDevice)
    {
        audioDevice->stop();
    }
    
    //Process::setPriority(Process::NormalPriority);
}

Result ProductionUnit::CheckSampleRate()
{
    int64 ticksPerSecond = Time::getHighResolutionTicksPerSecond();

	if (audioDevice)
	{
		int bufferSize = audioDevice->getCurrentBufferSizeSamples();
		double deviceSampleRate = audioDevice->getCurrentSampleRate();
		int64 totalTicks = 0;
		double measuredSampleRate = 0.0;
		double elapsedSeconds = 0.0;
		int totalSamples = 0;
        
        /*
        for (int i = 1; i < timestampCount.get(); ++i)
        {
            int64 elapsedTicks = timestamps[i] - timestamps[i-1];
            double elapsedMsec = double(elapsedTicks) / ticksPerSecond;
            DBG("i:" << i << "  timestamp:" << timestamps[i] << "  " << elapsedMsec);
        }
        */

		if (timestampCount.get() != 0)
		{
			int lastTimestampIndex = timestampCount.get();
			int minimumSamples = roundDoubleToInt(0.25 * deviceSampleRate);
			int samplesRecorded = lastTimestampIndex * bufferSize;
			if (samplesRecorded >= minimumSamples)
			{
				int firstTimestampIndex = lastTimestampIndex / 4;
				lastTimestampIndex -= 1;

				totalTicks = timestamps[lastTimestampIndex] - timestamps[firstTimestampIndex];
				totalSamples = (lastTimestampIndex - firstTimestampIndex) * bufferSize;
			}
		}

		measuredSampleRate = totalSamples;
		if (totalTicks != 0)
		{
			elapsedSeconds = double(totalTicks) / ticksPerSecond;

			measuredSampleRate = totalSamples / elapsedSeconds;
		}

        String const audioClockSourceString("Audio clock source");
		if ((measuredSampleRate >= _test->minSampleRate) && (measuredSampleRate <= _test->maxSampleRate))
		{
            if (0 != _test->sample_rate_check)
            {
                _content->AddResult(audioClockSourceString, true);
            }
			return Result::ok();
		}

		String error;
		
		if (0 == _test->sample_rate_check)
		{
			error = "\tSample rate " + String(audioDevice->getCurrentSampleRate(), 1) + " Hz\n";
			error += "       Measured sample rate " + String(measuredSampleRate, 1) + " Hz\n";
			double ratio = measuredSampleRate / audioDevice->getCurrentSampleRate();
			ratio *= 100.0;
			error += "       Ratio " + String(ratio, 3) + "%\n";
			error += "       Allowed " + String(_test->minSampleRate) + "/" + String(_test->maxSampleRate);
		}
		else
		{
			error = "       Measured sample rate " + String(measuredSampleRate, 1) + " Hz\n";
			error += "       Check PLL circuitry!";
            
            _content->AddResult(audioClockSourceString, false);
		}

		return Result::fail(error);
	}

	return Result::ok();
}

File ProductionUnit::getOutputFolder()
{
	File logfolder;

#if JUCE_WIN32
	logfolder = File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
#endif
#if JUCE_MAC
	logfolder = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory();
#endif

#if ACOUSTICIO_BUILD
	logfolder = logfolder.getChildFile("AIO Test Results");
	logfolder.createDirectory();
#else
#pragma message("Not defined")
#endif

	return logfolder;
}

void ProductionUnit::CreateLogFile()
{
    File logfolder(getOutputFolder());
    
    logfile = logfolder.getChildFile(_serial_number + "-Log" + ".txt");
    logfile.appendText(newLine);
}

void ProductionUnit::deviceRemoved()
{
	deviceAttached = false;

	if (_running)
	{
		_content->log(String::empty);
		_content->log(String::empty);
		_content->log("*** Device removed ***");
		_content->log(String::empty);
		_content->log(String::empty);

		_content->setFinalResult("UNIT FAILED\n(removed)", Colours::red);

		Cleanup();
	}
}

#ifdef ACOUSTICIO_BUILD
void ProductionUnit::runAIOTest(AIOTestVector function, String const groupName)
{
    String msg;
    String displayedChannel;
    bool ok = function(_script,
                       _dev,
                       msg,
                       displayedChannel,
                       aioTestAdapter,
                       _content,
                       errorCodes,
                       unitTree) == TestPrompt::ok;
    
    _content->log(msg + "\n");
    
    _unit_passed &= ok;
    
    _num_tests++;
    
    _channel_group_name = groupName;
    
    if (displayedChannel.isNotEmpty())
    {
        _channel_group_name += " " + displayedChannel;
    }
	_channel_group_passed = ok;
    
    FinishGroup();
    
    _script = _script->getNextElement();
}

void ProductionUnit::finishCalibration()
{
    const String testName("Calibration");
    
    _content->calibrationComponent.setVisible(false);
    
    _content->log(testName);
    
    switch (calibrationManager->getState())
    {
    default:
        {
            _content->AddResult(testName, false);
            _content->log("*** Calibration failed");
            _content->log("*** Unexpected calibration manager state " + String((int)calibrationManager->getState()));
            _unit_passed = false;
        }
        break;
            
    case CalibrationManagerV2::STATE_ERROR:
        {
            _content->AddResult(testName, false);
            _content->log("*** Calibration failed");
            _content->log(calibrationManager->getResult().getErrorMessage());
            _unit_passed = false;
        }
        break;
    
    case CalibrationManagerV2::STATE_SHOW_ACTIVE_CALIBRATION:
        {
			bool pass = calibrationManager->isUnitDone();
            _content->AddResult(testName, pass);
            if (pass)
                _content->log("Calibration OK");
            else
                _content->log("Calibration FAIL");
            _content->log( calibrationManager->getData().toString() );
        }
        break;
    }

    ParseScript();
}


void ProductionUnit::finishAIOSResistanceMeasurement()
{
	/*
    bool pass;
    const String testName("AIO-S Resistance Measurement");
    
    _content->log(testName);
    
    switch (calibrationManager.getState())
    {
        default:
            {
                _content->AddResult(testName, false);
                _content->log("*** Resistance measurement failed");
                _content->log("*** Unexpected calibraton manager state " + String((int)calibrationManager.getState()));
                _unit_passed = false;
            }
            break;
            
        case CalibrationManager::STATE_CANCELLED:
            {
                _content->AddResult(testName, false);
                _content->log("*** Resistance measurement failed");
                _content->log(calibrationManager.getResults(pass));
                _unit_passed = false;
            }
            break;
            
        case CalibrationManager::STATE_RESISTANCE_MEASUREMENT_DONE:
            {
                String results(calibrationManager.getResults(pass));
                _unit_passed &= pass;
                _content->AddResult(testName, pass);
                //_content->log( "Calibration Data");
                //_content->log( calibrationManager.calibrationDataAIOS.toString());
                _content->log( results);
            }
            break;
    }
	*/
    
    ParseScript();
}

void ProductionUnit::printErrorCodes(XmlElement *xe)
{
#ifdef JUCE_MAC
    if (0 == errorCodes.getCount())
    {
        _content->log("No error codes\n");
        return;
    }
    
    String const headerTextTag("text");
    String headerText;
    XmlElement* headerTextElement = xe->getChildByName(headerTextTag);
    if (headerTextElement)
    {
        // don't use getStringValue to avoid trimming the text
        headerText = headerTextElement->getAllSubText();
    }

    String const firstChannelTag("first_channel");
    int firstChannel = -1;
    if (false == getIntValue( xe, firstChannelTag, firstChannel))
    {
        _content->log("Missing tag " + firstChannelTag);
        return;
    }
    if (firstChannel < 0 || firstChannel >= _dev->getcaps()->numbusin())
    {
        _content->log(firstChannelTag + " value " + String(firstChannel) + " out of range");
        return;
    }
    
    String const lastChannelTag("last_channel");
    int lastChannel = -1;
    if (false == getIntValue( xe, lastChannelTag, lastChannel))
    {
        _content->log("Missing tag " + lastChannelTag);
        return;
    }
    if (lastChannel < 0 || lastChannel >= _dev->getcaps()->numbusin())
    {
        _content->log(lastChannelTag + " value " + String(lastChannel) + " out of range");
        return;
    }
    
    String const selectedCodesString(getStringValue(xe, "codes"));
    StringArray selectedCodesStringArray;
    selectedCodesStringArray.addTokens(selectedCodesString, false);
    Array<int> selectedCodes;
    for (int i = 0; i < selectedCodesStringArray.size(); ++i)
    {
        int code = selectedCodesStringArray[i].getIntValue();
        
        DBG("XML code " + String(code));
        
        if (code < 0 ||code > ErrorCodes::LAST)
        {
            _content->log("Invalid code " + String(code) + " in Codes tag");
            return;
        }
        selectedCodes.add(code);
    }
    
    String output(NewLine::getDefault());
    output += _serial_number + headerText + NewLine::getDefault();
    output += testStartTime.toString(true,true) + newLine;
    
    Array<int> printedCodes;
    for (int i = 0; i < errorCodes.getCount(); ++i)
    {
        uint32 code = errorCodes.getCode(i);
        int channel = code >> 4;
        DBG("Unit has code " + String(code));
        if (selectedCodes.contains(code & 0xf) && firstChannel <= channel && channel <= lastChannel)
        {
            printedCodes.add(code);
            DBG("Matched code " + String(code));
        }
    }
    
    switch (printedCodes.size())
    {
        case 0:
            return;
            
        case 1:
            output += "Code: ";
            break;
            
        default:
            output += "Codes: ";
            break;
    }
    
    for (int i = 0; i < printedCodes.size(); ++i)
    {
        output += String::toHexString(printedCodes[i]).toUpperCase() + " ";
    }
    
    Printer::print(output);
    
    DBG("Error code print done");
#endif
}

#endif

void ProductionUnit::runOfflineTest(XmlElement *script)
{
    bool ok;
    int input = 0,output = 0;
    
    ScopedPointer<ToneGeneratorAudioSource> offlineTone = new ToneGeneratorAudioSource;
    ScopedPointer<Test> offlineTest = Test::Create(_script,input,output,ok,this);
    
    if (!ok || (nullptr == offlineTest))
    {
        AlertWindow::showNativeDialogBox(	"Production Test",
                                         "Test object init failed; missing parameter?",
                                         false);
        return;
    }
    
    uint32 activeOutputs = 0xffffffff;
    offlineTest->Setup(audioDevice->getCurrentBufferSizeSamples(),*offlineTone,activeOutputs);
    
    FileChooser fc("So pick a wave file already",File::getSpecialLocation(File::userDesktopDirectory),"*.wav");
    if (fc.browseForFileToOpen())
    {
        File f(fc.getResult());
        ScopedPointer<FileInputStream> inputStream = new FileInputStream(f);
        if (nullptr == inputStream || false == inputStream->openedOk())
        {
            AlertWindow::showNativeDialogBox(	"Production Test",
                                             "Could not open " + f.getFullPathName(),
                                             false);
            return;
        }
        
        WavAudioFormat format;
        ScopedPointer<AudioFormatReader> reader(format.createReaderFor(inputStream.release(), true));
        if (nullptr == reader)
        {
            AlertWindow::showNativeDialogBox(	"Production Test",
                                             "Could not read " + f.getFullPathName(),
                                             false);
            return;
        }
        
		int numSamples;
		if (reader->lengthInSamples > INT_MAX)
			numSamples = INT_MAX;
		else
			numSamples = (int)reader->lengthInSamples;
		AudioSampleBuffer *buffer = new AudioSampleBuffer(reader->numChannels, numSamples);
		reader->read(buffer, 0, numSamples, 0, true, true);
        
        OwnedArray<AudioSampleBuffer> buffers;
        buffers.add(buffer);
        String msg;
        ErrorCodes codes;
        offlineTest->calc(buffers, msg, codes);
        AlertWindow::showNativeDialogBox(	"Production Test",
                                         msg,
                                         false);
        return;
    }
}

void ProductionUnit::valueChanged(Value& value)
{
	CalibrationManagerV2::State const state(application->calibrationManager->getState());
    
    DBG("ProductionUnit::valueChanged " << state);

	if (CalibrationManagerV2::STATE_SHOW_ACTIVE_CALIBRATION == state)
	{
		finishCalibration();
		return;
	}
    
    switch (state)
    {
        case CalibrationManagerV2::STATE_SHOW_ACTIVE_CALIBRATION:
            finishCalibration();
            break;
            
        case CalibrationManagerV2::STATE_MODULE_READY:
            calibrationManager->userAction(CalibrationManagerV2::ACTION_CALIBRATE);
            break;
            
        default:
            break;
    }
}
