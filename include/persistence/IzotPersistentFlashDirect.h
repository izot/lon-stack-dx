//
// IzotPersitentFlashDirect.h
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
#if !defined(DEFINED_IZOTPERSISTENTFLASHDIRECT_H)
#define DEFINED_IZOTPERSISTENTFLASHDIRECT_H

#include <stdio.h>
#include <string.h>
#include "IzotConfig.h"
#include "IzotPlatform.h"


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
 *  function will be used as the first parameter when calling 
 *  <IzotPersistentRead>. The application must maintain the handle used for 
 *  each segment. The application can invalidate a handle when 
 *  <IzotPersistentClose> is called for that handle.  
 */
extern IzotPersistentHandle IzotPersistentOpenForRead(const IzotPersistentSegmentType type); 
 
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
extern IzotPersistentHandle IzotPersistentOpenForWrite(const IzotPersistentSegmentType type, const size_t size); 

/* 
 *  Callback: IzotPersistentClose
 *  Close a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by 
 *           <IzotPersistentOpenForRead> or <IzotPersistentOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  handle and invalidates the handle. 
 */
extern void IzotPersistentClose(const IzotPersistentHandle handle);

/* 
 *  Callback: IzotPersistentRead
 *  Read a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by 
 *           <IzotPersistentOpenForRead>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API during initialization 
 *  to  read data from persistent storage. An error value is returned if the 
 *  specified handle does not exist.    
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
extern IzotApiError IzotPersistentRead(const IzotPersistentHandle handle, 
    const size_t offset, const size_t size, void * const pBuffer);
																
/* 
 *  Callback: IzotPersistentWrite
 *  Write a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by 
 *           <IzotPersistentOpenForWrite>
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
extern IzotApiError IzotPersistentWrite(const IzotPersistentHandle handle, 
    const size_t offset, const size_t size, const void* const pData);

/* 
 *  Callback: IzotPersistentIsInTransaction
 *  Returns TRUE if an PERSISTENT transaction was in progress last time the
 *  device shut down.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the Izot Device Stack API during initialization 
 *  prior to reading the segment data. This callback must return TRUE if an 
 *  PERSISTENT transaction had been started and never committed.  If this 
 *  function returns TRUE, the Izot Device Stack API will discard the segment, 
 *  otherwise, the IZOT Device Stack API will attempt to read the persistent 
 *  data. 
 */
extern IzotBool IzotPersistentIsInTransaction(const IzotPersistentSegmentType type); 
 
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
extern IzotApiError IzotPersistentEnterTransaction(const IzotPersistentSegmentType type);

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
extern IzotApiError IzotPersistentExitTransaction(const IzotPersistentSegmentType type);

extern void ErasePersistenceData(void);
extern void ErasePersistenceConfig(void);

#endif  /* defined(DEFINED_IZOTPERSISTENTFLASHDIRECT_H) */
 