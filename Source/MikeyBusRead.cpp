#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"

bool MikeyBusRead(XmlElement const *element,
                 ehw *dev,
                 String &msg,
                 String &displayedChannel,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes,
                 ValueTree &unitTree)
{
    uint32 module = element->getIntAttribute("module");
    uint32 page = element->getStringAttribute("page").getHexValue32();
    uint32 address = element->getStringAttribute("address").getHexValue32();
    uint8 expectedValue = (uint8)element->getStringAttribute("value").getHexValue32();
    uint8 actualValue;
    
    Result result( dev->readMikey(module, page, address, actualValue));
    if (result.failed())
    {
        msg = "*** Could not read MikeyBus register - FAIL";
        errorCodes.add(ErrorCodes::MIKEY_BUS, -1);
        return false;
    }
    
    if (expectedValue != actualValue)
    {
        msg = "*** MikeyBus read test mismatch - FAIL";
        errorCodes.add(ErrorCodes::MIKEY_BUS, -1);
        return false;
    }
    
    msg = "MikeyBus read test PASS";
    
    return true;
}

#endif