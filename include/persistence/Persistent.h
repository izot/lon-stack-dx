//
// Persitent.h
//
// Copyright (C) 2022-2025 EnOcean
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
#ifndef _PERSISTENT_H
#define _PERSISTENT_H

#include <stddef.h>
#include <string.h>
#include "IzotConfig.h"
#include "IzotApi.h"
#include "IzotOsal.h"
#include "isi_int.h"
#include "lcs_timer.h"
#include "lcs_node.h"

#if PROCESSOR_IS(MC200)
#include <wm_os.h>
#endif


/*------------------------------------------------------------------------------
  Section: Definations
  ------------------------------------------------------------------------------*/
typedef IZOT_STRUCT_BEGIN(IzotPersistenceHeader)
{
	unsigned int length;
	unsigned signature;
	unsigned appSignature;
	unsigned short version;
	unsigned short checksum;
} IZOT_STRUCT_END(IzotPersistenceHeader);

/*------------------------------------------------------------------------------
  Section: Function Prototypes
  ------------------------------------------------------------------------------*/

/*
 * Function: GetPersistentHeaderSize
 * This function compute Persistent header size

 */
extern unsigned GetPersistentHeaderSize(void);

/*
 * Function: computeChecksum
 * This function compute the checksum on data to be stored in flash.

 */
extern int computeChecksum(IzotByte* pImage, int length);

/*
 * Function: ValidateChecksum
 * This function validates the checksum from the data read from flash.

 */
extern IzotBool ValidateChecksum(IzotPersistenceHeader* pHdr, IzotByte* pImage);

/*
 * Function: GetAppSignature
 * This function returns application signature.

 */
extern unsigned GetAppSignature(void);

/*
 * Function: SetPeristenceGaurdBand
 * This function sets guardband duration .

 */
extern void SetPeristenceGaurdBand(int nTime);

/*
 * Function: SetPersistentDataType
 * This function sets the flag for particular segment for flushing .

 */
extern void SetPersistentDataType(IzotPersistentSegmentType type);

/*
 * Function: SchedulePersistentData
 * This function schedules the timer for flushing after guardband duration .

 */
extern void SchedulePersistentData(void);

/*
 * Function: NotifyErrorEvent
 * This function notifies the Error event .

 */
extern void NotifyErrorEvent(void);

/*
 * Function: StoreTask
 * This funtion saves the data into non-volatile memoty after flish timeout.

 */
extern void StoreTask(void);

/*
 * Function: CommitPersistentData
 * This funtion flush any pending data to the non volatile memory.

 */
extern void CommitPersistentData(void);

/*
 * Function: restore
 * This funtion restore the data of given type from flash and load this 
 * information to RAM.

 */
extern IzotApiError restore(unsigned short type);

/*
 *  Event: IzotEnterTransaction
 *  Calls the registered callback of <IzotPersistentEnterTransaction>.
 *
 *  Remarks:
 *
 */
extern IzotApiError IzotEnterTransaction(const IzotPersistentSegmentType type);

/*
 *  Event: IzotOpenForWrite
 *  Calls the registered callback of <IzotPersistentOpenForWrite>.
 *
 *  Remarks:
 *
 */
extern IzotPersistentHandle IzotOpenForWrite(const IzotPersistentSegmentType type, const size_t size);

/*
 *  Event: IzotWrite
 *  Calls the registered callback of <IzotPersistentWrite>.
 *
 *  Remarks:
 *
 */
extern IzotApiError IzotWrite(const IzotPersistentHandle handle, const size_t offset, 
		const size_t size, const void* const pData);

/*
 *  Event: IzotClose
 *  Calls the registered callback of <IzotPersistentClose>.
 *
 *  Remarks:
 *
 */
extern void IzotClose(const IzotPersistentHandle handle);

/*
 *  Event: IzotExitTransaction
 *  Calls the registered callback of <IzotPersistentExitTransaction>.
 *
 *  Remarks:
 *
 */
extern IzotApiError IzotExitTransaction(const IzotPersistentSegmentType type);

/*
 *  Event: IzotIsInTransaction
 *  Calls the registered callback of <IzotPersistentIsInTransaction>.
 *
 *  Remarks:
 *
 */
extern IzotBool IzotIsInTransaction(const IzotPersistentSegmentType type);

/*
 *  Event: IzotOpenForRead
 *  Calls the registered callback of <IzotPersistentOpenForRead>.
 *
 *  Remarks:
 *
 */
extern IzotPersistentHandle IzotOpenForRead(const IzotPersistentSegmentType type);

/*
 *  Event: IzotRead
 *  Calls the registered callback of <IzotPersistentRead>.
 *
 *  Remarks:
 *
 */
extern IzotApiError IzotRead(const IzotPersistentHandle handle, const size_t offset, 
		const size_t size, void * const pBuffer);

/*
 *  Function: IzotPersistentGetMaxSize
 *  Gets the number of bytes required to store persistent data.
 *
 *  Parameters:
 *  segmentType - The segment type, see <IzotPersistentSegmentType>
 *
 *  Returns:
 *  The number of bytes required to store persistent data for the specified
 *  segment.
 *
 *  Remarks:
 *  This function will not typically be called directly by the application,
 *  but may be used by persistent data event handlers (implemented by the
 *  application) to reserve space for persistent data segments.
 */
extern int IzotPersistentGetMaxSize(IzotPersistentSegmentType segmentType);

extern IzotBool isPersistentDataScheduled(void);

#endif /*_PERSISTENT_H*/