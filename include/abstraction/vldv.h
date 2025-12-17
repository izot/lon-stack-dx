/*
 * vldv.h
 *
 * Copyright (c) 1990-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Driver API Definitions
 * Purpose: Specifies the LON Driver API (LDV) level interface including
 *          typedefs and literals for the API.
 * Notes:   Created August 8th, 1990.
 */

#ifndef __VLDV_H
#define __VLDV_H

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
// the LDV API to map to LON link calls.

#define LDV_EXTERNAL_FN

#ifdef  __cplusplus
extern "C"
{
#endif  // __cplusplus
	
LonStatusCode LDV_EXTERNAL_FN OpenLonLink(const char *name, short *handle);
LonStatusCode LDV_EXTERNAL_FN CloseLonLink(short handle);
LonStatusCode LDV_EXTERNAL_FN ReadLonLink(short handle, void *msg, short len);
LonStatusCode LDV_EXTERNAL_FN WriteLonLink(short handle, void *msg, short len);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif	// __VLDV_H
