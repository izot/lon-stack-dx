/*
 * lcs_link.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Data Link Layer for LON USB and MIP Data Links
 * Purpose: Implements the LON data link layer (Layer 2) of the 
 *          ISO/IEC 14908-1 LON protocol stack.
 * Notes:   The functions in this file support LON data links using a
 *          LON USB network interface such as the U10 or U60.
 */

#ifndef _LINK_H
#define _LINK_H

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "abstraction/vldv.h"

typedef short LonLinkHandle;

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
void LKReset(void);
void LKSend(void);
void LKReceive(void);
#endif
