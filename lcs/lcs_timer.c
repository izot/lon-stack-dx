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

/*******************************************************************************
          File:  lsc_timer.c

       Purpose:  Timer APIs
*******************************************************************************/

#include "lcs_eia709_1.h"
#include "lcs_timer.h"
#include "lcs_node.h"


/*****************************************************************
Function:  GetCurrentMsTime
Returns:   Current Time (not necessarily real time)
Reference:
Purpose:   To get the current time.
Comments:  None
******************************************************************/
uint32 GetCurrentMsTime(void)
{
    return TMR_GetCurrentTime();
}

/*****************************************************************
Function:  MsTimerSet
Returns:   None
Reference: None
Purpose:   To set a timer value to a given value in ms.
Comments:  The timer is set to given value. The lastUpdateTime is
           set to current time. UpdateTimer is called later to
           update the timer value.  Setting a timer to 0 means it
		   is stopped (no expiration will occur).
******************************************************************/
void MsTimerSet(MsTimer *timerOut, uint16 initValueIn)
{
	if (initValueIn)
	{
		TMR_Start(timerOut, initValueIn);
	}
	else
	{
		TMR_Stop(timerOut);
	}
}


/*****************************************************************
Function:  MsTimerExpired
Returns:   TRUE if the timer has expired. FALSE if the timer has
            not expired or it has expired but has already been
            reported as expired.
Reference: None
Purpose:   To update given timer and test if it expired.
Comments:  None
******************************************************************/
Boolean MsTimerExpired(MsTimer *timerInOut)
{
	return TMR_Expired(timerInOut);
}


/*****************************************************************
Function:  MsTimerRunning
Purpose:   To see if a timer is still running.  Different from MsTimerExpired()
           in that it will return true once timer expires whereas MsTimerExpired() only
		   returns true once.
******************************************************************/
Boolean MsTimerRunning(MsTimer *timerInOut)
{
	return TMR_Running(timerInOut);
}

