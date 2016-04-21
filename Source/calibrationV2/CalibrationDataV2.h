#pragma once

#include "../AcousticIO.h"

struct CalibrationDataV2
{
	CalibrationDataV2();

	void updateChecksum();
	void validateChecksum();
	void updateDate();
	bool isChecksumOK() const;
	String getDate() const;
	uint32 calculateChecksum() const;
	void reset();
    void reset(int const moduleNumber);
	static String printValue(uint16 const value);
	String toString() const;
    
	AcousticIOCalibrationData data;

	static const Time epoch;

protected:
	bool checksumOK;
};
