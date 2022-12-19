//
// err.h
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

/*
 * Title: Error handling header file
 *
 * Abstract:
 * This file defines APIs for the error logging subsystem
 */

#ifndef ERR_H
#define ERR_H


/*
 * Enumeration : ErrSystem
 *
 */
typedef enum
{
	ERR_PLC_XCVR_TIMEOUT = 1,		// PLC interface retries exceeded, reset PLC
	ERR_COMMS_RESET_TIMEOUT = 2,	// No comms at all, reset MCU.
	ERR_SYS_UPGRD = 0x40,			// Firmware upgrade errors
} ErrSystem;

void ERR_LogEchErr(EchErr err);
EchErr ERR_GetLastEchErr(void);

void ERR_LogSystemErr(ErrSystem err);

#endif


