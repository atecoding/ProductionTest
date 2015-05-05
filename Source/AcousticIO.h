#ifndef _ACOUSTICIO_COMMON_H_
#define _ACOUSTICIO_COMMON_H_

#define ACOUSTICIO_EXTENSION_UNIT 0xa1


//======================================================================================
//
// USB controls
//
//======================================================================================

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
	ACOUSTICIO_PEAK_METERS_CONTROL,		// read-only

	//
	// These controls are present in firmware version 0x70 or later
	//
	ACOUSTICIO_CALIBRATION_VOLTAGE_CONTROL,
	ACOUSTICIO_CALIBRATION_DATA_CONTROL,
	ACOUSTICIO_FLASH_BLOCK_CONTROL,
	ACOUSTICIO_MODULE_TYPE_CONTROL
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
// ACOUSTICIO_MODULE_STATUS_CONTROL - (older firmware before version 0x70 only)
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
enum
{
	ACOUSTICIO_AMP_GAIN_1V_P2P = 26,
	ACOUSTICIO_AMP_GAIN_10V_P2P = 255
};


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


//
// ACOUSTICIO_CALIBRATION_VOLTAGE_CONTROL
//
// channel number: 0 for center module, 1 for outer module
//
// GET request: returns a single byte containing the current status of the calibration voltage
//		(nonzero == voltage on, zero == voltage off)
// SET request: accepts a single byte containing the setting for the calibration voltage
//		(nonzero == voltage on, zero == voltage off)
//

#define ACOUSTICIO_CALIBRATION_VOLTAGE_CONTROL_MIN_FIRMWARE_VERSION 0x0070


//
// ACOUSTICIO_CALIBRATION_DATA_CONTROL
//
// channel number is ignored
//
// GET request: returns a data structure containing the current calibration data
// SET request: accepts a data structure containing new calibration data
//
typedef struct _AcousticIOCalibrationData
{
	unsigned int time;		// minutes since midnight April 1st, 2015
	unsigned short inputGains[8];
	unsigned short outputGains[4];
	unsigned int checksum;
} AcousticIOCalibrationData;

#define ACOUSTICIO_CALIBRATION_DATA_CONTROL_MIN_FIRMWARE_VERSION 0x0070

#define ACOUSTICIO_CRC32_POLYNOMIAL 0xEDB88320


//
// ACOUSTICIO_FLASH_BLOCK_CONTROL
//
// channel number is the flash block number.  Block number is offset / ACOUSTICIO_FLASH_BLOCK_BYTES
//
// For example, to access flash address 0x100, block number is 0x100 / 32 == 8
//
// GET request: returns one flash block
// SET request: accepts one flash block
//

#define ACOUSTICIO_FLASH_BLOCK_CONTROL_MIN_FIRMWARE_VERSION 0x0070


//
// ACOUSTICIO_MODULE_TYPE_CONTROL (requires firmware version 0x70 or later)
//
// GET request only, channel number is ignored
//
// Returns a single byte:
//    Bits 7-4: Outer module type   2 = AIO-S module, 1 = Analog AIO module, 0 = not present
//    Bits 3-0: Center module type   2 = AIO-S module, 1 = Analog AIO module, 0 = not present
//
enum
{
	ACOUSTICIO_MODULE_NOT_PRESENT = 0,
	ACOUSTICIO_ANALOG_MODULE,
	ACOUSTICIO_SPKRMON_MODULE
};

#define ACOUSTICIO_MODULE_TYPE_CONTROL_MIN_FIRMWARE_VERSION 0x0070


//======================================================================================
//
// Flash memory map
//
//======================================================================================

enum
{
	ACOUSTICIO_FLASH_TOTAL_DATA_BYTES = 4096,
	ACOUSTICIO_FLASH_SECTOR_BYTES = 4096,
	ACOUSTICIO_FLASH_BLOCK_BYTES = 32,
	ACOUSTICIO_NUM_FLASH_BLOCKS = ACOUSTICIO_FLASH_TOTAL_DATA_BYTES / ACOUSTICIO_FLASH_BLOCK_BYTES,
	ACOUSTICIO_NUM_CALIBRATION_DATA_ENTRIES = (ACOUSTICIO_FLASH_TOTAL_DATA_BYTES / sizeof(AcousticIOCalibrationData)) - 1,
	ACOUSTICIO_CALIBRATION_INDEX_BLOCK = ACOUSTICIO_NUM_FLASH_BLOCKS - 1
};

typedef struct _AcousticIOCalibrationIndex
{
	unsigned int numCalibrations;
	unsigned int reserved[6];	// padding to fill one flash block; should be zero
	unsigned int checksum;
} AcousticIOCalibrationIndex;


//======================================================================================
//
// AIO-S channel numbers for integrated speaker monitor
//
// AIO-S module should be in the center module slot
//
//======================================================================================

#define AIOS_VOLTAGE_INPUT_CHANNEL 2
#define AIOS_CURRENT_INPUT_CHANNEL 3
#define AIOS_VOLTAGE_OUTPUT_CHANNEL 0

#endif
