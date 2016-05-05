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
                    String &displayedChannel,
                    AIOTestAdapter &testAdapter,
                    Content *content,
                    ErrorCodes &errorCodes,
                    ValueTree &unitTree)
{
	uint32 min_version = 0;
	uint32 max_version = 0;
	uint32 deviceFirmwareVersion = dev->getFirmwareVersion();
	uint32 AIO_Revision = dev->GetBoxRev();

	min_version = element->getStringAttribute("min_version").getHexValue32();
	max_version = element->getStringAttribute("max_version").getHexValue32();
	if (0 == max_version)
		max_version = 0xffffffff;

	//
	// Support for old interface modules
	// This way we don't need separate scripts for them
	//
	if (AIO_Revision == ECHOAIO_INTERFACE_MODULE_REV1)
	{
		uint8 AIO_Type(dev->GetBoxModuleTypes());

		switch (AIO_Type)
		{
		case AIO_TYPE_MA:
			min_version = 0x1001;
			max_version = 0x17ff;
			break;
		case AIO_TYPE_MM:
			min_version = 0x2002;
			max_version = 0x27ff;
			break;
		default:
			min_version = 0x72;
			max_version = 0x7ff;
		}
	}

    msg = "Device firmware version " + String::toHexString((int32)deviceFirmwareVersion).getLastCharacters(4);
	if (deviceFirmwareVersion >= min_version && deviceFirmwareVersion <= max_version)
    {
        msg += " OK";
        return true;
    }
    
    errorCodes.add(ErrorCodes::FIRMWARE);
    msg += " FAIL";
    return false;
}
#endif
