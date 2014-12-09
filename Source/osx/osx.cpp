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
    
    const UInt8 *data = CFDataGetBytePtr(modelIDCF);
    CFIndex length = CFDataGetLength(modelIDCF);
    String modelIDString( (const char *)data, length - 1);
    CFRelease(modelIDCF);
    IOObjectRelease(service);
    
    return modelIDString;
}
#endif