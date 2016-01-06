#include "base.h"
#if ACOUSTICIO_BUILD
#include "AcousticIO.h"
#include "ehw.h"
#include "content.h"
#include "AIOTestAdapter.h"
#include "ErrorCodes.h"
#include "xml.h"
#include "calibration/crc32.h"


bool RunCalibrationVerificationTest(XmlElement const *element,
                        ehw *dev,
                        String &msg,
                        String &displayedChannel,
                        AIOTestAdapter &testAdapter,
                        Content *content,
                        ErrorCodes &errorCodes,
                        ValueTree &unitTree)
{
    msg = String("Calibration verification: ") + newLine;
    
    //
    // Read calibration index from flash
    //
    AcousticIOCalibrationIndex calibrationIndex;
    
    {
        Result result(dev->readFlashBlock(ACOUSTICIO_CALIBRATION_INDEX_BLOCK, (uint8 * const)&calibrationIndex, sizeof(calibrationIndex)));
        if (result.failed())
        {
            msg += " Could not read calibration index - FAIL";
            errorCodes.add(ErrorCodes::CALIBRATION_VERIFICATION);
            return false;
        }
        
        uint32 checksum = CRC32Block((uint32 const *)&calibrationIndex, sizeof(calibrationIndex) / sizeof(uint32) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);
        bool checksumOK = checksum == calibrationIndex.checksum;
        if (!checksumOK)
        {
            msg += String(" Calibration index invalid checksum") + newLine;
            msg += " numCalibrations: " + String(calibrationIndex.numCalibrations) + newLine;
            for (int i = 0; i < numElementsInArray(calibrationIndex.reserved); ++i)
            {
                msg += " " + String::toHexString((int32)calibrationIndex.reserved[i]);
            }
            msg += newLine + " checksum: " + String::toHexString((int32) calibrationIndex.checksum) + newLine;
            msg += " Calculated checksum: " + String::toHexString((int32)checksum);
            errorCodes.add(ErrorCodes::CALIBRATION_VERIFICATION);
            return false;
        }
        
        msg += String("Flash index OK") + newLine;
    }
    
    //
    // Read calibration data from flash
    //
    AIOSCalibrationData calibrationDataFlash;
    
    {
        int8 block = (uint8)(calibrationIndex.numCalibrations - 1) % ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES;
        
        Result result(dev->readFlashBlock(block, (uint8 * const)&calibrationDataFlash.data, sizeof(calibrationDataFlash.data)));
        if (result.failed())
        {
            msg += " Could not read calibration data from flash - FAIL";
            errorCodes.add(ErrorCodes::CALIBRATION_VERIFICATION);
            return false;
        }
        
        calibrationDataFlash.validateChecksum();
        if (false == calibrationDataFlash.isChecksumOK())
        {
            msg += String(" Flash calibration data invalid checksum - FAIL") + newLine;
            uint32 const *data = (uint32 const *)&calibrationDataFlash.data;
            for (int i = 0; i < sizeof(AcousticIOCalibrationData)/sizeof(uint32); ++i)
            {
                msg += String::toHexString((int32)data[i]) + " ";
            }
            msg += newLine;
            msg += " Calculated checksum: " + String::toHexString((int32)calibrationDataFlash.calculateChecksum());
            errorCodes.add(ErrorCodes::CALIBRATION_VERIFICATION);
            return false;
        }
        
        msg += String("Flash data OK") + newLine;
    }
    
    //
    // Read calibration data from RAM - should match the data from flash
    //
    AIOSCalibrationData calibrationDataRAM;
    
    {
        Result result(dev->getCalibrationData(&calibrationDataRAM.data));
        if (result.failed())
        {
            msg += " Could not read calibration data from RAM - FAIL";
            errorCodes.add(ErrorCodes::CALIBRATION_VERIFICATION);
            return false;
        }
        
        if (0 != memcmp(&calibrationDataRAM.data, &calibrationDataFlash.data, sizeof(AcousticIOCalibrationData)))
        {
            msg += String("RAM calibration data does not match flash calibration data - FAIL") + newLine;
            errorCodes.add(ErrorCodes::CALIBRATION_VERIFICATION);
            return false;
        }
        
        msg += String("RAM data OK") + newLine;
    }
    
    msg += calibrationDataRAM.toString();
    msg += String("PASS") + newLine;
    
    return true;
}

#endif
