/*
 * storage_persistence.h
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Persistent Storage Data Access Functions
 * Purpose: Implements persistent storage data access functions
 *          using non-volatile memory or file storage.
 */

#if !defined(DEFINED_PERSISTENT_STORAGE_H)
#define DEFINED_PERSISTENT_STORAGE_H

#include <stdio.h>
#include <string.h>

#include "izot/IzotPlatform.h"

#ifdef FLASH_DEBUG
	#define FLASH_PRINTF	wmprintf
#endif

// A non-volatile transaction record; the data in a segment is
// considered valid if and only if (tx.tx_signature == TX_SIGNATURE)
// AND (tx_state == TX_DATA_VALID)
typedef struct {
    unsigned tx_signature;
    unsigned tx_state;
} PersistentTransactionRecord;

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
/* 
 * Opens a persistent storage data segment for reading
 * Parameters:
 *   persistent_seg_type: Type of non-volatile storage data to be opened
 * Returns:
 *   <IzotPersistentSegType> on success; IzotPersistentSegUnassigned on failure
 * Notes:
 *   If the storage exists and can be opened, this function returns
 *   a valid persistent segment type.  Otherwise it returns 
 *   IzotPersistentSegUnassigned.  If valid, the segment type
 *   passed to and returned by this function is used as the first
 *   parameter when calling IzotStorageReadSeg(). The application 
 *   must maintain the segment type used for each segment. The 
 *   application can invalidate a segment type by calling 
 *   IzotStorageCloseSeg() for that segment type.
 */
extern IzotPersistentSegType IzotStorageOpenSegForRead(IzotPersistentSegType persistent_seg_type); 

/* 
 * Opens a non-volatile storage data segment for writing.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to be opened
 *   data_size: Size of the data to be stored
 * Returns:
 *   <IzotPersistentSegType> on success; IzotPersistentSegUnassigned on failure
 * Notes:
 *  This function is called by LON Stack after making changes
 *  to data that is backed by persistent storage.  Multiple updates 
 *  may have been made, and this function is called only after LON 
 *  Stack determines that all updates have been completed, based on the
 *  flush timeout. 
 */
extern IzotPersistentSegType IzotStorageOpenSegForWrite(const IzotPersistentSegType persistent_seg_type, const size_t size); 

/* 
 * Closes a non-volatile storage data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to be closed
 * Returns:
 *   None
 */
extern void IzotStorageCloseSeg(const IzotPersistentSegType persistentSegmentType);

/* 
 * Reads a section of a non-volatile storage data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to read
 *   segment_offset: Offset within the segment
 *   data_size: Size of the data to be read
 *   data_buffer: Pointer to buffer to store the data
 * Returns:
 *   LonStatusNoError on success; LonStatusCodeerror code on failure
 * Notes:
 *   LON Stack calls this function during initialization to cache data
 *   from persistent storage.  The offset will always be 0 on the first
 *   call after opening the segment.  The offset in each subsequent call
 *   will be incremented by the size of the previous call.
 */
extern LonStatusCode IzotStorageReadSeg(const IzotPersistentSegType persistent_seg_type, 
    const size_t offset, const size_t size, IzotByte* const pBuffer);
																
/* 
 * Writes a section of a non-volatile storage data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to write
 *   segment_offset: Offset within the segment
 *   data_size: Size of the data to be written
 *   data_buffer: Pointer to the data to write into the segment
 * Returns:
 *   LonStatusNoError on success; LonStatusCodeerror code on failure
 * Notes:
 *   LON Stack calls this function after changing data that is backed by
 *   persistent storage.  LON Stack may apply multiple updates and call
 *   this function after the stack determines that all updates required
 *   in the short term have been completed.   
 *
 *   The offset will always be 0 on the first call after opening the segment.
 *   The offset in each subsequent call will be incremented by the size of
 *   the previous call.
 */
extern LonStatusCode IzotStorageWriteSeg(const IzotPersistentSegType persistent_seg_type, 
    const size_t offset, const size_t size, const IzotByte* const pData);

/* 
 * Returns TRUE if a storage segment is invalid.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to check
 * Returns:
 *   TRUE if a storage data segment is invalid; FALSE otherwise
 * Notes:
 *   This function is called by LON Stack during initialization prior to
 *   reading persistent storage data segments.  This allows LON Stack to
 *   determine whether the data in the segment is valid or not.  A storage
 *   segment is invalid if it was never initialized, for example after a
 *   first boot, or if an update to the segment was in progress and the 
 *   LON device was restarted prior to the update completing.  If a
 *   storage segment is invalid, LON Stack initializes the segment to 
 *   default values.  This function is called prior to reading the segment
 *   data.
 */
extern IzotBool IzotStorageSegIsInvalid(const IzotPersistentSegType persistent_seg_type); 

/*
 * Initiates an update to a non-volatile storage data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to write
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code on failure
 * Notes:
 *   LON Stack calls this function prior to updating a non-volatile
 *   storage data segment. This function marks the specified segment
 *   as invalid.  After calling this function, LON Stack completes
 *   updates to the specified segment and then marks the specified
 *   segment as valid.  If a reset or power cycle occurs before the
 *   process completes, the segment will be marked as invalid to 
 *   indicate that an update was interrupted.
 */
extern LonStatusCode IzotStorageStartSegUpdate(const IzotPersistentSegType persistent_seg_type);

/*
 * Completes an update to a non-volatile storage data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to write
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code on failure
 * Notes:
 *   LON Stack calls this function after completing an update to a
 *   non-volatile storage data segment. This function marks the
 *   specified segment as valid.  If a reset or power cycle occurs 
 *   during a segment update prior to calling this function, the
 *   segment will be marked as invalid to indicate that an update
 *   was interrupted.
 */
extern LonStatusCode IzotStorageFinishSegUpdate(const IzotPersistentSegType persistent_seg_type);

/* 
 * Erases persistent application data such as configuration properties.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.   
 */
extern LonStatusCode ErasePersistentAppData(void);

/* 
 * Erases the persistent network image from storage.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.   
 */
extern LonStatusCode ErasePersistentNetworkConfig(void);

/* 
 * Translate a segment type to a friendly name.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to name
 * Returns:
 *   Character string representing the persistent segment.
 * Notes:
 *   The friendly name is used for configuration file naming and 
 *   persistent tracing.
 */
extern char *IzotPersistentGetSegName(IzotPersistentSegType persistent_seg_type);

#endif  /* defined(DEFINED_PERSISTENT_STORAGE_H) */
 