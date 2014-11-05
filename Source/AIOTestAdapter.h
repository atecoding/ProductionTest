
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

	bool open();
	void close();
	int write(uint8 byte);
	int read(uint16 data[4]);

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
		ECHO_HID_TESTER_PRODUCT_ID = 0x200
	};
};

