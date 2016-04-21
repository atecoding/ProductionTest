#include "../base.h"
#include "CalibrationUnit.h"

CalibrationUnit::AIOPnpHandler::AIOPnpHandler(CalibrationUnit& calibrationUnit_, USBDevices* aioUSBDevices_) :
	calibrationUnit(calibrationUnit_),
	aioUSBDevices(aioUSBDevices_)
{
}

CalibrationUnit::AIOPnpHandler::~AIOPnpHandler()
{
}

void CalibrationUnit::AIOPnpHandler::changeListenerCallback(ChangeBroadcaster* source)
{
}


