#include "../base.h"
#include "CalibrationUnit.h"
#include "CalibrationManagerV2.h"

CalibrationUnit::AIOPnpHandler::AIOPnpHandler(CalibrationUnit& calibrationUnit_, USBDevices* aioUSBDevices_) :
	calibrationUnit(calibrationUnit_),
	aioUSBDevices(aioUSBDevices_)
{
	aioUSBDevices_->broadcaster.addChangeListener(this);
}

CalibrationUnit::AIOPnpHandler::~AIOPnpHandler()
{
	aioUSBDevices->broadcaster.removeChangeListener(this);
}

void CalibrationUnit::AIOPnpHandler::changeListenerCallback(ChangeBroadcaster* source)
{
	DBG("CalibrationUnit::changeListenerCallback");

	ReferenceCountedObjectPtr<USBDevice> currentAIO( aioUSBDevices->getCurrentDevice() );
	calibrationUnit.setAIOUSBDevice(currentAIO);
}

