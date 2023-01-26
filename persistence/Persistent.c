//
// Persitent.c
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
 * API to store the information in the hardware flash memory.  The API manages the
 * information with a peristent header. The persistent header contains the
 * application signature to verify the information while reading or writing 
 * to the flash memory.
 */

#include "Persistent.h"

/*------------------------------------------------------------------------------
  Section: Macros
  ------------------------------------------------------------------------------*/
#define WAIT_FOREVER                -1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define CURRENT_VERSION             1

/*------------------------------------------------------------------------------
  Section: Global
  ------------------------------------------------------------------------------*/
extern IzotPersistentDeserializeSegmentFunction izot_deserialize_handler;
extern IzotPersistentSerializeSegmentFunction izot_serialize_handler;
 
/*------------------------------------------------------------------------------
  Section: static
  ------------------------------------------------------------------------------*/
static unsigned      m_appSignature;
static unsigned long m_guardBandDuration = 1000;
static unsigned long lastUpdate = 0;
static IzotBool      bSync = FALSE;
static IzotBool      scheduled = FALSE;
static IzotBool      PersitenceList[IzotPersistentSegNumSegmentTypes];

/*
 * *****************************************************************************
 * SECTION: FUNCTIONS
 * *****************************************************************************
 *
 */

/*
 * Function: guardBandRemainig
 * This function calculates the time remaining for the flushing .

 */
static unsigned guardBandRemainig(void)
{
    unsigned timeElapsed = IzotGetTickCount() - lastUpdate;
    unsigned timeRemaining;
    if (timeElapsed <= m_guardBandDuration) {
        timeRemaining = m_guardBandDuration - timeElapsed;
    } else {
        timeRemaining = 0;  /* already expired. */
    }
    return(timeRemaining);
} 

/*
 * Function: deserializeLcsEppromPersistentSegment
 * This function deserializes the EEPROM Persistent segment.

 */
static IzotApiError deserializeLcsEppromPersistentSegment(
void* const pData, 
int len
)
{
    IzotApiError reason = IzotApiNoError;
    int imageLen = IzotPersistentGetMaxSize(IzotPersistentSegNetworkImage);
    
    if (len >= imageLen) {
        (void)memcpy((void*)(&eep->configData), (char* const)pData, imageLen);
    } else {
        return IzotApiPersistentFailure;
    }
    
    return reason;
}

/*
 * Function: serializeLcsEppromPersistentSegment
 * This function serializes the EEPROM Persistent segment.

 */
static IzotApiError serializeLcsEppromPersistentSegment(
IzotByte** pData, 
int *len
)
{
    int imageLen = IzotPersistentGetMaxSize(IzotPersistentSegNetworkImage);

    *pData = (IzotByte *) OsalMalloc(imageLen);
    *len = imageLen;
    
    memcpy((void *)*pData, (const void*)(&eep->configData), imageLen);
    
    return IzotApiNoError;
}

/*
 * Function: deserializeAppDataPersistentSegment
 * This function deserializes the Application Persistent segment.

 */
static IzotApiError deserializeAppDataPersistentSegment(
void* const pData, 
int len
)
{
    int imageLen = IzotPersistentGetMaxSize(IzotPersistentSegApplicationData);
    
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
 * Function: serializeAppDataPersistentSegment
 * This function serializes the Application Persistent segment.

 */
static IzotApiError serializeAppDataPersistentSegment(IzotByte** pData, int *len)
{
    int imageLen = IzotPersistentGetMaxSize(IzotPersistentSegApplicationData);
    
    *pData = (IzotByte *) OsalMalloc(imageLen);
    *len = imageLen;
    
    if (izot_serialize_handler != NULL) {
        return izot_serialize_handler(*pData, imageLen);
    } else {
        return IzotApiNotInitialized;
    }
}

/*
 * Function: store
 * This function store the information of given type into the non-volatile.
 * memory

 */
static IzotApiError store(unsigned short type)
{
    IzotByte* pImage = NULL;
    IzotPersistenceHeader hdr;
    int imageLen = 0;
    IzotApiError reason = IzotApiNoError;

    IzotEnterTransaction(type);
    
    hdr.version = CURRENT_VERSION;
    hdr.length = 0;
    hdr.signature = ISI_IMAGE_SIGNATURE0;
    hdr.checksum = 0;
    hdr.appSignature = m_appSignature;
    
    if (type == IzotPersistentSegNetworkImage) {
        reason = serializeLcsEppromPersistentSegment(&pImage, &imageLen);
    } else if (type == IzotPersistentSegApplicationData) {
        reason = serializeAppDataPersistentSegment(&pImage, &imageLen);
#ifdef SECURITY_II
    } else if (type == IzotPersistentSegSecurityII) {
        reason = serializeSecurityIIData(&pImage, &imageLen);
#endif
    } 
    
    if (reason == IzotApiNoError) {
        hdr.checksum = computeChecksum(pImage, imageLen);
        hdr.length = imageLen;

        IzotPersistentHandle f = 
         (IzotPersistentHandle)IzotOpenForWrite(type, sizeof(hdr) + hdr.length);
        if (f != NULL) {
            if (IzotWrite(f, 0, sizeof(hdr), &hdr) != 0 ||
                IzotWrite(f, sizeof(hdr), hdr.length, pImage) != 0) {
                reason = IzotApiPersistentFailure;
            }
            IzotClose(f);
        }
        
        if (reason != IzotApiNoError) {
            NotifyErrorEvent();
        } else {
            IzotExitTransaction(type);
        }
        
        if (pImage != NULL) {
            OsalFree(pImage);
        }
    }
    
    return reason;
}

/*
 * Function: save_data
 * This funtion checks flag for update in perticular segment. If there is 
 * a update then it will updoate the data in non-volatile memory.

 */
static void save_data(void)
{
    unsigned int i;

    for (i = 0; i < IzotPersistentSegNumSegmentTypes; i++) {
        if (PersitenceList[i] != FALSE) {
            if (store(i) == IzotApiNoError) {
                PersitenceList[i] = FALSE;
                bSync = FALSE;
                OsalSleep(20);
            }
        }
    }
    scheduled = FALSE;
}

/*
 * Function: GetPersistentHeaderSize
 * This function compute Persistent header size

 */
unsigned GetPersistentHeaderSize(void) 
{
    return sizeof(IzotPersistenceHeader);
}
 
/*
 * Function: computeChecksum
 * This function compute the checksum on data to be stored in flash.

 */
int computeChecksum(IzotByte* pImage, int length)
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
        if (computeChecksum(pImage, pHdr->length) != pHdr->checksum) {
            result = FALSE;
        }
    }
    return result;
}

/*
 * Function: setAppSignature
 * This function sets application signature.

 */
void setAppSignature(unsigned appSignature)
{
    m_appSignature = appSignature;
}

/*
 * Function: GetAppSignature
 * This function returns application signature.

 */
unsigned GetAppSignature(void)
{
    return m_appSignature;
}

/*
 * Function: SetPeristenceGaurdBand
 * This function sets guardband duration .

 */
void SetPeristenceGaurdBand(int nTime)
{
    int ticks;
    
    // To convert milliseconds to tick counts in a platform independent way
    ticks = nTime*GetTicksPerSecond() / 1000; 
    if (ticks == 0) {
        ticks = 1;
    }
    
    m_guardBandDuration = ticks;
}

/*
 * Function: SetPersistentDataType
 * This function sets the flag for particular segment for flushing .

 */
void SetPersistentDataType(IzotPersistentSegmentType type)
{
    PersitenceList[type] = TRUE;
} 

/*
 * Function: SchedulePersistentData
 * This function schedules the timer for flushing after guardband duration .

 */
void SchedulePersistentData(void)
{
    if (!scheduled) {
        lastUpdate = IzotGetTickCount();
    }

    scheduled = TRUE;
}

/*
 * Function: NotifyErrorEvent
 * This function notifies the Error event .

 */
void NotifyErrorEvent(void)
{
    LCS_RecordError(IzotEepromWriteFail);
}

/*
 * Function: StoreTask
 * This funtion saves the data into non-volatile memoty after flish timeout.

 */
void StoreTask(void)
{
    unsigned long guardTimeLeft = guardBandRemainig();  

    if (scheduled) {
        if (guardTimeLeft == 0 || bSync) {
            save_data();
        }
    }
}

/*
 * Function: CommitPersistentData
 * This funtion flush any pending data to the non volatile memory.

 */
void CommitPersistentData(void)
{
    bSync = TRUE;
}

/*
 * Function: restore
 * This funtion restore the data of given type from flash and load this 
 * information to RAM.

 */
IzotApiError restore(unsigned short type)
{
    IzotApiError reason = IzotApiNoError;
    IzotPersistenceHeader hdr;
    IzotByte* pImage = NULL;
    int imageLength = 0;

    if (IzotIsInTransaction(type)) {
        reason = IzotApiPersistentFailure;
    } else {
        IzotPersistentHandle f = (IzotPersistentHandle)IzotOpenForRead(type);
        memset(&hdr, 0, sizeof(hdr));
        if (f != NULL) {
            if (IzotRead(f, 0, sizeof(hdr), &hdr) != 0) {
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
                    IzotRead(f, sizeof(hdr), imageLength, pImage) != 0 ||
                    !ValidateChecksum(&hdr, pImage)) {
                    reason = IzotApiPersistentFailure;
                    OsalFree(pImage);
                    pImage = NULL;
                }
            }
            IzotClose(f);
        } else {
            reason = IzotApiPersistentFailure;
        }
    }

    if (reason == IzotApiNoError) {
        switch(type) {
        case IzotPersistentSegNetworkImage:
            reason = 
                deserializeLcsEppromPersistentSegment(pImage, imageLength);
            break;
        case IzotPersistentSegApplicationData:
            reason = 
                deserializeAppDataPersistentSegment(pImage, imageLength);
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
    return restore(IzotPersistentSegSecurityII);
}
#endif

/*
 * Function: isPersistentDataScheduled
 * This function check whether any persistent data is scheduled to flush.

 */
IzotBool isPersistentDataScheduled(void)
{
    unsigned int i;
    IzotBool isScheduled = FALSE;

    for (i = 0; i < IzotPersistentSegNumSegmentTypes; i++) {
        if (PersitenceList[i]) {
            isScheduled = TRUE;
            // If scheduled then do immediate commit of that data.
            CommitPersistentData();
            break;
        }
    }
    return isScheduled;
}