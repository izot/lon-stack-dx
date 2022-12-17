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

/* 
 * module_platform.h - Definitions for target environment
 *
 */


#ifndef MODULE_PLATFORM_H
#define	MODULE_PLATFORM_H

/*
 * Use the xxx_IS(yyy) macros to control conditional compilation
 * For example:
 *
 * #if PRODUCT_IS(SLB)
 * // SLB-only code
 * #endif
 *
 * If the conditional code is in the middle of a function,
 * a preferable method may be to use the macro as real expression
 * and let optimization remove unused code.
 * For example:
 *
 * if (PRODUCT_IS(SLB))
 * {
 *     // SLB-only code
 * }
 *
 */

// Do not start any of these ID macros at zero

#define PRODUCT_ID_SLB  		1  // Streetlight Bridge
#define PRODUCT_ID_ZIGBEE_IEM	2  // ZigBee IEM for meters 

#define PRODUCT_ID PRODUCT_ID_SLB

#define PLATFORM_ID_EXP430	1  // EXP430 board
#define PLATFORM_ID_IEM		2  // Our IEM board
#define PLATFORM_ID_SIM		3  // Windows simulator

#define PLATFORM_ID PLATFORM_ID_SIM

#define PROCESSOR_ID_F5438	1  // EXP board processor
#define PROCESSOR_ID_F5437	2  // Our initial IEM board processor
#define PROCESSOR_ID_F5437A	3  // Our next rev processor (production)

#if !defined(PROCESSOR_ID) && !PLATFORM_IS(SIM)		// Allow processor to be undefined for the simulator
#error You must define PROCESSOR_ID
#endif		

#endif // MODULE_PLATFORM_H