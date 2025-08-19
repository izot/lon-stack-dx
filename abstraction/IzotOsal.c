//
// IzotOsal.c
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
 * Title: Operaing System Abstaction Layer file
 *
 * Abstract:
 * This file contains the Platform dependent APIs. This file contains
 * APIs for FreeRTOS to get timing information.
 */

#include "abstraction/IzotOsal.h"

#if PROCESSOR_IS(MC200)
    #include <wm_os.h>
#else
    #if PLATFORM_IS(RPI_PICO)
        #include <Arduino.h>
    #endif
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>
    #include <unistd.h>
#endif

/*
 * Function:   IzotGetTickCount
 * Get the current system tick count.
 *
 * Returns:
 * The system tick count.
 * 
 * Remarks:
 * Returns the number of ticks since operating system startup.  The number of 
 * ticks per second can be determined by calling <GetTicksPerSecond>.
 * 
 */
OsalTickCount IzotGetTickCount(void)
{
    // Return the OS tick count.
#if PLATFORM_IS(FRTOS_ARM_EABI)
    return os_ticks_get();
#elif PLATFORM_IS(RPI) || PLATFORM_IS(RPI_PICO)
    unsigned long msecNow;

    msecNow = millis();
    return msecNow;
#else
    struct timespec timeNow;

    clock_gettime(CLOCK_REALTIME, &timeNow);
    return (timeNow.tv_sec * 1000) + (timeNow.tv_nsec / 1000000L);
#endif
}

/*
 * Function:   GetTicksPerSecond
 * Get the number of ticks in a second.
 *
 * Returns:
 * The number of ticks in one second.
 * 
 * Remarks:
 * Returns the number of ticks in one second.
 * 
 * See Also:
 * <IzotGetTickCount>.
 */
OsalTickCount GetTicksPerSecond(void){
    return 1000;
}

/*
 * Function:   OsalSleep
 * Sleep for a specified number of ticks.
 *
 * Parameters:
 * ticks - The number of ticks to sleep 
 * 
 * Returns:
 * <OsalStatus>.
 * 
 * See Also:
 * <IzotGetTickCount>.
 * 
 */
OsalStatus OsalSleep(unsigned int ticks)
{
    if (ticks <= MAX_TIMEOUT_TICKS) {
#if PLATFORM_IS(FRTOS_ARM_EABI)
        os_thread_sleep(os_msec_to_ticks(ticks));
#elif PLATFORM_IS(RPI) || PLATFORM_IS(RPI_PICO)
        delay(ticks);
#else
        nanosleep((const struct timespec[]){{(int)(ticks / 1000), (ticks % 1000) * 1000000L}}, NULL);
#endif
    } else {
        // Tick count exceeds the maximum number of ticks supported by 
        // uC/OS-II.  It will be necessary to wait more than one time.
        // But first, get the start time so that the remaining timeout can be 
        // calculated.
        //  
        OsalTickCount startTime = IzotGetTickCount();
        OsalTickCount ticksRemaining = ticks;

        do {        
            uint16_t timeout;
            if (ticksRemaining > MAX_TIMEOUT_TICKS) {
                // Cannot wait that long.  Wait for MAX_TIMEOUT_TICKS instead. 
                // 
                timeout = MAX_TIMEOUT_TICKS; 
            } else {
                timeout = ticksRemaining;
            }
            ticksRemaining -= timeout;
            
            // Wait for "timeout" tick counts.      
            #if PLATFORM_IS(FRTOS_ARM_EABI)
                os_thread_sleep(os_msec_to_ticks(timeout));
            #elif PLATFORM_IS(RPI) || PLATFORM_IS(RPI_PICO)
                delay(ticks);
            #else
                nanosleep((const struct timespec[]){{(int)(timeout / 1000), (timeout % 1000) * 1000000L}}, NULL);
            #endif

            if (ticksRemaining) {
                // Recalculate how many ticks are really remaining to prevent 
                // drift. 
                // 
                OsalTickCount elapsedTime = IzotGetTickCount() - startTime;
                if (ticks > elapsedTime) {
                    // ticksRemaining is the difference between the original 
                    // tick count and the number of elapsed ticks.
                    // 
                    ticksRemaining = ticks - elapsedTime;
                } else {
                    // Already timed out.
                    ticksRemaining = 0;
                }
            }
        } while (ticksRemaining);
    }

    return OSALSTS_SUCCESS;    
}

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
void *OsalMalloc(unsigned int size)
{
#if PLATFORM_IS(FRTOS_ARM_EABI)
    return os_mem_alloc(size);
#else
    return malloc(size);
#endif  // PLATFORM_IS(FRTOS_ARM_EABI)
}

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
void OsalFree(void *ptr)
{
#if PLATFORM_IS(FRTOS_ARM_EABI)
    os_mem_free(ptr);
#else
    free(ptr);
#endif  // PLATFORM_IS(FRTOS_ARM_EABI)
}
