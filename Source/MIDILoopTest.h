#pragma once

class ehw;

class MIDILoopTest : public MidiInputCallback, public Timer 
{
public:
	MIDILoopTest();
	~MIDILoopTest();

	bool start(XmlElement *xe, MessageListener *listener);
	void stop();

	void 	handleIncomingMidiMessage (MidiInput *source, const MidiMessage &message);

	void 	timerCallback ();

	Result getResult();
	String getReceivedData();

protected:
	ScopedPointer<MidiInput> inputDevice;
	ScopedPointer<MidiOutput> outputDevice;
	MessageListener *messageTarget;

	enum
	{
		EXPECTED_MESSAGES = 16
	};

	int messageReceivedCount;
	Array <uint8> receivedData;
};


