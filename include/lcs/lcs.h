/*
 * lcs.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Core Services
 * Purpose: Provides the main initialization and service functions for
 *          the LON Stack DX.
 * Notes:   All applications using the LON Stack DX must call the
 *          LCS_Init() function during initialization and the
 *          LCS_Service() function as often as practical (e.g., once
 * 			per millisecond).
 */

#ifndef _LCS_H
#define _LCS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_platform.h"
#include "lcs/lcs_api.h"
#include "lcs/lcs_custom.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"

// Main initialization and service functions for LON Stack DX.
LonStatusCode LCS_Init(IzotResetCause cause);
extern void LCS_Service(void);

#ifdef  __cplusplus
}
#endif
#endif // _LCS_H
