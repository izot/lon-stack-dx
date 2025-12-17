/*
 * isi_persistence.c
 *
 * Copyright (c) 2005-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   ISI Persistence File Management
 * Purpose: Manages persistent storage for ISI data.
 * Notes:   Created February 2005, Bernd Gauweiler
 */

#include "izot/IzotPlatform.h"
#if !ISI_IS(ISI_ID_NO_ISI)

#include "persistence/lon_persistence.h"

#define CURR_VERSION                1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define ISI_APP_SIGNATURE0          0
#define ISI_PERSISTENCE_HEADER_LEN  100

// These macros move data from host format to network format.
// The result is a "network byte ordered" value pointed to by p
// and the input is a "host byte ordered" value in a
// whose type is given by the macro name as follows:
//		NL: host long (32 bits)
//		N3: host 3 byte item (24 bits)
//      NS: host short (16 bits)
//		NB: host byte (8 bits)
// Following the move, "p" points just past the item moved.
#define PTONL(p,a) *p++ = (IzotByte)(a>>24); PTON3(p,a)
#define PTON3(p,a) *p++ = (IzotByte)(a>>16); PTONS(p,a)
#define PTONS(p,a) *p++ = (IzotByte)(a>>8);  PTONB(p,a)
#define PTONB(p,a) *p++ = (IzotByte)a;

// These macros move data from network format to host format.
// The input is a "network byte ordered" value pointed to by p
// and the result is a "host byte ordered" value in a
// whose type is given by the macro name as follows:
//		HL: host long (32 bits)
//		H3: host 3 byte item (24 bits)
//      HS: host short (16 bits)
//		HB: host byte (8 bits)
// Following the move, "p" points just past the item moved.
// This works without causing alignment traps.
// (unlike constructs such as a = *(int*)&p[2])
#define PTOHL(p,a) { a = (((((p[0]<<8)+p[1])<<8)+p[2])<<8)+p[3]; p+=4; }
#define PTOH3(p,a) { a = ((((p[0]<<8)+p[1])<<8)+p[2]); p+=3; }
#define PTOHS(p,a) { a = (p[0]<<8)+p[1]; p+=2; }
#define PTOHB(p,a) { a = *p++; }

#define PTOHBN(n,p,a) \
{ IzotByte* q = (IzotByte*)(a); for(int i=0; i<n;i++) { *q++ = *p++; } }

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/
/*
*  Function: serializeIsiNvdSegPersistentData
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  serialize data in _isiPersist
*
*/
LtPersistenceLossReason serializeIsiNvdSegPersistentData(
IzotByte** pBuffer, 
size_t* len
)
{
	size_t image_len = sizeof(IsiPersist);
    IzotByte* pBuf;

	// Allocate memory for the serialization
 	*pBuffer = (IzotByte *) OsalAllocateMemory(image_len);
    memset(*pBuffer, 0, image_len);
    *len = image_len;
	pBuf = *pBuffer;
#ifdef  ISI_SUPPORT_TIMG
	PTONB(pBuf, _isiPersist.Devices);
#endif  //  ISI_SUPPORT_TIMG
	PTONB(pBuf, _isiPersist.Nuid);
	PTONS(pBuf, _isiPersist.Serial);
	PTONS(pBuf, _isiPersist.BootType);
	PTONB(pBuf, _isiPersist.RepeatCount);

    return LT_PERSISTENCE_OK;
}

/*
*  Function: serializeIsiNvdSegConnectionTable

*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  Undocumented
*
*/
LtPersistenceLossReason serializeIsiNvdSegConnectionTable(
		IzotByte** pBuffer, size_t* len)
{
#if ISI_IS(SIMPLE) || ISI_IS(DA)
	size_t image_len = IsiGetConnectionTableSize() * sizeof(IsiConnection);
    
	// Allocate memory for the serialization
	*pBuffer = (IzotByte *) OsalAllocateMemory(image_len);
    *len = image_len;

    memcpy((void *)*pBuffer, (const void *)IsiGetConnection(0), image_len);
#endif

    return LT_PERSISTENCE_OK;
}

/*
*  Function: deserializeIsiNvdSegConnectionTable
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  deserialize data in Connection Table
*
*/
LtPersistenceLossReason deserializeIsiNvdSegConnectionTable(
		IzotByte* pBuffer, int len, int nVersion)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;

#if ISI_IS(SIMPLE) || ISI_IS(DA)
    int image_len = IsiGetConnectionTableSize() * sizeof(IsiConnection);

    if (len >= image_len)
    {
        memcpy((void *)IsiGetConnection(0), (void *)pBuffer ,image_len);
    }
    else
        reason = LT_PROGRAM_ATTRIBUTE_CHANGE;
#endif

    return reason;
}

/*
*  Function: deserializeIsiNvdSegData
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  deserialize data in _isiPersist
*
*/
LtPersistenceLossReason deserializeIsiNvdSegData(IzotByte* pBuffer, 
		unsigned int len, unsigned int nVersion)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;

    if (len >= sizeof(_isiPersist)) {
	    IzotByte* pBuf = pBuffer;
#ifdef  ISI_SUPPORT_TIMG
	    PTOHB(pBuf, _isiPersist.Devices);
#endif  //  ISI_SUPPORT_TIMG
	    PTOHB(pBuf, _isiPersist.Nuid);
	    PTOHS(pBuf, _isiPersist.Serial);
	    PTOHB(pBuf, _isiPersist.RepeatCount);
    }
    else
    {
        reason = LT_CORRUPTION;
    }

    return reason;
}

/*
*  Function: savePersistentData
*
*  Returns:
*  void
*
*  Remarks:
*  Undocumented
*
*/
void savePersistentData(IzotPersistentSegType persistentSegType)
{
	IzotByte* pImage = NULL;
	IzotBool failure = FALSE;
	IzotPersistenceHeader hdr;
	size_t imageLen;
    LtPersistenceLossReason reason = LT_NO_PERSISTENCE;
	
    hdr.version = CURR_VERSION;
    hdr.length = 0;
	hdr.signature = ISI_IMAGE_SIGNATURE0;
    hdr.checksum = 0;
    hdr.appSignature = IzotGetAppSignature();
    _IsiAPIDebug("savePersistentData - for persistentSegType=%d\n", persistentSegType); 
    if (persistentSegType == IzotPersistentSegConnectionTable)
        reason = serializeIsiNvdSegConnectionTable(&pImage, &imageLen);
    else
    if (persistentSegType == IzotPersistentSegIsi)
        reason = serializeIsiNvdSegPersistentData(&pImage, &imageLen);
 
    if (reason == LT_PERSISTENCE_OK)
    {
	    hdr.checksum = ComputeChecksum(pImage, imageLen);
	    hdr.length = imageLen;

		IzotPersistentSegType returnedSegType = IzotPersistentSegOpenForWrite(persistentSegType, sizeof(hdr) + hdr.length);
		if (returnedSegType != IzotPersistentSegUnassigned) {
			if (IzotPersistentSegWrite(returnedSegType, 0, sizeof(hdr), &hdr) != 0 ||
				  IzotPersistentSegWrite(returnedSegType, sizeof(hdr), hdr.length, pImage) != 0) {
				failure = TRUE;
			}
			IzotPersistentSegClose(returnedSegType);
		}
		
		if (failure) {
			IzotPersistentMemReportFailure();
		}
		
        if (pImage != NULL) {
	        OsalFreeMemory(pImage);
        }
    }
}

/*
*  Function: restorePersistentData
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  Undocumented
*
*/
LtPersistenceLossReason restorePersistentData(IzotPersistentSegType persistentSegType)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	IzotPersistenceHeader hdr;
	IzotByte* pBuffer = NULL;
	size_t imageLen = 0;
	int nVersion = 0;

	IzotPersistentSegType returnedSegType = IzotPersistentSegOpenForRead(persistentSegType);
	memset(&hdr, 0, sizeof(hdr));
	if (returnedSegType != IzotPersistentSegUnassigned) {
		if (IzotPersistentSegRead(returnedSegType, 0, sizeof(hdr), &hdr) != 0) {
			reason = LT_CORRUPTION;
		} else if (hdr.signature != ISI_IMAGE_SIGNATURE0) {
			reason = LT_SIGNATURE_MISMATCH;
		} else if (hdr.appSignature != IzotGetAppSignature()) {
			reason = LT_SIGNATURE_MISMATCH;
		} else if (hdr.version > CURR_VERSION) {
			reason = LT_VERSION_NOT_SUPPORTED;
		} else {
			nVersion = hdr.version;
			imageLen = hdr.length;
			pBuffer = (IzotByte *) OsalAllocateMemory(imageLen);
			if (pBuffer == NULL ||
				  IzotPersistentSegRead(returnedSegType, sizeof(hdr), imageLen, pBuffer) != 0 ||
				!ValidateChecksum(&hdr, pBuffer)) {
				reason = LT_CORRUPTION;
				OsalFreeMemory(pBuffer);
				pBuffer = NULL;
			}
		}
		IzotPersistentSegClose(returnedSegType);
	} else {
		reason = LT_NO_PERSISTENCE;
	}

	if (reason == LT_PERSISTENCE_OK) {
		if (persistentSegType == IzotPersistentSegConnectionTable) {
			reason  = 
               deserializeIsiNvdSegConnectionTable(pBuffer, imageLen, nVersion);
		} else {
		if (persistentSegType == IzotPersistentSegIsi)
			reason  = deserializeIsiNvdSegData(pBuffer, imageLen, nVersion);
		}
    }

    if (pBuffer != NULL) {
    	OsalFreeMemory(pBuffer);
    }
	
    return reason;
}

#endif  // !ISI_IS(ISI_ID_NO_ISI)