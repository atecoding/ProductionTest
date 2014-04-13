//
//  AIODevices.h
//  AppleBox
//
//  Created by Matthew Gonzalez & Philip Scheid on 11/6/13.
//
//

#ifndef __AppleBox__USBDevices__
#define __AppleBox__USBDevices__

#include "AIODevice.h"

#if JUCE_MAC
#include "libusb.h"
#endif

#if JUCE_WIN32
#include "tusbaudioapi.h"
#endif

class AIODevices
{
public:

    AIODevices();
    ~AIODevices();

	OwnedArray<AIODevice> devices;

private:
    void enumerate();

#if JUCE_MAC
    libusb_context *context;
#endif

    enum
    {
        ECHO_VENDOR_ID = 0x40f,
        ACOUSTICIO_PRODUCT_ID = 0xa0
    };
};
#endif /* defined(__AppleBox__USBDevices__) */
