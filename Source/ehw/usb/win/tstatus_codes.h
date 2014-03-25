/************************************************************************
 *
 *  Module:       tstatus_codes.h
 *
 *  Description:
 *    Common status code definitions used by both 
 *    kernel-mode and user-mode software layers.
 *
 *  Runtime Env.: Win32 / NT Kernel
 *
 *  Author(s):   
 *    Udo Eberhardt,  Udo.Eberhardt@thesycon.de
 *                
 *  Companies:
 *    Thesycon GmbH
 *                
 ************************************************************************/

#if !defined(__tstatus_codes_h__) || defined(__inside_tstatus_codes_str_h__)
#define __tstatus_codes_h__
#define __inside_tstatus_codes_h__


///////////////////////////////////////////////////////////////////
// types
///////////////////////////////////////////////////////////////////

#if defined(APPLE)

// Apple
#include <IOKit/IOReturn.h>

typedef IOReturn TSTATUS;

#define TSTATUS_CODE(x) iokit_vendor_specific_err(x)

#else

// Windows
#if defined(__BUILDMACHINE__) || defined(ENV_KERNEL_MODE)
// Windows kernel mode
typedef long TSTATUS;
#else
// Windows user mode
typedef unsigned long TSTATUS;
#endif

// for private status codes we use the range 0xE0000001 ... 0xE0001FFF.
#define TSTATUS_CODE(x) ((TSTATUS)( 0xE0000000L | ((x) & 0x1FFF) ))

// for status codes that were returned from lower stack we use the range
// 0xE0000001 ... 0xECFFFFFF
// the lower nibble of the MSB contains the Sev/C/R bits of the NTSTATUS code
// the upper nibble of the NTSTATUS facility code is destroyed by this but is
// assumed to be unused anyway
// example:
// 0xC0000001L (STATUS_UNSUCCESSFUL) will result in 0xEC000001
// If the C bit is already set, we pass the code unmodified.
#define TSTATUS_FROM_NTSTATUS(st) ( (TSTATUS)( ((st)&0x20000000) ? (st) : (0xE0000000L | (((st) & 0xE0000000L)>>4) | ((st) & 0xFFFFFF)) ) )

// for status codes set in URB header we use the range
// 0xEF000001 ... 0xEFFFFFFF
// this assumes that the USBD_STATUS_XXX codes does not use the upper nibble of the MSB
// example:
// 0xC0000011L (USBD_STATUS_XACT_ERROR) will result in 0xEF000011
#define TSTATUS_FROM_USBD_STATUS(st) ((TSTATUS)( 0xEF000000L | ((st) & 0xFFFFFF) ))

#endif


// define empty macro, if not included by tstatus_codes_str.h
#if !defined(__inside_tstatus_codes_str_h__)
#define case_TSTATUS_(x)
#endif


///////////////////////////////////////////////////////////////////
// common status codes
// This set of generic status codes may be used in all kernel mode and
// user mode modules. Don't add project-specific codes to this set.
// Add specific codes to tstatus_codes_ex.h instead.
// The generic status codes values have to use the range from
// 0x0001 .. 0x0FFF.
// Under Windows this is mapped to 
// 0xE0000001 ... 0xE0000FFF
///////////////////////////////////////////////////////////////////

#define				TSTATUS_SUCCESS										0
case_TSTATUS_(TSTATUS_SUCCESS)

#define				TSTATUS_FAILED										TSTATUS_CODE(0x001)
case_TSTATUS_(TSTATUS_FAILED)

#define				TSTATUS_INTERNAL_ERROR						TSTATUS_CODE(0x002)		
case_TSTATUS_(TSTATUS_INTERNAL_ERROR)

#define				TSTATUS_TIMEOUT										TSTATUS_CODE(0x003)
case_TSTATUS_(TSTATUS_TIMEOUT)

#define				TSTATUS_REJECTED									TSTATUS_CODE(0x004)
case_TSTATUS_(TSTATUS_REJECTED)

#define				TSTATUS_ABORTED										TSTATUS_CODE(0x005)
case_TSTATUS_(TSTATUS_ABORTED)

#define				TSTATUS_IN_USE										TSTATUS_CODE(0x006)
case_TSTATUS_(TSTATUS_IN_USE)

#define				TSTATUS_BUSY											TSTATUS_CODE(0x007)
case_TSTATUS_(TSTATUS_BUSY)

#define				TSTATUS_ALREADY_DONE							TSTATUS_CODE(0x008)
case_TSTATUS_(TSTATUS_ALREADY_DONE)

#define				TSTATUS_PENDING										TSTATUS_CODE(0x009)
case_TSTATUS_(TSTATUS_PENDING)



#define				TSTATUS_NO_MEMORY									TSTATUS_CODE(0x020)
case_TSTATUS_(TSTATUS_NO_MEMORY)

#define				TSTATUS_NO_RESOURCES							TSTATUS_CODE(0x021)
case_TSTATUS_(TSTATUS_NO_RESOURCES)

#define				TSTATUS_NO_MORE_ITEMS							TSTATUS_CODE(0x022)
case_TSTATUS_(TSTATUS_NO_MORE_ITEMS)

#define				TSTATUS_NO_DEVICES								TSTATUS_CODE(0x023)
case_TSTATUS_(TSTATUS_NO_DEVICES)


#define				TSTATUS_NOT_SUPPORTED							TSTATUS_CODE(0x030)
case_TSTATUS_(TSTATUS_NOT_SUPPORTED)

#define				TSTATUS_NOT_POSSIBLE							TSTATUS_CODE(0x031)
case_TSTATUS_(TSTATUS_NOT_POSSIBLE)

#define				TSTATUS_NOT_ALLOWED								TSTATUS_CODE(0x032)
case_TSTATUS_(TSTATUS_NOT_ALLOWED)

#define				TSTATUS_NOT_OPENED								TSTATUS_CODE(0x033)
case_TSTATUS_(TSTATUS_NOT_OPENED)

#define				TSTATUS_NOT_AVAILABLE							TSTATUS_CODE(0x034)
case_TSTATUS_(TSTATUS_NOT_AVAILABLE)

#define				TSTATUS_INSTANCE_NOT_AVAILABLE		TSTATUS_CODE(0x035)
case_TSTATUS_(TSTATUS_INSTANCE_NOT_AVAILABLE)



#define				TSTATUS_INVALID_IOCTL							TSTATUS_CODE(0x040)
case_TSTATUS_(TSTATUS_INVALID_IOCTL)

#define				TSTATUS_INVALID_PARAMETER					TSTATUS_CODE(0x041)
case_TSTATUS_(TSTATUS_INVALID_PARAMETER)

#define				TSTATUS_INVALID_LENGTH						TSTATUS_CODE(0x042)
case_TSTATUS_(TSTATUS_INVALID_LENGTH)

#define				TSTATUS_INVALID_BUFFER_SIZE				TSTATUS_CODE(0x043)
case_TSTATUS_(TSTATUS_INVALID_BUFFER_SIZE)

#define				TSTATUS_INVALID_INBUF_SIZE				TSTATUS_CODE(0x044)
case_TSTATUS_(TSTATUS_INVALID_INBUF_SIZE)

#define				TSTATUS_INVALID_OUTBUF_SIZE				TSTATUS_CODE(0x045)
case_TSTATUS_(TSTATUS_INVALID_OUTBUF_SIZE)

#define				TSTATUS_INVALID_TYPE							TSTATUS_CODE(0x046)
case_TSTATUS_(TSTATUS_INVALID_TYPE)

#define				TSTATUS_INVALID_INDEX							TSTATUS_CODE(0x047)
case_TSTATUS_(TSTATUS_INVALID_INDEX)

#define				TSTATUS_INVALID_HANDLE						TSTATUS_CODE(0x048)
case_TSTATUS_(TSTATUS_INVALID_HANDLE)

#define				TSTATUS_INVALID_DEVICE_STATE			TSTATUS_CODE(0x049)
case_TSTATUS_(TSTATUS_INVALID_DEVICE_STATE)

#define				TSTATUS_INVALID_DEVICE_CONFIG			TSTATUS_CODE(0x04A)
case_TSTATUS_(TSTATUS_INVALID_DEVICE_CONFIG)



#define				TSTATUS_INVALID_DESCRIPTOR				TSTATUS_CODE(0x050)
case_TSTATUS_(TSTATUS_INVALID_DESCRIPTOR)

#define				TSTATUS_INVALID_FORMAT						TSTATUS_CODE(0x051)
case_TSTATUS_(TSTATUS_INVALID_FORMAT)

#define				TSTATUS_INVALID_CONFIGURATION			TSTATUS_CODE(0x052)
case_TSTATUS_(TSTATUS_INVALID_CONFIGURATION)

#define				TSTATUS_INVALID_MODE							TSTATUS_CODE(0x053)
case_TSTATUS_(TSTATUS_INVALID_MODE)

#define				TSTATUS_INVALID_FILE							TSTATUS_CODE(0x054)
case_TSTATUS_(TSTATUS_INVALID_FILE)



#define				TSTATUS_VERSION_MISMATCH					TSTATUS_CODE(0x060)
case_TSTATUS_(TSTATUS_VERSION_MISMATCH)

#define				TSTATUS_LENGTH_MISMATCH						TSTATUS_CODE(0x061)
case_TSTATUS_(TSTATUS_LENGTH_MISMATCH)

#define				TSTATUS_MAGIC_MISMATCH						TSTATUS_CODE(0x062)
case_TSTATUS_(TSTATUS_MAGIC_MISMATCH)

#define				TSTATUS_VALUE_UNKNOWN							TSTATUS_CODE(0x063)
case_TSTATUS_(TSTATUS_VALUE_UNKNOWN)

#define				TSTATUS_UNEXPECTED_DEVICE_RESPONSE	TSTATUS_CODE(0x064)
case_TSTATUS_(TSTATUS_UNEXPECTED_DEVICE_RESPONSE)



#define				TSTATUS_ENUM_REQUIRED							TSTATUS_CODE(0x070)
case_TSTATUS_(TSTATUS_ENUM_REQUIRED)

#define				TSTATUS_DEVICE_REMOVED						TSTATUS_CODE(0x071)
case_TSTATUS_(TSTATUS_DEVICE_REMOVED)

#define				TSTATUS_DEVICES_EXIST							TSTATUS_CODE(0x072)
case_TSTATUS_(TSTATUS_DEVICES_EXIST)

#define				TSTATUS_WRONG_DEVICE_STATE				TSTATUS_CODE(0x073)
case_TSTATUS_(TSTATUS_WRONG_DEVICE_STATE)

#define				TSTATUS_BUFFER_TOO_SMALL					TSTATUS_CODE(0x074)
case_TSTATUS_(TSTATUS_BUFFER_TOO_SMALL)



#define				TSTATUS_REGISTRY_ACCESS_FAILED		TSTATUS_CODE(0x080)
case_TSTATUS_(TSTATUS_REGISTRY_ACCESS_FAILED)

#define				TSTATUS_OBJECT_NOT_FOUND					TSTATUS_CODE(0x081)
case_TSTATUS_(TSTATUS_OBJECT_NOT_FOUND)

#define				TSTATUS_NOT_IN_HIGH_SPEED_MODE		TSTATUS_CODE(0x082)
case_TSTATUS_(TSTATUS_NOT_IN_HIGH_SPEED_MODE)

#define				TSTATUS_WRONG_DIRECTION						TSTATUS_CODE(0x083)
case_TSTATUS_(TSTATUS_WRONG_DIRECTION)

#define				TSTATUS_PARAMETER_REJECTED				TSTATUS_CODE(0x084)
case_TSTATUS_(TSTATUS_PARAMETER_REJECTED)

#define				TSTATUS_ALREADY_OPEN							TSTATUS_CODE(0x085)
case_TSTATUS_(TSTATUS_ALREADY_OPEN)



#define				TSTATUS_OPEN_FILE_FAILED					TSTATUS_CODE(0x090)
case_TSTATUS_(TSTATUS_OPEN_FILE_FAILED)

#define				TSTATUS_READ_FILE_FAILED					TSTATUS_CODE(0x091)
case_TSTATUS_(TSTATUS_READ_FILE_FAILED)

#define				TSTATUS_WRITE_FILE_FAILED					TSTATUS_CODE(0x092)
case_TSTATUS_(TSTATUS_WRITE_FILE_FAILED)



// project-specific status codes
#include "tstatus_codes_ex.h"


#if !defined(__inside_tstatus_codes_str_h__)
#undef case_TSTATUS_
#endif


#undef __inside_tstatus_codes_h__


//
// utility function:
// convert status code to string
// The function returns NULL if the provided code is unknown.
// see tbase_func_TbConvertTStatusToString.inc for an implementation
//
const char*
TStatusStringImplA(
	TSTATUS st
	);


#ifdef TBASE_COMPILER_MICROSOFT

//
// wide character variant
//	
const wchar_t*
TStatusStringImplW(
	TSTATUS st
	);

#endif



#endif  // __tstatus_codes_h__

/*************************** EOF **************************************/
