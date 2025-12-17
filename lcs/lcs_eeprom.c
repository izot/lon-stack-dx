/*
 * lcs_eeprom.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Non-Volatile Memory Access Functions
 * Purpose: Provides functions to read and write non-volatile memory.
 * Notes:   This implementation assumes a simple model where all 
 * 			non-volatile data fits within 256 bytes.
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "persistence/lon_persistence.h"

#define NVM_START	(&eep->configData)
#define NVM_SIZE 	(sizeof(*eep)-sizeof(eep->readOnlyData))
#define MISC_START  (&gp->nvm)
#define MISC_SIZE	(sizeof(gp->nvm))

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
EEPROM eeprom[NUM_STACKS];

/*------------------------------------------------------------------------------
Section: Function Definitions
------------------------------------------------------------------------------*/
/*******************************************************************************
Function: LCW_WriteNvm
Returns:  void
Purpose:  Record all data to NVM.
*******************************************************************************/
void LCS_WriteNvm(void)
{
	eep->nodeState = IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE);
	IzotPersistentSegSetCommitFlag(IzotPersistentSegNetworkImage);
	IzotPersistentAppSegmentHasBeenUpdated();
}

/*******************************************************************************
Function: LCW_ReadNvm
Returns:  LonStatusNoError if successful, <LonStatusCode> error otherwise.
Purpose:  Read all data from NVM.
*******************************************************************************/
LonStatusCode LCS_ReadNvm(void)
{
	LonStatusCode status;
	status = IzotPersistentSegRestore(IzotPersistentSegNetworkImage);
	return ((status == 0) ? LonStatusNoError : LonStatusNotFound);
}

/*******************************************************************************
Function: LCW_WriteNvs
Returns:  void
Purpose:  Record all persistent NV data to NVM.
*******************************************************************************/
void LCS_WriteNvs(void)
{
	IzotPersistentSegSetCommitFlag(IzotPersistentSegApplicationData);
	IzotPersistentAppSegmentHasBeenUpdated();
}

/*******************************************************************************
Function: LCS_ReadNvs
Returns:  LonStatusNoError if successful, <LonStatusCode> error otherwise.
Purpose:  Read all NV data from NVM.
*******************************************************************************/
LonStatusCode LCS_ReadNvs(void)
{
	LonStatusCode status;
	status = IzotPersistentSegRestore(IzotPersistentSegApplicationData);
	return ((status == 0) ? LonStatusNoError : LonStatusNotFound);
}

