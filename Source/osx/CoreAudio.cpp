#include <CoreAudio/AudioHardware.h>
#include "../base.h"

String const Apple("Apple Inc.");

void CoreAudioTest()
{
    AudioObjectPropertyAddress aopa;
    aopa.mSelector = kAudioHardwarePropertyDevices;
    aopa.mScope = kAudioObjectPropertyScopeGlobal;
    aopa.mElement = kAudioObjectPropertyElementMaster;
    
    uint32 propSize;
    OSStatus error = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &aopa, 0, NULL, &propSize);
    if (noErr == error)
    {
        int deviceCount = propSize / sizeof(AudioDeviceID);
        HeapBlock<uint8> block;
        
        block.allocate(propSize, true);
        AudioDeviceID* audioDevices = (AudioDeviceID*) block.getData();
        
        error = AudioObjectGetPropertyData(kAudioObjectSystemObject, &aopa, 0, NULL, &propSize, audioDevices);
        if (noErr == error)
        {
            for (int i = 0; i < deviceCount; ++i)
            {
                AudioDeviceID deviceID = audioDevices[i];
                UInt32 propSize = sizeof(CFStringRef);
                
                CFStringRef result;
                aopa.mSelector = kAudioDevicePropertyDeviceManufacturerCFString;
                error = AudioObjectGetPropertyData(deviceID, &aopa, 0, NULL, &propSize, &result);
                if (noErr != error)
                {
                    continue;
                }
                CFRelease(result);
                
                aopa.mSelector = kAudioDevicePropertyDeviceNameCFString;
                error = AudioObjectGetPropertyData(deviceID, &aopa, 0, NULL, &propSize, &result);
                if (noErr != error)
                {
                    continue;
                }
                CFRelease(result);
            }
        }
    }
}
