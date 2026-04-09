/*
 * lcs_timer.c
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Timer Functions
 * Purpose: Provides interfaces for managing LON timers.
 * Notes:   Timers are used for various timing operations within the LON stack.
 */

#include "lcs/lcs_timer.h"
#include "izot/iap_types.h"

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
void SetLonTimer(LonTimer *timer, uint32_t duration)
{
	if (timer) {
		if (duration) {
			// Limit duration
			duration = min(duration, LON_TIMER_MAX_DURATION);
			timer->repeatTimeout = 0;
			timer->expiration = OsalGetTickCount() + duration;

			if (timer->expiration == 0) {
				// Zero signals that a timer is not running so disallow it
				timer->expiration = 1;
			}
		} else {
			timer->expiration = 0;
		}
	}
}

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
void SetLonRepeatTimer(LonTimer *timer, uint32_t first_duration, uint32_t repeat_duration)
{
	if (timer) {
		// Limit durations
		first_duration = min(first_duration, LON_TIMER_MAX_DURATION);
		repeat_duration = min(repeat_duration, LON_TIMER_MAX_DURATION);

		SetLonTimer(timer, first_duration);
		timer->repeatTimeout = repeat_duration;
	}
}

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
bool LonTimerExpired(LonTimer *timer)
{
	if (!timer || !timer->expiration) {
		return false;
	}
	int32_t delta = (int32_t) (timer->expiration - OsalGetTickCount());
	bool isExpired = timer->expiration && delta <= 0;
	if (isExpired) {
		timer->expiration = 0;
		if (timer->repeatTimeout) {
			uint32_t t = timer->repeatTimeout;
			if ((int32_t) (t + delta) < 0) {
				// Cap the adjustment at 0
				delta = 0;
			}
			SetLonTimer(timer, t + delta);
			timer->repeatTimeout = t;
		}
	}
    return isExpired;
}

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
bool LonTimerRunning(LonTimer *timer)
{
	return timer && timer->expiration && ((int32_t) (timer->expiration - OsalGetTickCount()) > 0);
}

/*
 * Checks time remaining for a timer.
 * Parameters:
 *   timer: Pointer to the timer to check
 * Returns:
 *   Number of milliseconds remaining until the timer expires, or 0 if
 *   the timer is not running or has expired
 */
uint32_t LonTimerRemaining(LonTimer *timer)
{
	uint32_t remaining = 0;

	if (timer && LonTimerRunning(timer)) {
		remaining = timer->expiration - OsalGetTickCount();
	}
	return remaining;
}

/*
 * Starts a stopwatch.
 * Parameters:
 *   watch: Pointer to the stopwatch to start
 * Returns:
 *   None
 */
void StartLonWatch(LonWatch *watch)
{
	if (watch) {
		watch->start = OsalGetTickCount();
	}
}

/*
 * Stops a stopwatch.
 * Parameters:
 *   watch: Pointer to the stopwatch to stop
 * Returns:
 *   None
 */
void StopLonWatch(LonWatch *watch)
{
	if (watch) {
		watch->start = 0;
	}
}

/*
 * Read the elapsed time on a stopwatch in milliseconds.
 * Parameters:
 *   watch: Pointer to the stopwatch to read
 * Returns:
 *   Number of milliseconds since the stopwatch was started with
 *   StartLonWatch(), or 0 if the stopwatch is not running
 */
uint32_t LonWatchElapsed(LonWatch *watch)
{
	uint32_t duration = 0;
	if (watch && watch->start) {
		duration = OsalGetTickCount() - watch->start;
	}
	return duration;
}

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
uint32_t ElapsedTimeToMs(const SNVT_elapsed_tm *elapsed_time)
{
	if (elapsed_time == NULL) {
		return 0;
	}
	uint16_t days = IZOT_GET_UNSIGNED_WORD(elapsed_time->day);
	if (days == 0xFFFF) {
		// Invalid value, return maximum duration
		return LON_TIMER_MAX_DURATION;
	}
	if (days >= 50) {
		// Cap at maximum duration
		return LON_TIMER_MAX_DURATION;
	}
	uint32_t total_ms = days * 864000000UL; // 24 hours/day * 60 minutes/hour * 60 seconds/minute * 1000 ms/second
	uint32_t hours_ms = (uint32_t)elapsed_time->hour * 3600000UL; // 60 minutes/hour * 60 seconds/minute * 1000 ms/second
	if (total_ms > LON_TIMER_MAX_DURATION - hours_ms) {
		// Cap at maximum duration
		return LON_TIMER_MAX_DURATION;
	}
	total_ms += hours_ms;
	uint32_t minutes_ms = (uint32_t)elapsed_time->minute * 60000UL; // 60 seconds/minute * 1000 ms/second
	if (total_ms > LON_TIMER_MAX_DURATION - minutes_ms) {
		// Cap at maximum duration
		return LON_TIMER_MAX_DURATION;
	}
	total_ms += minutes_ms;
	uint32_t seconds_ms = (uint32_t)elapsed_time->second * 1000UL; // 1000 ms/second
	if (total_ms > LON_TIMER_MAX_DURATION - seconds_ms) {
		// Cap at maximum duration
		return LON_TIMER_MAX_DURATION;
	}
	total_ms += seconds_ms;
	uint32_t milliseconds = IZOT_GET_UNSIGNED_WORD(elapsed_time->millisecond);
	if (total_ms > LON_TIMER_MAX_DURATION - milliseconds) {
		// Cap at maximum duration
		return LON_TIMER_MAX_DURATION;
	}
	total_ms += milliseconds;
	return total_ms;
}