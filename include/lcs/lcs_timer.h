/*
 * lcs_timer.h
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Timer Functions
 * Purpose: Provides interfaces for managing LON timers.
 * Notes:   Timers are used for various timing operations within the LON stack.
 */

#ifndef _TIMER_H
#define _TIMER_H

#include <stddef.h>
#include <stdbool.h>

#include "izot/IzotPlatform.h"
#include "izot/lon_types.h"
#include "lcs/lcs_eia709_1.h"

// Set LON timer maximum duration to 1/2 maximum full range for a 32-bit
// millisecond timer which is about 24 days
#define LON_TIMER_MAX_DURATION 0x7FFFFFFF

/*
 * Starts a one-shot timer.
 * Parameters:
 *   timer: Pointer to the timer to set
 *   duration: Initial timer duration in milliseconds; set to 0
 *       	   to stop the current time interval
 * Returns:
 *   None
 * Notes:
 *   The interval can be up to about 24 days.  Setting the value to 0
 *   stops the current time interval, but does not terminate a repeating
 *   timer.  Use SetLonRepeatTimer() to stop any repeats.
 */
void SetLonTimer(LonTimer *timer, uint32_t duration);

/*
 * Starts a repeating timer.
 * Parameters:
 *   timer: Pointer to the timer to set
 *   first_duration: First interval timer duration in milliseconds
 *   repeat_duration: Duration in milliseconds for repeated intervals
 * Returns:
 *   None
 * Notes:
 *   The intervals can be up to about 24 days.  Set both values to 0
 *   to stop the current time interval and any repeats.  Both intervals
 *   are limited to about 24 days.
 */
void SetLonRepeatTimer(LonTimer *timer, uint32_t first_duration, uint32_t repeat_duration);

/*
 * Checks if a timer has expired since the last time it was started.
 * Parameters:
 *   timer: Pointer to the timer to check
 * Returns:
 *   true if the timer has expired since the last time it was started;
 *   false if it has expired or has already been reported as expired.
 * Notes:
 *   Unlike LonTimerRunning(), this function returns true only once after
 *   each expiration.
 */
bool LonTimerExpired(LonTimer *timer);

/*
 * Checks if a timer is currently running.
 * Parameters:
 *   timer: Pointer to the timer to check
 * Returns:
 *   true if the timer is currently running
 * Notes:
 *   Returns true if the timer has not expired since the last
 *   time it was started.  This function is different from
 *   LonTimerExpired() in that it will return true as long
 *   as the timer is running, whereas LonTimerExpired() only
 *   returns true once after each expiration.
 */
bool LonTimerRunning(LonTimer *timer);

/*
 * Checks time remaining for a timer.
 * Parameters:
 *   timer: Pointer to the timer to check
 * Returns:
 *   Number of milliseconds remaining until the timer expires, or 0 if
 *   the timer is not running or has expired
 */
uint32_t LonTimerRemaining(LonTimer *timer);

/*
 * Starts a stopwatch.
 * Parameters:
 *   watch: Pointer to the stopwatch to start
 * Returns:
 *   None
 */
void StartLonWatch(LonWatch *watch);

/*
 * Stops a stopwatch.
 * Parameters:
 *   watch: Pointer to the stopwatch to stop
 * Returns:
 *   None
 */
void StopLonWatch(LonWatch *watch);

/*
 * Read the elapsed time on a stopwatch in milliseconds.
 * Parameters:
 *   watch: Pointer to the stopwatch to read
 * Returns:
 *   Number of milliseconds since the stopwatch was started with
 *   StartLonWatch(), or 0 if the stopwatch is not running
 */
uint32_t LonWatchElapsed(LonWatch *watch);

/*
 * Converts an IAP SNVT_elapsed_tm value to milliseconds.
 * Parameters:
 *   elapsed_time: Pointer to the SNVT_elapsed_tm value to convert
 * Returns:
 *   The equivalent duration in milliseconds, or 0 if the input pointer is NULL.
 * Notes:
 *   The maximum representable duration is limited to one half of the full
 *   unsigned 32-bit range, which is approximately 49.7 days.  Any input
 *   exceeding this duration is capped at LON_TIMER_MAX_DURATION 
 *   milliseconds which is about 24 days.  The function also checks for
 *   potential overflow when calculating the total duration and caps at
 *   LON_TIMER_MAX_DURATION if an overflow would occur.  If the input value
 *   is invalid (e.g., day field is 0xFFFF), the function= returns
 *   LON_TIMER_MAX_DURATION.
 */
uint32_t ElapsedTimeToMs(const SNVT_elapsed_tm *elapsed_time);

#endif // _TIMER_H