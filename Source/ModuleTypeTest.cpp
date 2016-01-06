#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"
#include "xml.h"

const char *moduleTypeNames[] =
{
    "No module detected",
    "TEDS + Class-D AMP module",
    "AIO-S module",
    "MB module"
};

const char *slotNames[] =
{
    "Center slot",
    "Outer slot"
};

bool RunModuleTypeTest(XmlElement const *element,
                       ehw *dev,
                       String &msg,
                       String &displayedChannel,
                       AIOTestAdapter &testAdapter,
                       Content *content,
                       ErrorCodes &errorCodes,
                       ValueTree &unitTree)
{
    int slot;
    int expectedModuleType,detectedModuleType;
    
    msg = "Module type: ";
    
    //
    // Check for firmware version 70 or greater
    //
    uint32 firmwareVersion = dev->getFirmwareVersion();
    if (firmwareVersion < ACOUSTICIO_MODULE_TYPE_CONTROL_MIN_FIRMWARE_VERSION)
    {
        msg += "Minimum firmware version "+ String::toHexString((int)ACOUSTICIO_MODULE_TYPE_CONTROL_MIN_FIRMWARE_VERSION) + " required; cannot verify module type";

		//
		// Pass anyhow since we can't check this
		//
        return true;
    }
    
    //
    // Get parameters from XML
    //
    slot = -1;
    getIntValue(element, "slot", slot);
    if (slot < 0 || slot > 1)
    {
        msg += "Invalid slot number specified in script";
        return false;
    }
    
    expectedModuleType = -1;
    getIntValue(element, "type", expectedModuleType);
    if (expectedModuleType < ACOUSTICIO_MODULE_NOT_PRESENT || expectedModuleType > ACOUSTICIO_MIKEYBUS_MODULE)
    {
        msg += "Invalid module type specified in script";
        return false;
    }
    
    //
    // Check the expected type vs detected type
    //
    msg += String(slotNames[slot]) + ", ";
    
    detectedModuleType = dev->getDescription()->getModuleType(slot);
    if (detectedModuleType < 0 || detectedModuleType >= numElementsInArray(moduleTypeNames))
        msg += "Unknown module type";
    else
        msg += moduleTypeNames[detectedModuleType];
    
    if (detectedModuleType == expectedModuleType)
    {
        msg += " - PASS";
        return true;
    }
    
    errorCodes.add(ErrorCodes::MODULE_TYPE);
    msg += " - FAIL";
    return false;
}

#endif
