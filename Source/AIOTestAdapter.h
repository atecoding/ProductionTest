
#pragma once

#ifdef JUCE_MAC
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDBase.h>
#include "ScopedCFObject.h"
#endif

class AIOTestAdapter
{
public:
	AIOTestAdapter();
	~AIOTestAdapter();
    
    enum
    {
        NUM_INPUTS_PER_ADAPTER = 4
    };

	bool open();
	void close();
	int write(uint8 byte);
	int read(Array<uint16> &data);
    
    bool checkProductID(uint32 const requiredProductID_) const
    {
        if (0 == productIDs.size())
            return false;
        
        for (int i = 0; i < productIDs.size(); ++i)
        {
            if (productIDs[i] != requiredProductID_)
                return false;
        }
        
        return true;
    }
    
    RelativeTime getMaxRequestTime() const
    {
        return RelativeTime( double(maxRequestTicks) / double(Time::getHighResolutionTicksPerSecond()));
    }

protected:
#ifdef _WIN32
	HANDLE deviceHandle;
	HANDLE readHandle;
	HANDLE writeHandle;
#endif
    
#ifdef JUCE_MAC
    void findAdapters(IOHIDManagerRef &managerRef, CFSetRef &deviceCFSetRef, HeapBlock<IOHIDDeviceRef> &deviceRefs, CFIndex &deviceCount);
    
    //ScopedCFObject<IOHIDDeviceRef> deviceRef;
    OwnedCFObjectArray<IOHIDDeviceRef> testAdapterDeviceRefs;
#endif
    
    int64 maxRequestTicks;
    Array<uint16> productIDs;
    
	enum
	{
		ECHO_VENDOR_ID = 0x40f,
        ECHO_HID_TESTER_PRODUCT_ID_V100 = 0x100,    // 0x100 == orginal
        ECHO_HID_TESTER_PRODUCT_ID_V200 = 0x200,    // 0x200 == 2nd version in case
		ECHO_HID_TESTER_PRODUCT_ID_V210 = 0x210		// 0x210 == 3rd version with calibration, low-pass filtering
	};
};

