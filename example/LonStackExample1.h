//
// LonStackExample1.h
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

// Title: LON Stack Example 1 Header File
//
// Abstract:
// Definitions for aimple example application for the LON Stack.

// Domain definition.  

#define EXAMPLE_DOMAIN_LENGTH 1
#define EXAMPLE_DOMAIN_ID 0x51
#define EXAMPLE_SUBNET 0x23
#define EXAMPLE_NODE 12


// Example target device definition.  The target device is for a self-installed
// connection to a second device running the same application.  The target device
// must be in the same domain, and must have a different subnet and/or node ID.
// When building the application image for the second device, assign the
// EXAMPLE_TARGET_SUBNET and EXAMPLE_TARGET_NODE values for the first device
// to the EXAMPLE_SUBNET and EXAMPLE_NODE values of the second device, and
// change the EXAMPLE_TARGET_SUBNET and EXAMPLE_TARGET_NODE values for the
// second device.

#define EXAMPLE_TARGET_SUBNET 0x27
#define EXAMPLE_TARGET_NODE 15


/*
* Typedef: IzotFloat
* 32-bit floating-point type defined by ANSI/IEEE 754-1985.
*/
#define IZOT_FLOAT_SIGN_MASK  0x80
#define IZOT_FLOAT_SIGN_SHIFT 7
#define IZOT_FLOAT_SIGN_FIELD Flags_1
#define IZOT_FLOAT_MSEXPONENT_MASK  0x7F
#define IZOT_FLOAT_MSEXPONENT_SHIFT 0
#define IZOT_FLOAT_MSEXPONENT_FIELD Flags_1
#define IZOT_FLOAT_LSEXPONENT_MASK  0x80
#define IZOT_FLOAT_LSEXPONENT_SHIFT 7
#define IZOT_FLOAT_LSEXPONENT_FIELD Flags_2
#define IZOT_FLOAT_MSMANTISSA_MASK  0x7F
#define IZOT_FLOAT_MSMANTISSA_SHIFT 0
#define IZOT_FLOAT_MSMANTISSA_FIELD Flags_2
typedef IZOT_STRUCT_BEGIN(IzotFloat)
{
    IzotByte  Flags_1   /* Contains sign and msexponent; use IZOT_FLOAT_* macros. */
    IzotByte  Flags_2   /* Contains lsexponent and msmantissa; use IZOT_FLOAT_* macros */
    IzotWord  LS_mantissa;
} IZOT_STRUCT_END(IzotFloat);

// Floating point example: to set the sign, exponent, and mantissa for <FloatVariable>:
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_SIGN, <SignBitValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_MSEXPONENT, <MsExponentValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_LSEXPONENT, <LsExponentValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_MSMANTISSA, <MsMantissaValue>);
//      IZOT_SET_UNSIGNED_WORD(<*FloatVariable>, <LsMantissaValue>);


//
// IAP type definitions (this is a small subset of the definitions; see https://www.lonmark.org/nvs/)
//

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
typedef IzotWord SNVT_flow_p
#define SNVT_flow_p_index 161

// SNVT_temp_f (63) -- Temperature (degrees Celsius; 4-byte float)
typedef IzotFloat SNVT_temp_f;
#define SNVT_temp_f_index 63

// SNVT_temp_p (105) -- Temperature (degrees Celsius; 2-byte signed long; scaled value = 1 * 10^-2 * (Raw + 0))
typedef IzotWord SNVT_temp_p
#define SNVT_flow_p_index 105

