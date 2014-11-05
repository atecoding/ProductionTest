#ifndef _ACOUSTICIO_COMMON_H_
#define _ACOUSTICIO_COMMON_H_

#define ACOUSTICIO_EXTENSION_UNIT 0xa1

//
// Control selectors
//
enum
{
	ACOUSTICIO_TEST_CONTROL = 0x80,
	ACOUSTICIO_MODULE_STATUS_CONTROL,
	ACOUSTICIO_MIC_GAIN_CONTROL,
	ACOUSTICIO_CONSTANT_CURRENT_CONTROL,
	ACOUSTICIO_AMP_GAIN_CONTROL,
	ACOUSTICIO_TEDS_DATA_CONTROL,		// read-only
	ACOUSTICIO_PEAK_METERS_CONTROL		// read-only
};


//
// ACOUSTICIO_TEST_CONTROL
//
enum
{
	ACOUSTICIO_TEST_CONTROL__INTERFACE_TEST = 0,	// channel 0

	ACOUSTICIO_TEST_BUFFER_SIZE = 64
};


//
// ACOUSTICIO_MODULE_STATUS_CONTROL
//
// GET request only, channel number is ignored
//
// Returns a single byte:
//    Bit 0: PRESENT1n - Module 1 is present when low
//    Bit 1: PRESENT2n - Module 2 is present when low
//    Bit 2: AMPERR1n - Module 1 amplifier error (voltage, temp) when low
//    Bit 3: AMPERR2n - Module 2 amplifier error (voltage, temp) when low
//

//
// ACOUSTICIO_MIC_GAIN_CONTROL
//
// channel 0-7
// GET request: returns a single byte containing the gain for that channel
// SET request: accepts a single byte containing the gain for that channel
//
// Valid gain values are 1, 10, and 100
//

//
// ACOUSTICIO_CONSTANT_CURRENT_CONTROL
//
// channel 0-7
// GET request: returns a single byte containing the constant current setting
// SET request: accepts a single byte setting the constant current for that channel
//
// 1 for constant current enabled, 0 for disabled
//

//
// ACOUSTICIO_AMP_GAIN_CONTROL
//
// channel 0-3
// GET request: returns a single byte containing the gain for that channel
// SET request: accepts a single byte containing the gain for that channel
//
// Gain is 20 Log (value/255).  value = 255 sets 10Vp-p, value = 26 sets 1Vp-p
//

//
// ACOUSTICIO_TEDS_DATA_CONTROL
//
// channel 0-7
// GET request only
//
// Returns 128 bytes of TEDS data, if not present then returns all 0xFF
//
enum
{
	ACOUSTICIO_TEDS_DATA_BYTES = 128
};

//
// ACOUSTICIO_PEAK_METERS
//
// GET request only, channel number is ignored
//
// Returns 48 bytes of peak meter bytes
//
// Each peak meter is a 4 byte signed integer, 4 output meters followed by 8 input meters
//
enum
{
	ACOUSTICIO_PEAK_METER_BYTES = 48
};

#endif