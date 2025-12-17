/*
 * lcs_link.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Data Link Layer for LON USB and MIP Data Links
 * Purpose: Implements the LON data link layer (Layer 2) of the 
 *          ISO/IEC 14908-1 LON protocol stack.
 * Notes:   The functions in this file support LON data links using a
 *          LON USB network interface such as the U10 or U60.
 */

#include "lcs/lcs_link.h"

#if !LINK_IS(ETHERNET) && !LINK_IS(WIFI)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "lcs/lcs_queue.h"
#include "lcs/lcs_netmgmt.h"

// Unique ID fetch interval in milliseconds
#define UNIQUE_ID_FETCH_INTERVAL 500
// LON PL transceiver parameters fetch interval in milliseconds
#define XCVR_PARAM_FETCH_INTERVAL 10000

typedef struct {
	IzotByte cmd;
	IzotByte len;
	IzotByte pdu[MAX_PDU_SIZE];
} L2Frame;

// 1-byte header definition of an LPDU in the queue
typedef struct {
	BITS3(priority,		1,
		  altPath,		1,
		  deltaBL,		6)
} LPDUHeader;

// Number of LON network interfaces to be supported
#define NUM_LON_NI 1

static LonTimer lonLinkXcvrPlFetchTimer;

// LON network interface definition structure
typedef struct {
    char *name;
    LonLinkHandle handle;
    bool linkOpened;
	bool isPowerLine;
    bool fetchXcvrParams;
    XcvrParam xcvrParams;
    bool setPlPhase;
} LonNiDef;

// LON network interface definition array
LonNiDef lonNi[NUM_LON_NI] = {
#if !PRODUCT_IS(SLB)
	{"LON1", -1, false, false, false, {0}, false}
#elif NUM_LON_NI > 1
   , {"LON2", -1, false, false, false, {0}, false}
#elif NUM_LON_NI > 2
   , {"LON3", -1, false, false, false, {0}, false}
#elif NUM_LON_NI > 3
   , {"LON4", -1, false, false, false, {0}, false}
#else   // PRODUCT_IS(SLB)
    {"RF", -1, false, false, false, {0}, false},
#if NUM_LON_NI == 2
	{"PLC", -1, false, true, true, {0}, true}
#endif  // NUM_LON_NI == 2	  
#endif  // PRODUCT_IS(SLB)
};

#define LNM_TAG 0x0F	// Tag reserved for local network management

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
void LKFetchXcvrPl(int niIndex); // Fetch PL transceiver params for a LON NI
void LKGetXcvrParams(int niIndex, XcvrParam *p);

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/

/*
 * Allocates space for link layer queues.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   Sets gp->resetOk to FALSE if unable to reset properly.
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
void LKReset(void)
{
    IzotUbits16 queueItemSize;
    IzotByte   *p;  // Used to initialize lkInQ
    IzotUbits16 i;
    bool anyPowerLineNi = false; // True if any LON NI is power line
    LonStatusCode lonNiSts = LonStatusNoError;

    // Allocate and initialize the input queue
    gp->lkInBufSize = DecodeBufferSize((IzotUbits16)gp->nwInBufSize) + 6;
    gp->lkInQCnt = DecodeBufferCnt((IzotUbits16)gp->nwInQCnt);
    gp->lkInQ = OsalAllocateMemory((size_t)(gp->lkInBufSize * gp->lkInQCnt));
    if (gp->lkInQ == NULL) {
        OsalPrintError(LonStatusNoMemoryAvailable, "LKReset: Unable to initialize the input queue");
        gp->resetOk = FALSE;
        return;
    }
    // Initialize the flag in each item of the queue to 0
    p = gp->lkInQ;
    for (i=0; i < gp->lkInQCnt; i++) {
        *p = 0;
        p = (uint8_t *)((char *)p + gp->lkInBufSize);
    }
    gp->lkInQHeadPtr = gp->lkInQTailPtr = gp->lkInQ;

    // Allocate and initialize the output queue
    gp->lkOutBufSize = DecodeBufferSize((IzotUbits16)gp->nwOutBufSize);
    gp->lkOutQCnt    = DecodeBufferCnt((IzotUbits16)gp->nwOutQCnt);
    queueItemSize    = gp->lkOutBufSize + sizeof(LKSendParam);

    lonNiSts = QueueInit(&gp->lkOutQ, queueItemSize, gp->lkOutQCnt);
    if (lonNiSts != LonStatusNoError) {
        OsalPrintError(lonNiSts, "LKReset: Unable to initialize the output queue");
        gp->resetOk = FALSE;
        return;
    }

    // Allocate and initialize the priority output queue
    gp->lkOutPriBufSize = gp->lkOutBufSize;
    gp->lkOutPriQCnt = DecodeBufferCnt((IzotUbits16)gp->nwOutPriQCnt);
    queueItemSize = gp->lkOutPriBufSize + sizeof(LKSendParam);

    lonNiSts = QueueInit(&gp->lkOutPriQ, queueItemSize, gp->lkOutPriQCnt);
    if (lonNiSts != LonStatusNoError) {
        OsalPrintError(lonNiSts, "LKReset: Unable to initialize the priority output queue");
        gp->resetOk = FALSE;
        return;
    }

	for (int niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
		LonLinkHandle handle;

		lonNiSts = OpenLonLink(lonNi[niIndex].name, &handle);

        if (lonNiSts != LonStatusNoError) {
            OsalPrintDebug(lonNiSts, "LKReset: Unable to open LON link %s",
                    lonNi[niIndex].name);
            lonNi[niIndex].linkOpened = false;
            continue;
		}
		lonNi[niIndex].linkOpened = true;

		if (lonNi[niIndex].isPowerLine) {
            anyPowerLineNi = true;
			bool requestUid = true;
	
			// Get the device Unique ID (Neuron ID or MAC ID) from the LON
            // network interface on every boot.  If this doesn't work, reset
            // and try again
			while (1) {
			    const int messageLength = 5;
				const L2Frame nidRead = {nicbLOCALNM, 14+messageLength,
                        0x70|LNM_TAG, 0x00, messageLength, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        NM_opcode_base|NM_READ_MEMORY, READ_ONLY_RELATIVE, 
                        0x00, 0x00, UNIQUE_NODE_ID_LEN};
				L2Frame sicbIn;
				OsalSleep(UNIQUE_ID_FETCH_INTERVAL);
				if (requestUid && WriteLonLink(handle, (void*)&nidRead, (short)(nidRead.len+2)) == LonStatusNoError) {
					requestUid = false;
				}
				if (ReadLonLink(handle, &sicbIn, sizeof(sicbIn)) == LonStatusNoError
                        && sicbIn.cmd == nicbRESPONSE 
                        && (sicbIn.pdu[0]&0x0F) == LNM_TAG 
                        && sicbIn.pdu[14] == (NM_resp_success|NM_READ_MEMORY)) {
					memcpy(eep->readOnlyData.UniqueNodeId, &sicbIn.pdu[15], UNIQUE_NODE_ID_LEN);
					break;
				}
			}
            LKFetchXcvrPl(niIndex);
		}
  	    lonNi[niIndex].handle = handle;
	}

    gp->resetOk = TRUE;

    if (anyPowerLineNi) {
 	    // Start a timer to periodically fetch LON PL transceiver parameters
	    SetLonRepeatTimer(&lonLinkXcvrPlFetchTimer, XCVR_PARAM_FETCH_INTERVAL, XCVR_PARAM_FETCH_INTERVAL);
    }
	
    return;
}

/*
 * Receives an NPDU from the network layer, adds link layer contents to the
 * NPDU to create an LPDU, and send the LPDU to the LON network interface.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The LON network interface is typically a U10 or U60 LON USB interface.
 *   Extra bytes are allocated in the buffers to accomodate layer-specific
 *   additions to the buffer contents.
 */
void LKSend(void)
{
    LKSendParam     *lkSendParamPtr;
    Queue           *lkSendQueuePtr;
    uint8_t            *npduPtr;
    LPDUHeader      *lpduHeaderPtr;
    bool             fetchTimerExpired;
    bool             priority;
	L2Frame		     sicb;
	int				 niIndex;

    fetchTimerExpired = LonTimerExpired(&lonLinkXcvrPlFetchTimer);
    for (niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
        if (lonNi[niIndex].isPowerLine && lonNi[niIndex].fetchXcvrParams && fetchTimerExpired) {
            LKFetchXcvrPl(niIndex);
        }
        if (lonNi[niIndex].isPowerLine && lonNi[niIndex].setPlPhase) {
            L2Frame mode = {nicbPHASE|2, 0};
            lonNi[niIndex].setPlPhase = WriteLonLink(lonNi[niIndex].handle, &mode, 2) != LonStatusNoError;
        }
    }

    // Make variables point to the new queue
    if (!QueueEmpty(&gp->lkOutPriQ)) {
        priority       = TRUE;
        lkSendQueuePtr = &gp->lkOutPriQ;
    } else if (!QueueEmpty(&gp->lkOutQ)) {
        priority       = FALSE;
        lkSendQueuePtr = &gp->lkOutQ;
    } else {
        return; // Nothing to send
    }

	lkSendParamPtr = QueuePeek(lkSendQueuePtr);
	npduPtr        = (uint8_t *) (lkSendParamPtr + 1);

	sicb.cmd = 0x12;
	sicb.len = lkSendParamPtr->pduSize+1;

	lpduHeaderPtr = (LPDUHeader *)sicb.pdu;
	lpduHeaderPtr->priority = priority;
	lpduHeaderPtr->altPath  = lkSendParamPtr->altPath;
	lpduHeaderPtr->deltaBL  = lkSendParamPtr->deltaBL;

	// Copy the NPDU
	if (lkSendParamPtr->pduSize <= sizeof(sicb.pdu)) {
		memcpy(&sicb.pdu[1], npduPtr, lkSendParamPtr->pduSize);
	}
	
    // Send the LPDU to all LON network interfaces
	for (niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
		WriteLonLink(lonNi[niIndex].handle, &sicb, (short)(sicb.len+2));
	}

	QueueDropHead(lkSendQueuePtr);

    return;
}

/*
 * Receives an LPDU from a LON network interface, extracts the NPDU, and transfers
 * the NPDU to the network layer.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The LON network interface is typically a U10 or U60 LON USB interface.
 *   Each item of the queue gp->lkInQ has the following form:
 *     flag pduSize LPDU
 *       flag is 1 byte long
 *       pduSize is 2 bytes long
 *       LPDU has header followed by the rest of the LPDU and then CRC
 *         LPDU header is 1 byte long
 *         CRC is 2 bytes
 *   If a packet is in lkInQ then it will fit into nwInQ.
 */
void LKReceive(void)
{
    NWReceiveParam *nwReceiveParamPtr;
    IzotByte       *npduPtr;
    LPDUHeader     *lpduHeaderPtr;
    IzotByte       *tempPtr;
    IzotUbits16     lpduSize;
	L2Frame			sicb;
	int				niIndex;
	
	for (niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
		if (ReadLonLink(lonNi[niIndex].handle, &sicb, sizeof(sicb)) == LonStatusNoError) {
            // Packet found to process
			break;
		}
	}
	
	if (niIndex == NUM_LON_NI) {
	  	// No packets to process
	  	return;
	}

	if (lonNi[niIndex].isPowerLine && sicb.cmd == nicbRESPONSE 
            && (sicb.pdu[0]&0x0F) == LNM_TAG 
            && sicb.pdu[14] == (ND_resp_success|ND_QUERY_XCVR)) {
	  	// This is the response to a PL xcvr register read (done in
        // LKFetchXcvrPl())--save the result
		memcpy(&lonNi[niIndex].xcvrParams.data, &sicb.pdu[15], 
                sizeof(lonNi[niIndex].xcvrParams.data));
		return;
	}

    lpduSize 	  =	sicb.len-3;	                // Subtract 2 for register info and 1 for zero crossing info
    lpduHeaderPtr = (LPDUHeader*)&sicb.pdu[1];	// Offset is 1 because of zero crossing info
	
	// Throw away layer 2 mode 2 packets that are smaller than 8 bytes long;
    // layer 2 mode 2 network interfaces report CRC errors as a packet with
    // a short length
	if (sicb.cmd == nicbINCOMING_L2M2 && lpduSize < 8 ||
		(sicb.cmd&0xF0) == (nicbERROR&0xF0)) {
	  	INCR_STATS(LcsTxError);
		return;
	} else if (sicb.cmd != nicbINCOMING_L2M2) {
	    if (lonNi[niIndex].isPowerLine && (sicb.cmd == nicbRESET
                || sicb.cmd == nicbINCOMING_L2 || sicb.cmd == nicbINCOMING_L2M1)) {
		  	// Phase setting was lost
			lonNi[niIndex].setPlPhase = true;
		}
	  	return;
	}

    if (lonNi[niIndex].isPowerLine) {
        // Fill in the packet specific register info for a PL network interface
	    tempPtr = &sicb.pdu[sicb.len-2];
	    lonNi[niIndex].xcvrParams.data[2] = *tempPtr++;
	    lonNi[niIndex].xcvrParams.data[3] = lonNi[niIndex].xcvrParams.data[4] = *tempPtr;
    }
		
    // CRC check was performed by the LON network interface;
    // increment the valid packet received count
    INCR_STATS(LcsL2Rx);

    // Check if the packet is for us
    if (sicb.cmd != nicbINCOMING_L2M2 || sicb.pdu[0] != nicbLOCALNM) {
        INCR_STATS(LcsMissed);
        return;
    }

    // Check if the packet is too small
    if (lpduSize < 8) {
        INCR_STATS(LcsRxError);
        return;
    }
    
    if (QueueFull(&gp->nwInQ)) {
        // Queue is full--lose this packet
        INCR_STATS(LcsMissed);
    } else {
        // Queue entry available--receive the packet
        nwReceiveParamPtr = QueueTail(&gp->nwInQ);
        npduPtr           = (uint8_t *)(nwReceiveParamPtr + 1);

        nwReceiveParamPtr->priority = lpduHeaderPtr->priority;
        nwReceiveParamPtr->altPath  = lpduHeaderPtr->altPath;
        tempPtr = (uint8_t *)((char *)lpduHeaderPtr + 1);
        nwReceiveParamPtr->pduSize  = lpduSize - 3;
        // The following line has been commented out because
        // there is no xcvrParams field in NWReceiveParam and
        // transceiver parameters are only fetched for a PL
        // network interface
 		// nwReceiveParamPtr->xcvrParams = lonLinkXcvrPlParam;
 
        // Copy the NPDU; if it was in link layer's queue, then the size
        // should be sufficient in the network layer's queue as they differ
        // by 3; play safe by checking the size first
        if (nwReceiveParamPtr->pduSize <= gp->nwInBufSize) {
            memcpy(npduPtr, tempPtr, nwReceiveParamPtr->pduSize);
        } else {
            OsalPrintError(LonStatusNoMemoryAvailable, "LKReceive: NPDU size is too large");
        }
        QueueWrite(&gp->nwInQ);
    }
    *(gp->lkInQHeadPtr) = 0;
    gp->lkInQHeadPtr = gp->lkInQHeadPtr + gp->lkInBufSize;
    if (gp->lkInQHeadPtr ==
            (gp->lkInQ + gp->lkInBufSize * gp->lkInQCnt)) {
        gp->lkInQHeadPtr = gp->lkInQ; // Wrap around
    }
	
    return;
}

/*
 * Computes 16-bit CRC.
 * Parameters:
 *   bufInOut: Input buffer containing data to be checksummed; the checksum
 *     will be appended to the end of this buffer
 *   sizeIn: Number of bytes in the input buffer to checksum
 * Returns:
 *   None
 */
void CRC16(uint8_t bufInOut[], IzotUbits16 sizeIn)
{
    IzotUbits16 poly = 0x1021;       // Generator polynomial
    IzotUbits16 crc = 0xffff;
    IzotUbits16 i,j;
    unsigned char byte, crcbit, databit;

    for (i = 0; i < sizeIn; i++) {
        byte = bufInOut[i];
        for (j = 0; j < 8; j++) {
            crcbit = crc & 0x8000 ? 1 : 0;
            databit = byte & 0x80 ? 1 : 0;
            crc = crc << 1;
            if (crcbit != databit) {
                crc = crc ^ poly;
            }
            byte = byte << 1;
        }
    }
    crc = crc ^ 0xffff;
    bufInOut[sizeIn]     = (crc >> 8);
    bufInOut[sizeIn + 1] = (crc & 0x00FF);
    return;
}

/*
 * Gets a pointer to the transceiver parameters for the specified LON network interface.
 * Parameters:
 *   index: The index of the LON network interface
 *   p: Pointer to an XcvrParam structure to receive the parameters
 * Returns:
 *   None
 */
void LKGetXcvrParams(int niIndex, XcvrParam *p)
{
  	*p = lonNi[niIndex].xcvrParams;
}

/*
 * Fetches transceiver parameters from the specified LON PL network interface.
 * Parameters:
 *   index: The index of the LON network interface
 * Returns:
 *   None
 * Notes:
 *  This function is called periodically to fetch the transceiver parameters
 *  from a LON PL network interface.  It is also called once during
 *  initialization of a LON PL network interface to kick off the process.
 *  The function just returns if there is no LON PL network interface.
 */
void LKFetchXcvrPl(int index)
{
	const int msgLen = 1;
	const L2Frame sicbOut = {nicbLOCALNM, 14+msgLen, 0x70|LNM_TAG, 0x00, msgLen, 
							 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							 ND_opcode_base|ND_QUERY_XCVR};
    if (lonNi[index].isPowerLine) {
	    // Send the fetch message; if send fails, set the fetch flag to try again next time
	    lonNi[index].fetchXcvrParams = WriteLonLink(lonNi[index].handle,
                (L2Frame*)&sicbOut, (short)(sicbOut.len+2)) != LonStatusNoError;
    }
}

#endif  // !LINK_IS(ETHERNET) && !LINK_IS(WIFI)
