//
// IapTypes.h
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
 * Title: IAP Types
 *
 * Abstract:
 * This file provides IoT Access Protocol (IAP) type definitions based on the IzoT
 * platform-independent types defined in IzotPlatform.h and IzotTypes.h.  This
 * file contains a small subset of the standard IAP types; see https://www.lonmark.org/nvs/
 * for a more complete list.
 */


#ifndef _IAP_TYPES_H
#define _IAP_TYPES_H

// Typedef: IzotFloat
// 32-bit floating-point type defined by ANSI/IEEE 754-1985.

#define IZOT_FLOAT_SIGN_MASK  0x80
#define IZOT_FLOAT_SIGN_SHIFT 7
#define IZOT_FLOAT_SIGN_FIELD msExpSign
#define IZOT_FLOAT_MSEXPONENT_MASK  0x7F
#define IZOT_FLOAT_MSEXPONENT_SHIFT 0
#define IZOT_FLOAT_MSEXPONENT_FIELD msExpSign
#define IZOT_FLOAT_LSEXPONENT_MASK  0x80
#define IZOT_FLOAT_LSEXPONENT_SHIFT 7
#define IZOT_FLOAT_LSEXPONENT_FIELD lsExpMsMantissa
#define IZOT_FLOAT_MSMANTISSA_MASK  0x7F
#define IZOT_FLOAT_MSMANTISSA_SHIFT 0
#define IZOT_FLOAT_MSMANTISSA_FIELD lsExpMsMantissa

typedef IZOT_STRUCT_BEGIN(IzotFloat)
{
    IzotByte  msExpSign;         // Contains sign and msexponent; use IZOT_FLOAT_* macros
    IzotByte  lsExpMsMantissa;   // Contains lsexponent and msmantissa; use IZOT_FLOAT_* macros
    IzotWord  lsMantissa;
} IZOT_STRUCT_END(IzotFloat);

// Floating point example: to set the sign, exponent, and mantissa for <FloatVariable>:
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_SIGN, <SignBitValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_MSEXPONENT, <MsExponentValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_LSEXPONENT, <LsExponentValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_MSMANTISSA, <MsMantissaValue>);
//      IZOT_SET_UNSIGNED_WORD(<*FloatVariable>, <LsMantissaValue>);


// SNVT_elapsed_tm (87) -- Elapsed time (days, hours, minutes, seconds, milliseconds; 7-byte structure)
typedef struct {
    IzotWord day;           // 0 -- 65,534; 65,535 is the invalid value (read with IZOT_GET_UNSIGNED_WORD(n))
    IzotByte hour;          // 0 -- 23
    IzotByte minute;        // 0 -- 59
    IzotByte second;        // 0 -- 59
    IzotWord millisecond;   // 0 -- 999 (read with IZOT_GET_UNSIGNED_WORD(n))
} SNVT_elapsed_tm;

#define SNVT_elapsed_tm_index 87

// SNVT_flow_f (53) -- Flow volume (liters/second; 4-byte float)
typedef IzotFloat SNVT_flow_f;
#define SNVT_flow_f_index 53

// SNVT_flow_p (161) -- Flow volume (cubic meters/hour; 2-byte unsigned long; scaled value = 1 * 10^-2 * (Raw + 0))
typedef IzotWord SNVT_flow_p;
#define SNVT_flow_p_index 161

// SNVT_temp_f (63) -- Temperature (degrees Celsius; 4-byte float)
typedef IzotFloat SNVT_temp_f;
#define SNVT_temp_f_index 63

// SNVT_temp_p (105) -- Temperature (degrees Celsius; 2-byte signed long; scaled value = 1 * 10^-2 * (Raw + 0))
typedef IzotWord SNVT_temp_p;
#define SNVT_temp_p_index 105

#endif // _IAP_TYPES_H