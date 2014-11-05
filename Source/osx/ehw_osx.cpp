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
    
#if JUCE_DEBUG
    //(*deviceInterface)->GetDeviceReleaseNumber(deviceInterface_,&productID);
    //DBG("GetDeviceReleaseNumber " << String::toHexString(productID));
#endif
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

const static String error("Error ");
Result ehw::createResult(IOReturn const status)
{
    if (kIOReturnSuccess == status)
        return Result::ok();
    return Result:: fail(error + String::toHexString((int)status));
}

Result ehw::setRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length)
{
    IOUSBDevRequest req;
    req.bmRequestType = USB_REQUEST_TO_DEV;
    req.bRequest = CUR;
    req.wValue = (type << 8) | channel;
    req.wIndex = uint16(unit) << 8;
    req.wLength = length;
    req.pData = data;
    req.wLenDone = 0;
    
    IOReturn rc = (*deviceInterface)->DeviceRequest(deviceInterface, &req);
    return createResult(rc);
}

Result ehw::getRequest(uint8 unit, uint8 type, uint8 channel, uint8 *data, uint16 length)
{
    IOUSBDevRequest req;
    req.bmRequestType = USB_REQUEST_FROM_DEV;
    req.bRequest = CUR;
    req.wValue = (type << 8) | channel;
    req.wIndex = uint16(unit) << 8;
    req.wLength = length;
    req.pData = data;
    req.wLenDone = 0;
    
    IOReturn rc = (*deviceInterface)->DeviceRequest(deviceInterface, &req);
    return createResult(rc);
}



void ehw::setMicGain(XmlElement const *element)
{
    uint8 channel;
    uint8 gain;
    int attribute;
    
    if (false == element->hasAttribute("input"))
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_mic_gain missing 'input' setting", false);
        return;
    }
    
    attribute = element->getIntAttribute("input", -1);
    if (attribute < 0 || attribute > 7)
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_mic_gain - input " + String(attribute) + " out of range", false);
        return;
    }
    channel = (uint8)attribute;
    
    
    if (false == element->hasAttribute("gain"))
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_mic_gain missing 'gain' setting", false);
        return;
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
    
    setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_MIC_GAIN_CONTROL, channel, &gain, 1);
    
#if 0
    TUSBAUDIO_AudioControlRequestSet(handle,
                                     ACOUSTICIO_EXTENSION_UNIT,	// unit ID
                                     CUR,
                                     ACOUSTICIO_MIC_GAIN_CONTROL,
                                     channel,
                                     (void *)&gain,
                                     1,
                                     NULL,
                                     1000);
#endif
}

void ehw::setAmpGain(XmlElement const *element)
{
    uint8 channel;
    uint8 gain;
    int attribute;
    
    if (false == element->hasAttribute("output"))
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_amp_gain missing 'output' setting", false);
        return;
    }
    
    attribute = element->getIntAttribute("output", -1);
    if (attribute < 0 || attribute > 3)
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_amp_gain - output " + String(attribute) + " out of range", false);
        return;
    }
    channel = (uint8)attribute;
    
    
    if (false == element->hasAttribute("gain"))
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_amp_gain missing 'gain' setting", false);
        return;
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
    
    setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_AMP_GAIN_CONTROL, channel, &gain, 1);
    
#if 0
            
    TUSBAUDIO_AudioControlRequestSet(handle,
                                     ACOUSTICIO_EXTENSION_UNIT,	// unit ID
                                     CUR,
                                     ACOUSTICIO_AMP_GAIN_CONTROL,
                                     channel,
                                     (void *)&gain,
                                     1,
                                     NULL,
                                     1000);
#endif
}

void ehw::setConstantCurrent(XmlElement const *element)
{
    uint8 channel;
    uint8 enabled;
    int attribute;
    
    if (false == element->hasAttribute("input"))
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_constant_current missing 'input' setting", false);
        return;
    }
    
    attribute = element->getIntAttribute("input", -1);
    if (attribute < 0 || attribute > 7)
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_constant_current - input " + String(attribute) + " out of range", false);
        return;
    }
    channel = (uint8)attribute;
    
    
    if (false == element->hasAttribute("enabled"))
    {
        AlertWindow::showNativeDialogBox(JUCEApplication::getInstance()->getApplicationName(),
                                         "AIO_set_constant_current missing 'enabled' setting", false);
        return;
    }
    
    enabled = element->getIntAttribute("enabled", 0) != 0;
    setConstantCurrent(channel, enabled);
}

void ehw::setConstantCurrent(uint8 const input, uint8 const enabled)
{
    setRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_CONSTANT_CURRENT_CONTROL, input, (uint8 *)&enabled, 1);
}

int ehw::readTEDSData(uint8 const input, uint8* data, size_t dataBufferBytes)
{
    Result result(getRequest(ACOUSTICIO_EXTENSION_UNIT, ACOUSTICIO_TEDS_DATA_CONTROL, input, data, dataBufferBytes));
    
    if (result.wasOk())
        return 0;

    return -1;
}

