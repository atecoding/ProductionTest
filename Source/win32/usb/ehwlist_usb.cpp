/********************************************************************************
 *
 * ehwlist.cpp -  USB hardware
 *
 * A class that builds a list of Echo hardware objects.
 *
 * Copyright (c) 2011 Echo Digital Audio Corporation.  All rights reserved
 *
 ********************************************************************************/

#include <new>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include "../../../base.h"
#include "ehw.h"
#include "ehwlist.h"

// G-Node interface GUID
static GUID InterfaceGuid = { 0x6E5764B2, 0x64de, 0x4616, { 0xa3, 0xfb, 0x41, 0xc0, 0x4c, 0x8e, 0x0f, 0x3c } };

#pragma comment( lib, "tusbaudioapi" )

//******************************************************************************
// ehwlist methods
//******************************************************************************

ehwlist::ehwlist(int /*num_vendor_ids*/,uint32* /*vendor_ids*/,CriticalSection* lock,FileLogger* logger) :
	_hwlist(NULL),
	_numdevices(0),
	_devchange(NULL),
	_logger(logger),
	_cs(lock),
	localLock(false)
{
	DBG("ehwlist::ehwlist");
	if (NULL == _cs)
	{
		_cs = new CriticalSection;
		localLock = true;
	}

	BuildDeviceList(NULL);
}

ehwlist::~ehwlist()
{
	Cleanup(NULL);

	delete _devchange;

	if (localLock)
		delete _cs;
} // destructor

//******************************************************************************
//
// BuildDeviceList
//
// Either get all the devices in the machine or just the specified ID
//
// Returns the last device found
//
//******************************************************************************

ehw *ehwlist::BuildDeviceList(char* /*pnpid*/)
{
	TUsbAudioStatus status;
	ehw *dev = NULL;

	DBG("ehwlist::BuildDeviceList");

	Cleanup(NULL);

	DBG_PRINTF(("\t\tdevice count %d",GetNumDevices()));

	status = TUSBAUDIO_EnumerateDevices();
	DBG_PRINTF(("TUSBAUDIO_EnumerateDevices 0x%x",status));
	if (TSTATUS_SUCCESS != status)
		return NULL;

	unsigned count = TUSBAUDIO_GetDeviceCount();
	DBG_PRINTF(("TUSBAUDIO_GetDeviceCount %d",count));

	_numdevices = 0;
	for (unsigned index = 0; index < count; index++)
	{
		TUsbAudioHandle handle;
		ehw::ptr list;

		status = TUSBAUDIO_OpenDeviceByIndex( index, &handle);
		DBG_PRINTF(("Opened  handle:0x%x  status:0x%x",handle,status));
		if (TSTATUS_SUCCESS != status)
			continue;

		TUsbAudioDeviceRunMode mode;

		status = TUSBAUDIO_GetDeviceUsbMode(handle,&mode);
		TUSBAUDIO_CloseDevice(handle);
		DBG_PRINTF(("TUSBAUDIO_CloseDevice handle:0x%x",handle));

		if (TSTATUS_SUCCESS != status)
			continue;

		if (DeviceRunMode_APP != mode)
		{
			DBG("device not in run mode");
			continue;
		}

		dev = new ehw(	index, NULL);

		//
		// add the new device to the list
		//
		dev->incReferenceCount();

		list = _hwlist;
		if (list)
			list->_prev = dev;
		dev->_next = list;
		_hwlist = dev;

		if (_logger)
		{
			String msg;

			msg = String::formatted("Found device %S  vendor:0x%x  type:0x%x",
				dev->GetUniqueName().toUTF8().getAddress(),dev->GetVendorId(),dev->GetBoxType());
			_logger->logMessage(msg);
		}

		_numdevices++;
	}

	return _hwlist;
}

//******************************************************************************
//
// RemoveDevice
//
//******************************************************************************

ehw * ehwlist::RemoveDevice(const char *pnpid)
{
	String strid(pnpid);
	ScopedLock lock(*_cs);
	ehw::ptr dev = NULL,prev,next;

	dev = _hwlist;
	while (dev)
	{
		prev = dev->_prev;
		next = dev->_next;

		if (strid.equalsIgnoreCase(dev->GetIdString()))
		{
			if (prev)
				prev->_next = next;
			if (next)
				next->_prev = prev;
			if (_hwlist == dev)
				_hwlist = next;

			dev->decReferenceCount();

			_numdevices--;

			break;
		}

		dev = next;
	}

	return dev;
}

//******************************************************************************
//
// FindDevice
//
//******************************************************************************

ehw *ehwlist::FindDevice(char *pnpid)
{
	String strid(pnpid);
	ScopedLock lock(*_cs);
	ehw::ptr dev = NULL;

	dev = _hwlist;
	while (dev)
	{
		if (strid.equalsIgnoreCase(dev->GetIdString()))
		{
			return dev;
		}

		dev = dev->_next;
	}

	return NULL;
}

//******************************************************************************
//
// Call this function to look at all the devices; index ranges from
// 0 to GetNumDevices()-1
//
//******************************************************************************

ehw *ehwlist::GetNthDevice(int32 index)
{
	ehw *pTemp = _hwlist;
	int32 count = 0;

	while (pTemp != NULL)
	{
		if (count == index)
			break;

		count++;
		pTemp = pTemp->_next;
	}

	return pTemp;
} // GetNthDevice

//******************************************************************************
//
// Cleanup
//
//******************************************************************************

void ehwlist::Cleanup(ehw *pDeviceToKeep)
{
	ScopedLock lock(*_cs);

	//
	// If the specified device is non-NULL, remove it from the list
	//
	DBG_PRINTF(("ehwlist::Cleanup  pDeviceToKeep:%p",pDeviceToKeep));
	if (NULL != pDeviceToKeep)
	{
		ehw *prev,*next;

		prev = pDeviceToKeep->_prev;
		next = pDeviceToKeep->_next;

		if (prev)
		{
			prev->_next = next;
			pDeviceToKeep->_prev = NULL;
		}

		if (next)
		{
			next->_prev = prev;
			pDeviceToKeep->_next = NULL;
		}
	}

	//
	// Dump the linked list except for the device passed in
	// pDeviceToKeep
	//
	ehw *dev;

	dev = _hwlist;
	while (NULL != (ehw *) dev)
	{
		ehw::ptr dead;

		dead = dev;
		DBG_PRINTF(("Dec ref count for  handle:0x%x  count:%d",dead->GetNativeHandle(),dead->getReferenceCount()));
		dead->decReferenceCount();
		dev = dev->_next;
 	}

	_hwlist = pDeviceToKeep;
}

//******************************************************************************
//
// register callbacks
//
//******************************************************************************

void ehwlist::RegisterMessageListener(MessageListener *listener)
{
	if (NULL == _devchange)
		_devchange = new CDevChangeNotify(&InterfaceGuid,this);

	_devchange->RegisterMessageListener(listener);
}

void ehwlist::UnregisterMessageListener(MessageListener *listener)
{
	if (_devchange)
	{
		_devchange->UnregisterMessageListener(listener);
	}
}