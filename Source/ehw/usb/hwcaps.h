#ifndef _hwcaps_h_
#define _hwcaps_h_

class hwcaps
{
protected:
	unsigned productId;
	friend class ehw;

	enum
	{
		max_chan = 64
	};
	
public:
	//
	// Connector types
	//
	enum
	{
		analog = 0,
		spdif,
		adat,
		digital,
		analog_control_room,
		headphones,
		i2s,
		guitar,
		piezo_guitar,
		guitar_string,

		virt = 0x10000,
		dummy
	};

	//
	// Clock types
	// 
	enum
	{
		internal_clock = 0,
		syt_clock,
		word_clock,
		spdif_clock,
		adat_clock,
		adat_b_clock,
		variable_clock,

		num_clock_sources,

		not_specified = 0xff
	};

	enum
	{
		ECHO2 = 0x0010,
		ECHO4 = 0x0040
	};
	
	void init(unsigned product_id);
	void dump(Logger *log);

	int32 numbusin();
	int32 numbusout();
	int32 numplaychan(int samplerate = 48000);
	int32 numrecchan(int samplerate = 48000);
	
	int32 NumMidiOut();
	int32 NumMidiIn();

	char const *BoxTypeName();
	
	int NumMixInGroups();
	int MixInGroupSize(int group);
	char const *MixInGroupName(int group);
	char const *MixInGroupShortName(int group);
	int MixInGroup(int chan);
	int MixInGroupOffset(int chan);
	int MixInType(int chan);
	
	bool HasPhysicalInputs() { return true; }
	bool HasInputNominal(int inbus);
	
	int NumMixOutGroups();
	int MixOutGroupSize(int group);
	char const *MixOutGroupName(int group);
	char const *MixOutGroupShortName(int group);
	int MixOutGroup(int chan);
	int MixOutGroupOffset(int chan);
	int MixOutType(int chan);
	bool HasOutputNominal();

	int MixPlayType(int chan);
	int32 NumVirtualOutputs() { return 0; }
	
	int MinSampleRate();
	int MaxSampleRate();
	
	bool HasAnalogMirror();

	bool HasDigitalModeSwitch();
	
	bool HasSpdifOut();
	bool HasSpdifIn();

	bool ExternalSync();
	bool ClockSupported(int clock);
	void GetClockName(int clock,String &name);

	bool HasIsocAudioMapping();
	bool HasOutputBussing();

	bool HasDsp();
	uint32 DspVersion();
	
	uint32 ArmVersion();

	bool HasFpga();
	uint32 FpgaVersion();

	bool HasPhantomPower();
	
	bool HasSoftClip();
	
	bool SupportsRobotGuitar();
	bool SupportsGuitarCharging();
};


#endif