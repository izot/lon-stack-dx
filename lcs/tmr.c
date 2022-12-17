//
// tmr.c
//
// Copyright (C) 2022 Dialog Semiconductor
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

// This file contains basic timer primitives.  The APIs
// support 32-bit millisecond timers and stopwatches.
//

#include <assert.h>

#include "echstd.h"
#include "tmr.h"

// Start a millisecond timer (works up to about 24 days).  
void TMR_Start(TmrTimer *pTimer, TmrDuration milliseconds)
{
	// If the timer duration is too large, timers don't work
	assert((Int32)milliseconds >= 0);

	pTimer->repeatTimeout = 0;
	pTimer->expiration = TMR_GetCurrentTime() + milliseconds;
	if (pTimer->expiration == 0)
	{
		// Zero signals that a timer is not running so disallow it.
		pTimer->expiration = 1;
	}
}

// Start a millisecond timer (works up to about 24 days) that repeats.  It will report TMR_Expired() once and then start running again.
// Equivalent to calling TMR_Start() again after expiration.
void TMR_StartRepeating(TmrTimer *pTimer, TmrDuration milliseconds)
{
	TMR_Start(pTimer, milliseconds);
	pTimer->repeatTimeout = milliseconds;
}

// Stop a timer
void TMR_Stop(TmrTimer *pTimer)
{
    pTimer->expiration = 0;
}

// Check if a timer has expired (returns true only if the timer was ever started and has expired - only returns true once!)
Bool TMR_Expired(TmrTimer *pTimer)
{
    Int32 delta = (Int32)(pTimer->expiration - TMR_GetCurrentTime());
    Bool isExpired = pTimer->expiration && delta <= 0;
	if (isExpired)
	{
		pTimer->expiration = 0;
		if (pTimer->repeatTimeout)
		{
		    TmrDuration t = pTimer->repeatTimeout;
			TMR_Start(pTimer, pTimer->repeatTimeout + delta);
			pTimer->repeatTimeout = t;
		}
	}
    return isExpired;
}

// Check if a timer is running (returns true only if the timer was ever started and hasn�t expired yet).  Similar to !TMR_Expired() but allows you to detect that a timer was never started.
Bool TMR_Running(TmrTimer *pTimer)
{
	return pTimer->expiration && !TMR_Expired(pTimer);
}

// Start a stop watch
void TMR_StartWatch(TmrWatch *pTimer)
{
    pTimer->start = TMR_GetCurrentTime();
    if (pTimer->start == 0)
	{
		// 0 indicates not running so force to previous millisecond.
		pTimer->start = (TmrDuration)-1;
	}
}

// Stop a stop watch
void TMR_StopWatch(TmrWatch *pTimer)
{
	pTimer->start = 0;
}

// Get milliseconds past on stop watch.  Return 0 if not running
TmrDuration TMR_Elapsed(TmrWatch *pTimer)
{
	TmrDuration duration = 0;
	if (pTimer->start)
	{
		duration = TMR_GetCurrentTime() - pTimer->start;
	}
	return duration;
}
