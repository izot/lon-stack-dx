/*
 * lcs_network.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Network Layer
 * Purpose: Implements the LON network layer (Layer 3) of the 
 *          ISO/IEC 14908-1 LON protocol stack.
 * Notes:   See ISO/IEC 14908-1, Section 6 for LON protocol details.
 */

#ifndef _NETWRK_H
#define _NETWRK_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "lcs/lcs_queue.h"

/*------------------------------------------------------------------------------
  Section: Function Prototypes
  ------------------------------------------------------------------------------*/
void   NWReset(void);
void   NWSend(void);
void   NWReceive(void);

#endif

