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
/*
 * Initializes the LON Stack link layer including queues used by the link layer.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if successful, LonStatusCode error code otherwise
 * Notes:
 *   Sets gp->resetOk to false if unable to reset properly.
 *   For a MIP LON link, the input queue is also used by the physical
 *   layer and is not a regular queue.  Each item in the queue has the
 *   following form:
 *     <flag> <LPDUSize> <LPDU>
 *   where
 *     <flag> is 1 byte
 *     <LPDUSize> is 2 bytes
 *     <LPDU> is of the form LPDU_HEADER RESTOFLPDU CRC
 *     LPDU_HEADER is 1 byte (LPDU does not include the syncbits)
 *     CRC uses 2 bytes.
 *   Total # bytes in addition to NPDU is thus 6 bytes.
 */
LonStatusCode LinkLayerReset(void);

/*
 * Sends an LPDU from the link layer output queue to the LON network interface.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The LON interface is typically a U10 or U60 LON USB interface.
 */
void LinkLayerUsbSend(void);

/*
 * Receives an LPDU from a LON interface, extracts the NPDU, and transfers
 * the NPDU to the network layer.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The LON interface is typically a U10 or U60 LON USB interface.
 *   Each item of the queue gp->lkInQ has the following form:
 *     flag pduSize LPDU
 *       flag is 1 byte long
 *       pduSize is 2 bytes long
 *       LPDU has header followed by the rest of the LPDU and then CRC
 *         LPDU header is 1 byte long
 *         CRC is 2 bytes
 *   If a packet is in lkInQ then it will fit into nwInQ.
 */
void LinkLayerUsbReceive(void);

/*
 * Reads the Unique ID (Neuron ID or MAC ID) from a LON USB network interface.
 * Parameters:
 *   niIndex: The index of the LON network interface to read the Unique ID from
 *   uidBuf: Buffer to receive the Unique ID
 * Returns:
 *   LonStatusNoError if successful, LonStatusCode error code otherwise
 * Notes:
 *   This function is called by IzotGetUniqueId() to get the Unique ID from a LON
 *   USB network interface.
 */
LonStatusCode LinkLayerReadUsbUid(int niIndex, IzotUniqueId *uidBuf);
#endif
