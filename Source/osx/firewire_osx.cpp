#include "juce.h"
#include <IOKit/IOTypes.h>
#include <IOKit/IOCFPlugin.h>
#include <IOKit/firewire/IOFireWireLib.h>
#include "firewire_osx.h"

#ifndef DBG_PRINTF
#define DBG_PRINTF(x) DBG(String::formatted x )
#endif

void FireWireBusReset()
{
	kern_return_t   result;
	mach_port_t     masterPort;
	
	DBG("FireWireBusReset");
	
	result = IOMasterPort( MACH_PORT_NULL, &masterPort );
	if (0 == result)
	{
		CFMutableDictionaryRef  matchingDictionary = IOServiceMatching( "IOFireWireDevice" );
		if (matchingDictionary)
		{
			kern_return_t   result;
			io_iterator_t   iterator;
			
			result = IOServiceGetMatchingServices( masterPort, matchingDictionary, &iterator );
			if (0 == result)
			{
				io_object_t aDevice;
				
				while ( (aDevice = IOIteratorNext( iterator ) ) != 0 ) 
				{
					IOCFPlugInInterface**   cfPlugInInterface = 0;
					IOReturn                result;
					SInt32                  theScore;
					
					result = IOCreatePlugInInterfaceForService( aDevice, kIOFireWireLibTypeID, kIOCFPlugInInterfaceID, &cfPlugInInterface, &theScore );
					if (0 == result)
					{
						IOFireWireLibDeviceRef  fwDeviceInterface = 0;
						
						(*cfPlugInInterface)->QueryInterface( cfPlugInInterface, CFUUIDGetUUIDBytes(kIOFireWireDeviceInterfaceID_v7 ), (void **) &fwDeviceInterface );
						if (*fwDeviceInterface)
						{
							IOReturn result;
							
							IOFireWireSessionRef session;
							
							session = (*fwDeviceInterface)->GetSessionRef(fwDeviceInterface);
							
							result = (*fwDeviceInterface)->Open(fwDeviceInterface);
							DBG_PRINTF(("open result %d (0x%x)",result,result));
							if (0 == result)
							{
								result = (*fwDeviceInterface)->BusReset(fwDeviceInterface);
								DBG_PRINTF(("BusReset result %d (0x%x)",result,result));
								
								(*fwDeviceInterface)->Close(fwDeviceInterface);
							}
							
							(*fwDeviceInterface)->Release(fwDeviceInterface);
						}
						
						IODestroyPlugInInterface(cfPlugInInterface);
					}
					IOObjectRelease(aDevice);
				}
				IOObjectRelease(iterator);
			}
		}
	}
}