#if ACOUSTICIO_BUILD
#include "base.h"
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"



class FlashMemoryTask : public ThreadWithProgressWindow
{
public:
    FlashMemoryTask(ehw *dev_) :
    ThreadWithProgressWindow("Testing flash memory...",
                             true,
                             true),
    result(Result::ok()),
    dev(dev_)
    {
    }
    
    void run()
    {
        flashTestRepeatedByte(0x55);
        if (result.wasOk())
        {
            flashTestRepeatedByte(0xaa);
            if (result.wasOk())
            {
                flashTestRepeatedByte(0x00);
                if (result.wasOk())
                {
                    flashTestRepeatedByte(0xff);
                }
            }
        }
    }
    
    Result result;
    
protected:
    ehw* dev;
    
    void flashTestRepeatedByte(uint8 byte)
    {
		uint8 saveBuffer[ACOUSTICIO_FLASH_BLOCK_BYTES]; 
		uint8 writeBuffer[ACOUSTICIO_FLASH_BLOCK_BYTES];
        uint8 readBuffer[ACOUSTICIO_FLASH_BLOCK_BYTES];
        
		result = dev->readFlashBlock(0, saveBuffer, sizeof(saveBuffer));
		if (result.failed())
			return;

		memset(writeBuffer, byte, sizeof(writeBuffer));
        
        setStatusMessage("Writing repeated " + String::toHexString(byte));
        
        result = dev->writeFlashBlock(0, writeBuffer, sizeof(writeBuffer));
        if (result.failed())
            return;
            
        zerostruct(readBuffer);
        
        setStatusMessage("Verifying repeated " + String::toHexString(byte));
        
        result = dev->readFlashBlock(0, readBuffer, sizeof(readBuffer));
        if (result.failed())
            return;
            
       for (size_t index = 0; index < sizeof(readBuffer); ++index)
       {
           if (readBuffer[index] != byte)
           {
               result = Result::fail("Error verifying flash block");
               return;
           }
       }

	   result = dev->writeFlashBlock(0, saveBuffer, sizeof(saveBuffer));
	   if (result.failed())
		   return;
    }
};

bool RunFlashMemoryTest(XmlElement const *element,
                 ehw *dev,
                 String &msg,
                 int &displayedInput,
                 AIOTestAdapter &testAdapter,
                 Content *content,
                 ErrorCodes &errorCodes)
{
    msg = "Flash memory test: ";
    
    displayedInput = -1;
    
    FlashMemoryTask task(dev);
	if (task.runThread())
	{
		msg += " PASS";
		return true;
	}
        
    errorCodes.add(ErrorCodes::FLASH);
    msg += " FAIL";
    return false;
}
#endif
