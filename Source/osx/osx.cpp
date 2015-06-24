#include "../base.h"
#include <IOKit/IOKitLib.h>

#if JUCE_MAC
String const getMacModelID()
{
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                       IOServiceMatching("IOPlatformExpertDevice"));
    
    CFDataRef modelIDCF = (CFDataRef)IORegistryEntryCreateCFProperty(service,
                                                                     CFSTR("model"),
                                                                     kCFAllocatorDefault,
                                                                     0);
    
    CFStringRef serialNumber = (CFStringRef)IORegistryEntryCreateCFProperty(service, CFSTR("IOPlatformSerialNumber"), kCFAllocatorDefault, 0);
    
    const UInt8 *data = CFDataGetBytePtr(modelIDCF);
    CFIndex length = CFDataGetLength(modelIDCF);
    String modelIDString( (const char *)data, length - 1);

    modelIDString += " " + String::fromCFString(serialNumber);

    CFRelease(modelIDCF);
    CFRelease(serialNumber);
    IOObjectRelease(service);
    
    return modelIDString;
}
#endif