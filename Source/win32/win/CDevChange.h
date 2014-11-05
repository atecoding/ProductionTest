#ifndef _cdevchange_h_
#define _cdevchange_h_

#include "NotificationWindow.h"
#ifdef ECHOUSB
#include "tusbaudioapi.h"
#endif

class ehw;

enum
{
	EHW_DEVICE_ARRIVAL = 0xecc00000,
	EHW_DEVICE_REMOVAL
};

class ehwlist;
class CDevChangeNotify : public Win32MessageListener
{
public:
	CDevChangeNotify(GUID *InterfaceGuid,ehwlist *devlist);
	~CDevChangeNotify();

	void RegisterMessageListener(MessageListener *listener);
	void UnregisterMessageListener(MessageListener *listener);

	virtual void Win32MessageReceived(UINT uMsg,WPARAM wParam,LPARAM lParam);

protected:
#ifndef ECHOUSB
	HDEVNOTIFY	_hdn;
#endif

	ehwlist			*_devlist;
	Array<MessageListener *> _listeners;

protected:
	void PostMessageToListeners(int const code,ehw * dev);
};

class DeviceChangeMessage : public Message
{
public:
	DeviceChangeMessage(int p1,int /*p2*/,int /*p3*/,void* p4):
	intParameter1(p1),
	pointerParameter (p4)
	{
	}
	int intParameter1;
	void* pointerParameter;

	JUCE_LEAK_DETECTOR (DeviceChangeMessage)
};

#endif