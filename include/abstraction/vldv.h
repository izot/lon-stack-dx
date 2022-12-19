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

/***************************************************************
 *  vldv.h
 *
 *  This file contains the, typedefs, and literals for the
 *  LON(R) Applications Programming Interface Driver LDV level
 *  interface.
 *
 *  Date Created:  August 8th, 1990
 ****************************************************************/

#ifndef __VLDV_H
#define __VLDV_H

#include "EchelonStandardDefinitions.h"

// Local NI commands
#define nicbRESPONSE		0x16
#define nicbINCOMING_L2		0x1A
#define nicbINCOMING_L2M1	0x1B		// Mode 1 frame
#define nicbINCOMING_L2M2   0x1C		// Mode 2 frame
#define nicbLOCALNM			0x22
#define nicbERROR			0x30
#define nicbPHASE			0x40		// Phase mode
#define nicbRESET			0x50	
#define nicbINITIATE 	 	0x51
#define nicbCHALLENGE  		0x52
#define nicbREPLY   		0x53
#define nicbPHASE_SET		0xC0	// IO_0 - IO_3 output low
#define nicbMODE_L5			0xD0
#define nicbMODE_L2			0xD1
#define nicbSSTATUS			0xE0
#define nicbNONLONESC		0xEF

#define SMP_MODE_L5		(nicbMODE_L5&0x0F)
#define SMP_MODE_L2		(nicbMODE_L2&0x0F)

// This is a non-standard form of LDV.  This is because we use
// the LDV API to map to VNI calls.  This provides the flexibility
// of VNI on the simulators.  We also rename the symbols from
// standard LDV to avoid conflicts with software that uses the
// actual LDV (such as the i.lon).

#define LDV_EXTERNAL_FN

typedef void            * pVoid;
typedef short           * pShort;
typedef char            * pStr;

#define LDVCODES \
    LDVCODE(OK,					0)	\
    LDVCODE(NOT_FOUND,			1)	\
    LDVCODE(ALREADY_OPEN,		2)	\
    LDVCODE(NOT_OPEN,			3)	\
    LDVCODE(DEVICE_ERR,			4)	\
    LDVCODE(INVALID_DEVICE_ID,	5)	\
    LDVCODE(NO_MSG_AVAIL,		6)	\
    LDVCODE(NO_BUFF_AVAIL,		7)	\
    LDVCODE(NO_RESOURCES,		8)	\
    LDVCODE(INVALID_BUF_LEN,	9)	\
	LDVCODE(NOT_ENABLED,		10) \
	LDVCODE(INITIALIZATION_FAILED, 11) \
	LDVCODE(OPEN_FAILURE,		   12) 	

#undef LDVCODE
#define LDVCODE(sym,val) LDV_##sym = val,

enum
{
	LDVCODES
};

typedef short LDVCode;

C_API_START
	
LDVCode LDV_EXTERNAL_FN vldv_open(const char* pName, pShort handle);
LDVCode LDV_EXTERNAL_FN vldv_close(short handle);
LDVCode LDV_EXTERNAL_FN vldv_read(short handle, pVoid msg_p, short len);
LDVCode LDV_EXTERNAL_FN vldv_write(short handle, pVoid msg_p, short len);

C_API_END

#endif	// __VLDV_H

