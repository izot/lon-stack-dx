/*
 * Persistent.c
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

#include "persistence/lon_persistence.h"

#define WAIT_FOREVER                -1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define CURRENT_VERSION             1

/*****************************************************************
 * Section: Globals
 *****************************************************************/
extern IzotPersistentSegDeserializeFunction izot_deserialize_handler;
extern IzotPersistentSegSerializeFunction izot_serialize_handler;
 
static unsigned      m_appSignature;
static unsigned long m_guardBandDuration = 1000;
static unsigned long lastUpdate = 0;
static IzotBool      commitFlag = FALSE;
static IzotBool      scheduled = FALSE;
static IzotBool      PersitenceList[IzotPersistentSegNumSegmentTypes];

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/
/*
 * Function: IzotPersistentMemGuardBandRemaining
 * This function calculates the time remaining for the flushing.
 */
static uint32_t IzotPersistentMemGuardBandRemaining(void)
{
    uint32_t timeElapsed = OsalGetTickCount() - lastUpdate;
    uint32_t timeRemaining;
    if (timeElapsed <= m_guardBandDuration) {
        timeRemaining = m_guardBandDuration - timeElapsed;
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
    LonStatusCode reason = LonStatusNoError;
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegNetworkImage);
    
    if (len >= imageLen) {
        (void)memcpy((void*)(&eep->configData), (char* const)pData, imageLen);
    } else {
        return LonStatusPersistentDataFailure;
    }
    
    return reason;
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
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegApplicationData);
    
    if (izot_deserialize_handler != NULL) {
        if (len >= imageLen) {
            return izot_deserialize_handler(pData, imageLen);
        } else {
            return LonStatusPersistentDataFailure;
        }
    } else {
        return LonStatusStackNotInitialized;
    }
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
    
    if (izot_serialize_handler != NULL) {
        return izot_serialize_handler(*pData, imageLen);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*
 * Function: IzotPersistentSegStore
 * This function stores the information of a given type into the non-volatile.
 * memory
 */
static LonStatusCode IzotPersistentSegStore(IzotPersistentSegType persistentSegType)
{
    IzotByte* pImage = NULL;
    IzotPersistenceHeader hdr;
    size_t imageLen = 0;
    LonStatusCode reason = LonStatusNoError;

    IzotPersistentSegEnterTransaction(persistentSegType);
    
    hdr.version = CURRENT_VERSION;
    hdr.length = 0;
    hdr.signature = ISI_IMAGE_SIGNATURE0;
    hdr.checksum = 0;
    hdr.appSignature = m_appSignature;
    
    if (persistentSegType == IzotPersistentSegNetworkImage) {
        reason = IzotPersistentSegSerializeNetworkImage(&pImage, &imageLen);
    } else if (persistentSegType == IzotPersistentSegApplicationData) {
        reason = IzotPersistentSegSerializeAppDataImage(&pImage, &imageLen);
#ifdef SECURITY_II
    } else if (type == IzotPersistentSegSecurityII) {
        reason = serializeSecurityIIData(&pImage, &imageLen);
#endif
    } 
    
    if (reason == LonStatusNoError) {
        hdr.checksum = ComputePersistenceChecksum(pImage, imageLen);
        hdr.length = imageLen;

        IzotPersistentSegType returnedSegType = 
          IzotPersistentSegOpenForWrite(persistentSegType, sizeof(hdr) + hdr.length);
        if (returnedSegType != IzotPersistentSegUnassigned) {
            if (IzotPersistentSegWrite(persistentSegType, 0, sizeof(hdr), &hdr) != 0 ||
                IzotPersistentSegWrite(persistentSegType, sizeof(hdr), hdr.length, pImage) != 0) {
                reason = LonStatusPersistentDataFailure;
            }
            IzotPersistentSegClose(persistentSegType);
        }
        
        if (reason != LonStatusNoError) {
            IzotPersistentMemReportFailure();
        } else {
            IzotPersistentSegExitTransaction(persistentSegType);
        }
        
        if (pImage != NULL) {
            OsalFreeMemory(pImage);
        }
    }
    
    return reason;
}

/*
 * Function: IzotPersistentMemCommit
 * This funtion checks the persistence flag for each persistend segment
 * and commits data to any segments with the flag set.
 */
static void IzotPersistentMemCommit(void)
{
    unsigned int i;

    for (i = 0; i < IzotPersistentSegNumSegmentTypes; i++) {
        if (PersitenceList[i] != FALSE) {
            if (IzotPersistentSegStore(i) == LonStatusNoError) {
                PersitenceList[i] = FALSE;
                commitFlag = FALSE;
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
    if (pHdr->signature != m_appSignature) {
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
    m_appSignature = appSignature;
}

/*
 * Function: GetAppSignature
 * This function returns the application signature.
 */
unsigned GetAppSignature(void)
{
    return m_appSignature;
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
    
    m_guardBandDuration = ticks;
}

/*
 * Function: IzotPersistentSegSetCommitFlag
 * This function flags a persistent segment to be committed.
 */
void IzotPersistentSegSetCommitFlag(IzotPersistentSegType persistentSegType)
{
    PersitenceList[persistentSegType] = TRUE;
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
        lastUpdate = OsalGetTickCount();
    }

    scheduled = TRUE;
}

/*
 * Function: IzotPersistentMemReportFailure
 * This function reports a persistent memory write failure.
 */
void IzotPersistentMemReportFailure(void)
{
    OsalPrintError(LonStatusEepromWriteFail, 
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
        if (guardTimeLeft == 0 || commitFlag) {
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
    commitFlag = TRUE;
}

/*
 * Function: IzotPersistentSegRestore
 * This function restores the specified memory segment contents to RAM.
 */
LonStatusCode IzotPersistentSegRestore(IzotPersistentSegType persistentSegType)
{
    LonStatusCode reason = LonStatusNoError;
    IzotPersistenceHeader hdr;
    IzotByte* pImage = NULL;
    size_t imageLength = 0;

    if (IzotPersistentSegIsInTransaction(persistentSegType)) {
        reason = LonStatusPersistentDataFailure;
    } else {
        IzotPersistentSegType returnedSegType = IzotPersistentSegOpenForRead(persistentSegType);
        memset(&hdr, 0, sizeof(hdr));
        if (persistentSegType != IzotPersistentSegUnassigned) {
            if (IzotPersistentSegRead(persistentSegType, 0, sizeof(hdr), &hdr) != 0) {
                reason = LonStatusPersistentDataFailure;
            } else if (hdr.signature != ISI_IMAGE_SIGNATURE0) {
                reason = LonStatusPersistentDataFailure;
            } else if (hdr.appSignature != m_appSignature) {
                reason = LonStatusPersistentDataFailure;
            } else if (hdr.version > CURRENT_VERSION) {
                reason = LonStatusPersistentDataFailure;
            } else {
                imageLength = hdr.length;
                pImage = (IzotByte *) OsalAllocateMemory(imageLength);
                if (pImage == NULL ||
                    IzotPersistentSegRead(persistentSegType, sizeof(hdr), imageLength, pImage) != 0 ||
                    !ValidatePersistenceChecksum(&hdr, pImage)) {
                    reason = LonStatusPersistentDataFailure;
                    OsalFreeMemory(pImage);
                    pImage = NULL;
                }
            }
            IzotPersistentSegClose(persistentSegType);
        } else {
            reason = LonStatusPersistentDataFailure;
        }
    }

    if (reason == LonStatusNoError) {
        switch(persistentSegType) {
        case IzotPersistentSegNetworkImage:
            reason = 
                IzotPersistentSegDeserializeNetworkImage(pImage, imageLength);
            break;
        case IzotPersistentSegApplicationData:
            reason = 
                IzotPersistentSegDeserializeAppDataImage(pImage, imageLength);
            break;

#ifdef SECURITY_II
        case IzotPersistentSegSecurityII:
            reason = deserializeSecurityIIData(pImage, imageLength);
            break;
#endif

        default:
            reason = LonStatusPersistentDataFailure;
        }
    }
    
    if (pImage != NULL) {
        OsalFreeMemory(pImage);
    }
    
    return reason;
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
        if (PersitenceList[i]) {
            isScheduled = TRUE;
            // If scheduled then do immediate commit of that data.
            IzotPersistentMemSetCommitFlag();
            break;
        }
    }
    return isScheduled;
}