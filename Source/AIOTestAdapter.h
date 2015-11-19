
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
        NUM_INPUTS = 4
    };

	bool open();
	void close();
	int write(uint8 byte);
	int read(uint16 data[NUM_INPUTS]);

protected:
#ifdef _WIN32
	HANDLE deviceHandle;
	HANDLE readHandle;
	HANDLE writeHandle;
#endif
    
#ifdef JUCE_MAC
    void findAdapter(IOHIDManagerRef &managerRef, CFSetRef &deviceCFSetRef, HeapBlock<IOHIDDeviceRef> &deviceRefs, CFIndex &deviceCount);
    
    ScopedCFObject<IOHIDDeviceRef> deviceRef;
#endif
    
	enum
	{
		ECHO_VENDOR_ID = 0x40f,
		ECHO_HID_TESTER_PRODUCT_ID = 0x210		// 0x100 = original, 0x200 = 2nd version in case,  0x210 = calibration version
	};
};

