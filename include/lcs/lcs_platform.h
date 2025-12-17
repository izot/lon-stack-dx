/*
 * lcs_platform.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Platform-Dependent Definitions
 * Purpose: Defines platform-dependent constants and macros.
 * Notes:   This file can be customized for each target platform.
 */

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "izot/IzotPlatform.h"
#include "common/lon_stack_dx_version.h"

// The base LON Stack version on which this implementation is based.
// The base firmware version number is reported in the ND_QUERY_STATUS
// response as an implementation-defined value.
#define BASE_FIRMWARE_VERSION   LON_STACK_DX_VERSION_MAJOR

// The version number of this LON Stack implementation
#define FIRMWARE_VERSION        LON_STACK_DX_VERSION_MAJOR
#define FIRMWARE_MINOR_VERSION  LON_STACK_DX_VERSION_MINOR
#define FIRMWARE_BUILD			LON_STACK_DX_VERSION_PATCH

// Available architecture numbers.  The architecture number is reported in the 
// ND_QUERY_STATUS response as an implementation-defined value.
#define LON_ARCH_UNSPECIFIED   0x00

/* 32-bit */
#define LON_ARCH_X86_32        0x20
#define LON_ARCH_ARM32         0x21

/* 64-bit */
#define LON_ARCH_X86_64        0x40

/* Modern RISC */
#define LON_ARCH_ARM64         0xA8  // ARMv8 / AArch64
#define LON_ARCH_RISCV64       0xA9

/* Virtual / reference */
#define LON_ARCH_SIMULATOR     0xC0
#define LON_ARCH_REFERENCE     0xC1

// The architecture number of this platform
#define ARCHITECTURE_NUMBER LON_ARCH_ARM64

// Turn on packing so that structures are packed on byte boundaries.
// This should be done globally via a compiler switch.  Otherwise,
// try using a pragma such as #pragma pack

// This macro takes a C enum type and turns it into a Byte type that will
//fit into C data structures that are sent on the network.
#define NetEnum(enumType) IzotByte

// LON Stack DX defines bitfields MSB first.
#define BITF_DECLARED_BIG_ENDIAN

// Target compiler expects bitfields LSB first.
#define BITF_LITTLE_ENDIAN

#include "common/bitfield.h"

#endif   /* _PLATFORM_H */
