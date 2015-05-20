/********************************************************************************
 *
 * ehw.h
 *
 * A single Echo USB hardware device
 *
 * Hardware calls return:
 *
 * 0 for success
 * 1 or more for an error
 * -1 or less if the console should try again later
 *
 ********************************************************************************/
#if _WIN32

#ifndef _Ehw_h_
#define _Ehw_h_

#include <setupapi.h>
typedef unsigned __int8 byte;
#ifdef WINDOWS_VOLUME_CONTROL
#include "SysVolume.h"
#endif
#include "tusbaudioapi.h"
#include "../Session.h"
#include "../hwcaps.h"

//
// ehw
//
class ehwlist;
class MixDevList;
class SysVolume;

#ifndef DBG_PRINTF
#define DBG_PRINTF(x) DBG(String::formatted x )
#endif

class ehw : public ReferenceCountedObject
{
public:
	typedef ReferenceCountedObjectPtr<ehw> ptr;

	ehw(
		unsigned device_index,
		CriticalSection *lock
		);	
	~ehw();
	
	void removed();
	
	hwcaps *getcaps()
	{	
		return &_caps;
	}
	
	String GetUniqueName()
	{
		return _uniquename;
	}
	
	String GetIdString();

	uint32 GetVendorId();
	uint32 GetBoxType();
	uint32 GetBoxRev();

	uint64 GetSerialNumber();

	uint32 GetDriverVersion();

	SESSION &GetSession()
	{
		return session;
	}
	
	void AddMixerListener(ChangeListener *cl);
	void RemoveMixerListener(ChangeListener *cl);

	TUsbAudioHandle GetNativeHandle()
	{
		return handle;
	}
	
	
	//------------------------------------------------------------------------------
	// Open and close the driver
	//------------------------------------------------------------------------------

	int OpenDriver();
	void CloseDriver();
	
	//efr_polled_stuff* getpolledstuff()
	//{
	//	return &(_pstuff.stuff);
	//}
	int updatepolledstuff();
	
	uint32 getFirmwareVersion() const;
	String getFirmwareVersionString() const;

	//------------------------------------------------------------------------------
	// Hardware controls
	//------------------------------------------------------------------------------

	enum
	{
		spdif_pro_format = 2	// Matches the 1394 driver
	};

	int changeboxflags(uint32 setmask,uint32 clearmask);
	int getboxflags(uint32 &flags);

	int changedriverflags(uint32 ormask,uint32 andmask);

	int getmirror(int &output);
	int setmirror(int output);

	int getdigitalmode(int &mode);
	int setdigitalmode(int mode);

	int getphantom(int &phantom);
	int setphantom(int phantom);
	
	int identify();

	uint32 GetClockEnableMask();
	uint32 ClockDetected(int clock);

	int SetCoreAudioRate(uint32 rate);
	int GetCoreAudioRate(uint32 &rate);

	int SendTestBuffer (uint8 const *buffer,size_t buffer_size);
	int ReceiveTestBuffer (uint8 *buffer,size_t buffer_size,size_t &bytes_received);


	//------------------------------------------------------------------------------
	// Input controls
	//------------------------------------------------------------------------------

	int getinshift(int input,int &shift);
	int setinshift(int input,int shift);

	int GetInputMeter(int input);

	int setingain(int input,float gain);


	//------------------------------------------------------------------------------
	// Monitor controls
	//------------------------------------------------------------------------------
	
	int getmonmute(uint32 input,uint32 output,uint32 &mute);
	int setmonmute(uint32 input,uint32 output,uint32 mute);

	int getmongain(uint32 input,uint32 output,float &gain);
	int setmongain(uint32 input,uint32 output,float gain);

	int getmonsolo(uint32 input,uint32 output,uint32 &solo);
	int setmonsolo(uint32 input,uint32 output,uint32 solo);
	
	int setmonpan(uint32 input,uint32 output,float pan);
	int getmonpan(uint32 input,uint32 output,float &pan);

	
	//------------------------------------------------------------------------------
	// Playback mixer controls
	//------------------------------------------------------------------------------

	int setplaygain(uint32 virtout,uint32 output,float gain);
	int setplaymute(uint32 virtout,uint32 output,uint32 mute);
	int setplaypan(uint32 virtout,uint32 output,float pan);
	int getplaygain(uint32 virtout,uint32 output,float &gain);
	int getplaymute(uint32 virtout,uint32 output,uint32 &mute);
	int getplaypan(uint32 virtout,uint32 output,float &pan);
	int setplaysolo(uint32 output,uint32 solo);
	int getplaysolo(uint32 output,uint32 &solo);

	
	//------------------------------------------------------------------------------
	// Master output controls
	//------------------------------------------------------------------------------

	int setmastergain(uint32 output,float gain);
	int setmastermute(uint32 output,uint32 mute);
	int getmastergain(uint32 output,float &gain);
	int getmastermute(uint32 output,uint32 &mute);
	int getoutshift(int output,int &shift);
	int setoutshift(int output,int shift);

	int GetMasterOutMeter(int output);


	//------------------------------------------------------------------------------
	// Transport controls
	//------------------------------------------------------------------------------

	enum
	{
		clock_not_specified = 0xffffffff,

		ASIO_BUSY = 1000,
		WINDOWS_AUDIO_BUSY,
		USB_MODE_ERROR
	};

#if TUSBAUDIO_API_VERSION_MJ >=4 
	int getbuffsize(uint32 &aframes);
	int setbuffsize(uint32 aframes);
	int getusbmode(int &mode);
	int setusbmode(int mode);
#else
	int getbuffsize(uint32 &usec);
	int setbuffsize(uint32 usec);

	int getUSBBufferSizes(Array<int> &sizes,unsigned &current);
	int getUSBBufferSize(int &usec);
	int setUSBBufferSize(int usec);

	int getAsioBufferSizes(Array<int> &sizes,unsigned &current);
#endif

	int getclock(int32 &samplerate,int32 &clock);
	int setclock(int32 samplerate,int32 clock);

	int getratelock(bool &locked);
	int setratelock(bool locked);
	
	int opentransport();
	int settransportevent(HANDLE transport_event);
	int closetransport();
	int starttransport();

#if ACOUSTICIO_BUILD

	Result setMicGain(XmlElement const *element);
	Result setAmpGain(XmlElement const *element);
	Result setConstantCurrent(XmlElement const *element);
	Result setConstantCurrent(uint8 const input, uint8 const enabled);
	Result readTEDSData(uint8 const input, uint8* data, size_t dataBufferBytes); 
	Result setAIOSReferenceVoltage(XmlElement const *element);
	Result setAIOSReferenceVoltage(bool const enabled);
	Result readFlashBlock(uint8 const block, uint8 * const buffer, size_t const bufferBytes);
	Result writeFlashBlock(uint8 const block, uint8 const * const buffer, size_t const bufferBytes);
	Result clearRAMCalibrationData();

#endif


	//------------------------------------------------------------------------------
	// MIDI status
	//------------------------------------------------------------------------------

	uint32 GetMidiStatus();
	bool MidiInActive(int input);
	bool MidiOutActive(int output);


	//------------------------------------------------------------------------------
	//  conversion routines
	//------------------------------------------------------------------------------

	static int32 DbToLin(float gaindb);
	static float LinToDb(uint32 gainlin);
	static uint32 PanFloatToEfc(float pan);
	static float EfcToPanFloat(uint32 pan);
	static float PanFloatToDb(float pan);

protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ehw)

	friend class ehwlist;
	friend class SysVolume;
	
	ehw *_prev;
	ehw *_next;
	
	hwcaps	_caps;
	String	_uniquename;
	
	CriticalSection *_lock;	
	bool localLock;

	unsigned deviceIndex;
	TUsbAudioHandle handle;
	TUsbAudioDeviceProperties props;
	int		_ok;
	String	_error;
	DWORD	m_dwRefCount;
	ScopedPointer<PropertiesFile> _file;

	int meters[10];
	
	SESSION session;
	void initializeSession();
	void loadSession();
	void saveSession();

	int setOutMixer(int output);
	int setInMixer(int input,int output);
	int setInMixers(int output);
	
#ifdef WINDOWS_VOLUME_CONTROL
	SysVolume *_SysVolume;
#endif

	enum
	{
		CUR = 1,
		GNODE_USB_TIMEOUT_MILLISECONDS = 20
	};

	enum
	{
		GNODE_GUITAR_COMMUNICATION_XU = 0xfb,

		GNODE_GUITAR_COMMUNICATION_CONTROL_SELECTOR = 0,
		GNODE_BLUETOOTH_UART_CONTROL_SELECTOR,

		GNODE_GET_BLUETOOTH_STATUS_SELECTOR = 0x40,
		GNODE_DIGITAL_PASS_THROUGH_SELECTOR,
		GNODE_MIDI_TEST_MODE_SELECTOR,
		GNODE_TEST_SELECTOR,
		GNODE_AUDIO_SELECTOR
	};

	enum
	{
		//
		// pass as channel number with GNODE_AUDIO_SELECTOR
		//
		GNODE_PEAK_METERS = 0
	};

};

#define COMMAND_TIMEOUT_MSEC				1000


#endif // _Ehw_h_

#endif


