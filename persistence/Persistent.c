//
// Persitent.c
//
// Copyright (C) 2023-2025 EnOcean
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
 * API to store the information in the hardware flash memory.  The API manages the
 * information with a peristent header. The persistent header contains the
 * application signature to verify the information while reading or writing 
 * to the flash memory.
 */

#include "persistence/Persistent.h"

/*------------------------------------------------------------------------------
  Section: Macros
  ------------------------------------------------------------------------------*/
#define WAIT_FOREVER                -1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define CURRENT_VERSION             1

/*------------------------------------------------------------------------------
  Section: Global
  ------------------------------------------------------------------------------*/
extern IzotPersistentSegDeserializeFunction izot_deserialize_handler;
extern IzotPersistentSegSerializeFunction izot_serialize_handler;
 
/*------------------------------------------------------------------------------
  Section: Static
  ------------------------------------------------------------------------------*/
static unsigned      m_appSignature;
static unsigned long m_guardBandDuration = 1000;
static unsigned long lastUpdate = 0;
static IzotBool      commitFlag = FALSE;
static IzotBool      scheduled = FALSE;
static IzotBool      PersitenceList[IzotPersistentSegNumSegmentTypes];

/*
 * *****************************************************************************
 * Section: Functions
 * *****************************************************************************
 */

/*
 * Function: IzotPersistentMemGuardBandRemaining
 * This function calculates the time remaining for the flushing.
 */
static uint32_t IzotPersistentMemGuardBandRemaining(void)
{
    uint32_t timeElapsed = IzotGetTickCount() - lastUpdate;
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
static IzotApiError IzotPersistentSegDeserializeNetworkImage(
        void* const pData, int len)
{
    IzotApiError reason = IzotApiNoError;
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegNetworkImage);
    
    if (len >= imageLen) {
        (void)memcpy((void*)(&eep->configData), (char* const)pData, imageLen);
    } else {
        return IzotApiPersistentFailure;
    }
    
    return reason;
}

/*
 * Function: IzotPersistentSegSerializeNetworkImage
 * This function serializes the network image persistent segment.
 */
static IzotApiError IzotPersistentSegSerializeNetworkImage(
        IzotByte** pData, int *len)
{
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegNetworkImage);

    *pData = (IzotByte *) OsalMalloc(imageLen);
    *len = imageLen;
    
    memcpy((void *)*pData, (const void*)(&eep->configData), imageLen);
    
    return IzotApiNoError;
}

/*
 * Function: IzotPersistentSegDeserializeAppDataImage
 * This function deserializes the application data persistent segment.
 */
static IzotApiError IzotPersistentSegDeserializeAppDataImage(
  void* const pData, int len)
{
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegApplicationData);
    
    if (izot_deserialize_handler != NULL) {
        if (len >= imageLen) {
            return izot_deserialize_handler(pData, imageLen);
        } else {
            return IzotApiPersistentFailure;
        }
    } else {
        return IzotApiNotInitialized;
    }
}

/*
 * Function: IzotPersistentSegSerializeAppDataImage
 * This function serializes the application data persistent segment.
 */
static IzotApiError IzotPersistentSegSerializeAppDataImage(IzotByte** pData, int *len)
{
    int imageLen = IzotPersistentSegGetMaxSize(IzotPersistentSegApplicationData);
    
    *pData = (IzotByte *) OsalMalloc(imageLen);
    *len = imageLen;
    
    if (izot_serialize_handler != NULL) {
        return izot_serialize_handler(*pData, imageLen);
    } else {
        return IzotApiNotInitialized;
    }
}

/*
 * Function: IzotPersistentSegStore
 * This function stores the information of a given type into the non-volatile.
 * memory
 */
static IzotApiError IzotPersistentSegStore(IzotPersistentSegType persistentSegType)
{
    IzotByte* pImage = NULL;
    IzotPersistenceHeader hdr;
    int imageLen = 0;
    IzotApiError reason = IzotApiNoError;

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
    
    if (reason == IzotApiNoError) {
        hdr.checksum = ComputeChecksum(pImage, imageLen);
        hdr.length = imageLen;

        IzotPersistentSegType returnedSegType = 
          IzotPersistentSegOpenForWrite(persistentSegType, sizeof(hdr) + hdr.length);
        if (returnedSegType != IzotPersistentSegUnassigned) {
            if (IzotPersistentSegWrite(persistentSegType, 0, sizeof(hdr), &hdr) != 0 ||
                IzotPersistentSegWrite(persistentSegType, sizeof(hdr), hdr.length, pImage) != 0) {
                reason = IzotApiPersistentFailure;
            }
            IzotPersistentSegClose(persistentSegType);
        }
        
        if (reason != IzotApiNoError) {
            IzotPersistentMemReportFailure();
        } else {
            IzotPersistentSegExitTransaction(persistentSegType);
        }
        
        if (pImage != NULL) {
            OsalFree(pImage);
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
            if (IzotPersistentSegStore(i) == IzotApiNoError) {
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
 * Function: ComputeChecksum
 * This function computes the checksum on data to be stored in flash.
 */
int ComputeChecksum(IzotByte* pImage, int length)
{
    int i; 
    unsigned short checksum = 0;
    for (i = 0; i < length - 1; i++) {
        checksum += pImage[i];
    }
    checksum += (unsigned short) length;
    return checksum;
} 

/*
 * Function: ValidateChecksum
 * This function validates the checksum from the data read from flash.
 */
IzotBool ValidateChecksum(IzotPersistenceHeader *pHdr, IzotByte *pImage)
{
    IzotBool result = TRUE;
    // Can't checksum signature 0 checksum.
    if (pHdr->signature != m_appSignature) {
        if (ComputeChecksum(pImage, pHdr->length) != pHdr->checksum) {
            result = FALSE;
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
    ticks = nTime * GetTicksPerSecond() / 1000; 
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
        lastUpdate = IzotGetTickCount();
    }

    scheduled = TRUE;
}

/*
 * Function: IzotPersistentMemReportFailure
 * This function reports a persistent memory write failure.
 */
void IzotPersistentMemReportFailure(void)
{
    LCS_RecordError(IzotEepromWriteFail);
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
IzotApiError IzotPersistentSegRestore(IzotPersistentSegType persistentSegType)
{
    IzotApiError reason = IzotApiNoError;
    IzotPersistenceHeader hdr;
    IzotByte* pImage = NULL;
    int imageLength = 0;

    if (IzotPersistentSegIsInTransaction(persistentSegType)) {
        reason = IzotApiPersistentFailure;
    } else {
        IzotPersistentSegType returnedSegType = IzotPersistentSegOpenForRead(persistentSegType);
        memset(&hdr, 0, sizeof(hdr));
        if (persistentSegType != IzotPersistentSegUnassigned) {
            if (IzotPersistentSegRead(persistentSegType, 0, sizeof(hdr), &hdr) != 0) {
                reason = IzotApiPersistentFailure;
            } else if (hdr.signature != ISI_IMAGE_SIGNATURE0) {
                reason = IzotApiPersistentFailure;
            } else if (hdr.appSignature != m_appSignature) {
                reason = IzotApiPersistentFailure;
            } else if (hdr.version > CURRENT_VERSION) {
                reason = IzotApiPersistentFailure;
            } else {
                imageLength = hdr.length;
                pImage = (IzotByte *) OsalMalloc(imageLength);
                if (pImage == NULL ||
                    IzotPersistentSegRead(persistentSegType, sizeof(hdr), imageLength, pImage) != 0 ||
                    !ValidateChecksum(&hdr, pImage)) {
                    reason = IzotApiPersistentFailure;
                    OsalFree(pImage);
                    pImage = NULL;
                }
            }
            IzotPersistentSegClose(persistentSegType);
        } else {
            reason = IzotApiPersistentFailure;
        }
    }

    if (reason == IzotApiNoError) {
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
            reason = IzotApiPersistentFailure;
        }
    }
    
    if (pImage != NULL) {
        OsalFree(pImage);
    }
    
    return reason;
}

#ifdef SECURITY_II
IzotApiError restoreSecurityIIData(void)
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