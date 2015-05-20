#pragma once

#include "AcousticIO.h"

struct AIOSCalibrationData
{
	AIOSCalibrationData();

	void updateChecksum();
	void validateChecksum();
	void updateDate();
	bool isChecksumOK() const;
	String getDate() const;
	uint32 calculateChecksum() const;
	void reset();
	static String printValue(uint16 const value);
	String toString() const;
	AcousticIOCalibrationData data;

	static const Time epoch;

protected:
	bool checksumOK;
};

struct ExternalSpeakerMonitorCalibrationData
{
	ExternalSpeakerMonitorCalibrationData();

	float voltageInputGain;
	float currentInputGain;

	static String printValue(float const value);
};