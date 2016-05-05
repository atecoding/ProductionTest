#include "../base.h"
#include "CalibrationUnit.h"
#include "CalibrationManagerV2.h"
#include "CalibrationProcedureAIOA.h"
#include "CalibrationProcedureAIOS.h"
#include "CalibrationProcedureAIOM.h"
#include "crc32.h"

const Result CalibrationUnit::invalidProcedureResult(Result::fail("Invalid calibration procedure"));

CalibrationUnit::CalibrationUnit(USBDevices* aioUSBDevices_, CalibrationManagerV2& calibrationManager_) :
firstModule(-1),
moduleNumber(-1),
numModulesToCalibrate(0),
writeToFlashWhenDone(true),
calibrationManager(calibrationManager_),
pnpHandler(*this, aioUSBDevices_)
{
}

CalibrationUnit::~CalibrationUnit()
{
}


bool CalibrationUnit::isDone() const
{
    return moduleNumber >= (firstModule + numModulesToCalibrate);
}

bool CalibrationUnit::isModuleProcedureDone() const
{
    if (nullptr == procedure)
        return false;
    
    return procedure->isDone();
}


Result CalibrationUnit::prepareUnitForCalibration()
{
    //
    // Make sure there's an AIO
    //
    if (nullptr == aioUSBDevice)
        return Result::fail("AIO not detected");
    
    //
    // Make sure there's a test adapter
    //
    if (nullptr == testAdapter)
    {
        testAdapter = new AIOTestAdapter;
    }
    
    if (false == testAdapter->open())
        return Result::fail("AIO Test Adapter not found");
    
    //
    // Reset module number and module count
    //
    if (firstModule < 0)
    {
        firstModule = 0;
    }
    moduleNumber = firstModule;
    
    if (numModulesToCalibrate <= 0)
    {
        Description const * const descripton = aioUSBDevice->getDescription();
        numModulesToCalibrate = 0;
        for (int tempModuleNumber = 0; tempModuleNumber < Description::MAX_MODULES; ++tempModuleNumber)
        {
            AIOModule* module = descripton->getModuleObject(tempModuleNumber);
            if (module->supportsCalibration())
                numModulesToCalibrate++;
        }
    }
    
    return Result::ok();
}


Result CalibrationUnit::createModuleProcedure()
{
    DBG("CalibrationUnit::createModuleProcedure()  moduleNumber:" << moduleNumber);
    
    //
    // Create the procedure object
    //
    procedure = nullptr;
    
    if (nullptr == aioUSBDevice)
        return Result::fail("No AIO detected");
    
    AIOModule* module = aioUSBDevice->getDescription()->getModuleObject(moduleNumber);
    if (nullptr == module)
        return Result::fail("No AIO module object");
    
    DBG("Creating new procedure");
    
    switch (module->getType())
    {
        case ACOUSTICIO_ANALOG_MODULE:
            procedure = new CalibrationProcedureAIOA(this, module);
            break;
            
        case ACOUSTICIO_SPKRMON_MODULE:
            procedure = new CalibrationProcedureAIOS(this, module);
            break;
            
        case ACOUSTICIO_MIKEYBUS_MODULE:
            procedure = new CalibrationProcedureAIOM(this, module);
            break;
    }
    
    if (nullptr == procedure)
        return invalidProcedureResult;
    
    //
    // Zero the gains for the appropriate channels in the calibration data
    // and write to device RAM
    //
    calibrationData.reset(module->getModuleNumber());
    return aioUSBDevice->setCalibrationData(&calibrationData.data);
}


Result CalibrationUnit::analyzeRecording(AudioBuffer<float> recordBuffer)
{
    if (procedure)
    {
        return procedure->analyzeRecording(recordBuffer, calibrationData);
    }
    
    return invalidProcedureResult;
}



Result CalibrationUnit::finishModuleProcedureStage()
{
    DBG("CalibrationUnit::finishModuleProcedureStage()");
    
    if (nullptr == procedure)
        return invalidProcedureResult;
    
    calibrationData.updateDate();
    calibrationData.updateChecksum();
    Result result (aioUSBDevice->setCalibrationData(&calibrationData.data));
    if (result.wasOk())
    {
        procedure->finishStage();
    }
    
    return result;
}


Result CalibrationUnit::runModuleProcedureStage()
{
    DBG("CalibrationUnit::runModuleProcedureStage()");
    
    if (nullptr == procedure)
        return invalidProcedureResult;
    
    return procedure->prepareModuleForCalibration();
}


Result CalibrationUnit::finishModuleCalibration()
{
    if (nullptr == procedure)
        return invalidProcedureResult;
    
    Result result( procedure->finishModuleCalibration() );
    if (result.wasOk())
    {
            moduleNumber++;
    }
    
    if (isDone() && writeToFlashWhenDone)
    {
        result = writeActiveCalibrationToFlash();
    }
    
    return result;
}


Result CalibrationUnit::resetRAMCalibrationData()
{
    calibrationData.reset();
    calibrationData.updateDate();
    calibrationData.updateChecksum();
    return aioUSBDevice->setCalibrationData(&calibrationData.data);
}


Result CalibrationUnit::eraseFlashCalibrationData()
{
    Result result(resetRAMCalibrationData());
    if (result.wasOk())
    {
        result = writeActiveCalibrationToFlash();
    }
    
    return result;
}


String CalibrationUnit::getHistory()
{
    if (nullptr == aioUSBDevice)
        return String::empty;
    
    //
    // Get the index block
    //
    AcousticIOCalibrationIndex calibrationIndex;
    bool checksumOK;
    
    Result result(getFlashIndex(calibrationIndex,checksumOK));
    if (result.failed())
        return String::empty;
    
    if (false == checksumOK)
        return "No calibration history found";
    
    String text("Unit calibration count: " + String(calibrationIndex.numCalibrations) + newLine + newLine);
    
    uint32 count = calibrationIndex.numCalibrations;
    int8 block = (uint8)(calibrationIndex.numCalibrations - 1) % ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES;
    count = jmin(count, (uint32)ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES);
    while (count != 0)
    {
        CalibrationDataV2 entry;
        
        result = aioUSBDevice->readFlashBlock(block, (uint8 * const)&entry.data, sizeof(entry.data));
        if (result.failed())
            return String::empty;
        
        entry.validateChecksum();
        if (false == entry.isChecksumOK())
        {
            text += String("Found invalid checksum") + newLine;
            break;
        }
        if (0 == entry.data.time)
        {
            text += String("Found invalid time") + newLine;
            break;
        }
        
        text += entry.toString();
        text += newLine;
        
        block--;
        if (block < 0)
            block += ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES;
        count--;
    }
    
    return text;
}

void CalibrationUnit::setDevice(ReferenceCountedObjectPtr<USBDevice> aioUSBDevice_)
{
	originalCalibrationData.reset();
	calibrationData.reset();

	if (aioUSBDevice_)
	{
		aioUSBDevice_->getCalibrationData(&originalCalibrationData.data);
		originalCalibrationData.validateChecksum();
		calibrationData = originalCalibrationData;
	}

	aioUSBDevice = aioUSBDevice_;
	calibrationManager.aioChanged();
}

void CalibrationUnit::configure(CalibrationManagerConfiguration& configuration)
{
	originalCalibrationData.reset();
	calibrationData.reset();

	if (configuration.aioUSBDevice)
	{
		configuration.aioUSBDevice->getCalibrationData(&originalCalibrationData.data);
		originalCalibrationData.validateChecksum();
		calibrationData = originalCalibrationData;
	}
    
    firstModule = configuration.firstModule;
    numModulesToCalibrate = configuration.numModulesToCalibrate;
    writeToFlashWhenDone = configuration.writeToFlash;

	aioUSBDevice = configuration.aioUSBDevice;
	calibrationManager.aioChanged();
}

Result CalibrationUnit::getFlashIndex(AcousticIOCalibrationIndex &calibrationIndex, bool &checksumOK)
{
    zerostruct(calibrationIndex);
    
    Result result(aioUSBDevice->readFlashBlock(ACOUSTICIO_CALIBRATION_INDEX_BLOCK, (uint8 * const)&calibrationIndex, sizeof(calibrationIndex)));
    if (result.failed())
        return result;
    
    uint32 checksum = CRC32Block((uint32 const *)&calibrationIndex, sizeof(calibrationIndex) / sizeof(uint32) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);
    checksumOK = checksum == calibrationIndex.checksum;
    
    return Result::ok();
}

Result CalibrationUnit::writeActiveCalibrationToFlash()
{
    AcousticIOCalibrationIndex calibrationIndex;
    
    DBG("CalibrationUnit::writeActiveCalibrationToFlash()");
    
    uint32 numCalibrations;
    bool checksumOK;
    Result result(getFlashIndex(calibrationIndex, checksumOK));
    if (result.failed())
        return result;
    
    if (checksumOK && calibrationIndex.numCalibrations != 0)
    {
        calibrationIndex.numCalibrations++;
    }
    else
    {
        calibrationIndex.numCalibrations = 1;
    }
    
    //
    // Update the index block
    //
    numCalibrations = calibrationIndex.numCalibrations;
    zerostruct(calibrationIndex.reserved);
    calibrationIndex.checksum = CRC32Block((uint32 const *)&calibrationIndex, sizeof(calibrationIndex) / sizeof(uint32) - 1, ACOUSTICIO_CRC32_POLYNOMIAL);
    
    //
    // Write the calibration data to flash
    //
    uint8 calibrationBlock = (uint8)((numCalibrations - 1) % ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES);
    
    result = aioUSBDevice->writeFlashBlock(calibrationBlock, (uint8 * const)&calibrationData.data, sizeof(calibrationData.data));
    if (result.failed())
        return result;
    
    //
    // Write the updated index block to flash
    //
    result = aioUSBDevice->writeFlashBlock(ACOUSTICIO_CALIBRATION_INDEX_BLOCK, (uint8 * const)&calibrationIndex, sizeof(calibrationIndex));
    if (result.failed())
        return result;
    
    //aioUSBDevice->dumpFlash();
    
    return Result::ok();
}


#if JUCE_MAC
String const CalibrationUnit::getCoreAudioName() const
{
    if (aioUSBDevice)
    {
        return aioUSBDevice->getDescription()->getCoreAudioName();
    }
    return String::empty;
}
#endif


void CalibrationUnit::cancelCalibration()
{
    calibrationData = originalCalibrationData;
    
    if (aioUSBDevice)
    {
        aioUSBDevice->setCalibrationData(&calibrationData.data);
        
        if (procedure)
        {
            procedure->cancelCalibration();
        }
    }
    
    moduleNumber = 0;
}
