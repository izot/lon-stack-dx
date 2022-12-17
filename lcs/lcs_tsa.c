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

/*********************************************************************
          File:        lcs_tsa.c (Transport Session Authentication)

       Version:        1

     Reference:        Sections 8, 9, Protocol Specification.

       Purpose:        Transport, Session and Authentication Layers.

          Note:        None.

         To Do:        None.
*********************************************************************/
/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcs_eia709_1.h"
#include "lcs_node.h"
#include "lcs_queue.h"
#include "lcs_tcs.h"
#include "lcs_tsa.h"
#include "lcs_app.h" /* For TAG related macros and constants */
#include "lcs_netmgmt.h"

/*--------------------------------------------------------------------
Section: Constant Definitions.
--------------------------------------------------------------------*/
/* #define DEBUG */
/* The last few tries for a message are sent using alternate path.
   The following constant determines how many are sent like this.
   A message is sent on alternate path if retries_left <= ALT_PATH_COUNT.
   Thus actual # of messages sent on alternate path is ALT_PATH_COUNT + 1 */
#define ALT_PATH_COUNT 1

/*-------------------------------------------------------------------
Section: Type Definitions.
-------------------------------------------------------------------*/
typedef enum
{
    TRANSPORT,
    SESSION
} Layer;

typedef enum
{
    ACKD_MSG        = 0,  /* for Transport */
    REQUEST_MSG     = 0,  /* for Session   */
    CHALLENGE_MSG   = 0,  /* for Authentication */
	CHALLENGE_OMA_MSG = 1,/* for Authentication */
    UNACK_RPT_MSG   = 1,  /* for Transport */
    ACK_MSG         = 2,  /* for Transport */
    RESPONSE_MSG    = 2,  /* for Session */
    REPLY_MSG       = 2,  /* for Authentication */
    REPLY_OMA_MSG   = 3,  /* for Authentication */
    REMINDER_MSG    = 4,  /* for Transport and Session */
    REM_MSG_MSG     = 5   /* for Transport and Session */
} PDUMsgType;  /* Type of msg sent in the PDU
                  (TPDU or SPDU or AuthPDU) */
typedef enum
{
	AF_BROADCAST,
	AF_MULTICAST,
	AF_SUBNETNODE,
	AF_UNIQUE_ID
} AddressFormat;

typedef struct
{
	BITS3(auth				,1,	// Needs auth?
		  pduMsgType		,3,	// Type of PDU.  See PDUMsgType
		  transNum			,4)
    Byte          data[1];        /* Variable length field */
} TSPDU; /* Transport or Session PDU */

typedef TSPDU *TSPDUPtr;

typedef struct
{
	BITS3(fmt			 ,2,	// Same as addrfmt
		  pduMsgType     ,2,	// Type of AuthPDU.  See PDUMsgType
		  transNum       ,4)	// Transaction number
    union {
        Byte randomBytes[8];    /* Random number for challenge.    */
        Byte cryptoBytes[8];    /* Encrypted value in response.    */
    } value;
    Byte group;                /* Present only if fmt = 1.        */
} AuthPDU;

typedef AuthPDU *AuthPDUPtr;

/*-------------------------------------------------------------------
Section: Globals.
-------------------------------------------------------------------*/
/* None */


/* Array to convert address format to address mode. If format is 2,
   we convert to SUBNET_NODE instead of MULTICAST_ACK */
static const AddrMode addrFmtToMode[4] =
{
    BROADCAST,     /* 0 */
    MULTICAST,     /* 1 */
    SUBNET_NODE,   /* 2 */
    UNIQUE_NODE_ID /* 3 */
};

/* Array to convert address mode to format. For example,
   BROADCAST to 0 MULTICAST to 1 etc. */
static const Byte addrModeToFmt[6] =
{
    0,
    2, /* SUBNET_NODE   */
    3, /* UNIQUE_NODE_ID*/
    0, /* BROADCAST     */
    1, /* MULTICAST     */
    2  /* MULTICAST_ACK */
};

/*-------------------------------------------------------------------
Section: Local Function Prototypes.
-------------------------------------------------------------------*/
/* Authentication related functions. */
static void InitiateChallenge(uint16 rrIndexIn);
static void SendReplyToChallenge(Boolean useOma);
static void ProcessReply(Boolean useOma);

/* Transport layer related functions. */
static void TPSendAck(uint16 rrIndexIn);
static void TPReceiveAck(void);

/* Session layer related functions. */
static void SNSendResponse(uint16 rrIndexIn, Boolean nullResponse, Boolean flexResponse);
static void SNReceiveResponse(void);

/* Functions that are common to both transport and session layers. */
static void XmitTimerExpiration(Layer layerIn, Boolean priorityIn);
static void TerminateTrans(Boolean priorityIn);
static void SendNewMsg(Layer layerIn, Boolean priorityIn);
static void ReceiveNewMsg(Layer layerIn);
static void ReceiveRem(Layer layerIn);
static void Deliver(uint16 rrIndexIn);

static int16 AllocateRR(void);
static Bool FindRR(RequestId id, uint16 *pIndex);
static int16 RetrieveRR(SourceAddress srcAddrIn, Boolean priorityIn);

static uint16 ComputeRecvTimerValue(AddrMode      addrModeIn,
                                    MulticastAddress group);
static void Encrypt(Byte rand[], APDU *apdu, uint16 apduSize,
                    Byte *pKey, Byte encryptValue[], Boolean isOma, OmaAddress* pOmaDest);

/*-------------------------------------------------------------------
Section: Function Definitions.
-------------------------------------------------------------------*/

/*****************************************************************
Function: AuthOma
Purpose:  Returns true if this is an OMA system
******************************************************************/
Boolean AuthOma(void)
{
	return eep->domainTable[0].authType == AUTH_OMA;
}

/*****************************************************************
Function: SendCompletion
Purpose:  Send a completiont even up to the app
******************************************************************/
void SendCompletion(TSASendParam *tsaSendParamPtr, Boolean success)
{
	// If there is no room for the completion event then we just don't dequeue the outgoing message and the 
	// caller will try again later.
    if (!QueueFull(&gp->appInQ))
    {
	    Queue *tsaQPtr                      = tsaSendParamPtr->priority ? &gp->tsaOutPriQ : &gp->tsaOutQ;     /* Pointer to source queue */
        APPReceiveParam *appReceiveParamPtr = QueueTail(&gp->appInQ);
        appReceiveParamPtr->indication		= COMPLETION;
        appReceiveParamPtr->success			= success;
        appReceiveParamPtr->tag				= tsaSendParamPtr->tag;
		appReceiveParamPtr->proxy			= tsaSendParamPtr->proxy;
		appReceiveParamPtr->proxyCount		= tsaSendParamPtr->proxyCount;
		appReceiveParamPtr->proxyDone       = tsaSendParamPtr->proxyDone;
        EnQueue(&gp->appInQ);
        DeQueue(tsaQPtr);
    }
}

/*****************************************************************
Function: SendBlocked
Purpose:  Returns true if sending is blocked due to power up delay
******************************************************************/
Boolean SendBlocked(void)
{
    return MsTimerRunning(&gp->tsDelayTimer);
}

/*****************************************************************
Function:  TSAReset
Returns:   None
Reference: None
Purpose:   To initialize the queues used by the transport and session
           layers and to initialize transmit and receive records.
Comments:  Sets gp->resetOk to FALSE if unable to reset properly.
******************************************************************/
void TSAReset(void)
{
    uint16  queueItemSize;
    uint16  i;

    /* Allocate and initialize the input queue. */

    /* Some TSPDUs have APDU attached and others do not.
       The max # bytes for TSPDU with no APDU is 10 (REMINDER).
       The max header size for those with APDU is 4 (REM/MSG). */
    gp->tsaInBufSize =
        DecodeBufferSize((uint8)eep->readOnlyData.appInBufSize) + 4;
    gp->tsaInBufSize = MAX(gp->tsaInBufSize, 10);
    gp->tsaInQCnt    = DecodeBufferCnt((uint8)eep->readOnlyData.appInBufCnt);
    queueItemSize    = gp->tsaInBufSize + sizeof(TSAReceiveParam);

    if (QueueInit(&gp->tsaInQ, queueItemSize, gp->tsaInQCnt)
            != SUCCESS)
    {
        ErrorMsg("TSAReset: Unable to initialize the input queue.");
        gp->resetOk = FALSE;
        return;
    }

    /* Allocate and initialize the output queue. */
    gp->tsaOutBufSize =
        DecodeBufferSize((uint8)eep->readOnlyData.appOutBufSize) + 4;
    gp->tsaOutBufSize = MAX(gp->tsaOutBufSize, 10);
    gp->tsaOutQCnt    =
        DecodeBufferCnt((uint8)eep->readOnlyData.appOutBufCnt);
    queueItemSize     = gp->tsaOutBufSize + sizeof(TSASendParam);

    if (QueueInit(&gp->tsaOutQ, queueItemSize, gp->tsaOutQCnt)
            != SUCCESS)
    {
        ErrorMsg("TSAReset: Unable to initialize the output queue.");
        gp->resetOk = FALSE;
        return;
    }

    /* Allocate and initialize the priority output queue. */
    gp->tsaOutPriBufSize = gp->tsaOutBufSize;
    gp->tsaOutPriQCnt =
        DecodeBufferCnt((uint8)eep->readOnlyData.appOutBufPriCnt);
    queueItemSize    = gp->tsaOutPriBufSize + sizeof(TSASendParam);

    if (QueueInit(&gp->tsaOutPriQ, queueItemSize, gp->tsaOutPriQCnt)
            != SUCCESS)
    {
        ErrorMsg("TSAReset: Unable to initialize the priority output queue.");
        gp->resetOk = FALSE;
        return;
    }

    /* Allocate and initialize the responses queue. */
    gp->tsaRespBufSize = gp->tsaOutBufSize;
    gp->tsaRespQCnt    = gp->tsaOutQCnt;
    queueItemSize      = gp->tsaRespBufSize + sizeof(TSASendParam);

    if (QueueInit(&gp->tsaRespQ, queueItemSize, gp->tsaRespQCnt)
            != SUCCESS)
    {
        ErrorMsg("TSAReset: Unable to initialize the responses queue.");
        gp->resetOk = FALSE;
        return;
    }

    /* Initialize the transmit records. */
    gp->xmitRec.status    = UNUSED_TX;
    gp->priXmitRec.status = UNUSED_TX;

    /* Initialize the receive records. */
    gp->recvRecCnt = RECEIVE_TRANS_COUNT;
    gp->recvRec    = AllocateStorage((uint16)(gp->recvRecCnt *
                                     sizeof(ReceiveRecord)));
    if (gp->recvRec == NULL)
    {
        ErrorMsg("TSAReset: Insufficient space for allocating receive records.");
        gp->resetOk = FALSE;
        return;
    }

    for (i = 0; i < gp->recvRecCnt; i++)
    {
        gp->recvRec[i].response =
            AllocateStorage(DecodeBufferSize((uint8) eep->readOnlyData.appOutBufSize));
        gp->recvRec[i].apdu =
            AllocateStorage(DecodeBufferSize((uint8) eep->readOnlyData.appInBufSize));
        if (gp->recvRec[i].response == NULL ||
                gp->recvRec[i].apdu     == NULL)
        {
            ErrorMsg("TSAReset: Insufficient space for response or apdu.");
            gp->resetOk = FALSE;
            return;
        }
        gp->recvRec[i].status = UNUSED_RR;
    }

    /* Initialize the running count for request id assignment. */
    gp->reqId = 0;

    return;
}


/*****************************************************************
Function:  TPSend
Returns:   None
Reference: Section 8, Protocol Specification.
Purpose:   To implement send algorithm for transport layer.
           If there is anything to be sent by transport layer,
           it processes that message and calls the right function
           that sends it.
Comments:  Update the priority transaction timer, if it exists.
           Update the non-priority transaction timer, if it exists.
           If the priority transaction timer expired then
              process this event.
           else if there is a priority message to be sent and there is space
                in priority queue of the network layer then
              process the priority message.
           else if non-priority transaction timer expired then
              process that event.
           else if there is a non-priority message to be sent and there
                is space in the non-priority queue of the network layer then
              process the non-priority message.
           else
                there is nothing to do. return.
Note:
******************************************************************/
void TPSend(void)
{
    /* Delay TPSend after power-up or external reset. */
    if (SendBlocked())
    {
        return; /* Do nothing */
    }

    /***************************************************
      Priority transaction timer expired event.
     **************************************************/
    if (gp->priXmitRec.status == TRANSPORT_TX && !MsTimerRunning(&gp->priXmitRec.xmitTimer))
    {
        XmitTimerExpiration(TRANSPORT, TRUE);
        return;
    }
    /***************************************************
      Send a new priority message event.
     **************************************************/
    else if (gp->priXmitRec.status == UNUSED_TX &&
             ! QueueEmpty(&gp->tsaOutPriQ)      &&
             ! QueueFull(&gp->nwOutPriQ) )
    {
        SendNewMsg(TRANSPORT, TRUE);
        return;
    }
    /***************************************************
      Non-priority transaction timer expired event.
      *************************************************/
    else if (gp->xmitRec.status == TRANSPORT_TX && !MsTimerRunning(&gp->xmitRec.xmitTimer))
    {
        XmitTimerExpiration(TRANSPORT, FALSE);
    }
    /***************************************************
      Send a new non-priority message.
     **************************************************/
    else if (gp->xmitRec.status == UNUSED_TX &&
             ! QueueEmpty(&gp->tsaOutQ)      &&
             ! QueueFull(&gp->nwOutQ) )
    {
        SendNewMsg(TRANSPORT, FALSE);
    }
    else
    {
        /* Either there is no work or there is no space. */
        return;
    }

    return;

}

/*****************************************************************
Function:  TerminateTrans
Returns:   None
Reference: None
Purpose:   To terminate a transaction for transport or session
           layer and send the completion indication to application
           layer. If the application layer's input queue is full,
           we don't terminate the transaction.
Comments:  layerIn is not passed as it is not needed.
******************************************************************/
static void TerminateTrans(Boolean priorityIn)
{
    Queue           *tsaQPtr;     /* Pointer to source queue */
    TSASendParam    *tsaSendParamPtr;
    TransmitRecord  *xmitRecPtr;  /* Ptr to xmit rec (pri or nonpri)*/
    Boolean          success;

    if (QueueFull(&gp->appInQ))
    {
        return; /* Can't send the indication. Come back later. */
    }

    if (priorityIn)
    {
        tsaQPtr         = &gp->tsaOutPriQ;
        tsaSendParamPtr = QueueHead(tsaQPtr);
        xmitRecPtr      = &gp->priXmitRec;
    }
    else
    {
        tsaQPtr         = &gp->tsaOutQ;
        tsaSendParamPtr = QueueHead(tsaQPtr);
        xmitRecPtr      = &gp->xmitRec;
    }

    if (tsaSendParamPtr->service == UNACK_RPT ||
            xmitRecPtr->destCount == xmitRecPtr->ackCount ||
            (xmitRecPtr->nwDestAddr.addressMode == BROADCAST &&
             xmitRecPtr->ackCount >= 1)
       )
    {
        /* UNACK_RPT or ACK and got all acks(or resp). */
        success = TRUE;
    }
    else
    {
        INCR_STATS(LcsTxFailure);
        success = FALSE; /* REQUEST or ACK and did not get all acks. */
    }

    TransDone(priorityIn); /* Call to TCS. */
    xmitRecPtr->status = UNUSED_TX;
	if (success)
	{
		DebugMsg("TermTran: Terminated the transaction. Success.");
	}
	else
	{
		DebugMsg("TermTran: Terminated the transaction. Fail.");
	}
	SendCompletion(tsaSendParamPtr, success);
    return;
}

/*****************************************************************
Function:  XmitTimerExpiration
Returns:   None
Reference: None
Purpose:   To process the XmitTimer expiration event (pri or nonpri).
           Retranmission and termination of transaction are
           handled. Retransmission might be reply to already
           initiated challenge or it might be the APDU itself.
           If there is no space for retransmission, the retry is
           lost.
Comments:  None
******************************************************************/
static void XmitTimerExpiration(Layer layerIn, Boolean priorityIn)
{
    TSASendParam *tsaSendParamPtr;/* Param in tsaQ (Pri or nonPri).   */
    NWSendParam  *nwSendParamPtr; /* Param in nwQ (Pri or nonPri).    */
    TransmitRecord  *xmitRecPtr;  /* Ptr to xmit rec (pri or nonpri). */
    Queue        *tsaQPtr;        /* Pointer to source queue.         */
    Queue        *nwQPtr;         /* Pointer to target queue.         */
    TSPDUPtr      pduPtr;         /* Pointer to TSPDU being formed.   */
    uint8         deltaBL;
    uint16        pduSize;
    int8          i;
    uint8         length; /* For length of reminder in bytes. */
    uint16        queueSpace;

    if (priorityIn)
    {
        tsaQPtr         = &gp->tsaOutPriQ;
        tsaSendParamPtr = QueueHead(tsaQPtr);
        nwQPtr          = &gp->nwOutPriQ;
        nwSendParamPtr  = QueueTail(nwQPtr);
        xmitRecPtr      = &gp->priXmitRec;
    }
    else
    {
        tsaQPtr         = &gp->tsaOutQ;
        tsaSendParamPtr = QueueHead(tsaQPtr);
        nwQPtr          = &gp->nwOutQ;
        nwSendParamPtr  = QueueTail(nwQPtr);
        xmitRecPtr      = &gp->xmitRec;
    }

    /* First, check if we really need to retry the message. */
    if (xmitRecPtr->retriesLeft == 0 ||
            xmitRecPtr->destCount == xmitRecPtr->ackCount ||
            (xmitRecPtr->nwDestAddr.addressMode == BROADCAST &&
             xmitRecPtr->ackCount >= 1)
       )
    {
        /* No More retries left or all acks have been received.
           Terminate the transaction. Send indication to the application
           layer. */
        TerminateTrans(priorityIn);
        return;
    }

    /* Check if there is space in the network buffer for retransmission */
    if (QueueFull(nwQPtr))
    {
        /* We are losing a retry chance locally due to lack of space
           in network queue. If we don't want to lose the retry, we
           simply delete the next two lines of code */
        xmitRecPtr->retriesLeft--;
        /* Start the transmit timer */
        MsTimerSet(&xmitRecPtr->xmitTimer, xmitRecPtr->xmitTimerValue);
        DebugMsg("XmitTimerExp: Retry failure due to no space in net");
        return;
    }

    /* Now, we need to retransmit the message again. */
    /* Form the PDU to be sent directly in the target queue. */
    pduPtr              = (TSPDUPtr) (nwSendParamPtr + 1);
    pduPtr->auth        = tsaSendParamPtr->auth;
	pduPtr->transNum    = xmitRecPtr->transNum;
	
    if (tsaSendParamPtr->service == UNACK_RPT)
    {
        pduPtr->pduMsgType     = UNACK_RPT_MSG;
        memcpy(pduPtr->data, xmitRecPtr->apdu,
               xmitRecPtr->apduSize);
        pduSize = xmitRecPtr->apduSize + 1;
        DebugMsg("XmitTimerExp: Resending UNACK_RPT packet.");
    }
    else if (xmitRecPtr->nwDestAddr.addressMode != MULTICAST)
    {
        if (layerIn == TRANSPORT)
        {
            pduPtr->pduMsgType = ACKD_MSG;
            DebugMsg("XmitTimerExp: Resending ACKD packet.");
        }
        else if (tsaSendParamPtr->service == REQUEST)
        {
            pduPtr->pduMsgType = REQUEST_MSG;
            DebugMsg("XmitTimerExp: Resending REQUEST packet.");
        }
        else
        {
            /* Response Messages are retried. Something is wrong. */
            /* Force retriesLeft to 0 so that next time we will
               terminate the transaction. */
            xmitRecPtr->retriesLeft = 0;
            DebugMsg("XmitTimerExp: Response Message??. What is wrong?");
            return;
        }
        memcpy(pduPtr->data, xmitRecPtr->apdu,
               xmitRecPtr->apduSize);
        pduSize = xmitRecPtr->apduSize + 1;
    }
    else
    {
        /* Multicast Retry */
        /* Compute the highest numbered group member that has
           acknowledged and form the M_LIST up to that. Since the
           rest of the nodes have not acknowledged, they will
           respond when they see that their bit is missing.
           However, the last byte of M_LIST should be padded with
           0 as those members need to acknowledge. Note that a node
           will respond if its member number is not even present
           in the M_List. */
        /* Group members are 0..MAX_GROUP_NUMBER */
        if (xmitRecPtr->ackCount == 0)
        {
            length = 0;
        }
        else
        {
            /* ackCount > 0, So, we have at least one ack.
               Find the highest member who have responded. */
            for (i = MAX_GROUP_NUMBER; i >= 0; i--)
            {
                if (xmitRecPtr->ackReceived[i])
                {
                    break;
                }
            }
            if (i == -1)
            {
                /* There should have been at least one ack. */
                ErrorMsg("XmitTimerExpiration: Something is wrong."
                         " Check code. Atleast one ack member expected.");
                xmitRecPtr->retriesLeft = 0;
                return;
            }
            length = i / 8 + 1; /* Number of bytes in M_List. */
        }

        if (length <= 2)
        {
            pduPtr->pduMsgType     = REM_MSG_MSG;
            pduPtr->data[0]     = length; /* # of bytes in M_List. */
            /* Copy the M_LIST. See Fig 8.2 in Protocol Specification. */
            /* We use the fact that ackReceived[i] is 0 or 1. */
            /* The padding of 0's of last byte is automatic as
               ackReceived[i] for those are anyway 0. */
            pduPtr->data[1] = 0; /* Init anyway even if not used. */
            pduPtr->data[2] = 0; /* Init anyway even if not used. */
            for (i = 0; i < 8*length; i++)
            {
                pduPtr->data[1 + i / 8] |=
                    (xmitRecPtr->ackReceived[i] << (i % 8));
            }
            /* Copy APDU. */
            memcpy(&pduPtr->data[1+length],
                   xmitRecPtr->apdu,
                   xmitRecPtr->apduSize);
            /* TSPDU = 1 byte header + 1 byte for length + M_LIST. */
            pduSize = xmitRecPtr->apduSize + 2 + length;
            DebugMsg("XmitTimerExp: Resending REMINDER  packet.");
        }
        else
        {
            /* Length > 2 */
            /* A Pair is sent in this case. First, send the REMINDER
               and then send the ACKD or REQUEST message. */
            /* In this case, we are going to send two msgs.
               So, we need to make sure that the queue has space
               for 2 msgs. If not, return and come back later to
               do this case. */
            queueSpace = QueueCnt(nwQPtr) - QueueSize(nwQPtr);
            if (queueSpace < 2)
            {
                /* We are losing a retry chance locally due to lack
                   of space in network queue. */
                xmitRecPtr->retriesLeft--;
                /* Start the transmit timer. */
                MsTimerSet(&xmitRecPtr->xmitTimer,
                           xmitRecPtr->xmitTimerValue);
                DebugMsg("XmitTimerExp: Retry failure due to no"
                         " space in network buffer.");
                return; /* Not enough space in the queue. Come back. */
            }
            /* Send the REMINDER message. */
            if (tsaSendParamPtr->service == ACKD ||
                    tsaSendParamPtr->service == REQUEST)
            {
                nwSendParamPtr = QueueTail(nwQPtr);
                nwSendParamPtr->dropIfUnconfigured = TRUE;
                pduPtr = (TSPDUPtr)
                         ((char *)nwSendParamPtr + sizeof(NWSendParam));

                pduPtr->auth        = tsaSendParamPtr->auth;
                pduPtr->pduMsgType  = REMINDER_MSG;
                pduPtr->transNum    = xmitRecPtr->transNum;
                pduPtr->data[0]     = length;

                /* Copy the M_LIST. See Fig 8.2 in Protocol Specification. */
                /* First, initialize all the M_LIST fields to 0. */
                for (i = 1; i <= length; i++)
                {
                    pduPtr->data[i] = 0;
                }
                /* Set the bits for M_LIST field based on received acks. */
                for (i = 0; i < 8*length; i++)
                {
                    pduPtr->data[1 + i / 8] |=
                        (xmitRecPtr->ackReceived[i] << (i % 8));
                }

                pduSize = 2 + length; /* REMINDER has no APDU. */

                /* Fill in the NWSendParam structure. */
                nwSendParamPtr->destAddr = xmitRecPtr->nwDestAddr;
                if (layerIn == TRANSPORT)
                {
                    nwSendParamPtr->pduType   = TPDU_TYPE;
                }
                else
                {
                    nwSendParamPtr->pduType   = SPDU_TYPE;
                }
                nwSendParamPtr->deltaBL  = 0; /* REMINDER has deltaBL 0. */
                nwSendParamPtr->pduSize  = pduSize;

                /* UnAck_rpt packets do not use alt path. */
                if (tsaSendParamPtr->service != UNACK_RPT)
                {
                    nwSendParamPtr->altPath  =
                        (xmitRecPtr->retriesLeft <= (ALT_PATH_COUNT + 1));
                }
                else
                {
                    nwSendParamPtr->altPath  = FALSE;
                }

                /* if altPath has override, use it */
                if (tsaSendParamPtr->altPathOverride)
                {
                    nwSendParamPtr->altPath = tsaSendParamPtr->altPath;
                }

                /* Add TSPDU into the queue. */
                EnQueue(nwQPtr);
            }

            /* Send the ACKD or REQUEST. */
            nwSendParamPtr = QueueTail(nwQPtr);
            nwSendParamPtr->dropIfUnconfigured = TRUE;
            pduPtr = (TSPDUPtr)
                     ((char *)nwSendParamPtr + sizeof(NWSendParam));
            pduPtr->auth        = tsaSendParamPtr->auth;
            pduPtr->transNum    = xmitRecPtr->transNum;
            if (tsaSendParamPtr->service == ACKD)
            {
                pduPtr->pduMsgType     = ACKD_MSG;
            }
            else
            {
                pduPtr->pduMsgType     = REQUEST_MSG;
            }
            memcpy(pduPtr->data,
                   xmitRecPtr->apdu,
                   xmitRecPtr->apduSize);
            pduSize = xmitRecPtr->apduSize + 1;

            DebugMsg("XmitTimerExp: Resending REM/MSG pair.");
        }
    }


    if (tsaSendParamPtr->service != UNACK_RPT)
    {
        INCR_STATS(LcsRetry);
    }

    /* Compute the delta backlog value. */
    deltaBL = 1; /* for subnet and unique id messages. */
    if (tsaSendParamPtr->service == UNACK_RPT)
    {
        deltaBL = 0; /* Only on first attempt, deltaBL is retries left. */
    }
    else if (xmitRecPtr->nwDestAddr.addressMode == BROADCAST)
    {
        /* Domainwide or subnet BROADCAST. */
        /* If there is no override value for deltaBL, then it is 15. */
        if (tsaSendParamPtr->destAddr.bcast.backlog)
        {
            deltaBL = tsaSendParamPtr->destAddr.bcast.backlog;
        }
        else
        {
            deltaBL = 15;
        }
    }
    else if (xmitRecPtr->nwDestAddr.addressMode == MULTICAST)
    {
        /* deltaBL is outstanding responses or acknowledgements. */
        deltaBL = xmitRecPtr->destCount - xmitRecPtr->ackCount;
    }

    /* Fill in the NWSendParam structure */
    nwSendParamPtr->destAddr = xmitRecPtr->nwDestAddr;
    if (layerIn == TRANSPORT)
    {
        nwSendParamPtr->pduType   = TPDU_TYPE;
    }
    else
    {
        nwSendParamPtr->pduType   = SPDU_TYPE;
    }
    nwSendParamPtr->deltaBL  = deltaBL;

    /* UnAck_rpt packets do not use alt path. */
    if (tsaSendParamPtr->service != UNACK_RPT)
    {
        nwSendParamPtr->altPath  =
            (xmitRecPtr->retriesLeft <= (ALT_PATH_COUNT + 1));
    }
    else
    {
        nwSendParamPtr->altPath  = FALSE;
    }
    /* if altPath has override, use it */
    if (tsaSendParamPtr->altPathOverride)
    {
        nwSendParamPtr->altPath = tsaSendParamPtr->altPath;
    }


    nwSendParamPtr->pduSize  = pduSize;

    xmitRecPtr->retriesLeft--;

    /* Add TSPDU into the queue. */
    EnQueue(nwQPtr);

    /* Start the transmit timer. */
	if (xmitRecPtr->retriesLeft == 0)
	{
		// Add in any "last retry" extra wait.  This wait is needed for proxy where a timeout happens simultaneously
		// on multiple repeaters and thus extra time is needed to propagate a failing response down the chain.
		xmitRecPtr->xmitTimerValue += xmitRecPtr->txTimerDeltaLast;
	}
    MsTimerSet(&xmitRecPtr->xmitTimer, xmitRecPtr->xmitTimerValue);

    return;
}

/*****************************************************************
Function:  SendNewMsg
Returns:   None
Reference: None
Purpose:   To process a new request from the application layer that
           is in the tsa output queue (pri or nonpri). Request or ACKD.
Comments:  This fn is called only if there is space in the
           corresponding queue of the network layer.
******************************************************************/
static void SendNewMsg(Layer layerIn, Boolean priorityIn)
{
    Queue          *tsaQPtr;        /* Pointer to the source queue.    */
    TSASendParam   *tsaSendParamPtr;/* Param in tsaQ (Pri or non-pri). */
    Queue          *nwQPtr;         /* Pointer to target queue.        */
    NWSendParam    *nwSendParamPtr; /* Param in nwQ (Pri or non-pri).  */
    TransmitRecord *xmitRecPtr;     /* Ptr to xmit rec.                */
    DestinationAddress nwDestAddr;  /* Destination address.            */
    TSPDUPtr        pduPtr;         /* Pointer to TSPDU being formed.  */
    Status          status;
    uint16          rptTimer;
    uint8           retryCount;
    uint16          txTimer;
    uint8           deltaBL;
    uint16          nwBufSize;
    int8            i;
	uint16			rrIndex;

    if (priorityIn)
    {
        tsaQPtr         = &gp->tsaOutPriQ;
        tsaSendParamPtr = QueueHead(tsaQPtr);
        nwQPtr          = &gp->nwOutPriQ;
        nwSendParamPtr  = QueueTail(nwQPtr);
        nwBufSize       = gp->nwOutPriBufSize;
        xmitRecPtr      = &gp->priXmitRec;
    }
    else
    {
        tsaQPtr         = &gp->tsaOutQ;
        tsaSendParamPtr = QueueHead(tsaQPtr);
        nwQPtr          = &gp->nwOutQ;
        nwSendParamPtr  = QueueTail(nwQPtr);
        nwBufSize       = gp->nwOutBufSize;
        xmitRecPtr      = &gp->xmitRec;
    }

    /* If processing a new message, make sure that it is for this
       layer. If not, we are done. */

    if (layerIn == TRANSPORT &&
            tsaSendParamPtr->service != ACKD &&
            tsaSendParamPtr->service != UNACK_RPT)
    {
        return;
    }

    if (layerIn == TRANSPORT && NV_LAST_TAG(tsaSendParamPtr->tag))
    {
		SendCompletion(tsaSendParamPtr, TRUE);
        return;
    }

    if (layerIn == SESSION && tsaSendParamPtr->service != REQUEST)
    {
        /* Responses are placed in the response queue. */
        return;
    }

    /* Make sure that large group size is not used for ack
       or request service. */
    if (tsaSendParamPtr->destAddr.group.groupFlag &&
            tsaSendParamPtr->destAddr.group.groupSize == 0 &&
            tsaSendParamPtr->service != UNACK_RPT)
    {
        /* Large groups can only use unack or unack_rpt services. */
        /* Indicate failure of this message to application layer. */
		SendCompletion(tsaSendParamPtr, FALSE);
        return;
    }
    /* Make sure that groupSize is in the proper range. */
#ifdef GROUP_SIZE_COMPATIBILITY
    if (tsaSendParamPtr->destAddr.group.groupFlag &&
            (tsaSendParamPtr->destAddr.group.groupSize == 1 ||
             tsaSendParamPtr->destAddr.group.groupSize > MAX_GROUP_NUMBER+1) )
#else
    if (tsaSendParamPtr->destAddr.group.groupFlag &&
            tsaSendParamPtr->destAddr.group.groupSize > MAX_GROUP_NUMBER)
#endif
    {
		SendCompletion(tsaSendParamPtr, FALSE);
        return;
    }

    /* Make sure there is space in network buffer. If not, we fail. */
    /* apdu + (tran or session header of 1 byte) should fit. */
    if ((tsaSendParamPtr->apduSize + 1) > nwBufSize)
    {
        /* We can't send this message as it is too big for
           the network layer's buffer. */
        /* Right now, we haven't allocated any transmit record. */
        /* So, we directly give the indication to application. */
		SendCompletion(tsaSendParamPtr, FALSE);
        return;
    }

    /* First, compute nwDestAddr from destAddr. */
    /* First, initialize domainIndex. Only if it is COMPUTE_DOMAIN_INDEX,
       we need to recompute it based on destAddr field value. */
    nwDestAddr.dmn = tsaSendParamPtr->dmn;

	// Do some stuff common to all address types
    txTimer = DecodeTxTimer((uint8)tsaSendParamPtr->destAddr.snode.txTimer, (Boolean)tsaSendParamPtr->destAddr.snode.longTimer);
    rptTimer = DecodeRptTimer((uint8)tsaSendParamPtr->destAddr.snode.rptTimer);
    retryCount = tsaSendParamPtr->destAddr.snode.retryCount;
	if (tsaSendParamPtr->dmn.domainIndex == COMPUTE_DOMAIN_INDEX)
	{
		nwDestAddr.dmn.domainIndex =
			tsaSendParamPtr->destAddr.snode.domainIndex;
	}

	// Check for group format
    if (tsaSendParamPtr->destAddr.group.groupFlag)
    {
	    nwDestAddr.addressMode = MULTICAST;
		nwDestAddr.addr.addr1 =
			tsaSendParamPtr->destAddr.group.groupID;
	}
	else
	{
		switch (tsaSendParamPtr->destAddr.snode.addrMode)
		{
		case UNBOUND:
			/* Not in use or turnaround format. */
			SendCompletion(tsaSendParamPtr, FALSE);
			return;
		case SUBNET_NODE:
			nwDestAddr.addressMode = SUBNET_NODE;
			nwDestAddr.addr.addr2a.subnet =
				tsaSendParamPtr->destAddr.snode.subnetID;
			nwDestAddr.addr.addr2a.selField = 1; /* always 1 */
			nwDestAddr.addr.addr2a.node   =
				tsaSendParamPtr->destAddr.snode.node;
			break;
		case UNIQUE_NODE_ID:
			nwDestAddr.addressMode = UNIQUE_NODE_ID;
			nwDestAddr.addr.addr3.subnet =
				tsaSendParamPtr->destAddr.uniqueNodeId.subnetID;
			memcpy(nwDestAddr.addr.addr3.uniqueId,
				   tsaSendParamPtr->destAddr.uniqueNodeId.uniqueId,
				   UNIQUE_NODE_ID_LEN);
			break;
		case BROADCAST:
			if (!tsaSendParamPtr->destAddr.bcast.broadcastGroup)
			{
				tsaSendParamPtr->destAddr.bcast.maxResponses = 1;
			}
			/* Broadcast group addressing is optional.  It can not be assumed that all devices
			 * support this form of addressing. */
			nwDestAddr.addr.addr0 =
				tsaSendParamPtr->destAddr.bcast.subnetID;
			break;
		default:
			/* It must be some invalid value. Let us fail. */
			LCS_RecordError(BAD_ADDRESS_TYPE);
			SendCompletion(tsaSendParamPtr, FALSE);
			return;
		} /* switch */
	}

    /* Get transaction number using nwDestAddr. */
    status = NewTrans(priorityIn, nwDestAddr, &xmitRecPtr->transNum);
    if (status == FAILURE)
    {
        /* Unable to get the transaction number. Give up. Try later. */
        return;
    }

    /* Initialize the xmit record. */
    if (layerIn == TRANSPORT)
    {
        xmitRecPtr->status = TRANSPORT_TX;
    }
    else
    {
        xmitRecPtr->status = SESSION_TX;
    }
    xmitRecPtr->nwDestAddr = nwDestAddr; /* Save it for future use. */
    for (i = 0; i <= MAX_GROUP_NUMBER; i++)
    {
        xmitRecPtr->ackReceived[i] = FALSE;
    }
	if (tsaSendParamPtr->proxy)
	{
	 	if (tsaSendParamPtr->txInherit && FindRR(tsaSendParamPtr->tag, &rrIndex))
		{
		  	// OK, we take advantage of the fact that the tag for a proxy request is the same as the receive TX index
		  	// for the incoming message.
	  		xmitRecPtr->transNum = gp->recvRec[rrIndex].transNum;
			OverrideTrans(priorityIn, xmitRecPtr->transNum);
		}
		xmitRecPtr->txTimerDeltaLast = tsaSendParamPtr->txTimerDeltaLast;
		memcpy(&xmitRecPtr->altKey, &tsaSendParamPtr->altKey, sizeof(xmitRecPtr->altKey));
	}
	else
	{
		// Do these things really only need to apply to proxy?  Well, no, they could be general but the way cStack is structured
		// requires that a new field in TSASendParam be initialized by every path that creates it and I don't want to go mess with all of them now.
		xmitRecPtr->txTimerDeltaLast = 0;
		xmitRecPtr->altKey.altKey = FALSE;
	}

    if (nwDestAddr.addressMode == MULTICAST)
    {
        /* If the node is a member of the group, then groupsize
           field is set to 1 more than actual group size
           by in  app layer or app pgm.
           However, provide an option to set this to the
           true group size and transport and session layers
           will take care of this. */
        xmitRecPtr->destCount =
            tsaSendParamPtr->destAddr.group.groupSize - 1;
        /* To be fully compatible with all applications, you must use the GROUP_SIZE_COMPATIBILITY
         * option.  This behavior only affects explicitly addressed messages originated by the
         * application. */
#ifndef GROUP_SIZE_COMPATIBILITY
        if (!IsGroupMember(tsaSendParamPtr->destAddr.group.domainIndex,
                           tsaSendParamPtr->destAddr.group.group,
                           &groupMember))
        {
            /* node is not a member & group size is true size */
            xmitRecPtr->destCount =
                tsaSendParamPtr->destAddr.group.groupSize;
        }
#endif
        if (xmitRecPtr->destCount == 0)
        {
            /* If the value is incorrect, set it to 1. We need at least
               one acknowledgement or response. */
            DebugMsg("SendNewMsg: groupSize incorrect. Default assumed.");
            xmitRecPtr->destCount = 1;
        }
    }
    else
    {
        xmitRecPtr->destCount = 1;
    }

    xmitRecPtr->retriesLeft = retryCount;
    xmitRecPtr->ackCount    = 0;
    xmitRecPtr->apdu        = (APDU *) (tsaSendParamPtr + 1);
    xmitRecPtr->apduSize    = tsaSendParamPtr->apduSize;
    if (tsaSendParamPtr->service == UNACK_RPT)
    {
        xmitRecPtr->xmitTimerValue = rptTimer;
    }
    else
    {
        xmitRecPtr->xmitTimerValue = txTimer;
    }

    /* Form the TSPDU to be sent directly in queue. */
    pduPtr              = (TSPDUPtr)
                          ((char *)nwSendParamPtr + sizeof(NWSendParam));
    pduPtr->auth        = tsaSendParamPtr->auth;
    /* Save auth status so that if a challenge comes later on
       we can at least verify that it was legitimate challenge */
    xmitRecPtr->auth    = tsaSendParamPtr->auth;
    if (tsaSendParamPtr->service == UNACK_RPT)
    {
        pduPtr->pduMsgType     = UNACK_RPT_MSG;
        DebugMsg("SendNewMsg: Sending a new UNACK_RPT packet.");
    }
    else if (layerIn == TRANSPORT)
    {
        pduPtr->pduMsgType     = ACKD_MSG;
        DebugMsg("SendNewMsg: Sending a new ACKD packet.");
    }
    else if (tsaSendParamPtr->service == REQUEST)
    {
        pduPtr->pduMsgType     = REQUEST_MSG;
        DebugMsg("SendNewMsg: Sending a new REQ packet.");
    }
    else
    {
        DebugMsg("SendNewMsg: Something is wrong. Unknown service.");
    }
    pduPtr->transNum    = xmitRecPtr->transNum;
    memcpy(pduPtr->data, xmitRecPtr->apdu,
           xmitRecPtr->apduSize);

    /* Compute the delta backlog value. */
    deltaBL = 1; /* For subnet and unique node id messages. */
    if (tsaSendParamPtr->service == UNACK_RPT)
    {
        deltaBL = xmitRecPtr->retriesLeft;
    }
    else if (nwDestAddr.addressMode == BROADCAST)
    {
        if (tsaSendParamPtr->destAddr.bcast.backlog)
        {
            deltaBL = tsaSendParamPtr->destAddr.bcast.backlog;
        }
        else
        {
            deltaBL = 15;
        }
    }
    else if (nwDestAddr.addressMode == MULTICAST)
    {
        deltaBL = xmitRecPtr->destCount;
    }

    /* Fill in the NWSendParam structure. */
    nwSendParamPtr->dropIfUnconfigured = TRUE;
    nwSendParamPtr->destAddr = nwDestAddr;
    if (layerIn == TRANSPORT)
    {
        nwSendParamPtr->pduType  = TPDU_TYPE;
    }
    else
    {
        nwSendParamPtr->pduType  = SPDU_TYPE;
    }
    nwSendParamPtr->deltaBL  = deltaBL;

    /* UnAck_rpt packets do not use alternate path. */
    if (tsaSendParamPtr->service != UNACK_RPT)
    {
        nwSendParamPtr->altPath  = (retryCount <= ALT_PATH_COUNT);
    }
    else
    {
        nwSendParamPtr->altPath  = FALSE;
    }

    /* if altPath has override, use it */
    if (tsaSendParamPtr->altPathOverride)
    {
        nwSendParamPtr->altPath = tsaSendParamPtr->altPath;
    }

    nwSendParamPtr->pduSize  = xmitRecPtr->apduSize + 1;

    /* Add the TSPDU into the queue. */
    EnQueue(nwQPtr);

    /* Start the transmit timer. */
    MsTimerSet(&xmitRecPtr->xmitTimer, xmitRecPtr->xmitTimerValue);

    return;
}


/*****************************************************************
Function:  TPReceive
Returns:   None
Reference: None
Purpose:   To receive and process incoming TPDU's by calling
           apprpriate functions. Also handle receive timers.
Comments:  None
******************************************************************/
void TPReceive(void)
{
    TSAReceiveParam *tsaReceiveParamPtr;/* Param in tsaQ. */
    TSPDUPtr          pduInPtr;         /* Pointer to TPDU received. */
    int16           i;

    /* Update all the receive timers, if they do exist */
    for (i = 0; i < gp->recvRecCnt; i++)
    {
        if (gp->recvRec[i].status == TRANSPORT_RR)
        {
            if (MsTimerExpired(&gp->recvRec[i].recvTimer))
            {
                /* Timer expired. See if RR can be released. */
                DebugMsg("TPReceive: Receive timer expired.");
                gp->recvRec[i].status = UNUSED_RR; /* Release the RR */
            }
        }
    }

    /* Check if there is TPDU to be processed. */
    if (QueueEmpty(&gp->tsaInQ))
    {
        /* There is nothing to process. */
        return;
    }

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduInPtr           = (TSPDUPtr) (tsaReceiveParamPtr + 1);

    /* Check if the PDU is for transport layer. If not, we are done. */
    if (tsaReceiveParamPtr->pduType != TPDU_TYPE)
    {
        return;
    }

	if (tsaReceiveParamPtr->pduSize < 1)
	{
        ErrorMsg("TPReceive: Illegal PDU size.");
		DeQueue(&gp->tsaInQ);
		return;
	}

    /* We have a TPDU to be processed. Check the type and
       process accordingly. */
    switch (pduInPtr->pduMsgType)
    {
    case ACK_MSG:
        TPReceiveAck();
        return;
    case ACKD_MSG:
        /* Fall through. */
    case UNACK_RPT_MSG:
        ReceiveNewMsg(TRANSPORT);
        return;
    case REMINDER_MSG:
        /* Fall through. */
    case REM_MSG_MSG:
        ReceiveRem(TRANSPORT);
        return;
    default:
        ErrorMsg("TPReceive: Unknown TPDU type received.");
        DeQueue(&gp->tsaInQ);
        LCS_RecordError(UNKNOWN_PDU);
    }

    return;
}

/*****************************************************************
Function:  TPReceiveAck
Returns:   None
Reference: None
Purpose:   To process ACK message received by the transport layer.
Comments:  ACK might correspond to a transmit record or it may be
           stale, in which case we ignore it.
******************************************************************/
static void TPReceiveAck(void)
{
    TSAReceiveParam *tsaReceiveParamPtr;
    TSPDUPtr          pduPtr;
    TransmitRecord  *xmitRecPtr;  /* Ptr to xmit rec (pri or nonpri). */

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduPtr             = (TSPDUPtr) (tsaReceiveParamPtr + 1);

    /* Set the corresponding transmit pointer. */
    if (tsaReceiveParamPtr->priority)
    {
        xmitRecPtr = &gp->priXmitRec;
    }
    else
    {
        xmitRecPtr = &gp->xmitRec;
    }

    if (ValidateTrans(tsaReceiveParamPtr->priority, pduPtr->transNum)
            == TRANS_NOT_CURRENT)
    {
        /* Stale ACK. Ignore it. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("TPReceiveAck: Stale Ack Discarded");
        INCR_STATS(LcsLateAck);
        return;
    }

    /* Check if the ACK really corresponds to the transaction in progress. */
    if (xmitRecPtr->status != TRANSPORT_TX ||
            xmitRecPtr->nwDestAddr.dmn.domainIndex !=
            tsaReceiveParamPtr->srcAddr.dmn.domainIndex)
    {
        /* Transmit record is not ours or the domain index for Ack
           and the transaction do not match. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("TPReceiveAck: Stale ack discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    if (xmitRecPtr->nwDestAddr.addressMode == SUBNET_NODE &&
            (xmitRecPtr->nwDestAddr.addr.addr2a.subnet !=
             tsaReceiveParamPtr->srcAddr.subnetAddr.subnet ||
             xmitRecPtr->nwDestAddr.addr.addr2a.node !=
             tsaReceiveParamPtr->srcAddr.subnetAddr.node))
    {
        /* Source address of ACK does not match destination address of
           transmit record. */
        /* ACK does not seem to correspond to what we are expecting. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("TPReceiveAck: Stale Ack discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    if (xmitRecPtr->nwDestAddr.addressMode == MULTICAST &&
            (tsaReceiveParamPtr->srcAddr.addressMode != MULTICAST_ACK ||
             xmitRecPtr->nwDestAddr.addr.addr1 !=
             tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.group))
    {
        /* MULTICAST message but not MULTICAST_ACK or the ack's group
           # does not match that of xmitRecord. */
        /* ACK does not seem to correspond to what we are expecting. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("TPReceiveAck: Stale ack discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    /* For broadcast messages, ACK can come back with subnet value of 0
       from unconfigured nodes. We should accept these acks too. */
    if (xmitRecPtr->nwDestAddr.addressMode == BROADCAST &&
            tsaReceiveParamPtr->srcAddr.subnetAddr.subnet != 0 &&
            xmitRecPtr->nwDestAddr.addr.addr0 != 0 &&
            xmitRecPtr->nwDestAddr.addr.addr0 !=
            tsaReceiveParamPtr->srcAddr.subnetAddr.subnet)
    {
        /* Subnet broadcast but the ACK's subnet does not match .*/
        /* ACK does not seem to correspond to what we are expecting. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("TPReceiveAck: Stale ack discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    switch (xmitRecPtr->nwDestAddr.addressMode)
    {
    case BROADCAST:    /* Fall Through. */
    case SUBNET_NODE:  /* Fall Through. */
    case UNIQUE_NODE_ID:
        /* Normally the first ack should have terminated the transaction.
           If it did not, we can try to terminate again. There is no
           harm in doing this. Also, there is no harm in increment
           ackCount as XmitTimerExpiration checks for ackCount >= 1. */
        xmitRecPtr->ackCount++; /* Got one more ack. */
        TerminateTrans(tsaReceiveParamPtr->priority);
        break;
    case MULTICAST:
        /* Group acknowledgement. */
        if (tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.member
                > MAX_GROUP_NUMBER)
        {
            ErrorMsg("TPReceiveAck: Invalid group number.");
            break;
        }
        if (!xmitRecPtr->ackReceived[
                    tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.member])
        {
            /* We did not receive this ack in past. */
            xmitRecPtr->ackReceived[
                tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.member]
            = TRUE;
            xmitRecPtr->ackCount++;
            DebugMsg("TPReceiveAck: A new multicast ACK recvd.");
        }
        else
        {
            /* Else, it is a duplicate Ack. Ignore it. */
            DebugMsg("TPReceiveAck: A Duplicate Mul. ACK ignored");
        }
        if (xmitRecPtr->destCount == xmitRecPtr->ackCount)
        {
            TerminateTrans(tsaReceiveParamPtr->priority);
        }
        break;
    default:
        LCS_RecordError(BAD_ADDRESS_TYPE);
        ErrorMsg("TPReceiveAck: Invalid address mode");
    }

    DebugMsg("TPReceiveAck: Ack was received.");

    DeQueue(&gp->tsaInQ);

    /* Restart the Xmit Timer if the Xmit record is still active. */
    if (xmitRecPtr->status != UNUSED_TX)
    {
        MsTimerSet(&xmitRecPtr->xmitTimer, xmitRecPtr->xmitTimerValue);
    }

    return;
}


/*****************************************************************
Function:  SNReceiveResponse
Returns:   None
Reference: None
Purpose:   To process response message received by the session layer.
Comments:  Response should correspond to current transaction in
           progress Or else it is thrown away.
******************************************************************/
static void SNReceiveResponse(void)
{
    TSAReceiveParam *tsaReceiveParamPtr;
    TSASendParam    *tsaSendParamPtr;
    TSPDUPtr         pduPtr;
    TransmitRecord  *xmitRecPtr;  /* Ptr to xmit rec (pri or nonpri). */
    APPReceiveParam *appReceiveParamPtr;
    APDU            *apduPtr;

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduPtr             = (TSPDUPtr) (tsaReceiveParamPtr + 1);

    /* Set the corresponding transmit pointer. */
    if (tsaReceiveParamPtr->priority)
    {
        xmitRecPtr      = &gp->priXmitRec;
        tsaSendParamPtr = QueueHead(&gp->tsaOutPriQ);
    }
    else
    {
        xmitRecPtr      = &gp->xmitRec;
        tsaSendParamPtr = QueueHead(&gp->tsaOutQ);
    }

    if (ValidateTrans(tsaReceiveParamPtr->priority, pduPtr->transNum)
            == TRANS_NOT_CURRENT)
    {
        /* Unsolicited response. Ignore it. */
        DebugMsg("SNReceiveResponse: Unsolicited response ignored.");
        DeQueue(&gp->tsaInQ);
        INCR_STATS(LcsLateAck);
        return;
    }

    /* Check if possible if the Response really corresponds to the
       transaction in progress. */
    if (xmitRecPtr->status != SESSION_TX ||
            xmitRecPtr->nwDestAddr.dmn.domainIndex !=
            tsaReceiveParamPtr->srcAddr.dmn.domainIndex)
    {
        /* Transmit record is not ours or the domain indices for
           the response and the transaction record do not match. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("SNReceiveResponse: Stale response discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    if (xmitRecPtr->nwDestAddr.addressMode == SUBNET_NODE &&
            (xmitRecPtr->nwDestAddr.addr.addr2a.subnet !=
             tsaReceiveParamPtr->srcAddr.subnetAddr.subnet ||
             xmitRecPtr->nwDestAddr.addr.addr2a.node !=
             tsaReceiveParamPtr->srcAddr.subnetAddr.node))
    {
        /* Source address of response does not match dest addr of
           transmit record. */
        /* Response does not seem to correspond to what we are expecting. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("SNReceiveResponse: Stale response discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    if (xmitRecPtr->nwDestAddr.addressMode == MULTICAST &&
            (tsaReceiveParamPtr->srcAddr.addressMode != MULTICAST_ACK ||
             xmitRecPtr->nwDestAddr.addr.addr1 !=
             tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.group))
    {
        /* MULTICAST message but not MULTICAST_ACK or the response's group
           # does not match that of xmitRecord. */
        /* Response does not seem to correspond to what we are expecting. */
        DeQueue(&gp->tsaInQ);
        DebugMsg("SNReceiveResponse: Stale response discarded.");
        INCR_STATS(LcsLateAck);
        return;
    }

    /* For brodacast messages, the subnet in the response can be 0 from
       unconfigured nodes. Another possibility is that the response might
       come through a router in a scenerio where the router replaces the
       subnet with its own. Thus the response can be valid even if the
       subnet does not match. So, we should skip this test. */


    /* Check if there is space in application layer's input queue. */
    if (QueueFull(&gp->appInQ))
    {
        return; /* Can't give the response yet. Come back later. */
    }
	
    appReceiveParamPtr = QueueTail(&gp->appInQ);
    apduPtr            = (APDU *)(appReceiveParamPtr + 1);

    /* Deliver the partial response to application layer. */
    appReceiveParamPtr->indication = MESSAGE;
    appReceiveParamPtr->srcAddr    = tsaReceiveParamPtr->srcAddr;
    appReceiveParamPtr->service    = RESPONSE;
    appReceiveParamPtr->priority   = tsaReceiveParamPtr->priority;
    appReceiveParamPtr->pduSize    = tsaReceiveParamPtr->pduSize - 1;
    appReceiveParamPtr->auth       = FALSE;
    appReceiveParamPtr->tag        = tsaSendParamPtr->tag;
	appReceiveParamPtr->proxy	   = tsaSendParamPtr->proxy;
	appReceiveParamPtr->proxyCount = tsaSendParamPtr->proxyCount;
	appReceiveParamPtr->proxyDone  = tsaSendParamPtr->proxyDone;
	appReceiveParamPtr->xcvrParams = tsaReceiveParamPtr->xcvrParams;
    memcpy(apduPtr, pduPtr->data, tsaReceiveParamPtr->pduSize - 1);

	// Special case for bidirectional signal strength responses.  These
	// are amended with the signal strength of this response message!
	if (xmitRecPtr->apdu->code.allBits == (ND_opcode_base|ND_QUERY_XCVR_BIDIR) &&
		appReceiveParamPtr->pduSize == 1+NUM_COMM_PARAMS)
	{
		memcpy(&apduPtr->data[NUM_COMM_PARAMS], &tsaReceiveParamPtr->xcvrParams, NUM_COMM_PARAMS);
		appReceiveParamPtr->pduSize += NUM_COMM_PARAMS;
	}
	
	// Once we deliver a proxy response, we don't want to deliver any other response or completion code because
	// this is a one shot deal.  If the transaction is not a request, then the proxy response occurs at the time
	// of the completion event (see ProcessLtepCompletion()).
	tsaSendParamPtr->proxyDone = true;

    /* Don't deliver the response to application yet. It could be
       a duplicate. */
    switch (xmitRecPtr->nwDestAddr.addressMode)
    {
    case BROADCAST:
        /* We deliver up to N responses to app layer where N =
           tsaSendParamPtr->destAddr.bcast.maxResponses.
           But we succeed if at least one resp is received. */
        if (xmitRecPtr->ackCount <
                tsaSendParamPtr->destAddr.bcast.maxResponses)
        {
            EnQueue(&gp->appInQ);
            xmitRecPtr->ackCount++;
            if (xmitRecPtr->ackCount ==
                    tsaSendParamPtr->destAddr.bcast.maxResponses)
            {
                TerminateTrans(tsaReceiveParamPtr->priority);
            }
        }
        /* else, we don't want this response. Ignore it. */
        break;
    case SUBNET_NODE:  /* Fall through. */
    case UNIQUE_NODE_ID:
        if (xmitRecPtr->ackCount == 0)
        {
            EnQueue(&gp->appInQ);
           xmitRecPtr->ackCount++; /* First response. */
            TerminateTrans(tsaReceiveParamPtr->priority);
        }
        /* else, it is a duplicate response. Ignore it. */
        break;
    case MULTICAST:
        /* Group request message. Check response address mode. */
        if (tsaReceiveParamPtr->srcAddr.addressMode
                != MULTICAST_ACK)
        {
            ErrorMsg("SNReceiveResponse: RESPONSE should be "
                     "MULTICAST_ACK.");
            break;
        }
        if (tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.member
                > MAX_GROUP_NUMBER)
        {
            ErrorMsg("SNReceiveResponse: Invalid group number.");
            break;
        }
        if (!xmitRecPtr->ackReceived[
                    tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.member])
        {
            DebugMsg("SNReceiveResp: A multicast resp delivered.");
            EnQueue(&gp->appInQ);
            xmitRecPtr->ackReceived[
                tsaReceiveParamPtr->srcAddr.ackNode.groupAddr.member]
            = TRUE;
            xmitRecPtr->ackCount++;
        }
        else
        {
            /* Else, it is a duplicate response. Ignore it */
            DebugMsg("SNReceiveResp: A duplicate multicast response ignored.");
        }
        if (xmitRecPtr->destCount == xmitRecPtr->ackCount)
        {
            TerminateTrans(tsaReceiveParamPtr->priority);
        }
        break;
    default:
        LCS_RecordError(BAD_ADDRESS_TYPE);
        ErrorMsg("SNReceiveResponse: Invalid address mode.");
    }

    DeQueue(&gp->tsaInQ);

    /* Restart the timer if the xmit rec is still active. */
    if (xmitRecPtr->status != UNUSED_TX)
    {
        MsTimerSet(&xmitRecPtr->xmitTimer, xmitRecPtr->xmitTimerValue);
    }

    return;
}

/*****************************************************************
Function:  ReceiveNewMsg
Returns:   None
Reference: None
Purpose:   To process either a new message received by transport
           (ACKD or UNACK_RPT) or session layer (REQUEST).
Comments:  The message could be new or a duplicate. If new, we
           need to allocate a RR. If not, we use the existing RR.
******************************************************************/
static void ReceiveNewMsg(Layer layerIn)
{
    TSAReceiveParam *tsaReceiveParamPtr;
    TSPDUPtr         pduPtr;
    uint16           recvTimerValue;
    int16            i;
    Boolean          initRR; /* Used to check if RR needs to init. */

    /* We have a new msg in TSA In queue. */

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduPtr          = (TSPDUPtr) (tsaReceiveParamPtr + 1);

    /* First retrieve associated RR, if it exists. */
    /* Since we can receive at most one message from a node
       (one for priority & one for non-priority), we can reuse
       receive records if the sender determines that a message
       transmission is over by transmitting a new message with a
       new transaction number or different service (i.e layer) or
       a new APDU. */

    i = RetrieveRR(tsaReceiveParamPtr->srcAddr,
                   tsaReceiveParamPtr->priority);
    if (i == -1)
    {
        /* No Associated RR found. We need to allocate a new one. */
        i = AllocateRR();
        if (i == -1)
        {
            /* Unable to allocate a new RR. */
            INCR_STATS(LcsRxTxFull);
            DebugMsg("ReceiveNewMsg: Unable to allocate RR. Msg is lost.");
            DeQueue(&gp->tsaInQ); /* Remove item from queue. */
            return;
        }
        /* We did allocate a new RR. We need to init this RR. */
        initRR = TRUE;
    }
    else if (gp->recvRec[i].transNum != pduPtr->transNum ||
             gp->recvRec[i].status   != (layerIn == TRANSPORT?TRANSPORT_RR:SESSION_RR) ||
             tsaReceiveParamPtr->pduSize - 1 !=
             gp->recvRec[i].apduSize  ||
             /* This check causes the contents of the APDU to always be factored into the duplicate
              * detection algorithm.  It is considered a requirement that the APDU contents be
              * factored into the duplicate detection algorithm only in the case where the
              * message is authenticated and the request is idempotent (because retries are
              * not re-authenticated).  Whether this is done by storing the entire APDU
              * for comparison or storing only the APDU length and a checksum of the APDU
              * data is implementation dependent.  If a checksum method is used, the
              * checksum should be at least 24 bits in length.  In the case where a
              * non-matching APDU vectors into an existing authenticated transaction, two
              * choices are possible:
              * a. Treat the message as if it were not a duplicate (as is done here).
              * b. Treat the message as a duplicate but mark it as not authenticated (if
              *    it is necessary to deliver the retry to the application - see
              *    "Saved Response Length" below). */
             memcmp(pduPtr->data, gp->recvRec[i].apdu,
                    gp->recvRec[i].apduSize) != 0)
    {
        /* Something does not match. Must be a new message from
           the sender. Let us reuse this RR, even if it is a
           request. If it is a request and response comes later,
           it won't match this record anyway due to reqid. */
        initRR = TRUE;
        /* If the old message was not deliverd, increment lost msg stat */
        if (gp->recvRec[i].transState != DELIVERED &&
                gp->recvRec[i].transState != DONE      &&
                gp->recvRec[i].transState != RESPONDED)
        {
            INCR_STATS(LcsLost);
        }
    }
    else
    {
        /* It is an existing RR with everything matching. */
        /* Nothing to do. We use the information in existing RR. */
        /* Set the alternate path bit based on new message so that
           acks, challenge etc will use the same path. Change only if
           it is not already using alternate path. */
        if (! gp->recvRec[i].altPath)
        {
            gp->recvRec[i].altPath = tsaReceiveParamPtr->altPath;
        }
        initRR = FALSE;
    }

    if (initRR)
    {
        if (layerIn == TRANSPORT)
        {
            gp->recvRec[i].status          = TRANSPORT_RR;
        }
        else
        {
            gp->recvRec[i].status          = SESSION_RR;
        }
        gp->recvRec[i].srcAddr         = tsaReceiveParamPtr->srcAddr;
        gp->recvRec[i].transNum        = pduPtr->transNum;
        gp->recvRec[i].transState      = JUST_RECEIVED;
        gp->recvRec[i].priority        = tsaReceiveParamPtr->priority;
        gp->recvRec[i].altPath         = tsaReceiveParamPtr->altPath;
        gp->recvRec[i].auth            = FALSE;
        gp->recvRec[i].reqId           = 0; /* Init to invalid reqid. */
		gp->recvRec[i].xcvrParams	   = tsaReceiveParamPtr->xcvrParams;
        if (layerIn == TRANSPORT)
        {
            gp->recvRec[i].serviceType     =
                pduPtr->pduMsgType == ACKD_MSG? ACKD: UNACK_RPT;
        }
        else
        {
            gp->recvRec[i].serviceType     = REQUEST;
            /* reqId can wrap around. Never use 0 as reqId so that
               a valid reqId will never match a receive record that
               does not correspond to corresponding request. */
            if (gp->reqId == 0)
            {
                gp->reqId++;
            }
            gp->recvRec[i].reqId           = gp->reqId++;
        }
        gp->recvRec[i].apduSize = tsaReceiveParamPtr->pduSize - 1;
        memcpy(gp->recvRec[i].apdu,
               pduPtr->data,
               gp->recvRec[i].apduSize); /* Store the APDU. */
        /* Compute the recvTimer value to be used. */
        recvTimerValue = ComputeRecvTimerValue(
                             tsaReceiveParamPtr->srcAddr.addressMode,
                             tsaReceiveParamPtr->srcAddr.group);
        MsTimerSet(&gp->recvRec[i].recvTimer, recvTimerValue);
    }

    DeQueue(&gp->tsaInQ); /* Remove item from queue. */

    /* Now, we have a RR for this message. */
    /* Determine if the msg needs authentication and store. */
    /* Allow authentication for UnAck_Rpt. */
    gp->recvRec[i].needAuth = pduPtr->auth;

    /* Authentication is performed if it is a new message or
       in the process of authenticating and it needs to be
       autheneticated. */
    if ((gp->recvRec[i].transState == JUST_RECEIVED ||
            gp->recvRec[i].transState == AUTHENTICATING) &&
            gp->recvRec[i].needAuth)
    {
        /* The message needs authentication. Initiate Challenge. */
        /* Or ReInitiate Challenge as the prev one is probably lost. */
        DebugMsg("ReceiveNewMsg: Initiating Challenge.");
        InitiateChallenge(i);
        return;
    }
	
    if (gp->recvRec[i].transState != DELIVERED &&
            gp->recvRec[i].transState != RESPONDED &&
            gp->recvRec[i].transState != DONE)
    {
        /* Deliver the message to the application layer. */
        Deliver(i);
    }
    else
    {
        DebugMsg("ReceiveNewMsg: Msg received was already delivered.");
    }

    if (layerIn == TRANSPORT)
    {
        if (gp->recvRec[i].transState  == DELIVERED &&
                gp->recvRec[i].serviceType == ACKD)
        {
            /* Compose and send the acknowledgement. */
            TPSendAck(i);
        }
    }
    else if (gp->recvRec[i].transState == RESPONDED)
    {
        /* Must be a request that was already responded. */
        /* We already have a resp. Simply send it. */
        SNSendResponse(i, FALSE, FALSE); /* It can't be a null response. */
    }
    else if (gp->recvRec[i].transState == DONE)
    {
        DebugMsg("ReceiveNewMsg: Nothing to do.");
    }
    return;
}

/*****************************************************************
Function:  ReceiveRem
Returns:   None
Reference: None
Purpose:   To process REMINDER and REM/MSG messages.
           We either do nothing or send an ack (for Transport)
           or a response(for Session) if possible.
Comments:  None
******************************************************************/
static void ReceiveRem(Layer layerIn)
{
    TSAReceiveParam *tsaReceiveParamPtr;
    TSPDUPtr         pduPtr;
    int16            i;
    APDU            *apduPtr;
    uint16           apduSize;
    uint8            length;
    uint8            member;
    uint8            mlistIndex, mlistOffset;
    Byte            *mlistPtr;
    uint16           recvTimerValue;

    /* We have a REMINDER or REM/MSG in TSA input queue. */

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduPtr             = (TSPDUPtr) (tsaReceiveParamPtr + 1);
    length             = pduPtr->data[0];

    if (pduPtr->pduMsgType == REMINDER_MSG)
    {
        apduSize = 0;
        apduPtr  = NULL;
    }
    else
    {
        /* Must be REM_MSG_MSG. */
        /* TSPDU header size before APDU is actual 1 byte header,
           1 byte length field and the length of M_List field. */
        apduSize = tsaReceiveParamPtr->pduSize - 2 - length;
        apduPtr  = (APDU *)&pduPtr->data[1 + length];
    }

    /* First retrieve the associated RR, if it exists. */
    i = RetrieveRR(tsaReceiveParamPtr->srcAddr,
                   tsaReceiveParamPtr->priority);


    /* Check if the REMINDER msg has an associated RR. */
    if (i == -1 && pduPtr->pduMsgType == REMINDER_MSG)
    {
        /* No associated RR. Discard this REMINDER msg. */
        DebugMsg("ReceiveRem: REMINDER msg has no associated RR");
        DeQueue(&gp->tsaInQ);
        return;
    }

    if (i != -1 &&
            (gp->recvRec[i].srcAddr.addressMode != MULTICAST ||
             gp->recvRec[i].transNum != pduPtr->transNum ||
             gp->recvRec[i].status != (layerIn == TRANSPORT?TRANSPORT_RR:SESSION_RR) ||
             (apduPtr && apduSize != gp->recvRec[i].apduSize) ||
             (apduPtr &&
              memcmp(apduPtr, gp->recvRec[i].apdu, apduSize) != 0))
       )

    {
        /* Associate RR does not match properly. Either it is not
           a multicast message or transNum does not match or
           APDU does not match. This is not OK for REMINDER_MSG.
           If it is however REM_MSG_MSG, we want to treat it as a
           new message. */
        if (pduPtr->pduMsgType == REMINDER_MSG)
        {
            DebugMsg("ReceiveRem: Associated RR does not match.");
            DeQueue(&gp->tsaInQ);
            return;
        }
        if (gp->recvRec[i].transState == DELIVERED ||
                gp->recvRec[i].transState == DONE)
        {
            /* Reuse this receive record. We will free this record and allocate
               a new one. */
            gp->recvRec[i].status = UNUSED_RR;
            i = -1;
        }
        else
        {
            i = -1; /* Allocate new one. */
        }
    }

    if (i != -1 && gp->recvRec[i].serviceType == UNACK_RPT)
    {
        if (pduPtr->pduMsgType == REMINDER_MSG)
        {
            /* We should not have gotton this REMINDER as the original
               message was UNACK_RPT. So, ignore this message. */
            DebugMsg("ReceiveRem: Original msg is UNACK_RPT.");
            DeQueue(&gp->tsaInQ);
            return;
        }
        /* Treat REM_MSG_MSG as new one. Fack this by setting i to -1. */
        i = -1;
    }

    if (i != -1)
    {
        /* We have already received the corresponding APDU. */
        /* Probably the ack or response sent earlier was lost. */
        /* Sent the ack or response again. */
        if (!IsGroupMember(gp->recvRec[i].srcAddr.dmn.domainIndex,
                           gp->recvRec[i].srcAddr.group,
                           &member))
        {
            /* We are not member of the group used for this msg.
               Strange! Ignore this msg too!
               Network layer should not have delivered such msg
               to upper layers. */
            DebugMsg("ReceiveRem: Strange! Not a group member!!.");
            DeQueue(&gp->tsaInQ);
            return;
        }
        mlistIndex  = member / 8;
        mlistOffset = member % 8;
        mlistPtr    = &pduPtr->data[1];
        if (
            length == 0
            ||
            mlistIndex >= length
            ||
            (mlistPtr[mlistIndex] & (1 << mlistOffset)) == 0
        )
        {
            /* We are asked to respond. Server did not get our ack or response. */

            /* If it is a REMINDER_MSG, it will be followed by the retry.
               So, we simply ignore it. */
            if (pduPtr->pduMsgType == REMINDER_MSG)
            {
                DeQueue(&gp->tsaInQ);
                return;
            }

            if (gp->recvRec[i].needAuth &&
                    (gp->recvRec[i].transState == JUST_RECEIVED ||
                     gp->recvRec[i].transState == AUTHENTICATING)
               )
            {
                /* Need authentication. Either a new msg or prev challenge
                   was lost. */
                DebugMsg("ReceiveRem: Initiating Challenge.");
                InitiateChallenge(i);
                DeQueue(&gp->tsaInQ);
                return;
            }
            if (gp->recvRec[i].transState != DELIVERED &&
                    gp->recvRec[i].transState != RESPONDED &&
                    gp->recvRec[i].transState != DONE)
            {
                /* Deliver the message to application layer. */
                Deliver(i);
            }

            /* Send ack or response. */
            if (gp->recvRec[i].transState  == DELIVERED ||
                    gp->recvRec[i].transState  == RESPONDED)
            {
                DeQueue(&gp->tsaInQ);
                if (layerIn == TRANSPORT)
                {
                    /* Compose and send the acknowledgement. */
                    TPSendAck(i);
                }
                else if (gp->recvRec[i].transState == RESPONDED)
                {
                    SNSendResponse(i, FALSE, FALSE); /* can't be null resp. */
                }
                else
                {
                    /* we are still waiting for a response. */
                    DebugMsg("ReceiveRem: Don't have response yet or have null response.");
                }
            }
            else
            {
                /* We are still waiting for resp & so can't respond
                   or in DONE state indicating server already received
                   the ack or response. */
                DebugMsg("ReceiveRem: Can't respond or no need to respond.");
                DeQueue(&gp->tsaInQ);
            }
        }
        else
        {
            /* We are not asked to acknowledege or respond. Ignore msg. */
            if (gp->recvRec[i].transState == DELIVERED ||
                    gp->recvRec[i].transState == RESPONDED)
            {
                /* Note down the fact that the ack or resp has been received */
                gp->recvRec[i].transState = DONE;
            }
            DebugMsg("ReceiveRem: We are not asked to ack or respond.");
            DeQueue(&gp->tsaInQ);
        }
        return;
    }

    /* Now we must have a REM_MSG_MSG with no associated RR. */
    /* REMINDER msg with no associated record was taken care of
       earlier. */
    i = AllocateRR();
    if (i == -1)
    {
        /* Unable to allocate a new RR. Give up. */
        return;
    }
    if (layerIn == TRANSPORT)
    {
        gp->recvRec[i].status          = TRANSPORT_RR;
    }
    else
    {
        gp->recvRec[i].status          = SESSION_RR;
    }
    gp->recvRec[i].srcAddr         = tsaReceiveParamPtr->srcAddr;
    gp->recvRec[i].transNum        = pduPtr->transNum;
    gp->recvRec[i].transState      = JUST_RECEIVED;
    gp->recvRec[i].priority        = tsaReceiveParamPtr->priority;
    gp->recvRec[i].altPath         = tsaReceiveParamPtr->altPath;
    gp->recvRec[i].auth            = FALSE; /* Not authenticated yet. */
    if (layerIn == TRANSPORT)
    {
        /* We have REM/MSG. So service type must be ACKD. */
        gp->recvRec[i].serviceType     = ACKD;
    }
    else
    {
        gp->recvRec[i].serviceType     = REQUEST;
        /* reqId can wrap around. Never use 0 as reqId so that
           a valid reqId will never match a receive record that
           does not correspond to corresponding request. */
        if (gp->reqId == 0)
        {
            gp->reqId++;
        }
        gp->recvRec[i].reqId           = gp->reqId++;
    }
    gp->recvRec[i].apduSize = apduSize;
    memcpy(gp->recvRec[i].apdu, apduPtr, apduSize);
    /* Compute the recvTimer value to be used. */
    recvTimerValue =
        ComputeRecvTimerValue(
            tsaReceiveParamPtr->srcAddr.addressMode,
            tsaReceiveParamPtr->srcAddr.group
        );
    DeQueue(&gp->tsaInQ); /* Remove the item from queue. */

    MsTimerSet(&gp->recvRec[i].recvTimer, recvTimerValue);

	gp->recvRec[i].needAuth = pduPtr->auth;

    /* Now, we have a RR for this message. */
    if (gp->recvRec[i].needAuth)
    {
        /* The message needs authentication. Initiate Challenge. */
        DebugMsg("ReceiveRem: Initiating Challenge.");
        InitiateChallenge(i);
        return;
    }

    /* Deliver the message to the application layer. */
    Deliver(i);

    if (layerIn == TRANSPORT)
    {
        if (gp->recvRec[i].transState  == DELIVERED)
        {
            /* Compose and send the acknowledgement. */
            TPSendAck(i);
        }
    }
    else
    {
        /* We just delivered. Can't have a response yet. */
    }
    return;
}

/*****************************************************************
Function:  TPSendAck
Returns:   None
Reference: None
Purpose:   To Send an ACK message for ACKD msg received by the
           transport layer. If there is no space in network
           output queue, then nothing is sent.
Comments:  None.
******************************************************************/
static void TPSendAck(uint16 rrIndexIn)
{
    NWSendParam     *nwSendParamPtr;   /* Ptr to NW Send Param. */
    TSPDUPtr          pduPtr;          /* Ptr to TPDU sent.     */
    Queue           *nwQueuePtr;
    DestinationAddress    destAddr;

    if (gp->recvRec[rrIndexIn].priority)
    {
        nwQueuePtr = &gp->nwOutPriQ;
    }
    else
    {
        nwQueuePtr = &gp->nwOutQ;
    }

    if (QueueFull(nwQueuePtr))
    {
        return; /* Can't send the acknowledgement now. */
    }

    nwSendParamPtr = QueueTail(nwQueuePtr);
    pduPtr        = (TSPDUPtr) (nwSendParamPtr + 1);

    /* Form the TPDU directly in the network layer's queue. */
    pduPtr->auth     = FALSE; /* Acks are not authenticated. */
    pduPtr->pduMsgType  = ACK_MSG;
    pduPtr->transNum = gp->recvRec[rrIndexIn].transNum;

    /* Fill in the details for the network layer. */

    /* First, form sender's (i.e our) address. */
    destAddr.dmn = gp->recvRec[rrIndexIn].srcAddr.dmn;

    /* We send unicast ACK or multicast ACK depending on msg recvd. */
    if (gp->recvRec[rrIndexIn].srcAddr.addressMode == MULTICAST)
    {
        /* Send ACK in address format 2b. */
        destAddr.addressMode = MULTICAST_ACK;
        destAddr.addr.addr2b.subnetAddr =
            gp->recvRec[rrIndexIn].srcAddr.subnetAddr;
        destAddr.addr.addr2b.groupAddr.group  =
            gp->recvRec[rrIndexIn].srcAddr.group;
        if (!IsGroupMember(destAddr.dmn.domainIndex, destAddr.addr.addr2b.groupAddr.group,
                           &destAddr.addr.addr2b.groupAddr.member))
        {
            ErrorMsg("SendACKTPDU: Ack msg for a non-existing group.");
            return;
        }
    }
    else
    {
        /* Send Ack in address format 2a. */
        destAddr.addressMode = SUBNET_NODE;
        destAddr.addr.addr2a =
            gp->recvRec[rrIndexIn].srcAddr.subnetAddr;
    }

    nwSendParamPtr->dropIfUnconfigured = FALSE; /* acks are not dropped */
    nwSendParamPtr->destAddr = destAddr;
    nwSendParamPtr->pduType  = TPDU_TYPE;
    nwSendParamPtr->deltaBL  = 0;
    nwSendParamPtr->altPath  = gp->recvRec[rrIndexIn].altPath;
    nwSendParamPtr->pduSize  = 1;

    DebugMsg("TPSendAck. Sending an ACK.");
    EnQueue(nwQueuePtr);
}

/*****************************************************************
Function:  SNSendResponse
Returns:   None
Reference: None
Purpose:   To send a response message for REQUEST msg received by the
           session layer. If there is no space in network
           output queue, then nothing is sent.
           If it is a null response, then the receive record is freed
           and no response goes out.
Comments:  This function is called only when the application layer
           has already given the response. (Or else how can this
           function know about how to respond anyway?)
******************************************************************/
static void SNSendResponse(uint16 rrIndexIn, Boolean nullResponse, Boolean flexResponse)
{
    NWSendParam       *nwSendParamPtr;   /* Ptr to NW Send Param. */
    TSPDUPtr           pduPtr;          /* Ptr to TPDU sent.     */
    Queue             *nwQueuePtr;
    DestinationAddress destAddr;

    if (gp->recvRec[rrIndexIn].priority)
    {
        nwQueuePtr = &gp->nwOutPriQ;
    }
    else
    {
        nwQueuePtr = &gp->nwOutQ;
    }

    if (QueueFull(nwQueuePtr) && !nullResponse)
    {
        return; /* Can't sent the response now. */
    }

    if (gp->recvRec[rrIndexIn].transState != RESPONDED &&
            gp->recvRec[rrIndexIn].transState != DONE)
    {
        ErrorMsg("SNSendResponse: How can I be called without "
                 " atleast one resp?");
        return;
    }

    if (nullResponse)
    {
        /* Set state to DONE and do not send the response. */
        DebugMsg("SendResponse: Null response. Nothing goes out.");
        gp->recvRec[rrIndexIn].transState = DONE;
        return;
    }

    nwSendParamPtr = QueueTail(nwQueuePtr);
    pduPtr        = (TSPDUPtr) (nwSendParamPtr + 1);

    if (gp->nwOutBufSize < gp->recvRec[rrIndexIn].rspSize + 1)
    {
        ErrorMsg("SNSendResponse: Network buf too small for response.");
        return;
    }
    pduPtr->auth     = FALSE; /* Responses are not authenticated. */
    pduPtr->pduMsgType  = RESPONSE_MSG;
    pduPtr->transNum = gp->recvRec[rrIndexIn].transNum;
    /* Copy the existing response from RR */
    if (gp->recvRec[rrIndexIn].rspSize <=
            DecodeBufferSize((uint8) eep->readOnlyData.appOutBufSize))
    {
        /* *** Saved Response Length *** */
        /* This implementation saves the entire response in the receive transaction record.  Thus,
         * if and when a retry of the request is received, the response can be re-transmitted without
         * re-delivering the request to the application for construction of a response.  It is acceptable
         * to save only responses of certain length (e.g., as little as 1 byte) in order to minimize
         * RAM use.  Under these conditions, a retry of a request which illicited a response of length
         * greater than that minimium length is considered to be idempotent (can be safely executed
         * again) and may be re-delivered to the application for reformulation of the response.  It
         * is also required that under these conditions that the application be informed of the
         * duplicate nature of the request in case the application chooses to treat the request as
         * non-idempotent by saving the response for retransmission without recomputation.  In this
         * case a key must also be provided for the application to use to map the request to a saved
         * response. */
        memcpy(pduPtr->data, gp->recvRec[rrIndexIn].response,
               gp->recvRec[rrIndexIn].rspSize);
    }
    else
    {
        DebugMsg("SNSendResponse: Discarding a long response.");
        return;
    }

    /* Fill in the details for the network layer. */
    destAddr.dmn = gp->recvRec[rrIndexIn].srcAddr.dmn;

	if (flexResponse)
	{
	  	// Make it look like a flex domain response so that we use 0/0 for the source subnet/node.
		destAddr.dmn.domainIndex = FLEX_DOMAIN;
	}
	
    /* We send unicast response or multicast response depending on
       the request received. */
    if (gp->recvRec[rrIndexIn].srcAddr.addressMode == MULTICAST)
    {
        /* Send the response in address format 2b. */
        destAddr.addressMode = MULTICAST_ACK;
        destAddr.addr.addr2b.subnetAddr =
            gp->recvRec[rrIndexIn].srcAddr.subnetAddr;
        destAddr.addr.addr2b.groupAddr.group  =
            gp->recvRec[rrIndexIn].srcAddr.group;
        if (!IsGroupMember(destAddr.dmn.domainIndex, destAddr.addr.addr2b.groupAddr.group,
                           &destAddr.addr.addr2b.groupAddr.member))
        {
            ErrorMsg("SNSendResponse: Response for a non-existing group.");
            return;
        }
    }
    else
    {
        /* Send response in address format 2a. */
        destAddr.addressMode = SUBNET_NODE;
        destAddr.addr.addr2a =
            gp->recvRec[rrIndexIn].srcAddr.subnetAddr;
    }

    nwSendParamPtr->dropIfUnconfigured = FALSE; /* responses are not dropped */
    nwSendParamPtr->destAddr = destAddr;
    nwSendParamPtr->pduType  = SPDU_TYPE;
    nwSendParamPtr->deltaBL  = 0;
    nwSendParamPtr->altPath  = gp->recvRec[rrIndexIn].altPath;
    nwSendParamPtr->pduSize  = gp->recvRec[rrIndexIn].rspSize + 1;

    DebugMsg("SNSendResponse: Sending a response.");
    EnQueue(nwQueuePtr);
}

/*****************************************************************
Function:  Deliver
Returns:   None
Reference: None
Purpose:   To deliver a message received by tranport or session layer
           to the application layer.
Comments:  layerIn is not needed as it is not used.
******************************************************************/
static void Deliver(uint16 rrIndexIn)
{
    APPReceiveParam    *appReceiveParamPtr;
    char               *apduInPtr;
    uint16              i = rrIndexIn; /* Use i instead of rrIndexIn. */

    if (gp->recvRec[i].needAuth && gp->recvRec[i].transState != AUTHENTICATED)
    {
        /* The message needs authentication but was not authenticated. */
        gp->recvRec[i].transState = DONE;
        return;
    }

    if (QueueFull(&gp->appInQ))
    {
        /* We can wait, but then we may not be able to guarantee delivery in
           sequence from a given source node. So, it is better to drop the
           message and let the retry mechanism take care of redelivery. */
	    DebugMsg("Deliver: No space for the message");
        INCR_STATS(LcsLost);
        return;
    }

    appReceiveParamPtr = QueueTail(&gp->appInQ);
    apduInPtr          = (char *)(appReceiveParamPtr + 1);

    appReceiveParamPtr->indication = MESSAGE;
    appReceiveParamPtr->srcAddr    = gp->recvRec[i].srcAddr;
    appReceiveParamPtr->service    = gp->recvRec[i].serviceType;
    appReceiveParamPtr->priority   = gp->recvRec[i].priority;
    appReceiveParamPtr->altPath    = gp->recvRec[i].altPath;
    appReceiveParamPtr->auth       = gp->recvRec[i].auth;
    appReceiveParamPtr->pduSize    = gp->recvRec[i].apduSize;
    appReceiveParamPtr->reqId      = gp->recvRec[i].reqId;
	appReceiveParamPtr->xcvrParams = gp->recvRec[i].xcvrParams;
	appReceiveParamPtr->proxy	   = FALSE;

    if (gp->appInBufSize < gp->recvRec[i].apduSize)
    {
        ErrorMsg("Deliver: APDU size too big");
        /* We can never deliver this APDU. So, make it look like
           it was delivered so that it can be discarded. We don't
           want to send any ACK or response for this message. */
        LCS_RecordError(WRITE_PAST_END_OF_APPL_BUFFER);
        return;
    }
    /* Now it should be safe to do memcpy. */
    memcpy(apduInPtr, gp->recvRec[i].apdu, gp->recvRec[i].apduSize);
    EnQueue(&gp->appInQ);
    gp->recvRec[i].transState      = DELIVERED;
    DebugMsg("Deliver: Packet has been delivered to the application layer.");
}

/*****************************************************************
Function:  RetrieveRR
Returns:   Index of RR Table that matches given input parameters.
           -1 if none exists.
Reference: None
Purpose:   To retrieve the receive record from the RR table
           that corresponds to a message received. For matching,
           we use priority, domainIndex, addressMode, and source address.
           We also match subnet if the message is broadcast
           or group if the message is multicast.
Comments:  None
******************************************************************/
static int16 RetrieveRR(SourceAddress srcAddrIn,
                        Boolean priorityIn)
{
    int16 i;

    /* Search through all the receive records for a match. */

    for (i = 0; i < gp->recvRecCnt; i++)
    {
        if (
            priorityIn == gp->recvRec[i].priority
            &&
            /* Destination subnet/node match is based on domainIndex. */
            srcAddrIn.dmn.domainIndex == gp->recvRec[i].srcAddr.dmn.domainIndex
            &&
            (srcAddrIn.addressMode == gp->recvRec[i].srcAddr.addressMode)
            &&
            /* Source node address should always match. */
            (memcmp(&srcAddrIn.subnetAddr,
                    &gp->recvRec[i].srcAddr.subnetAddr,
                    sizeof(SubnetAddress)) == 0)
            &&
            /* Make sure BROADCAST address matches for broadcast messages. */
            ( srcAddrIn.addressMode != BROADCAST ||
              srcAddrIn.broadcastSubnet == gp->recvRec[i].srcAddr.broadcastSubnet )
            &&
            /* Make sure MULTICAST address matches for multicast messages. */
            ( srcAddrIn.addressMode != MULTICAST ||
              srcAddrIn.group == gp->recvRec[i].srcAddr.group )
        )

        {
            break;
        }
    }

    if (i == gp->recvRecCnt)
    {
        return(-1); /* Matching RR was not found. */
    }

    return(i); /* Found matching RR. */
}


/*****************************************************************
Function:  AllocateRR
Returns:   Index of RR table that can be used for a new msg.
Reference: None
Purpose:   To find an index in the RR table that is UNUSED.
Comments:  None
******************************************************************/
static int16 AllocateRR(void)
{
    int16 i;

    /* First search for one which is unused. */
    for (i = 0; i < gp->recvRecCnt; i++)
    {
        if (gp->recvRec[i].status == UNUSED_RR)
        {
            return(i); /* Found one that is unused. */
        }
    }

    return(-1);
}


/*****************************************************************
Function:  FindRR
Returns:   1 if RR is found with specified ID.
Purpose:   To find an RR with the specified ID.
Comments:  None
******************************************************************/
static Bool FindRR(RequestId id, uint16 *pIndex)
{
    Bool result = true;
    uint16 i;
	for (i = 0; i < gp->recvRecCnt; i++)
	{
		if (gp->recvRec[i].status == SESSION_RR &&
			gp->recvRec[i].reqId == id)
		{
			break;
		}
	}
	if (i == gp->recvRecCnt ||
			gp->recvRec[i].serviceType != REQUEST ||
			gp->recvRec[i].transState != DELIVERED)
	{
	    result = false;
	}
	*pIndex = i;
	return result;
}


/*****************************************************************
Function:  ComputeRecvTimerValue
Returns:   Receive Timer value in milli seconds.
Reference: None
Purpose:   To compute the value to be used for receive timer
           based on the address format of the received message
           and group id.
Comments:  None
******************************************************************/
static uint16 ComputeRecvTimerValue(AddrMode      addrModeIn,
                                    MulticastAddress groupIdIn)
{
    uint16 i, max = 0, temp;

    if (addrModeIn == UNIQUE_NODE_ID)
    {
        return(NGTIMER_SPCL_VAL);
    }
    if (addrModeIn == MULTICAST)
    {
        /* Search through the address table to find the receiver timer val. */
        /* If there is more than one entry with the same group,
           use the one with the max rcv timer value */
        for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++)
        {
            if (eep->addrTable[i].addrFormat >= 128 &&
                    eep->addrTable[i].groupEntry.groupID == groupIdIn)
            {
                /* Group format match */
                temp = DecodeRcvTimer((uint8)
                                      eep->addrTable[i].groupEntry.rcvTimer);
                /* Using the maximum receive timer for the group is not required.  It
                 * is acceptable to use the receive timer for the first group entry
                 * found in the table. */
                if (temp > max)
                {
                    max = temp;
                }
            }
        }
        return(max);
    }
    /* All other messages use non-group timer value. */
    return(DecodeRcvTimer((uint8) eep->configData.nonGroupTimer));
}

/*****************************************************************
Function:  SNSend
Returns:   None
Reference: Section 8, Protocol Spec.
Purpose:   To implement Send algorithm for the session layer.
           If there is anything to be sent by the session layer,
           this function will process that message and sends it.
           If there is any ongoing message, it might need some
           processing such as retry, timer expiry etc. This
           function will process such events too.
Comments:  Update the priority transmit timer, if it exists.
           Update the non-priority transmit timer, if it exists.
           If the priority transmit timer expired then
              process this event.
           else if there is priority message to be sent and there is space
                in priority queue of the network layer then
              process the priority message.
           else if the non-priority transmit timer expired then
              process that event.
           else if there is non-priority message to be sent and there is space
                in non-priority queue of network layer then
              process the non-priority message.
           else
                nothing to do. return.
Note:
******************************************************************/
void SNSend(void)
{
    TSASendParam    *tsaSendParamPtr;
    uint16           i;

    /* Delay SNSend after power-up or external reset. */
    if (SendBlocked())
    {
        return; /* Do nothing */
    }

    /**************************************************
      Send response, if any, first. Responses are not like transactions
      and hence it is better to send it first.
    **************************************************/
    if (!QueueEmpty(&gp->tsaRespQ) &&
            !QueueFull(&gp->nwOutQ))
    {
        /* We have a response to be sent out.
           Make sure the response is not stale.
           Response should have a reqId.
           If there is no RR for this reqId, then it is stale.
           Throw the response away, if it is stale. */
        tsaSendParamPtr = QueueHead(&gp->tsaRespQ);

        if (tsaSendParamPtr->service != RESPONSE)
        {
            /* Throw away this message. Only responses are
               allowed in this queue. */
            DebugMsg("SNSend: Response queue is only for responses.");
            DeQueue(&gp->tsaRespQ);
            return;
        }

        /* Search for the associated RR for this response. */
		if (!FindRR(tsaSendParamPtr->reqId, &i))
		{
            /* Stale or duplicate response. Ignore it. */
            DeQueue(&gp->tsaRespQ);
            DebugMsg("SNSend: Discarding stale or duplicate response.");
            INCR_STATS(LcsLateAck);
            return;
        }
        /* Copy the response to the receive record and send response. */
        gp->recvRec[i].rspSize = tsaSendParamPtr->apduSize;
        /* Resp should fit in response field as it was found to be
           fit in the TSA queue. */
        memcpy(gp->recvRec[i].response,
               (char *) (tsaSendParamPtr + 1),
               gp->recvRec[i].rspSize);
        gp->recvRec[i].transState = RESPONDED;
        SNSendResponse(i, tsaSendParamPtr->nullResponse, tsaSendParamPtr->flexResponse);
         DeQueue(&gp->tsaRespQ);
        return;
    }

    /***************************************************
      Priority transmit timer expired event.
     **************************************************/
    if (gp->priXmitRec.status == SESSION_TX && !MsTimerRunning(&gp->priXmitRec.xmitTimer))
    {
        XmitTimerExpiration(SESSION, TRUE);
        return;
    }
    /***************************************************
      Send a new priority message event.
     **************************************************/
    else if (gp->priXmitRec.status == UNUSED_TX &&
             ! QueueEmpty(&gp->tsaOutPriQ)      &&
             ! QueueFull(&gp->nwOutPriQ) )
    {
        SendNewMsg(SESSION, TRUE);
        return;
    }
    /***************************************************
      Non-priority timer expired event.
      *************************************************/
    else if (gp->xmitRec.status == SESSION_TX && !MsTimerRunning(&gp->xmitRec.xmitTimer))
    {
        XmitTimerExpiration(SESSION, FALSE);
    }
    /***************************************************
      Send a new non-priority message.
     **************************************************/
    else if (gp->xmitRec.status == UNUSED_TX &&
             ! QueueEmpty(&gp->tsaOutQ)      &&
             ! QueueFull(&gp->nwOutQ) )
    {
        SendNewMsg(SESSION, FALSE);
    }
    else
    {
        /* Either there is no work or there is no space. */
        return;
    }

    return;

}

/*****************************************************************
Function:  SNReceive
Returns:   None
Reference: None
Purpose:   To receive and process incoming SPDU's.
Comments:  None
******************************************************************/
void SNReceive(void)
{
    TSAReceiveParam *tsaReceiveParamPtr;/* Param in tsa input queue.  */
    TSPDUPtr         spduInPtr;         /* Pointer to SPDU received. */
    int16            i;

    /* Update Receive Timers, if they do exist */
    for (i = 0; i < gp->recvRecCnt; i++)
    {
        if (gp->recvRec[i].status == SESSION_RR)
        {
            if (MsTimerExpired(&gp->recvRec[i].recvTimer))
            {
                /* Timer expired. Release the receive record. */
                DebugMsg("SNReceive: Receive timer expired.");
                gp->recvRec[i].status = UNUSED_RR; /* Release the RR */
            }
        }
    }
	
    /* Check if there is SPDU to be processed. */
    if (QueueEmpty(&gp->tsaInQ))
    {
        /* There is nothing to process. */
        return;
    }

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    spduInPtr          = (TSPDUPtr) ((char *)tsaReceiveParamPtr
                                     + sizeof(TSAReceiveParam));

    /* Check if the PDU is for session layer. If not, we are done. */
    if (tsaReceiveParamPtr->pduType != SPDU_TYPE)
    {
        return;
    }

	if (tsaReceiveParamPtr->pduSize < 1)
	{
        ErrorMsg("SNReceive: Illegal PDU size.");
		DeQueue(&gp->tsaInQ);
		return;
	}

    /* We have a SPDU to be processed. Check the type and
       process accordingly. */
    switch (spduInPtr->pduMsgType)
    {
    case RESPONSE_MSG:
        SNReceiveResponse();
        return;
    case REQUEST_MSG:
        ReceiveNewMsg(SESSION);
        return;
    case REMINDER_MSG:
        /* Fall through. */
    case REM_MSG_MSG:
        ReceiveRem(SESSION);
        return;
    default:
        LCS_RecordError(UNKNOWN_PDU);
        ErrorMsg("SNReceive: Unknown SPDU type received.");
        DeQueue(&gp->tsaInQ);
    }

    return;
}

/*****************************************************************
Function:  AuthSend
Returns:   None
Reference: None
Purpose:   To send challenges that are pending in receive records.
           These are challenges that could not be sent earlier due
           to unavailable space in network queue.
Comments:  None
******************************************************************/
void AuthSend(void)
{
    int16 i;

    /* Only thing the authentication layer can do here is to check if any
       challenges need to be sent. */
    for (i = 0; i < gp->recvRecCnt; i++)
    {
        if (gp->recvRec[i].status != UNUSED_RR &&
                gp->recvRec[i].needAuth &&
                gp->recvRec[i].transState == JUST_RECEIVED)
        {
            InitiateChallenge(i);
        }
    }

    return;
}

/*****************************************************************
Function:  AuthReceive
Returns:   None
Reference: None
Purpose:   To receive an incoming AUTH_PDU packet and process.
Comments:  None
******************************************************************/

void AuthReceive(void)
{
    TSAReceiveParam *tsaReceiveParamPtr; /* Parameter in tsa input queue. */
    AuthPDUPtr       pduInPtr;           /* Ptr to PDU received. */

    /* Check if there is AuthPDU to be processed. */
    if (QueueEmpty(&gp->tsaInQ))
    {
        /* There is nothing to process. */
        return;
    }

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduInPtr           = (AuthPDUPtr) (tsaReceiveParamPtr + 1);

    /* Check if the PDU is for session layer. If not, we are done. */
    if (tsaReceiveParamPtr->pduType != AUTHPDU_TYPE)
    {
        return;
    }

    /* We have a AuthPDU to be processed. Check the type and
       process accordingly. */
    if (pduInPtr->pduMsgType == CHALLENGE_MSG || pduInPtr->pduMsgType == CHALLENGE_OMA_MSG)
    {
        SendReplyToChallenge((Boolean)(pduInPtr->pduMsgType == CHALLENGE_OMA_MSG));
    }
    else if (pduInPtr->pduMsgType == REPLY_MSG || pduInPtr->pduMsgType == REPLY_OMA_MSG)
    {
        ProcessReply((Boolean)(pduInPtr->pduMsgType == REPLY_OMA_MSG));
    }
    else
    {
        ErrorMsg("AuthReceive: Unknown pdu type received.");
        LCS_RecordError(UNKNOWN_PDU);
        DeQueue(&gp->tsaInQ);
        return;
    }

}

/*****************************************************************
Function:  InitiateChallenge
Returns:   None
Reference: None
Purpose:   To send a challenge message for a message received.
Comments:  None
******************************************************************/
static void InitiateChallenge(uint16 rrIndexIn)
{
    AuthPDUPtr       pduOutPtr;          /* Ptr to PDU sent. */
    NWSendParam     *nwSendParamPtr;
    Queue           *nwQueuePtr;
    Byte             randomValue[8];
    uint8            i;

    if (gp->recvRec[rrIndexIn].priority)
    {
        nwQueuePtr = &gp->nwOutPriQ;
    }
    else
    {
        nwQueuePtr = &gp->nwOutQ;
    }

    if (QueueFull(nwQueuePtr))
    {
        /* No space to send challenge anyway. Come back later. */
        return;
    }

    nwSendParamPtr = QueueTail(nwQueuePtr);
    pduOutPtr      = (AuthPDU *)(nwSendParamPtr + 1);

    /* First compute the random bytes to be sent */
    if (gp->recvRec[rrIndexIn].transState != AUTHENTICATING)
    {
        /* Generate random number only the first time we are called
           for this message. subsequent calls use the same rand. */
        for (i = 0; i < 8; i++)
        {
            randomValue[i] = (Byte)((gp->prevChallenge[i] + rand() % 256 +
                                     TMR_GetCurrentTime() % 256) % 256);
            gp->prevChallenge[i] = randomValue[i];
        }
        memcpy(gp->recvRec[rrIndexIn].rand, randomValue, 8); /* Save */
    }

    /* Form the challenge AuthPDU. */
    pduOutPtr->fmt = addrModeToFmt[gp->recvRec[rrIndexIn].srcAddr.addressMode];
    if (gp->recvRec[rrIndexIn].srcAddr.addressMode == MULTICAST)
    {
        /* For Multicast message, send the group info with AuthPDU. */
        nwSendParamPtr->pduSize = 10;
        pduOutPtr->group    = gp->recvRec[rrIndexIn].srcAddr.group;
    }
    else
    {
        nwSendParamPtr->pduSize = 9;
    }
    pduOutPtr->pduMsgType  = AuthOma() ? CHALLENGE_OMA_MSG : CHALLENGE_MSG;
    pduOutPtr->transNum = gp->recvRec[rrIndexIn].transNum;
    memcpy(pduOutPtr->value.randomBytes,
           gp->recvRec[rrIndexIn].rand,
           8);

    /* Now fill in the NWSendParam structure. */
    nwSendParamPtr->dropIfUnconfigured = FALSE; /* Challenges are not dropped */
    /* Challenge must have come from a particular node.
       Use format 2b for multicast and format 2a for others. */
    nwSendParamPtr->destAddr.dmn = gp->recvRec[rrIndexIn].srcAddr.dmn;
    if (gp->recvRec[rrIndexIn].srcAddr.addressMode == MULTICAST)
    {
        nwSendParamPtr->destAddr.addressMode = MULTICAST_ACK;
        nwSendParamPtr->destAddr.addr.addr2b.subnetAddr =
            gp->recvRec[rrIndexIn].srcAddr.subnetAddr;
        nwSendParamPtr->destAddr.addr.addr2b.groupAddr.group =
            gp->recvRec[rrIndexIn].srcAddr.group;
        if (!IsGroupMember(gp->recvRec[rrIndexIn].srcAddr.dmn.domainIndex,
                           gp->recvRec[rrIndexIn].srcAddr.group,
                           &nwSendParamPtr->destAddr.addr.addr2b.groupAddr.member))
        {
            ErrorMsg("InitiateChallenge: Strange: We are not member of group???.");
            return; /* Don't challenge. This case should not happen. */
        }
    }
    else
    {
        nwSendParamPtr->destAddr.addressMode = SUBNET_NODE;
        nwSendParamPtr->destAddr.addr.addr2a =
            gp->recvRec[rrIndexIn].srcAddr.subnetAddr;
    }

    nwSendParamPtr->pduType = AUTHPDU_TYPE;
    nwSendParamPtr->deltaBL = 0;
    nwSendParamPtr->altPath = gp->recvRec[rrIndexIn].altPath;
    gp->recvRec[rrIndexIn].transState = AUTHENTICATING;
    EnQueue(nwQueuePtr);
    DebugMsg("InitiateChallenge: Sending a challenge.");
    return;
}

/*****************************************************************
Function:  ReplyOmaDestAddr
Returns:   None
Reference: None
Purpose:   This routine sets up the OMA destination address on receipt of a reply
Comments:  None
******************************************************************/
void ReplyOmaDestAddr(SourceAddress* pSrcAddr, AuthPDUPtr pPdu, OmaAddress* pOmaAddr)
{
	memset(pOmaAddr, 0xFF, sizeof(OmaAddress));

	switch (pPdu->fmt)
	{
		case AF_UNIQUE_ID:
			memcpy(pOmaAddr->physical.uniqueId, eep->readOnlyData.uniqueNodeId, UNIQUE_NODE_ID_LEN);
			break;
		case AF_SUBNETNODE:
			pOmaAddr->logical.addr.snode.subnet = eep->domainTable[pSrcAddr->dmn.domainIndex].subnet;
			pOmaAddr->logical.addr.snode.selField = 0;
			pOmaAddr->logical.addr.snode.node = eep->domainTable[pSrcAddr->dmn.domainIndex].node;
			break;                ;
		case AF_MULTICAST:
			pOmaAddr->logical.addr.group = pPdu->group;
			break;
	}

	if (pPdu->fmt != AF_UNIQUE_ID)
	{
		// We use the domain table version because the algorithm always uses 6 bytes and we need
		// predictable contents.  This assumes that the NM assigns consistent padding to the unused
		// bytes of the domain ID!!!!
		DomainStruct *pDom = &eep->domainTable[pSrcAddr->dmn.domainIndex];
		pOmaAddr->logical.domainLen = pDom->len;
		memcpy(pOmaAddr->logical.domainId, pDom->domainId, DOMAIN_ID_LEN);
	}
}


/*****************************************************************
Function:  ChallengeOmaDestAddr
Returns:   None
Reference: None
Purpose:   This routine sets up the OMA destination address on receipt of a challenge
Comments:  None
******************************************************************/
void ChallengeOmaDestAddr(DestinationAddress* pAddr, OmaAddress* pOmaAddr)
{
	memset(pOmaAddr, 0xFF, sizeof(OmaAddress));

	switch (pAddr->addressMode)
	{
		case UNIQUE_NODE_ID:
			memcpy(pOmaAddr->physical.uniqueId, pAddr->addr.addr3.uniqueId, UNIQUE_NODE_ID_LEN);
			break;
		case SUBNET_NODE:
			pOmaAddr->logical.addr.snode = pAddr->addr.addr2a;
			pOmaAddr->logical.addr.snode.selField = 0;
			break;
		case MULTICAST:
			pOmaAddr->logical.addr.group = pAddr->addr.addr1;
			break;
		default:
		    // These shouldn't occur
			break;
	}

	if (pAddr->addressMode != UNIQUE_NODE_ID)
	{
		// We use the domain table version because the algorithm always uses 6 bytes and we need
		// predictable contents.  This assumes that the NM assigns consistent padding to the unused
		// bytes of the domain ID!!!!
		DomainStruct *pDom = &eep->domainTable[pAddr->dmn.domainIndex];
		pOmaAddr->logical.domainLen = pDom->len;
		memcpy(pOmaAddr->logical.domainId, pDom->domainId, DOMAIN_ID_LEN);
	}
}


/*****************************************************************
Function:  SendReplyToChallenge
Returns:   None
Reference: None
Purpose:   To send a reply to a challenge just received.
Comments:  None
******************************************************************/
static void SendReplyToChallenge(Boolean useOma)
{
    TSAReceiveParam *tsaReceiveParamPtr; /* Parameter in tsa input queue. */
    AuthPDUPtr       pduInPtr;           /* Ptr to PDU received. */
    AuthPDUPtr       pduOutPtr;          /* Ptr to PDU sent.     */
    NWSendParam     *nwSendParamPtr;
    Queue           *nwQueuePtr;
    TransmitRecord  *xmitRecPtr;
    Byte             encryptValue[8];
	OmaAddress		 omaDest;

	tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduInPtr           = (AuthPDUPtr) (tsaReceiveParamPtr + 1);

    if (tsaReceiveParamPtr->srcAddr.dmn.domainIndex == FLEX_DOMAIN)
    {
        /* Challenge was received in flex domain which is not
           possible. We don't initiate challenge in flex domain.
           Ignore the challenge. */
        DeQueue(&gp->tsaInQ);
        return;
    }

    if (tsaReceiveParamPtr->priority)
    {
        nwQueuePtr = &gp->nwOutPriQ;
        xmitRecPtr = &gp->priXmitRec;
    }
    else
    {
        nwQueuePtr = &gp->nwOutQ;
        xmitRecPtr = &gp->xmitRec;
    }


    /* Make sure that this challenge for current transaction
       in progress. If not, it is stale. Ignore it. */
    /* Also do not reply if we did not set auth bit or
       the group value in challenge msg, if present,
       does not match the one in transmit record. */
    if (
        xmitRecPtr->status == UNUSED_TX ||
        ! xmitRecPtr->auth ||
        pduInPtr->transNum != xmitRecPtr->transNum ||
        addrFmtToMode[pduInPtr->fmt] != xmitRecPtr->nwDestAddr.addressMode ||
        (pduInPtr->fmt == 1 &&
         xmitRecPtr->nwDestAddr.addr.addr1 != pduInPtr->group)
    )
    {
        DeQueue(&gp->tsaInQ);
        INCR_STATS(LcsLateAck);
        return;
    }

    if (QueueFull(nwQueuePtr))
    {
        /* No Space to send reply anyway. Come back later. */
        return;
    }
    nwSendParamPtr = QueueTail(nwQueuePtr);
    pduOutPtr      = (AuthPDU *)(nwSendParamPtr + 1);

	// If we are not using the alternate key, then fill it in from the domain
	if (!xmitRecPtr->altKey.altKey)
	{
		int i;
		int domIdx = useOma ? 0 : tsaReceiveParamPtr->srcAddr.dmn.domainIndex;
		for (i=0; i<2 && domIdx<2; i++)
		{
			memcpy(xmitRecPtr->altKey.altKeyValue[i], eep->domainTable[domIdx].key, sizeof(xmitRecPtr->altKey.altKeyValue));
			domIdx++;
		}
	}

	// Set up OMA destination address
	ChallengeOmaDestAddr(&xmitRecPtr->nwDestAddr, &omaDest);

    /* First compute the cryptoBytes to be sent. */
    Encrypt(pduInPtr->value.randomBytes,
            xmitRecPtr->apdu, xmitRecPtr->apduSize,
            (Byte*)xmitRecPtr->altKey.altKeyValue,
            encryptValue, useOma, &omaDest);

    /* Form the reply AuthPDU. */
    pduOutPtr->fmt = pduInPtr->fmt;
    if (pduInPtr->fmt == 1)
    {
        pduOutPtr->group = pduInPtr->group;
        nwSendParamPtr->pduSize = 10;
    }
    else
    {
        nwSendParamPtr->pduSize = 9;
    }
    pduOutPtr->pduMsgType = useOma ? REPLY_OMA_MSG : REPLY_MSG;
    pduOutPtr->transNum = xmitRecPtr->transNum;
    memcpy(pduOutPtr->value.cryptoBytes, encryptValue, 8);

    /* Now fill in the NWSendParam structure */
    nwSendParamPtr->dropIfUnconfigured = FALSE; /* Replies are not dropped */

    /* Use subnet addressing to send to that node unless we received the challenge with src node 0 from a NID formatted request. */
	if (tsaReceiveParamPtr->srcAddr.subnetAddr.node == 0 &&
		pduInPtr->fmt == 3)
	{
		nwSendParamPtr->destAddr = xmitRecPtr->nwDestAddr;
	}
	else
	{
		nwSendParamPtr->destAddr.addressMode = SUBNET_NODE;
	    nwSendParamPtr->destAddr.addr.addr2a =
		    tsaReceiveParamPtr->srcAddr.subnetAddr;
	}
    nwSendParamPtr->destAddr.dmn.domainIndex =
        tsaReceiveParamPtr->srcAddr.dmn.domainIndex;
    nwSendParamPtr->pduType = AUTHPDU_TYPE;
    nwSendParamPtr->deltaBL = 0;
    nwSendParamPtr->altPath = tsaReceiveParamPtr->altPath;
    EnQueue(nwQueuePtr);
    DeQueue(&gp->tsaInQ);
    DebugMsg("SendReply: Sending a reply msg.");
    /* Restart the transmit timer. */
    MsTimerSet(&xmitRecPtr->xmitTimer, xmitRecPtr->xmitTimerValue);
    return;
}

/*****************************************************************
Function:  ProcessReply
Returns:   None
Reference: None
Purpose:   To process a reply msg received to see if it meets the
           challenge sent earlier.
Comments:  None
******************************************************************/
static void ProcessReply(Boolean useOma)
{
    TSAReceiveParam *tsaReceiveParamPtr;  /* Parameter in tsa input queue. */
    AuthPDUPtr       pduInPtr;            /* Ptr to PDU received. */
    int16            i;
    Byte             encryptValue[8];
    Byte             domainIndex;
	Byte			 authKey[2][AUTH_KEY_LEN];
	OmaAddress		 omaAddr;

    tsaReceiveParamPtr = QueueHead(&gp->tsaInQ);
    pduInPtr           = (AuthPDUPtr) (tsaReceiveParamPtr + 1);

    /* If the reply indicates that the original address format is
       multicast, then make sure that the receive record matches
       this group number. Force it so that RetriveRR will do the
       match. */
    tsaReceiveParamPtr->srcAddr.group = pduInPtr->group;

    /* The address format of the reply will be different from the address
       format of the original message. The original address format is in
       the Reply PDU. Change the addressmode in source address before
       calling RetrieveRR. */
    tsaReceiveParamPtr->srcAddr.addressMode = addrFmtToMode[pduInPtr->fmt];

    /* Retrieve the associated record.

       If the original address format is broadcast, we don't have
       the subnet number in the current reply(as the reply itself does
       not use broadcast address mode) and hence we can't match
       subnet number. So, we don't support authentication in
       broadcast mode. */

    i = RetrieveRR(tsaReceiveParamPtr->srcAddr,
                   tsaReceiveParamPtr->priority);

    if (i == -1)
    {
        /* We did not find an associated RR. Ignore reply. */
        DeQueue(&gp->tsaInQ);
        INCR_STATS(LcsLateAck);
        return;
    }

    /* We have a RR. */

    /* Check if we have already authenticated. */
    if (gp->recvRec[i].transState != AUTHENTICATING)
    {
        /* Ignore this reply as it is probably a duplicate. */
        DeQueue(&gp->tsaInQ);
        return;
    }

    /* Check if other things match to make sure that we are
       authenticating the right RR. */
    if (pduInPtr->fmt == 1 &&
            pduInPtr->group != gp->recvRec[i].srcAddr.group)
    {
        /* Group in AuthPDU does not match one in RR. */
        DebugMsg("ProcessReply: Group does not match. Ignore reply.");
        DeQueue(&gp->tsaInQ);
        return;
    }

    if (pduInPtr->transNum != gp->recvRec[i].transNum)
    {
        /* Transaction Number does not match. Ignore reply. */
        DebugMsg("ProcessReply: TId does not match. Ignore reply.");
        DeQueue(&gp->tsaInQ);
        return;
    }

    /* Now check if the reply matches our encryption. */
    /* First compute the value of E(range, apdu) */

    domainIndex = gp->recvRec[i].srcAddr.dmn.domainIndex;

	if (useOma)
	{
		memcpy(authKey[0], eep->domainTable[0].key, sizeof(authKey[0]));
		memcpy(authKey[1], eep->domainTable[1].key, sizeof(authKey[1]));
	}
	else
	{
		int domIdx = domainIndex;
		if (domIdx == FLEX_DOMAIN)
		{
			// For flex domain auth with traditional, take the key from the first configured domain.
			domIdx = eep->domainTable[0].invalid ? 1 : 0;
		}
		memcpy(authKey[0], eep->domainTable[domIdx].key, sizeof(authKey[0]));
	}

	ReplyOmaDestAddr(&gp->recvRec[i].srcAddr, pduInPtr, &omaAddr);

    Encrypt(gp->recvRec[i].rand,
            gp->recvRec[i].apdu, gp->recvRec[i].apduSize,
			(Byte*)authKey, encryptValue, useOma, &omaAddr);

    if (memcmp(encryptValue, pduInPtr->value.cryptoBytes, 8) == 0)
    {
        /* Matches */
        DebugMsg("ProcessReply: Reply matches.");
        gp->recvRec[i].auth = TRUE;
    }
    else
    {
        DebugMsg("ProcessReply: Reply Does not match");
        LCS_RecordError(AUTHENTICATION_MISMATCH);
        gp->recvRec[i].auth = FALSE;
    }
    gp->recvRec[i].transState = AUTHENTICATED;

    /* Deliver the message now to the application layer. */
    Deliver(i);

    /* Send ack message if it is for transport layer and ackd msg. */
    if (gp->recvRec[i].status == TRANSPORT_RR &&
            gp->recvRec[i].serviceType == ACKD    &&
            gp->recvRec[i].transState == DELIVERED)
    {
        TPSendAck(i);
    }

    DeQueue(&gp->tsaInQ);
    return;
}


/*****************************************************************
Function:  Encrypt
Returns:   None
Reference: Protocol Spec (Online version)
Purpose:   To compute the encryption key based on authentication
           key in the domain table, the APDU, and the random
           number given.
Comments:  None
******************************************************************/
static void Encrypt(Byte randIn[], APDU *apduIn, uint16 apduSizeIn,
                    Byte *pKey, Byte encryptValueOut[], Boolean isOma, OmaAddress* pOmaDest)
{
    Int8om i,j,k;
    Byte m, n;
    Byte *apduBytes = (Byte*)&apduIn->code;
    uint16 apduSize = apduSizeIn;

	Byte omaBuffer[MAX_DATA_SIZE + sizeof(OmaAddress)];
    int keyLength;
    int keyIterations;  // Number of iterations over the key bytes.  
                        // For classic keys, this is the same as
                        // the key length, but for OMA, its 1 1/2 times
                        // the key length - byte 0-11 followed by 0-5.

    if (isOma)
    {   
		// Include OMA destination data with the message data.
		memcpy(omaBuffer, pOmaDest, sizeof(OmaAddress));
        memcpy(&omaBuffer[sizeof(OmaAddress)], apduBytes, apduSize);
        apduBytes = omaBuffer;
        apduSize += sizeof(OmaAddress);
        keyLength = OMA_KEY_LEN;
        keyIterations = OMA_KEY_LEN + OMA_KEY_LEN/2;
    }
    else
    {
        keyLength = AUTH_KEY_LEN;
        keyIterations = keyLength;  // Once over the entire key.
    }

    for (i = 0; i < 8; i++)
    {
        encryptValueOut[i] = randIn[i];
    }

    while (apduSize > 0)
    {
        for (i = 0; i < keyIterations; i++)
        {
            for (j = 7; j >= 0; j--)
            {
                k = (j + 1) % 8;
                if (apduSize > 0)
                {
                    m = apduBytes[--apduSize];
                }
                else
                {
                    m = 0;
                }
                n = ~(encryptValueOut[j] + j);
                if (pKey[i % keyLength] & (1 << (7 - j)) )
                {
                    encryptValueOut[j] =
                        encryptValueOut[k] + m + ((n << 1) + (n >> 7));
                }
                else
                {
                    encryptValueOut[j] =
                        encryptValueOut[k] + m - ((n >> 1) + (n << 7));
                }
            }
        }
	}
}

/*------------------------End of tsa.c------------------------*/
