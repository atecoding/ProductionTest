#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbt.h>
#include "../../base.h"
#include "NotificationWindow.h"
#include "cdevchange.h"
#include "ehw.h"
#include "ehwlist.h"

static char classname[] = "Device notification window";

#pragma warning(disable:4100) // disable warning for parameter not used
CDevChangeNotify::CDevChangeNotify(GUID* InterfaceGuid,ehwlist* devlist) :
	_devlist(devlist)
{
	DBG("CDevChangeNotify");

	Win32NotificationWindow::addListener(this);

#ifdef ECHOUSB
	//
	// Register for TUSBAUDIO PnP notification
	//
	TUsbAudioStatus status;

	status = TUSBAUDIO_RegisterPnpNotification(NULL,NULL,Win32NotificationWindow::handle(),WM_USER,0);
#else
	//
	// register for WM_DEVICECHANGE
	//
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
	NotificationFilter.dbcc_size =			sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype =	DBT_DEVTYP_DEVICEINTERFACE;
	memcpy( &NotificationFilter.dbcc_classguid, InterfaceGuid, sizeof(GUID));

	_hdn = RegisterDeviceNotification(	Win32NotificationWindow::handle(),
										&NotificationFilter,
										DEVICE_NOTIFY_WINDOW_HANDLE);
#endif
}

CDevChangeNotify::~CDevChangeNotify()
{
	DBG("~CDevChangeNotify");

#ifdef ECHOUSB
	TUSBAUDIO_UnregisterPnpNotification();
#else
	if (_hdn)
		UnregisterDeviceNotification(_hdn);
#endif

	Win32NotificationWindow::removeListener(this);
}

//******************************************************************************
//
// Window message callback
//
//******************************************************************************

void CDevChangeNotify::Win32MessageReceived(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
#ifdef ECHOUSB
	if ((WM_USER == uMsg) && (_listeners.size()))
	{
		bool post_message = false;
		int parameter = 0;

		switch (wParam)
		{
		case TUSBAUDIO_DEVICE_ARRIVAL_MSG :
		{
			DBG("CDevChangeNotify::Win32MessageReceived - TUSBAUDIO_DEVICE_ARRIVAL_MSG");
			//Logger::outputDebugString("TUSBAUDIO_DEVICE_ARRIVAL_MSG");
			parameter = EHW_DEVICE_ARRIVAL;
			post_message = true;
			break;
		}

		case TUSBAUDIO_DEVICE_REMOVED_MSG :
			DBG("CDevChangeNotify::Win32MessageReceived - TUSBAUDIO_DEVICE_REMOVED_MSG");
			//Logger::outputDebugString("TUSBAUDIO_DEVICE_REMOVED_MSG");
			parameter = EHW_DEVICE_REMOVAL;
			post_message = true;
			break;
		}

		if (post_message)
		{
			for (int index = 0; index < _listeners.size(); index++)
			{
				DeviceChangeMessage *msg;

				msg = new DeviceChangeMessage(parameter,0,0,_listeners[index]);
				DBG(String::formatted("Message 0x%x to 0x%p",msg->intParameter1,_listeners[index]));
				_listeners[index]->postMessage(msg);
			}
		}
	}
#else
	if ((WM_DEVICECHANGE == uMsg) && (_listeners.size() != 0))
	{
		DEV_BROADCAST_HDR *dbh;
		ehw *dev;
		DEV_BROADCAST_DEVICEINTERFACE_A *dbdi;

		DBG_PRINTF(("WM_DEVICECHANGE 0x%x",wParam));

		switch (wParam)
		{
			case DBT_DEVICEARRIVAL :
				dbh = (DEV_BROADCAST_HDR *) lParam;
				if (DBT_DEVTYP_DEVICEINTERFACE == dbh->dbch_devicetype)
				{
					DBG("Device arrival");

					//
					// Add the new PnP ID to the device list
					//
					dbdi = (DEV_BROADCAST_DEVICEINTERFACE_A *) lParam;
					dev = _devlist->BuildDeviceList(dbdi->dbcc_name);

					//
					// Post a message to JUCE-land
					//
					if (dev)
					{
						PostMessageToListeners(EHW_DEVICE_ARRIVAL,dev);
					}
				}
				break;

			case DBT_DEVICEQUERYREMOVE :
			case DBT_DEVICEREMOVECOMPLETE :
			case DBT_DEVICEREMOVEPENDING :
				dbh = (DEV_BROADCAST_HDR *) lParam;
				if (DBT_DEVTYP_DEVICEINTERFACE == dbh->dbch_devicetype)
				{
					DBG("Device removal");

					//
					// See if this device is in the list
					//
					dbdi = (DEV_BROADCAST_DEVICEINTERFACE_A *) lParam;
					dev = _devlist->FindDevice(dbdi->dbcc_name);
					if (NULL == dev)
						return;

					//
					// Verify that the device has disconnected
					//
					if (0 == dev->updatepolledstuff())
					{
						DBG("Device is still connected");
						return;
					}

					//
					// Remove this PnP ID from the device list
					//
					DBG("Removing device from list");
					_devlist->RemoveDevice(dbdi->dbcc_name);

					//
					// Post a message to JUCE-land
					//
					if (dev)
					{
						PostMessageToListeners(EHW_DEVICE_REMOVAL,dev);
					}
				}
				break;
		}
	}
#endif
}

void CDevChangeNotify::PostMessageToListeners(int const code,ehw * dev)
{
	for (int index = 0; index < _listeners.size(); index++)
	{
		DeviceChangeMessage *msg;

		msg = new DeviceChangeMessage(code,0,0,dev);
		_listeners[index]->postMessage(msg);
	}
}

//******************************************************************************
//
// register for notification messages
//
//******************************************************************************

void CDevChangeNotify::RegisterMessageListener(MessageListener *listener)
{
	if (false == _listeners.contains(listener))
	{
		_listeners.add( listener );
	}
}

void CDevChangeNotify::UnregisterMessageListener(MessageListener *listener)
{
	int index = _listeners.indexOf(listener);
	if (index >= 0)
	{
		_listeners.remove( index );
	}
}

#endif
