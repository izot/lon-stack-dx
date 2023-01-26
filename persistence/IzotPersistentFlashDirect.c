//
// IzotPersitentFlashDirect.c
//
// Copyright (C) 2023 EnOcean
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
 * Abstract:
 * Example implementation of the Izot non-volatile data (PERSISTENT) functions
 * using Marvell 88MC200 flash access routines. 
 * 
 * You can port this file to support your hardware and application
 * requirements.
 */

#include "IzotPersistentFlashDirect.h"

/*------------------------------------------------------------------------------
Section: Macros
------------------------------------------------------------------------------*/
// Translate a <IzotPersistentSegmentType> to a <IzotPersistentHandle>. 
// Add one to the type because the handle value of 0 is reserved.
#define TypeToHandle(type) ((IzotPersistentHandle)((type)+1))

// Translate a <IzotPersistentHandle> to <IzotPersistentSegmentType>.
#define HandleToType(handle) \
    ((handle) == 0 ? IzotPersistentSegNumSegmentTypes : ((IzotPersistentSegmentType)(handle))-1)
    
// A unique value to identify an initialized transaction record.
#define TX_SIGNATURE 0x89ABCDEF

// Value of the transaction state when the associated data is valid.
#define TX_DATA_VALID 0xffffffff

/*------------------------------------------------------------------------------
Section: Definations
------------------------------------------------------------------------------*/
// A non-volatile transaction record.  
// The data in a segment is considered valid if and only if 
// (tx.signature == TX_SIGNATURE) AND (txState =- TX_DATA_VALID)
typedef struct
{
    unsigned signature;
    unsigned txState;
} PersistentTransactionRecord;

// The SegmentMap structure is used to define the segmentMap array. The 
// segmentMap array is built at runtime and is used to map segments to 
// offsets within flash.  In order to ensure that modifying one segment does 
// not effect another, all segments start on flash block boundaries. The 
// first part of the segment is the transaction record, with the data portion 
// following immediately after.
typedef struct 
{
    unsigned segmentStart;   // Offset of the start of the segment within 
                             // flash memory
    unsigned txOffset;       // Offset of the transaction record within flash 
                             // memory.
    unsigned dataOffset;     // Offset of the data within flash memory
    unsigned maxDataSize;    // The maximum size reserved for the data.
} SegmentMap;

/*------------------------------------------------------------------------------
Section: static
------------------------------------------------------------------------------*/
// Debug trace flag.
#ifdef FLASH_DEBUG
static IzotBool persistentTraceEnabled = 0;
#endif

// The segmentMap array is built at runtime and is used to map segments to 
// offsets within flash.  The segmentMap array is indexed by segment type.  
static SegmentMap segmentMap[IzotPersistentSegNumSegmentTypes];

// The flashBlockSize is the size, in bytes, of each block of flash. This 
// value is determined at runtime by calling the HAL function 
// HalGetFlashInfo.
static int flashBlockSize = 0;

// The lowestUsedFlashDataOffset variable is used to allocate contiguous blocks 
// of flash to each segment. When the flash size is determined, the value is 
// set to the end of flash.  Then, as the amount of flash to be reserved for 
// each segment is determined, the offset is reduced accordingly.
static int lowestUsedFlashDataOffset = 0;

// Table indexed by <IzotPersistentSegmentType> and containing the maximum 
// size of the data segment.  Computed at runtime by 
// calling <IzotPersistentGetMaxSize>.

static int dataSegmentSize[IzotPersistentSegNumSegmentTypes] =
{
    0,       // IzotPersistentSegNetworkImage
    0,       // IzotPersistentSegSecurityII
    0,       // IzotPersistentSegNodeDefinition
    0,       // IzotPersistentSegApplicationData
    0,       // IzotPersistentSegUniqueId
    0,       // IsiPersistentSegConnectionTable
    0,       // IsiPersistentSegPersistent
};

/*------------------------------------------------------------------------------
Section: Forward References
------------------------------------------------------------------------------*/
// Open the flash for access via the HAL.
static IzotApiError OpenFlash(const IzotPersistentSegmentType type, void **pFd);

// Initialize the segmentMap array.
static IzotApiError InitSegmentMap(const IzotPersistentSegmentType type);

// Erase a segment.
static IzotApiError EraseSegment(
void *fd, 
const IzotPersistentSegmentType type, 
int size
);

#ifdef FLASH_DEBUG
// Print a time stamp for PERSISTENT tracing.
static void PrintTimeStamp();

// Translate a segment type to a name for PERSISTENT tracing.
static const char *GetPersistentName(IzotPersistentSegmentType type);
#endif

/*
 * *****************************************************************************
 * SECTION: CALLBACK FUNCTIONS
 * *****************************************************************************
 *
 */
 
/* 
 *  Callback: IzotPersistentOpenForRead
 *  Open a non-volatile data segment for reading.
 *
 *  Parameters:
 *  type -  type of non-volatile data to be opened
 *
 *  Returns:
 *  <IzotPersistentHandle>.   
 *
 *  Remarks:
 *  This function opens the data segment for reading.  If the file exists and 
 *  can be opened, this function returns a valid application-specific 32-bit 
 *  value as the handle.  Otherwise it returns 0.  The handle returned by this 
 *  function will be used as the first parameter when 
 *  calling <IzotPersistentRead>. The application must maintain the handle 
 *  used for each segment. The application can invalidate a handle when 
 *  <IzotPersistentClose> is called for that handle.  
 */
const IzotPersistentHandle IzotPersistentOpenForRead(
const IzotPersistentSegmentType type
)
{
    // This function does nothing more than translate the segment type to a 
    // handle. The actual flash data is opened and closed on each access.
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("IzotPersistentOpenForRead(%s)\r\n", 
                                                      GetPersistentName(type));  
    }
#endif
    return TypeToHandle(type);
}

/* 
 *  Callback: IzotPersistentOpenForWrite
 *  Open a non-volatile data segment for writing.
 *
 *  Parameters:
 *  type - type of non-volatile data to be opened
 *  size - size of the data to be stored
 *
 *  Returns:
 *  <IzotPersistentHandle>.   
 *
 *  Remarks:
 *  This function is called by the IZOT protocol stack after changes 
 *  have been made to data that is backed by persistent storage.  Note that 
 *  many updates might have been made, and this function is called only after 
 *  the stack determines that all updates have been completed, based on a flush 
 *  timeout defined by the LonTalk Interface Developer. 
 *
 *  An error value is returned if the data cannot be written.
 */
const IzotPersistentHandle IzotPersistentOpenForWrite(
const IzotPersistentSegmentType type, 
const size_t size
)
{
    IzotApiError sts = IzotApiNoError;
    void *fd;
    IzotPersistentHandle izotHandle = NULL;
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotPersistentOpenForWrite(%s, %ld)\r\n", 
                                                GetPersistentName(type), size);  
    }
#endif
    /* Open the flash so that we can erase it */
    sts = OpenFlash(type, &fd);
    if (sts == IzotApiNoError) {
        if (size <= segmentMap[type].maxDataSize) {
            // Erase enough space for both the TX record and all of the data. 
            // After the block has been erased, we can update individual bytes.
            // Note that this leaves the transaction record in the following
            // state:
            //   signature = 0xffffffff (invalid)
            //   txState   = 0xffffffff (TX_DATA_VALID)
            // Because the signature is invalid, a transaction is still 
            // in progress.
            // 
            sts = EraseSegment(
            fd, type, size + sizeof(PersistentTransactionRecord)
            );
        }

        // Close the flash.  It will be opened again when necessary.
        HalFlashDrvClose(fd);
    }

    if (sts == IzotApiNoError) {   
        // Translate the type to a handle.
        izotHandle = TypeToHandle(type);
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotPersistentOpenForWrite(%s, %ld), sts = %d\r\n",  
               GetPersistentName(type), size, sts);  
    }
#endif
    return izotHandle;
}

/* 
 *  Callback: IzotPersistentClose
 *  Close a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned 
 *           by <IzotPersistentOpenForRead> or <IzotPersistentOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  handle and invalidates the handle. 
 */
void IzotPersistentClose(const IzotPersistentHandle handle)
{
    // The flash is opened and closed on each access - so there is nothing to 
    // do in this function. 
}

/* 
 *  Callback: IzotPersistentDelete
 *  Delete non-volatile data segment.
 *
 *  Parameters:
 *  type - type of non-volatile data to be deleted
 *
 *  Remarks:
 *  This function is used to delete the non-volatile memory segment referenced 
 *  by the data type.  The Izot Device Stack API attempts to close the 
 *  file before deleting it.  
 *
 *  Note that this function can be called even if the segment does not exist.  
 *  It is not necessary for this function to actually destroy the data or free 
 *  it.
 */
void IzotPersistentDelete(const IzotPersistentSegmentType type)
{
    // There is nothing to be gained by deleting the segment, because the space 
    // is reserved.  This function is just a stub.  
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("IzotPersistentDelete(%s)\r\n", GetPersistentName(type));  
    }
#endif
}

/* 
 *  Callback: IzotPersistentRead
 *  Read a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned 
 *           by <IzotPersistentOpenForRead>
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
const IzotApiError IzotPersistentRead(
const IzotPersistentHandle handle, 
const size_t offset, 
const size_t size, 
void * const pBuffer
) 
{
    IzotApiError sts = IzotApiNoError;
    void *fd;

    // Find the type from the handle.
    IzotPersistentSegmentType type = HandleToType(handle);
    
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotPersistentRead(%s, %ld, %ld)\r\n",  
               GetPersistentName(type), offset, size);  
    }
#endif

    // Open the flash
    sts = OpenFlash(type, &fd);
    if (sts == IzotApiNoError) {
        // Read the data using the segmentMap directory.
        if (HalFlashDrvRead(
        fd, pBuffer, size, segmentMap[type].dataOffset + offset
        ) != 0) {
            sts = IzotApiPersistentFileError;
        }
#ifdef FLASH_DEBUG
        else {
            int i;
            unsigned char *pBuf = (unsigned char *)pBuffer;
            FLASH_PRINTF("Reading %d bytes from %X\r\n", 
                                    size, segmentMap[type].dataOffset + offset);
            for (i = 0; i < size; i++) {
                FLASH_PRINTF("%X ", pBuf[i]);
            }
            FLASH_PRINTF("\r\n");
        }
#endif
        // Close the flash.
        HalFlashDrvClose(fd);
    }
    
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotPersistentRead(%s, %ld, %ld), sts = %d\r\n",  
               GetPersistentName(type), offset, size, sts);  
    }
#endif

    return sts;
}

/* 
 *  Callback: IzotPersistentWrite
 *  Write a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned 
 *           by <IzotPersistentOpenForWrite>
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
const IzotApiError IzotPersistentWrite (
const IzotPersistentHandle handle, 
const size_t offset, 
const size_t size, 
const void* const pData
) 
{
    IzotApiError sts = IzotApiNoError;
    void *fd;

    // Find the type from the handle.
    IzotPersistentSegmentType type = HandleToType(handle);

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotPersistentWrite(%s, %ld, %ld)\r\n",  
               GetPersistentName(type), offset, size);  
    }
#endif

    // Open the flash
    sts = OpenFlash(type, &fd);
    if (sts == IzotApiNoError) {
            // Calculate the starting offset within the flash. The flashOffset 
            // will be updated as each block of data is written.
            
        int flashOffset = offset + segmentMap[type].dataOffset; 
            // dataRemaining is the number of bytes left to write.
        int dataRemaining = size;
            // Get a pointer to the next data to be written.
        unsigned char *pBuf = (unsigned char *)pData;

#ifdef FLASH_DEBUG
        int i;
        FLASH_PRINTF("writing %d bytes at %X\r\n", size, flashOffset);
        for (i = 0; i < size; i++) {
            FLASH_PRINTF("%02X ", pBuf[i]);
        }
        FLASH_PRINTF("\r\n");
#endif
        // Write a block of data at a time.  All of the data that needs to be 
        // written was erased by IzotPersistentOpenForWrite.

        while (sts == IzotApiNoError && dataRemaining) {
                // Offset within flash block
            int blockOffset = flashOffset % flashBlockSize;  
                // Number of bytes in block starting at offset
            int dataInBlock = flashBlockSize - blockOffset;
                // Number of bytes to write this time
            int sizeToWrite;

            if (dataInBlock < dataRemaining) {
                // Write the whole block
                sizeToWrite = dataInBlock;
            } else {
                // Write only the partial block remaining.
                sizeToWrite = dataRemaining;
            }

            // Update the current block.
            if (HalFlashDrvWrite(fd, pBuf, sizeToWrite, flashOffset) != 0) {
                sts = IzotApiPersistentFileError;
            } else {
                // Adjust offsets, data pointer and data remaining.
                flashOffset += sizeToWrite;
                pBuf += sizeToWrite;
                dataRemaining -= sizeToWrite;
            }
        }

        // Close the flash.
        HalFlashDrvClose(fd);
    }

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotPersistentWrite(%s, %ld, %ld), sts = %d\r\n",  
               GetPersistentName(type), offset, size, sts);  
    }
#endif
    return sts;
}

/* 
 *  Callback: IzotPersistentIsInTransaction
 *  Returns TRUE if an PERSISTENT transaction was in progress last time the device 
 *  shut down.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API during initialization 
 *  prior to reading the segment data. This callback must return TRUE if an PERSISTENT 
 *  transaction had been started and never committed.  If this function returns 
 *  TRUE, the Izot Device Stack API will discard the segment, otherwise, the IZOT 
 *  Device Stack API will attempt to read the persistent data. 
 */
const IzotBool IzotPersistentIsInTransaction(const IzotPersistentSegmentType type)
{
    // inTransaction is set to TRUE.  Any error reading the transaction record 
    // will be interpreted as being in a transaction - that is, the data 
    // segment is invalid.

    IzotBool inTransaction = 1;
    void *fd = NULL;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotPersistentIsInTransaction(%s)\r\n",  
                                                       GetPersistentName(type)); 
    }
#endif

    // Open the flash to read the transaction.
    if (OpenFlash(type, &fd) == IzotApiNoError) {
        PersistentTransactionRecord txRecord;
        // Read the transaction record.  Because the transaction record is 
        // always at the beginning of a block, we assume that it cannot span 
        // blocks.

        if (HalFlashDrvRead(fd, (uint8_t *)&txRecord, 
        sizeof(txRecord), segmentMap[type].txOffset) == 0) {
            // Successfully read the transaction record, so maybe the data is 
            // valid after all - if the signature matches and the state is
            // TX_DATA_VALID, the data is valid, and therefore we are
            // not in a transaction.

#ifdef FLASH_DEBUG
            FLASH_PRINTF("read from %X\r\n",segmentMap[type].txOffset);
            FLASH_PRINTF("txRecord.signature:%X\r\n",txRecord.signature);
            FLASH_PRINTF("txRecord.txState:%X\r\n",txRecord.txState);
#endif
            inTransaction = !(txRecord.signature == TX_SIGNATURE && 
                             txRecord.txState == TX_DATA_VALID);
        }
        // Close the flash.
        HalFlashDrvClose(fd);
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF(
        "End IzotPersistentIsInTransaction(%s), inTransaction = %d\r\n",  
        GetPersistentName(type), inTransaction
        );  
    }
#endif
    return(inTransaction);
}

/* 
 *  Callback: IzotPersistentEnterTransaction
 *  Initiate a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API when the first request 
 *  arrives to update data stored in the specified segment.  The API updates 
 *  the non-persistent image, and schedules writes to update the non-volatile 
 *  storage at a later time.  
 */
const IzotApiError IzotPersistentEnterTransaction(
const IzotPersistentSegmentType type
)
{
    IzotApiError sts = IzotApiNoError;
    void *fd = NULL;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotPersistentEnterTransaction(%s)\r\n", 
                                                    GetPersistentName(type));  
    }
#endif
    
    // Open flash.
    sts = OpenFlash(type, &fd);
    if (sts == IzotApiNoError) {
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
        // by IzotPersistentOpenForWrite).
        // If the transaction record was not valid in the first place, writing 
        // this pattern will not make it valid - and so the transaction is
        // still in progress because we consider that a transaction is in 
        // progress any time the transaction record is not valid.
        
#ifdef FLASH_DEBUG
        FLASH_PRINTF("writing at %X\r\n",segmentMap[type].txOffset);
        FLASH_PRINTF("txRecord.signature: %X\r\n",txRecord.signature);
        FLASH_PRINTF("txRecord.txState: %X\r\n",txRecord.txState);
#endif
        if (HalFlashDrvWrite(fd, (uint8_t *)&txRecord, 
        sizeof(txRecord), segmentMap[type].txOffset) != 0) {
            sts = IzotApiPersistentFileError;
        }

        // Done, close the flash.
        HalFlashDrvClose(fd);
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotPersistentEnterTransaction(%s), sts = %d\r\n",  
               GetPersistentName(type), sts);  
    }
#endif
    return(sts);
}

/* 
 *  Callback: IzotPersistentExitTransaction
 *  Complete a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API after 
 *  <IzotPersistentWrite> has returned success and there are no further 
 *  updates required.   
 */
const IzotApiError IzotPersistentExitTransaction(
const IzotPersistentSegmentType type
)
{
    IzotApiError sts = IzotApiNoError;
    void *fd = NULL;

#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("Start IzotPersistentExitTransaction(%s)\r\n", 
                                                      GetPersistentName(type));
    }
#endif

    // Open the flash. */
    sts = OpenFlash(type, &fd);
    if (sts == IzotApiNoError) {
        PersistentTransactionRecord txRecord;
        txRecord.signature = TX_SIGNATURE;
        txRecord.txState = TX_DATA_VALID; 

        // We expect that this should work because this function is only 
        // called after successfully updating the data segment.  First the IZOT 
        // protocol stack calls IzotPersistentOpenForWrite, which will erase 
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
        FLASH_PRINTF("writing at %X\r\n",segmentMap[type].txOffset);
        FLASH_PRINTF("txRecord.signature: %X\r\n",txRecord.signature);
        FLASH_PRINTF("txRecord.txState: %X\r\n",txRecord.txState);
#endif

        if (HalFlashDrvWrite(fd, (uint8_t *)&txRecord, 
        sizeof(txRecord), segmentMap[type].txOffset) != 0) {
            sts = IzotApiPersistentFileError;
        }
        HalFlashDrvClose(fd);
    }
#ifdef FLASH_DEBUG
    if (persistentTraceEnabled) {
        PrintTimeStamp();
        FLASH_PRINTF("End IzotPersistentExitTransaction(%d), sts = %d\r\n", 
                                                                    type, sts);  
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
    EraseSegment(
    NULL, IzotPersistentSegApplicationData, 
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
    EraseSegment(
    NULL, IzotPersistentSegNetworkImage, 
    segmentMap[IzotPersistentSegNetworkImage].maxDataSize);
}

void EraseSecIIPersistentData(void)
{
    EraseSegment(NULL, IzotPersistentSegSecurityII, segmentMap[IzotPersistentSegSecurityII].maxDataSize);
}

/* 
 *  Function: OpenFlash
 *  Open a flash segment, initializing the segmentMap if necessary.
 *
 *  Parameters:
 *  type - segment type
 *  pFd -  pointer to flash file descriptor, to be returned by this function.
 *
 *  Returns:
 *  <IzotApiError>.   
 */
static IzotApiError OpenFlash(const IzotPersistentSegmentType type, void **pFd)
{
    IzotApiError sts = IzotApiNoError;
    if (type > IzotPersistentSegNumSegmentTypes) {
        // Invalid data type.
        sts = IzotApiPersistentFileError;
    } else {
        // Initialize segmentMap, if necessary.
        sts = InitSegmentMap(type);
        if (sts == IzotApiNoError) {
            // Open the flash.
            *pFd = (void *)HalFlashDrvOpen(0);
            if (*pFd == 0) {
                sts = IzotApiPersistentFileError;
            }
        }
    }
    return(sts);
}

/* 
 *  Function: InitSegmentMap
 *  If it has not already been done, initialize the segmentMap for the 
 *  specified type.  The first time this function is called, it must determine
 *  the flash size by opening the flash and reading the parameters from the HAL.
 *  Note that the segment types must be initialized in order.
 *
 *  Parameters:
 *  type - segment type
 *
 *  Returns:
 *  <IzotApiError>.   
 */
static IzotApiError InitSegmentMap(const IzotPersistentSegmentType type)
{
    IzotApiError sts = IzotApiNoError;

    sts = HalFlashDrvInit();
    if (sts != 0) {
        sts = IzotApiPersistentFileError;
    }

    // flashBlockSize is initially 0, and will be updated by this function.
    if (flashBlockSize == 0) {
        void *fd;

        // Open the flash
        fd = (void *)HalFlashDrvOpen(0);
        if (fd == 0) {
            sts = IzotApiPersistentFileError;
        } else {
            unsigned long offset;// Offset of this region from start of flash
            unsigned long region_size;// Size of this erase region
            int number_of_blocks;     // Number of blocks in this region
            unsigned long block_size; // Size of each block in this erase region
            int number_of_regions;
            
            // Get the flash information from the HAL.  */
            if ((HalGetFlashInfo(fd, &offset, &region_size, 
            &number_of_blocks, &block_size, &number_of_regions) == 0) && 
            number_of_regions >= 1) {
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
    }
    if (dataSegmentSize[type] == 0 && type != IzotPersistentSegNodeDefinition 
    && type != IzotPersistentSegUniqueId) {
        int flashOffset;
        int blockOffset;
    
        dataSegmentSize[type] = 
                IzotPersistentGetMaxSize(type) + GetPersistentHeaderSize();
    
        // The segments are allocated starting at the highest 
        // address of the flash region.  Starting at 
        // lowestUsedFlashDataOffset, adjust the offset 
        // to allow for the transaction record and data segment.
    
        flashOffset = lowestUsedFlashDataOffset -
            (sizeof(PersistentTransactionRecord) + dataSegmentSize[type]);
    
        blockOffset = flashOffset % flashBlockSize;
    
        // Adjust offset to start on a block boundary */
        flashOffset -= blockOffset;
    
        // NOTE: This function assumes that code is loaded into 
        // low flash and that there is enough room to fit the data 
        // without overwriting the code.
    
        segmentMap[type].segmentStart = flashOffset;
        segmentMap[type].txOffset = flashOffset;
        segmentMap[type].dataOffset = flashOffset + 
                                   sizeof(PersistentTransactionRecord);
        segmentMap[type].maxDataSize = dataSegmentSize[type];
#ifdef FLASH_DEBUG
        FLASH_PRINTF("segmentMap[%d].segmentStart: %X\r\n", 
                                        type, segmentMap[type].segmentStart);
        FLASH_PRINTF("segmentMap[%d].txOffset: %X\r\n", 
                                          type, segmentMap[type].txOffset);
        FLASH_PRINTF("segmentMap[%d].dataOffset: %X\r\n", 
                                          type, segmentMap[type].dataOffset);
        FLASH_PRINTF("segmentMap[%d].maxDataSize: %X\r\n", 
                                          type, segmentMap[type].maxDataSize);
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
 *  type - segment type
 *  size - size of data, in bytes
 *
 *  Returns:
 *  <IzotApiError>.   
 *
 *  Remarks:
 *  After erasing the flash, all of the bytes will be 0xff.  Writing to the 
 *  flash can only change 1 bits to 0 bits.
 */
static IzotApiError EraseSegment(void *fd, const IzotPersistentSegmentType type, 
                                int size)
{
    IzotApiError sts = IzotApiNoError;

    // Get the start of the segment.
    int offset = segmentMap[type].segmentStart;
        // Keep erasing blocks until bytesErased >= size.
    int bytesErased = 0;

    // Erase blocks, a block at a time, until the request has been satisfied.
    for (bytesErased = 0; bytesErased < size; bytesErased += flashBlockSize) {
        // Erase the block.
        if (HalFlashDrvErase(fd, offset, flashBlockSize) != 0) {
            sts = IzotApiPersistentFileError;
            break;
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
    int time = IzotGetTickCount()*1000/GetTicksPerSecond();
    FLASH_PRINTF("[%d.%.3d]", time/1000, time % 1000);
}

/* 
 *  Function: GetPersistentName
 *  Translate a segment type to a name, used for PERSISTENT tracing.
 *
 *  Parameters:
 *  type -  type of non-volatile data to be opened
 *
 *  Returns:
 *  character string representing the PERSISTENT segment.  
 *
 */
static const char *GetPersistentName(IzotPersistentSegmentType type)
{
    const char *name = NULL;
    switch (type) {
    case IzotPersistentSegNetworkImage:
        name = "IzotPersistentSegNetworkImage";
        break;
    case IzotPersistentSegNodeDefinition:
          name = "IzotPersistentSegNodeDefinition";
        break;
    case IzotPersistentSegApplicationData:
        name = "IzotPersistentSegApplicationData";
        break;
    case IzotPersistentSegUniqueId:
        name = "IzotPersistentSegUniqueId";
        break;
    case IsiPersistentSegConnectionTable:
        name = "IsiPersistentSegConnectionTable";
        break;
    case IsiPersistentSegPersistent:
        name = "IsiPersistentSegPersistent";
        break;
    default:
        name = "????";
    }
    return name;
}
#endif
