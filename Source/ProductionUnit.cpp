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

ProductionUnit::ProductionUnit(ehw *dev,ehwlist *devlist,Content *content) :
_dev(dev),
_devlist(devlist),
_asio(nullptr),
_content(content),
_ok(true),
_script(NULL),
active_outputs(0),
_unit_passed(true),
_skipped(false),
_num_tests(0),
_running(false)
{
	zerostruct(input_meters);

	_dev->incReferenceCount();

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
		AudioSampleBuffer *buffer = new AudioSampleBuffer(1,THDN_SAMPLES_REQUIRED);
		_inbuffs.add(buffer);
	}
}

ProductionUnit::~ProductionUnit(void)
{
	Cleanup();

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
	extern bool CloseUSBDevice(TUsbAudioHandle handle);
	CloseUSBDevice(_dev->GetNativeHandle());
#endif

	_dev->CloseDriver();
	_dev->decReferenceCount();

	DBG("ProductionUnit::~ProductionUnit");
}

void ProductionUnit::Cleanup()
{
}

bool ProductionUnit::status()
{
	return _ok;
}

void ProductionUnit::RunTests()
{
	_channel_group_name = String::empty;
	_content->Reset();
	_skipped = false;
	_unit_passed = true;
	_running = true;

	//
	// Load the set of tests from XML
	//
	File f;
	String devname(_dev->getcaps()->BoxTypeName()),txt;

	f = File::getSpecialLocation(File::currentApplicationFile);
#ifdef PCI_BUILD
	f = f.getParentDirectory().getChildFile("PCIProductionTests.xml");
	if (false == f.exists())
	{
		f = f.getParentDirectory().getParentDirectory().getChildFile("PCIProductionTests.xml");
	}

#else
	f = f.getParentDirectory();
	f = f.getChildFile("ProductionTests.xml");
	if (false == f.exists())
	{
		f = f.getParentDirectory().getParentDirectory().getChildFile("ProductionTests.xml");
	}
#endif
	XmlDocument myDocument(f);
	XmlElement *temp,*child;

	_root = myDocument.getDocumentElement();
	DBG(String("Parsing XML for ") + devname);
	if (_root)
	{
		child = _root->getFirstChildElement();
		while (child)
		{
			if (child->hasTagName("device"))
			{
				temp = child->getFirstChildElement();
				if (temp && temp->isTextElement())
				{
					txt = temp->getText();
					txt = txt.trimStart();
					txt = txt.trimEnd();
					DBG(txt);
					if (txt == devname)
					{
						_script = child;
						break;
					}
				}
			}
			child = child->getNextElement();
		}
	}
	else
	{
		AlertWindow::showNativeDialogBox(	"Production Test",
											"Could not load ProductionTests.xml.",
											false);
		JUCEApplication::quit();
		_ok = false;
		return;
	}

	if (!_ok || (NULL == _script))
	{
		AlertWindow::showNativeDialogBox(	"Production Test",
											"Could not parse ProductionTests.xml.",
											false);
		JUCEApplication::quit();
		return;
	}

	//
	// Create the ASIO driver
	//
	if (!CreateASIO(_script))
		return;

	//
	// onscreen logging
	//
	String msg;

	msg = T("Begin testing for ");
	msg += _dev->getcaps()->BoxTypeName();
	_content->log(String::empty);
	_content->log(String::empty);
	_content->log(T("---------------------------------------"));
	_content->log(msg);
	_content->log(Time::getCurrentTime().toString(true,true));
	_content->log(String::empty);

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
}

void ProductionUnit::audioDeviceAboutToStart(AudioIODevice *device)
{
}

void ProductionUnit::audioDeviceStopped()
{
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
	int in,out,i;
	float tonebuff[device_buffer_size_samples],*tonebuff_ptr = tonebuff;
	static int passes = 0;
	static LARGE_INTEGER last_timestamp = { 0, 0 };

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	if (0 != last_timestamp.QuadPart)
	{
		uint64 elapsed = now.QuadPart - last_timestamp.QuadPart;
		_average_callback_interval.QuadPart -= _average_callback_interval.QuadPart >> 5;
		_average_callback_interval.QuadPart += elapsed >> 5;
	}
	last_timestamp = now;

	//
	// Update test state if necessary
	//
	if (InterlockedExchange(&new_test,0))
	{
		record_done = false;
		callback_count = 0;
		InterlockedExchange(&blocks_recorded,0);
		last_timestamp.QuadPart = 0;
		_average_callback_interval.QuadPart = 0;
	}
	else
	{
		callback_count++;
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
	if (callback_count > callback_skip_count)
	{
		int count,temp;

		temp = InterlockedIncrement(&blocks_recorded);
		if (temp <= (THDN_SAMPLES_REQUIRED/device_buffer_size_samples))
		{
			temp = (temp-1)*device_buffer_size_samples;
			count = jmin(numSamples,THDN_SAMPLES_REQUIRED - temp);
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
			InterlockedDecrement(&blocks_recorded);
		}
	}

	//
	// Playback
	//
	AudioSourceChannelInfo asci;
	AudioSampleBuffer asb(&tonebuff_ptr,1,device_buffer_size_samples);
	float dc_offset = _dc_offset;
	int sawtooth = _sawtooth;
	int pulsate = _pulsate;

	asci.buffer = &asb;
	asci.numSamples = device_buffer_size_samples;
	asci.startSample = 0;
	_tone.getNextAudioBlock(asci);

	for (out = 0; out < numOutputChannels; out++)
	{
		if (active_outputs & (1 << out)) {
			for(int i = 0; i < device_buffer_size_samples; i++)
			{
				if(sawtooth)
					outputChannelData[out][i] = 0.5f * (float) i / (float) device_buffer_size_samples;
				else if (pulsate)
				{
					if(i < 40)
						if(passes < 50)
							outputChannelData[out][i] = 0.5f;
						else
							outputChannelData[out][i] = -0.5f;
					else
						outputChannelData[out][i] = 0.0f;
				}
				else
					outputChannelData[out][i] = tonebuff[i] + dc_offset;
			}
		}
		else
			memset(outputChannelData[out],0,numSamples*sizeof(float));
	}
	passes++;
	if(passes >= 100)
		passes = 0;
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

			if (_asio)
			{
				_asio->stop();
				//DBG("_asio->stop");
			}

			Process::setPriority(Process::NormalPriority);

			bool sample_rate_ok = CheckSampleRate();

			result = _test->calc(_inbuffs,msg);
			result &= sample_rate_ok;
			_unit_passed &= result;
			_channel_group_passed &= (int)result;

		#if 0
			if (false == result)
				_content->log(msg);
		#else
			if (false == result)
				msg = String("*** ") + msg;
			_content->log(msg);

			if (false == sample_rate_ok)
			{
				_content->log("*** Sample rate out of range");
			}
		#endif

			ParseScript();
		}
		break;

		case MESSAGE_MIDI_TEST_DONE:
		{
			Result result (midiLoopTest.getResult());
			String msg("\nMIDI loopback test OK");

			midiLoopTest.stop();

			_unit_passed &= result.wasOk();

			if (result.failed())
			{
				msg = String("\n*** MIDI loopback test failed");
			}
			_content->log(msg);
			_content->log(midiLoopTest.getReceivedData());

			ParseScript();
		}
		break;
	}
}

void ProductionUnit::ParseScript()
{
	bool ok;
	int rval;

	while (_script  && _running)
	{
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
			int yesno;

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
			// get "yes/no" flag
			//
			ok = getIntValue(_script,"show_yes_no",yesno);
			if (false == ok)
				yesno = 0;

			if (yesno)
			{
				ok = AlertWindow::showOkCancelBox(AlertWindow::NoIcon,"Production Test",text->getText(),T("Yes"),T("No"));
				_unit_passed &= ok;

				log_text = _script->getChildByName("log");
				if (log_text)
				{
					String msg;

					_content->log(String::empty);
					msg = log_text->getFirstChildElement()->getText();
					_content->AddResult(msg,(int)ok);
					msg += ": ";
					msg += ok ? "ok" : "failed";
					_content->log(msg);
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
			ok = OpenASIO(tp.sample_rate);
			if (!ok)
				return;

			//
			// show user prompt
			//
			tp.Setup(device_buffer_size_samples,_tone,active_outputs,_dc_offset,_sawtooth,_pulsate);

			if (0 == tp.start_group)
			{
				_asio->start(this);
				rval = tp.ShowMeterWindow(_content, _dev, input_meters);
				_asio->stop();
				//DBG("_asio start and stop");
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
			_test = Test::Create(_script,_input,_output,ok);

			if (!ok || (NULL == _test))
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Test object init failed; missing parameter?",
													false);
				return;
			}

			_num_tests++;

			if (_test->title.isNotEmpty())
			{
				_content->log(String::empty);
				_content->log(_test->title);
			}

			_test->Setup(device_buffer_size_samples,_tone,active_outputs,_dc_offset,_sawtooth,_pulsate);

			ok = OpenASIO(_test->sample_rate);
			if (!ok)
				return;

			Process::setPriority(Process::RealtimePriority);

			_asio->start(this);
			Sleep(20);  // prime the pump
			InterlockedExchange(&new_test,1);
			//DBG("_asio->start");

			_script = _script->getNextElement();
			return;
		}

		//-----------------------------------------------------------------------------
		//
		// Write to the test register?
		//
		//-----------------------------------------------------------------------------

#if ECHO1394
		if (_script->hasTagName(T("write_rip_test_register")))
		{
			extern bool WriteRIPTestRegister(ehw *dev,XmlElement *xe,uint32 &reg_value);
			uint32 reg_value;

			ok = WriteRIPTestRegister(_dev,_script,reg_value);
			if (!ok)
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Could not write test register.",
													false);
				JUCEApplication::quit();
				return;
			}

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Check the RIP status register?
		//
		//-----------------------------------------------------------------------------

#if ECHO1394
		if (_script->hasTagName(T("check_rip_status_register")))
		{
			extern bool CheckRIPStatusRegister(ehw *dev,XmlElement *xe,bool &passed);
			bool passed;
			String msg;
			XmlElement *text;

			text = _script->getFirstChildElement();
			if (!text->isTextElement())
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No title for status register check in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			ok = CheckRIPStatusRegister(_dev,_script,passed);
			if (!ok)
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Could not read status register.",
													false);
				JUCEApplication::quit();
				return;
			}

			msg = text->getText();
			msg += ": ";
			msg += passed ? "ok" : "failed";
			_content->log(msg);

			_unit_passed &= passed;
			if (!passed)
			{
				_content->FinishTests(false,_skipped);
				Cleanup();
				return;
			}

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Check the boxstatus register?
		//
		//-----------------------------------------------------------------------------

#if ECHO1394
		if (_script->hasTagName(T("boxstatus")))
		{
			extern bool CheckBoxstatusRegister(ehw *dev,XmlElement *xe,bool &passed);
			bool passed;
			String msg;
			XmlElement *text;

			text = _script->getFirstChildElement();
			if (!text->isTextElement())
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No title for status register check in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			ok = CheckBoxstatusRegister(_dev,_script,passed);
			if (!ok)
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Could not read boxstatus register.",
													false);
				JUCEApplication::quit();
				return;
			}

			_content->log(String::empty);
			msg = text->getText();
			_content->AddResult(msg,(int)passed);
			msg += ": ";
			msg += passed ? "ok" : "failed";
			_content->log(msg);

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Set digital mode?
		//
		//-----------------------------------------------------------------------------

#ifdef PCI_BUILD
		if (_script->hasTagName(T("setDigitalMode")))
		{
			int mode, result;
			XmlElement *text;

			text = _script->getFirstChildElement();
			if (!text->isTextElement())
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No type for digital mode in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			if (String("ADAT") == text->getText())
				mode = hwcaps::digital_mode_adat;
			else if (String("SPDIF_COAX") == text->getText())
				mode = hwcaps::digital_mode_spdif_coax;
			else if (String("SPDIF_OPTICAL") == text->getText())
				mode = hwcaps::digital_mode_spdif_optical;
			else
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Invalid digital mode type",
													false);
				JUCEApplication::quit();
				return;
			}

			if (_asio)
			{
				_asio->close();
			}

			result = _dev->setdigitalmode(mode);
			Sleep(500);
			DBG(String::formatted("setdigitalmode %d  result:%d",mode, result));

			mode = -1;
			_dev->getdigitalmode(mode);
			DBG(String::formatted("getdigitalmode %d",mode));

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Guitar charge mode?
		//
		//-----------------------------------------------------------------------------

#if ECHO1394
		if (_script->hasTagName(T("set_rip_charge_mode")))
		{
			bool SetChargeMode(ehw *dev,XmlElement *xe);

			SetChargeMode(_dev,_script);

			_script = _script->getNextElement();
			continue;
		}
#endif

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

			ao.Setup(device_buffer_size_samples,_tone,active_outputs,_dc_offset,_sawtooth,_pulsate);

			//
			// Start the audio driver
			//
			ok = OpenASIO(ao.sample_rate);
			if (!ok)
				return;

			_asio->start(this);
			//DBG("_asio->start");

			_script = _script->getNextElement();
			continue;
		}

		//
		// Ignore ASIO_driver
		//
		if (_script->hasTagName("ASIO_driver"))
		{
			_script = _script->getNextElement();
			continue;
		}

		//-----------------------------------------------------------------------------
		//
		// Test MIDI in and out?
		//
		//-----------------------------------------------------------------------------

#if ECHO1394
		if (_script->hasTagName(T("testMIDI")))
		{
			extern bool MidiTest(ehw *dev,bool &passed);
			bool passed;

			String msg;
			XmlElement *text;

			text = _script->getFirstChildElement();
			if (!text->isTextElement())
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No title for MIDI check in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			ok = MidiTest(_dev, passed);
			if (!ok)
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"Could not open MIDI device.",
													false);
				JUCEApplication::quit();
				return;
			}

			_content->log(String::empty);
			msg = text->getText();
			_content->AddResult(msg,(int)passed);
			msg += ": ";
			msg += passed ? "ok" : "failed";
			_content->log(msg);
			_unit_passed &= passed;

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Show message box?
		//
		//-----------------------------------------------------------------------------

#if ECHO1394
		if (_script->hasTagName(T("softclip")))
		{
			int rval;
			int set;

			ok = getIntValue(_script,T("value"), set);

			if(ok)
			{
				if (set)
					rval = _dev->changeboxflags( efc_flag_soft_clip_enabled, 0);
				else
					rval = _dev->changeboxflags( 0, efc_flag_soft_clip_enabled);
				}
			else
			{
				AlertWindow::showNativeDialogBox(	"Production Test",
													"No setting for softclip in script.",
													false);
				JUCEApplication::quit();
				return;
			}

			_script = _script->getNextElement();
			continue;
		}
#endif

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
		// MIDI loopback test?
		//
		//-----------------------------------------------------------------------------

		if (_script->hasTagName("MIDILoopTest"))
		{
			midiLoopTest.start(_script, this);

			_num_tests++;

			_script = _script->getNextElement();
			return;
		}

		//-----------------------------------------------------------------------------
		//
		// Input clock detect?
		//
		//-----------------------------------------------------------------------------

#ifdef ECHOPCI
		if (_script->hasTagName(T("ClockDetectTest")))
		{
			clockDetectTest();

			_num_tests++;

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Software phantom power switch?
		//
		//-----------------------------------------------------------------------------

#if defined(ECHOPCI) || defined(ECHO1394)
		if (_script->hasTagName(T("PhantomPower")))
		{
			int mode = _script->getAllSubText().getIntValue();
			_dev->setphantom(mode);

			_script = _script->getNextElement();
			continue;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// Touch panel stuff for Echo2?
		//
		//-----------------------------------------------------------------------------

#if defined(ECHOUSB) && defined(ECHO2_BUILD)
		if (_script->hasTagName("set_touch_panel_reg"))
		{
			extern void SetTouchPanelRegister(XmlElement const *element,ehw *dev);

			SetTouchPanelRegister(_script,_dev);

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("echo2_touch_panel_test"))
		{
			String msg;
			bool ok = RunTouchPanelTest(_content,_dev,msg) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_channel_group_name = "Touch panel";
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("echo2_initial_power_test"))
		{
			String msg;
			bool ok = RunEcho2InitialPowerTest(_content,_dev,_devlist,msg) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_channel_group_name = "Initial power check";
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("echo2_final_power_test"))
		{
			String msg;
			bool ok = RunEcho2FinalPowerTest(_content,_dev,_devlist,msg) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_channel_group_name = "Final power check";
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			break;
		}
#endif

		//-----------------------------------------------------------------------------
		//
		// AcousticIO
		//
		//-----------------------------------------------------------------------------

#ifdef ACOUSTICIO_BUILD
		if (_script->hasTagName("AIO_set_mic_gain"))
		{
			_dev->setMicGain(_script);

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_set_amp_gain"))
		{
			_dev->setAmpGain(_script);

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_set_constant_current"))
		{
			_dev->setConstantCurrent(_script);

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_TEDS_test"))
		{
			bool RunTEDSTest(XmlElement const *element, ehw *dev, String &msg, int &input);

			String msg;
			int input;
			bool ok = RunTEDSTest(_script, _dev, msg, input) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_num_tests++;

			_channel_group_name = "TEDS";
			if (input == 4)
			{
				_channel_group_name += " 1-4";
			}
			if (input == 8)
			{
				_channel_group_name += " 5-8";
			}
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_write_test_adapter"))
		{
			uint8 byte = (uint8)_script->getStringAttribute("byte").getHexValue32();

			int count = _content->aioTestAdapter.write(byte);
			// _content->log("HID write count:" + String(count) + " value:0x" + String::toHexString(byte));
			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_mic_supply_off_voltage_test"))
		{
			bool RunCCVoltageTest(XmlElement const *element, String &msg, int &displayedInput, AIOTestAdapter &testAdapter);

			String msg;
			int input;
			bool ok = RunCCVoltageTest(_script, msg, input, _content->aioTestAdapter) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_num_tests++;

			_channel_group_name = "Mic Supply off voltage ";
			if (input >= 0)
			{
				_channel_group_name += " " + String(input) + "-" + String(input + 3);
			}
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_mic_supply_on_voltage_test"))
		{
			bool RunCCVoltageTest(XmlElement const *element, String &msg, int &displayedInput, AIOTestAdapter &testAdapter);

			String msg;
			int input;
			bool ok = RunCCVoltageTest(_script, msg, input, _content->aioTestAdapter) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_num_tests++;

			_channel_group_name = "Mic Supply on voltage ";
			if (input >= 0)
			{
				_channel_group_name += " " + String(input) + "-" + String(input + 3);
			}
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			continue;
		}

		if (_script->hasTagName("AIO_mic_supply_current_test"))
		{
			bool RunCCCurrentTest(XmlElement const *element, String &msg, int &displayedInput, AIOTestAdapter &testAdapter);

			String msg;
			int input;
			bool ok = RunCCCurrentTest(_script, msg, input, _content->aioTestAdapter) == TestPrompt::ok;

			_content->log(String::empty);
			_content->log(msg);

			_unit_passed &= ok;

			_num_tests++;

			_channel_group_name = "Mic Supply current ";
			if (input >= 0)
			{
				_channel_group_name += " " + String(input) + "-" + String(input + 3);
			}
			_channel_group_passed = ok;

			FinishGroup();

			_script = _script->getNextElement();
			continue;
		}
#endif

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

		_content->log(String::empty);
		if (_unit_passed && !_skipped && _running)
			msg = T("Unit passed.");
		else if (_unit_passed && _skipped)
			msg = T("Unit failed (skipped tests).");
		else if (_unit_passed && (!_running))
			msg = T("Unit failed (stopped).");
		else
			msg = T("Unit failed.");
		_content->log(msg);
		AlertWindow::showMessageBox(AlertWindow::NoIcon,"Production Test",msg,"OK");
	}

	_content->FinishTests(_unit_passed,_skipped);
	Cleanup();
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

bool ProductionUnit::CreateASIO(XmlElement *script)
{
	//
	// Load the ASIO driver
	//
	AudioDeviceManager *manager = _content->GetAudioDeviceManager();
   const OwnedArray <AudioIODeviceType> &types(manager->getAvailableDeviceTypes());
	String asioname;

	DBG_PRINTF((T("CreateASIO %p"),_asio));

	forEachXmlChildElementWithTagName(*script,child,"ASIO_driver")
	{
		asioname = child->getAllSubText();
		if ((NULL == child) || asioname.isEmpty())
		{
			AlertWindow::showNativeDialogBox(	"Production Test",
												"ASIO driver XML tag missing.",
												false);
			return false;
		}

		AudioDeviceManager::AudioDeviceSetup setup;

		setup.outputDeviceName = asioname;
		setup.inputDeviceName = asioname;
		String result = manager->setAudioDeviceSetup(setup,true);
		DBG("setAudioDeviceSetup:");
		DBG(result);
		if (result.isEmpty())
		{
			_asio = manager->getCurrentAudioDevice();
			return true;
		}

		 for (int i = 0; i < types.size(); ++i)
		 {
			  String typeName (types[i]->getTypeName());  // This will be things like "DirectSound", "CoreAudio", etc.

			  DBG(typeName);
				if (typeName != String("ASIO"))
					continue;

			  types[i]->scanForDevices();                 // This must be called before getting the list of devices

			  StringArray deviceNames (types[i]->getDeviceNames());  // This will now return a list of available devices of this type

			  for (int j = 0; j < deviceNames.size(); ++j)
			  {
				if (deviceNames[j] == asioname)
				{
					_asio = types[i]->createDevice(asioname,asioname);
					DBG("CreateASIO ok");
					return true;
				}
			  }
		 }
	}

	AlertWindow::showNativeDialogBox(	"Production Test",
										"ASIO driver not found.",
										false);
	JUCEApplication::quit();
	_ok = false;
	return false;
}

bool ProductionUnit::OpenASIO(int sample_rate)
{
	BitArray inputs,outputs;
	String err;

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

	if (_asio->isOpen())
	{
		//DBG("_asio is open");
		if ((_asio->getCurrentSampleRate() != sample_rate) ||
			 (inputs != _asio->getActiveInputChannels ()) ||
			 (outputs != _asio->getActiveOutputChannels()))
		{
			_asio->close();
			DBG("_asio->close");
		}
	}

	if (false == _asio->isOpen())
	{
		//DBG("configuring _asio");
		err = _asio->open(inputs,outputs,sample_rate,device_buffer_size_samples);
		if (err.isNotEmpty())
		{
			AlertWindow::showNativeDialogBox(	"Production Test",
												err,
												false);
			return false;
		}
		//DBG("_asio->open");
	}

	return true;
}

bool ProductionUnit::CheckSampleRate()
{
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);

	if (_asio)
	{
		double msec = (_average_callback_interval.QuadPart * 1000.0)/((double)freq.QuadPart);
		double expected_msec = 1000.0 * _asio->getCurrentBufferSizeSamples()/_asio->getCurrentSampleRate();
		double ratio = 100.0 * msec / expected_msec;
		DBG_PRINTF(("Average callback interval %f msec",msec));
		DBG_PRINTF(("Sample rate %f",_asio->getCurrentSampleRate()));
		DBG_PRINTF(("Expected %f msec",expected_msec));
		DBG_PRINTF(("Ratio %f %%",ratio));

		return (ratio <= 104.0) && (ratio >= 96.0);
	}

	return true;
}