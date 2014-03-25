/********************************************************************************
 *
 * ehwlist.cpp - PCI and FireWire hardware
 *
 * A class that builds a list of Echo hardware objects.
 *
 * Copyright (c) 2008 Echo Digital Audio Corporation.  All rights reserved
 *
 ********************************************************************************/

#include <new>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include "juce.h"
#include "ehw.h"
#include "ehwlist.h"


//******************************************************************************
// Globals
//******************************************************************************

#ifdef PCI_BUILD

static GUID InterfaceGuid = SPLUNGE_CLASS_GUID;

#endif


#ifdef ECHO1394

static GUID InterfaceGuid = { 0x8e0985ca, 0x509a, 0x4718, { 0xa8, 0xa5, 0xef, 0xc, 0xfc, 0x99, 0xee, 0x87 } };

#endif


//******************************************************************************
// ehwlist methods
//******************************************************************************

ehwlist::ehwlist(int num_vendor_ids,uint32 *vendor_ids,CriticalSection *lock,FileLogger *logger) :
	_hwlist(NULL),
	_numdevices(0),
	_devchange(NULL),
	_logger(logger),
	_cs(lock),
	localLock(false)
{
	if (NULL == _cs)
	{
		_cs = new CriticalSection;
		localLock = true;
	}
	for (int index = 0; index < num_vendor_ids; index++)
	{
		_vendor_ids.add(vendor_ids[index]);
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

ehw *ehwlist::BuildDeviceList(char *pnpid)
{
	ScopedLock lock(*_cs);
	MemoryBlock mb(256);
	HDEVINFO hdi;
	ehw *dev = NULL;
	CriticalSection *device_lock;

	if (localLock)
		device_lock = NULL;	// each device should use its own lock
	else
		device_lock = _cs;	// each device should use the global lock

	//
	// Class handle
	//
	hdi = SetupDiGetClassDevs(	&InterfaceGuid,
								NULL,
								NULL,
								DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (INVALID_HANDLE_VALUE != hdi)
	{
		SP_DEVICE_INTERFACE_DATA did;
		DWORD index;

		index = 0;
		while (GetInterface(hdi,&did,mb,index++) != 0)
		{
			PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDetailData;
			ehw::ptr list;

			//
			// If pnpid is non-NULL, compare the strings
			// and only create the specified device
			//
			pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A) mb.getData();
			if (pnpid && (0 != _stricmp(pnpid,pDetailData->DevicePath)))
				continue;
				
			//
			// make the new device
			//
			dev = new ehw(	pDetailData->DevicePath,
							hdi,
							&did,
							device_lock);

			//
			// Check the vendor ID
			//
			// Accept any vendor ID if the vendor ID list is empty
			//
			bool keep = 0 == _vendor_ids.size();

			for (int index = 0; index < _vendor_ids.size(); index++)
			{
				if (dev->GetVendorId() == _vendor_ids[index])
				{
					keep = true;
					break;
				}
			}

			if (!keep)
			{
				if (_logger)
				{
					String msg;

					msg = String::formatted(T("Ignoring device %S  vendor:0x%x  type:0x%x"),
						dev->GetUniqueName(),dev->GetVendorId(),dev->GetBoxType());
					_logger->logMessage(msg);							
				}
				delete dev;
				dev = NULL;
				continue;
			}
			
			//
			// add the new device to the list
			//
			dev->incReferenceCount();

#if 1
			list = _hwlist;
			if (list)
				list->_prev = dev;
			dev->_next = list;
			_hwlist = dev;
#else
			list = _hwlist;
			if (list)
			{
				if (list->_next)
				{
					list->_next->_prev = dev;
				}
				
				dev->_prev = list;
				dev->_next = list->_next;
				
				list->_next = dev;
			}
			else
			{
				_hwlist = dev;
			}
#endif

			if (_logger)
			{
				String msg;

				msg = String::formatted(T("Found device %S  vendor:0x%x  type:0x%x  "),
					dev->GetUniqueName().toUTF8().getAddress(),dev->GetVendorId(),dev->GetBoxType());
				msg += "UniqueID:";
				msg += String( String::toHexString( (int64) dev->GetSerialNumber()) );
				_logger->logMessage(msg);	
			}

			_numdevices++;
		}

		SetupDiDestroyDeviceInfoList(hdi);
	}

	SortList();

	return dev;
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
// Using the handle returned by GetClassHandle, call GetInterface
// repeatedly until you get NULL as a return value.  index should be zero
// the first time you call and should be incremented by one for each subsequent
// call.
//
// If the return value from GetInterface is not NULL, you can use
// the string in the DevicePath field of the PSP_DEVICE_INTERFACE_DETAIL_DATA
// structure to create a new device object.
//
//******************************************************************************

BOOL ehwlist::GetInterface
(
	HDEVINFO hdi,
	PSP_DEVICE_INTERFACE_DATA did,	
	MemoryBlock &mb,
	DWORD index
)
{
	BOOL						rval;

	did->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	rval = SetupDiEnumDeviceInterfaces(	hdi,
										NULL,
										&InterfaceGuid,
										index,
										did);
	if (TRUE == rval)
	{
		DWORD	dwSize;

		//
		// Get the size for the details
		//
		SetupDiGetDeviceInterfaceDetail(	hdi,
											did,
											NULL,
											0,
											&dwSize,
											NULL);
		mb.ensureSize(dwSize);

		//
		// Now get the interface details using the correct size
		//
		PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDetailData;

		pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A) mb.getData();
		pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
		rval = SetupDiGetDeviceInterfaceDetailA(hdi,
												did,
												pDetailData,
												dwSize,
												NULL,
												NULL);
	}

	return rval;

} // GetInterface



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
	if (NULL != _devchange)
	{
		_devchange->UnregisterMessageListener(listener);
	}
}


//******************************************************************************
//
// Find a device based on the name
//
//******************************************************************************

/*
ehw *ehwlist::MatchName(char *devname)
{
	ScopedLock lock(*_cs);
	ehw *dev;
	String namestr(devname);
	
	dev = _hwlist;
	while (NULL != (ehw *) dev)
	{
		if (namestr.equalsIgnoreCase( dev->_path ))
			return dev;
	
		dev = dev->_next;
	}
	
	return NULL;
}
*/


void ehwlist::SortList()
{
	ehw *dev,*next,*prev;
	bool sorted;
	ScopedLock lock(*_cs);	

	//Logger::outputDebugString("ehwlist::SortList");

	do
	{
		sorted = true;
		dev = _hwlist;
		while ((nullptr != dev) && (nullptr != dev->_next))
		{
			next = dev->_next;

			//Logger::outputDebugString(String::formatted("dev %p, next %p",dev,next));
			
			if (dev->GetSerialNumber() > next->GetSerialNumber())
			{
				prev = dev->_prev;

				//Logger::outputDebugString(String::formatted("0x%I64x with 0x%I64x",dev->GetSerialNumber(),next->GetSerialNumber()));

				//
				// swap
				//
				if (nullptr != next->_next)
				{
					next->_next->_prev = dev;
					//Logger::outputDebugString(String::formatted("%p set to %p",next->_next->_prev,dev));
				}

				dev->_next = next->_next;
				dev->_prev = next;

				//Logger::outputDebugString(String::formatted("%p next:%p prev:%p",dev,dev->_next,dev->_prev));

				next->_prev = prev;
				next->_next = dev;

				//Logger::outputDebugString(String::formatted("%p next:%p prev:%p",next,next->_next,next->_prev));

				if (nullptr != prev)
				{
					prev->_next = dev;

					//Logger::outputDebugString(String::formatted("%p set to %p",prev->_next,dev));
				}

				if (_hwlist == dev)
				{
					_hwlist = next;

					//Logger::outputDebugString(String::formatted("_hwlist set to %p",next));
				}

				sorted = false;
			}

			dev = dev->_next;
		}
	} while (false == sorted);
}
