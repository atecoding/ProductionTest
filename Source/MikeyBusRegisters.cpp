#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"

static const char *slotNames[] =
{
    "center",
    "outer"
};

bool MikeyBusRegisters(XmlElement const *element,
                 ehw *dev,
                 String &msg,
                 String &displayedChannel,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes,
                 ValueTree &unitTree)
{
    String const pageString("page");
    String const addressString("address");
    String const valueString("value");
    String const readString("read");
    String const writeString("write");
    uint8 module = (uint8)(element->getIntAttribute("module") & 1);
    String moduleName(String(slotNames[module]) + " module");
    
    forEachXmlChildElement (*element, child)
    {
		uint8 page = (uint8) child->getStringAttribute(pageString).getHexValue32();
		uint8 address = (uint8) child->getStringAttribute(addressString).getHexValue32();
        uint8 value = (uint8) child->getStringAttribute(valueString).getHexValue32();
        
        if (child->hasTagName (readString))
        {
            uint8 readValue;
            Result result( dev->readMikey(module, page, address, readValue));
            if (result.failed())
            {
                msg = "*** Could not read MikeyBus register for " + moduleName + " - FAIL";
                errorCodes.add(ErrorCodes::MIKEY_BUS, module);
                return false;
            }
            
            if (value != readValue)
            {
                msg = "*** MikeyBus read test mismatch for " + moduleName + " page " + String::toHexString(page) + ", address " + String::toHexString(address) + " value:" + String::toHexString(readValue) + " - FAIL";
				errorCodes.add(ErrorCodes::MIKEY_BUS, module);
                return false;
            }
            
            continue;
        }
        
        if (child->hasTagName(writeString))
        {
            Result result( dev->writeMikey(module, page, address, value));
            if (result.failed())
            {
                msg = "*** Could not write MikeyBus register " + moduleName + " - FAIL";
                errorCodes.add(ErrorCodes::MIKEY_BUS, module);
                return false;
            }
            
            continue;
        }
        
        //Thread::sleep(1);
    }

    msg = "MikeyBus " + moduleName + " register test - PASS";
    
    return true;
}

#endif