//
// IzotOsal.c
//
// Copyright (C) 2022 EnOcean
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
 * Title: Operaing System Abstaction Layer file
 *
 * Abstract:
 * This file contains the Platform dependent APIs. This file contains
 * APIs for FreeRTOS to get timing information.
 */

#include "IzotConfig.h"
#include "IzotPlatform.h"

#if !defined(DEFINED_IZOTOSAL_H)
#define DEFINED_IZOTOSAL_H


/*
 * SECTION: General OSAL definitions
 *
 * This section defines components used by the OSAL sub-system.
 */

/*
 *  Macro: MAX_TIMEOUT_TICKS
 *  The maximum number of ticks supported by uC/OS-II when waiting on an event.
 *
 *  The uC/OS-II primitives use a 16-bit tick count for all of their timeouts,
 *  but the OSAL supports 32-bit timeouts.
 */
#define MAX_TIMEOUT_TICKS 0xffff

/* 
 *  Enumeration: OsalStatus
 *  OSAL status code.
 *
 *  This enumeration defines status codes returned from OSAL functions.
 */
typedef enum
{
    OSALSTS_SUCCESS,			// Succeeded.
    OSALSTS_TIMEOUT,			// A wait operation timed out.
    OSALSTS_CSERROR,			// Generic error accessing a critical section.
    OSALSTS_BSEM_ERROR,			// A generic error creating or accessing 
                                // a binary  semaphore. 

    OSALSTS_EVENT_ERROR,		// A generic error creating or accessing 
                                // an event.
    OSALSTS_CREATE_TASK_FAILED,	// Failed to create a task.
    OSALSTS_SLEEP_ERROR,		// Failed to sleep.
    OSALSTS_NOT_IMPLEMENTED,    // Function is not implemented.
} OsalStatus;

/*
 *  Typedef: OsalTickCount
 *  A 32-bit unsigned value representing the number of ticks since startup of 
 *  the OS.
 *  See <IzotGetTickCount> and <GetTicksPerSecond>.
 */
typedef unsigned int OsalTickCount;

/*
 * SECTION: Timing FUNCTIONS
 *
 * This section details the OSAL functions involving timing.
 */

/*
 *  Function: GetTicksPerSecond
 *  Get the number of ticks in a second.
 *
 *  Returns:
 *  The number of ticks in one second.
 *
 *  Remarks:
 *  Returns the number of ticks in one second.
 *
 *  See Also:
 *  <IzotGetTickCount>.
 */
extern OsalTickCount GetTicksPerSecond(void);

/*
 *  Function: OsalSleep
 *  Sleep for a specified number of ticks.
 *
 *  Parameters:
 *  ticks - The number of ticks to sleep 
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Suspend the task for the specified number of clock ticks.
 *
 *  See Also:
 *  <IzotGetTickCount>.
 */
extern OsalStatus OsalSleep(int ticks);

/*
 * Function:   OsalMalloc
 * Similar to malloc POSIX
 *
 * Returns:
 * Pointer to the memory area allocated.
 * 
 * Remarks:
 * Allocates Memory Area as instructed
 * 
 */
extern void *OsalMalloc(unsigned int size);

/*
 * Function:   OsalFree
 * Similar to free in  POSIX
 *
 * Returns:
 * void
 * 
 * Remarks:
 * Frees Memory Area as instructed
 * 
 */
extern void OsalFree(void *ptr);

#endif  /* defined(DEFINED_IZOTOSAL_H) */
/* end of file. */