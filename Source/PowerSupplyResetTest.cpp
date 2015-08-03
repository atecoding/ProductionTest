#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"
#include "xml.h"
#include "Test.h"

const double relativeLevelThresholdDecibels = -95.0;

bool static getXmlParameters(XmlElement const *element, ehw* dev, String &msg, int &firstChannel, int &lastChannel)
{
    String const firstChannelTag("first_channel");
    firstChannel = -1;
    if (false == getIntValue( element, firstChannelTag, firstChannel))
    {
        msg += "Missing tag " + firstChannelTag;
        return false;
    }
    if (firstChannel < 0 || firstChannel >= dev->getcaps()->numbusin())
    {
        msg += firstChannelTag + " value " + String(firstChannel) + " out of range";
        return false;
    }
    
    String const lastChannelTag("last_channel");
    lastChannel = -1;
    if (false == getIntValue( element, lastChannelTag, lastChannel))
    {
        msg += "Missing tag " + lastChannelTag;
        return false;
    }
    if (lastChannel < 0 || lastChannel >= dev->getcaps()->numbusin())
    {
         msg += lastChannelTag + " value " + String(lastChannel) + " out of range";
        return false;
    }
    
    return true;
}


bool RunPowerSupplyResetTest(XmlElement const *element,
                       ehw *dev,
                       String &msg,
                       int &displayedInput,
                       AIOTestAdapter &testAdapter,
                       Content *content,
                       ErrorCodes &errorCodes,
                       ValueTree &unitTree)
{
    int firstChannel,lastChannel;
    
    displayedInput = -1;
    
    msg = newLine + "Power supply reset check: " + newLine;
    
    if (false == getXmlParameters(element, dev, msg, firstChannel, lastChannel))
    {
        msg += " FAIL";
        return false;
    }
        
    //
    // if no Mic Supply errors AND all TEDS fails AND all 10x inputs are < 95dB then "+/-15V Analog Supply Error"
    //
    
    //
    // look for constant current voltage or current failures
    //
    int numCodes = errorCodes.getCount();
    bool micSupplyError = false;
    
    for (int i = 0; i < numCodes; ++i)
    {
        uint32 code = errorCodes.getCode(i);
        uint32 category = code & 0xf;
        int channel = (code >> 4) & 0xf;
        
        switch (category)
        {
            case ErrorCodes::MIC_SUPPLY_VOLTAGE:
            case ErrorCodes::MIC_SUPPLY_CURRENT:
                if (firstChannel <= channel && channel <= lastChannel)
                {
                    micSupplyError = true;
                }
                break;
        }
    }
    
    if (micSupplyError)
    {
        msg += "    No mic supply current or voltage errors" + newLine;
    }
    else
    {
        msg += "   Found at least one mic supply current or voltage error" + newLine;
    }
    
    //
    // Look for TEDS failures
    //
    BigInteger tedsErrorMask;
    
    for (int i = 0; i < numCodes; ++i)
    {
        uint32 code = errorCodes.getCode(i);
        uint32 category = code & 0xf;
        int channel = (code >> 4) & 0xf;
        
        if (ErrorCodes::TEDS == category)
        {
            tedsErrorMask.setBit(channel - 1);
            DBG("tedsErrorMask " << tedsErrorMask.toString(16));
        }
    }
    
    BigInteger channelMask;
    channelMask.setRange(firstChannel, lastChannel - firstChannel + 1, true);
    
    DBG("channelMask " << channelMask.toString(16));
    
    bool allTedsFailed = channelMask == tedsErrorMask;
    if (allTedsFailed)
    {
        msg += "   All TEDS failed";
    }
    else if (tedsErrorMask.isZero())
    {
        msg += "   No TEDS errors";
    }
    else
    {
        msg += "   Partial TEDS errors";
    }
    
    String channelsString(" for channels " + String(firstChannel + 1) + " through " + String(lastChannel + 1) + newLine);
    msg += channelsString;
    
    //
    // Get the 10x relative level values from the unitTree
    //
    ValueTree relativeLevelTree(unitTree.getChildWithName(RelativeLevelTest::relativeLevelResults));
    BigInteger relativeLevelsMask;
    
    for (int channel = firstChannel; channel <= lastChannel; ++channel)
    {
        Identifier id("channel" + String(channel));
        double db = relativeLevelTree[id];
        if (db <= relativeLevelThresholdDecibels)
        {
            relativeLevelsMask.setBit(channel);
        }
    }
    
    bool allRelativeLevelsTooLow = channelMask == relativeLevelsMask;
    if (allRelativeLevelsTooLow)
    {
        msg += "   All relative levels too low";
    }
    else if (relativeLevelsMask.isZero())
    {
        msg += "   No relative levels too low";
    }
    else
    {
        msg += "   Partial relative levels too low";
    }
    
    msg += channelsString;
    
    //
    // Pass or fail?
    //
    if (false == micSupplyError &&
        true == allTedsFailed &&
        true == allRelativeLevelsTooLow)
    {
        //
        // This unit likely has a power supply reset problem
        //
        msg += newLine + "====== Please set aside for further testing" + newLine +
        "====== Do not ship to customer" + newLine + newLine;
        msg += "FAIL" + newLine;
        return false;
    }

    
    //
    // Seems OK
    //
    msg += "PASS";
    return true;
    
#if 0
    
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
    if (expectedModuleType < ACOUSTICIO_MODULE_NOT_PRESENT || expectedModuleType > ACOUSTICIO_SPKRMON_MODULE)
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
#endif
}

#endif
