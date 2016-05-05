/********************************************************************************
 *
 * ehw.cpp
 *
 ********************************************************************************/

#ifdef _WIN32

#pragma warning(disable:4100)

#include <new>
#include <windows.h>
#include <winioctl.h>
#include "../../../base.h"
#include "ehw.h"
#include "ehwlist.h"
#include "../hwcaps.h"
#include "NotificationWindow.h"

#ifdef ECHO2_BUILD
#include "/proj/xmos/common/Echo2.h"
#endif
#ifdef ECHO4_BUILD
#include "/proj/xmos/common/Echo4.h"
#endif

#include "../DescriptionAIO.h"
#include "../DescriptionAM1.h"
#include "../DescriptionAM2.h"
#include "../calibrationV2/CalibrationDataV2.h"

//******************************************************************************
// conversion routines
//******************************************************************************

int32 ehw::DbToLin(float gaindb)
{
	const float DbScale = 1.0f / 20.0f;
	const float GainScale = (float) (1 << 24);
	int32 lin;

	lin = roundFloatToInt(GainScale * pow(10.0f,gaindb * DbScale));

	return lin;
}

float ehw::LinToDb(uint32 gainlin)
{
	const float GainScaleInv = 1.0f / ((float) (1 << 24));
	float db;

	if (0 == gainlin)
		return -144.0f;

	db = 20.0f * log10( ((float) gainlin) * GainScaleInv );
	return db;
}

uint32 ehw::PanFloatToEfc(float pan)
{
	return roundFloatToInt(pan * PAN_HARD_RIGHT);
}

float ehw::EfcToPanFloat(uint32 pan)
{
	static const float scale = 1.0f / PAN_HARD_RIGHT;

	return pan * scale;
}

float ehw::PanFloatToDb(float pan)
{
	if (pan >= 0.5f)
		return 0.0f;

	if (0.0f == pan)
		return -128.0f;

	return 20.0f * log10(pan * 2.0f);
}

//******************************************************************************
// ctor/dtor
//******************************************************************************

ehw::ehw
(
	unsigned device_index,
	CriticalSection *lock
) :
	_ok(FALSE),
	_prev(NULL),
	_next(NULL),
	_lock(lock),
	deviceIndex(device_index),
	handle(0),
	localLock(false)
#ifdef WINDOWS_VOLUME_CONTROL
	,_SysVolume(NULL)
#endif
{
	DBG_PRINTF(("ehw::ehw %p",this));

	m_dwRefCount = 0;
	//memset(&_pstuff,0,sizeof(_pstuff));

	if (NULL == _lock)
	{
		_lock = new CriticalSection;
		localLock = true;
	}

	TUsbAudioStatus status;
	TUsbAudioHandle tempHandle;

	properties.usbProductId = 0;

	status = TUSBAUDIO_OpenDeviceByIndex(device_index, &tempHandle);
	DBG_PRINTF(("TUSBAUDIO_OpenDeviceByIndex handle:0x%x status:0x%x", tempHandle, status));
	if (TSTATUS_SUCCESS == status)
	{
		status = TUSBAUDIO_GetDeviceProperties(tempHandle, &properties);
		//if (TSTATUS_SUCCESS == status)
		//{
		//	switch (props.usbProductId)
		//	{
		//	case hwcaps::ECHO4:
		//		_uniquename = "Echo4";
		//		break;
		//	}
		//}

		DBG_PRINTF(("usbProductId:0x%04x  usbRevisionId:0x%04x  tempHandle:0x%x", properties.usbProductId, properties.usbRevisionId, tempHandle));

		uint8 moduleTypes = getModuleTypes(tempHandle);
		switch (properties.usbProductId)
		{
		case hwcaps::AIO_PRODUCT_ID:
			description = new DescriptionAIO(moduleTypes, (uint16)properties.usbRevisionId);
			break;

		case hwcaps::AIO_M1_PRODUCT_ID:
			description = new DescriptionAM1(moduleTypes, (uint16)properties.usbRevisionId);
			break;

		case hwcaps::AIO_M2_PRODUCT_ID:
			moduleTypes = (ACOUSTICIO_MIKEYBUS_MODULE << 4) | ACOUSTICIO_MIKEYBUS_MODULE;
			description = new DescriptionAM2(moduleTypes, (uint16)properties.usbProductId, (uint16)properties.usbRevisionId);			break;

		default:
			description = new Description(moduleTypes, (uint16)properties.usbRevisionId);
			break;
		}

		TUSBAUDIO_CloseDevice(tempHandle);
		DBG_PRINTF(("Closing 0x%x", tempHandle));
		tempHandle = 0;
	}

	//
	// Create the SysVolume object
	//
#ifdef WINDOWS_VOLUME_CONTROL
	_SysVolume = SysVolume::create(this);
#endif

	memset(meters,0,sizeof(meters));

	_caps.init(properties.usbProductId);
	_uniquename = _caps.BoxTypeName();

	
	//initializeSession();
	//loadSession();
} // constructor

ehw::~ehw()
{
	DBG_PRINTF(("ehw::~ehw %p",this));

	CloseDriver();

	//saveSession();

#ifdef WINDOWS_VOLUME_CONTROL
	delete _SysVolume;
#endif

	if (localLock)
		delete _lock;
} // destructor

//==============================================================================
// Query versions
//==============================================================================

uint32 ehw::GetDriverVersion()
{
	TUsbAudioStatus status;
	TUsbAudioDriverInfo info;
	uint32 version = 0;

	status = TUSBAUDIO_GetDriverInfo(&info);
	if (TSTATUS_SUCCESS == status)
	{
		version =	(info.driverVersionMajor << 24) |
					(info.driverVersionMinor << 16) |
					(info.driverVersionSub << 8);
	}

	return version;
}

uint32 ehw::getFirmwareVersion() const
{
	return properties.usbRevisionId;
}


String ehw::getFirmwareVersionString() const
{
	return String::toHexString((int32)properties.usbRevisionId);
}

//******************************************************************************
//
// Get vendor ID, box ID, and box revision
//
//******************************************************************************

uint32 ehw::GetVendorId()
{
	return properties.usbVendorId;
}

uint32 ehw::GetBoxType()
{
	return properties.usbProductId;
}

uint32 ehw::getFirmwareVersion()
{
	return properties.usbRevisionId;
}

uint64 ehw::GetSerialNumber()
{
	return 0;
}

//******************************************************************************
//
// Open and close the hardware driver
//
//******************************************************************************

//==============================================================================
// Open the driver
//==============================================================================

int ehw::OpenDriver()
{
	const ScopedLock locker(*_lock);
	TUsbAudioStatus status;

	if (0 == handle)
	{
		status = TUSBAUDIO_OpenDeviceByIndex( deviceIndex, &handle);
		DBG_PRINTF(("ehw::OpenDriver  status:0x%x  handle:0x%x",status,handle));
		if (TSTATUS_SUCCESS == status)
		{
			TUsbAudioDeviceRunMode mode;

			status = TUSBAUDIO_GetDeviceUsbMode(handle,&mode);
			DBG_PRINTF(("ehw::OpenDriver   status 0x%x  mode %d",status,mode));
			if (DeviceRunMode_APP != mode)
			{
				DBG("device not in run mode");
				CloseDriver();
				return 1;
			}

			//uint32 version;
			//if (0 == getFirmwareVersion(version))
			//{
			//	if (version < 0x42)
			//	{
			//		Logger::outputDebugString(String::formatted("Firmware version 0x%x not supported",version));
			//		CloseDriver();
			//		return 1;
			//	}
			//}
		}

		return TSTATUS_SUCCESS != status;
	}

	return 0;
} // OpenDriver

//==============================================================================
// Close the driver
//==============================================================================

void ehw::CloseDriver()
{
	const ScopedLock locker(*_lock);

	DBG_PRINTF(("ehw::CloseDriver handle:0x%x",handle));
	if (handle)
	{
		TUSBAUDIO_CloseDevice(handle);
		DBG_PRINTF(("\tclosed 0x%x",handle));
		handle = 0;
	}
}

//******************************************************************************
//
// Input bus controls
//
//******************************************************************************

#if 0

//==============================================================================
// get input nominal shift
//==============================================================================

int ehw::getinshift(int input,int &shift)
{
	return 0;
} // getinshift

//==============================================================================
// set input nominal shift
//==============================================================================

int ehw::setinshift(int input,int shift)
{
	return 0;
} // setinshift

//==============================================================================
// get input meter
//==============================================================================

int ehw::GetInputMeter(int input)
{
	return meters[input];
}

//==============================================================================
// set input gain
//==============================================================================

int ehw::setingain(int input,float gain)
{
	return 0;
}


//******************************************************************************
//
// Monitor controls
//
//******************************************************************************

//==============================================================================
// get monitor mute
//==============================================================================

int ehw::getmonmute(uint32 input,uint32 output,uint32 &mute)
{
	if (1 == input)
		input = 0;

	mute = session.s.monitorflags[input][output] & SESSION_MUTE_BIT;
	return 0;
} // getmonmute

//==============================================================================
// set monitor mute
//==============================================================================

int ehw::setmonmute(uint32 input,uint32 output,uint32 mute)
{
	if (1 == input)
		input = 0;

	if (mute)
		session.s.monitorflags[input][output] |= SESSION_MUTE_BIT;
	else
		session.s.monitorflags[input][output] &= ~SESSION_MUTE_BIT;
	return setInMixer(input,output);
} // setmonmute

//==============================================================================
// get monitor gain
//==============================================================================

int ehw::getmongain(uint32 input,uint32 output,float &gain)
{
	if (1 == input)
		input = 0;

	gain = LinToDb(session.monitorgains[input][output >> 1]);
	return 0;
} // getmongain

//==============================================================================
// set monitor gain
//==============================================================================

int ehw::setmongain(uint32 input,uint32 output,float gain)
{
	if (1 == input)
		input = 0;

	session.monitorgains[input][output >> 1] = DbToLin(gain);
	return setInMixer(input,output);
}

//==============================================================================
// get monitor solo
//==============================================================================

int ehw::getmonsolo(uint32 input,uint32 output,uint32 &solo)
{
	return 0;
} // getmonsolo

//==============================================================================
// set monitor solo
//==============================================================================

int ehw::setmonsolo(uint32 input,uint32 output,uint32 solo)
{
	return 0;
} // setmonsolo

//==============================================================================
// get monitor pan
//==============================================================================

int ehw::getmonpan(uint32 input,uint32 output,float &pan)
{
	if (1 == input)
		input = 0;
	pan = EfcToPanFloat( session.s.monitorpans[input][output >> 1] );
	return 0;
} // getmonpan

//==============================================================================
// set monitor pan
//==============================================================================

#if 0
int ehw::setmonpan(uint32 input, uint32 output, float pan)
{
	if (1 == input)
		input = 0;

	session.s.monitorpans[input][output >> 1] = PanFloatToEfc(pan);
	return setInMixer(input, output);
} // setmonpan

#endif // 0

////==============================================================================
//// get most recent polled stuff
////==============================================================================
//
int ehw::updatepolledstuff()
{
//	TUsbAudioStatus status;
//
//	status = TUSBAUDIO_AudioControlRequestGet(	handle,
//										GNODE_GUITAR_COMMUNICATION_XU,
//										CUR,
//										GNODE_AUDIO_SELECTOR,
//										GNODE_PEAK_METERS,
//										&meters,
//										sizeof(meters),
//										NULL,
//										GNODE_USB_TIMEOUT_MILLISECONDS);
//	return TSTATUS_SUCCESS != status;
	return 0;
}
#endif

//******************************************************************************
//
// Playback gain
//
//******************************************************************************

#if 0
//==============================================================================
// set playback gain
//==============================================================================

int ehw::setplaygain(uint32 virtout,uint32 output,float gain)
{
#ifdef WINDOWS_VOLUME_CONTROL
	session.playbackgains[output] = DbToLin(gain);
	return _SysVolume->SetPlayGain(virtout,output,gain);
#else
	session.playbackgains[output] = DbToLin(gain);

	return setOutMixer(output);
#endif
}

//==============================================================================
// set playback mute
//==============================================================================

int ehw::setplaymute(uint32 virtout,uint32 output,uint32 mute)
{
	session.s.playbacks[output].mute = mute;
#ifdef WINDOWS_VOLUME_CONTROL
	return _SysVolume->SetPlayMute(virtout,output,mute);
#else
	return setOutMixer(output);
#endif
}

//==============================================================================
// get playback gain
//==============================================================================

int ehw::getplaygain(uint32 virtout,uint32 output,float &gain)
{
	gain = LinToDb(session.playbackgains[output]);
	return 0;
}

//==============================================================================
// get playback mute
//==============================================================================

int ehw::getplaymute(uint32 virtout,uint32 output,uint32 &mute)
{
	mute = session.s.playbacks[output].mute;
	return 0;
}

//==============================================================================
// get playback solo
//==============================================================================

int ehw::getplaysolo(uint32 output,uint32 &solo)
{
	return 0;
}

//==============================================================================
// set playback solo
//==============================================================================

int ehw::setplaysolo(uint32 output,uint32 solo)
{
	return 0;
}

//******************************************************************************
//
// Master outputs
//
//******************************************************************************

//==============================================================================
// set master output gain
//==============================================================================

int ehw::setmastergain(uint32 output,float gain)
{
	//session.outputgains[output] = DbToLin(gain);
	int result = setInMixers(output);
	if (result)
		return result;

	return setOutMixer(output);
}

//==============================================================================
// set master output mute
//==============================================================================

int ehw::setmastermute(uint32 output,uint32 mute)
{
	int result = setInMixers(output);
	if (result)
		return result;

	return setOutMixer(output);
}

int ehw::getmastergain(uint32 output,float &gain)
{
	gain = LinToDb(session.outputgains[output]);
	return 0;
}

int ehw::getmastermute(uint32 output,uint32 &mute)
{
	mute = session.s.outputs[output].mute;
	return 0;
}

//==============================================================================
// get output nominal shift
//==============================================================================

int ehw::getoutshift(int output,int &shift)
{
	return 0;
}

//==============================================================================
// set output nominal shift
//==============================================================================

int ehw::setoutshift(int output,int shift)
{
	return 0;
}

//==============================================================================
// get master out meter
//==============================================================================

int ehw::GetMasterOutMeter(int output)
{
	return meters[output + _caps.numbusin()];
}

//******************************************************************************
//
// get/set clock and samplerate
//
//******************************************************************************

int ehw::getclock(int32 &samplerate,int32 &clock)
{
	samplerate = 48000;
	clock = hwcaps::internal_clock;
	return 0;
}

int ehw::setclock(int32 samplerate,int32 clock)
{
	return 0;
}

int ehw::getratelock(bool &locked)
{
	return 0;
}

int ehw::setratelock(bool locked)
{
	return 0;
}

//******************************************************************************
//
// Box flags
//
//******************************************************************************

//int ehw::changeboxflags(uint32 setmask,uint32 clearmask)
//{
//	return 0;
//}
//
//int ehw::getboxflags(uint32 &flags)
//{
//	return 0;
//}
//
//
//int ehw::changedriverflags(uint32 ormask,uint32 andmask)
//{
//	return 0;
//}

//******************************************************************************
//
// Make the box identify itself to the user by blinking an
// appropriate LED
//
//******************************************************************************

int ehw::identify()
{
	return 0;
}

//******************************************************************************
//
// MIDI activity
//
//******************************************************************************

uint32 ehw::GetMidiStatus()
{
	return 0;
}

bool ehw::MidiInActive(int input)
{
	return 0;
}

bool ehw::MidiOutActive(int output)
{
	return 0;
}

//******************************************************************************
//
// Clocking
//
//******************************************************************************

uint32 ehw::GetClockEnableMask()
{
	return 0;
}

uint32 ehw::ClockDetected(int clock)
{
	return 0;
}

//******************************************************************************
//
// Manage mixer listeners - Windows only
//
//******************************************************************************

void ehw::AddMixerListener(ChangeListener *cl)
{
#ifdef WINDOWS_VOLUME_CONTROL
	_SysVolume->addChangeListener(cl);
#endif
}

void ehw::RemoveMixerListener(ChangeListener *cl)
{
#ifdef WINDOWS_VOLUME_CONTROL
	_SysVolume->removeChangeListener(cl);
#endif
}

//******************************************************************************
//
// Core Audio sample rate
//
//******************************************************************************

int ehw::GetCoreAudioRate(uint32 &rate)
{
	return 0;
}

int ehw::SetCoreAudioRate(uint32 rate)
{
	return 0;
}

//******************************************************************************
//
// session
//
//******************************************************************************

void ehw::initializeSession()
{
	unsigned input, output;

	DBG("ehw::initializeSession");

	memset(&session,0,sizeof(session));

	for (output = 0; output < MAX_PHY_AUDIO_OUT; output ++)
	{
		session.outputgains[output] = 0x1000000;
		session.playbackgains[output] = 0x1000000;

		for (input = 0; input < MAX_PHY_AUDIO_IN; input ++)
		{
			unsigned char pan;
			int type = _caps.MixInType(input);

			session.monitorgains [input] [output] = 0x1000000;

			if (hwcaps::guitar_string == type)
			{
				int string = _caps.MixInGroupOffset(input);

				session.s.monitorflags[input] [output] = SESSION_MUTE_BIT;

				pan = (PAN_HARD_RIGHT * string)/5;
			}
			else
			{
				if ((input & ~1) != (output & ~1))
					session.s.monitorflags[input] [output] = SESSION_MUTE_BIT;

				if (input & 1)
					pan = PAN_HARD_RIGHT;
				else
					pan = 0;
			}

			session.s.monitorpans[input][output] = pan;
		}
	}

	session.s.monitorpans[0][0] = PAN_HARD_RIGHT/2;
}
//
//
int ehw::setOutMixer(int output)
{
//	float playback_db,master_db,db;
//	short db_16bit;
//	TUsbAudioStatus status;
//	int outputs = _caps.numbusout();
//
//	playback_db = LinToDb(session.playbackgains[output]);
//	master_db = LinToDb(session.outputgains[output]);
//	db = playback_db + master_db;
//	db = jlimit(-128.0f, 0.0f, db);
//	db_16bit = roundFloatToInt(db * 256.0f);
//	if (session.s.outputs[output].mute || session.s.playbacks[output].mute)
//	{
//		db_16bit = short(0x8000);
//	}
//	DBG_PRINTF(("%d (%d)   master_db:%f  playback_db:%f  db:%f  db_16bit:%d",output,output * outputs + output,master_db,playback_db,db,db_16bit));
//	status = TUSBAUDIO_AudioControlRequestSet(	handle,
//         0x3c,	// mixer XU unit ID
//         CUR,
//         1,		// CS = 1
//         output * outputs + output,
//         ( void * )&db_16bit,
//         sizeof( db_16bit ),
//         NULL,
//         GNODE_USB_TIMEOUT_MILLISECONDS );
//
//	return TSTATUS_SUCCESS != status;
	return 0;
}

int ehw::setInMixer(int input,int output)
{
//	float input_db,master_db,pan_db,db,pan;
//	short db_16bit;
//	TUsbAudioStatus status;
//	int outputs = _caps.numbusout();
//	int mute,node;
//
//	output &= ~1;
//	if (1 == input)
//	{
//		input = 0;
//	}
//
//	input_db = LinToDb(session.monitorgains[input][output >> 1]);
//	pan = EfcToPanFloat( session.s.monitorpans[input][output >> 1] );
//
//	//
//	// left
//	//
//	master_db = LinToDb(session.outputgains[output]);
//	pan_db = PanFloatToDb(1.0f - pan);
//	db = input_db + master_db + pan_db;
//	db = jlimit(-128.0f, 0.0f, db);
//	db_16bit = roundFloatToInt(db * 256.0f);
//	mute = session.s.outputs[output].mute;
//	mute |= session.s.monitorflags[input][output >> 1] & SESSION_MUTE_BIT;
//	if (mute)
//	{
//		db_16bit = short(0x8000);
//	}
//
//	node = outputs * outputs + input*outputs + output;
//	DBG_PRINTF(("%d->%d (%d)   master_db:%f  pan_db:%f  input_db:%f  db:%f  db_16bit:%d",input,output,node,master_db,pan_db,input_db,db,db_16bit));
//	status = TUSBAUDIO_AudioControlRequestSet(	handle,
//         0x3c,	// mixer XU unit ID
//         CUR,
//         1,		// CS = 1
//         node,
//         ( void * )&db_16bit,
//         sizeof( db_16bit ),
//         NULL,
//         GNODE_USB_TIMEOUT_MILLISECONDS );
//	if (TSTATUS_SUCCESS != status)
//		return 1;
//
//	//
//	// right
//	//
//	output++;
//	master_db = LinToDb(session.outputgains[output]);
//	pan_db = PanFloatToDb(pan);
//	db = input_db + master_db + pan_db;
//	db = jlimit(-128.0f, 0.0f, db);
//	db_16bit = roundFloatToInt(db * 256.0f);
//	mute = session.s.outputs[output].mute;
//	mute |= session.s.monitorflags[input][output >> 1] & SESSION_MUTE_BIT;
//	if (mute)
//	{
//		db_16bit = short(0x8000);
//	}
//
//	node = outputs * outputs + input*outputs + output;
//	DBG_PRINTF(("%d->%d (%d)   master_db:%f  pan_db:%f  input_db:%f  db:%f  db_16bit:%d\n",input,output,node,master_db,pan_db,input_db,db,db_16bit));
//	status = TUSBAUDIO_AudioControlRequestSet(	handle,
//         0x3c,	// mixer XU unit ID
//         CUR,
//         1,		// CS = 1
//         node,
//         ( void * )&db_16bit,
//         sizeof( db_16bit ),
//         NULL,
//         GNODE_USB_TIMEOUT_MILLISECONDS );
//
//	return TSTATUS_SUCCESS != status;
	return 0;
}
//
//
int ehw::setInMixers(int output)
{
//	int result;
//	int input,inputs = _caps.numbusin();
//
//	for (input = 0; input < inputs; input++)
//	{
//		result = setInMixer(input,output);
//		if (result)
//			break;
//	}
//
//	return result;
	return 0;
}

String printInputKey(int input)
{
	return "Input"+String(input);
}

String printPlaybackKey(int output)
{
	return "Playback"+String(output);
}

String printOutputKey(int output)
{
	return "Output"+String(output);
}

const char *GainTag = "Gain";
const char *MuteTag = "Mute";
const char *PanTag = "Pan";

void ehw::loadSession()
{
	//
	// Create or open the file
	//
	DBG("ehw::loadSession - prop file");
	String filename( File::createLegalFileName( GetUniqueName() ) );
	String dirname( "Echo USB mixer settings" );

    PropertiesFile::Options options;
    options.applicationName     = filename;
    options.folderName          = dirname;
    options.filenameSuffix      = "MixerSettings";
    options.osxLibrarySubFolder = "Application Support";

	_file = new PropertiesFile (options);

	//
	// Read the settings from the file
	//
	int input,output;
	int gain,pan;

	DBG("ehw::loadSession - read settings");
	for (output = 0; output < _caps.numbusout(); output ++)
	{
		String key(printOutputKey(output));
		ScopedPointer<XmlElement> element = _file->getXmlValue(key);

		if (element)
		{
			gain = element->getIntAttribute(	GainTag,
												session.outputgains[output]);
			session.outputgains[output] = jlimit( 0, 0x1000000, gain);

			session.s.outputs[output].mute = element->getIntAttribute(	MuteTag,
																		session.s.outputs[output].mute);
		}
	}

	for (output = 0; output < _caps.numbusout(); output ++)
	{
		String key(printPlaybackKey(output));
		ScopedPointer<XmlElement> element = _file->getXmlValue(key);

		if (element)
		{
			gain = element->getIntAttribute(	GainTag,
												session.playbackgains[output]);
			session.playbackgains[output] = jlimit( 0, 0x1000000, gain);
			session.s.playbacks[output].mute = element->getIntAttribute(MuteTag,
																		session.s.playbacks[output].mute);
		}
	}

	for (input = 0; input < _caps.numbusin(); input++)
	{
		String input_key(printInputKey(input));
		ScopedPointer<XmlElement> input_element = _file->getXmlValue(input_key);

		if (NULL == input_element)
			continue;

		for (output = 0; output < _caps.numbusout()/2; output ++)
		{
			int mute;
			String output_key(printOutputKey(output * 2));
			XmlElement *output_element;

			output_element = input_element->getChildByName(output_key);
			if (NULL == output_element)
				continue;

			gain = output_element->getIntAttribute(	GainTag,
													session.monitorgains[input][output]);
			session.monitorgains[input][output] = jlimit( 0, 0x1000000, gain);
			pan = output_element->getIntAttribute(	PanTag,
													session.s.monitorpans[input][output]);
			session.s.monitorpans[input][output] = jlimit(0, PAN_HARD_RIGHT, pan);
			mute = output_element->getIntAttribute(	MuteTag,
													session.s.monitorflags[input][output] & SESSION_MUTE_BIT);
			if (mute)
				session.s.monitorflags[input][output] |= SESSION_MUTE_BIT;
			else
				session.s.monitorflags[input][output] &= ~SESSION_MUTE_BIT;
		}
	}

	//
	// Send settings to hardware
	//
	DBG("ehw::loadSession - set hardware");
	if (0 == OpenDriver())
	{
		DBG("setting up mixer");

		for (output = 0; output < _caps.numbusout(); output += 2)
		{
			setInMixers(output);
			setOutMixer(output);
			setOutMixer(output + 1);
		}

		CloseDriver();
	}
}

void ehw::saveSession()
{
	DBG("ehw::saveSession");

	//
	// Write the mixer settings to the file
	//
	int input,output;

	for (output = 0; output < _caps.numbusout(); output ++)
	{
		String key(printOutputKey(output));
		XmlElement element(key);

		element.setAttribute(GainTag,int(session.outputgains[output]));
		element.setAttribute(MuteTag,session.s.outputs[output].mute);

		_file->setValue(key,&element);
	}

	for (output = 0; output < _caps.numbusout(); output ++)
	{
		String key(printPlaybackKey(output));
		XmlElement element(key);

		element.setAttribute(GainTag,int(session.playbackgains[output]));
		element.setAttribute(MuteTag,session.s.playbacks[output].mute);

		_file->setValue(key,&element);
	}

	for (input = 0; input < _caps.numbusin(); input++)
	{
		String input_key(printInputKey(input));
		XmlElement input_element(input_key);

		for (output = 0; output < _caps.numbusout()/2; output ++)
		{
			String output_key(printOutputKey(output * 2));
			XmlElement *output_element = new XmlElement(output_key);

			output_element->setAttribute(GainTag,int(session.monitorgains[input][output]));
			output_element->setAttribute(PanTag,int(session.s.monitorpans[input][output]));
			output_element->setAttribute(MuteTag,session.s.monitorflags[input][output] & SESSION_MUTE_BIT);

			input_element.addChildElement(output_element);
		}

		_file->setValue(input_key,&input_element);}
}
#endif

//******************************************************************************
//
// buffer size
//
//******************************************************************************
#if TUSBAUDIO_API_VERSION_MJ >=4
int ehw::getbuffsize(uint32 &aframes)
#else
int ehw::getbuffsize(uint32 &usec)
#endif
{
	TUsbAudioStatus status;

#if TUSBAUDIO_API_VERSION_MJ >=4
	status = TUSBAUDIO_GetASIOBufferSize(&aframes);
#else
	status = TUSBAUDIO_GetAsioBufferSize(&usec);
#endif
	return status != TSTATUS_SUCCESS;
}

#if TUSBAUDIO_API_VERSION_MJ >=4
int ehw::setbuffsize(uint32 aframes)
#else
int ehw::setbuffsize(uint32 usec)
#endif
{
	TUsbAudioStatus status;

#if TUSBAUDIO_API_VERSION_MJ >=4
	DBG_PRINTF(("ehw::setbuffsize %d",aframes));

	uint32 minimum;

	status = TUSBAUDIO_GetMinimumASIOBufferSize(48000,&minimum);
	if (TSTATUS_SUCCESS == status)
	{
		if (aframes < minimum)
		{
			DBG_PRINTF(("ehw::setbuffsize limit to %d",minimum));
			aframes = minimum;
		}

		status = TUSBAUDIO_SetASIOBufferSize(aframes);
		DBG_PRINTF(("\tehw::setbuffsize status 0x%x",status));
	}
#else
	status = TUSBAUDIO_SetAsioBufferSize(usec);
	DBG_PRINTF(("\tehw::setbuffsize  usec:%d  status 0x%x",usec,status));
#endif
	return status != TSTATUS_SUCCESS;
}

#if TUSBAUDIO_API_VERSION_MJ >=4

int ehw::getusbmode(int &mode)
{
	TUsbAudioStatus status;
	TUsbAudioUsbStreamingMode temp;

	status = TUSBAUDIO_GetUsbStreamingMode(&temp);
	mode = int(temp);
	return status != TSTATUS_SUCCESS;
}

int ehw::setusbmode(int mode)
{
	TUsbAudioStatus status;
	TUsbAudioUsbStreamingMode temp = (TUsbAudioUsbStreamingMode) mode;

	status = TUSBAUDIO_SetUsbStreamingMode(temp);
	DBG_PRINTF(("ehw::setusbmode %d  status 0x%x",mode,status));
	switch (status)
	{
	case TSTATUS_SUCCESS:
		return 0;

	case TSTATUS_ASIO_IN_USE:
		return ASIO_BUSY;

	case TSTATUS_SOUND_DEVICE_IN_USE:
		return WINDOWS_AUDIO_BUSY;
	}

	return USB_MODE_ERROR;
}

#else

int ehw::getUSBBufferSizes(Array<int> &sizes,unsigned &current)
{
	TUsbAudioStatus status;
	unsigned temp[256],count;

	status = TUSBAUDIO_GetStreamingBufferSizeSet(	sizeof(temp)/sizeof(int),
													temp,
													&count,
													&current);
	sizes.clear();
	if (TSTATUS_SUCCESS == status)
	{
		for (unsigned i = 0; i < count; i++)
		{
			sizes.add(temp[i]);
		}
	}

	return TSTATUS_SUCCESS != status;
}

int ehw::getUSBBufferSize(int &usec)
{
	TUsbAudioStatus status;
	unsigned temp;

	status = TUSBAUDIO_GetStreamingBufferSize(&temp);
	usec = 0;
	if (TSTATUS_SUCCESS == status)
	{
		usec = temp;
	}

	return TSTATUS_SUCCESS != status;
}

int ehw::setUSBBufferSize(int usec)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_SetStreamingBufferSize(usec);
	switch (status)
	{
	case TSTATUS_SUCCESS:
		return 0;

	case TSTATUS_ASIO_IN_USE:
		return ASIO_BUSY;

	case TSTATUS_SOUND_DEVICE_IN_USE:
		return WINDOWS_AUDIO_BUSY;
	}

	return USB_MODE_ERROR;
}

int ehw::getAsioBufferSizes(Array<int> &sizes,unsigned &current)
{
	TUsbAudioStatus status;
	unsigned temp[256],count;

	status = TUSBAUDIO_GetAsioBufferSizeSet(	sizeof(temp)/sizeof(int),
												temp,
												&count,
												&current);
	sizes.clear();
	if (TSTATUS_SUCCESS == status)
	{
		for (unsigned i = 0; i < count; i++)
		{
			sizes.add(temp[i]);
		}
	}

	return TSTATUS_SUCCESS != status;
}
#endif

String ehw::GetIdString()
{
	TUsbAudioStatus status;

	if (handle)
	{
		TUsbAudioDeviceProperties props;

		status = TUSBAUDIO_GetDeviceProperties(handle,&props);
		if (TSTATUS_SUCCESS == status)
			return String(props.serialNumberString);
	}
	return String::empty;
}

#if 0
int ehw::SendTestBuffer (uint8 const *buffer,size_t buffer_size)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestSet(	handle,
         ECHO4_EXTENSION_UNIT,	// unit ID
         CUR,
         ECHO4_TEST_CONTROL,
         ECHO4_TEST_CONTROL__INTERFACE_TEST,
         ( void * )buffer,
         buffer_size,
         NULL,
         COMMAND_TIMEOUT_MSEC );

	return TSTATUS_SUCCESS != status;
}

int ehw::ReceiveTestBuffer (uint8 *buffer,size_t buffer_size,size_t &bytes_received)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestGet(	handle,
										ECHO4_EXTENSION_UNIT,
										CUR,
										ECHO4_TEST_CONTROL,
										ECHO4_TEST_CONTROL__INTERFACE_TEST,
										buffer,
										buffer_size,
										&bytes_received,
										COMMAND_TIMEOUT_MSEC);
	return TSTATUS_SUCCESS != status;
}
#endif

#if ACOUSTICIO_BUILD

#include "../AcousticIO.h"

uint8 ehw::getModuleTypes(TUsbAudioHandle tempHandle)
{
	TUsbAudioStatus status;

	uint8 moduleTypes = 0;
	if (getFirmwareVersion() >= ACOUSTICIO_MODULE_TYPE_CONTROL_MIN_FIRMWARE_VERSION)
	{
		//
		// Use ACOUSTICIO_MODULE_TYPE_CONTROL to detect AIO-S or analog module
		//
		status = TUSBAUDIO_AudioControlRequestGet(tempHandle,
			ACOUSTICIO_EXTENSION_UNIT, CUR,
			ACOUSTICIO_MODULE_TYPE_CONTROL, 0, &moduleTypes, 1,
			NULL, COMMAND_TIMEOUT_MSEC);
		if (TSTATUS_SUCCESS != status)
			moduleTypes = 0;
	}
	else
	{
		//
		// Use ACOUSTICIO_MODULE_STATUS_CONTROL for older firmware
		//
		uint8 moduleStatus = 0;
		status = TUSBAUDIO_AudioControlRequestGet(tempHandle,
			ACOUSTICIO_EXTENSION_UNIT, CUR,
			ACOUSTICIO_MODULE_STATUS_CONTROL, 0, &moduleStatus, 1,
			NULL, COMMAND_TIMEOUT_MSEC);

		if (TSTATUS_SUCCESS != status)
		{
			moduleTypes = 0;
		}
		else
		{
			//
			// Check the low 2 bits of moduleStatus - the module is present
			// if the bit is low
			//
			if (moduleStatus & 2)
			{
				moduleTypes = ACOUSTICIO_MODULE_NOT_PRESENT;
			}
			else
			{
				moduleTypes = ACOUSTICIO_ANALOG_MODULE;
			}

			if (moduleStatus & 1)
			{
				moduleTypes |= ACOUSTICIO_MODULE_NOT_PRESENT << 4;
			}
			else
			{
				moduleTypes |= ACOUSTICIO_ANALOG_MODULE << 4;
			}
		}
	}
	return moduleTypes;
}

Result ehw::setMicGain(XmlElement const *element)
{
	uint8 channel;
	uint8 gain;
	int attribute;

	if (false == element->hasAttribute("input"))
	{
		Result error(Result::fail("AIO_set_mic_gain missing 'input' setting"));
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}

	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		Result error(Result::fail("AIO_set_mic_gain - input " + String(attribute) + " out of range"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}
	channel = (uint8)attribute;


	if (false == element->hasAttribute("gain"))
	{
		Result error(Result::fail("AIO_set_mic_gain missing 'gain' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}

	attribute = element->getIntAttribute("gain", -1);
	switch (attribute)
	{
	case 1:
	case 10:
	case 100:
		break;
	default:
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_set_mic_gain - gain " + String(attribute) + " is invalid", false);
		break;
	}
	gain = (uint8)attribute;

	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_MIC_GAIN_CONTROL,
		channel,
		(void *)&gain,
		1,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
	{
		uint8 temp;

		status = TUSBAUDIO_AudioControlRequestGet(handle,
			ACOUSTICIO_EXTENSION_UNIT,	// unit ID
			CUR,
			ACOUSTICIO_MIC_GAIN_CONTROL,
			channel,
			(void *)&temp,
			1,
			NULL,
			COMMAND_TIMEOUT_MSEC);
		if (TSTATUS_SUCCESS == status)
		{
			if (temp == gain)
			{
				return Result::ok();
			}
			else
			{
				String error("Failed to verify mic gain for input " + String((int)channel));
				error += " - expected " + String::toHexString(gain) + ", read " + String::toHexString(temp);
				return Result::fail(error);
			}
		}
		else
		{
			String error("Failed to get mic gain for input " + String((int)channel));
			error += " - error " + String::toHexString((int)status);
			return Result::fail(error);
		}
	}
	String error("Failed to set mic gain for input " + String((int)channel));
	error += " - error " + String::toHexString((int)status);
	return Result::fail(error);
}

Result ehw::setMicGain(uint8 channel, uint8 gain)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_MIC_GAIN_CONTROL,
		channel,
		(void *)&gain,
		1,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to set mic gain");
	error += " - error " + String::toHexString((int)status);
	return Result::fail(error);
}

Result ehw::setAmpGain(XmlElement const *element)
{
	uint8 channel;
	uint8 gain;
	int attribute;

	if (false == element->hasAttribute("output"))
	{
		Result error(Result::fail("AIO_set_amp_gain missing 'output' setting"));
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}

	attribute = element->getIntAttribute("output", -1);
	if (attribute < 0 || attribute > 3)
	{
		Result error(Result::fail("AIO_set_amp_gain - output " + String(attribute) + " out of range"));
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}
	channel = (uint8)attribute;


	if (false == element->hasAttribute("gain"))
	{
		Result error(Result::fail("AIO_set_amp_gain missing 'gain' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_set_amp_gain missing 'gain' setting", false);
		return error;
	}

	attribute = element->getIntAttribute("gain", -1);
	if (1 == attribute)
		attribute = 26;
	if (10 == attribute)
		attribute = 255;
	switch (attribute)
	{
	case 26:
	case 255:
		break;
	default:
		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_set_amp_gain - gain " + String(attribute) + " is invalid", false);
		break;
	}
	gain = (uint8)attribute;

	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_AMP_GAIN_CONTROL,
		channel,
		(void *)&gain,
		1,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
	{
		uint8 temp;
		status = TUSBAUDIO_AudioControlRequestGet(handle,
			ACOUSTICIO_EXTENSION_UNIT,	// unit ID
			CUR,
			ACOUSTICIO_AMP_GAIN_CONTROL,
			channel,
			(void *)&temp,
			1,
			NULL,
			COMMAND_TIMEOUT_MSEC);
		if (TSTATUS_SUCCESS == status)
		{
			if (temp == gain)
			{
				return Result::ok();
			}
			else
			{
				String error("Failed to verify amp gain for input " + String((int)channel));
				error += " - expected " + String::toHexString(gain) + ", read " + String::toHexString(temp);
				return Result::fail(error);
			}
		}
		else
		{
			String error("Failed to get amp gain for input " + String((int)channel));
			error += " - error " + String::toHexString((int)status);
			return Result::fail(error);
		}
	}

	String error("Failed to set amp gain for input " + String((int)channel));
	error += " - error " + String::toHexString((int)status);
	return Result::fail(error);
}

Result ehw::setAmpGain(uint8 channel, uint8 gain)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_AMP_GAIN_CONTROL,
		channel,
		(void *)&gain,
		1,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to set amp gain");
	error += " - error " + String::toHexString((int)status);
	return Result::fail(error);
}

Result ehw::setConstantCurrent(XmlElement const *element)
{
	uint8 channel;
	uint8 enabled;
	int attribute;

	if (false == element->hasAttribute("input"))
	{
		Result error(Result::fail("AIO_set_constant_current missing 'input' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_set_constant_current missing 'input' setting", false);
		return error;
	}

	attribute = element->getIntAttribute("input", -1);
	if (attribute < 0 || attribute > 7)
	{
		Result error(Result::fail("AIO_set_constant_current - input " + String(attribute) + " out of range"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_set_constant_current - input " + String(attribute) + " out of range", false);
		return error;
	}
	channel = (uint8)attribute;


	if (false == element->hasAttribute("enabled"))
	{
		Result error(Result::fail("AIO_set_constant_current missing 'enabled' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			"AIO_set_constant_current missing 'enabled' setting", false);
		return error;
	}

	enabled = element->getIntAttribute("enabled", 0) != 0;
	return setConstantCurrent(channel, enabled);
}

Result ehw::setConstantCurrent(uint8 const input, uint8 const enabled)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CONSTANT_CURRENT_CONTROL,
		input,
		(void *)&enabled,
		1,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
	{
		uint8 temp;
		status = TUSBAUDIO_AudioControlRequestGet(handle,
			ACOUSTICIO_EXTENSION_UNIT,	// unit ID
			CUR,
			ACOUSTICIO_CONSTANT_CURRENT_CONTROL,
			input,
			(void *)&temp,
			1,
			NULL,
			COMMAND_TIMEOUT_MSEC);
		if (TSTATUS_SUCCESS == status)
		{
			if (temp == enabled)
			{
				return Result::ok();
			}
			else
			{
				String error("Failed to verify CC for input " + String((int)input));
				error += " - expected " + String::toHexString(enabled) + ", read " + String::toHexString(temp);
				return Result::fail(error);
			}
		}
		else
		{
			String error("Failed to get CC for input " + String((int)input));
			error += " - error " + String::toHexString((int)status);
			return Result::fail(error);
		}
	}

	String error("Failed to set CC for input " + String((int)input));
	error += " - error " + String::toHexString((int)status);
	return Result::fail(error);
}

Result ehw::setClockSource(XmlElement const *element)
{
	String tmp;
	if (false == element->hasAttribute("source"))
	{
		Result error(Result::fail("AIO_set_clock_source missing 'source' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}

	tmp = element->getStringAttribute("source");
	if (tmp == "internal")
		return setClockSource(ACOUSTICIO_INTERNAL_CLOCK);
	else if (tmp == "USB")
		return setClockSource(ACOUSTICIO_USB_CLOCK);
	else
	{
		Result error(Result::fail("AIO_set_clock_source missing 'USB' or 'internal' as source"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}
}

Result ehw::setClockSource(uint8 source)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CLOCK_SOURCE_CONTROL,
		0,
		(void *)&source,
		sizeof(source),
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to set clock source");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::setUSBClockRate(XmlElement const *element)
{
	if (false == element->hasAttribute("rate"))
	{
		Result error(Result::fail("AIO_set_USB_clock_rate missing 'source' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}

	unsigned int rate = element->getIntAttribute("rate", 0);
	return setUSBClockRate(rate);
}

Result ehw::setUSBClockRate(unsigned int rate)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_USB_CLOCK_RATE_CONTROL,
		0,
		(void *)&rate,
		sizeof(rate),
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to set USB clock rate");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::readTEDSData(uint8 const input, uint8* data, size_t dataBufferBytes)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestGet(handle,
		ACOUSTICIO_EXTENSION_UNIT,
		CUR,
		ACOUSTICIO_TEDS_DATA_CONTROL,
		input,
		data,
		dataBufferBytes,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to read TEDS for input " + String((int)input));
	error += " - error " + String::toHexString((int)status);
	return Result::fail(error);
}

Result ehw::setCalibrationReferenceVoltage(XmlElement const *element)
{
	if (false == element->hasAttribute("enabled"))
	{
		Result error(Result::fail("AIOS_set_reference_voltage missing 'enabled' setting"));

		AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
			error.getErrorMessage(), false);
		return error;
	}

	bool enabled = element->getIntAttribute("enabled", 0) != 0;
	int const module = 0; // assume AIO center module for now
	return setCalibrationReferenceVoltage(module, enabled);
}

Result ehw::setCalibrationReferenceVoltage(int const module, bool const enabled)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CALIBRATION_VOLTAGE_CONTROL,
		(uint8)module,
		(void *)&enabled,
		1,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to set reference voltage");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::readFlashBlock(uint8 const block, uint8 * const buffer, size_t const bufferBytes)
{
	TUsbAudioStatus status;
	uint32 bytesTransferred = 0;
	status = TUSBAUDIO_AudioControlRequestGet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_FLASH_BLOCK_CONTROL,
		block,
		(void *)buffer,
		ACOUSTICIO_FLASH_BLOCK_BYTES,
		&bytesTransferred,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to read flash for block " + String((int)block));
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::writeFlashBlock(uint8 const block, uint8 const * const buffer, size_t const bufferBytes)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_FLASH_BLOCK_CONTROL,
		block,
		(void *)buffer,
		ACOUSTICIO_FLASH_BLOCK_BYTES,
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to write flash for block " + String((int)block));
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::clearRAMCalibrationData()
{
	CalibrationDataV2 data;

	return setCalibrationData(&data.data);
}

Result ehw::setCalibrationData(AcousticIOCalibrationData const * const data)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CALIBRATION_DATA_CONTROL,
		0,
		(void *)data,
		sizeof(AcousticIOCalibrationData),
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to set RAM calibration data ");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::getCalibrationData(AcousticIOCalibrationData * const data)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestGet(handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CALIBRATION_DATA_CONTROL,
		0,
		(void *)data,
		sizeof(AcousticIOCalibrationData),
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to read RAM calibration data");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

static uint8 getUnitForModule(uint8 const module)
{
	switch (module & 1)
	{
	case 0:
		return MIKEY_EXTENSION_UNIT0;

	case 1:
		return MIKEY_EXTENSION_UNIT1;
	}

	return 0;
}

Result ehw::readMikey(uint8 module, uint8 page, uint8 address, uint8 &value)
{
	TUsbAudioStatus status;
	uint8 unit = getUnitForModule(module);

	status = TUSBAUDIO_AudioControlRequestGet(handle,
		unit,	// unit ID
		CUR,
		page,
		address,
		(void *)&value,
		sizeof(value),
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to read MikeyBus");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

Result ehw::writeMikey(uint8 module, uint8 page, uint8 address, uint8 value)
{
	TUsbAudioStatus status;
	uint8 unit = getUnitForModule(module);

	status = TUSBAUDIO_AudioControlRequestSet(handle,
		unit,	// unit ID
		CUR,
		page,
		address,
		(void *)&value,
		sizeof(value),
		NULL,
		COMMAND_TIMEOUT_MSEC);
	if (TSTATUS_SUCCESS == status)
		return Result::ok();

	String error("Failed to write MikeyBus");
	error += " - error " + String::toHexString((int32)status);
	return Result::fail(error);
}

#endif
#endif
