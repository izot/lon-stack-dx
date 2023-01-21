//
// EchelonStandardDefinitions.h
//
// Copyright (C) 2022 EnOcean
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
 * File: EchelonStandardDefinitions.h
 * Title: Echelon Standard Definitions header file
 *
 * Abstract:
 * This file contains the macros for common error conditions and macros that are generic;
 * typedefs for various datatypes; and macros that are used for accessing bits in a bit 
 * array that is made up of an array of bytes; and macros for malloc library extraction.
 *
 */
#ifdef SUPPORT_PRAGMA_ONCE
#pragma once
#endif

#ifndef __EchelonStandardDefinitions_h
#define __EchelonStandardDefinitions_h

// Include the address space definitions.
#include "module_platform.h"

typedef int  		Bool;

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1
#endif

#ifndef false
#define false		0
#endif

#ifndef true
#define true		1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

//
// Types to use for when the size must be exact.
//
typedef unsigned char  		Bool8;
typedef unsigned char  		Byte;

typedef signed char    		Int8;
typedef short   			Int16;
typedef long    			Int32;
#ifdef _MSC_VER
typedef __int64				Int64;
#else
typedef long long      		Int64;
#endif

typedef unsigned char  		UInt8;
typedef unsigned short 		UInt16;
typedef unsigned long 		UInt32;
#ifdef _MSC_VER
typedef unsigned __int64	UInt64;
#else
typedef unsigned long long  UInt64;
#endif

//
// Types to use for integers when the size has to be <n> bits or more (thus the om).
//
typedef Int16         	 	Int8om;
typedef Int16				Int16om;
typedef Int32				Int32om;
typedef Int64				Int64om;

typedef UInt16				UInt8om;
typedef UInt16				UInt16om;
typedef UInt32				UInt32om;
typedef UInt64				UInt64om;

//
// Standard error returns.  Convention is that error returns contain
// an optional "area" in the high byte.  Error codes between 1..127
// are valid for every area.  Error codes between 128 and 255 are area
// specific.  Error code 0 is reserved for the generic area only.
// In particular, OK should always be 0x0000 regardless of the area.
//
typedef UInt16om						EchErr;

#define ECHERR_OK						0
#define ECHERR_OUT_OF_RANGE				1
#define ECHERR_TIMEOUT					2
#define ECHERR_INVALID_PARAM			3
#define ECHERR_NO_MEMORY				4
#define ECHERR_UNDERFLOW				5
#define ECHERR_OVERFLOW					6
#define ECHERR_DATA_INTEGRITY			7
#define ECHERR_NOT_FOUND				8
#define ECHERR_ALREADY_OPEN				9
#define ECHERR_NOT_OPEN					10
#define ECHERR_DEVICE_ERR				11
#define ECHERR_INVALID_DEVICE_ID		12
#define ECHERR_NO_MSG_AVAILABLE			13
#define ECHERR_NO_BUFFER_AVAILABLE		14
#define ECHERR_NO_RESOURCES				15
#define ECHERR_INVALID_LENGTH			16
#define ECHERR_OPEN_FAILURE				17
#define ECHERR_SECURITY_VIOLATION		18
#define ECHERR_CREATE_FAILURE			19
#define ECHERR_REMOVE_FAILURE			20
#define ECHERR_INVALID_OPERATION		21

#define ECHERR_END_GLOBAL_ERRORS		127
#define ECHERR_START_AREA_ERRORS		128

// Echelon Error Areas
#define ECHERR_AREA_GLOBAL				0   // use global error codes above
#define ECHERR_AREA_SMPL				1   // simplicity error codes
#define ECHERR_AREA_PAL					2   // see pal.h
#define ECHERR_AREA_RTP                 3   // see rtp.h
#define ECHERR_AREA_SLBM                4   // see slbm.h
#define ECHERR_AREA_UPGRADE				5	// see upgrd.h
#define ECHERR_AREA_RFM                 6   // see rfm.h
#define ECHERR_AREA_RAL					7   // see ral.h
#define ECHERR_AREA_AES					8	// see aes.h

#define ECHERR_GET_ERROR(e)				((e) & 0xFF)
#define ECHERR_GET_AREA(e)				(((e)>>8) & 0xFF)
#define ECHERR_SET_AREA(e,a)			((e) ? ((e) | ((a)<<8)) : ECHERR_OK)

#define ECHERR_IS_OK(e)                 ((e) == ECHERR_OK)

#ifdef NDEBUG
# define STATIC static
#else
# define STATIC
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

// Use these macros to bracket a collection of C routines.
#ifdef __cplusplus
#define C_API_START	extern "C" {
#define C_API_END	}
#else
#define C_API_START
#define C_API_END
#endif

#ifdef _MSC_VER
#	define C_DECL	/* __cdecl */
#else
#	define C_DECL
#endif

#if defined(_DEBUG) && defined(TARGET_PLATFORM) && defined(LEAK_CHECK)
// some files use this so map them too after we do it
#define MALLOC(n)	Malloc_debug(n, __FILE__, __LINE__)
#define FREE(p)		Free_debug(p)
#define STRDUP(s)   Strdup_debug(s, __FILE__, __LINE__)
#else
#define MALLOC(n)	malloc(n)
#define FREE(p)		free(p)
#define STRDUP(s)   strdup(s)
#endif

// rand() only returns a value in the range 0-0x7fff; following generates 32 bits.
#define RAND32()		((rand()<<17) | (rand()<<2) | (rand() & 0x3))

// These are used for accessing bits in a bit array that is made up of an array of bytes.
#define BITS_PER_BYTE				8
#define BITS_ARRAY_SIZE(bits)		(((bits)+BITS_PER_BYTE-1)/BITS_PER_BYTE)
#define BITS_BYTE_OFFSET(bitIndex)	((bitIndex)/BITS_PER_BYTE)
#define BITS_MASK(bitIndex)			(1<<((bitIndex)%BITS_PER_BYTE))

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

#endif		// __EchelonStandardDefinitions_h
