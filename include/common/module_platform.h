//
// module_platform.h
//
// Copyright (C) 2023 EnOcean
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
 * Title:- Target Address Space Definitions
 *
 * Abstract:
 * Definitions for the target address space.
 */

#ifndef MODULE_PLATFORM_H
#define	MODULE_PLATFORM_H

#include "IzotConfig.h"
#include "IzotPlatform.h"

// Macro for addresses that can be > 64K -- these are not currently used
#if PLATFORM_IS(SIM)
#define BIGADDR				// They're all 32 bit anyway
typedef unsigned long BIGPTR;
#define FAR_READ_UINT8(addr)	*(IzotByte *)(addr)
#define FAR_WRITE_UINT8(addr, val)	do { *(IzotByte*)(addr) = (val);} while (0);
#else
#define BIGADDR __data20	// Takes up 32 bits
typedef unsigned long BIGPTR;
#define FAR_READ_UINT8(addr)		__data20_read_char(addr)
#define FAR_WRITE_UINT8(addr, val)	__data20_write_char(addr, val)
#define FAR_WRITE_UINT16(addr, val)	__data20_write_short(addr, val)
#endif

#endif // MODULE_PLATFORM_H
