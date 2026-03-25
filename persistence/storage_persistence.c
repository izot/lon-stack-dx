/*
 * storage_persistence.c
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Persistent Storage Data Access Functions
 * Purpose: Implements persistent storage data access functions
 *          using non-volatile memory or file storage.
 */

#include "persistence/storage_persistence.h"

// Unique value to identify an initialized transaction record
#define TX_SIGNATURE 0x89ABCDEF

// Transaction states for the associated data
#define TX_DATA_VALID 0xFFFFFFFF
#define TX_DATA_INVALID 0

// Element of the segment_map array. The segment_map array is built at
// runtime and is used to map segments to offsets within persistent
// memory.  To ensure that modifying one segment does not effect
// another, all segments start on flash block boundaries. The first
// part of the segment is the transaction record, with the data portion
// following immediately after.
typedef struct {
    size_t segment_start;    // Offset of the start of the segment
                             // within persistent memory
    size_t tx_offset;        // Offset of the transaction record within 
                             // persistent memory
    size_t data_offset;      // Data starting offset within persistent
                             // memory
    size_t max_data_size;    // Maximum size reserved for persistent
                             // data
    bool erase_required;     // Erase required prior to segment write flag
    uint8_t erase_value;     // Byte value required for erasing the segment
} SegmentMap;

/*****************************************************************
 * Section: Globals
 *****************************************************************/

// Debug trace flag
#ifdef FLASH_DEBUG
static IzotBool persistentTraceEnabled = FALSE;
#endif

// Array built at runtime that is indexed by segment type and used
// to map segments to offsets within persistent memory  
static SegmentMap segment_map[IzotPersistentSegNumSegmentTypes];

// Size, in bytes, of each block of persistent memory; this 
// value is determined at runtime by calling HalStorageInfo()
static size_t storage_block_size = 0;

// Tracks allocation of contiguous blocks of persistent memory
// to each persistent segment; when the persistent memory size
// is determined, the value is set to the end of the memory; as
// the amount of memory to be reserved for each segment is 
// determined, the offset is reduced accordingly
static size_t lowest_used_storage_data_offset = 0;

// Table indexed by <IzotPersistentSegType> and containing the
// maximum size of each segment type; this is used to determine
// size of the data segment and is computed at runtime by 
// calling <IzotPersistentSegGetMaxSize>
static size_t data_segment_size[IzotPersistentSegNumSegmentTypes] = {
    0,       // IzotPersistentSegNetworkImage
    0,       // IzotPersistentSegSecurityII
    0,       // IzotPersistentSegNodeDefinition
    0,       // IzotPersistentSegApplicationData
    0,       // IzotPersistentSegUniqueId
    0,       // IzotPersistentSegConnectionTable
    0,       // IzotPersistentSegIsi
};

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/

// Opens the persistent memory for access via the HAL functions
static LonStatusCode OpenStorageSegment(
        const IzotPersistentSegType persistent_seg_type);

// Initializes the persistent segment map
static LonStatusCode InitSegmentMap(
        const IzotPersistentSegType persistent_seg_type);

// Erases a persistent segment
static LonStatusCode PrepareStorageSegment(
        const IzotPersistentSegType persistent_seg_type, size_t size);

#ifdef FLASH_DEBUG
// Prints a time stamp for persistent memory access tracing
static void PrintTimeStamp();
#endif

/*****************************************************************
 * Section: Function Definitions
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
IzotPersistentSegType IzotStorageOpenSegForRead(IzotPersistentSegType persistent_seg_type)
{
    LonStatusCode status;
    if (LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {   
        OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotStorageOpenSegForRead: %s opened", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return persistent_seg_type;
    } else {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageOpenSegForRead: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        // Return unassigned persistent segment type
        return IzotPersistentSegUnassigned;
    }
}

/* 
 * Opens a persistent data storage segment.
 * Parameters:
 *   persistent_seg_type: Persistent data storage segment to be opened
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
IzotPersistentSegType IzotStorageOpenSegForWrite(const IzotPersistentSegType persistent_seg_type, 
        const size_t data_size)
{
    LonStatusCode status;
    if (!LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageOpenSegForWrite: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return IzotPersistentSegUnassigned;
    }
    if (data_size <= segment_map[persistent_seg_type].max_data_size) {
        // Set all bytes of the transaction record to 0xFF, and erase
        // the data segment if erasing is required prior to writing. 
        // This leaves the transaction record in the following state:
        //   tx_signature = 0xFFFFFFFF (invalid)
        //   tx_state   = 0xFFFFFFFF (TX_DATA_VALID)
        // Because the transaction signature is invalid, a transaction 
        // is reported as in progress until data and a valid
        // transaction signature is written
        status = PrepareStorageSegment(persistent_seg_type, 
                data_size + sizeof(PersistentTransactionRecord));
    }
    // Close the storage--it will be opened again when necessary
    if (!LON_SUCCESS(status = HalCloseStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageOpenSegForWrite: Cannot close %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return IzotPersistentSegUnassigned;
    }
    OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotStorageOpenSegForWrite: %s opened", 
            IzotPersistentGetSegName(persistent_seg_type));  
    return persistent_seg_type;
}

/* 
 * Closes a persistent data storage segment.
 * Parameters:
 *   persistent_seg_type: Persistent data storage segment to be closed
 * Returns:
 *   None
 */
void IzotStorageCloseSeg(const IzotPersistentSegType persistent_seg_type)
{
    // Storage is opened and closed on each access - so there is nothing to 
    // do in this function
    OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotStorageCloseSeg: Segment %s closed", IzotPersistentGetSegName(persistent_seg_type));
}

/* 
 * Deletes a non-volatile storage data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to be deleted
 * Returns:
 *   None
 * Notes:
 *   This function can be called even if the segment does not exist.  It is not
 *   necessary for this function to actually destroy the data or free it.
 */
void IzotStorageDeleteSeg(const IzotPersistentSegType persistent_seg_type)
{
    // There is nothing to be gained by deleting the segment, because the space 
    // is reserved.  This function is just a stub.  
    OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotStorageDeleteSeg: Segment %s deleted", IzotPersistentGetSegName(persistent_seg_type));
}

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
LonStatusCode IzotStorageReadSeg(const IzotPersistentSegType persistent_seg_type, const size_t segment_offset, 
        const size_t data_size, IzotByte * const data_buffer) 
{
    LonStatusCode status = LonStatusNoError;
    if (!LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageReadSeg: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    // Read the data using the segment_map directory
    if (!LON_SUCCESS(status = HalReadStorageSegment(persistent_seg_type, (IzotByte *)data_buffer,
            segment_map[persistent_seg_type].segment_start, 
            segment_map[persistent_seg_type].segment_start + segment_offset, data_size))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageReadSeg: Cannot read %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return status;
    }
    if (!LON_SUCCESS(status = HalCloseStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageReadSeg: Cannot close %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    OsalPrintLog(INFO_LOG, status, "IzotStorageReadSeg: Read %ld bytes starting at %ld for %s",  
            data_size, segment_offset, IzotPersistentGetSegName(persistent_seg_type));
    return status;
}

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
LonStatusCode IzotStorageWriteSeg(const IzotPersistentSegType persistent_seg_type, const size_t segment_offset, 
        const size_t data_size, const IzotByte* const data_buffer) 
{
    LonStatusCode status = LonStatusNoError;
    if (!LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageWriteSeg: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return status;
    }
    // Calculate the starting offset within the flash; the offset is updated
    // as each block of data is written
    size_t next_offset = segment_offset + segment_map[persistent_seg_type].segment_start; 
    // Number of bytes left to write
    size_t remaining_data_size = data_size;
    // Get a pointer to the next data to be written.
    IzotByte *next_buffer = (IzotByte *)data_buffer;
    // Write a block of data at a time
    while (LON_SUCCESS(status) && remaining_data_size) {
        // Offset within storage block
        size_t block_offset = next_offset % storage_block_size;  
        // Number of bytes in block starting at offset
        size_t block_data_size = storage_block_size - block_offset;
        // Number of bytes to write this time
        size_t size_to_write;
        if (block_data_size < remaining_data_size) {
            // Write the entire block
            size_to_write = block_data_size;
        } else {
            // Write only the partial block remaining
            size_to_write = remaining_data_size;
        }
        // Update the current block
        if (!LON_SUCCESS(status = HalWriteStorageSegment(persistent_seg_type, next_buffer,
                segment_map[persistent_seg_type].segment_start, next_offset, size_to_write))) {
            OsalPrintLog(ERROR_LOG, status, "IzotStorageWriteSeg: Cannot write %s", 
                    IzotPersistentGetSegName(persistent_seg_type));
            return status;
        }
        // Adjust offsets, data pointer, and data remaining
        next_offset += size_to_write;
        next_buffer += size_to_write;
        remaining_data_size -= size_to_write;
    }
    if (!LON_SUCCESS(status = HalCloseStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageWriteSeg: Cannot close %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    OsalPrintLog(INFO_LOG, status, "IzotStorageWriteSeg: Wrote %ld bytes starting at %ld for %s",  
            data_size, segment_offset, IzotPersistentGetSegName(persistent_seg_type));
    return status;
}

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
IzotBool IzotStorageSegIsInvalid(const IzotPersistentSegType persistent_seg_type)
{
    // Initialize invalid_segment is set to TRUE; any error reading the
    // transaction record will be interpreted as an invalid segment
    IzotBool invalid_segment = TRUE;
    LonStatusCode status = LonStatusNoError;
    // Open the storage segment to read the transaction record
    if (!LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageSegIsInvalid: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return invalid_segment;
    }
    PersistentTransactionRecord transaction_record;
    // Read the transaction record; the transaction record is always at
    // the beginning of a block, and therefore cannot span blocks
    if (!LON_SUCCESS(status = HalReadStorageSegment(persistent_seg_type,
            (IzotByte *)&transaction_record, segment_map[persistent_seg_type].segment_start,
            segment_map[persistent_seg_type].tx_offset, sizeof(transaction_record)))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageSegIsInvalid: Cannot read %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return invalid_segment;
    }

    // Successfully read the transaction record; if the transaction signature
    // matches and the state is TX_DATA_VALID, the data is valid
    invalid_segment = !(transaction_record.tx_signature == TX_SIGNATURE 
            && transaction_record.tx_state == TX_DATA_VALID);
    if (!LON_SUCCESS(status = HalCloseStorageSegment(persistent_seg_type))) {
        invalid_segment = TRUE;
        OsalPrintLog(ERROR_LOG, status, "IzotStorageSegIsInvalid: Cannot close %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return invalid_segment;
    }
    // Optionally print the result; neither value is an error condition since a
    // storage data segment is always invalid on first use
    if (invalid_segment) {
        OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotStorageSegIsInvalid: %s is invalid or uninitialized (%X transaction signature, %X validity)", 
                IzotPersistentGetSegName(persistent_seg_type), transaction_record.tx_signature, transaction_record.tx_state);  
    } else {
        OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotStorageSegIsInvalid: %s is valid", 
                IzotPersistentGetSegName(persistent_seg_type));  
    }
    return(invalid_segment);
}

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
LonStatusCode IzotStorageStartSegUpdate(const IzotPersistentSegType persistent_seg_type)
{
    LonStatusCode status = LonStatusNoError;
    if (!LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageStartSegUpdate: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    // Invalidate the segment; if the segment transaction record was valid 
    // before it looks like this:
    //   tx_signature = TX_SIGNATURE (0x89ABCDEF, valid)
    //   tx_state  = TX_DATA_VALID (0xFFFFFFFF, valid))
    // In that case it can be updated to the following on all platforms,
    // because only 1 bits are changed by setting them to 0 bits:
    //   tx_signature = TX_SIGNATURE (0x89ABCDEF, valid)
    //   tx_state  = TX_DATA_INVALID (0x00000000, invalid)
    // After this is done, if a segment erase is required, the segment
    // must be erased, and then the segment can be updated.  If the
    // transaction record was not valid in the first place, writing 
    // this pattern will not make it valid.  Once the segment is updated,
    // the transaction record must be updated to make it valid again.
    PersistentTransactionRecord transaction_record;
    transaction_record.tx_signature = TX_SIGNATURE;
    transaction_record.tx_state = TX_DATA_INVALID;  // No longer valid
    if (!LON_SUCCESS(status = HalWriteStorageSegment(persistent_seg_type, (IzotByte *)&transaction_record,
            segment_map[persistent_seg_type].segment_start,
            segment_map[persistent_seg_type].tx_offset, sizeof(transaction_record)))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageStartSegUpdate: Cannot write %s", 
                IzotPersistentGetSegName(persistent_seg_type));
    }
    if (!LON_SUCCESS(status = HalCloseStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageStartSegUpdate: Cannot close %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    OsalPrintLog(INFO_LOG, status, "IzotStorageStartSegUpdate: %s invalidated for write (%X transaction signature, %X validity)",  
            IzotPersistentGetSegName(persistent_seg_type), transaction_record.tx_signature, transaction_record.tx_state);
    return(status);
}

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
LonStatusCode IzotStorageFinishSegUpdate(const IzotPersistentSegType persistent_seg_type)
{
    LonStatusCode status = LonStatusNoError;
    if (!LON_SUCCESS(status = OpenStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageFinishSegUpdate: Cannot open %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    // Mark the segment as valid; if the segment transaction record was erased 
    // by a previsous call to IzotStorageOpenSegForWrite() it looks like this:
    //   tx_signature = TX_SIGNATURE (0xFFFFFFFF, invalid)
    //   tx_state  = TX_DATA_VALID (0xFFFFFFFF, valid))
    // In that case it can be updated to the following on all platforms,
    // because only 1 bits are changed by setting them to 0 bits:
    //   tx_signature = TX_SIGNATURE (0x89ABCDEF, valid)
    //   tx_state  = TX_DATA_VALID (0xFFFFFFFF, valid)
    // Later, a new update to the same segment can be started by setting tx_state
    // to invalid, leaving the following state:
    //   tx_signature = TX_SIGNATURE (0x89ABCDEF, valid)
    //   tx_state  = TX_DATA_INVALID (0x00000000, invalid)
    PersistentTransactionRecord transaction_record;
    transaction_record.tx_signature = TX_SIGNATURE;
    transaction_record.tx_state = TX_DATA_VALID; 
    if (!LON_SUCCESS(status = HalWriteStorageSegment(persistent_seg_type, (IzotByte *)&transaction_record,
            segment_map[persistent_seg_type].segment_start,
            segment_map[persistent_seg_type].tx_offset, sizeof(transaction_record)))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageFinishSegUpdate: Cannot write %s", 
                IzotPersistentGetSegName(persistent_seg_type));
    }
    if (!LON_SUCCESS(status = HalCloseStorageSegment(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "IzotStorageFinishSegUpdate: Cannot close %s", 
                IzotPersistentGetSegName(persistent_seg_type));  
        return status;
    }
    OsalPrintLog(INFO_LOG, status, "IzotStorageFinishSegUpdate: %s marked valid after write (%X transaction signature, %X validity)",  
            IzotPersistentGetSegName(persistent_seg_type), transaction_record.tx_signature, transaction_record.tx_state);
    return(status);
}

/* 
 * Erases persistent application data such as configuration properties.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.   
 */
LonStatusCode ErasePersistentAppData(void)
{
    return PrepareStorageSegment(IzotPersistentSegApplicationData, 
            segment_map[IzotPersistentSegApplicationData].max_data_size);
}

/* 
 * Erases the persistent network image from storage.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.   
 */
LonStatusCode ErasePersistentNetworkConfig(void)
{
    return PrepareStorageSegment(IzotPersistentSegNetworkImage, 
            segment_map[IzotPersistentSegNetworkImage].max_data_size);
}

/* 
 * Opens a storage segment, initializing the segment map if necessary.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to open
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure   
 */
static LonStatusCode OpenStorageSegment(const IzotPersistentSegType persistent_seg_type)
{
    LonStatusCode status = LonStatusNoError;
    if (persistent_seg_type > IzotPersistentSegNumSegmentTypes) {
        // Invalid persistent segment type
        status = LonStatusPersistentDataAccessError;
        OsalPrintLog(ERROR_LOG, status, "OpenStorageSegment: Invalid segment type %d", persistent_seg_type);
        return status;
    }
    // Initialize persistent data storage, if required
    if (!LON_SUCCESS(status = HalInitStorage())) {
        OsalPrintLog(ERROR_LOG, status, "OpenStorageSegment: Cannot initialize storage for %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return status;
    }
    // Initialize the segment_map, if required
    if (!LON_SUCCESS(status = InitSegmentMap(persistent_seg_type))) {
        OsalPrintLog(ERROR_LOG, status, "OpenStorageSegment: Cannot initialize segment map for %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return status;
    }
    // Open the persistent data storage
    if (!LON_SUCCESS(status = HalOpenStorageSegment(persistent_seg_type,
            IzotPersistentGetSegName(persistent_seg_type),
            segment_map[persistent_seg_type].max_data_size + sizeof(PersistentTransactionRecord)))) {
        OsalPrintLog(ERROR_LOG, status, "OpenStorageSegment: Cannot open storage for %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return status;
    }
    OsalPrintLog(INFO_LOG, status, "OpenStorageSegment: %s opened", IzotPersistentGetSegName(persistent_seg_type));
    return status;
}

/* 
 * Initializes the segment_map for the specified segment type.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to initialize
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure   
 * Notes:
 *   The first time this function is called, it determines the storage size
 *   by opening the storage and reading the parameters from the HAL.
 *   Segment types must be initialized in order.
 */
static LonStatusCode InitSegmentMap(const IzotPersistentSegType persistent_seg_type)
{
    LonStatusCode status = LonStatusNoError;
    PersistentTransactionRecord transactionRecord = {TX_SIGNATURE, TX_DATA_VALID};
    size_t region_offset;   // Offset of this region from start of storage
    size_t region_size;     // Size of this erase region
    int number_of_blocks;   // Number of blocks in this region
    size_t block_size;      // Size of each block in this erase region
    int number_of_regions;  // Number of regions in storage
    bool erase_required;    // Erase required prior to write flag
    uint8_t erase_value;    // Byte value required for erasing the segment

#if 0
    // TBD: Remove this code when HAL storage init is reliable
    // Initialize persistent data storage
    if (!LON_SUCCESS(status = HalInitStorage())) {
        OsalPrintLog(ERROR_LOG, status, "InitSegmentMap: Cannot initialize storage for %s", 
                IzotPersistentGetSegName(persistent_seg_type));
        return status;
    }
#endif
    // Storage block size is initially 0, and will be updated by this function
    if (storage_block_size == 0) {
#if 0
        // TBD: Remove this code when HAL storage init is reliable
        // Open the storage
        if (!LON_SUCCESS(status = HalOpenStorageSegment(persistent_seg_type, IzotPersistentGetSegName(persistent_seg_type)))) {
            // Storage cannot be opened
            OsalPrintLog(ERROR_LOG, status, "InitSegmentMap: Cannot open storage for %s", 
                    IzotPersistentGetSegName(persistent_seg_type));
            return status;
        } 
#endif
        // Get the storage information from the HAL
        if (!LON_SUCCESS(status = HalStorageInfo(&region_offset, &region_size, &number_of_blocks, 
                &block_size, &number_of_regions, &erase_required, &erase_value))) {
            // Storage information cannot be read
            OsalPrintLog(ERROR_LOG, status, "InitSegmentMap: Cannot get storage info for %s", 
                    IzotPersistentGetSegName(persistent_seg_type));
            return status;
        }
        if (number_of_regions >= 1) {
            // This code always places all the data segments in the first 
            // region, and expects that region to start at offset 0; the 
            // segments are allocated starting at the highest address of 
            // the storage region

            // Start at the end of the storage region
            lowest_used_storage_data_offset = region_offset + region_size;
            
            // Update the storage block size; in addition to recording this 
            // information, it serves as a flag to indicate that the
            // segment_map is initialized
            storage_block_size = block_size;
        }
    }
    if (data_segment_size[persistent_seg_type] == 0 
            && persistent_seg_type != IzotPersistentSegNodeDefinition 
            && persistent_seg_type != IzotPersistentSegUniqueId) {
        int virtual_storage_offset;
        int block_offset;
    
        data_segment_size[persistent_seg_type] = 
                IzotPersistentSegGetMaxSize(persistent_seg_type) + IzotPersistentSegGetHeaderSize();
    
        // The segments are allocated starting at the highest 
        // virtual address of the storage region; starting at 
        // lowest_used_storage_data_offset, adjust the offset 
        // to allow for the transaction record and data segment
        virtual_storage_offset = lowest_used_storage_data_offset 
                - (sizeof(PersistentTransactionRecord) + data_segment_size[persistent_seg_type]);
    
        // Adjust offset to start on a block boundary
        block_offset = virtual_storage_offset % storage_block_size;
        virtual_storage_offset -= block_offset;
    
        // This function assumes that if the same storage is used
        // for code and persistent data, the code is loaded into 
        // low storage and there is enough room to fit the data 
        // in high storage without overwriting the code
        segment_map[persistent_seg_type].segment_start = virtual_storage_offset;
        segment_map[persistent_seg_type].tx_offset = virtual_storage_offset;
        segment_map[persistent_seg_type].data_offset = virtual_storage_offset
                + sizeof(PersistentTransactionRecord);
        segment_map[persistent_seg_type].max_data_size
                = data_segment_size[persistent_seg_type];
        segment_map[persistent_seg_type].erase_required = erase_required;
        segment_map[persistent_seg_type].erase_value = erase_value;

        // Update the lowest used storage data offset
        lowest_used_storage_data_offset = virtual_storage_offset;

        OsalPrintLog(INFO_LOG, status, "InitSegmentMap: %s segment start: %X", 
                IzotPersistentGetSegName(persistent_seg_type), segment_map[persistent_seg_type].segment_start);
        OsalPrintLog(INFO_LOG, status, "InitSegmentMap: %s transaction record offset: %X", 
                IzotPersistentGetSegName(persistent_seg_type), segment_map[persistent_seg_type].tx_offset);
        OsalPrintLog(INFO_LOG, status, "InitSegmentMap: %s data offset: %X", 
                IzotPersistentGetSegName(persistent_seg_type), segment_map[persistent_seg_type].data_offset);
        OsalPrintLog(INFO_LOG, status, "InitSegmentMap: %s maximum data size: %X", 
                IzotPersistentGetSegName(persistent_seg_type), segment_map[persistent_seg_type].max_data_size);
        OsalPrintLog(INFO_LOG, status, "InitSegmentMap: %s lowest used storage data offset: %d", 
                IzotPersistentGetSegName(persistent_seg_type), lowest_used_storage_data_offset);
    }    
    return status;
}

/* 
 * Erases a persistent data segment.
 * Parameters:
 *   persistent_seg_type: Non-volatile storage data segment to erase
 *   size: Size of data, in bytes
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.   
 * Notes:
 *   After erasing the segment, all of the bytes will be 0xFF.  On some
 *   platforms, writing to storage can only change 1 bits to 0 bits.  This
 *   leaves the transaction record at the beginning of the segment with an
 *   invalid value, indicating that a transaction is in progress.
 */
static LonStatusCode PrepareStorageSegment(const IzotPersistentSegType persistent_seg_type, size_t size)
{
    LonStatusCode status = LonStatusNoError;
    // Get the start of the segment
    size_t seg_start = segment_map[persistent_seg_type].segment_start;
    size_t offset = seg_start;
    size_t erase_block_size = storage_block_size;
    if (segment_map[persistent_seg_type].erase_required == false) {
        // No erase required for the entire segment, just erase the transaction record
        size = sizeof(PersistentTransactionRecord);
        erase_block_size = size;
    }
    // Keep erasing blocks until bytes_erased >= size
    size_t bytes_erased = 0;
    // Erase blocks, a block at a time, until the request has been satisfied
    for (bytes_erased = 0; bytes_erased < size; bytes_erased += storage_block_size) {
        // Prepare the block
        if (!LON_SUCCESS(status = HalPrepareStorageSegment(persistent_seg_type,
                seg_start, offset, erase_block_size, segment_map[persistent_seg_type].erase_value))) {
            OsalPrintLog(ERROR_LOG, status, "PrepareStorageSegment: Cannot erase %s", IzotPersistentGetSegName(persistent_seg_type));
            return status;
        }
        // Next block
        offset += storage_block_size;
    }
    return status;
}

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
char *IzotPersistentGetSegName(IzotPersistentSegType persistent_seg_type)
{
    static char *name = NULL;
    switch (persistent_seg_type) {
    case IzotPersistentSegNetworkImage:
        name = "LonNetworkImage";
        break;
    case IzotPersistentSegNodeDefinition:
        name = "LonNodeDefinition";
        break;
    case IzotPersistentSegApplicationData:
        name = "LonApplicationData";
        break;
    case IzotPersistentSegUniqueId:
        name = "LonUniqueId";
        break;
    case IzotPersistentSegConnectionTable:
        name = "LonConnectionTable";
        break;
    case IzotPersistentSegIsi:
        name = "LonIsi";
        break;
    default:
        name = "LonUnknown";
    }
    return name;
}