/*
 * lcs_link.c
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
#include "lon_usb/lon_usb_link.h"

// Unique ID fetch interval in milliseconds
#define UNIQUE_ID_FETCH_INTERVAL 500
// LON PL transceiver parameters fetch interval in milliseconds
#define XCVR_PARAM_FETCH_INTERVAL 10000

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
    char *lon_dev_name;
    char *usb_dev_name;
    int iface_index;
    LonUsbInterfaceMode configured_iface_mode;
    LonUsbIfaceModel lon_usb_iface_model;
    bool linkOpened;
	bool isPowerLine;
    bool fetchXcvrParams;
    XcvrParam xcvrParams;
    bool setPlPhase;
} LonNiDef;

// LON network interface definition array
LonNiDef lonNi[NUM_LON_NI] = {
#if !PRODUCT_IS(SLB)
	 {"LON1", "/dev/ttyACM0", -1, LON_IFACE_MODE_LAYER2, U60_FT, false, false, false, {0}, false}
  #if NUM_LON_NI > 1
   , {"LON2", "/dev/ttyACM1", -1, LON_IFACE_MODE_LAYER2, U60_FT, false, false, false, {0}, false}
  #elif NUM_LON_NI > 2
   , {"LON3", "/dev/ttyACM2", -1, LON_IFACE_MODE_LAYER2, U60_FT, false, false, false, {0}, false}
  #elif NUM_LON_NI > 3
   , {"LON4", "/dev/ttyACM3", -1, LON_IFACE_MODE_LAYER2, U60_FT, false, false, false, {0}, false}
  #endif
#else   // PRODUCT_IS(SLB)
      {"RF", "/dev/ttyUSB0", -1, LON_IFACE_MODE_LAYER2, RF_900, false, false, false, {0}, false}
  #if NUM_LON_NI > 1
   , {"PLC", "/dev/ttyUSB1", -1, LON_IFACE_MODE_LAYER2, U70_PL, false, true, true, {0}, true}
  #endif  // NUM_LON_NI > 1	  
#endif  // PRODUCT_IS(SLB) || !PRODUCT_IS(SLB)
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
LonStatusCode LinkLayerReset(void)
{
    IzotUbits16 queueItemSize;
    IzotByte   *p;  // Used to initialize lkInQ
    IzotUbits16 i;
    bool anyPowerLineNi = false; // True if any LON NI is power line
    LonStatusCode status = LonStatusNoError;

    // Allocate and initialize the input queue
    if (!LON_SUCCESS(status = DecodeBufferSize(LK_IN_BUF_SIZE, &gp->lkInBufSize))) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to decode input link buffer size");
        gp->resetOk = FALSE;
        return status;
    }
    gp->lkInBufSize += 6;
    if (!LON_SUCCESS(status = DecodeBufferCnt(LK_IN_Q_CNT, &gp->lkInQCnt))) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to decode input link queue count");
        gp->resetOk = FALSE;
        return status;
    }
    gp->lkInQ = OsalAllocateMemory((size_t)(gp->lkInBufSize * gp->lkInQCnt));
    if (gp->lkInQ == NULL) {
        OsalPrintLog(ERROR_LOG, LonStatusNoMemoryAvailable, "LinkLayerReset: Unable to initialize the input queue");
        gp->resetOk = FALSE;
        return status;
    }
    // Initialize the flag in each item of the queue to 0
    p = gp->lkInQ;
    for (i=0; i < gp->lkInQCnt; i++) {
        *p = 0;
        p = (uint8_t *)((char *)p + gp->lkInBufSize);
    }
    gp->lkInQHeadPtr = gp->lkInQTailPtr = gp->lkInQ;

    // Allocate and initialize the output queue
    if (!LON_SUCCESS(status = DecodeBufferSize(LK_OUT_BUF_SIZE, &gp->lkOutBufSize))) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to decode output link buffer size");
        gp->resetOk = FALSE;
        return status;
    }
    if (!LON_SUCCESS(status = DecodeBufferCnt(LK_OUT_Q_CNT, &gp->lkOutQCnt))) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to decode output link queue count");
        gp->resetOk = FALSE;
        return status;
    }
    queueItemSize    = gp->lkOutBufSize + sizeof(LKSendParam);
    status = QueueInit(&gp->lkOutQ, "link layer output", queueItemSize, gp->lkOutQCnt);
    if (status != LonStatusNoError) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to initialize the output queue");
        gp->resetOk = FALSE;
        return status;
    }

    // Allocate and initialize the priority output queue
    gp->lkOutPriBufSize = gp->lkOutBufSize;
    if (!LON_SUCCESS(status = DecodeBufferCnt(LK_OUT_PRI_Q_CNT, &gp->lkOutPriQCnt))) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to decode priority output link queue count");
        gp->resetOk = FALSE;
        return status;
    }
    queueItemSize = gp->lkOutPriBufSize + sizeof(LKSendParam);
    if (!LON_SUCCESS(status = QueueInit(&gp->lkOutPriQ, "link layer priority output", queueItemSize, gp->lkOutPriQCnt))) {
        OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to initialize the priority output queue");
        gp->resetOk = FALSE;
        return status;
    }
    OsalPrintLog(INFO_LOG, LonStatusNoError, "LinkLayerReset: Link layer queues initialized");

    // Open the LON interfaces and get PL transceiver parameters for any power line interfaces
	for (int niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
		int iface_index;

        if (!LON_SUCCESS(status = OpenLonUsbLink(lonNi[niIndex].lon_dev_name,
                lonNi[niIndex].usb_dev_name,
                &lonNi[niIndex].iface_index,
                lonNi[niIndex].configured_iface_mode,
                lonNi[niIndex].lon_usb_iface_model))) {
            OsalPrintLog(ERROR_LOG, status, "LinkLayerReset: Unable to open LON link %s", lonNi[niIndex].lon_dev_name);
            lonNi[niIndex].linkOpened = false;
            gp->resetOk = FALSE;
            return status;
		}
		lonNi[niIndex].linkOpened = true;
        OsalPrintLog(INFO_LOG, LonStatusNoError, "LinkLayerReset: LON link %s opened", lonNi[niIndex].lon_dev_name);
         // If this is a power line interface, get its Unique ID
         // TBD: review power line initialization and move to later in the startup process
		if (lonNi[niIndex].isPowerLine) {
            anyPowerLineNi = true;
			bool requestUid = true;
			// Get the device Unique ID (Neuron ID or MAC ID) from the LON
            // network interface on every boot.  If this doesn't work, reset
            // and try again
			while (1) {
			    const int messageLength = 5;
				const LonDataFrame nidRead = {LonNiLocalNetMgmtCmd, 14+messageLength,
                        0x70|LNM_TAG, 0x00, messageLength, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        NM_opcode_base|NM_READ_MEMORY, READ_ONLY_RELATIVE, 
                        0x00, 0x00, IZOT_UNIQUE_ID_LENGTH};
				LonDataFrame sicbIn;
				OsalSleep(UNIQUE_ID_FETCH_INTERVAL);
				if (requestUid && WriteLonUsbMsg(lonNi[niIndex].iface_index, (void*)&nidRead) == LonStatusNoError) {
					requestUid = false;
				}
				if (ReadLonUsbMsg(lonNi[niIndex].iface_index, &sicbIn) == LonStatusNoError
                        && sicbIn.ni_command == LonNiResponseCmd 
                        && (sicbIn.pdu[0]&0x0F) == LNM_TAG 
                        && sicbIn.pdu[14] == (NM_resp_success|NM_READ_MEMORY)) {
					memcpy(eep->readOnlyData.UniqueNodeId, &sicbIn.pdu[15], IZOT_UNIQUE_ID_LENGTH);
					break;
				}
			}
            LKFetchXcvrPl(niIndex);
		}
	}
    gp->resetOk = TRUE;
    if (anyPowerLineNi) {
 	    // Start a timer to periodically fetch LON PL transceiver parameters
	    SetLonRepeatTimer(&lonLinkXcvrPlFetchTimer, XCVR_PARAM_FETCH_INTERVAL, XCVR_PARAM_FETCH_INTERVAL);
    }
    return status;
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
void LinkLayerUsbSend(void)
{
    LKSendParam     *lkSendParamPtr;
    Queue           *lkSendQueuePtr;
    uint8_t         *npduPtr;
    LPDUHeader      *lpduHeaderPtr;
    bool             fetchTimerExpired;
    bool             priority;
	LonDataFrame     sicb;
	int				 niIndex;
	static OsalTickCount last_report_time = 0;
	if (OsalGetTickCount() - last_report_time > 1000) {
		last_report_time = OsalGetTickCount();
        if (!QueueEmpty(&gp->lkOutPriQ) || !QueueEmpty(&gp->lkOutQ)) {
            OsalPrintLog(INFO_LOG, LonStatusNoError,
                "LinkLayerUsbSend: %zu output priority queue entries,  %zu output normal queue entries",
                QueueEntries(&gp->lkOutPriQ), QueueEntries(&gp->lkOutQ));
        }
	}

    // Fetch the PL transceiver parameters for each power line LON interface
    fetchTimerExpired = LonTimerExpired(&lonLinkXcvrPlFetchTimer);
    for (niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
        if (lonNi[niIndex].isPowerLine && lonNi[niIndex].fetchXcvrParams && fetchTimerExpired) {
            LKFetchXcvrPl(niIndex);
        }
        if (lonNi[niIndex].isPowerLine && lonNi[niIndex].setPlPhase) {
            LonDataFrame mode = {LonNiPhaseModeCmd|2, 0};
            lonNi[niIndex].setPlPhase = WriteLonUsbMsg(lonNi[niIndex].iface_index, &mode) != LonStatusNoError;
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
    // TODO: this assumes that the NPDU is located in the buffer immediately
    // following the first byte of the LKSendParam structure in the queue item;
    // consider changing to have the first entry in the buffer be the size of
    // the NPDU and then the NPDU itself
	npduPtr = (uint8_t *) (lkSendParamPtr + 1);

	sicb.ni_command = LonNiNetworkMgmtCmd;
	sicb.short_pdu_length = lkSendParamPtr->pduSize+1;
	lpduHeaderPtr = (LPDUHeader *)sicb.pdu;
	lpduHeaderPtr->priority = priority;
	lpduHeaderPtr->altPath = lkSendParamPtr->altPath;
	lpduHeaderPtr->deltaBL = lkSendParamPtr->deltaBL;
	// Copy the NPDU
	if (lkSendParamPtr->pduSize <= sizeof(sicb.pdu)) {
		memcpy(&sicb.pdu[1], npduPtr, lkSendParamPtr->pduSize);
	}
    // Send the LPDU to all open LON interface downlink queues
	for (niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
        // Send pending downlink requests from the link layer output queue to NI downlink queue niIndex
        if (lonNi[niIndex].linkOpened && LonUsbLinkReady(lonNi[niIndex].iface_index)) {
		    WriteLonUsbMsg(niIndex, &sicb);
        }
 	}
    // Remove the LPDU from the link layer output queue
	QueueDropHead(lkSendQueuePtr);
    return;
}

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
void LinkLayerUsbReceive(void)
{
    NWReceiveParam *nwReceiveParamPtr;
    IzotByte       *npduPtr;
    LPDUHeader     *lpduHeaderPtr;
    IzotByte       *tempPtr;
    IzotUbits16     lpduSize;
	LonDataFrame	sicb;
	int				niIndex;
	
	for (niIndex=0; niIndex<NUM_LON_NI; niIndex++) {
		if (ReadLonUsbMsg(niIndex, &sicb) == LonStatusNoError) {
            // Packet found to process
			break;
		}
	}
	if (niIndex == NUM_LON_NI) {
	  	// No packets to process
	  	return;
	}
	if (lonNi[niIndex].isPowerLine && sicb.ni_command == LonNiResponseCmd 
            && (sicb.pdu[0]&0x0F) == LNM_TAG 
            && sicb.pdu[14] == (ND_resp_success|ND_QUERY_XCVR)) {
	  	// This is the response to a PL xcvr register read (done in
        // LKFetchXcvrPl())--save the result
		memcpy(&lonNi[niIndex].xcvrParams.data, &sicb.pdu[15], 
                sizeof(lonNi[niIndex].xcvrParams.data));
		return;
	}
    if (lonNi[niIndex].isPowerLine) {
        lpduSize 	  =	sicb.short_pdu_length-3;	// Subtract 2 for register info and 1 for zero crossing info
        lpduHeaderPtr = (LPDUHeader*)&sicb.pdu[1];	// Offset is 1 because of zero crossing info
    } else {
        lpduSize      =	sicb.short_pdu_length;
        lpduHeaderPtr = (LPDUHeader*)&sicb.pdu[0];
    }
	if (sicb.ni_command == LonNiIncomingL2Mode2Cmd && lpduSize < 8 
            || (sicb.ni_command & 0xF0) == (LonNiError & 0xF0)) {
        // Ignore error packets; for power line interfaces, CRC errors are
        // reported as a packet with a short length
	  	INCR_STATS(LcsTxError);
		return;
    } else if (lonNi[niIndex].isPowerLine) {
        // Process packet received from a power line interface
        if (sicb.ni_command != LonNiIncomingL2Mode2Cmd) {
            if (sicb.ni_command == LonNiResetDeviceCmd
                    || sicb.ni_command == LonNiIncomingL2Cmd
                    || sicb.ni_command == LonNiIncomingL2Mode1Cmd) {
                // Phase setting was lost
                lonNi[niIndex].setPlPhase = true;
            }
            // Ignore packet from a power line interface that is not a layer 2 message
            INCR_STATS(LcsMissed);
	  	    return;
	    }
        // Fill in the packet specific register info for a PL network interface
	    tempPtr = &sicb.pdu[sicb.short_pdu_length-2];
	    lonNi[niIndex].xcvrParams.data[2] = *tempPtr++;
	    lonNi[niIndex].xcvrParams.data[3] = lonNi[niIndex].xcvrParams.data[4] = *tempPtr;
    } else if (sicb.ni_command != LonNiIncomingL2Cmd) {
        // Ignore packet from a non-power line interface that is not a layer 2 message
        INCR_STATS(LcsMissed);
        return;
    }
    // CRC check was performed by the LON interface;
    // increment the valid packet received count
    INCR_STATS(LcsL2Rx);
    if (QueueFull(&gp->nwInQ)) {
        // Network layer input queue is full--lose this packet
        INCR_STATS(LcsMissed);
    } else {
        // Network layer input queue entry available--receive the packet
        nwReceiveParamPtr = QueueTail(&gp->nwInQ);
        npduPtr           = (uint8_t *)(nwReceiveParamPtr + 1);

        nwReceiveParamPtr->priority = lpduHeaderPtr->priority;
        nwReceiveParamPtr->altPath  = lpduHeaderPtr->altPath;
        tempPtr = (uint8_t *)((char *)lpduHeaderPtr + 1);
        nwReceiveParamPtr->pduSize  = lpduSize - 3;
        // TODO: The following line has been commented out because
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
            OsalPrintLog(ERROR_LOG, LonStatusNoMemoryAvailable, "LinkLayerUsbReceive: NPDU size is too large");
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
LonStatusCode LinkLayerReadUsbUid(int niIndex, IzotUniqueId *uidBuf)
{
    if (niIndex < 0 || niIndex >= NUM_LON_NI) {
        return LonStatusInvalidParameter;
    }
    if (!lonNi[niIndex].linkOpened) {
        return LonStatusNotOpen;
    }
    return ReadUsbNiUid(lonNi[niIndex].iface_index, uidBuf);
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
 * Gets a pointer to the transceiver parameters for the specified LON interface.
 * Parameters:
 *   index: The index of the LON interface
 *   p: Pointer to an XcvrParam structure to receive the parameters
 * Returns:
 *   None
 */
void LKGetXcvrParams(int niIndex, XcvrParam *p)
{
  	*p = lonNi[niIndex].xcvrParams;
}

/*
 * Fetches transceiver parameters from the specified LON PL interface.
 * Parameters:
 *   index: The index of the LON interface
 * Returns:
 *   None
 * Notes:
 *   This function is called periodically to fetch the transceiver parameters
 *   from a LON PL interface.  It is also called once during initialization
 *   of a LON PL interface to kick off the process.  The function just returns
 *   if there is no LON PL interface.
 */
void LKFetchXcvrPl(int index)
{
	const int msgLen = 1;
	const LonDataFrame sicbOut = {LonNiLocalNetMgmtCmd, 14+msgLen, 0x70|LNM_TAG, 0x00, msgLen, 
							 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							 ND_opcode_base|ND_QUERY_XCVR};
    if (lonNi[index].isPowerLine) {
	    // Send the fetch message; if send fails, set the fetch flag to try again next time
	    lonNi[index].fetchXcvrParams = WriteLonUsbMsg(lonNi[index].iface_index,
                (LonDataFrame*)&sicbOut) != LonStatusNoError;
    }
}

#endif  // !LINK_IS(ETHERNET) && !LINK_IS(WIFI)
