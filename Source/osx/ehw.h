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

#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>
#include <IOKit/IOCFPlugIn.h>

#include "hwcaps.h"

#if ACOUSTICIO_BUILD
#include "Description.h"
#include "../calibration/CalibrationData.h"
#endif

//
// ehw
//
class ehwlist;


#ifndef DBG_PRINTF
#define DBG_PRINTF(x) DBG(String::formatted x )
#endif

class ehw : public ReferenceCountedObject
{
public:
	typedef ReferenceCountedObjectPtr<ehw> ptr;

    ehw(IOUSBDeviceInterface** deviceInterface_);
	~ehw();
	
	void removed();
    
    Description const * const getDescription() const
    {
        return description;
    }
    
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

	uint32 GetDriverVersion();
	
	
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
	
#if ACOUSTICIO_BUILD

    Result setMicGain(uint8 channel, uint8 gain);
    Result setAmpGain(uint8 channel, uint8 gain);
    Result setMicGain(XmlElement const *element);
	Result setAmpGain(XmlElement const *element);
	Result setConstantCurrent(XmlElement const *element);
	Result setConstantCurrent(uint8 const input, uint8 const enabled);
    Result readTEDSData(uint8 const input, uint8* data, size_t dataBufferBytes);
    Result setAIOSReferenceVoltage(XmlElement const *element);
    Result setAIOSReferenceVoltage(int const module, bool const enabled);
    Result readFlashBlock(uint8 const block, uint8 * const buffer, size_t const bufferBytes);
    Result writeFlashBlock(uint8 const block, uint8 const * const buffer, size_t const bufferBytes);
    Result clearRAMCalibrationData();
    Result setCalibrationData(AcousticIOCalibrationData const * const data);
    Result getCalibrationData(AcousticIOCalibrationData * const data);
    
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
	
	ehw *_prev;
	ehw *_next;
	
	hwcaps	_caps;
	String	_uniquename;
    uint16 firmwareVersion;
	
    int		_ok;
	String	_error;

	enum
	{
		CUR = 1,
        USB_REQUEST_FROM_DEV = 0xa1,
        USB_REQUEST_TO_DEV = 0x21
	};
    
    IOUSBDeviceInterface** deviceInterface;
    ScopedPointer<Description> description;
    
    uint8 getModuleTypes();
    
    Result createResult(IOReturn const status);
    IOReturn setRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length);
    IOReturn getRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length);
};

#define COMMAND_TIMEOUT_MSEC				2000



