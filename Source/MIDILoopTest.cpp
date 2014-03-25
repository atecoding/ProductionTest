#include "base.h"
#include "MIDILoopTest.h"
#include "xml.h"
#include "OldMessage.h"

MIDILoopTest::MIDILoopTest()
{
}


MIDILoopTest:: ~MIDILoopTest()
{
}


void dump (const MidiMessage &message)
{
	int index;
	String text;

	for (index = 0; index < message.getRawDataSize(); index ++)
	{
		text += String::toHexString(message.getRawData() + index, 1, 1);
	}

	DBG(text);

}

bool MIDILoopTest::start(XmlElement *xe, MessageListener *listener)
{
	String name (xe->getAllSubText());
	StringArray input_list (MidiInput::getDevices());
	StringArray output_list (MidiOutput::getDevices());
	int index, channel;

	messageTarget = listener;

	stop();

	messageReceivedCount = 0;
	receivedData.clearQuick();
	receivedData.ensureStorageAllocated(EXPECTED_MESSAGES*3);

	name = name.trim();
	for (index = 0; index < input_list.size(); index ++)
	{
		if (input_list [index].contains (name))
		{
			inputDevice = MidiInput::openDevice(index, this);
			if (inputDevice)
			{
				inputDevice->start();
			}
			break;
		}
	}

	jassert(inputDevice);

	for (index = 0; index < output_list.size(); index ++)
	{
		if (output_list [index].contains (name))
		{
			outputDevice = MidiOutput::openDevice(index);
			if (outputDevice)
			{
				for (channel = 1; channel <= 16; channel ++)
				{
					MidiMessage message (MidiMessage::noteOff(channel,~channel & 0x7f));

					//dump (message);
					outputDevice->sendMessageNow(message);
				}
			}
			break;
		}
	}

	startTimer(2000);

	return (nullptr != inputDevice) && (nullptr != outputDevice);
}


void MIDILoopTest::stop()
{
	if (inputDevice)
	{
		inputDevice->stop();
		inputDevice = nullptr;
	}

	outputDevice = nullptr;

	stopTimer();
}


void 	MIDILoopTest::handleIncomingMidiMessage (MidiInput * /*source*/, const MidiMessage &message)
{
	int index;
	int note = message.getNoteNumber();
	int channel = message.getChannel();

	for (index = 0; index < message.getRawDataSize(); index ++)
	{
		receivedData.add (message.getRawData()[index]);
	}

	if (message.isNoteOff() &&
		(channel == (messageReceivedCount + 1)) &&
		 (note == (~channel & 0x7f)))
	{
		messageReceivedCount++;
		if (EXPECTED_MESSAGES == messageReceivedCount)
		{
			messageTarget->postMessage (new OldMessage(MESSAGE_MIDI_TEST_DONE,0,0,nullptr));
		}
	}
}


void 	MIDILoopTest::timerCallback ()
{
	if (EXPECTED_MESSAGES != messageReceivedCount)
		messageTarget->postMessage (new OldMessage(MESSAGE_MIDI_TEST_DONE,0,0,nullptr));
	stopTimer();
}


Result MIDILoopTest::getResult()
{
	if (EXPECTED_MESSAGES == messageReceivedCount)
		return Result::ok();

	return Result::fail("Not enough MIDI messages received");
}


String MIDILoopTest::getReceivedData()
{
	int index;
	String text;

	for (index = 0; index < receivedData.size(); index ++)
	{
		text += String::toHexString(0xff & (int)(receivedData [index]));
		text += ' ';
	}

	return text;
}