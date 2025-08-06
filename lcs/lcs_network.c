//
// lcs_network.c
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

/*******************************************************************************
     Reference:        ISO/IEC 14908-1, Section 6.

       Purpose:        Implement the LON network layer.
*******************************************************************************/

/*------------------------------------------------------------------------------
  Section: Includes
  ------------------------------------------------------------------------------*/
#include "lcs/lcs_network.h"

/*------------------------------------------------------------------------------
  Section: Constants
  ------------------------------------------------------------------------------*/
#ifdef SECURITY_II
#define LT_AES_GCM_PACKET_CODE      0x4e
#endif

/*------------------------------------------------------------------------------
  Section: Type Definitions
  ------------------------------------------------------------------------------*/

/*******************************************************************************
   IzotByte data[1] is used so that variable data has address assigned by
   the compiler. Once we know the size of the record, we will use
   data[0], data[1], etc.

   data[0] is source subnet.
   data[1] is source node.
   Based on the addrFmt field and 1st bit of data[1], the rest of
   the data array is used appropriately.
*******************************************************************************/
typedef struct __attribute__ ((packed))
{
    BITS4(protocolVersion,  2,
          pduType,          2,
          addrFmt,          2,
          domainLength,     2)
    IzotByte       data[MAX_DATA_SIZE/*1*/];                /* Variable part */
} NPDU;

/*------------------------------------------------------------------------------
  Section: Globals
  ------------------------------------------------------------------------------*/
extern IzotByte NmAuth;

/*------------------------------------------------------------------------------
  Section: Local Function Prototypes
  ------------------------------------------------------------------------------*/
static IzotByte DecodeDomainLength(IzotByte lengthCode);
Status EncodeDomainLength(IzotByte length, IzotByte* pValue);

/*------------------------------------------------------------------------------
  Section: Function Definitions
  ------------------------------------------------------------------------------*/

/*******************************************************************************
Function:  NWReset
Returns:   None
Reference: None
Purpose:   To initialize the queues used by the network layer.
Comments:  None.
*******************************************************************************/
void   NWReset(void)
{
    IzotUbits16 queueItemSize;

    /* Allocate and initialize the input queue. */
    gp->nwInBufSize   = DecodeBufferSize(NW_IN_BUF_SIZE); //1280
    gp->nwInQCnt      = DecodeBufferCnt((IzotByte)IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_INBUF_CNT));
    queueItemSize     = gp->nwInBufSize + sizeof(NWReceiveParam);


    if (QueueInit(&gp->nwInQ, queueItemSize, gp->nwInQCnt) != SUCCESS)
    {
        gp->resetOk = FALSE;
        return;
    }

    /* Allocate and initialize the output queue. */
    gp->nwOutBufSize  = DecodeBufferSize(IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUTBUF_SIZE)) + 12; //1280
    gp->nwOutQCnt     = DecodeBufferCnt((IzotByte)IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUTBUF_CNT));
    queueItemSize     = gp->nwOutBufSize + sizeof(NWSendParam);
    if (gp->nwOutQCnt < 2)
    {
        gp->resetOk = FALSE;
        return;
    }

    if (QueueInit(&gp->nwOutQ, queueItemSize, gp->nwOutQCnt)
            != SUCCESS)
    {
        DBG_vPrintf(TRUE, "NWReset: Unable to init the output queue.\n");
        gp->resetOk = FALSE;
        return;
    }

    /* Allocate and initialize the priority output queue. */
    gp->nwOutPriBufSize = gp->nwOutBufSize; //1280
    gp->nwOutPriQCnt    = DecodeBufferCnt((IzotByte)IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUT_PRICNT));
    queueItemSize       = gp->nwOutPriBufSize + sizeof(NWSendParam);

    if (gp->nwOutPriQCnt < 1)
    {
        gp->resetOk = FALSE;
        return;
    }

    if (QueueInit(&gp->nwOutPriQ, queueItemSize, gp->nwOutPriQCnt) != SUCCESS)
    {
        DBG_vPrintf(TRUE, "NWReset: Unable to init the priority output queue.\n");
        gp->resetOk = FALSE;
        return;
    }

    return;
}


/*******************************************************************************
Function:  NWSendTerminate
Returns:   None
Purpose:   To clean up the current NW output queue and send a completion event
              if necessary.
*******************************************************************************/
void NWSendTerminate(void)
{
    NWSendParam  *nwSendParamPtr; /* Param in nwOutQ or nwPriOutQ.   */
    nwSendParamPtr = QueueHead(gp->nwCurrent);
    /* Send completion event if it was an APDU */
    if (nwSendParamPtr->pduType == APDU_TYPE)
    {
        APPReceiveParam *appReceiveParamPtr;
        appReceiveParamPtr = QueueTail(&gp->appCeRspInQ);
        appReceiveParamPtr->indication = COMPLETION;
        appReceiveParamPtr->success    = FALSE;
        appReceiveParamPtr->tag        = nwSendParamPtr->tag;
        appReceiveParamPtr->proxy      = nwSendParamPtr->proxy;
        appReceiveParamPtr->proxyCount = 0;    // This technically isn't correct but this case should never happen on a working system.
        appReceiveParamPtr->proxyDone  = FALSE;
        EnQueue(&gp->appCeRspInQ);
    }
    DeQueue(gp->nwCurrent);
}

/*******************************************************************************
Function:  NWSend
Returns:   None
Reference: None. No algorithms in protocol specification.
Purpose:   To send outgoing PDUS (APDU or SPDU or TPDU or AuthPDU)
           waiting on the queue (pri or nonpri) for network layer.
           Network layer forms the NPDU and the parameters for
           sending the NPDU and writes to the queue for the
           link/mac layer.
Comments:  Network buffer size is guaranteed to be at least
           20 bytes long as the encoding table's (page 9-9
           Technology Device Data Book Rev 3) minimum value is 20.
           The NPDU's header's worst case size is 16. So, we are
           OK. No need to check for space when writing headers.
*******************************************************************************/
void   NWSend(void)
{
    NWSendParam     *nwSendParamPtr; /* Param in nwOutQ or nwPriOutQ.   */
    LKSendParam     *lkSendParamPtr; /* Param in lkOutQ or lkPriOutQ.   */
    APPReceiveParam *appReceiveParamPtr;
    NPDU            *npduPtr;        /* Pointer to NPDU being formed.   */
    IzotByte        *pduPtr;         /* Pointer to PDU etc being sent.  */
    IzotByte         j;              /* For temporary use.              */
    IzotUbits16      npduSize;       /* Size of NPDU formed.            */
    IzotByte         numDomains;     /* Number of domains for this node.*/
    IzotByte         domainLength;   /* Length of domain value sent.    */
#ifdef SECURITY_II
	IzotByte         priority;
#endif
    /* Check if there is work to do and set pointers */
    if (!QueueEmpty(&gp->nwOutPriQ) && !QueueFull(&gp->lkOutPriQ))
    {
        /* Process priority message if there is one and it can be processed. */
        gp->nwCurrent    = &gp->nwOutPriQ;
        gp->lkCurrent   = &gp->lkOutPriQ;
#ifdef SECURITY_II
		priority = TRUE;
#endif
    }
    else if (!QueueEmpty(&gp->nwOutQ) && !QueueFull(&gp->lkOutQ))
    {
        /* Process non-priority message if there is one and can be processed */
        gp->nwCurrent    = &gp->nwOutQ;
        gp->lkCurrent   = &gp->lkOutQ;
#ifdef SECURITY_II
		priority = FALSE;
#endif
    }
    else
    {
        /* Either there is nothing to send or there is no space in link layer */
        return;
    }

    nwSendParamPtr  = QueueHead(gp->nwCurrent);
    lkSendParamPtr  = QueueTail(gp->lkCurrent);

    /* For application layer messages, we need to give completion event
       using the tag given. This is for consistency with transport/session
       layers. Thus completion events are streamlined in one place in
       application layer rather than lots of places. */
    if (nwSendParamPtr->pduType == APDU_TYPE && QueueFull(&gp->appCeRspInQ))
    {
        /* Can't deliver the indication. Wait until we can send indication */
        DBG_vPrintf(TRUE, "NWSend: No room for indication");
        return;
    }

    /* Process the waiting PDU, form the NPDU and send it */
    /* ptr to APDU or TPDU or SPDU or AuthPDU. */
    pduPtr  = (IzotByte *)(nwSendParamPtr + 1);
    /* ptr to NPDU constructed. */
    npduPtr = (NPDU *)(lkSendParamPtr + 1);
    /* Write the NPDU header. */
    npduPtr->protocolVersion = nwSendParamPtr->version;
    npduPtr->pduType         = nwSendParamPtr->pduType;
    switch (nwSendParamPtr->destAddr.addressMode)
    {
    case AM_BROADCAST:
        npduPtr->addrFmt = 0;
        break;
    case AM_MULTICAST:
        npduPtr->addrFmt = 1;
        break;
    case AM_SUBNET_NODE:
    case AM_MULTICAST_ACK:
        npduPtr->addrFmt = 2;
        break;
    case AM_UNIQUE_NODE_ID:
        npduPtr->addrFmt = 3;
        break;
    default:
        DBG_vPrintf(TRUE, "NWSend: Unknown address mode.\n");
        /* Discard the packet as addrmode is wrong */
        LCS_RecordError(IzotBadAddressType);
        NWSendTerminate();
        return;
    }
    /* Write the domain length. */
    /* First determine the number of domains for this node */
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS) == 1)
    {
        numDomains = 2;
    }
    else
    {
        numDomains = 1;
    }

    /* if a node is in in unconfigured state and the message is not in flex
       domain, then we discard the message. We should not use the domain table
       in unconfigured state, irrespective of whether they are valid or not.
       However, we allow acks, response, challenge and reply. The field
       dropIfUnconfigured indicates whether this check is done or not. */
    if (nwSendParamPtr->dropIfUnconfigured && nwSendParamPtr->destAddr.dmn.domainIndex != FLEX_DOMAIN &&
    NodeUnConfigured())
    {
        /* drop this packet. */
        NWSendTerminate();
        return;
    }

    /* If the domain used is not in use, it cannot send any packet */
    if (nwSendParamPtr->destAddr.dmn.domainIndex < numDomains &&
     IZOT_GET_ATTRIBUTE(eep->domainTable[nwSendParamPtr->destAddr.dmn.domainIndex], IZOT_DOMAIN_INVALID))
    {
        if (!nwSendParamPtr->dropIfUnconfigured)
        {
            /* It is not ACK, RESP etc. Don't log domain error in this case.
               LNS might use join domain to leave a domain with IzotServiceAcknowledged. So, the
               ACK send by the transport layer will be in an invalid domain
               but should be ignored. */

            /* Discard the packet as the domain table entry is not in use */
            LCS_RecordError(IzotInvalidDomain);
        }
        DBG_vPrintf(TRUE, "NWSend: NWSendTerminate: Domain used is not in use\n");
        NWSendTerminate();
        return;
    }
    /* Use destAddr to determine the domain and write it.
       compute and store domainLength and domainId for later use. */
    if (nwSendParamPtr->destAddr.dmn.domainIndex < numDomains)
    {
        IzotByte selField = 0;
        /* Determine the selField value. */
        /* Only AM_MULTICAST_ACK has selector field as 0. For all others, it is 1. */
        if (nwSendParamPtr->destAddr.addressMode != AM_MULTICAST_ACK)
        {
            selField = 1;
        }
        /* One of this node's domains. */

        nwSendParamPtr->destAddr.dmn.domainLen = 
        IZOT_GET_ATTRIBUTE(eep->domainTable[nwSendParamPtr->destAddr.dmn.domainIndex], IZOT_DOMAIN_ID_LENGTH);
        memcpy(nwSendParamPtr->destAddr.dmn.domainId, eep->domainTable[nwSendParamPtr->destAddr.dmn.domainIndex].Id, 
        nwSendParamPtr->destAddr.dmn.domainLen);

        npduPtr->data[0] = eep->domainTable[nwSendParamPtr->destAddr.dmn.domainIndex].Subnet;
        npduPtr->data[1] = selField << 7 | 
                       IZOT_GET_ATTRIBUTE(eep->domainTable[nwSendParamPtr->destAddr.dmn.domainIndex], IZOT_DOMAIN_NODE);
    }
    else
    {
        npduPtr->data[0] = 0; /* It is 0 for flex domain response. */
        npduPtr->data[1] = 1 << 7; /* SrcNode is 0. */
    }

    if (EncodeDomainLength(nwSendParamPtr->destAddr.dmn.domainLen, &domainLength) == FAILURE)
    {
        /* Protocol specification indicates that domainLength has to be
           one of the above values. If not, it is a bad value. */
        DBG_vPrintf(TRUE, "NWSend: Domain length is not valid.\n");
        LCS_RecordError(IzotInvalidDomain);
        NWSendTerminate();
        return;
    }
    npduPtr->domainLength = domainLength;

    // Restore unencoded value
    domainLength = nwSendParamPtr->destAddr.dmn.domainLen;

    /* Write the destination address. */
    /* Set j to the index for writing the domain field. */
    switch (nwSendParamPtr->destAddr.addressMode)
    {
    case AM_BROADCAST:
        npduPtr->data[2] = nwSendParamPtr->destAddr.addr.addr0.SubnetId;
        j = 3;
        break;
    case AM_MULTICAST:
        npduPtr->data[2] = nwSendParamPtr->destAddr.addr.addr1.GroupId;
        j = 3;
        break;
    case AM_SUBNET_NODE:
        IZOT_SET_ATTRIBUTE(nwSendParamPtr->destAddr.addr.addr2a, IZOT_RECEIVESN_SELFIELD, 1);
        memcpy(&npduPtr->data[2], &nwSendParamPtr->destAddr.addr.addr2a, 2);
        j = 4;
        break;
    case AM_MULTICAST_ACK:
        IZOT_SET_ATTRIBUTE(nwSendParamPtr->destAddr.addr.addr2b.subnetAddr, IZOT_RECEIVESN_SELFIELD, 1);
        memcpy(&npduPtr->data[2], &nwSendParamPtr->destAddr.addr.addr2b, 4);
        j = 6;
        break;
    case AM_UNIQUE_NODE_ID:
        memcpy(&npduPtr->data[2], &nwSendParamPtr->destAddr.addr.addr3, 7);
        j = 9;
        break;
    default:
        DBG_vPrintf(TRUE, "NWSend: Unknown address format.\n");
        /* Discard the packet as the address Mode is wrong. */
        LCS_RecordError(IzotBadAddressType);
        NWSendTerminate();
        return;
    }

    /* Now, j has the index of data field in which domain goes. */
    /* Write the domain. We saved this information earlier. */

    memcpy(&npduPtr->data[j], nwSendParamPtr->destAddr.dmn.domainId, domainLength);

    j += domainLength;

    /* Write the enclosed PDU. */
    if (1 + j + nwSendParamPtr->pduSize > gp->nwOutBufSize)
    {
        /* Discard the packet as it is too long. */
        LCS_RecordError(IzotWritePastEndOfNetBuffer);
        NWSendTerminate();
        return;
    }

    memcpy(&npduPtr->data[j], pduPtr, nwSendParamPtr->pduSize);
    /* NPDU size is header_size + enclosed PDU size. */
    npduSize = 1 + j + nwSendParamPtr->pduSize;

    /* Write the parameters for the link layer. */
    lkSendParamPtr->deltaBL = nwSendParamPtr->deltaBL;
    lkSendParamPtr->altPath = nwSendParamPtr->altPath;
    lkSendParamPtr->pduSize = npduSize;
    lkSendParamPtr->DomainIndex = nwSendParamPtr->destAddr.dmn.domainIndex;

    /* Update both queues. */
    DeQueue(gp->nwCurrent);
#ifdef SECURITY_II
	Byte serviceType = -1;
	Byte pduTypeValue = (pduPtr[0] & 0x70) >> 4;
	Byte apduIndex = 1;
	if (nwSendParamPtr->pduType == APDU_TYPE) {
		serviceType = IzotServiceUnacknowledged;
		apduIndex = 0;
	} else if (nwSendParamPtr->pduType == TPDU_TYPE) {
		if (pduTypeValue == 0) {
			serviceType = IzotServiceAcknowledged;
		} else if (pduTypeValue == 1) {
			serviceType = IzotServiceRepeated;
		}
		if (nwSendParamPtr->version == LsProtocolModeEnhanced) {
			apduIndex = 2;
		}
	} else if (nwSendParamPtr->pduType == SPDU_TYPE) {
		if (pduTypeValue == 0) {
			serviceType = IzotServiceRequest;
		} else if (pduTypeValue == 2) {
			serviceType = IzotServiceResponse;
		}
		if (nwSendParamPtr->version == LsProtocolModeEnhanced) {
			apduIndex = 2;
		}
	}

	if (pduPtr[apduIndex] == LT_AES_GCM_PACKET_CODE) {
		if (processAesGcm(false, serviceType, priority,
				&pduPtr[apduIndex], // Original APDU with no AES-GCM header included
				nwSendParamPtr->pduSize - apduIndex, // Length of original APDU
				(Byte *)npduPtr, j+apduIndex)) {
			return;
		}
	}
#endif
    EnQueue(gp->lkCurrent);
    DBG_vPrintf(TRUE,"NWSend: Sending a packet.");
    
    INCR_STATS(LcsL3Tx);

    /* Send completion event if it was an APDU */
    if (nwSendParamPtr->pduType == APDU_TYPE)
    {
        appReceiveParamPtr = QueueTail(&gp->appCeRspInQ);
        appReceiveParamPtr->indication = COMPLETION;
        appReceiveParamPtr->success    = TRUE;
        appReceiveParamPtr->tag        = nwSendParamPtr->tag;
        appReceiveParamPtr->proxy      = nwSendParamPtr->proxy;
        appReceiveParamPtr->proxyCount = 0;
        appReceiveParamPtr->proxyDone  = FALSE;
        EnQueue(&gp->appCeRspInQ);
    }

    return;
}

/*******************************************************************************
Function:  NWReceive
Returns:   None
Reference: None. No receive algorithms in protocol specification.
Purpose:   To receive packets waiting for the network layer
           from the link layer. The NPDU is retrieved, processed
           and the enclosed PDU is sent to the proper destination queue.
           If the NPDU is not for this node, it is discarded.
Comments:  Discard packets originated from this node itself.
           It might receive such packets in the presence of repeaters.

           Discard packets in which version field is not correct.

           Discard packets which match a domain but not
           subnet or group.

           When a node is not in configured state and not in
           hard-offline state, it can only receive broadcast
           or matching Unique Node ID messages. In such a case,
           the domain on which it is received need not match its
           own domain. If it does not match, then the message is
           said to be received in a flex domain.
*******************************************************************************/
void   NWReceive(void)
{
    NWReceiveParam      *nwReceiveParamPtr; /* Param in gp->nwInQ.   */
    APPReceiveParam     *appReceiveParamPtr;
    TSAReceiveParam     *tsaReceiveParamPtr;
    SourceAddress        srcAddr;    /* Address of source node.      */
    NPDU                *npduPtr;    /* ptr to NPDU being received.  */
    IzotByte            *pduPtr;     /* ptr to item in target queue. */
    IzotUbits16          pduSize;    /* Size of enclosed PDU.        */
    IzotByte             flexDomain; /* TRUE => NPDU in flexdomain.  */
    IzotByte             numDomains; /* # of domains of this node.   */
    IzotByte             domainLength;  /* Domain length             */
    IzotByte             domainId[DOMAIN_ID_LEN]; /* Temp.           */
    IzotByte             uniqueNodeId[IZOT_UNIQUE_ID_LENGTH]; /* Temp.  */
    IzotReceiveSubnetNode   destAddr;   /* Temp.                        */
    IzotByte                j;          /* Temp.                        */
    
    /* First, check if we have any packets to process. */
    if (QueueEmpty(&gp->nwInQ))
    {
        return; /* Nothing to process. */
    }

    /* Until we determine what type of PDU we have, we cannot
       check for space availability in destination queue.
       Also, it is possible that the NPDU may very well be
       discarded. */

    /* Set the pointer to NPDU in nwInQ. */
    nwReceiveParamPtr = QueueHead(&gp->nwInQ);
    npduPtr           = (NPDU *)(nwReceiveParamPtr + 1);
    
    /* Determine the source address. */
    memcpy(&srcAddr.subnetAddr, npduPtr->data, 2);

    /* Determine the destination address used and set srcAddr properly. */
    /* For AM_MULTICAST and AM_MULTICAST_ACK address modes, the
       group and/or member values are copied into srcAddr.group
       or srcAddr.ackNode. For AM_BROADCAST and AM_SUBNET_NODE address
       modes, destAddr is used to store subnet and/or node
       values. For AM_UNIQUE_NODE_ID, uniqueNodeId is used & subnet is ignored */
    /* Also, set j to domain field's index. */
    switch (npduPtr->addrFmt)
    {
    case 0:
        srcAddr.addressMode = AM_BROADCAST;
        destAddr.Subnet   = npduPtr->data[2];
        j = 3;
        break;
    case 1:
        srcAddr.addressMode = AM_MULTICAST;
        srcAddr.group.GroupId = npduPtr->data[2];
        j = 3;
        break;
    case 2:
        if (IZOT_GET_ATTRIBUTE(srcAddr.subnetAddr, 
        IZOT_RECEIVESN_SELFIELD) == 1)
        {
            srcAddr.addressMode = AM_SUBNET_NODE;
            memcpy(&destAddr, &npduPtr->data[2], 2);
            j = 4;
        }
        else
        {
            srcAddr.addressMode = AM_MULTICAST_ACK;
            memcpy(&destAddr, &npduPtr->data[2], 2);
            memcpy(&srcAddr.ackNode.subnetAddr, &destAddr, 2);
            memcpy(&srcAddr.ackNode.groupAddr, &npduPtr->data[4], 2);
            j = 6;
        }
        break;
    case 3:
        srcAddr.addressMode = AM_UNIQUE_NODE_ID;
        destAddr.Subnet = npduPtr->data[2]; /* Routing Purpose */
        memcpy(uniqueNodeId, &npduPtr->data[3], IZOT_UNIQUE_ID_LENGTH);
        j = 3 + IZOT_UNIQUE_ID_LENGTH;
        break;
    default:
        /* Discard it as the address format is wrong. */
        DBG_vPrintf(TRUE, "NWReceive: Unknown addFmt.\n");
        LCS_RecordError(IzotBadAddressType);
        DeQueue(&gp->nwInQ);
        return;
    }

    /* Determine the domain. */
    domainLength = DecodeDomainLength(npduPtr->domainLength);

    if (domainLength != 0 && domainLength != 1 && domainLength != 3)
    {
        DBG_vPrintf(TRUE, "NWReceive: Domain length is not valid.\n");
        LCS_RecordError(IzotInvalidDomain);
        /* Discard the packet as the domain length is invalid. */
        DeQueue(&gp->nwInQ);
        return;
    }

    /* domainLength is good. Safe to use memcpy now. */
    memcpy(domainId, &npduPtr->data[j], domainLength);

    j += domainLength; /* Now j points to enclosed PDU. */

    /* Determine the number of domains for this node. */
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS))
    {
        numDomains = 2;
    }
    else
    {
        numDomains = 1;
    }

    /* Check if the NPDU is received in flexDomain.
       If domainId does not match any of this node's domains,
       then the msg is said to have been received in flex domain. */

    flexDomain = FALSE; /* Assume it is not flex domain. */

    // Save the domain away regardless (used to only save the flex domain but always saving it is more general).
    srcAddr.dmn.domainLen = domainLength;
    memcpy(srcAddr.dmn.domainId, domainId, domainLength);

    if (NodeConfigured() && !IZOT_GET_ATTRIBUTE(eep->domainTable[0], IZOT_DOMAIN_INVALID) && 
    domainLength == IZOT_GET_ATTRIBUTE(eep->domainTable[0], IZOT_DOMAIN_ID_LENGTH) && 
    memcmp(domainId, eep->domainTable[0].Id, domainLength) == 0)
    {
        /* Matches domainId in index 0 */
        srcAddr.dmn.domainIndex = 0;
    }
    else if (NodeConfigured() && numDomains == 2 && !IZOT_GET_ATTRIBUTE(eep->domainTable[1], IZOT_DOMAIN_INVALID) &&
         domainLength == IZOT_GET_ATTRIBUTE(eep->domainTable[1], IZOT_DOMAIN_ID_LENGTH) && 
         memcmp(domainId, eep->domainTable[1].Id, domainLength) == 0)
    {
        /* Matches domainId in index 1 */
        srcAddr.dmn.domainIndex = 1;
    }
    else
    {
        /* Must be a flex domain. */
        srcAddr.dmn.domainIndex = FLEX_DOMAIN;
        flexDomain = TRUE;
    }

    /* Determine if the packet was sent by myself. If so, drop. */
    /* We can do this check only in non-flexdomain as
       src subnet and node are 0 in flex domain. */
    if (! flexDomain && memcmp(&srcAddr.subnetAddr, &eep->domainTable[srcAddr.dmn.domainIndex].Subnet, 2) == 0 &&
    srcAddr.dmn.domainIndex != 1)
    {
        /* Not flex domain and source addr matches. */
        DeQueue(&gp->nwInQ); /* Discard packet. */
        DBG_vPrintf(TRUE, "\nNWReceive: DEQUEUE --- %x - %x", 
        srcAddr.subnetAddr, eep->domainTable[srcAddr.dmn.domainIndex].Subnet);
        return;
    }
    /* Drop packet in various address modes if not for us. */

    switch (srcAddr.addressMode)
    {
    case AM_BROADCAST:
        if (!flexDomain &&  destAddr.Subnet != 0 && /* subnet broadcast. */
        memcmp(&destAddr.Subnet, &eep->domainTable[srcAddr.dmn.domainIndex].Subnet, 1) != 0)
        {
            /* Domain matches but destAddr does not. Not for us. */
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, "NWReceive: Discard BC pck. Not my subnet.\n");
            return;
        }
        srcAddr.broadcastSubnet = destAddr.Subnet;
        break;
    case AM_MULTICAST:
        if (!flexDomain && !IsGroupMember(srcAddr.dmn.domainIndex, srcAddr.group.GroupId, NULL))
        {
            /* Domain matches but group does not. Not for us. */
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, "NWReceive: Discard MC pck. Not my group.\n");
            return;
        }
        break;
    case AM_SUBNET_NODE:
        if (!flexDomain && memcmp(&destAddr, &eep->domainTable[srcAddr.dmn.domainIndex].Subnet, 2) != 0)
        {
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, "NWReceive: Discard MC pck. Not my subnet.\n");
            return;
        }
        break;
    case AM_MULTICAST_ACK:
        /* Make sure the destination subnet/node matches. */
        if (!flexDomain && memcmp(&destAddr, &eep->domainTable[srcAddr.dmn.domainIndex].Subnet, 2) != 0)
        {
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, "NWReceive: Discard MC pck. Not my multicast.\n");
            return;
        }
        /* Also make sure that group matches. */
        if (!flexDomain && !IsGroupMember(srcAddr.dmn.domainIndex, srcAddr.ackNode.groupAddr.group.GroupId, NULL))
        {
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, 
            "NWReceive: Discard multicast ack packet. Not my group.\n");
            return;
        }
        break;
    case AM_UNIQUE_NODE_ID:
        if (memcmp(uniqueNodeId, eep->readOnlyData.UniqueNodeId, IZOT_UNIQUE_ID_LENGTH) != 0)
        {
            /* Unique Node Id message but not for our id. */
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, 
            "NWReceive: Discard Unique Node ID packet. Not my Id.\n");
            return;
        }
        break;
    default:
        ; /* Null statement. */
        /* Error message has been already printed in the previous switch. */
        /* Control should not come here. But, let us play safe. */
        DeQueue(&gp->nwInQ);
        DBG_vPrintf(TRUE, "NWReceive: DEQUEUE 2.\n");
        return;
    }

    /* If a node is in unconfigured state,
       only broadcast and Unique Node ID messages can be received. */
    if (!NodeConfigured() && srcAddr.addressMode != AM_BROADCAST && srcAddr.addressMode != AM_UNIQUE_NODE_ID)
    {
        /* Drop the packet. */
        DeQueue(&gp->nwInQ);
        DBG_vPrintf(TRUE, "NWReceive: Discard packet. We are not online.\n");
        return;
    }
    /* Drop packets received on flexDomain if the state
       is not unconfigured and it is not Unique Node ID addressed. */
    /* Unique Node ID addressed packets are always received. */
    /* i.e if node is configured, flexdomain is not possible
       unless it is Unique Node ID addressed. */

    if (flexDomain && NodeConfigured() && srcAddr.addressMode != AM_UNIQUE_NODE_ID)
    {
        /* Drop the packet. */
        DeQueue(&gp->nwInQ);
        DBG_vPrintf(TRUE, "NWReceive: Discard packet. Flex domain & not UID\n");
        return;
    }
    /* We now got a packet that must be received. */
    INCR_STATS(LcsL3Rx);

    /* pduSize = npduSize - npduHeaderSize. */
    /* j is length of the variable part header of NPDU. */
    /* The fixed portion of NPDU header is always 1 byte. */
    if (nwReceiveParamPtr->pduSize <= j+1)
    {
        // Malformed packet.
        DeQueue(&gp->nwInQ);
        DBG_vPrintf(TRUE, "NWReceive: Discard short packet.\n");
        return;
    }

    pduSize = nwReceiveParamPtr->pduSize - j - 1;
    /* Set the pdu pointer properly. */
    switch (npduPtr->pduType)
    {
    case APDU_TYPE:
        if (QueueFull(&gp->appInQ) || pduSize > gp->appInBufSize)
        {
            /* No space or insufficient space. Discard packet. */
            if (pduSize > gp->appInBufSize)
            {
                LCS_RecordError(IzotWritePastEndOfApplBuffer);
            }
            INCR_STATS(LcsLost);
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, 
            "NWReceive: Discard packet. Insufficient space.\n");
            return;
        }
        /* Queue is not full and buffer has sufficient space. */
        appReceiveParamPtr = QueueTail(&gp->appInQ);
        pduPtr = (IzotByte *) appReceiveParamPtr + sizeof(APPReceiveParam);
        appReceiveParamPtr->indication = MESSAGE;
        appReceiveParamPtr->srcAddr    = srcAddr;
        appReceiveParamPtr->priority   = nwReceiveParamPtr->priority;
        appReceiveParamPtr->altPath    = nwReceiveParamPtr->altPath;
        appReceiveParamPtr->pduSize    = pduSize;
        appReceiveParamPtr->auth       = NmAuth;
        appReceiveParamPtr->service    = IzotServiceUnacknowledged;
#ifdef SECURITY_II
		if (npduPtr->data[j] == LT_AES_GCM_PACKET_CODE) {
			if (processAesGcm(true, appReceiveParamPtr->service, appReceiveParamPtr->priority, 
			    &npduPtr->data[j], appReceiveParamPtr->pduSize, /* Length of original Encrypted APDU */
			    (Byte *) npduPtr, j)) {
				return;
			}
		}
#endif		
        memcpy(pduPtr, &npduPtr->data[j], pduSize);
        LCS_LogRxStat(appReceiveParamPtr->altPath, RX_UNSOLICITED);
        EnQueue(&gp->appInQ);
        DeQueue(&gp->nwInQ);
        return;
    case TPDU_TYPE: /* Fall through. */
    case SPDU_TYPE: /* Fall through. */
    case AUTHPDU_TYPE:
        if (QueueFull(&gp->tsaInQ) || pduSize > gp->tsaInBufSize)
        {
            /* No space or insufficient space. Discard packet. */
            if (pduSize > gp->tsaInBufSize)
            {
                /* Buffer sizes are based on app buf sizes. See
                   TSAReset function. */
                LCS_RecordError(IzotWritePastEndOfApplBuffer);
            }
            INCR_STATS(LcsLost);
            DeQueue(&gp->nwInQ);
            DBG_vPrintf(TRUE, 
            "NWReceive: Discard packet. Insufficient space.\n");
            return;
        }
        /* Queue is not full and buffer has sufficient space. */
        tsaReceiveParamPtr = QueueTail(&gp->tsaInQ);
        pduPtr = (IzotByte *) tsaReceiveParamPtr + sizeof(TSAReceiveParam);
        tsaReceiveParamPtr->pduType   = (PDUType)npduPtr->pduType;
        tsaReceiveParamPtr->srcAddr   = srcAddr;
        tsaReceiveParamPtr->priority  = nwReceiveParamPtr->priority;
        tsaReceiveParamPtr->altPath   = nwReceiveParamPtr->altPath;
        tsaReceiveParamPtr->pduSize   = pduSize;
        tsaReceiveParamPtr->version   = npduPtr->protocolVersion;
        memcpy(pduPtr, &npduPtr->data[j], pduSize);
        EnQueue(&gp->tsaInQ);
        DeQueue(&gp->nwInQ);
        return;
    default:
        DBG_vPrintf(TRUE, "NWReceive: Unknown PDU was received.\n");
        LCS_RecordError(IzotUnknownPdu);
        DeQueue(&gp->nwInQ);
        return;
    }

    /* Should not come here. */
}

/*******************************************************************************
Function:  DecodeDomainLength
Returns:   Decoded value of domain length code.
Reference: None.
Purpose:   To compute the actual domain length from code.
Comments:  None.
*******************************************************************************/
static IzotByte DecodeDomainLength(IzotByte lengthCodeIn)
{
    switch (lengthCodeIn)
    {
    case 0:
        return(0);
    case 1:
        return(1);
    case 2:
        return(3);
    case 3:
        return(6);
    default:
        /* Impossible to come here as lengthCode is 2 bits. */
        ;
    }
    return(0); /* To silence the compiler from complaining. */
}

/*******************************************************************************
Function:  EncodeDomainLength
Returns:   Encode value of domain length.
Reference: None
Purpose:   To compute the encoded value of domain length given.
Comments:
*******************************************************************************/
Status EncodeDomainLength(IzotByte lengthIn, IzotByte* pValue)
{
    Status sts = SUCCESS;
    switch (lengthIn)
    {
    case 0:
        *pValue = 0;
        break;
    case 1:
        *pValue = 1;
        break;
    case 3:
        *pValue = 2;
        break;
    case 6:
        *pValue = 3;
        break;
    default:
        sts = FAILURE;
        break;
    }
    return sts;
}

/*******************************************************************************
Function:  IncrementStat
Purpose:   Increment a network statistic with capping.
Comments:
    Note that the decision was made to keep all statistics in host order.  This
    may make debugging a bit harder on the target but keeps some other things
    simple (such as dealing with statistics relative read memory commands!)
*******************************************************************************/
void IncrementStat(LcsStatistic stat)
{
    IzotByte* p = &nmp->stats.stats[(int)stat*2];
    if (p[0] != 0xff || p[1] != 0xff)
    {
        if (++p[1] == 0)
        {
            ++p[0];
        }
    }
}
/*-------------------------------End of network.c-----------------------------*/
