#include "../base.h"
#include "ehw.h"
#include "ehwlist.h"
#include "AcousticIO.h"
#include "../calibration/CalibrationData.h"

ehw::ehw(IOUSBDeviceInterface** deviceInterface_) :
deviceInterface(deviceInterface_)
{
    uint16 productID = 0;
    
    (*deviceInterface_)->GetDeviceReleaseNumber(deviceInterface_,&firmwareVersion);
    (*deviceInterface_)->GetDeviceProduct(deviceInterface_,&productID);
    
    uint8 moduleTypes = getModuleTypes();
    switch (productID)
    {
        case hwcaps::ACOUSTICIO_PRODUCT_ID:
            description = new DescriptionAIO(moduleTypes);
            break;
            
        case hwcaps::ACOUSTICIO_M1_PRODUCT_ID:
            moduleTypes = (ACOUSTICIO_MIKEYBUS_MODULE << 4) | ACOUSTICIO_ANALOG_MODULE;
            description = new DescriptionAM1(moduleTypes);
            break;
            
        case hwcaps::ACOUSTICIO_M2_PRODUCT_ID:
            moduleTypes = (ACOUSTICIO_MIKEYBUS_MODULE << 4) | ACOUSTICIO_MIKEYBUS_MODULE;
            description = new DescriptionAM2(moduleTypes);
            break;
            
        default:
            description = new Description(moduleTypes);
            break;
    }
    
    _caps.init(productID);
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

uint8 ehw::getModuleTypes()
{
    uint8 moduleTypes = 0;
    if (firmwareVersion >= ACOUSTICIO_MODULE_TYPE_CONTROL_MIN_FIRMWARE_VERSION)
    {
        //
        // Use ACOUSTICIO_MODULE_TYPE_CONTROL to detect AIO-S or analog module
        //
        IOReturn status = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_MODULE_TYPE_CONTROL, 0, &moduleTypes, 1);
        Result result(createResult(status));
        if (result.failed())
            moduleTypes = 0;
    }
    else
    {
        //
        // Use ACOUSTICIO_MODULE_STATUS_CONTROL for older firmware
        //
        uint8 moduleStatus = 0;
        IOReturn status = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_MODULE_STATUS_CONTROL, 0, &moduleStatus, 1);
        Result result(createResult(status));
        
        if (result.failed())
        {
            moduleTypes = 0;
        }
        else
        {
            //
            // Check the low 2 bits of moduleStatus - the module is present
            // if the bit is low
            //
            if (moduleStatus & 2)
            {
                moduleTypes = ACOUSTICIO_MODULE_NOT_PRESENT;
            }
            else
            {
                moduleTypes = ACOUSTICIO_ANALOG_MODULE;
            }
            
            if (moduleStatus & 1)
            {
                moduleTypes |= ACOUSTICIO_MODULE_NOT_PRESENT << 4;
            }
            else
            {
                moduleTypes |= ACOUSTICIO_ANALOG_MODULE << 4;
            }
        }
    }
    return moduleTypes;
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

Result ehw::setMicGain(uint8 channel, uint8 gain)
{
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_MIC_GAIN_CONTROL, channel, &gain, 1);
    
    if (kIOReturnSuccess == rc)
    {
        return Result::ok();
    }
    
    String error("Failed to set mic gain");
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


Result ehw::setAmpGain(uint8 channel, uint8 gain)
{
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_AMP_GAIN_CONTROL, channel, &gain, 1);
    
    if (kIOReturnSuccess == rc)
    {
        return Result::ok();
    }
    
    String error("Failed to set amp gain");
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
    int const module = 0;
    return setAIOSReferenceVoltage(module, enabled);
}

Result ehw::setAIOSReferenceVoltage(int const module, bool const enabled)
{
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

Result ehw::clearRAMCalibrationData()
{
    AIOSCalibrationData data;
    
    return setCalibrationData(&data.data);
}

Result ehw::setCalibrationData(AcousticIOCalibrationData const * const data)
{
    IOReturn rc = setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_CALIBRATION_DATA_CONTROL,
                             0, (uint8*)data, sizeof(AcousticIOCalibrationData));
    if (kIOReturnSuccess == rc)
        return Result::ok();
    
    String error("Failed to set RAM calibration data ");
    error += " - error " + String::toHexString((int32)rc);
    return Result::fail(error);
}

Result ehw::getCalibrationData(AcousticIOCalibrationData * const data)
{
    IOReturn rc = getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_CALIBRATION_DATA_CONTROL, 0, (uint8*)data, sizeof(AcousticIOCalibrationData));
    if (kIOReturnSuccess == rc)
        return Result::ok();
    
    String error("Failed to read RAM calibration data");
    error += " - error " + String::toHexString(rc);
    return Result::fail(error);
}

static uint8 getUnitForModule(uint8 const module)
{
    switch (module & 1)
    {
        case 0:
            return MIKEY_EXTENSION_UNIT0;
            
        case 1:
            return MIKEY_EXTENSION_UNIT1;
    }
    
    return 0;
}

Result ehw::readMikey(uint8 module, uint8 page, uint8 address, uint8 &value)
{
    uint8 unit = getUnitForModule(module);
    IOReturn rc = getRequest(unit, page, address, &value, sizeof(value));
    if (kIOReturnSuccess == rc)
    {
        //DBG(String::formatted("readMikey module:%d  page:%04x  address:%04x  data:%02x",module,page,address,value));
        return Result::ok();
    }

    value = 0;
    return Result::fail("Failed to read MikeyBus register");
}

Result ehw::writeMikey(uint8 module, uint8 page, uint8 address, uint8 value)
{
    uint8 unit = getUnitForModule(module);
    IOReturn rc = setRequest(unit, page, address, &value, sizeof(value));
    if (kIOReturnSuccess == rc)
    {
        //DBG(String::formatted("writeMikey module:%d  page:%04x  address:%04x  data:%02x",module,page,address,value));
        return Result::ok();
    }
    return Result::fail("Failed to write MikeyBus register");
}

