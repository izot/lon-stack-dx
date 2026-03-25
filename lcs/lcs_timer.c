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

/*****************************************************************
Function:  SetLonTimer
Returns:   None
Purpose:   Sets a timer value to a given value in milliseconds.
Comments:  Sets the timer to the specified value. The interval
		   can be up to about 24 days.  Set the value to 0 to stop 
		   the current time interval, but does not terminate a
		   repeating timer.  Use SetLonRepeatTimer() to stop
		   any repeats.
******************************************************************/
void SetLonTimer(LonTimer *timerOut, uint32_t initValueIn)
{
	if (initValueIn) {
		// Limit duration
		initValueIn = min(initValueIn, LON_TIMER_MAX_DURATION);
    	timerOut->repeatTimeout = 0;
    	timerOut->expiration = OsalGetTickCount() + initValueIn;

		if (timerOut->expiration == 0) {
			// Zero signals that a timer is not running so disallow it
			timerOut->expiration = 1;
		}
	} else {
		timerOut->expiration = 0;
	}
}

/*****************************************************************
Function:  SetLonRepeatTimer
Returns:   None
Purpose:   Sets a repeating timer value to an initial value for
		   the first interval in milliseconds and repeating at the 
		   a repeat interval specified in milliseconds.
Comments:  The intervals can be up to about 24 days.  Set both 
		   values to 0 to stop the current time interval and any 
		   repeats.
******************************************************************/
void SetLonRepeatTimer(LonTimer *timerOut, uint32_t initValueIn, uint32_t repeatValueIn)
{
    // Limit durations
    initValueIn = min(initValueIn, LON_TIMER_MAX_DURATION);
	repeatValueIn = min(repeatValueIn, LON_TIMER_MAX_DURATION);

	SetLonTimer(timerOut, initValueIn);
   	timerOut->repeatTimeout = repeatValueIn;
}

/*****************************************************************
Function:  LonTimerExpired
Returns:   TRUE if the timer has expired. FALSE if the timer has
           not expired or it has expired but has already been
           reported as expired.
Reference: None
Purpose:   To update given timer and test if it expired.
Comments:  V1 used TMR_Expired().
******************************************************************/
bool LonTimerExpired(LonTimer *timerInOut)
{
	int32_t delta = (int32_t) (timerInOut->expiration - OsalGetTickCount());
	bool isExpired = timerInOut->expiration && delta <= 0;
	if (isExpired) {
		timerInOut->expiration = 0;
		if (timerInOut->repeatTimeout) {
			uint32_t t = timerInOut->repeatTimeout;
			if ((int32_t) (t + delta) < 0) {
				// Cap the adjustment at 0
				delta = 0;
			}
			SetLonTimer(timerInOut, t + delta);
			timerInOut->repeatTimeout = t;
		}
	}
    return isExpired;
}

/*****************************************************************
Function:  LonTimerRunning
Returns:   TRUE if the timer is still running.
Purpose:   Check if a timer is still running.  Returns true only
           if the timer was ever started and has not expired yet.  
           Different from LonTimerExpired() in that it will return 
		   true once timer expires whereas LonTimerExpired() only 
		   returns true once.
Comments:  V1 used TMR_Running().
******************************************************************/
bool LonTimerRunning(LonTimer *timerInOut)
{
	return timerInOut->expiration && !LonTimerExpired(timerInOut);
}

/*****************************************************************
Function:  LonTimerRemaining
Returns:   Number of milliseconds left on the timer.
Purpose:   Return the number of milliseconds left on a timer, or
		   return 0 if the timer is not running or expired.
******************************************************************/
uint32_t LonTimerRemaining(LonTimer *timerInOut)
{
	uint32_t remaining = 0;

	if (LonTimerRunning(timerInOut)) {
		remaining = timerInOut->expiration - OsalGetTickCount();
	}
	return remaining;
}

/*****************************************************************
Function:  StartLonWatch
Returns:   None.
Purpose:   Start a stopwatch.
******************************************************************/
void StartLonWatch(LonWatch *watch)
{
	watch->start = OsalGetTickCount();
}

/*****************************************************************
Function:  StopLonWatch
Returns:   None.
Purpose:   Stop a stopwatch.
******************************************************************/
void StopLonWatch(LonWatch *watch)
{
	watch->start = 0;
}

/*****************************************************************
Function:  LonWatchElapsed
Returns:   Number of milliseconds since the watch was started.
Purpose:   Return the number of milliseconds since StartLonWatch()
		   was called.
******************************************************************/
uint32_t LonWatchElapsed(LonWatch *watch)
{
	uint32_t duration = 0;
	if (watch->start) {
		duration = OsalGetTickCount() - watch->start;
	}

	return duration;
}

