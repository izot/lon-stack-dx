/*
 * IzotEndian.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Byte Order (Endianness) Definitions
 * Purpose: Defines macros and functions for handling byte order conversions.
 * Notes:   This file helps in converting data between host and network byte order.
 */

#ifndef _IZOT_ENDIAN_H
#define _IZOT_ENDIAN_H

#include "lcs/lcs_platform.h"

#undef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234

#undef BIG_ENDIAN
#define BIG_ENDIAN 4321

/*
 * Define the byte order of the system.
 * Required for conversion of network data to host byte order
 * and reverse.  Allowed values are LITTLE_ENDIAN and BIG_ENDIAN
 */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define hton16(s) (((uint16_t)(s) << 8) + ((uint16_t)(s) >> 8))
#define ntoh16(s)  hton16(s)
#else
#define hton16(s) (s)
#define ntoh16(s) (s)
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define hton32(s) (((uint32_t)(s) << 24) + (((uint32_t)(s) << 8)&0xFF0000) + (((uint32_t)(s) >> 8)&0xFF00) + (((uint32_t)(s) >> 24)&0xFF))
#define ntoh32(s) hton32(s)
#else
#define hton32(s) (s)
#define ntoh32(s) (s)
#endif

static uint16_t EndianSwap16(uint16_t in) {
	return (in >> 8) | (in << 8);
}

static uint32_t EndianSwap32(uint32_t in) {
    return ((in >> 24) & 0x000000FF) |
           ((in >> 8)  & 0x0000FF00) |
           ((in << 8)  & 0x00FF0000) |
           ((in << 24) & 0xFF000000);
}
#endif // _IZOT_ENDIAN_H
