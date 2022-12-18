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

#include "tmr.h"

typedef TmrTimer MsTimer;

IzotUbits32  GetCurrentMsTime(void);
void    MsTimerSet(MsTimer *timerOut, IzotUbits16 initValueIn);
// Returns true once upon expiration.
IzotByte MsTimerExpired(MsTimer *timerInOut);
// Returns true as long as timer is running.
IzotByte MsTimerRunning(MsTimer *timerInOut);

#endif // _TIMER_H