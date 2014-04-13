//
//  AIODevice.cpp
//  AppleBox
//
//  Created by Matthew Gonzalez on 11/6/13.
//
//

#include "base.h"

#if JUCE_WIN32 && ACOUSTICIO_BUILD
#include "AIODevice.h"
#include "AcousticIO.h"

const float AIODevice::peakScale = 1.0f / 2147483648.0f;

AIODevice::AIODevice(TUsbAudioHandle handle_) :
handle(handle_)
{
}

AIODevice::~AIODevice()
{
	TUSBAUDIO_CloseDevice(handle);
}

Result AIODevice::getTEDSData(uint8 channel, uint8 data[TEDS_DATA_BYTES])
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestGet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_TEDS_DATA_CONTROL,
		channel,
		( void * )data,
		TEDS_DATA_BYTES,
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

uint8 AIODevice::getModuleStatus()
{
	uint8 moduleStatus = 0;
	TUSBAUDIO_AudioControlRequestGet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_MODULE_STATUS_CONTROL,
		0,
		( void * )&moduleStatus,
		1,
		NULL,
		TIMEOUT_MSEC );

	return moduleStatus;
}

bool AIODevice::isInputPresent(int channelNum)
{
	uint8 status = getModuleStatus();
	int module = channelNum / INPUTS_PER_MODULE;
	bool present = false;

	switch (module)
	{
	case 0:
		present = 0 == (status &AIODevice::MODULE1_ABSENT);
		break;

	case 1:
		present = 0 == (status &AIODevice::MODULE2_ABSENT);
		break;
	}
	return present;
}

bool AIODevice::isOutputPresent(int channelNum)
{
	uint8 status = getModuleStatus();
	int module = channelNum / OUTPUTS_PER_MODULE;
	bool present = false;

	switch (module)
	{
	case 0:
		present = 0 == (status &AIODevice:: MODULE1_ABSENT);
		break;

	case 1:
		present = 0 == (status &AIODevice:: MODULE2_ABSENT);
		break;
	}
	return present;
}

Result AIODevice::setMicGain( uint8 channel, uint8 gain )
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(	handle,
	 		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
	 		CUR,
	 		ACOUSTICIO_MIC_GAIN_CONTROL,
	 		channel,
	 		( void * )&gain,
	 		1,
	 		NULL,
	 		TIMEOUT_MSEC );

	return createResult(status);
}

Result AIODevice::getMicGain( uint8 channel, uint8 &gain )
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestGet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_MIC_GAIN_CONTROL,
		channel,
		( void * )&gain,
		1,
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

Result AIODevice::setAmpGain( uint8 channel, uint8 gain )
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_AMP_GAIN_CONTROL,
		channel,
		( void * )&gain,
		1,
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

Result AIODevice::getAmpGain( uint8 channel, uint8 &gain )
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestGet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_AMP_GAIN_CONTROL,
		channel,
		( void * )&gain,
		1,
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

Result AIODevice::setConstantCurrent( uint8 channel, bool enabled)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestSet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CONSTANT_CURRENT_CONTROL,
		channel,
		( void * )&enabled,
		1,
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

Result AIODevice::getConstantCurrent( uint8 channel, bool &enabled)
{
	TUsbAudioStatus status;
	status = TUSBAUDIO_AudioControlRequestGet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_CONSTANT_CURRENT_CONTROL,
		channel,
		( void * )&enabled,
		1,
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

Result AIODevice::getPeakMeters(Meters & meters)
{
	TUsbAudioStatus status;

	status = TUSBAUDIO_AudioControlRequestGet(	handle,
		ACOUSTICIO_EXTENSION_UNIT,	// unit ID
		CUR,
		ACOUSTICIO_PEAK_METERS_CONTROL,
		0,
		&meters,
		sizeof(meters),
		NULL,
		TIMEOUT_MSEC );

	return createResult(status);
}

const static String error("Error ");
Result AIODevice::createResult( TUsbAudioStatus status )
{
	if (TSTATUS_SUCCESS == status)
		return Result::ok();
	return Result:: fail(error + String::toHexString((int)status));
}

#endif
