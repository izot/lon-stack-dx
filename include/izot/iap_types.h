/*
 * iap_types.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   IAP Types
 * Purpose: Provides IoT Access Protocol (IAP) type definitions based on the LON
 *          platform-independent types defined in IzotPlatform.h and lon_types.h.
 *          This file contains a small subset of the standard IAP types; see
 *          https://www.lonmark.org/nvs/ for a more complete list.
 */

#ifndef _IAP_TYPES_H
#define _IAP_TYPES_H

// Structure: IzotFloat
// 32-bit floating-point type defined by ANSI/IEEE 754-1985.
//
// Remarks:
// Floating point example: to set the sign, exponent, and mantissa for <FloatVariable>:
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_SIGN, <SignBitValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_MSEXPONENT, <MsExponentValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_LSEXPONENT, <LsExponentValue>);
//      IZOT_SET_ATTRIBUTE(<*FloatVariable>, IZOT_FLOAT_MSMANTISSA, <MsMantissaValue>);
//      IZOT_SET_UNSIGNED_WORD(<*FloatVariable>, <LsMantissaValue>);
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


// Enumeration: toggle_state_t
// Enumeration for an on/off state.  Referenced by SNVT_switch
typedef IZOT_ENUM_BEGIN(toggle_state_t) 
{
    TOGGLE_NUL  = -1,       // Undefined state
    TOGGLE_OFF  = 0,        // Off state
    TOGGLE_ON   = 1         // On state
} IZOT_ENUM_END(toggle_state_t);

// Structure: SNVT_elapsed_tm (Index 87)
// Elapsed time (days, hours, minutes, seconds, milliseconds; 7-byte structure).
#define SNVT_elapsed_tm_index 87
typedef struct {
    IzotWord day;           // 0 -- 65,534; 65,535 is the invalid value (read with IZOT_GET_UNSIGNED_WORD(n))
    IzotByte hour;          // 0 -- 23
    IzotByte minute;        // 0 -- 59
    IzotByte second;        // 0 -- 59
    IzotWord millisecond;   // 0 -- 999 (read with IZOT_GET_UNSIGNED_WORD(n))
} SNVT_elapsed_tm;

// Structure: SNVT_flow_f (Index 53)
// Flow volume (liters/second; 4-byte float).
#define SNVT_flow_f_index 53
typedef IzotFloat SNVT_flow_f;

// Structure: SNVT_flow_p (Index 161)
// Flow volume (cubic meters/hour; 2-byte unsigned long; scaled value = 1 * 10^-2 * (Raw + 0)).
typedef IzotWord SNVT_flow_p;
#define SNVT_flow_p_index 161

// Structure: SNVT_switch (Index 95)
// Switch control with an analog level and discrete state (level and state; 2-byte structure).
#define SNVT_switch_index 95
typedef struct {
    IzotByte value;         // Level in percent; 0 -- 200; <scaled value> = (<raw value> * 5) * (10^-1)
    toggle_state_t state;   // On/Off state
} SNVT_switch;

// Structure: SNVT_temp_f (Index 63)
// Temperature (degrees Celsius; 4-byte float).
typedef IzotFloat SNVT_temp_f;
#define SNVT_temp_f_index 63

// Structure: SNVT_temp_p (Index 105)
//Temperature (degrees Celsius; 2-byte signed long; scaled value = 1 * 10^-2 * (Raw + 0)).
typedef IzotWord SNVT_temp_p;
#define SNVT_temp_p_index 105

#endif // _IAP_TYPES_H