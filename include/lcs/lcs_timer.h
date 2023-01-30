//
// lcs_timer.h
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

/*******************************************************************************
   Purpose:  Interfaces for timer APIs.
*******************************************************************************/

#ifndef _TIMER_H
#define _TIMER_H

#include <stddef.h>
#include "IzotConfig.h"
#include "IzotPlatform.h"
#include "IzotTypes.h"
#include "lcs_eia709_1.h"

// Set LON timer maximum duration to 1/4 maximum full range for a 32-bit
// millisecond timer which is about 12 days
#define LON_TIMER_MAX_DURATION 0x3FFFFFFF

void    SetLonTimer(LonTimer *timerOut, IzotUbits32 initValueIn);
void    SetLonRepeatTimer(LonTimer *timerOut, IzotUbits32 initValueIn);
// Returns true once upon expiration.
IzotByte LonTimerExpired(LonTimer *timerInOut);
// Returns true as long as timer is running.
IzotByte LonTimerRunning(LonTimer *timerInOut);
IzotUbits32 LonTimerRemaining(LonTimer *timerInOut);

#endif // _TIMER_H