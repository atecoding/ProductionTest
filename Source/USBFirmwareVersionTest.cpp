#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"

bool RunUSBFirmwareVersionTest(XmlElement const *element,
                 ehw *dev,
                 String &msg,
                 int &displayedInput,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes)
{
    uint32 min_version = element->getStringAttribute("min_version").getHexValue32();
    uint32 deviceFirmwareVersion = dev->getFirmwareVersion();
    
    displayedInput = -1;
    
    msg = "Device firmware version " + String::toHexString((int32)deviceFirmwareVersion).getLastCharacters(4);
    if (deviceFirmwareVersion >= min_version)
    {
        msg += " OK";
        return true;
    }
    
    errorCodes.add(ErrorCodes::FIRMWARE);
    msg += " FAIL";
    return false;
}
#endif
