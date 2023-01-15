//
// Persitence.c
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
//

/*
 * Created February 2005, Bernd Gauweiler
 *
 * Abstract:
 * implement ISI persistence files
 */
#include "isi_int.h"
#include <stddef.h>
#include <wm_os.h>
#include "Persistent.h"

/*------------------------------------------------------------------------------
Section: Macros
------------------------------------------------------------------------------*/
#define CURR_VERSION                1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define ISI_APP_SIGNATURE0          0
#define ISI_PERSISTENCE_HEADER_LEN  100

//
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

//
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

/*------------------------------------------------------------------------------
Section: Definations
------------------------------------------------------------------------------*/

/*
 * *****************************************************************************
 * SECTION: FUNCTIONS
 * *****************************************************************************
 *
 */

/*
*  Function: serializeIsiNvdSegPersistentData
*  Undocumented
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
int* len
)
{
	int image_len = sizeof(IsiPersist);
    IzotByte* pBuf;

	// Allocate memory for the serialization
 	*pBuffer = (IzotByte *) OsalMalloc(image_len);
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
*  Undocumented
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  Undocumented
*
*/
LtPersistenceLossReason serializeIsiNvdSegConnectionTable(
IzotByte** pBuffer, 
int* len
)
{
	int image_len = IsiGetConnectionTableSize() * sizeof(IsiConnection);
    
	// Allocate memory for the serialization
	*pBuffer = (IzotByte *) OsalMalloc(image_len);
    *len = image_len;

    memcpy((void *)*pBuffer, (const void *)IsiGetConnection(0), image_len);

    return LT_PERSISTENCE_OK;
}

/*
*  Function: deserializeIsiNvdSegConnectionTable
*  Undocumented
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  deserialize data in Connection Table
*
*/
LtPersistenceLossReason deserializeIsiNvdSegConnectionTable(
IzotByte* pBuffer, 
int len, 
int nVersion
)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
    int image_len = IsiGetConnectionTableSize() * sizeof(IsiConnection);

    if (len >= image_len)
    {
        memcpy((void *)IsiGetConnection(0), (void *)pBuffer ,image_len);
    }
    else
        reason = LT_PROGRAM_ATTRIBUTE_CHANGE;

    return reason;
}

/*
*  Function: deserializeIsiNvdSegData
*  Undocumented
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  deserialize data in _isiPersist
*
*/
LtPersistenceLossReason deserializeIsiNvdSegData(
IzotByte* pBuffer, 
int len, 
int nVersion
)
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
*  Undocumented
*
*  Returns:
*  void
*
*  Remarks:
*  Undocumented
*
*/
void savePersistentData(IzotPersistentSegmentType type)
{
	IzotByte* pImage = NULL;
	IzotBool failure = FALSE;
	IzotPersistenceHeader hdr;
	int imageLen;
    LtPersistenceLossReason reason = LT_NO_PERSISTENCE;
	
    hdr.version = CURR_VERSION;
    hdr.length = 0;
	hdr.signature = ISI_IMAGE_SIGNATURE0;
    hdr.checksum = 0;
    hdr.appSignature = IzotGetAppSignature();
    _IsiAPIDebug("savePersistentData - for type=%d\n", type); 
    if (type == IsiPersistentSegConnectionTable)
        reason = serializeIsiNvdSegConnectionTable(&pImage, &imageLen);
    else
    if (type == IsiPersistentSegPersistent)
        reason = serializeIsiNvdSegPersistentData(&pImage, &imageLen);
 
    if (reason == LT_PERSISTENCE_OK)
    {
	    hdr.checksum = computeChecksum(pImage, imageLen);
	    hdr.length = imageLen;

		IzotPersistentHandle f = 
                              IzotOpenForWrite(type, sizeof(hdr) + hdr.length);
		if (f != NULL) {
			if (IzotWrite(f, 0, sizeof(hdr), &hdr) != 0 ||
				IzotWrite(f, sizeof(hdr), hdr.length, pImage) != 0)	{
				failure = TRUE;
			}
			IzotClose(f);
		}
		
		if (failure) {
			NotifyErrorEvent();
		}
		
        if (pImage != NULL) {
	        OsalFree(pImage);
        }
    }
}

/*
*  Function: restorePersistentData
*  Undocumented
*
*  Returns:
*  LtPersistenceLossReason
*
*  Remarks:
*  Undocumented
*
*/
LtPersistenceLossReason restorePersistentData(IzotPersistentSegmentType type)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	IzotPersistenceHeader hdr;
	IzotByte* pBuffer = NULL;
	int imageLen = 0;
	int nVersion = 0;

	IzotPersistentHandle f = IzotOpenForRead(type);
	memset(&hdr, 0, sizeof(hdr));
	if (f != NULL) {
		if (IzotRead(f, 0, sizeof(hdr), &hdr) != 0) {
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
			pBuffer = (IzotByte *) OsalMalloc(imageLen);
			if (pBuffer == NULL ||
				IzotRead(f, sizeof(hdr), imageLen, pBuffer) != 0 ||
				!ValidateChecksum(&hdr, pBuffer)) {
				reason = LT_CORRUPTION;
				OsalFree(pBuffer);
				pBuffer = NULL;
			}
		}
		IzotClose(f);
	} else {
		reason = LT_NO_PERSISTENCE;
	}


	if (reason == LT_PERSISTENCE_OK) {
		if (type == IsiPersistentSegConnectionTable)
			reason  = 
               deserializeIsiNvdSegConnectionTable(pBuffer, imageLen, nVersion);
		else
		if (type == IsiPersistentSegPersistent)
			reason  = deserializeIsiNvdSegData(pBuffer, imageLen, nVersion);
    }

    if (pBuffer != NULL) {
    	OsalFree(pBuffer);
    }
	
    return reason;
}
