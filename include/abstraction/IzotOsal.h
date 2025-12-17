/*
 * IzotOsal.h
 *
 * Copyright (c) 2021-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Operating System Abstraction Layer
 * Purpose: Defines portable functions and types of operating
 *          system interfaces.
 * Notes:   You can customize this file to add support for 
 *          additional operating systems.
 */

#ifndef _OSAL_H
#define _OSAL_H

#ifdef  __cplusplus
extern "C"
{
#endif  // __cplusplus

#include <stddef.h>
#include "izot/IzotPlatform.h"

#if OS_IS(LINUX)
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#elif OS_IS(FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include <string.h>
#include <stdio.h>  
#include <stdlib.h>
#include <stdint.h> 
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif  // OS_IS(LINUX) || OS_IS(FREERTOS)

// Maximum number of milliseconds for OsalSleep()
#define MAX_TIMEOUT_MS 0xffffffff

// Number of USB line disciplines supported
#ifndef NR_LDISCS
#define NR_LDISCS 30
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

#ifndef DEVICE_NAME_MAX
#define DEVICE_NAME_MAX 64
#endif

#ifndef OSAL_ERROR_STRING_MAXLEN
#define OSAL_ERROR_STRING_MAXLEN 256
#endif

// Log level enumeration for message reporting
typedef enum {
    LOG_NONE  = 0,
    LOG_ERROR = 1,
    LOG_DEBUG = 2,
    LOG_TRACE = 3
} LogLevel;

// Set INITIAL_LOG_LEVEL to a LogLevel value
#ifndef INITIAL_LOG_LEVEL
#define INITIAL_LOG_LEVEL LOG_ERROR
#endif

// Tick count type used by OSAL timing functions
typedef uint32_t OsalTickCount;

// OS-dependent lock, event, thread ID, and entry point typedefs
#if OS_IS(LINUX)
    typedef pthread_mutex_t OsalLockType;
    typedef struct {
        pthread_cond_t cond;
        pthread_mutex_t mutex;
        uint8_t flag;
    } OsalEvent;
    typedef pthread_t OsalThreadId;
    typedef void *(*OsalEntryPoint)(void *);
#elif OS_IS(FREERTOS)
    typedef SemaphoreHandle_t OsalLockType;
    typedef struct {
        SemaphoreHandle_t semaphore;
        uint8_t flag;
    } OsalEvent;
    typedef TaskHandle_t OsalThreadId;
    typedef void (*OsalEntryPoint)(void *);
#else
    // Future or unknown OS
    #pragma message("Implement OS-dependent OsalLockType, OsalEvent, OsalThreadId, and OsalEntryPoint types")
    typedef void* OsalLockType;
    typedef struct {
        void *event_impl;
        uint8_t flag;
    } OsalEvent;
    typedef void* OsalThreadId;
    typedef void* (*OsalEntryPoint)(void *);
#endif

typedef OsalEvent* OsalHandle;

#if OS_IS(LINUX_KERNEL)
#include <linux/delay.h>
#include <linux/wait.h>
typedef spinlock_t mutex_lock;
#else
#define IFNAMSIZE 16
#endif  // !OS_IS(LINUX_KERNEL)

typedef int mutex_lock;

/*****************************************************************
 * Section: Semaphore Lock Management Function Definitions
 *****************************************************************/
/*
 * Initializes a mutex or spinlock instance.
 * Parameters:
 *   lock: Pointer to the lock object to initialize.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode OsalInitMutex(OsalLockType *lock);

/*
 * Locks a mutex or spinlock.
 * Parameters:
 *   lock: Pointer to the lock to be acquired.
 * Returns:
 *   None
 */
void OsalLockMutex(OsalLockType *lock);

/*
 * Unlocks a mutex or spinlock.
 * Parameters:
 *   lock: Pointer to the lock to be released.
 * Returns:
 *   None
 */
void OsalUnlockMutex(OsalLockType *lock);

/*****************************************************************
 * Section: Event Management Function Definitions
 *****************************************************************/
/*
 * Creates an event object.
 * Parameters:
 *   eventHandle: Pointer to the event handle to be created.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */ 
LonStatusCode OsalCreateEvent(OsalHandle *eventHandle);

/*
 * Deletes an event object.
 * Parameters:
 *   eventHandle: Pointer to the event handle to be deleted.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode OsalDeleteEvent(OsalHandle *eventHandle);

/*
 * Waits for an event to fire.
 * Parameters:
 *   eventHandle: Pointer to the event handle to wait for.
 *   waittime: Maximum time to wait in milliseconds.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode OsalWaitForEvent(OsalHandle eventHandle, unsigned int waittime);

/*
 * Sets (fires) an event.
 * Parameters:
 *   eventHandle: Pointer to the event handle to be set.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode OsalSetEvent(OsalHandle eventHandle);

/*****************************************************************
 * Section: Timing, Tasking, and Memory Allocation
 *          Function Definitions
 *****************************************************************/
/*
 * Returns the current system tick count.
 * Parameters:
 *   None.
 * Returns:
 *   Number of ticks since operating system startup.
 * Notes:
 *  The number of ticks per second is set to 1000, or 1 millisecond
 *  per tick.  This can be determined by calling <OsalGetTicksPerSecond>.
 *  The tick count is a 32-bit unsigned integer that wraps around
 *  approximately every 49.7 days with 1 millisecond per tick.
 */
OsalTickCount OsalGetTickCount(void);

/*
 * Gets the number of ticks in a second.
 * Parameters:
 *   None.
 * Returns:
 *   The number of ticks in one second.
 * Notes:
 *   See also OsalGetTickCount().
 */
OsalTickCount OsalGetTicksPerSecond(void);

/*
 * Creates a thread to run the specified entry point function.
 * Parameters:
 *   threadEntry: Pointer to the thread entry point function.
 *   threadData:  Pointer to data to be passed to the thread entry point.
 * Returns:
 *   Thread ID if successful; 0 or NULL if unsuccessful.
 */
OsalThreadId OsalCreateThread(OsalEntryPoint threadEntry, void* threadData);

/*
 * Sleeps for a specified number of milliseconds.
 * Parameters:
 *   msecs - The number of milliseconds to sleep 
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful.
 *  Notes:
 *   If the specified number of milliseconds exceeds the maximum timeout
 *   value of 0xFFFFFFFE, the sleep duration is capped at that value. 
 *   See also: OsalGetTickCount().
 */
LonStatusCode OsalSleep(unsigned int msecs);

/*
 * Allocates memory block of the specified size.
 * Parameters:
 *   size: Number of bytes to allocate
 * Returns:
 *   Pointer to allocated memory, or NULL if allocation failed
 */
void* OsalAllocateMemory(size_t size);

/*
 * Frees memory block previously allocated with OsalAllocateMemory.
 * Parameters:
 *   buf: pointer to memory block to free
 * Returns:
 *   None
 */
void OsalFreeMemory(void *buf);

/*****************************************************************
 * Section: Message Reporting Abstraction Types and Function Declarations
 *****************************************************************/
/*
 * Sets the current log level for message reporting.
 * Parameters:
 *   level: New log level (LOG_NONE, LOG_ERROR, or LOG_DEBUG)
 * Returns:
 *   None
 */
void OsalSetLogLevel(LogLevel level);

/*
 * Gets the current log level for message reporting.
 * Parameters:
 *   None
 * Returns:
 *   Current log level (LOG_NONE, LOG_ERROR, LOG_DEBUG, LOG_TRACE)
 */
LogLevel OsalGetLogLevel(void);

/*
 * Prints a system call error message with optional message code and text.
 * Parameters:
 *   errorCode: Error code to include in message, or LonStatusNoError for none
 *   errorString: printf-style format string for message
 *   ...: Variable arguments for format string
 * Returns:
 *   None
 * Notes:
 *   If errorCode is >= 0, the message is prefixed with "Error <errorCode>: ".
 *   If errno is set, the string from strerror(errno) is appended to the 
 *   message.  This function is intended for reporting system call errors.
 */
void OsalPrintSysError(LonStatusCode errorCode, char *errorString, ...);

/*
 * Prints an error message with optional message code and text.
 * Parameters:
 *   errorCode: Error code to include in message, or LonStatusNoError for none
 *   errorString: printf-style format string for message
 *   ...: Variable arguments for format string
 * Returns:
 *   None
 * Notes:
 *  This function only prints messages if the current log level is LOG_ERROR
 *  or LOG_DEBUG.
 */
void OsalPrintError(LonStatusCode errorCode, char *errorString, ...);

/*
 * Prints a debug message with optional message code and text.
 * Parameters:
 *   errorCode: Error code to include in message, or LonStatusNoError for none
 *   errorString: printf-style format string for message
 *   ...: Variable arguments for format string
 * Returns:
 *   None
 * Notes:
 *  This function only prints messages if the current log level is LOG_DEBUG.
 */
void OsalPrintDebug(LonStatusCode errorCode, char *errorString, ...); 

/*
 * Prints a trace message with optional message code and text.
 * Parameters:
 *   errorCode: Error code to include in message, or -1 for none
 *   errorString: printf-style format string for message
 *   ...: Variable arguments for format string
 * Returns:
 *   None
 * Notes:
 *   This function only prints messages if the current log level is LOG_TRACE.
 */
void OsalPrintTrace(LonStatusCode errorCode, char *errorString, ...);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _OSAL_H
