//
// tmr.h
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

// This file contains the definitions for basic timer primitives.  The APIs
// support 32-bit millisecond timers and stopwatches.

#ifndef _TMR_H
#define _TMR_H

#include "echstd.h"

typedef UInt32 TmrDuration;

typedef struct
{
	TmrDuration	expiration;		// Time to expire
	TmrDuration	repeatTimeout;  // Repeat timeout on expiration (0 means not repeating)
} TmrTimer;

typedef struct
{
	TmrDuration	start;			// Time watch started
} TmrWatch;

//
// Basic time primitives - init TMR and return a running counter of milliseconds.  Wraps every 49 days.
//
void TMR_Init(void);
__monitor TmrDuration TMR_GetCurrentTime(void);

//
// Millisecond timers
//

C_API_START

// Start a millisecond timer (works up to about 24 days).  
void TMR_Start(TmrTimer *pTimer, TmrDuration milliseconds);

// Start a repeating millisecond timer (works up to about 24 days).
void TMR_StartRepeating(TmrTimer *pTimer, TmrDuration milliseconds);

// Stop a timer
void TMR_Stop(TmrTimer *pTimer);

// Check if a timer has expired (returns true only if the timer was ever started and has expired - only returns true once!).
Bool TMR_Expired(TmrTimer *pTimer);

// Check if a timer is running (returns true only if the timer was ever started and hasn�t expired yet).  Similar to !TMR_Expired() but allows you to detect that a timer was never started.
Bool TMR_Running(TmrTimer *pTimer);

TmrDuration TMR_Remaining(TmrTimer *pTimer);

//
// Millisecond stopwatches
//

// Start a stop watch (for up to about 24 days)
void TMR_StartWatch(TmrWatch *pTimer);

// Stop a stop watch
void TMR_StopWatch(TmrWatch *pTimer);

// Get milliseconds past on stop watch
TmrDuration TMR_Elapsed(TmrWatch *pTimer);

C_API_END

#endif  // _TMR_H





	