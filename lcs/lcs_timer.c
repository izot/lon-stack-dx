//
// lcs_timer.c
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
       Purpose:  Implement timer APIs.
*******************************************************************************/
#include <stddef.h>
#include "IzotTypes.h"
#include "lcs_eia709_1.h"
#include "lcs_timer.h"
#include "lcs_node.h"

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
void SetLonTimer(LonTimer *timerOut, IzotUbits16 initValueIn)
{
	if (initValueIn) {
    	timerOut->repeatTimeout = 0;
    	timerOut->expiration = IzotGetTickCount() + initValueIn;

		if (timerOut->expiration == 0) {
			// Zero signals that a timer is not running so disallow it.
			timerOut->expiration = 1;
		}
	} else {
		timerOut->expiration = 0;
	}
}

/*****************************************************************
Function:  SetLonRepeatTimer
Returns:   None
Purpose:   Sets a repeating timer value to a given value in
		   milliseconds and repeating at the same interval.
Comments:  Sets the timer to the specified value. The interval
		   can be up to about 24 days.  Set the value to 0 to stop 
		   the current time interval and any repeats.
******************************************************************/
void SetLonRepeatTimer(LonTimer *timerOut, IzotUbits16 initValueIn)
{
	SetLonTimer(timerOut, initValueIn);
	if (initValueIn) {
    	timerOut->repeatTimeout = initValueIn;
 	} else {
		timerOut->repeatTimeout = 0;
	}
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
IzotByte LonTimerExpired(LonTimer *timerInOut)
{
    IzotBits32 delta = (IIzotBits32nt32) (timerInOut->expiration - IzotGetTickCount());
    IzotByte isExpired = timerInOut->expiration && delta <= 0;
    if (isExpired) {
        timerInOut->expiration = 0;
        if (timerInOut->repeatTimeout) {
            IzotUbits16 t = timerInOut->repeatTimeout;
            if ((IzotBits32) (t + delta) < 0) {
                // The repeat timeout is behind the current time.  Set the
                // delta to 0 to miss a tick.
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
IzotByte LonTimerRunning(LonTimer *timerInOut)
{
	return timerInOut->expiration && !LonTimerExpired(timerInOut);
}

/*****************************************************************
Function:  LonTimerRemaining
Returns:   Number of milliseconds left on the timer.
Purpose:   Return the number of milliseconds left on a timer, or
		   return 0 if the timer is not running or expired.
******************************************************************/
IzotUbits16 LonTimerRemaining(LonTimer *timerInOut)
{
	IzotUbits16 remaining = 0;
	if (LonTimerRunning(timerInOut)) {
		remaining = timerInOut->expiration - IzotGetTickCount();
	}
	return remaining;
}

