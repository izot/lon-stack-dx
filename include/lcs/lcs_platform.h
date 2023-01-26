//
// lcs_platform.h
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

/*********************************************************************
  Purpose:        Platform-dependent definitions.
*********************************************************************/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "IzotConfig.h"
#include "IzotPlatform.h"
#include "echversion.h"

// The base Neuron firmware version on which this implementation is based.
#define BASE_FIRMWARE_VERSION   16

// The version number of this firmware
#define FIRMWARE_VERSION        VER_MAJOR
#define FIRMWARE_MINOR_VERSION  VER_MINOR
#define FIRMWARE_BUILD			    VER_BUILD

// The model number of this platform
#define MODEL_NUMBER            0x74


// Turn on packing so that structures are packed on byte boundaries.  This should be done globally via a compiler switch.  Otherwise, try using
// a pragma such as #pragma pack

// This macro takes a C enum type and turns it into a Byte type that will fit into C data structures that are sent on the network.
#define NetEnum(enumType) IzotByte

// C stack defines bitfields MSB first.
#define BITF_DECLARED_BIG_ENDIAN
// Target compiler expects bitfields LSB first.
#define BITF_LITTLE_ENDIAN

#include "bitfield.h"

// Specify a way for the application program to suspend so that other apps can run and we don't consume all the CPU
#ifdef WIN32
#include "windows.h"
#define TAKE_A_BREAK Sleep(1);
#endif

#endif   /* _PLATFORM_H */
