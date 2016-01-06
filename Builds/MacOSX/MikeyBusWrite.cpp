#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"

bool MikeyBusWrite(XmlElement const *element,
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
    uint8 value = (uint8)element->getStringAttribute("value").getHexValue32();
    // uint8 actualValue;
    
    Result result( dev->writeMikey(module, page, address, value));
    if (result.failed())
    {
        msg = "*** Could not write MikeyBus register - FAIL";
        errorCodes.add(ErrorCodes::MIKEY_BUS, -1);
        return false;
    }
    
#if 0
    if (expectedValue != actualValue)
    {
        msg = "*** MikeyBus read test mismatch - FAIL";
        errorCodes.add(ErrorCodes::MIKEY_BUS, -1);
        return false;
    }
#endif
    
    msg = "MikeyBus write test PASS";
    
    return true;
}

#endif