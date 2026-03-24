/*
 * lcs_link.h
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Data Link Layer for LON USB and MIP Data Links
 * Purpose: Implements the LON data link layer (Layer 2) of the 
 *          ISO/IEC 14908-1 LON protocol stack.
 * Notes:   The functions in this file support LON data links using a
 *          LON USB network interface such as the U10 or U60.
 */

#ifndef _LINK_H
#define _LINK_H

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"

typedef short LonLinkHandle;

// LON NI frame structure used for non-extended and extended frames as
// stored in the link layer queues; use the LonNiExtendedFrame structure
// to access the extended length field

typedef struct LonDataFrame{
	uint8_t ni_command;			// Network interface command; use LonNiCommand values
	uint8_t short_pdu_length; 	// PDU length if < EXT_LENGTH; else EXT_LENGTH
	uint8_t pdu[MAX_LON_MSG_EX_LEN];
} LonDataFrame;

// LON NI frame structure used for extended frames as stored in the
// link layer queues; the first two bytes match the LonNiFrame structure; the
// ext_length_be field is in network byte order (big-endian) format
typedef struct LonExtDataFrame {
	uint8_t ni_command;			// Network interface command; use LonNiCommand values
	uint8_t short_pdu_length; 	// PDU length if < EXT_LENGTH; else EXT_LENGTH
	uint16_t ext_length_be; 	// Big-endian PDU length in bytes
	uint8_t ext_pdu[MAX_LON_MSG_EX_LEN - 2];
} LonExtDataFrame;

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
LonStatusCode LinkLayerReset(void);
void LinkLayerUsbSend(void);
void LinkLayerUsbReceive(void);
#endif
