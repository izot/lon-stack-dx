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
// TBD: merge these into LonNiCommand in lon_types.h, from lon_usb_link.h
#define nicbRESPONSE		0x16
#define nicbINCOMING_L2		0x1A
#define nicbINCOMING_L2M1	0x1B	// Mode 1 frame
#define nicbINCOMING_L2M2   0x1C	// Mode 2 frame
#define nicbLOCALNM			0x22
#define nicbERROR			0x30
#define nicbPHASE			0x40	// Phase mode
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

#endif	// __VLDV_H
