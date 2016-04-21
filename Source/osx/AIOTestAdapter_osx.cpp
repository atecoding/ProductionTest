#if ACOUSTICIO_BUILD

#include "../base.h"
#if JUCE_MAC
#include "../AIOTestAdapter.h"

// ----------------------------------------------------
// function to create a matching dictionary for usage page & usage
static CFMutableDictionaryRef hu_CreateMatchingDictionaryUsagePageUsage( Boolean isDeviceNotElement,
                                                                        UInt32 inUsagePage,
                                                                        UInt32 inUsage )
{
    // create a dictionary to add usage page / usages to
    CFMutableDictionaryRef result = CFDictionaryCreateMutable( kCFAllocatorDefault,
                                                              0,
                                                              &kCFTypeDictionaryKeyCallBacks,
                                                              &kCFTypeDictionaryValueCallBacks );
    
    if ( result ) {
        if ( inUsagePage ) {
            // Add key for device type to refine the matching dictionary.
            CFNumberRef pageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsagePage );
            
            if ( pageCFNumberRef ) {
                if ( isDeviceNotElement ) {
                    CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsagePageKey ), pageCFNumberRef );
                } else {
                    CFDictionarySetValue( result, CFSTR( kIOHIDElementUsagePageKey ), pageCFNumberRef );
                }
                CFRelease( pageCFNumberRef );
                
                // note: the usage is only valid if the usage page is also defined
                if ( inUsage ) {
                    CFNumberRef usageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsage );
                    
                    if ( usageCFNumberRef ) {
                        if ( isDeviceNotElement ) {
                            CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsageKey ), usageCFNumberRef );
                        } else {
                            CFDictionarySetValue( result, CFSTR( kIOHIDElementUsageKey ), usageCFNumberRef );
                        }
                        CFRelease( usageCFNumberRef );
                    } else {
                        fprintf( stderr, "%s: CFNumberCreate( usage ) failed.", __PRETTY_FUNCTION__ );
                    }
                }
            } else {
                fprintf( stderr, "%s: CFNumberCreate( usage page ) failed.", __PRETTY_FUNCTION__ );
            }
        }
    } else {
        fprintf( stderr, "%s: CFDictionaryCreateMutable failed.", __PRETTY_FUNCTION__ );
    }
    return result;
}	// hu_CreateMatchingDictionaryUsagePageUsage

void AIOTestAdapter::findAdapters(IOHIDManagerRef &managerRef, CFSetRef &deviceCFSetRef, HeapBlock<IOHIDDeviceRef> &deviceRefs, CFIndex &deviceCount)
{
    managerRef = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
    
    IOHIDManagerSetDeviceMatching( managerRef, NULL );
    
    IOReturn status = IOHIDManagerOpen( managerRef, kIOHIDOptionsTypeNone );
    if (KERN_SUCCESS != status)
        return;
    
    deviceCFSetRef = IOHIDManagerCopyDevices( managerRef );
    if (NULL == deviceCFSetRef)
        return;
    
   deviceCount = CFSetGetCount( deviceCFSetRef );
    
    // allocate a block of memory to extact the device ref's from the set into
    deviceRefs.allocate(deviceCount, true);
    
    CFSetGetValues( deviceCFSetRef, (const void **) deviceRefs.getData() );
    
    for (int i = 0; i < deviceCount; ++i)
    {
        ScopedCFObject<CFTypeRef> vendorIDTypeRef;
        
        vendorIDTypeRef.object = IOHIDDeviceGetProperty( deviceRefs[i], CFSTR(kIOHIDVendorIDKey));
        
        if (vendorIDTypeRef.object)
        {
            if (CFNumberGetTypeID() == CFGetTypeID(vendorIDTypeRef.object))
            {
                int32 vendorID;
                bool result = CFNumberGetValue((CFNumberRef) vendorIDTypeRef.object, kCFNumberSInt32Type, &vendorID);
                if (result && ECHO_VENDOR_ID == vendorID)
                {
                    ScopedCFObject<CFTypeRef> productIDTypeRef;
                    
                    productIDTypeRef.object = IOHIDDeviceGetProperty( deviceRefs[i], CFSTR(kIOHIDProductIDKey));
                    if (productIDTypeRef.object)
                    {
                        if (CFNumberGetTypeID() == CFGetTypeID(productIDTypeRef.object))
                        {
                            int32 productID_;
                            result = CFNumberGetValue((CFNumberRef) productIDTypeRef.object, kCFNumberSInt32Type, &productID_);
                            if (result)
                            {
                                switch (productID_)
                                {
                                    case ECHO_HID_TESTER_PRODUCT_ID_V100:
                                    case ECHO_HID_TESTER_PRODUCT_ID_V200:
                                    case ECHO_HID_TESTER_PRODUCT_ID_V210:
                                        DBG("Test adapter found");
                                        
                                        testAdapterDeviceRefs.objects.add(deviceRefs[i]);
                                        CFRetain(deviceRefs[i]);
                                        productIDs.add( productID_ );
                                        break;
                                }
                            }
                        }
                    }
                }
            }
        }
        
    }
}

AIOTestAdapter::AIOTestAdapter() :
maxRequestTicks(0)
{
}

AIOTestAdapter::~AIOTestAdapter()
{
	close();
}


bool AIOTestAdapter::open()
{
    IOHIDManagerRef managerRef = NULL;
    CFSetRef deviceCFSetRef = NULL;
    HeapBlock<IOHIDDeviceRef> deviceRefs;
    CFIndex deviceCount;
    
    findAdapters(managerRef, deviceCFSetRef, deviceRefs, deviceCount);
    
    CFRelease(deviceCFSetRef);
    CFRelease(managerRef);
    
    maxRequestTicks = 0;
    
    int openCount = 0;
    for (int i = 0; i < testAdapterDeviceRefs.objects.size(); ++i)
    {
        IOHIDDeviceRef hidDeviceRef = testAdapterDeviceRefs.objects[i];
        if (hidDeviceRef)
        {
            IOReturn status = IOHIDDeviceOpen(testAdapterDeviceRefs.objects[i], kIOHIDOptionsTypeNone);
            if (kIOReturnSuccess != status)
            {
                CFRelease(testAdapterDeviceRefs.objects[i]);
                testAdapterDeviceRefs.objects.set(i, 0);
            }
            else
            {
                openCount++;
            }
        }
    }
    
    return openCount != 0;
}

void AIOTestAdapter::close()
{
    for (int i = 0; i < testAdapterDeviceRefs.objects.size(); ++i)
    {
        IOHIDDeviceRef hidDeviceRef = testAdapterDeviceRefs.objects[i];
        
        if (hidDeviceRef)
        {
            IOHIDDeviceClose(hidDeviceRef, kIOHIDOptionsTypeNone);
            
            CFRelease(hidDeviceRef);
            testAdapterDeviceRefs.objects.set(i, 0);
        }
    }
}

int AIOTestAdapter::write(uint8 byte)
{
    int result = 1;
    
    for (int i = 0; i < testAdapterDeviceRefs.objects.size(); ++i)
    {
        IOHIDDeviceRef hidDeviceRef = testAdapterDeviceRefs.objects[i];
        
        if (0 == hidDeviceRef)
            continue;
        
        {
            int64 begin = Time::getHighResolutionTicks();
            
            IOReturn status = IOHIDDeviceSetReport( hidDeviceRef,          // IOHIDDeviceRef for the HID device
                                                    kIOHIDReportTypeOutput,   // IOHIDReportType for the report
                                                    0,           // CFIndex for the report ID
                                                    &byte,             // address of report buffer
                                                    sizeof(byte));      // length of the report
            
            int64 end = Time::getHighResolutionTicks();
            maxRequestTicks = jmax(maxRequestTicks, end - begin);
            
            result &= kIOReturnSuccess == status;
        }
        Thread::sleep(100);
        
        {
            int64 begin = Time::getHighResolutionTicks();
            
            IOReturn status = IOHIDDeviceSetReport( hidDeviceRef,          // IOHIDDeviceRef for the HID device
                                                    kIOHIDReportTypeOutput,   // IOHIDReportType for the report
                                                    0,           // CFIndex for the report ID
                                                    &byte,             // address of report buffer
                                                    sizeof(byte));      // length of the report
            
            int64 end = Time::getHighResolutionTicks();
            maxRequestTicks = jmax(maxRequestTicks, end - begin);
            
            result &= kIOReturnSuccess == status;
        }
        Thread::sleep(100);
    }
    
    return result;
}

int AIOTestAdapter::read(Array<uint16> &data)
{
    data.clearQuick();
    data.ensureStorageAllocated(NUM_INPUTS_PER_ADAPTER * testAdapterDeviceRefs.objects.size());

    for (int i = 0; i < testAdapterDeviceRefs.objects.size(); ++i)
    {
        IOHIDDeviceRef hidDeviceRef = testAdapterDeviceRefs.objects[i];
        if (0 == hidDeviceRef)
            continue;
        
        uint16 temp[NUM_INPUTS_PER_ADAPTER];
        CFIndex length = sizeof(temp);
        int64 begin = Time::getHighResolutionTicks();
        IOReturn status = IOHIDDeviceGetReport(
                                                hidDeviceRef,          // IOHIDDeviceRef for the HID device
                                                kIOHIDReportTypeInput,   // IOHIDReportType for the report
                                                0,           // CFIndex for the report ID
                                                (uint8 *)temp,             // address of report buffer
                                                &length);       // address of length of the report
        
        int64 end = Time::getHighResolutionTicks();
        maxRequestTicks = jmax(maxRequestTicks, end - begin);
        
        if (status != kIOReturnSuccess)
        {
            data.clearQuick();
            break;
        }
        
        for (int j = 0; j < NUM_INPUTS_PER_ADAPTER; ++j)
        {
            data.add(temp[j]);
        }
    }

    return data.size();
}

#endif
#endif
