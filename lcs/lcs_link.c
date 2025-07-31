//
// lcs_link.c
//
// Copyright (C) 2023-2025 EnOcean
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
     Reference:        ISO/IEC 14908-1 Data Link Layer

       Purpose:        Data structures and functions for the LON Data Link layer
                       implemented on a Neuron processor with MIP firmware.
*******************************************************************************/

#include "lcs_link.h"

#if LINK_IS(MIP) || LINK_IS(USB)

/*------------------------------------------------------------------------------
  Section: Includes
  ------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcs_eia709_1.h"
#include "lcs_timer.h"
#include "lcs_node.h"
#include "lcs_queue.h"
#include "lcs_netmgmt.h"
#include "link.h"
/*------------------------------------------------------------------------------
  Section: Constant Definitions
  ------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
  Section: Type Definitions
  ------------------------------------------------------------------------------*/
typedef struct
{
	Byte cmd;
	Byte len;
	Byte pdu[MAX_PDU_SIZE];
} L2Frame;

/* Need a structure that represents the 1 byte header portion for
   LPDU in the queue */
typedef struct
{
	BITS3(priority,		1,
		  altPath,		1,
		  deltaBL,		6)
} LPDUHeader;

#define NUM_VNI 2
static LinkHandle 	vniHandle[NUM_VNI];
static XcvrParam 	vniXcvrParam[NUM_VNI];
static LonTimer 	xcvrTimer;
static int 			plcVni;
static Bool		 	xcvrFetch = false;
static Bool			setPhase = true;

typedef struct
{
    char *szName;
	Bool isPlc;
} VniDef;

const VniDef vni[NUM_VNI] = 
{
#if PRODUCT_IS(SLB)
    {"RF", false},
#if NUM_VNI == 2
	{"PLC", true},	// Note that vldv.c assumes that names starting with 'P' are PLC channels!
#endif	  
#else
	{"LON2", true}
#endif
};

#define LNM_TAG 0x0F	// Tag reserved for local NM

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/
void LKFetchXcvr(void);
void LKGetTransceiverParams(int index, XcvrParam *p);

/*------------------------------------------------------------------------------
Section: Function Definitions
------------------------------------------------------------------------------*/

/*******************************************************************************
Function:  LKReset
Returns:   None
Reference: None
Purpose:   To allocate space for link layer queues.

Comments:  Sets gp->resetOk to FALSE if unable to reset properly.
*******************************************************************************/
void LKReset(void)
{
    uint16 queueItemSize;
    Byte   *p; /* Used to initialize lkInQ. */
    uint16 i;

    /****************************************************************************
       Allocate and initialize the input queue.
       Since input queue is also used by the physical layer,
       it is not a regular queue. Each item in the queue has the
       form
          <flag> <LPDUSize> <LPDU>
       where
          <flag> is 1 byte
          <LPDUSize> is 2 bytes
          <LPDU> is of the form LPDU_HEADER RESTOFLPDU CRC
          LPDU_HEADER is 1 byte (LPDU does not include the syncbits)
          CRC uses 2 bytes.
       Total # bytes in addition to NPDU is thus 6 bytes.
    ****************************************************************************/
    gp->lkInBufSize   =
        DecodeBufferSize((uint8)eep->readOnlyData.nwInBufSize) + 6;
    gp->lkInQCnt      =
        DecodeBufferCnt((uint8)eep->readOnlyData.nwInBufCnt);

    gp->lkInQ = AllocateStorage((uint16)(gp->lkInBufSize * gp->lkInQCnt));
    if (gp->lkInQ == NULL)
    {
        ErrorMsg("LKReset: Unable to init the input queue.\n");
        gp->resetOk = FALSE;
        return;
    }
    /* Init the flag in each item of the queue to 0. */
    p = gp->lkInQ;
    for (i = 0; i < gp->lkInQCnt; i++)
    {
        *p = 0;
        p  = (Byte *)((char *)p + gp->lkInBufSize);
    }

    gp->lkInQHeadPtr = gp->lkInQTailPtr = gp->lkInQ;

    /* Allocate and initialize the output queue. */
    gp->lkOutBufSize  =
        DecodeBufferSize((uint8)eep->readOnlyData.nwOutBufSize);
    gp->lkOutQCnt     =
        DecodeBufferCnt((uint8)eep->readOnlyData.nwOutBufCnt);
    queueItemSize    = gp->lkOutBufSize + sizeof(LKSendParam);

    if (QueueInit(&gp->lkOutQ, queueItemSize, gp->lkOutQCnt)
            != SUCCESS)
    {
        ErrorMsg("LKReset: Unable to init the output queue.\n");
        gp->resetOk = FALSE;
        return;
    }

    /* Allocate and initialize the priority output queue. */
    gp->lkOutPriBufSize = gp->lkOutBufSize;
    gp->lkOutPriQCnt  =
        DecodeBufferCnt((uint8)eep->readOnlyData.nwOutBufPriCnt);
    queueItemSize    = gp->lkOutPriBufSize + sizeof(LKSendParam);

    if (QueueInit(&gp->lkOutPriQ, queueItemSize, gp->lkOutPriQCnt)
            != SUCCESS)
    {
        ErrorMsg("LKReset: Unable to init the priority output queue.\n");
        gp->resetOk = FALSE;
        return;
    }

	for (i=0; i<NUM_VNI; i++)
	{
		LinkHandle handle;
		
		vldv_open(vni[i].szName, &handle);
	
		if (vni[i].isPlc)
		{
			Bool requestNid = true;
	
			plcVni = i;
			// Get the Neuron ID from the MIP.  We do this on every boot.  If this doesn't work, we'll just reset and try again.
			while (1)
			{
			    const int MSGLEN = 5;
				const L2Frame nidRead = {nicbLOCALNM, 14+MSGLEN, 0x70|LNM_TAG, 0x00, MSGLEN, 
										 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
								         NM_opcode_base|NM_READ_MEMORY, READ_ONLY_RELATIVE, 0x00, 0x00, UNIQUE_NODE_ID_LEN};
				L2Frame sicbIn;
				TAKE_A_BREAK;
				if (requestNid && vldv_write(handle, (void*)&nidRead, (short)(nidRead.len+2)) == LDV_OK)
				{
					requestNid = false;
				}
				if (vldv_read(handle, &sicbIn, sizeof(sicbIn)) == LDV_OK && sicbIn.cmd == nicbRESPONSE && (sicbIn.pdu[0]&0x0F) == LNM_TAG && sicbIn.pdu[14] == (NM_resp_success|NM_READ_MEMORY))
				{
					memcpy(eep->readOnlyData.uniqueNodeId, &sicbIn.pdu[15], UNIQUE_NODE_ID_LEN);
					break;
				}
			}
		}
  	    vniHandle[i] = handle;
	}

	// Start a timer to periodically fetch xcvr params plus kick off a fetch to get things initialized.
	StartLonRepeatTimer(&xcvrTimer, 10000);
	LKFetchXcvr();
	
    return;
}

/*******************************************************************************
Function:  LKSend
Returns:   None
Reference: None
Purpose:   To take the NPDU from link layer's output queue and put it
           in the queue for the physical layer.
Comments:  We assume that there will be sufficient space as we
           allocated the extra bytes based on header size etc.
*******************************************************************************/
void LKSend(void)
{
    LKSendParam     *lkSendParamPtr;
    Queue           *lkSendQueuePtr;
    Byte            *npduPtr;
    LPDUHeader      *lpduHeaderPtr;
    Boolean          priority;
	L2Frame		     sicb;
	int				 i;

	if (LonTimerExpired(&xcvrTimer) || xcvrFetch)
	{
	  	LKFetchXcvr();
	}
	
	if (setPhase)
	{
	    L2Frame mode = {nicbPHASE|2, 0};
	    if (vldv_write(vniHandle[plcVni], &mode, 2) == LDV_OK)
		{
		    setPhase = false;
		}
	}
	
    /* First, make variables point to the right queue. */
    if (!QueueEmpty(&gp->lkOutPriQ))
    {
        priority        = TRUE;
        lkSendQueuePtr  = &gp->lkOutPriQ;
    }
    else if (!QueueEmpty(&gp->lkOutQ))
    {
        priority        = FALSE;
        lkSendQueuePtr  = &gp->lkOutQ;
    }
    else
    {
        return; /* Nothing to send. */
    }

	lkSendParamPtr = QueueHead(lkSendQueuePtr);
	npduPtr        = (Byte *) (lkSendParamPtr + 1);

	sicb.cmd = 0x12;
	sicb.len = lkSendParamPtr->pduSize+1;

	lpduHeaderPtr = (LPDUHeader *)sicb.pdu;
	lpduHeaderPtr->priority = priority;
	lpduHeaderPtr->altPath  = lkSendParamPtr->altPath;
	lpduHeaderPtr->deltaBL  = lkSendParamPtr->deltaBL;

	/* Copy the NPDU. */
	if (lkSendParamPtr->pduSize <= sizeof(sicb.pdu))
	{
		memcpy(&sicb.pdu[1], npduPtr, lkSendParamPtr->pduSize);
	}
			 
	for (i=0; i<NUM_VNI; i++)
	{
		vldv_write(vniHandle[i], &sicb, (short)(sicb.len+2));
	}

	DeQueue(lkSendQueuePtr);

    return;
}

/*******************************************************************************
Function:  LKReceive
Returns:   None
Reference: None
Purpose:   To receive the incoming LPDUs and process them.
Comments:  Each item of the queue gp->lkInQ has the following form:
           flag pduSize LPDU
           flag is 1 byte long.
           pduSize is 2 bytes long.
           LPDU has header followed by the rest of the LPDU and then CRC.
           The LPDU header is 1 byte long. CRC uses 2 bytes.
           If a packet is in lkInQ then it should fit into nwInQ.
*******************************************************************************/
void LKReceive(void)
{
    NWReceiveParam *nwReceiveParamPtr;
    Byte           *npduPtr;
    LPDUHeader     *lpduHeaderPtr;
    Byte           *tempPtr;
    uint16          lpduSize;
	L2Frame			sicb;
	int				i;
	XcvrParam		xcvrParams;
	
	for (i=0; i<NUM_VNI; i++)
	{
		if (vldv_read(vniHandle[i], &sicb, sizeof(sicb)) == LDV_OK)
		{
		  	LKGetTransceiverParams(i, &xcvrParams);
			break;
		}
	}
	
	if (i== NUM_VNI)
	{
	  	// No packets to process!
	  	return;
	}
	
	if (sicb.cmd == nicbRESPONSE && (sicb.pdu[0]&0x0F) == LNM_TAG && sicb.pdu[14] == (ND_resp_success|ND_QUERY_XCVR))
	{
	  	// This is the response to a xcvr register read (done in LKFetchXcvr()).  Save the result.
		memcpy(&vniXcvrParam[plcVni], &sicb.pdu[15], sizeof(vniXcvrParam[0]));
		return;
	}
		
    lpduSize 		  =	sicb.len-3;	// Subtract 2 for register info and 1 for zero crossing info
    lpduHeaderPtr     = (LPDUHeader*)&sicb.pdu[1];	// Offset is 1 because of zero crossing info
	
   	/* Throw away packets that are smaller than 8 bytes long. */
	/* For pseudo L2 MIP, CRC errors are reported with a short length. */
	if (sicb.cmd == nicbINCOMING_L2M2 && lpduSize < 8 ||
		(sicb.cmd&0xF0) == (nicbERROR&0xF0))
	{
	  	INCR_STATS(LcsTxError);
		return;
	}
	else if (sicb.cmd != nicbINCOMING_L2M2)
	{
	    if (sicb.cmd == nicbRESET || sicb.cmd == nicbINCOMING_L2 ||
			sicb.cmd == nicbINCOMING_L2M1)
		{
		  	// Phase setting got lost!
			setPhase = true;
		}
	  	return;
	}
	
	// Fill in the packet specific register info
	tempPtr = &sicb.pdu[sicb.len-2];
	xcvrParams.data[2] = *tempPtr++;
	xcvrParams.data[3] = xcvrParams.data[4] = *tempPtr;	
		
    /* Do CRC check. */
    /* this check is now made in the mac sublayer */
    /* Only packets with valid CRC and >= 8 bytes are placed
       in the lkInQ by mac sublayer. */

    INCR_STATS(LcsL2Rx); /* Got a good packet. */

	/* We need to receive this message. */
    if (QueueFull(&gp->nwInQ))
    {
        /* We are losing this packet. */
        INCR_STATS(LcsMissed);
    }
    else
    {
        nwReceiveParamPtr = QueueTail(&gp->nwInQ);
        npduPtr           = (Byte *)(nwReceiveParamPtr + 1);

        nwReceiveParamPtr->priority = lpduHeaderPtr->priority;
        nwReceiveParamPtr->altPath  = lpduHeaderPtr->altPath;
        tempPtr = (Byte *)((char *)lpduHeaderPtr + 1);
        nwReceiveParamPtr->pduSize  = lpduSize - 3;
		nwReceiveParamPtr->xcvrParams = xcvrParams;

        /* Copy the NPDU. */
        /* if it was in link layer's queue, then the size should be
           sufficient in network layer's queue as they differ by 3.
           However, let us play safe by checking the size first. */
        if (nwReceiveParamPtr->pduSize <= gp->nwInBufSize)
        {
            memcpy(npduPtr, tempPtr, nwReceiveParamPtr->pduSize);
        }
        else
        {
            ErrorMsg("LKReceive: NPDU size seems too large.\n");
        }
        EnQueue(&gp->nwInQ);
    }
    *(gp->lkInQHeadPtr) = 0;
    gp->lkInQHeadPtr = gp->lkInQHeadPtr + gp->lkInBufSize;
    if (gp->lkInQHeadPtr ==
            (gp->lkInQ + gp->lkInBufSize * gp->lkInQCnt))
    {
        gp->lkInQHeadPtr = gp->lkInQ; /* wrap around. */
    }
	
    return;
}

/*******************************************************************************
Function:  CRC16
Returns:   16 bit CRC computed.
Purpose:   To compute the 16 bit CRC for a given buffer.
Comments:  None.
*******************************************************************************/
void CRC16(Byte bufInOut[], uint16 sizeIn)
{
    uint16 poly = 0x1021;       /* Generator Polynomial. */
    uint16 crc = 0xffff;
    uint16 i,j;
    unsigned char byte, crcbit, databit;

    for (i = 0; i < sizeIn; i++)
    {
        byte = bufInOut[i];
        for (j = 0; j < 8; j++)
        {
            crcbit = crc & 0x8000 ? 1 : 0;
            databit = byte & 0x80 ? 1 : 0;
            crc = crc << 1;
            if (crcbit != databit)
            {
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

void LKGetTransceiverParams(int index, XcvrParam *p)
{
  	*p = vniXcvrParam[index];
}

//
// LKFetchXcvr
//
// Fetch XCVR params for PLC.  For RF, we have nothing to poll!
//
void LKFetchXcvr(void)
{
	const int msgLen = 1;
	const L2Frame sicbOut = {nicbLOCALNM, 14+msgLen, 0x70|LNM_TAG, 0x00, msgLen, 
							 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							 ND_opcode_base|ND_QUERY_XCVR};
	// If write fails, we'll try again next time.
	xcvrFetch = vldv_write(vniHandle[plcVni], (L2Frame*)&sicbOut, (short)(sicbOut.len+2)) != LDV_OK;
}

#endif  // LINK_IS(MIP) || LINK_IS(USB)
