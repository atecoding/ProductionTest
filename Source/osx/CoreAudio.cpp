#include <CoreAudio/AudioHardware.h>
#include "../base.h"

String const appleName("Apple Inc.");
String const airplayName("AirPlay");

void CoreAudioTest()
{
    StringArray appleOutputDeviceNames;
    
    AudioObjectPropertyAddress aopa;
    aopa.mSelector = kAudioHardwarePropertyDevices;
    aopa.mScope = kAudioObjectPropertyScopeGlobal;
    aopa.mElement = kAudioObjectPropertyElementMaster;
    
    //UInt32 channel = 1; // Channel 0  is master, if available
    AudioObjectPropertyAddress outputVolumeProperty =
    {
        kAudioDevicePropertyVolumeScalar,
        kAudioDevicePropertyScopeOutput,
        0 // channel number (0 is master if supported)
    };

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
                
                //
                // Look for Apple hardware only
                //
                aopa.mSelector = kAudioDevicePropertyDeviceManufacturerCFString;
                error = AudioObjectGetPropertyData(deviceID, &aopa, 0, NULL, &propSize, &result);
                if (noErr != error)
                {
                    continue;
                }
                CFRelease(result);
                
                //
                // Output volume property?
                //
                outputVolumeProperty.mElement = 0;
                bool hasMasterVolume = AudioObjectHasProperty(deviceID, &outputVolumeProperty);
                
                outputVolumeProperty.mElement = 1;
                bool hasChannelVolume = AudioObjectHasProperty(deviceID, &outputVolumeProperty);
                if (false == hasChannelVolume && false == hasMasterVolume)
                {
                    continue;
                }
                
                //
                // Get the device name
                //
                aopa.mSelector = kAudioDevicePropertyDeviceNameCFString;
                error = AudioObjectGetPropertyData(deviceID, &aopa, 0, NULL, &propSize, &result);
                if (noErr != error)
                {
                    continue;
                }
                
                //
                // Reject AirPlay
                //
                String deviceName(String::fromCFString(result));
                CFRelease(result);

                if (deviceName == airplayName)
                    continue;
                
                //
                // OK keep it
                //
                appleOutputDeviceNames.add(deviceName);
            }
        }
    }
    
    for (int i = 0; i < appleOutputDeviceNames.size(); ++i)
    {
        DBG(appleOutputDeviceNames[i]);
    }
}
