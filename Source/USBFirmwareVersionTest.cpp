#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"
#include "Description.h"
#include "xml.h"


static String const interfaceModuleString("interface_module");

bool RunUSBFirmwareVersionTest(XmlElement const *element,
                    ehw *dev,
                    String &msg,
                    String &displayedChannel,
                    AIOTestAdapter &testAdapter,
                    Content *content,
                    ErrorCodes &errorCodes,
                    ValueTree &unitTree)
{
	uint32 deviceFirmwareVersion = dev->getFirmwareVersion();
    uint32 minFirmwareVersion = 0;
    uint32 maxFirmwareVersion = 0;
	uint32 AIO_Revision = dev->getDescription()->getInterfaceModuleVersion();
    
    {
        XmlElement* child = element->getFirstChildElement();
        
        while (child != nullptr)
        {
            if (child->getTagName() != interfaceModuleString)
            {
                msg = "Invalid child tag for firmware version test: " + child->getTagName();
                return false;
            }
            
            uint32 interfaceModuleRevision;
            if (false == getHexValue(child, "revision", interfaceModuleRevision))
            {
                msg = "Missing 'revision' scrpipt value for USB firmware version test";
                return false;
            }
            
            if (interfaceModuleRevision == AIO_Revision)
            {
                if (false == getHexValue(child, "min_version", minFirmwareVersion))
                {
                    msg = "Missing 'min_version' value for USB firmware version test";
                    return false;
                }
                
                if (false == getHexValue(child, "max_version", maxFirmwareVersion))
                {
                    maxFirmwareVersion = 0xffffffff;
                }
                
                break;
            }
            
            child = child->getNextElement();
        }
        
        if (nullptr == child)
        {
            msg = String::formatted("USB firmware version test - interface module type 0x%x not found in script", AIO_Revision);
            return false;
        }
    }
    

    msg = "Device firmware version " + String::toHexString((int32)deviceFirmwareVersion).getLastCharacters(4);
	if (deviceFirmwareVersion >= minFirmwareVersion && deviceFirmwareVersion <= maxFirmwareVersion)
    {
        msg += " OK";
        return true;
    }
    
    errorCodes.add(ErrorCodes::FIRMWARE);
    msg += " FAIL";
    return false;
}
#endif
