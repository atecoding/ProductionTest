/********************************************************************************
 *
 * ehwlist.h
 *
 * A class that builds a list of Echo hardware objects
 *
 ********************************************************************************/

#pragma once

#if JUCE_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>
#include <IOKit/IOCFPlugIn.h>
#include "ScopedCFObject.h"
#include "ScopedIOObject.h"

#endif

#include "ehw.h"

enum
{
    EHW_DEVICE_ARRIVAL = 0xecc00000,
    EHW_DEVICE_REMOVAL
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

class ehw;
class ehwlist
{
protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ehwlist);
    
    ReferenceCountedArray<ehw> devices;
    
    static void serviceMatched(void *context,io_iterator_t iterator);
    static void serviceTerminated(void *context,io_iterator_t iterator);
    
    class Type
    {
    public:
        Type(ehwlist *parent, SInt32 productID);
        ~Type();
        
        ScopedCFObject<CFMutableDictionaryRef> dictionary;
        IONotificationPortRef notificationPort;
        ScopedIOObject<io_iterator_t> iteratorMatching;
        ScopedIOObject<io_iterator_t> iteratorTerminated;
    };
    
    OwnedArray<Type> types;
    
    void PostChangeMessage(int code, ehw *device);

public:
	ehwlist(int num_vendor_ids,uint32 *vendor_ids,CriticalSection *lock = NULL,FileLogger *logger = NULL);
	~ehwlist();

	ehw *BuildDeviceList(char *pnpid);
	ehw *RemoveDevice(const char *pnpid);
	ehw *FindDevice(char *pnpid);

	int32 GetNumDevices()
	{
		return devices.size();
	}

	ehw *GetNthDevice(int32 index);
	
	void Cleanup(ehw *pDeviceToKeep = NULL);
	
	void RegisterMessageListener(MessageListener *listener);
	void UnregisterMessageListener(MessageListener *listener);
	
	CriticalSection	*_cs;
	bool localLock;
    
    Array<MessageListener *> _listeners;

	enum
	{
		MATCH_ANY_VENDOR = 0
	};
    
    enum
    {
        ECHO_VENDOR_ID = 0x40f,
        XMOS_VENDOR_ID = 0x20b1,
        SLICEKIT_PRODUCT_ID = 0x0008
    };
};
