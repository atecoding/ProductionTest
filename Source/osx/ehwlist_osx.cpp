#include "../base.h"
#include "ehwlist.h"

ehwlist::ehwlist(int num_vendor_ids,uint32 *vendor_ids,CriticalSection *lock,FileLogger *logger)
{
    BuildDeviceList(nullptr);
}

ehwlist::~ehwlist()
{
    devices.clear();
    types.clear();
}

ehwlist::Type::Type(ehwlist *parent, SInt32 productID) :
notificationPort(NULL)
{
    kern_return_t          status;
    SInt32                 vid = ECHO_VENDOR_ID;
    
    dictionary.object = IOServiceMatching(kIOUSBDeviceClassName);
    if (NULL == dictionary.object)
        return;
    
    CFRunLoopSourceRef runloop = NULL;
    
    notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    if (NULL == notificationPort)
        return;
    
    runloop = IONotificationPortGetRunLoopSource(notificationPort);
    if (NULL == runloop)
        return;
    
    CFDictionaryAddValue(dictionary.object, CFSTR(kUSBVendorID), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vid));
    CFDictionaryAddValue(dictionary.object, CFSTR(kUSBProductID), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &productID));
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runloop, kCFRunLoopDefaultMode);
    
    //
    // Device arrival notification
    //
    // IOServiceAddMatchingNotification consumes a reference to the dictionary,
    // so retain it first
    //
    CFRetain(dictionary.object);
    status = IOServiceAddMatchingNotification(notificationPort,
                                              kIOMatchedNotification,
                                              dictionary.object,
                                              serviceMatched,
                                              parent,
                                              &iteratorMatching.object);
    if (0 != status)
        return;
    
    //
    // Walk through the device arrival iterator and look for devices
    //
    serviceMatched(parent,iteratorMatching.object);
    
    //
    // Device removal notification
    //
    CFRetain(dictionary.object);
    status = IOServiceAddMatchingNotification(notificationPort,
                                              kIOTerminatedNotification,
                                              dictionary.object,
                                              serviceTerminated,
                                              parent,
                                              &iteratorTerminated.object);
    if (0 != status)
        return;
    
    //
    // Clear out the device removal iterator; have to do this on startup
    // or it doesn't work properly
    //
    serviceTerminated(parent,iteratorTerminated.object);
}

ehwlist::Type::~Type()
{
    if (notificationPort)
        IONotificationPortDestroy( notificationPort);
}

void ehwlist::serviceMatched(void *context,io_iterator_t iterator)
{
    io_object_t service;
    ehwlist *that = (ehwlist *)context;
    
    while ((service = IOIteratorNext(iterator)))
    {
        SInt32 score;
        IOCFPlugInInterface** pluginInterface = 0;
        kern_return_t status;
        
        DBG("Found a matching service");
        status = IOCreatePlugInInterfaceForService(service,
                                                   kIOUSBDeviceUserClientTypeID,
                                                   kIOCFPlugInInterfaceID,
                                                   &pluginInterface,
                                                   &score);
        if (kIOReturnSuccess != status || NULL == pluginInterface)
        {
            DBG("IOCreatePlugInInterfaceForService returned " << (int)status);
            IOObjectRelease(service);
            continue;
        }
        
        // Use the plugin interface to retrieve the device interface.
        IOUSBDeviceInterface** deviceInterface;
        status = (*pluginInterface)->QueryInterface(pluginInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID *)&deviceInterface);
        (*pluginInterface)->Release(pluginInterface);
        
        if (kIOReturnSuccess == status && NULL != deviceInterface)
        {
            DBG("New device interface OK");
            
            ehw* device = that->devices.add(new ehw(deviceInterface));
            
            that->PostChangeMessage(EHW_DEVICE_ARRIVAL, device);
        }
        else
        {
            DBG("(*pluginInterface)->QueryInterface returned " << (int)status);
        }
        
        IOObjectRelease(service);
    }
}

void ehwlist::serviceTerminated(void *context,io_iterator_t iterator)
{
    ehwlist *that = (ehwlist *)context;
    io_object_t service;
    
    while ((service = IOIteratorNext(iterator)))
    {
        ehw *device = that->devices[0];
        
        that->devices.remove(0);
        
        that->PostChangeMessage(EHW_DEVICE_REMOVAL, device);
        
        IOObjectRelease(service);
    }
}

ehw *ehwlist::BuildDeviceList(char *pnpid)
{
    types.add(new Type(this, hwcaps::ACOUSTICIO_PRODUCT_ID));
    types.add(new Type(this, hwcaps::ACOUSTICIO_M1_PRODUCT_ID));
    types.add(new Type(this, hwcaps::ACOUSTICIO_M2_PRODUCT_ID));
    
    return nullptr;
}

#if 0
ehw *ehwlist::RemoveDevice(const char *pnpid)
{
    return nullptr;
}

ehw *ehwlist::FindDevice(char *pnpid)
{
    return nullptr;
}
#endif

ehw *ehwlist::GetNthDevice(int32 index)
{
    return devices[index];
}

void ehwlist::Cleanup(ehw *pDeviceToKeep)
{
    devices.clear();
}

void ehwlist::RegisterMessageListener(MessageListener *listener)
{
    if (false == _listeners.contains(listener))
    {
        _listeners.add( listener );
    }
}

void ehwlist::UnregisterMessageListener(MessageListener *listener)
{
    int index = _listeners.indexOf(listener);
    if (index >= 0)
    {
        _listeners.remove( index );
    }
}

void ehwlist::PostChangeMessage(int code, ehw *device)
{
    for (int index = 0; index < _listeners.size(); index++)
    {
        DeviceChangeMessage *msg;
        
        msg = new DeviceChangeMessage(code,0,0,device);
        _listeners[index]->postMessage(msg);
    }
}
