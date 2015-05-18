#include "../base.h"
#include "ehw.h"
#include "ehwlist.h"
#include "AcousticIO.h"

ehw::ehw(IOUSBDeviceInterface** deviceInterface_) :
deviceInterface(deviceInterface_)
{
    uint16 productID = 0;
    
    (*deviceInterface_)->GetDeviceProduct(deviceInterface_,&productID);
    switch (productID)
    {
        case hwcaps::ACOUSTICIO_PRODUCT_ID:
            description = new DescriptionAIO;
            break;
            
        case hwcaps::ACOUSTICIO_MB_PRODUCT_ID:
            description = new DescriptionAMB;
            break;
            
        default:
            description = new Description;
            break;
    }
    
    _caps.init(productID);
    
    (*deviceInterface_)->GetDeviceReleaseNumber(deviceInterface_,&firmwareVersion);
}

ehw::~ehw()
{
    (*deviceInterface)->Release(deviceInterface);
}

int ehw::OpenDriver()
{
    return 0;
}

void ehw::CloseDriver()
{
}

uint64 ehw::GetSerialNumber()
{
    jassertfalse;
    return 0;
}

uint32 ehw::getFirmwareVersion() const
{
    return firmwareVersion;
}

String ehw::getFirmwareVersionString() const
{
    return String::toHexString(firmwareVersion);
}

const static String error("Error ");
Result ehw::createResult(IOReturn const status)
{
    if (kIOReturnSuccess == status)
        return Result::ok();
    return Result:: fail(error + String::toHexString((int)status));
}

IOReturn ehw::setRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length)
{
    IOUSBDevRequest req;
    req.bmRequestType = USB_REQUEST_TO_DEV;
    req.bRequest = CUR;
    req.wValue = (type << 8) | channel;
    req.wIndex = uint16(unit) << 8;
    req.wLength = length;
    req.pData = data;
    req.wLenDone = 0;
    
    return (*deviceInterface)->DeviceRequest(deviceInterface, &req);
}

IOReturn ehw::getRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length)
{
    IOUSBDevRequest req;
    req.bmRequestType = USB_REQUEST_FROM_DEV;
    req.bRequest = CUR;
    req.wValue = (type << 8) | channel;
    req.wIndex = uint16(unit) << 8;
    req.wLength = length;
    req.pData = data;
    req.wLenDone = 0;
    
    return (*deviceInterface)->DeviceRequest(deviceInterface, &req);
}

Result ehw::setMicGain(XmlElement const *element)
{
    uint8 channel;
    uint8 gain;
    int attribute;
    
    if (false == element->hasAttribute("input"))
    {
        Result error(Result::fail("AIO_set_mic_gain missing 'input' setting"));
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         error.getErrorMessage(), false);
        return error;
    }
    
    attribute = element->getIntAttribute("input", -1);
    if (attribute < 0 || attribute > 7)
    {
        Result error(Result::fail("AIO_set_mic_gain - input " + String(attribute) + " out of range"));
        
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         error.getErrorMessage(), false);
        return error;
    }
    channel = (uint8)attribute;
    
    
    if (false == element->hasAttribute("gain"))
    {
        Result error(Result::fail("AIO_set_mic_gain missing 'gain' setting"));
        
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         error.getErrorMessage(), false);
        return error;
    }
    
    attribute = element->getIntAttribute("gain", -1);
    switch (attribute)
    {
        case 1:
        case 10:
        case 100:
            break;
        default:
            AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                             "AIO_set_mic_gain - gain " + String(attribute) + " is invalid", false);
            break;
    }
    gain = (uint8)attribute;
    
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_MIC_GAIN_CONTROL, channel, &gain, 1);
    if (kIOReturnSuccess == rc)
    {
        uint8 temp;
        rc = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_MIC_GAIN_CONTROL, channel, &temp, 1);
        if (kIOReturnSuccess == rc)
        {
            if (temp == gain)
            {
                return Result::ok();
            }
            else
            {
                String error("Failed to verify mic gain for input " + String((int)channel));
                error += " - expected " + String::toHexString(gain) + ", read " + String::toHexString(temp);
                return Result::fail(error);
            }
        }
        else
        {
            String error("Failed to get mic gain for input " + String((int)channel));
            error += " - error " + String::toHexString(rc);
            return Result::fail(error);
        }
    }

    String error("Failed to set mic gain for input " + String((int)channel));
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

Result ehw::setAmpGain(XmlElement const *element)
{
    uint8 channel;
    uint8 gain;
    int attribute;
    
    if (false == element->hasAttribute("output"))
    {
        Result error(Result::fail("AIO_set_amp_gain missing 'output' setting"));
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         error.getErrorMessage(), false);
        return error;
    }
    
    attribute = element->getIntAttribute("output", -1);
    if (attribute < 0 || attribute > 3)
    {
        Result error(Result::fail("AIO_set_amp_gain - output " + String(attribute) + " out of range"));
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         error.getErrorMessage(), false);
        return error;
    }
    channel = (uint8)attribute;
    
    
    if (false == element->hasAttribute("gain"))
    {
        Result error(Result::fail("AIO_set_amp_gain missing 'gain' setting"));
        
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_amp_gain missing 'gain' setting", false);
        return error;
    }
    
    attribute = element->getIntAttribute("gain", -1);
    if (1 == attribute)
        attribute = 26;
    if (10 == attribute)
        attribute = 255;
    switch (attribute)
    {
        case 26:
        case 255:
            break;
        default:
            AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                             "AIO_set_amp_gain - gain " + String(attribute) + " is invalid", false);
            break;
    }
    gain = (uint8)attribute;
    
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_AMP_GAIN_CONTROL, channel, &gain, 1);
    if (kIOReturnSuccess == rc)
    {
        uint8 temp;
        rc = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_AMP_GAIN_CONTROL, channel, &temp, 1);
        if (kIOReturnSuccess == rc)
        {
            if (temp == gain)
            {
                return Result::ok();
            }
            else
            {
                String error("Failed to verify amp gain for input " + String((int)channel));
                error += " - expected " + String::toHexString(gain) + ", read " + String::toHexString(temp);
                return Result::fail(error);
            }
        }
        else
        {
            String error("Failed to get amp gain for input " + String((int)channel));
            error += " - error " + String::toHexString(rc);
            return Result::fail(error);
        }
    }
    
    String error("Failed to set amp gain for input " + String((int)channel));
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

Result ehw::setConstantCurrent(XmlElement const *element)
{
    uint8 channel;
    uint8 enabled;
    int attribute;
    
    if (false == element->hasAttribute("input"))
    {
        Result error(Result::fail("AIO_set_constant_current missing 'input' setting"));
        
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_constant_current missing 'input' setting", false);
        return error;
    }
    
    attribute = element->getIntAttribute("input", -1);
    if (attribute < 0 || attribute > 7)
    {
        Result error(Result::fail("AIO_set_constant_current - input " + String(attribute) + " out of range"));
        
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_constant_current - input " + String(attribute) + " out of range", false);
        return error;
    }
    channel = (uint8)attribute;
    
    
    if (false == element->hasAttribute("enabled"))
    {
        Result error(Result::fail("AIO_set_constant_current missing 'enabled' setting"));

        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_constant_current missing 'enabled' setting", false);
        return error;
    }
    
    enabled = element->getIntAttribute("enabled", 0) != 0;
    return setConstantCurrent(channel, enabled);
}

Result ehw::setConstantCurrent(uint8 const input, uint8 const enabled)
{
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_CONSTANT_CURRENT_CONTROL, input, (uint8 *)&enabled, 1);
    
    if (kIOReturnSuccess == rc)
    {
        uint8 temp;
        rc = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_CONSTANT_CURRENT_CONTROL, input, &temp, 1);
        if (kIOReturnSuccess == rc)
        {
            if (temp == enabled)
            {
                return Result::ok();
            }
            else
            {
                String error("Failed to verify CC for input " + String((int)input));
                error += " - expected " + String::toHexString(enabled) + ", read " + String::toHexString(temp);
                return Result::fail(error);
            }
        }
        else
        {
            String error("Failed to get CC for input " + String((int)input));
            error += " - error " + String::toHexString(rc);
            return Result::fail(error);
        }
    }
    
    String error("Failed to set CC for input " + String((int)input));
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

Result ehw::setAIOSReferenceVoltage(XmlElement const *element)
{
    if (false == element->hasAttribute("enabled"))
    {
        Result error(Result::fail("AIOS_set_reference_voltage missing 'enabled' setting"));
        
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         error.getErrorMessage(), false);
        return error;
    }
    
    bool enabled = element->getIntAttribute("enabled", 0) != 0;
    return setAIOSReferenceVoltage(enabled);
}

Result ehw::setAIOSReferenceVoltage(bool const enabled)
{
    uint8 const module = 0; // assume AIO center module for now
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_CALIBRATION_VOLTAGE_CONTROL, module, (uint8 *)&enabled, 1);
    
    if (kIOReturnSuccess == rc)
    {
        return Result::ok();
    }
    
    String error("Failed to set reference voltage");
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}


Result ehw::readTEDSData(uint8 const input, uint8* data, size_t dataBufferBytes)
{
    IOReturn rc = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_TEDS_DATA_CONTROL, input, data, dataBufferBytes);
    if (kIOReturnSuccess == rc)
        return Result::ok();
    
    String error("Failed to read TEDS for input " + String((int)input));
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

Result ehw::readFlashBlock(uint8 const block, uint8 * const buffer, size_t const bufferBytes)
{
    IOReturn rc = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_FLASH_BLOCK_CONTROL, block, (uint8*)buffer, bufferBytes);
    if (kIOReturnSuccess == rc)
        return Result::ok();
    
    String error("Failed to read flash for block " + String((int)block));
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

Result ehw::writeFlashBlock(uint8 const block, uint8 const * const buffer, size_t const bufferBytes)
{
    
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_FLASH_BLOCK_CONTROL, block, (uint8*)buffer, bufferBytes);
    if (kIOReturnSuccess == rc)
        return Result::ok();
    
    String error("Failed to write flash for block " + String((int)block));
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

