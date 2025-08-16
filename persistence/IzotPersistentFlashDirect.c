/*
 * IzotPersitentFlashDirect.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Persistent Data Access Functions
 * Purpose: Implements persistent data access functions using
 *          non-volatile memory or file storage.
 * Notes:   Porting may be required for alternate forms of 
 *          non-volatile data storage.
 */

#include "persistence/IzotPersistentFlashDirect.h"

// Unique value to identify an initialized transaction record
#define TX_SIGNATURE 0x89ABCDEF

// Value of the transaction state when the associated data is valid
#define TX_DATA_VALID 0xFFFFFFFF

// A non-volatile transaction record; the data in a segment is
// considered valid if and only if (tx.signature == TX_SIGNATURE)
// AND (txState =- TX_DATA_VALID)
typedef struct {
    unsigned signature;
    unsigned txState;
} PersistentTransactionRecord;

// Element of the segmentMap array. The segmentMap array is built at
// runtime and is used to map segments to offsets within persistent
// memory.  To ensure that modifying one segment does not effect
// another, all segments start on flash block boundaries. The first
// part of the segment is the transaction record, with the data portion
// following immediately after.
typedef struct {
    size_t segmentStart;     // Offset of the start of the segment
                             // within persistent memory
    size_t txOffset;         // Offset of the transaction record within 
                             // persistent memory
    size_t dataOffset;       // Data starting offset within persistent
                             // memory
    size_t maxDataSize;      // Maximum size reserved for persistent
                             // data
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
static SegmentMap segmentMap[IzotPersistentSegNumSegmentTypes];

// Size, in bytes, of each block of persistent memory; this 
// value is determined at runtime by calling HalGetFlashInfo()
static size_t flashBlockSize = 0;

// Tracks allocation of contiguous blocks of persistent memory
// to each persistent segment; when the persistent memory size
// is determined, the value is set to the end of the memory; as
// the amount of memory to be reserved for each segment is 
// determined, the offset is reduced accordingly
static size_t lowestUsedFlashDataOffset = 0;

// Table indexed by <IzotPersistentSegType> and containing the
// maximum size of each segment type; this is used to determine
// size of the data segment and is computed at runtime by 
// calling <IzotPersistentSegGetMaxSize>
static size_t dataSegmentSize[IzotPersistentSegNumSegmentTypes] = {
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
static IzotApiError OpenFlash(
        const IzotPersistentSegType persistentSegType);

// Initializes the persistent segment map
static IzotApiError InitSegmentMap(
        const IzotPersistentSegType persistentSegType);

// Erases a persistent segment
static IzotApiError EraseSegment(
        const IzotPersistentSegType persistentSegType, size_t size);

#ifdef FLASH_DEBUG
// Prints a time stamp for persistent memory access tracing
static void PrintTimeStamp();

// Translates a segment type to a name for persistent memory tracing
static const char *IzotPersistentGetSegName(IzotPersistentSegType persistentSegType);
#endif

/*****************************************************************
 * Section: Callback Function Definitions
 *****************************************************************/

/* 
 * Opens a persistent data segment for reading
 * Parameters:
 *   persistentSegType - type of non-volatile data to be opened
 * Returns:
 *   <IzotPersistentSegType>
 * Notes:
 *   This function opens a persistent data segment for reading.
 *   If the file exists and can be opened, this function returns
 *   a valid persistent segment type.  Otherwise it returns 
 *   IzotPersistentSegUnassigned.  If valid, the segment type
 *   passed to and returned by this function is used as the first
 *   parameter when calling IzotFlashSegRead(). The application 
 *   must maintain the segment type used for each segment. The 
 *   application can invalidate a segment type by calling 
 *   IzotFlashSegClose() for that segment type.
 */
IzotPersistentSegType IzotFlashSegOpenForRead(const IzotPersistentSegType persistentSegType)
{
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("IzotFlashSegOpenForRead(%s)\r\n", 
                IzotPersistentGetSegName(persistentSegType));  
    }
#endif
    if (IZOT_SUCCESS(HalFlashDrvOpen())) {   
        return persistentSegType;
    } else {
        // Return unassigned persistent segment type
        return IzotPersistentSegUnassigned;
    }
}

/* 
 *  Callback: IzotFlashSegOpenForWrite
 *  Open a non-volatile data segment for writing.
 *
 *  Parameters:
 *  persistentSegType - type of non-volatile data to be opened
 *  size - size of the data to be stored
 *
 *  Returns:
 *  <IzotPersistentSegType>.   
 *
 *  Remarks:
 *  This function is called by the LON stack after changes have been
 *  made to data that is backed by persistent storage.  Multiple updates 
 *  may have been made, and this function is called only after the LON 
 *  stack determines that all updates have been completed, based on the
 *  flush timeout. 
 *
 *  IzotPersistentSegUnassigned is returned if the data cannot be written.
 */
IzotPersistentSegType IzotFlashSegOpenForWrite(const IzotPersistentSegType persistentSegType, 
        const size_t size)
{
    IzotApiError sts = IzotApiNoError;
    IzotPersistentSegType returnedSegType = IzotPersistentSegUnassigned;
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotFlashSegOpenForWrite(%s, %ld)\r\n", 
                IzotPersistentGetSegName(persistentSegType), size);  
    }
#endif
    /* Open the flash so that we can erase it */
    if (!IZOT_SUCCESS(sts = OpenFlash(persistentSegType))) {
        return IzotPersistentSegUnassigned;
    }
    if (size <= segmentMap[persistentSegType].maxDataSize) {
        // Erase enough space for both the TX record and all of the data. 
        // After the block has been erased, we can update individual bytes.
        // This leaves the transaction record in the following state:
        //   signature = 0xffffffff (invalid)
        //   txState   = 0xffffffff (TX_DATA_VALID)
        // Because the signature is invalid, a transaction is still 
        // in progress
        sts = EraseSegment(persistentSegType, 
                size + sizeof(PersistentTransactionRecord));
    }
    // Close the flash--it will be opened again when necessary
    if (!IZOT_SUCCESS(sts = HalFlashDrvClose())) {
        return IzotPersistentSegUnassigned;
    }
    // Return the specified segment type
    returnedSegType = persistentSegType;
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotFlashSegOpenForWrite(%s, %ld), sts = %d\r\n",  
               IzotPersistentGetSegName(persistentSegType), size, sts);  
    }
#endif
    return returnedSegType;
}

/* 
 *  Callback: IzotFlashSegClose
 *  Close a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned 
 *           by <IzotFlashSegOpenForRead> or <IzotFlashSegOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  handle and invalidates the handle. 
 */
void IzotFlashSegClose(const IzotPersistentSegType persistentSegType)
{
    // The flash is opened and closed on each access - so there is nothing to 
    // do in this function. 
}

/* 
 *  Callback: IzotFlashSegDelete
 *  Delete non-volatile data segment.
 *
 *  Parameters:
 *  persistentSegType - type of non-volatile data to be deleted
 *
 *  Remarks:
 *  This function is used to delete the non-volatile memory segment referenced 
 *  by the data type.  The LON stack attempts to close the file before deleting it.
 *  it.  
 *
 *  This function can be called even if the segment does not exist.  It is not
 *  necessary for this function to actually destroy the data or free it.
 */
void IzotFlashSegDelete(const IzotPersistentSegType persistentSegType)
{
    // There is nothing to be gained by deleting the segment, because the space 
    // is reserved.  This function is just a stub.  
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("IzotFlashSegDelete(%s)\r\n", IzotPersistentGetSegName(persistentSegType));  
    }
#endif
}

/* 
 *  Callback: IzotFlashSegRead
 *  Read a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned 
 *           by <IzotFlashSegOpenForRead>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API during initialization 
 *  to read data from persistent storage. An error value is returned if the 
 *  specified handle does not exist.    
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
IzotApiError IzotFlashSegRead(const IzotPersistentSegType persistentSegType, const size_t offset, 
        const size_t size, IzotByte * const pBuffer) 
{
    IzotApiError sts = IzotApiNoError;
    
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotFlashSegRead(%s, %ld, %ld)\r\n",  
               IzotPersistentGetSegName(persistentSegType), offset, size);  
    }
#endif

    // Open the flash
    if (!IZOT_SUCCESS(sts = OpenFlash(persistentSegType))) {
        return sts;
    }
    // Read the data using the segmentMap directory
    sts = HalFlashDrvRead((IzotByte *)pBuffer, 
            segmentMap[persistentSegType].dataOffset + offset, size);
    if (!IZOT_SUCCESS(sts)) {
        return sts;
    }
#ifdef FLASH_DEBUG
    int i;
    unsigned char *pBuf = (unsigned char *)pBuffer;
    FLASH_PRINTF("Reading %d bytes from %X\r\n", 
                            size, segmentMap[persistentSegType].dataOffset + offset);
    for (i = 0; i < size; i++) {
        FLASH_PRINTF("%X ", pBuf[i]);
    }
    FLASH_PRINTF("\r\n");
#endif
    // Close the flash
    sts = HalFlashDrvClose();
    
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotFlashSegRead(%s, %ld, %ld), sts = %d\r\n",  
               IzotPersistentGetSegName(persistentSegType), offset, size, sts);  
    }
#endif

    return sts;
}

/* 
 *  Callback: IzotFlashSegWrite
 *  Write a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned 
 *           by <IzotFlashSegOpenForWrite>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pData - pointer to the data to write into the segment
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API after changes have been 
 *  made to data that is backed by persistent storage.  Note that many updates 
 *  might have been made, and this function is called only after the stack 
 *  determines that all updates have been completed.   
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
IzotApiError IzotFlashSegWrite(const IzotPersistentSegType persistentSegType, const size_t offset, 
        const size_t size, const IzotByte* const pData) 
{
    IzotApiError sts = IzotApiNoError;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotFlashSegWrite(%s, %ld, %ld)\r\n",  
               IzotPersistentGetSegName(persistentSegType), offset, size);  
    }
#endif

    // Open the flash
    if (!IZOT_SUCCESS(sts = OpenFlash(persistentSegType))) {
        return sts;
    }
    // Calculate the starting offset within the flash. The flashOffset 
    // will be updated as each block of data is written.
    size_t flashOffset = offset + segmentMap[persistentSegType].dataOffset; 
    // Number of bytes left to write.
    size_t dataRemaining = size;
    // Get a pointer to the next data to be written.
    IzotByte *pBuf = (IzotByte *)pData;

#ifdef FLASH_DEBUG
    int i;
    FLASH_PRINTF("writing %d bytes at %X\r\n", size, flashOffset);
    for (i = 0; i < size; i++) {
        FLASH_PRINTF("%02X ", pBuf[i]);
    }
    FLASH_PRINTF("\r\n");
#endif
    // Write a block of data at a time.  All of the data that needs to be 
    // written was erased by IzotFlashSegOpenForWrite.
    while (IZOT_SUCCESS(sts) && dataRemaining) {
        // Offset within flash block
        size_t blockOffset = flashOffset % flashBlockSize;  
        // Number of bytes in block starting at offset
        size_t dataInBlock = flashBlockSize - blockOffset;
        // Number of bytes to write this time
        size_t sizeToWrite;

        if (dataInBlock < dataRemaining) {
            // Write the entire block
            sizeToWrite = dataInBlock;
        } else {
            // Write only the partial block remaining
            sizeToWrite = dataRemaining;
        }

        // Update the current block
        if (!IZOT_SUCCESS(sts = HalFlashDrvWrite(pBuf, flashOffset, sizeToWrite))) {
            return sts;
        }

        // Adjust offsets, data pointer and data remaining.
        flashOffset += sizeToWrite;
        pBuf += sizeToWrite;
        dataRemaining -= sizeToWrite;
    }

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotFlashSegWrite(%s, %ld, %ld), sts = %d\r\n",  
               IzotPersistentGetSegName(persistentSegType), offset, size, sts);  
    }
#endif
    // Close the flash
    if (IZOT_SUCCESS(sts)) {
        sts = HalFlashDrvClose();
    }

    return sts;
}

/* 
 *  Callback: IzotFlashSegIsInTransaction
 *  Returns TRUE if an PERSISTENT transaction was in progress last time the device 
 *  shut down.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API during initialization 
 *  prior to reading the segment data. This callback must return TRUE if an PERSISTENT 
 *  transaction had been started and never committed.  If this function returns 
 *  TRUE, the Izot Device Stack API will discard the segment, otherwise, the IZOT 
 *  Device Stack API will attempt to read the persistent data. 
 */
IzotBool IzotFlashSegIsInTransaction(const IzotPersistentSegType persistentSegType)
{
    // inTransaction is set to TRUE.  Any error reading the transaction record 
    // will be interpreted as being in a transaction - that is, the data 
    // segment is invalid.
    IzotBool inTransaction = 1;
    IzotApiError sts = IzotApiNoError;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotFlashSegIsInTransaction(%s)\r\n",  
                                                       IzotPersistentGetSegName(persistentSegType)); 
    }
#endif

    // Open the flash to read the transaction record
    if (!IZOT_SUCCESS(sts = OpenFlash(persistentSegType))) {
        return sts;
    }
    PersistentTransactionRecord txRecord;
    // Read the transaction record.  Because the transaction record is 
    // always at the beginning of a block, we assume that it cannot span 
    // blocks.
    sts = HalFlashDrvRead((IzotByte *)&txRecord, 
            segmentMap[persistentSegType].txOffset, sizeof(txRecord));
    if (!IZOT_SUCCESS(sts)) {
        return sts;
    }

    // Successfully read the transaction record, so maybe the data is 
    // valid after all - if the signature matches and the state is
    // TX_DATA_VALID, the data is valid, and therefore we are
    // not in a transaction
#ifdef FLASH_DEBUG
    FLASH_PRINTF("read from %X\r\n",segmentMap[persistentSegType].txOffset);
    FLASH_PRINTF("txRecord.signature:%X\r\n",txRecord.signature);
    FLASH_PRINTF("txRecord.txState:%X\r\n",txRecord.txState);
#endif
    inTransaction = !(txRecord.signature == TX_SIGNATURE 
            && txRecord.txState == TX_DATA_VALID);
    // Close the flash
    if (IZOT_SUCCESS(sts)) {
        sts = HalFlashDrvClose();
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF(
        "End IzotFlashSegIsInTransaction(%s), inTransaction = %d\r\n",  
        IzotPersistentGetSegName(persistentSegType), inTransaction
        );  
    }
#endif
    return(inTransaction);
}

/* 
 *  Callback: IzotFlashSegEnterTransaction
 *  Initiate a non-volatile transaction.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API when the first request 
 *  arrives to update data stored in the specified segment.  The API updates 
 *  the non-persistent image, and schedules writes to update the non-volatile 
 *  storage at a later time.  
 */
IzotApiError IzotFlashSegEnterTransaction(const IzotPersistentSegType persistentSegType)
{
    IzotApiError sts = IzotApiNoError;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotFlashSegEnterTransaction(%s)\r\n", 
                                                    IzotPersistentGetSegName(persistentSegType));  
    }
#endif
    // Open flash
    if (!IZOT_SUCCESS(sts = OpenFlash(persistentSegType))) {
        return sts;
    }
    PersistentTransactionRecord txRecord;
    txRecord.signature = TX_SIGNATURE;
    txRecord.txState = 0;  // No longer valid

    // Clear the transaction record.  If the transaction record was valid 
    // before it looks like this:
    //   signature = TX_SIGNATURE (valid)
    //   txState   = 0xffffffff (TX_DATA_VALID)
    // In that case we can update it to the following, because we are only
    // changing 1 bits to 0 bits:
    //   signature = TX_SIGNATURE (valid)
    //   txState   = 0 (invalid)
    // After this is done, the block must be erased before the transaction
    // can be updated to valid again (this is done 
    // by IzotFlashSegOpenForWrite).
    // If the transaction record was not valid in the first place, writing 
    // this pattern will not make it valid - and so the transaction is
    // still in progress because we consider that a transaction is in 
    // progress any time the transaction record is not valid.
        
#ifdef FLASH_DEBUG
    FLASH_PRINTF("writing at %X\r\n",segmentMap[persistentSegType].txOffset);
    FLASH_PRINTF("txRecord.signature: %X\r\n",txRecord.signature);
    FLASH_PRINTF("txRecord.txState: %X\r\n",txRecord.txState);
#endif
    sts = HalFlashDrvWrite((IzotByte *)&txRecord, 
            segmentMap[persistentSegType].txOffset, sizeof(txRecord));
    // Close the flash
    if (IZOT_SUCCESS(sts)) {
        sts = HalFlashDrvClose();
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotFlashSegEnterTransaction(%s), sts = %d\r\n",  
               IzotPersistentGetSegName(persistentSegType), sts);  
    }
#endif
    return(sts);
}

/* 
 *  Callback: IzotFlashSegExitTransaction
 *  Complete a non-volatile transaction.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API after 
 *  <IzotFlashSegWrite> has returned success and there are no further 
 *  updates required.   
 */
IzotApiError IzotFlashSegExitTransaction(const IzotPersistentSegType persistentSegType)
{
    IzotApiError sts = IzotApiNoError;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotFlashSegExitTransaction(%s)\r\n", 
                                                      IzotPersistentGetSegName(persistentSegType));
    }
#endif
    // Open the flash */
    if (!IZOT_SUCCESS(sts = OpenFlash(persistentSegType))) {
        return sts;
    }
    PersistentTransactionRecord txRecord;
    txRecord.signature = TX_SIGNATURE;
    txRecord.txState = TX_DATA_VALID; 

    // We expect that this should work because this function is only 
    // called after successfully updating the data segment.  First the IZOT 
    // protocol stack calls IzotFlashSegOpenForWrite, which will erase 
    // the entire segment, including the transaction record, then the data 
    // will be 
    // updated.  The transaction record is still in the erased state:
    //   signature = 0xffffffff (invalid)
    //   txState   = 0xffffffff (TX_DATA_VALID)
    // Because we can change 1 bits to 0 bits, this write will result in:
    //   signature = TX_SIGNATURE (valid)
    //   txState   = 0xffffffff (TX_DATA_VALID)
    // Later, if we want to start a transaction, we can update the txState
    // making it invalid again.  
#ifdef FLASH_DEBUG
    FLASH_PRINTF("writing at %X\r\n",segmentMap[persistentSegType].txOffset);
    FLASH_PRINTF("txRecord.signature: %X\r\n",txRecord.signature);
    FLASH_PRINTF("txRecord.txState: %X\r\n",txRecord.txState);
#endif
    sts = HalFlashDrvWrite((IzotByte *)&txRecord,  
            segmentMap[persistentSegType].txOffset, sizeof(txRecord));
    // Close the flash
    if (IZOT_SUCCESS(sts)) {
        sts = HalFlashDrvClose();
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotFlashSegExitTransaction(%d), sts = %d\r\n", 
                persistentSegType, sts);  
    }
#endif
    return(sts);
}

/* 
 *  Function: ErasePersistenceData
 *  Perserve persistent data (CPs, etc) from flash.
 *
 *  Parameters:
 *  void
 *
 *  Returns:
 *  <void>.   
 */
void ErasePersistenceData(void)
{
    EraseSegment(IzotPersistentSegApplicationData, 
            segmentMap[IzotPersistentSegApplicationData].maxDataSize);
}

/* 
 *  Function: ErasePersistenceConfig
 *  Erase the network Image from flash.
 *
 *  Parameters:
 *  void
 *
 *  Returns:
 *  <void>.   
 */
void ErasePersistenceConfig(void)
{
    EraseSegment(IzotPersistentSegNetworkImage, 
            segmentMap[IzotPersistentSegNetworkImage].maxDataSize);
}

void EraseSecIIPersistentData(void)
{
    EraseSegment(IzotPersistentSegSecurityII,
            segmentMap[IzotPersistentSegSecurityII].maxDataSize);
}

/* 
 *  Function: OpenFlash
 *  Open a flash segment, initializing the segmentMap if necessary.
 *
 *  Parameters:
 *  persistentSegType - segment type
 *  pFd -  pointer to flash file descriptor, to be returned by this function.
 *
 *  Returns:
 *  <IzotApiError>.   
 */
static IzotApiError OpenFlash(const IzotPersistentSegType persistentSegType)
{
    IzotApiError sts = IzotApiNoError;

    if (persistentSegType > IzotPersistentSegNumSegmentTypes) {
        // Invalid data type
        sts = IzotApiPersistentFileError;
    } else {
        // Initialize segmentMap, if necessary
        if (IZOT_SUCCESS(sts = InitSegmentMap(persistentSegType))) {
            // Open the flash.
            sts = HalFlashDrvOpen();
        }
    }

    return sts;
}

/* 
 *  Function: InitSegmentMap
 *  If it has not already been done, initialize the segmentMap for the 
 *  specified type.  The first time this function is called, it must determine
 *  the flash size by opening the flash and reading the parameters from the HAL.
 *  Note that the segment types must be initialized in order.
 *
 *  Parameters:
 *  persistentSegType - segment type
 *
 *  Returns:
 *  <IzotApiError>.   
 */
static IzotApiError InitSegmentMap(const IzotPersistentSegType persistentSegType)
{
    IzotApiError sts = IzotApiNoError;

    // Initialize persistent memory
    if (!IZOT_SUCCESS(sts = HalFlashDrvInit())) {
        return sts;
    }

    // flashBlockSize is initially 0, and will be updated by this function
    if (flashBlockSize == 0) {
        // Open the flash
        if (!IZOT_SUCCESS(sts = HalFlashDrvOpen())) {
            // Flash cannot be opened.
            return IzotApiPersistentFileError;
        } 

        // Get the flash information from the HAL
        size_t offset;          // Offset of this region from start of flash
        size_t region_size;     // Size of this erase region
        int number_of_blocks;   // Number of blocks in this region
        size_t block_size;      // Size of each block in this erase region
        int number_of_regions;
        
        sts = HalGetFlashInfo(&offset, &region_size, &number_of_blocks, 
                &block_size, &number_of_regions);
        if (!IZOT_SUCCESS(sts)) {
            // Flash information cannot be read
            return IzotApiPersistentFileError;
        }
        if (number_of_regions >= 1) {
            // This code always places all the data segments in the first 
            // region, and expects that region to start at offset 0. The 
            // segments are allocated starting at the highest address of 
            // the flash region.

            // Start at the end of flash.
            lowestUsedFlashDataOffset = offset + region_size;
            
            // Update the flash block size.  In addition to recording this 
            // information, it serves as a flag to indicate that we do not 
            // need to initialize the segmentMap again.
            flashBlockSize = block_size;
        }
    }
    if (dataSegmentSize[persistentSegType] == 0 && persistentSegType != IzotPersistentSegNodeDefinition 
    && persistentSegType != IzotPersistentSegUniqueId) {
        int flashOffset;
        int blockOffset;
    
        dataSegmentSize[persistentSegType] = 
                IzotPersistentSegGetMaxSize(persistentSegType) + IzotPersistentSegGetHeaderSize();
    
        // The segments are allocated starting at the highest 
        // address of the flash region.  Starting at 
        // lowestUsedFlashDataOffset, adjust the offset 
        // to allow for the transaction record and data segment.
        flashOffset = lowestUsedFlashDataOffset - (sizeof(PersistentTransactionRecord) 
                + dataSegmentSize[persistentSegType]);
    
        blockOffset = flashOffset % flashBlockSize;
    
        // Adjust offset to start on a block boundary */
        flashOffset -= blockOffset;
    
        // This function assumes that code is loaded into 
        // low flash and that there is enough room to fit the data 
        // without overwriting the code.
        segmentMap[persistentSegType].segmentStart = flashOffset;
        segmentMap[persistentSegType].txOffset = flashOffset;
        segmentMap[persistentSegType].dataOffset = flashOffset + 
                                   sizeof(PersistentTransactionRecord);
        segmentMap[persistentSegType].maxDataSize = dataSegmentSize[persistentSegType];
#ifdef FLASH_DEBUG
        FLASH_PRINTF("segmentMap[%d].segmentStart: %X\r\n", 
                                        persistentSegType, segmentMap[persistentSegType].segmentStart);
        FLASH_PRINTF("segmentMap[%d].txOffset: %X\r\n", 
                                          persistentSegType, segmentMap[persistentSegType].txOffset);
        FLASH_PRINTF("segmentMap[%d].dataOffset: %X\r\n", 
                                          persistentSegType, segmentMap[persistentSegType].dataOffset);
        FLASH_PRINTF("segmentMap[%d].maxDataSize: %X\r\n", 
                                          persistentSegType, segmentMap[persistentSegType].maxDataSize);
#endif
        // Update the lowestUsedFlashDataOffset flash offset. */
        lowestUsedFlashDataOffset = flashOffset;
        
    }    
    return sts;
}

/* 
 *  Function: EraseSegment
 *  Erase enough blocks in the segment to satisfy the requested size, starting 
 *  from the beginning of the segment.  Note that this leaves the transaction
 *  record (at the beginning of the segment) with an invalid value, indicating 
 *  that a transaction is in progress.
 *
 *  Parameters:
 *  fd -  open flash file descriptor
 *  persistentSegType - segment type
 *  size - size of data, in bytes
 *
 *  Returns:
 *  <IzotApiError>.   
 *
 *  Remarks:
 *  After erasing the flash, all of the bytes will be 0xff.  Writing to the 
 *  flash can only change 1 bits to 0 bits.
 */
static IzotApiError EraseSegment(const IzotPersistentSegType persistentSegType, size_t size)
{
    IzotApiError sts = IzotApiNoError;

    // Get the start of the segment.
    size_t offset = segmentMap[persistentSegType].segmentStart;
        // Keep erasing blocks until bytesErased >= size.
    size_t bytesErased = 0;

    // Erase blocks, a block at a time, until the request has been satisfied.
    for (bytesErased = 0; bytesErased < size; bytesErased += flashBlockSize) {
        // Erase the block.
        if (!IZOT_SUCCESS(sts = HalFlashDrvErase(offset, flashBlockSize))) {
            return sts;
        }
        // Next block.
        offset += flashBlockSize;
    }
    return sts;
}

/* 
 *  Function: PrintTimeStamp
 *  Print a time stamp used for PERSISTENT tracing.
 */
#ifdef FLASH_DEBUG
static void PrintTimeStamp()
{
    uint32_t time = IzotGetTickCount()*1000/GetTicksPerSecond();
    FLASH_PRINTF("[%d.%.3d]", time/1000, time % 1000);
}
#endif

/* 
 *  Function: IzotPersistentGetSegName
 *  Translate a segment type to a name, used for configuration
 *  file naming and persistant tracing.
 *
 *  Parameters:
 *  persistentSegType -  type of non-volatile data to be opened
 *
 *  Returns:
 *  Character string representing the persistent segment.  
 */
static const char *IzotPersistentGetSegName(IzotPersistentSegType persistentSegType)
{
    const char *name = NULL;
    switch (persistentSegType) {
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

