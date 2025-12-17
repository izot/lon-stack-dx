/*
 * lon_persistence.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Persistent Memory Management
 * Purpose: Provides functions to manage persistent memory storage.
 * Notes:   This module handles reading and writing data to hardware flash
 *          memory, ensuring data integrity with a persistent header
 *          containing an application signature.
 */

#ifndef _PERSISTENT_H
#define _PERSISTENT_H

#include <stddef.h>
#include <string.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "abstraction/IzotOsal.h"
#include "isi/isi_int.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"

#if PROCESSOR_IS(MC200)
#include <wm_os.h>
#endif

typedef IZOT_STRUCT_BEGIN(IzotPersistenceHeader)
{
	unsigned int length;
	unsigned signature;
	unsigned appSignature;
	unsigned short version;
	unsigned short checksum;
} IZOT_STRUCT_END(IzotPersistenceHeader);

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
/*
 * Function: IzotPersistentSegGetHeaderSize
 * This function compute Persistent header size
 */
extern unsigned IzotPersistentSegGetHeaderSize(void);

/*
 * Function: ComputePersistenceChecksum
 * This function compute the checksum on data to be stored in flash.
 */
extern uint8_t ComputePersistenceChecksum(uint8_t* pImage, size_t length);

/*
 * Function: ValidatePersistenceChecksum
 * This function validates the checksum from the data read from flash.
 */
extern bool ValidatePersistenceChecksum(IzotPersistenceHeader* pHdr, uint8_t* pImage);

/*
 * Function: GetAppSignature
 * This function returns application signature.
 */
extern unsigned GetAppSignature(void);

/*
 * Function: SetPeristenceGuardBand
 * This function sets guardband duration .
 */
extern void SetPeristenceGuardBand(int nTime);

/*
 * Function: IzotPersistentSegSetCommitFlag
 * This function flags a persistent segment to be committed.
 */
extern void IzotPersistentSegSetCommitFlag(IzotPersistentSegType persistentSegType);

/*
 * Function: IzotPersistentMemStartCommitTimer
 * This function starts the commit timer if it is not already running.
 * Persistent data is committed to the persistent memory when the time
 * expires.
 */
extern void IzotPersistentMemStartCommitTimer(void);

/*
 * Function: IzotPersistentMemReportFailure
 * This function reports a persistent memory write failure.
 */
extern void IzotPersistentMemReportFailure(void);

/*
 * Function: IzotPersistentMemCommitCheck
 * This funtion checks the commit timer and flag, and commits data to
 * persistent memory if the timer has expired or the commit flag is set.
 */
extern void IzotPersistentMemCommitCheck(void);

/*
 * Function: IzotPersistentMemSetCommitFlag
 * This function sets the persistent memory commit flag to force a 
 * commit on the next commit check.
 */
extern void IzotPersistentMemSetCommitFlag(void);

/*
 * Function: IzotPersistentSegRestore
 * This function restores the specified memory segment contents to RAM.
 */
extern LonStatusCode IzotPersistentSegRestore(IzotPersistentSegType persistentSegType);

/*
 *  Event: IzotPersistentSegEnterTransaction
 *  Calls the registered callback of <IzotFlashSegEnterTransaction>.
 */
extern LonStatusCode IzotPersistentSegEnterTransaction(const IzotPersistentSegType persistentSegType);

/*
 *  Event: IzotPersistentSegOpenForWrite
 *  Calls the registered callback of <IzotFlashSegOpenForWrite>.
 */
extern IzotPersistentSegType IzotPersistentSegOpenForWrite(const IzotPersistentSegType persistentSegType, const size_t size);

/*
 *  Event: IzotPersistentSegWrite
 *  Calls the registered callback of <IzotFlashSegWrite>.
 */
extern LonStatusCode IzotPersistentSegWrite(const IzotPersistentSegType persistentSegType, const size_t offset, 
		const size_t size, const void* const pData);

/*
 *  Event: IzotPersistentSegClose
 *  Calls the registered callback of <IzotFlashSegClose>.
 */
extern void IzotPersistentSegClose(const IzotPersistentSegType persistentSegType);

/*
 *  Event: IzotPersistentSegExitTransaction
 *  Calls the registered callback of <IzotFlashSegExitTransaction>.
 */
extern LonStatusCode IzotPersistentSegExitTransaction(const IzotPersistentSegType persistentSegType);

/*
 *  Event: IzotPersistentSegIsInTransaction
 *  Calls the registered callback of <IzotFlashSegIsInTransaction>.
 */
extern IzotBool IzotPersistentSegIsInTransaction(const IzotPersistentSegType persistentSegType);

/*
 *  Event: IzotPersistentSegOpenForRead
 *  Calls the registered callback of <IzotFlashSegOpenForRead>.
 */
extern IzotPersistentSegType IzotPersistentSegOpenForRead(const IzotPersistentSegType persistentSegType);

/*
 *  Event: IzotPersistentSegRead
 *  Calls the registered callback of <IzotFlashSegRead>.
 */
extern LonStatusCode IzotPersistentSegRead(const IzotPersistentSegType persistentSegType, const size_t offset, 
		const size_t size, void * const pBuffer);

/*
 *  Function: IzotPersistentSegGetMaxSize
 *  Gets the number of bytes required to store persistent data.
 *
 *  Parameters:
 *  persistentSegType - The segment type, see <IzotPersistentSegType>
 *
 *  Returns:
 *  The number of bytes required to store persistent data for the specified
 *  segment.
 *
 *  Remarks:
 *  This function will not typically be called directly by the application,
 *  but may be used by persistent data event handlers (implemented by the
 *  application) to reserve space for persistent data segments.
 */
extern int IzotPersistentSegGetMaxSize(IzotPersistentSegType persistentSegType);

/*
 * Function: IzotPersistentSegCommitScheduled
 * This function checks whether any persistent data is scheduled to
 * be committed.  If so, it sets the commit flag to force an immediate
 * commit of that data.  This function is typically called when a
 * reset is requested, to ensure that all persistent data is committed
 * before the reset.
 */
extern IzotBool IzotPersistentSegCommitScheduled(void);

#endif /*_PERSISTENT_H*/