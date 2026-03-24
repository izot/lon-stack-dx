/*
 * Persistent.c
 *
 * Copyright (c) 2023-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Persistent Memory Management
 * Purpose: Provides functions to manage persistent memory storage.
 * Notes:   This module handles reading and writing data to hardware flash
 *          memory, ensuring data integrity with a persistent header
 *          containing an application signature.
 */

#include "persistence/lon_persistence.h"
#include "persistence/storage_persistence.h"

#define WAIT_FOREVER                -1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define CURRENT_VERSION             1

/*****************************************************************
 * Section: Globals
 *****************************************************************/
extern IzotPersistentSegDeserializeFunction izot_deserialize_handler;
extern IzotPersistentSegSerializeFunction izot_serialize_handler;
 
static unsigned      app_signature;
static unsigned long guard_band_duration = 1000;
static unsigned long last_update = 0;
static IzotBool      commit_flag = FALSE;
static IzotBool      scheduled = FALSE;
static IzotBool      persistence_list[IzotPersistentSegNumSegmentTypes];

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/
/*
 * Function: IzotPersistentMemGuardBandRemaining
 * This function calculates the time remaining for the flushing.
 */
static uint32_t IzotPersistentMemGuardBandRemaining(void)
{
    uint32_t timeElapsed = OsalGetTickCount() - last_update;
    uint32_t timeRemaining;
    if (timeElapsed <= guard_band_duration) {
        timeRemaining = guard_band_duration - timeElapsed;
    } else {
        timeRemaining = 0;  /* already expired. */
    }
    return(timeRemaining);
} 

/*
 * Function: IzotPersistentSegDeserializeNetworkImage
 * This function deserializes the network image persistent segment.
 */
static LonStatusCode IzotPersistentSegDeserializeNetworkImage(
        void* const pData, int len)
{
    LonStatusCode status = LonStatusNoError;
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegNetworkImage);
    if (len >= imageLen) {
        (void)memcpy((void*)(&eep->configData), (char* const)pData, imageLen);
    } else {
        status = LonStatusPersistentDataFailure;
        OsalPrintLog(ERROR_LOG, status,
                "IzotPersistentSegDeserializeNetworkImage: Data length %d is less than expected %d",
                len, imageLen);
    }
    return status;
}

/*
 * Function: IzotPersistentSegSerializeNetworkImage
 * This function serializes the network image persistent segment.
 */
static LonStatusCode IzotPersistentSegSerializeNetworkImage(
        IzotByte** pData, size_t *len)
{
    size_t imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegNetworkImage);
    *pData = (IzotByte *) OsalAllocateMemory(imageLen);
    *len = imageLen;
    memcpy((void *)*pData, (const void*)(&eep->configData), imageLen);
    return LonStatusNoError;
}

/*
 * Function: IzotPersistentSegDeserializeAppDataImage
 * This function deserializes the application data persistent segment.
 */
static LonStatusCode IzotPersistentSegDeserializeAppDataImage(
  void* const pData, int len)
{
    LonStatusCode status = LonStatusNoError;
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegApplicationData);
    if (izot_deserialize_handler != NULL) {
        if (len >= imageLen) {
            status = izot_deserialize_handler(pData, imageLen);
        } else {
            status = LonStatusPersistentDataFailure;
            OsalPrintLog(ERROR_LOG, status,
                    "IzotPersistentSegDeserializeAppDataImage: Data length %d is less than expected %d",
                    len, imageLen);
        }
    } else {
        status = LonStatusStackNotInitialized;
    }
    return status;
}

/*
 * Function: IzotPersistentSegSerializeAppDataImage
 * This function serializes the application data persistent segment.
 */
static LonStatusCode IzotPersistentSegSerializeAppDataImage(IzotByte** pData, size_t *len)
{
    size_t imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegApplicationData);
    
    *pData = (IzotByte *) OsalAllocateMemory(imageLen);
    *len = imageLen;
    LonStatusCode status = LonStatusNoError;
    if (izot_serialize_handler != NULL) {
        status = izot_serialize_handler(*pData, imageLen);
    } else {
        status = LonStatusStackNotInitialized;
    }
    return status;
}

/*
 * Function: IzotPersistentSegStore
 * This function stores the information of a given type into the non-volatile.
 * memory
 */
static LonStatusCode IzotPersistentSegStore(IzotPersistentSegType persistent_seg_type)
{
    IzotByte* pImage = NULL;
    IzotPersistenceHeader hdr;
    size_t imageLen = 0;
    LonStatusCode status = LonStatusNoError;

    IzotPersistentSegEnterTransaction(persistent_seg_type);
    hdr.version = CURRENT_VERSION;
    hdr.length = 0;
    hdr.isiSignature = ISI_IMAGE_SIGNATURE0;
    hdr.checksum = 0;
    hdr.appSignature = app_signature;
    if (persistent_seg_type == IzotPersistentSegNetworkImage) {
        status = IzotPersistentSegSerializeNetworkImage(&pImage, &imageLen);
    } else if (persistent_seg_type == IzotPersistentSegApplicationData) {
        status = IzotPersistentSegSerializeAppDataImage(&pImage, &imageLen);
#ifdef SECURITY_II
    } else if (persistent_seg_type == IzotPersistentSegSecurityII) {
        status = serializeSecurityIIData(&pImage, &imageLen);
#endif
    } 
    if (status == LonStatusNoError) {
        hdr.checksum = ComputePersistenceChecksum(pImage, imageLen);
        hdr.length = imageLen;
        IzotPersistentSegType returnedSegType = 
                IzotPersistentSegOpenForWrite(persistent_seg_type, sizeof(hdr) + hdr.length);
        if (returnedSegType != IzotPersistentSegUnassigned) {
            if (IzotPersistentSegWrite(persistent_seg_type, sizeof(PersistentTransactionRecord), sizeof(hdr), &hdr) != 0 ||
                IzotPersistentSegWrite(persistent_seg_type, sizeof(PersistentTransactionRecord) + sizeof(hdr), hdr.length, pImage) != 0) {
                status = LonStatusPersistentDataFailure;
                OsalPrintLog(ERROR_LOG, status,
                        "IzotPersistentSegStore: Failed to write %s segment",
                        IzotPersistentGetSegName(persistent_seg_type));
            }
            IzotPersistentSegClose(persistent_seg_type);
        }
        if (status != LonStatusNoError) {
            IzotPersistentMemReportFailure();
        } else {
            IzotPersistentSegExitTransaction(persistent_seg_type);
            OsalPrintLog(INFO_LOG, status, "IzotPersistentSegStore: %s segment stored successfully", 
                    IzotPersistentGetSegName(persistent_seg_type));
        }
        if (pImage != NULL) {
            OsalFreeMemory(pImage);
        }
    }
    return status;
}

/*
 * Function: IzotPersistentMemCommit
 * This funtion checks the persistence flag for each persistend segment
 * and commits data to any segments with the flag set.
 */
static void IzotPersistentMemCommit(void)
{
    unsigned int i;
    OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotPersistentMemCommit: Checking for required NVM writes");
    for (i = 0; i < IzotPersistentSegNumSegmentTypes; i++) {
        if (persistence_list[i] != FALSE) {
            OsalPrintLog(INFO_LOG, LonStatusNoError, "IzotPersistentMemCommit: Committing segment %s", IzotPersistentGetSegName(i));
            if (IzotPersistentSegStore(i) == LonStatusNoError) {
                persistence_list[i] = FALSE;
                commit_flag = FALSE;
                OsalSleep(20);
            }
        }
    }
    scheduled = FALSE;
}

/*
 * Function: IzotPersistentSegGetHeaderSize
 * This function computes the persistent header size
 */
unsigned IzotPersistentSegGetHeaderSize(void) 
{
    return sizeof(IzotPersistenceHeader);
}
 
/*
 * Function: ComputePersistenceChecksum
 * This function computes the checksum on data to be stored in flash.
 */
uint8_t ComputePersistenceChecksum(uint8_t* pImage, size_t length)
{
    size_t i; 
    uint8_t checksum = 0;
    for (i = 0; i < length - 1; i++) {
        checksum += pImage[i];
    }
    checksum += (unsigned short) length;
    return checksum;
} 

/*
 * Function: ValidatePersistenceChecksum
 * This function validates the checksum from the data read from flash.
 */
bool ValidatePersistenceChecksum(IzotPersistenceHeader *pHdr, uint8_t *pImage)
{
    bool result = true;
    // Can't checksum signature 0 checksum.
    if (pHdr->appSignature != app_signature) {
        if (ComputePersistenceChecksum(pImage, pHdr->length) != pHdr->checksum) {
            result = false;
        }
    }
    return result;
}

/*
 * Function: SetAppSignature
 * This function sets the application signature.
 */
void SetAppSignature(unsigned appSignature)
{
    app_signature = appSignature;
}

/*
 * Function: GetAppSignature
 * This function returns the application signature.
 */
unsigned GetAppSignature(void)
{
    return app_signature;
}

/*
 * Function: SetPeristenceGuardBand
 * This function sets guardband duration .
 */
void SetPeristenceGuardBand(int nTime)
{
    int ticks;
    
    // Convert milliseconds to tick counts in a platform independent way
    ticks = nTime * OsalGetTicksPerSecond() / 1000; 
    if (ticks == 0) {
        ticks = 1;
    }
    
    guard_band_duration = ticks;
}

/*
 * Function: IzotPersistentSegSetCommitFlag
 * This function flags a persistent segment to be committed.
 */
void IzotPersistentSegSetCommitFlag(IzotPersistentSegType persistent_seg_type)
{
    persistence_list[persistent_seg_type] = TRUE;
} 

/*
 * Function: IzotPersistentMemStartCommitTimer
 * This function starts the commit timer if it is not already running.
 * Persistent data is committed to the persistent memory when the time
 * expires.
 */
void IzotPersistentMemStartCommitTimer(void)
{
    if (!scheduled) {
        last_update = OsalGetTickCount();
    }

    scheduled = TRUE;
}

/*
 * Function: IzotPersistentMemReportFailure
 * This function reports a persistent memory write failure.
 */
void IzotPersistentMemReportFailure(void)
{
    OsalPrintLog(ERROR_LOG, LonStatusEepromWriteFail, 
        "IzotPersistentMemReportFailure: Persistent memory write failure");
}

/*
 * Function: IzotPersistentMemCommitCheck
 * This funtion checks the commit timer and flag, and commits data to
 * persistent memory if the timer has expired or the commit flag is set.
 */
void IzotPersistentMemCommitCheck(void)
{
    unsigned long guardTimeLeft = IzotPersistentMemGuardBandRemaining();  

    if (scheduled) {
        if (guardTimeLeft == 0 || commit_flag) {
            IzotPersistentMemCommit();
        }
    }
}

/*
 * Function: IzotPersistentMemSetCommitFlag
 * This function sets the persistent memory commit flag to force a 
 * commit on the next commit check.
 */
void IzotPersistentMemSetCommitFlag(void)
{
    commit_flag = TRUE;
}

/*
 * Function: IzotPersistentSegRestore
 * This function restores the specified memory segment contents to RAM.
 */
LonStatusCode IzotPersistentSegRestore(IzotPersistentSegType persistent_seg_type)
{
    LonStatusCode status = LonStatusNoError;
    IzotPersistenceHeader hdr;
    IzotByte* pImage = NULL;
    size_t imageLength = 0;

    if (IzotPersistentSegIsInTransaction(persistent_seg_type)) {
        status = LonStatusPersistentDataFailure;
        OsalPrintLog(ERROR_LOG, status,
                "IzotPersistentSegRestore: Segment %s is in a transaction",
                IzotPersistentGetSegName(persistent_seg_type));
    } else {
        IzotPersistentSegType returnedSegType = IzotPersistentSegOpenForRead(persistent_seg_type);
        memset(&hdr, 0, sizeof(hdr));
        if (persistent_seg_type != IzotPersistentSegUnassigned) {
            if (IzotPersistentSegRead(persistent_seg_type, sizeof(PersistentTransactionRecord), sizeof(hdr), &hdr) != 0) {
                status = LonStatusPersistentDataFailure;
                OsalPrintLog(ERROR_LOG, status,
                        "IzotPersistentSegRestore: Failed to read header of segment %s",
                        IzotPersistentGetSegName(persistent_seg_type));
            } else if (hdr.isiSignature != ISI_IMAGE_SIGNATURE0) {
                status = LonStatusPersistentDataFailure;
                OsalPrintLog(ERROR_LOG, status,
                        "IzotPersistentSegRestore: Invalid ISI signature of %d in segment %s, expected %d",
                        hdr.isiSignature, IzotPersistentGetSegName(persistent_seg_type), ISI_IMAGE_SIGNATURE0);
            } else if (hdr.appSignature != app_signature) {
                status = LonStatusPersistentDataFailure;
                OsalPrintLog(ERROR_LOG, status,
                        "IzotPersistentSegRestore: Application signature mismatch in segment %s",
                        IzotPersistentGetSegName(persistent_seg_type));
            } else if (hdr.version > CURRENT_VERSION) {
                status = LonStatusPersistentDataFailure;
                OsalPrintLog(ERROR_LOG, status,
                        "IzotPersistentSegRestore: Unsupported version %d in segment %s",
                        hdr.version, IzotPersistentGetSegName(persistent_seg_type));
            } else {
                imageLength = hdr.length;
                pImage = (IzotByte *) OsalAllocateMemory(imageLength);
                if (pImage == NULL ||
                    IzotPersistentSegRead(persistent_seg_type, sizeof(PersistentTransactionRecord) + sizeof(hdr), imageLength, pImage) != 0 ||
                    !ValidatePersistenceChecksum(&hdr, pImage)) {
                    status = LonStatusPersistentDataFailure;
                    OsalPrintLog(ERROR_LOG, status,
                            "IzotPersistentSegRestore: Checksum validation failed for segment %s",
                            IzotPersistentGetSegName(persistent_seg_type));
                    OsalFreeMemory(pImage);
                    pImage = NULL;
                }
            }
            IzotPersistentSegClose(persistent_seg_type);
        } else {
            status = LonStatusPersistentDataFailure;
            OsalPrintLog(ERROR_LOG, status,
                    "IzotPersistentSegRestore: Cannot open segment %s for read",
                    IzotPersistentGetSegName(persistent_seg_type));
        }
    }

    if (status == LonStatusNoError) {
        switch(persistent_seg_type) {
        case IzotPersistentSegNetworkImage:
            status = 
                IzotPersistentSegDeserializeNetworkImage(pImage, imageLength);
            break;
        case IzotPersistentSegApplicationData:
            status = 
                IzotPersistentSegDeserializeAppDataImage(pImage, imageLength);
            break;

#ifdef SECURITY_II
        case IzotPersistentSegSecurityII:
            status = deserializeSecurityIIData(pImage, imageLength);
            break;
#endif

        default:
            status = LonStatusPersistentDataFailure;
            OsalPrintLog(ERROR_LOG, status,
                    "IzotPersistentSegRestore: Invalid segment type %d",
                    persistent_seg_type);
        }
    }
    
    if (pImage != NULL) {
        OsalFreeMemory(pImage);
    }
    
    return status;
}

#ifdef SECURITY_II
LonStatusCode restoreSecurityIIData(void)
{
    return IzotPersistentSegRestore(IzotPersistentSegSecurityII);
}
#endif

/*
 * Function: IzotPersistentSegCommitScheduled
 * This function checks whether any persistent data is scheduled to
 * be committed.  If so, it sets the commit flag to force an immediate
 * commit of that data.  This function is typically called when a
 * reset is requested, to ensure that all persistent data is committed
 * before the reset.
 */
IzotBool IzotPersistentSegCommitScheduled(void)
{
    unsigned int i;
    IzotBool isScheduled = FALSE;

    for (i = 0; i < IzotPersistentSegNumSegmentTypes; i++) {
        if (persistence_list[i]) {
            isScheduled = TRUE;
            // If scheduled then do immediate commit of that data.
            IzotPersistentMemSetCommitFlag();
            break;
        }
    }
    return isScheduled;
}