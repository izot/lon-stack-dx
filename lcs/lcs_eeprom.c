/*
 * lcs_eeprom.c
 *
 * Copyright (c) 2022-2026 EnOcean
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

/*****************************************************************
 * Section: Globals
 *****************************************************************/
EEPROM eeprom[NUM_STACKS];

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/
/*
 * Writes network image to non-volatile data storage.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
void LCS_WritePersistentNetworkImage(void)
{
	eep->nodeState = IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE);
	IzotPersistentSegSetCommitFlag(IzotPersistentSegNetworkImage);
	IzotPersistentDataHasBeenUpdated();
}

/*
 * Reads network image from non-volatile data storage.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a LonStatusCode error code indicating failure
 */
LonStatusCode LCS_ReadPersistentNetworkImage(void)
{
	LonStatusCode status;
	status = IzotPersistentSegRestore(IzotPersistentSegNetworkImage);
	if (status == LonStatusNoError) {
		OsalPrintLog(INFO_LOG, LonStatusNoError, "LCS_ReadPersistentNetworkImage: Restored network image from non-volatile memory");
	} else if (status == LonStatusPersistentDataFailure) {
		OsalPrintLog(INFO_LOG, LonStatusNoError, "LCS_ReadPersistentNetworkImage: Failed to restore network image from NVM--may be first boot");
	} else {
		OsalPrintLog(ERROR_LOG, status, "LCS_ReadPersistentNetworkImage: Failed to restore network image from non-volatile memory");
	}
	return (status);
}

/*
 * Writes all persistent application data to non-volatile data storage.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
void LCS_WritePersistentAppData(void)
{
	IzotPersistentSegSetCommitFlag(IzotPersistentSegApplicationData);
	IzotPersistentDataHasBeenUpdated();
}

/*
 * Reads all persistent application data from non-volatile data storage.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a LonStatusCode error code indicating failure
 */
LonStatusCode LCS_ReadPersistentAppData(void)
{
	LonStatusCode status;
	status = IzotPersistentSegRestore(IzotPersistentSegApplicationData);
	if (status == LonStatusNoError) {
		OsalPrintLog(INFO_LOG, LonStatusNoError, "LCS_ReadPersistentAppData: Restored application data from non-volatile memory");
	} else {
		OsalPrintLog(INFO_LOG, status, "LCS_ReadPersistentAppData: Failed to restore application data from non-volatile memory--may be first boot");
	}
	return (status);
}

