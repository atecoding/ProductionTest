#if ACOUSTICIO_BUILD

#include "base.h"
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

void AIOTestAdapter::findAdapter(IOHIDManagerRef &managerRef, CFSetRef &deviceCFSetRef, HeapBlock<IOHIDDeviceRef> &deviceRefs, CFIndex &deviceCount)
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
                                bool match = false;
                                switch (productID_)
                                {
                                    case ECHO_HID_TESTER_PRODUCT_ID_V100:
                                    case ECHO_HID_TESTER_PRODUCT_ID_V200:
                                    case ECHO_HID_TESTER_PRODUCT_ID_V210:
                                        DBG("Test adapter found");
                                        
                                        deviceRef.object = deviceRefs[i];
                                        CFRetain(deviceRef.object);
                                        productId = productID_;
                                        match = true;
                                        break;
                                }
                                
                                if (match)
                                    break;
                            }
                        }
                    }
                }
            }
        }
        
    }
}

AIOTestAdapter::AIOTestAdapter() :
productId(0)
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
    
    findAdapter(managerRef, deviceCFSetRef, deviceRefs, deviceCount);
    
    CFRelease(deviceCFSetRef);
    CFRelease(managerRef);
    
    if (deviceRef.object != 0)
    {
        IOReturn status = IOHIDDeviceOpen(deviceRef.object, kIOHIDOptionsTypeNone);
        if (kIOReturnSuccess != status)
        {
            CFRelease(deviceRef.object);
            deviceRef.object = 0;
        }
    }
    
    return deviceRef.object != 0;
}

void AIOTestAdapter::close()
{
    if (deviceRef.object)
    {
        IOHIDDeviceClose(deviceRef.object, kIOHIDOptionsTypeNone);
        
        CFRelease(deviceRef.object);
        deviceRef.object = 0;
    }
}

int AIOTestAdapter::write(uint8 byte)
{
#if 0
	uint8 data[2];
	data[0] = 0;
	data[1] = byte;

	if (INVALID_HANDLE_VALUE == writeHandle)
		return 0;

	HidD_SetOutputReport(writeHandle, data, sizeof(data));
	Thread::sleep(100);
	BOOLEAN result = HidD_SetOutputReport(writeHandle, data, sizeof(data));		// send twice because of buffering somewhere -- quien sabes?

	return result;
#else
    
    IOReturn  status = IOHIDDeviceSetReport(
                                               deviceRef.object,          // IOHIDDeviceRef for the HID device
                                               kIOHIDReportTypeOutput,   // IOHIDReportType for the report
                                               0,           // CFIndex for the report ID
                                               &byte,             // address of report buffer
                                               sizeof(byte));      // length of the report
    Thread::sleep(100);
    IOReturn status2 = IOHIDDeviceSetReport(
                                            deviceRef.object,          // IOHIDDeviceRef for the HID device
                                            kIOHIDReportTypeOutput,   // IOHIDReportType for the report
                                            0,           // CFIndex for the report ID
                                            &byte,             // address of report buffer
                                            sizeof(byte));      // length of the report
    
    return kIOReturnSuccess == status && kIOReturnSuccess == status2;
#endif
}

int AIOTestAdapter::read(uint16 data[AIOTestAdapter::NUM_INPUTS])
{
#if 0
	uint8 temp[9];

	if (INVALID_HANDLE_VALUE == readHandle)
		return 0;

	zerostruct(temp);
	BOOLEAN result = HidD_GetInputReport(readHandle, temp, sizeof(temp));
	if (0 == result)
	{
		return 0;
	}

	uint16 *source = (uint16 *)(temp + 1);
	for (int i = 0; i < 4; ++i)
	{
		data[i] = source[i];
	}
	
	return 4;
#else
    CFIndex length = sizeof(uint16) * 4;
    IOReturn  status = IOHIDDeviceGetReport(
                                               deviceRef.object,          // IOHIDDeviceRef for the HID device
                                               kIOHIDReportTypeInput,   // IOHIDReportType for the report
                                               0,           // CFIndex for the report ID
                                               (uint8 *)data,             // address of report buffer
                                               &length);       // address of length of the report
    return kIOReturnSuccess == status;
#endif
}

#endif