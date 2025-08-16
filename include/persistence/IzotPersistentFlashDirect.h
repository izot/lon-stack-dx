//
// IzotPersitentFlashDirect.h
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
 * Example implementation of the Izot non-volatile data (PERSISTENT) functions
 * using Marvell 88MC200 flash access routines. 
 * 
 * You can port this file to support your hardware and application
 * requirements.
 */
#if !defined(DEFINED_IZOTPERSISTENTFLASHDIRECT_H)
#define DEFINED_IZOTPERSISTENTFLASHDIRECT_H

#include <stdio.h>
#include <string.h>
#include "abstraction/IzotConfig.h"
#include "izot/IzotPlatform.h"


/*------------------------------------------------------------------------------
  Section: Macros
  ------------------------------------------------------------------------------*/
#ifdef FLASH_DEBUG
	#define FLASH_PRINTF	wmprintf
#endif

/*
 * *****************************************************************************
 * SECTION: PERSISTENT CALLBACK PROTOTYPES
 * *****************************************************************************
 *
 *  This section contains the Izot Device Stack API callback functions 
 *  supporting memory for Non-Volatile Data (PERSISTENT). 
 *
 *  Remarks:
 *  The application developer may need to modify these functions to fit the type 
 *  of non-volatile storage supported by the application.
 *
 *  Callback functions are called by the IZOT protocol stack 
 *  immediately, as needed, and may be called from any IZOT task.  The 
 *  application *must not* call into the Izot Device Stack API from within a 
 *  callback.
 */ 

/* 
 *  Callback: IzotFlashSegOpenForRead
 *  Open a non-volatile data segment for reading.
 *
 *  Parameters:
 *  persistentSegType -  type of non-volatile data to be opened
 *
 *  Returns:
 *  <IzotPersistentSegType>.   
 *
 *  Remarks:
 *  This function opens the data segment for reading.  If the file exists and 
 *  can be opened, this function returns a valid persistent segment type.
 *  Otherwise it returns IzotPersistentSegUnassigned.  The segment type 
 *  returned by this function will be used as the first parameter when calling 
 *  <IzotFlashSegRead>. The application must maintain the segment type used
 *  for each segment. The application can invalidate a segment type when 
 *  <IzotFlashSegClose> is called for that segment type.  
 */
extern IzotPersistentSegType IzotFlashSegOpenForRead(const IzotPersistentSegType persistentSegType); 
 
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
 *  This function is called by the LON stack after changes 
 *  have been made to data that is backed by persistent storage.  Multiple 
 *  updates may have been made.  This function is called only after the
 *  LON stack determines that all updates have been completed, based on a 
 *  flush timeout. 
 *
 *  IzotPersistentSegUnassigned is returned if the data cannot be written.
 */
extern IzotPersistentSegType IzotFlashSegOpenForWrite(const IzotPersistentSegType persistentSegType, const size_t size); 

/* 
 *  Callback: IzotFlashSegClose
 *  Close a non-volatile data segment.
 *
 *  Parameters:
 *  persistentSegType - persistent segment type returned by 
 *           <IzotFlashSegOpenForRead> or <IzotFlashSegOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  segment and invalidates the segment type. 
 */
extern void IzotFlashSegClose(const IzotPersistentSegType persistentSegmentType);

/* 
 *  Callback: IzotFlashSegRead
 *  Read a section of a non-volatile data segment.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type returned by 
 *           <IzotFlashSegOpenForRead>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by the LON stack during initialization 
 *  to read data from persistent storage. IzotPersistentSegUnassigned is 
 *  returned if the specified handle does not exist.    
 *
 *  The offset will always be 0 on the first call after opening the
 *  segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
extern IzotApiError IzotFlashSegRead(const IzotPersistentSegType persistentSegType, 
    const size_t offset, const size_t size, IzotByte* const pBuffer);
																
/* 
 *  Callback: IzotFlashSegWrite
 *  Write a section of a non-volatile data segment.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type returned by 
 *           <IzotFlashSegOpenForWrite>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pData - pointer to the data to write into the segment
 *
 *  Remarks:
 *  This function is called by the LON stack after changes have been 
 *  made to data that is backed by persistent storage.  Multiple updates 
 *  may have been made.  This function is called only after the LON stack 
 *  determines that all updates have been completed.   
 *
 *  The offset will always be 0 on the first call after opening the
 *  segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
extern IzotApiError IzotFlashSegWrite(const IzotPersistentSegType persistentSegType, 
    const size_t offset, const size_t size, const IzotByte* const pData);

/* 
 *  Callback: IzotFlashSegIsInTransaction
 *  Returns TRUE if a persistent transaction was in progress during the
 *  last time the device shut down.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the LON stack API during initialization 
 *  prior to reading the segment data. This callback must return TRUE if a 
 *  persistent transaction had been started and never committed.  If this 
 *  function returns TRUE, the LON stack will discard the segment, 
 *  otherwise, the LON stack API will attempt to read the persistent 
 *  data. 
 */
extern IzotBool IzotFlashSegIsInTransaction(const IzotPersistentSegType persistentSegType); 
 
/* 
 *  Callback: IzotFlashSegEnterTransaction
 *  Initiate a non-volatile transaction.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the LON stack API when the first request 
 *  arrives to update data stored in the specified segment.  The LON stack
 *  updates the non-persistent image, and schedules writes to update the 
 *  non-volatile storage at a later time.  
 */
extern IzotApiError IzotFlashSegEnterTransaction(const IzotPersistentSegType persistentSegType);

/* 
 *  Callback: IzotFlashSegExitTransaction
 *  Complete a non-volatile transaction.
 *
 *  Parameters:
 *  persistentSegType - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the LON stack after <IzotFlashSegWrite> 
 *  has returned success and there are no further updates required.
 */
extern IzotApiError IzotFlashSegExitTransaction(const IzotPersistentSegType persistentSegType);

extern void ErasePersistenceData(void);
extern void ErasePersistenceConfig(void);

#endif  /* defined(DEFINED_IZOTPERSISTENTFLASHDIRECT_H) */
 