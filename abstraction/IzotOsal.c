/*
 * IzotOsal.c
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

#include "izot/IzotPlatform.h"

#if PROCESSOR_IS(MC200)
    #include <wm_os.h>
#else
    #if PLATFORM_IS(RPI_PICO)
        #include <Arduino.h>
    #endif
    #include <stdarg.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>
    #include <unistd.h>
#endif

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
LonStatusCode OsalInitMutex(OsalLockType *lock)
{
#if OS_IS(LINUX)
    if (!lock) {
        return LonStatusInvalidParameter;
    }
    int rc = pthread_mutex_init(lock, NULL);
    return (rc == 0) ? LonStatusNoError : LonStatusCriticalSectionError;
#elif OS_IS(FREERTOS)
    if (!lock) {
        return LonStatusInvalidParameter;
    }
    *lock = xSemaphoreCreateMutex();
    return (*lock != NULL) ? LonStatusNoError : LonStatusSemaphoreError;
#else
    // Add implementation
    #pragma message("Implement OS-dependent definition of OsalInitMutex()")
    return LonStatusCriticalSectionError;
#endif
}

/*
 * Locks a mutex or spinlock.
 * Parameters:
 *   lock: Pointer to the lock to be acquired.
 * Returns:
 *   None
 */
void OsalLockMutex(OsalLockType *lock)
{
#if OS_IS(LINUX)
    pthread_mutex_lock(lock);
#elif OS_IS(FREERTOS)
    xSemaphoreTake(*lock, portMAX_DELAY);
#else
    // Add implementation
    #pragma message("Implement OS-dependent definition of OsalLock()")
#endif
}

/*
 * Unlocks a mutex or spinlock.
 * Parameters:
 *   lock: Pointer to the lock to be released.
 * Returns:
 *   None
 */
void OsalUnlockMutex(OsalLockType *lock)
{
#if OS_IS(LINUX)
    pthread_mutex_unlock(lock);
#elif OS_IS(FREERTOS)
    xSemaphoreGive(*lock);
#else
    // Add implementation for future OSes
    #pragma message("Implement OS-dependent definition of OsalUnlock()")
#endif
}

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
LonStatusCode OsalCreateEvent(OsalHandle *eventHandle)
{
#if OS_IS(LINUX)
    OsalEvent *event = (OsalEvent*)OsalAllocateMemory(sizeof(OsalEvent));
    pthread_cond_init(&event->cond, NULL);
    pthread_mutex_init(&event->mutex, NULL);
    event->flag = 0;
    *eventHandle = event;
    return LonStatusNoError;
#elif OS_IS(FREERTOS)
    OsalEvent *event = (OsalEvent*)OsalAllocateMemory(sizeof(OsalEvent));
    event->semaphore = xSemaphoreCreateBinary();
    event->flag = 0;
    *eventHandle = event;
    return event->semaphore ? LonStatusNoError : LonStatusEventError;
#else
    // Add implementation
    #pragma message("Implement OS-dependent definition of OsalCreateEvent()")
    *eventHandle = NULL;
    return LonStatusEventError;
#endif
}

/*
 * Deletes an event object.
 * Parameters:
 *   eventHandle: Pointer to the event handle to be deleted.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */ 
LonStatusCode OsalDeleteEvent(OsalHandle *eventHandle)
{
#if OS_IS(LINUX)
    pthread_cond_destroy(&(*eventHandle)->cond);
    pthread_mutex_destroy(&(*eventHandle)->mutex);
    free(*eventHandle);
    *eventHandle = NULL;
    return LonStatusNoError;
#elif OS_IS(FREERTOS)
    vSemaphoreDelete((*eventHandle)->semaphore);
    vPortFree(*eventHandle);
    *eventHandle = NULL;
    return LonStatusNoError;
#else
    // Add implementation
    #pragma message("Implement OS-dependent definition of OsalDeleteEvent()")
    *eventHandle = NULL;
    return LonStatusEventError;
#endif
}

/*
 * Waits for an event to fire.
 * Parameters:
 *   eventHandle: Pointer to the event handle to wait for.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */ 
LonStatusCode OsalWaitForEvent(OsalHandle eventHandle, unsigned int waittime)
{
#if OS_IS(LINUX)
    OsalEvent *event = (OsalEvent*)eventHandle;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += waittime / 1000;
    ts.tv_nsec += (waittime % 1000) * 1000000;
    pthread_mutex_lock(&event->mutex);
    while (event->flag == 0) {
        if (pthread_cond_timedwait(&event->cond, &event->mutex, &ts) != 0) break;
    }
    LonStatusCode retVal = (event->flag == 1) ? LonStatusNoError : LonStatusTimeout;
    event->flag = 0;
    pthread_mutex_unlock(&event->mutex);
    return retVal;
#elif OS_IS(FREERTOS)
    OsalEvent *event = (OsalEvent*)eventHandle;
    if (xSemaphoreTake(event->semaphore, pdMS_TO_TICKS(waittime)) == pdTRUE) {
        event->flag = 0;
        return LonStatusNoError;
    }
    return LonStatusTimeout;
#else
    // Add implementation
    #pragma message("Implement OS-dependent definition of OsalWaitForEvent()")
    return LonStatusEventError;
#endif
}

/*
 * Sets (fires) an event.
 * Parameters:
 *   eventHandle: Pointer to the event handle to be set.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode OsalSetEvent(OsalHandle eventHandle)
{
#if OS_IS(LINUX)
    OsalEvent *event = (OsalEvent*)eventHandle;
    pthread_mutex_lock(&event->mutex);
    if (event->flag == 0) {
        event->flag = 1;
        pthread_cond_signal(&event->cond);
    }
    pthread_mutex_unlock(&event->mutex);
    return LonStatusNoError;
#elif OS_IS(FREERTOS)
    OsalEvent *event = (OsalEvent*)eventHandle;
    if (event->flag == 0) {
        event->flag = 1;
        xSemaphoreGive(event->semaphore);
    }
    return LonStatusNoError;
#else
    // Add implementation
    #pragma message("Implement OS-dependent definition of OsalSetEvent()")
    return LonStatusEventError;
#endif
}

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
OsalTickCount OsalGetTickCount(void)
{
#if OS_IS(LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (OsalTickCount)((ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL));
#elif OS_IS(FREERTOS)
    // Return tick count * (ms per tick)
    TickType_t ticks = xTaskGetTickCount();
    return (OsalTickCount)(ticks * 1000 / configTICK_RATE_HZ);
#elif PLATFORM_IS(RPI) || PLATFORM_IS(RPI_PICO)
    unsigned long msecNow;
    msecNow = millis();
    return msecNow;
#else
    // Future implementation
    #pragma message("Implement OS-dependent definition of OsalGetTickCount()")
    return 0;
#endif
}

/*
 * Gets the number of ticks in a second.
 * Parameters:
 *   None.
 * Returns:
 *   The number of ticks in one second.
 * Notes:
 *   See also OsalGetTickCount().
 */
OsalTickCount OsalGetTicksPerSecond(void)
{
    return 1000;
}

/*
 * Creates a thread to run the specified entry point function.
 * Parameters:
 *   threadEntry: Pointer to the thread entry point function.
 *   threadData:  Pointer to data to be passed to the thread entry point.
 * Returns:
 *   Thread ID if successful; 0 or NULL if unsuccessful.
 */
OsalThreadId OsalCreateThread(OsalEntryPoint threadEntry, void* threadData)
{
#if OS_IS(LINUX)
    pthread_t tid;
    int result = pthread_create(&tid, NULL, threadEntry, threadData);
    return (result == 0) ? tid : (pthread_t)0;

#elif OS_IS(FREERTOS)
    TaskHandle_t handle = NULL;
    BaseType_t result = xTaskCreate(threadEntry, "U61Thread", configMINIMAL_STACK_SIZE, threadData, tskIDLE_PRIORITY+1, &handle);
    return (result == pdPASS) ? handle : NULL;

#else
    // Future implementation
    #pragma message("Implement OS-dependent definition of OsalCreateThread()")
    return NULL;
#endif
}

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
LonStatusCode OsalSleep(unsigned int msecs)
{
    if (msecs > MAX_TIMEOUT_MS) {
        msecs = MAX_TIMEOUT_MS;
    }
#if OS_IS(LINUX)
    struct timespec ts;
    ts.tv_sec = msecs / 1000;
    ts.tv_nsec = (msecs % 1000) * 1000000L;
    nanosleep(&ts, NULL);   // High resolution sleep in user-space
#elif OS_IS(LINUX_KERNEL)
    msleep(msecs);          // Sleeps for the given number of milliseconds
#elif OS_IS(FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(msecs)); // Uses the FreeRTOS tick conversion macro
#elif PLATFORM_IS(RPI) || PLATFORM_IS(RPI_PICO)
    delay(msecs);
#else
    #pragma message("Implement millisecond delay")
    return LonStatusNotImplemented;
#endif
    return LonStatusNoError;    
}

/*
 * Allocates memory block of the specified size.
 * Parameters:
 *   size: Number of bytes to allocate
 * Returns:
 *   Pointer to allocated memory, or NULL if allocation failed
 */
void* OsalAllocateMemory(size_t size)
{
#if OS_IS(LINUX)
    return malloc(size);
#elif OS_IS(FREERTOS)
    return pvPortMalloc(size);
#else
    void *ptr;
    if (gp->mallocUsedSize + sizeIn > MALLOC_SIZE) {
        // No space for requested size
        OsalPrintError(LonStatusMemoryAllocFailure, "OsalAllocateMemory: out of memory");
        return(NULL);
    }
    ptr = gp->mallocStorage + gp->mallocUsedSize;
    gp->mallocUsedSize += sizeIn;
    return(ptr);
#endif
}

/*
 * Frees memory block previously allocated with OsalAllocateMemory().
 * Parameters:
 *   buf: pointer to memory block to free
 * Returns:
 *   None
 */
void OsalFreeMemory(void *buf)
{
#if OS_IS(LINUX)
    free(buf);
#elif OS_IS(FREERTOS)
    vPortFree(buf);
#else
    // Future implementation
    #pragma message("Implement OS-dependent definition of OsalFreeMemoryMemory()")
#endif
}

/*****************************************************************
 * Section: Message Reporting Global and Function Definitions
 *****************************************************************/
 /*
  * Formats a debug or error message.
  * Parameters:
  *   buffer: Pointer to buffer to receive formatted message.
  *   bufferLen: Length of the buffer in bytes
  *   errorCode: Error code to include in message, or LonStatusNoError for none
  *   errorString: printf-style format string for message
  *   args: Variable argument list for format string
  * Returns:
  *   None
  * Notes:
  *  This is a helper function used by OsalPrintError(), OsalPrintSysError(),
  *  and OsalPrintDebug().  If errorCode is >= 0, the message is prefixed with
  *  "Error <errorCode>: ".
  */
 static void OsalFormatErrorString(char *buffer, size_t bufferLen, LonStatusCode errorCode, const char *errorString, va_list args)
{
    if (errorCode == LonStatusNoError) {
        vsnprintf(buffer, bufferLen, errorString, args);
    } else {
        snprintf(buffer, bufferLen, "Error %d: ", errorCode);
        vsnprintf(buffer + strlen(buffer), bufferLen - strlen(buffer), errorString, args);
    }
}

/*
 * Sets the current log level for message reporting.
 * Parameters:
 *   level: New log level (LOG_NONE, LOG_ERROR, LOG_DEBUG, LOG_TRACE)
 * Returns:
 *   None
 */
static LogLevel logLevel = INITIAL_LOG_LEVEL;

void OsalSetLogLevel(LogLevel level) {
    logLevel = level;
}

/*
 * Gets the current log level for message reporting.
 * Parameters:
 *   None
 * Returns:
 *   Current log level (LOG_NONE, LOG_ERROR, LOG_DEBUG, LOG_TRACE)
 */
LogLevel OsalGetLogLevel(void)
{
    return logLevel;
}

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
void OsalPrintSysError(LonStatusCode errorCode, char *errorString, ...)
{
    if (logLevel <= LOG_NONE)
        return;

    char formatted[OSAL_ERROR_STRING_MAXLEN];
    va_list args;
    va_start(args, errorString);
    OsalFormatErrorString(formatted, sizeof(formatted), errorCode, errorString, args);
    va_end(args);

#if OS_IS(LINUX)
    fprintf(stderr, "%s (strerror(errno))\n", formatted, strerror(errno));
#elif OS_IS(FREERTOS)
    printf("%s\n", formatted);
#else
    // Implement as needed
    #pragma message("Implement OS-dependent definition of OsalPrintSysError()")
#endif
}

/*
 * Logs an error code and prints an error message with optional message code and text.
 * Parameters:
 *   errorCode: Error code to log and include in message, or LonStatusNoError for none
 *   errorString: printf-style format string for message
 *   ...: Variable arguments for format string
 * Returns:
 *   None
 * Notes:
 *  This function only prints messages if the current log level is LOG_ERROR
 *  or LOG_DEBUG.
 */
void OsalPrintError(LonStatusCode errorCode, char *errorString, ...)
{
    // Log to non-volatile memory if the error code has changed to avoid wearing
    // out flash memory with redundant values
    if (errorCode != LonStatusNoError && eep->errorLog != errorCode){
        eep->errorLog = errorCode;
        LCS_WriteNvm();
    }

    if (logLevel <= LOG_NONE)
        return;

    char formatted[OSAL_ERROR_STRING_MAXLEN];
    va_list args;
    va_start(args, errorString);
    OsalFormatErrorString(formatted, sizeof(formatted), errorCode, errorString, args);
    va_end(args);

#if OS_IS(LINUX)
    fprintf(stderr, "%s\n", formatted);
#elif OS_IS(FREERTOS)
    printf("%s\n", formatted);
#else
    // Implement as needed
    #pragma message("Implement OS-dependent definition of OsalPrintError()")
#endif
}

/*
 * Prints a debug message with optional message code and text.
 * Parameters:
 *   errorCode: Error code to include in message, or -1 for none
 *   errorString: printf-style format string for message
 *   ...: Variable arguments for format string
 * Returns:
 *   None
 * Notes:
 *  This function only prints messages if the current log level is LOG_DEBUG.
 */
void OsalPrintDebug(LonStatusCode errorCode, char *errorString, ...)
{
    if (logLevel <= LOG_ERROR)
        return;

    char formatted[OSAL_ERROR_STRING_MAXLEN];
    va_list args;
    va_start(args, errorString);
    OsalFormatErrorString(formatted, sizeof(formatted), errorCode, errorString, args);
    va_end(args);

#if OS_IS(LINUX)
    fprintf(stderr, "%s\n", formatted);
#elif OS_IS(FREERTOS)
    printf("%s\n", formatted);
#else
    // Implement as needed
    #pragma message("Implement OS-dependent definition of OsalPrintDebug()")
#endif
}

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
void OsalPrintTrace(LonStatusCode errorCode, char *errorString, ...)
{
    if (logLevel <= LOG_DEBUG)
        return;

    char formatted[OSAL_ERROR_STRING_MAXLEN];
    va_list args;
    va_start(args, errorString);
    OsalFormatErrorString(formatted, sizeof(formatted), errorCode, errorString, args);
    va_end(args);

#if OS_IS(LINUX)
    fprintf(stderr, "%s\n", formatted);
#elif OS_IS(FREERTOS)
    printf("%s\n", formatted);
#else
    // Implement as needed
    #pragma message("Implement OS-dependent definition of OsalPrintTrace()")
#endif
}

