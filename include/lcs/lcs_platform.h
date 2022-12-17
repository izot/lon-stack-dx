// Copyright (C) 2022 Dialog Semiconductor
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

/*********************************************************************
  File:           Platform.h

  Purpose:        Contains platform dependent definitions.
*********************************************************************/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "EchVersion.h"

// The base Neuron firmware version on which this implementation is based.
#define BASE_FIRMWARE_VERSION   16
// The version number of this firmware
#define FIRMWARE_VERSION        (0x80 + VER_MAJOR_D)
#define FIRMWARE_BUILD			VER_BUILD_D

// The model number of this platform
#define MODEL_NUMBER            0xD0

/* The following type definitions need to be changed based on the
   compiler used. The rest of the files should use only int8,
   int16, uint8 etc. Application programs should use
   nint (8-bit int), nlong etc as much as possible. */
typedef signed char         int8;
typedef short int           int16;
typedef long int            int32;
typedef unsigned char       uint8;
typedef unsigned short int  uint16;
typedef unsigned long int   uint32;

/* Typical 709.1 definitions for int long etc. */
typedef int8                nshort;
typedef int8                nint;
typedef uint8               nuint;
typedef uint8               nushort;
typedef int16               nlong;
typedef uint16              nulong;

typedef unsigned char		Boolean;
typedef unsigned char		BitField;

// Turn on packing so that structures are packed on byte boundaries.  This should be done globally via a compiler switch.  Otherwise, try using
// a pragma such as #pragma pack

// Number of stacks on this platform
#define NUM_STACKS 1

// Define code to toggle the service LED
#ifdef WIN32
#define TOGGLE_SERVICE_LED
#else
#define TOGGLE_SERVICE_LED            gp->ioOutputPin1 = 1 - gp->ioOutputPin1; /* Toggle. */
#endif

// This macro takes a C enum type and turns it into a Byte type that will fit into C data structures that are sent on the network.
#define NetEnum(enumType) Byte

// C stack defines bitfields MSB first.  
#define BITF_DECLARED_BIG_ENDIAN
// Target compiler expects bitfields LSB first.
#define BITF_LITTLE_ENDIAN

#include "bitfield.h"

#define LITTLE_ENDIAN
#include "endian.h"

// Specify a way for the application program to suspend so that other apps can run and we don't consume all the CPU
#ifdef WIN32
#include "windows.h"
#define TAKE_A_BREAK Sleep(1);
#else
#include "smip_ldv.h"
#define TAKE_A_BREAK SMP_Service();
#endif

#endif   /* _PLATFORM_H */