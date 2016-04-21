#pragma once

#include "../AIOTestAdapter.h"
#include "CalibrationDataV2.h"
#include "CalibrationProcedure.h"
#include "../USBDevice.h"
#include "../USBDevices.h"

class CalibrationManagerV2;

class CalibrationUnit
{
public:
    CalibrationUnit(USBDevices* aioUSBDevices_, CalibrationManagerV2& calibrationManager_);
    ~CalibrationUnit();
    
    CalibrationProcedure* const getProcedure() const
    {
        return procedure;
    }
    
#if JUCE_MAC
    String const getCoreAudioName() const;
#endif
    
    Result prepareUnitForCalibration();
    Result createModuleProcedure();
    Result runModuleProcedureStage();
    Result analyzeRecording(AudioBuffer<float> recordBuffer);
    Result finishModuleProcedureStage();
    Result finishModuleCalibration();
    bool isModuleProcedureDone() const;
    bool isCalibrated() const;
    void cancelCalibration();
    
    Result resetRAMCalibrationData();
    Result eraseFlashCalibrationData();
    String getHistory();

	void setDevice(ReferenceCountedObjectPtr<USBDevice> aioUSBDevice_);

	ReferenceCountedObjectPtr<USBDevice> aioUSBDevice;
    
    CalibrationDataV2 originalCalibrationData;
    CalibrationDataV2 calibrationData;

private:
    friend class CalibrationProcedureAIOA;
    friend class CalibrationProcedureAIOS;
    
    Result getFlashIndex(AcousticIOCalibrationIndex &calibrationIndex, bool &checksumOK);
    Result writeActiveCalibrationToFlash();
    
    int moduleNumber;
    int numModulesToCalibrate;

    ScopedPointer<AIOTestAdapter> testAdapter;
    CalibrationManagerV2& calibrationManager;
    ScopedPointer<CalibrationProcedure> procedure;
    static const Result invalidProcedureResult;

	struct AIOPnpHandler : public ChangeListener
	{
		AIOPnpHandler(CalibrationUnit& calibrationUnit_, USBDevices* aioUSBDevices_);
		~AIOPnpHandler();

		CalibrationUnit& calibrationUnit;
		USBDevices* aioUSBDevices;

		virtual void changeListenerCallback(ChangeBroadcaster* source) override;
	} pnpHandler;
};
