//
//  AIODevice.h
//  AppleBox
//
//  Created by Matthew Gonzalez on 11/6/13.
//
//

#ifndef __AppleBox__USBDevice__
#define __AppleBox__USBDevice__

#if JUCE_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>
#include <IOKit/IOCFPlugIn.h>

#endif

#if JUCE_WIN32
#include "tusbaudioapi.h"
#endif
#include "AcousticIO.h"

class AIODevice
{
public:
#if JUCE_MAC
	AIODevice(IOUSBDeviceInterface** deviceInterface_);
#endif

#if JUCE_WIN32
	AIODevice(TUsbAudioHandle handle_);
#endif

	~AIODevice();

	enum
	{
		MODULE1_ABSENT = 2,
		MODULE2_ABSENT = 1,
		MODULE1_AMP_ERROR = 8,
		MODULE2_AMP_ERROR = 4
	};

	enum
	{
		INPUTS_PER_MODULE = 4,
		OUTPUTS_PER_MODULE = 2,
		TEDS_DATA_BYTES = ACOUSTICIO_TEDS_DATA_BYTES
	};

	typedef struct  
	{
		int32 outputPeaksLinear[OUTPUTS_PER_MODULE  * 2];
		int32 inputPeaksLinear[INPUTS_PER_MODULE * 2];
	} Meters;

	uint8 getModuleStatus();

	Result setMicGain(uint8 channel, uint8 gain);
	Result getMicGain(uint8 channel, uint8 &gain);

	Result setAmpGain(uint8 channel, uint8 gain);
	Result getAmpGain(uint8 channel, uint8 &gain);

	Result setConstantCurrent(uint8 channel, bool enabled);
	Result getConstantCurrent(uint8 channel, bool &enabled);

	Result getTEDSData(uint8 channel, uint8 data[TEDS_DATA_BYTES]);

	Result getPeakMeters(Meters &meters);

	bool isInputPresent(int channelNum);
	bool isOutputPresent(int channelNum);

	static const float peakScale;

private:
#if JUCE_MAC
    IOUSBDeviceInterface** deviceInterface;
    
    Result setRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length);
    Result getRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length);
    Result createResult(IOReturn const status);
#endif
#if JUCE_WIN32
	TUsbAudioHandle handle;

	Result createResult(TUsbAudioStatus status);
#endif

	enum
	{
		CUR = 1,
		USB_REQUEST_FROM_DEV = 0xa1,
		USB_REQUEST_TO_DEV = 0x21,
		TIMEOUT_MSEC = 1000
	};
};


#endif /* defined(__AppleBox__USBDevice__) */
