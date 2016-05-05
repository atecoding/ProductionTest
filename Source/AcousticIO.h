#ifndef _ACOUSTICIO_COMMON_H_
#define _ACOUSTICIO_COMMON_H_

#define ACOUSTICIO_EXTENSION_UNIT 0xa1
#define MIKEY_EXTENSION_UNIT0 0xa3  // m0
#define MIKEY_EXTENSION_UNIT1 0xa2  // m1

//======================================================================================
//
// USB interface module detection
//
// To detect the interface module version, read the bcdDevice field from the USB
// device descriptor.  Apply ECHOAIO_INTERFACE_MODULE_BCDDEVICE_MASK with a bitwise and; 
// the remaining bits indicate the interface module revision.  Anything below
// is a revision 1 interface module.
//
//======================================================================================

#define ECHOAIO_INTERFACE_MODULE_BCDDEVICE_MASK 0xf000

enum
{
	ECHOAIO_INTERFACE_MODULE_REV1 = 0x0000,	// Original interface module for 2014 and 2015
											// XMOS XCore XS1 processor
	ECHOAIO_INTERFACE_MODULE_REV2 = 0x3000	// Interface module introduced for 2016
											// Supports 192 kHz, full calibration, USB sync
											// XMOS Xcore200 XE216 processor
};


//======================================================================================
//
// Protected interface module
//
// If the bcdVersion value from unit has this bit set, then this unit is locked
//
//======================================================================================

enum
{
    ECHOAIO_LOCKED_INTERFACE_MODULE_BIT = 11,
    ECHOAIO_LOCKED_INTERFACE_MODULE_MASK = 1 << ECHOAIO_LOCKED_INTERFACE_MODULE_BIT
};


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
    ACOUSTICIO_MODULE_TYPE_CONTROL,

    //
    // Controls for interface module version 2 (200 series processor)
    //
    ACOUSTICIO_CLOCK_SOURCE_CONTROL,
    ACOUSTICIO_USB_CLOCK_RATE_CONTROL,

	//
	// Split output and input meter read commands
	//
	ACOUSTICIO_OUTPUT_PEAK_METERS_CONTROL = 0xa0,
	ACOUSTICIO_INPUT_PEAK_METERS_CONTROL
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
// Gain is 20 Log (value/255).  value = 255 sets 10X output gain, value = 26 sets unity gain
//
enum
{
    ACOUSTICIO_AMP_GAIN_1X = 26,
    ACOUSTICIO_AMP_GAIN_10X = 255
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
    ACOUSTICIO_SPKRMON_MODULE,
	ACOUSTICIO_MIKEYBUS_MODULE,
	ACOUSTICIO_LINE_MODULE
};

#define AIO_TYPE_AA  (ACOUSTICIO_ANALOG_MODULE << 4 | ACOUSTICIO_ANALOG_MODULE)
#define AIO_TYPE_XA  (ACOUSTICIO_MODULE_NOT_PRESENT << 4 | ACOUSTICIO_ANALOG_MODULE)
#define AIO_TYPE_AS  (ACOUSTICIO_ANALOG_MODULE << 4 | ACOUSTICIO_SPKRMON_MODULE)
#define AIO_TYPE_MA  (ACOUSTICIO_MIKEYBUS_MODULE << 4 | ACOUSTICIO_ANALOG_MODULE)
#define AIO_TYPE_MM  (ACOUSTICIO_MIKEYBUS_MODULE << 4 | ACOUSTICIO_MIKEYBUS_MODULE)

#define ACOUSTICIO_MODULE_TYPE_CONTROL_MIN_FIRMWARE_VERSION 0x0070

//
// ACOUSTICIO_CLOCK_SOURCE_CONTROL
//
enum
{
    ACOUSTICIO_INTERNAL_CLOCK = 1,
    ACOUSTICIO_USB_CLOCK
};


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
// AIO channel numbers for analog module
//
//======================================================================================

#define AIO_MIC1_INPUT_CHANNEL			0
#define AIO_MIC2_INPUT_CHANNEL			1
#define AIO_MIC3_INPUT_CHANNEL			2
#define AIO_MIC4_INPUT_CHANNEL			3
#define AIO_MIC5_INPUT_CHANNEL			4
#define AIO_MIC6_INPUT_CHANNEL			5
#define AIO_MIC7_INPUT_CHANNEL			6
#define AIO_MIC8_INPUT_CHANNEL			7
#define AIO_AMP1_OUTPUT_CHANNEL			0
#define AIO_AMP2_OUTPUT_CHANNEL			1
#define AIO_AMP3_OUTPUT_CHANNEL			2
#define AIO_AMP4_OUTPUT_CHANNEL			3


//======================================================================================
//
// AIO channel numbers for AIO-S module
//
//======================================================================================

#define AIOS_VOLTAGE_INPUT_CHANNEL 2
#define AIOS_CURRENT_INPUT_CHANNEL 3
#define AIOS_VOLTAGE_OUTPUT_CHANNEL 0

#define AIOS_VOLTAGE_INPUT_CHANNEL_A	2
#define AIOS_CURRENT_INPUT_CHANNEL_A	3
#define AIOS_VOLTAGE_INPUT_CHANNEL_B	6
#define AIOS_CURRENT_INPUT_CHANNEL_B	7
#define AIOS_VOLTAGE_OUTPUT_CHANNEL_A	0
#define AIOS_AMP1_OUTPUT_CHANNEL_A		1
#define AIOS_VOLTAGE_OUTPUT_CHANNEL_B	2
#define AIOS_AMP3_OUTPUT_CHANNEL_B		3

#endif
