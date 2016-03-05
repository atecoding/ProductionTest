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

void mb_bulk_payload_tx(ehw* dev, uint8 module, uint8 funcID, uint8 *payload, uint8 length);

//--------------------------------------------------------------------------
//
// Handle MikeyBus bulk writes
//
// mikey b %1 0xff 0x06 0x53 0x0a 0x0d 0x0e 0x0f
//
// %1 is the module number (m0 or m1)
// 0xff is the MikeyBus function number
// 0x06 is the length of the bulk transfer, including this length byte
// 0x53 0x0a 0x0d 0x0e 0x0f is the remaining data
//
//
// XML version
//
// <bulk>0xff 0x06 0x53 0x0a 0x0d 0x0e 0x0f</bulk>
//
static Result MikeyBusBulkWrite(XmlElement const *element,
                         ehw *dev,
                         uint8 const moduleNumber)
{
    StringArray valueHexStrings;
    String elementText(element->getAllSubText());
    
    DBG("MikeyBusBulkWrite " + elementText);
    
    valueHexStrings.addTokens(elementText, false);
    
    HeapBlock<uint8> block(valueHexStrings.size(), true);
    
    uint32 actualLength = 0;
    for (int i = 0; i < valueHexStrings.size(); ++i)
    {
        if (valueHexStrings[i].isEmpty())
            break;
        //DBG(String(i) + ": " + valueHexStrings[i]);
        block[i] = (uint8)(valueHexStrings[i].getHexValue32() & 0xff);
        actualLength++;
    }
    
    uint8 length = block[1];
    uint8 funcID = block[0];
    
    if (length != actualLength - 1)
    {
        Result result(Result::fail(String::formatted("MikeyBus bulk mismatch - length byte is 0x%x, should be 0x%x", length, actualLength - 1)));
        return result;
    }
    
    mb_bulk_payload_tx(dev, moduleNumber, funcID, block.getData() + 1, length);
    
    return Result::ok();
}

//--------------------------------------------------------------------------
//
// Main MikeyBus test function
//
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
    String const bulkString("bulk");
    uint8 moduleNumber = (uint8)(element->getIntAttribute("module") & 1);
    String moduleName(String(slotNames[moduleNumber]) + " module");
    
    forEachXmlChildElement (*element, child)
    {
		uint8 page = (uint8) child->getStringAttribute(pageString).getHexValue32();
		uint8 address = (uint8) child->getStringAttribute(addressString).getHexValue32();
        uint8 value = (uint8) child->getStringAttribute(valueString).getHexValue32();
        
        if (child->hasTagName (readString))
        {
            uint8 readValue;
            Result result( dev->readMikey(moduleNumber, page, address, readValue));
            if (result.failed())
            {
                msg = "*** Could not read MikeyBus register for " + moduleName + " - FAIL";
                errorCodes.add(ErrorCodes::MIKEY_BUS, moduleNumber);
                return false;
            }
            
            if (value != readValue)
            {
                msg = "*** MikeyBus read test mismatch for " + moduleName + " page " + String::toHexString(page) + ", address " + String::toHexString(address) + " value:" + String::toHexString(readValue) + " - FAIL";
				errorCodes.add(ErrorCodes::MIKEY_BUS, moduleNumber);
                return false;
            }
            
            continue;
        }
        
        if (child->hasTagName(writeString))
        {
            Result result( dev->writeMikey(moduleNumber, page, address, value));
            if (result.failed())
            {
                msg = "*** Could not write MikeyBus register " + moduleName + " - FAIL";
                errorCodes.add(ErrorCodes::MIKEY_BUS, moduleNumber);
                return false;
            }
            
            Thread::sleep(10);
            
            continue;
        }
        
        if (child->hasTagName(bulkString))
        {
            Result result(MikeyBusBulkWrite(child, dev, moduleNumber));
            if (result.failed())
            {
                msg = "*** Could not perform MikeyBus bulk write for " + moduleName + " - FAIL";
                errorCodes.add(ErrorCodes::MIKEY_BUS, moduleNumber);
                return false;
            }
            
            Thread::sleep(150);
        }
    }

    msg = "MikeyBus " + moduleName + " register test - PASS";
    
    return true;
}

#endif