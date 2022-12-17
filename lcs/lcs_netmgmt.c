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

/*******************************************************************************
          File:        lcs_netmgmt.c

       Version:        1

     Reference:        Section 10, Protocol Specification.

       Purpose:        App Layer/Network Management

                       The functions in this file handle the network
                       management messages (HandleNM()) and network
                       diagnostic messages (HandleND()).

          Note:
                           Discard if    Honor even if     Never
                           Not Request   read/write prot   Authenticated
                           -----------   ---------------   -----------
Network Management Messages:
NM_QUERY_ID           0x61  YES           YES               YES
NM_RESPOND_TO_QUERY   0x62  no            YES               YES
NM_UPDATE_DOMAIN      0x63  no            YES               no
NM_LEAVE_DOMAIN       0x64  no            YES               no
NM_UPDATE_KEY         0x65  no            YES               no
NM_UPDATE_ADDR        0x66  no            YES               no
NM_QUERY_ADDR         0x67  YES           YES               no
NM_QUERY_NV_CNFG      0x68  YES           YES               no
NM_UPDATE_GROUP_ADDR  0x69  no            YES               no
NM_QUERY_DOMAIN       0x6A  YES           YES               no
NM_UPDATE_NV_CNFG     0x6B  no            YES               no
NM_SET_NODE_MODE      0x6C  no            YES               no
NM_READ_MEMORY        0x6D  YES           Limited           no
NM_WRITE_MEMORY       0x6E  no            Limited           no
NM_CHECKSUM_RECALC    0x6F  no            YES               no
NM_WINK               0x70  no            YES               no
NM_MEMORY_REFRESH     0x71  no            YES               no
NM_QUERY_SNVT         0x72  YES           YES               no
NM_NV_FETCH           0x73  YES           YES               no

Network Diagnostic Messages:
ND_QUERY_STATUS       0x51  YES           YES               YES
ND_PROXY_COMMAND      0x52  YES           YES               YES
ND_CLEAR_STATUS       0x53  NO            YES               no
ND_QUERY_XCVR         0x54  YES           YES               no

Manual Service Request Message:
NM_MANUAL_SERVICE_REQUEST
                      0x1F -na-          -na-              -na-

The HandleNM() function is called for each network management
message, and does the appropriate processing.  Some messages
are simple enough to be processed right in HandleNM(), others
have their own function which is called by HandleNM().

The HandleND() function is called for each network diagnostic
message.  Like HandleNM(), the HandleND() function takes care
of simple messages, and more complicated messages are handled
by their own function.

*******************************************************************************/
/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <string.h>

#include <lcs_eia709_1.h>
#include <lcs_node.h>
#include <lcs_app.h>
#include <lcs_api.h>
#include <lcs_netmgmt.h>
#include <pal.h>

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Type Definitions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Function Prototypes.
------------------------------------------------------------------------------*/

/* External function from physical layer to get the transceiver status. */
extern void GetTransceiverStatus(Byte transceiverStatusOut[]);

/*------------------------------------------------------------------------------
Section: Function Definitions
------------------------------------------------------------------------------*/
/*******************************************************************************
Function:  RecomputeChecksum
Returns:   None.
Reference: None
Purpose:   To compute the configuration checksum and store.
Comments:  None.
*******************************************************************************/
static void RecomputeChecksum(void)
{
    eep->configCheckSum = ComputeConfigCheckSum();
}


/*******************************************************************************
Function:  ManualServiceRequestMessage
Returns:   TRUE if the message is sent, FALSE otherwise.
Reference: None
Purpose:   Produces a manual service request message.
Comments:  Prototype in api.h so that application program can use this
           function too. Returns TRUE or FALSE so that application
           program can determine whether the message was sent or not.
*******************************************************************************/
Boolean ManualServiceRequestMessage(void)
{
    NWSendParam       *nwSendParamPtr;
    APDU              *apduRespPtr;

    if (QueueFull(&gp->nwOutQ))
    {
        return(FALSE); /* Can't send it now. Try later. */
    }

    /* Send unack domain wide broadcast message. */
    nwSendParamPtr          = QueueTail(&gp->nwOutQ);
    nwSendParamPtr->pduSize = 1 + UNIQUE_NODE_ID_LEN + ID_STR_LEN;
    if (nwSendParamPtr->pduSize > gp->nwOutBufSize)
    {
        return(FALSE); /* Do not have sufficient space to send the message. */
    }
    apduRespPtr                            = (APDU *)(nwSendParamPtr + 1);
    apduRespPtr->code.allBits              = 0x7F;  /* Manual Service Request. */
    nwSendParamPtr->destAddr.dmn.domainIndex= FLEX_DOMAIN;
    nwSendParamPtr->destAddr.dmn.domainLen = 0;
    nwSendParamPtr->pduType                = APDU_TYPE;
    nwSendParamPtr->destAddr.addressMode   = BROADCAST;
    nwSendParamPtr->destAddr.addr.addr0    = 0; /* Domain wide broadcast. */
    nwSendParamPtr->deltaBL                = 0;
    nwSendParamPtr->altPath                = 0; /* don't use alternate path. */
    nwSendParamPtr->tag                    = MANUAL_SERVICE_REQ_TAG_VALUE;

    memcpy(apduRespPtr->data, eep->readOnlyData.uniqueNodeId,
           UNIQUE_NODE_ID_LEN);
    memcpy(&(apduRespPtr->data[UNIQUE_NODE_ID_LEN]),
           eep->readOnlyData.progId, ID_STR_LEN);
    EnQueue(&gp->nwOutQ);
    gp->manualServiceRequest = FALSE;
    return(TRUE);
}


/*******************************************************************************
Function:  NMNDRespond
Returns:   None
Reference: None
Purpose:   Respond with success or failure code to current message.
           Can be called either for ND or NM messages.
Comments:  None
*******************************************************************************/
static void NMNDRespond(NtwkMgmtMsgType msgType,
                        Status success,
                        APPReceiveParam *appReceiveParamPtr,
                        APDU            *apduPtr)
{
	Byte				code;
	Byte				subCode;
	int					len = 0;
	Byte				data[1];

    /* If service type is not request, nothing to do. */
    if (appReceiveParamPtr->service != REQUEST)
    {
        /* Does not make sense to respond if the original is not a request. */
        return;
    }

    if (msgType == NM_MESSAGE)
    {
		subCode = apduPtr->code.nm.nmCode;
        code = (success == SUCCESS?NM_resp_success:NM_resp_failure) | subCode;
    }
    else
    {
		subCode = apduPtr->code.nd.ndCode;
        code = (success == SUCCESS?ND_resp_success:ND_resp_failure) | subCode;
    }
	// Look for expanded commands.  The response to these includes the sub-command
	if (subCode == 0)
	{
		len = 1;
		data[0] = apduPtr->data[0];
	}
	SendResponse(appReceiveParamPtr->reqId, code, len, data);
}


/*******************************************************************************
Function:  HandleNDQueryUnconfig
Returns:   None
Reference: None
Purpose:   Handle Query Unconfig through Proxy message.
Comments:  This is a simplified version of HandleNMQueryId.
           Only unconfig version of Query ID is supported.
*******************************************************************************/
void HandleNDQueryUnconfig(APPReceiveParam *appReceiveParamPtr,
                           APDU            *apduPtr)
{
    /* Check for proper size of the message */
    if (appReceiveParamPtr->pduSize != 2)
    {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (!NodeUnConfigured())
    {
        /* Not unconfigured - don't respond. */
		SendNullResponse(appReceiveParamPtr->reqId);
    }
	else 
    {
		Byte code =	ND_resp_success | (apduPtr->code.allBits & 0x0F);
		Byte data[UNIQUE_NODE_ID_LEN + ID_STR_LEN];
        memcpy(data, eep->readOnlyData.uniqueNodeId, UNIQUE_NODE_ID_LEN);
        memcpy(&data[UNIQUE_NODE_ID_LEN], eep->readOnlyData.progId, ID_STR_LEN);
		SendResponse(appReceiveParamPtr->reqId, code, sizeof(data), data);
    }
}


/*******************************************************************************
Function:  HandleNDTransceiverStatus
Returns:   none
Reference: None
Purpose:   Handle network diagnostics transceiver status message.
Comments:  None.
*******************************************************************************/
void HandleNDTransceiverStatus(APPReceiveParam *appReceiveParamPtr,
                               APDU            *apduPtr)
{
	XcvrParam      transceiverStatus;
    uint8          n;
	Byte		   response = ND_resp_success | (apduPtr->code.allBits & 0x0F);

    /* Check for proper size of the message. Regular pdusize is 1 byte.
       If this fn is called due to proxy request, then it is 2 bytes. */
    if (appReceiveParamPtr->pduSize > 2)
    {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    else if (appReceiveParamPtr->pduSize == 2)
    {
        /* Make sure it is a proxy request. Or else, length is wrong. */
        if (apduPtr->code.nd.ndFlag == 0x5 &&
                apduPtr->code.nd.ndCode == ND_PROXY_COMMAND &&
                apduPtr->data[0] == 2)
        {
            ; /* It is indeed a proxy command. Length ok. Proceed. */
        }
        else
        {
            NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    }

    if (eep->configData.commType == SPECIAL_PURPOSE)
    {
 	    transceiverStatus = appReceiveParamPtr->xcvrParams;
        n = NUM_COMM_PARAMS;
    }
    else
    {
        n = 0;
		response = ND_resp_failure | (apduPtr->code.allBits & 0x0F);
    }

	SendResponse(appReceiveParamPtr->reqId, response, n, transceiverStatus.data);
}


/*******************************************************************************
Purpose:   Handle incoming NM Update Domain message.
*******************************************************************************/
void HandleNMUpdateDomain(APPReceiveParam *appReceiveParamPtr,
                          APDU            *apduPtr)
{
    Status sts = FAILURE;
    if (appReceiveParamPtr->pduSize >= 2 + sizeof(DomainStruct))
    {
        sts = UpdateDomain((DomainStruct *)&apduPtr->data[1], apduPtr->data[0], true);
        RecomputeChecksum();
    }
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}


/*******************************************************************************
Function:  HandleNMLeaveDomain
Returns:   None
Reference: None
Purpose:   Handle incoming NM LeaveDomain message.
              - Delete the indicated domain table entry.
              - Recompute the configuration checksum.
              - If message was received on the indicated domain,
                do not respond
              - If node no longer belongs to any Domain:
                + become unconfigured
                + reset
Comments:  None.
*******************************************************************************/
void HandleNMLeaveDomain(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{
    /* Fail if message is not 2 bytes long */
    if (appReceiveParamPtr->pduSize != 2)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* If the domain index is bad, fail */
    if (apduPtr->data[0] != 0 && apduPtr->data[0] != 1)
    {
        LCS_RecordError(INVALID_DOMAIN);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Leave the domain */
	memset(&eep->domainTable[apduPtr->data[0]], 0xFF, sizeof(eep->domainTable[0]));
    memcpy(eep->domainTable[apduPtr->data[0]].domainId,
           "gmrdwf",
           DOMAIN_ID_LEN);
    eep->domainTable[apduPtr->data[0]].subnet      = 0;
    eep->domainTable[apduPtr->data[0]].node        = 0;

    /* Recompute the configuration checksum */
    RecomputeChecksum();

    /* If message not received on domain just left, then respond */
    if (apduPtr->data[0] != appReceiveParamPtr->srcAddr.dmn.domainIndex)
    {
        NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr,apduPtr);
    }

    /* If not a member of any domain, go unconfigured and reset. */
    if ((!eep->readOnlyData.twoDomains &&
            eep->domainTable[0].invalid) ||
            (eep->readOnlyData.twoDomains    &&
             eep->domainTable[0].invalid &&
             eep->domainTable[1].invalid) )
    {
        eep->readOnlyData.nodeState = APPL_UNCNFG;
        nmp->resetCause             = SOFTWARE_RESET;
        gp->resetNode               = TRUE; /* Scheduler will reset the node */
    }
}


/*******************************************************************************
Purpose:   Handle incoming NM Update Address message.
*******************************************************************************/
void HandleNMUpdateKey(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
    int i;

    /* Fail if message is not of correct length or domain index is bad. */
    if (appReceiveParamPtr->pduSize != 2 + AUTH_KEY_LEN)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    if (apduPtr->data[0] != 0 && apduPtr->data[0] != 1)
    {
	    
        LCS_RecordError(INVALID_DOMAIN);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    for (i = 0; i < AUTH_KEY_LEN; i++)
    {
        eep->domainTable[apduPtr->data[0]].key[i] +=
            apduPtr->data[i+1];
    }
    RecomputeChecksum();
    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr,apduPtr);
}


/*******************************************************************************
Purpose:   Handle incoming NM Update Address message.
*******************************************************************************/
void HandleNMUpdateAddr(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
    Status sts = FAILURE;
    /* Check for incorrect size. Not sure why UNBOUND tolerates a missing last byte.  Probably legacy from a LB bug. */
    if (appReceiveParamPtr->pduSize >= (apduPtr->data[1]==UNBOUND ? 6:7))
    {
        sts = UpdateAddress((AddrTableEntry *)&apduPtr->data[1], apduPtr->data[0]);
        RecomputeChecksum();
    }
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
Function:  HandleNMQueryAddr
Returns:   None
Reference: None
Purpose:   Handle incoming NM Query Address message.
Comments:  None.
*******************************************************************************/
void HandleNMQueryAddr(APPReceiveParam *appReceiveParamPtr,
                       APDU            *apduPtr)
{
    if (appReceiveParamPtr->pduSize < 2)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Fail if the address table index is bad and set statistics. */
    if (apduPtr->data[0] >= NUM_ADDR_TBL_ENTRIES)
    {
        LCS_RecordError(INVALID_ADDR_TABLE_INDEX);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Send response */
	SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_QUERY_ADDR, sizeof(AddrTableEntry), (Byte*)AccessAddress(apduPtr->data[0]));
}

/*******************************************************************************
Function:  HandleNMQueryNvCnfg
Returns:   None
Reference: None
Purpose:   Handle incoming NM Query Netvar Config message.
Comments:  None.
*******************************************************************************/
void HandleNMQueryNvCnfg(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{
    Queue             *tsaOutQPtr;
    TSASendParam      *tsaSendParamPtr;
    APDU              *apduRespPtr;
    uint16            n;

    /* Fail if the request does not have correct size */
    if (appReceiveParamPtr->pduSize != 2 && appReceiveParamPtr->pduSize != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    n = apduPtr->data[0];
    if (n == 255 && appReceiveParamPtr->pduSize != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
		return;
    }

    /* Decode index */

    /* In implementations which handle a maximum number of network variables that is less
     * than 255, it is not necessary to check for network variable escapes of 255. */
    if (n == 255)
    {
        n = (uint16) apduPtr->data[1];
        n = (n << 8) | apduPtr->data[2];
    }

    /* Fail if there is insufficient space to send the response */
    if (
        (n < nmp->nvTableSize && (1 + sizeof(NVStruct)) > gp->tsaRespBufSize ) ||
        (n >= nmp->nvTableSize && n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE &&
         (1 + sizeof(AliasStruct)) > gp->tsaRespBufSize )
    )
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Send response */
    tsaOutQPtr                    = &gp->tsaRespQ;
    tsaSendParamPtr               = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    apduRespPtr                   = (APDU *)(tsaSendParamPtr + 1);
    apduRespPtr->code.allBits     = NM_resp_success | NM_QUERY_NV_CNFG;
    if (n < nmp->nvTableSize)
    {
        /* Copy the NVStruct entry */
        tsaSendParamPtr->apduSize = 1 + sizeof(NVStruct);
        memcpy(apduRespPtr->data, &(eep->nvConfigTable[n]),
               sizeof(NVStruct));
    }
    else if (n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        /* Copy the alias table entry. */
        tsaSendParamPtr->apduSize = 1 + sizeof(AliasStruct);
        n = n - nmp->nvTableSize;
        memcpy(apduRespPtr->data, &(eep->nvAliasTable[n]),
               sizeof(AliasStruct));
    }
    else
    {
        LCS_RecordError(INVALID_NV_INDEX);
        apduRespPtr->code.allBits = NM_resp_failure | NM_QUERY_NV_CNFG;
        tsaSendParamPtr->apduSize = 1;
    }

    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
Function:  HandleNMNVFetch
Returns:   None
Reference: None
Purpose:   Handle incoming NM NV Fetch message.
Comments:  None.
*******************************************************************************/
void HandleNMNVFetch(APPReceiveParam *appReceiveParamPtr,
                     APDU            *apduPtr)
{
    Queue             *tsaOutQPtr;
    TSASendParam      *tsaSendParamPtr;
    APDU              *apduRespPtr;
    uint16            n;
    uint8             i;

    /* Check if the message has correct size. If not, fail. */
    if (appReceiveParamPtr->pduSize != 2 &&
            appReceiveParamPtr->pduSize != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    n = apduPtr->data[0];
    if (n == 255 && appReceiveParamPtr->pduSize != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    tsaOutQPtr = &gp->tsaRespQ;
    /* Send response */
    tsaSendParamPtr               = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    apduRespPtr                   = (APDU *)(tsaSendParamPtr + 1);

    if (n == 255)
    {
        n = apduPtr->data[1];
        n = (n << 8) | apduPtr->data[2];
        memcpy(apduRespPtr->data, apduPtr->data, 3); /* copy the index */
        i = 3; /* index where value of nv is copied */
    }
    else
    {
        apduRespPtr->data[0] = (char) n;
        i = 1;
    }
    if (n < nmp->nvTableSize)
    {
        /* Make sure there is sufficient space for the response. Else, fail. */
        if (nmp->nvFixedTable[n].nvLength + i + 1 > gp->tsaRespBufSize)
        {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
        memcpy(&apduRespPtr->data[i],
               nmp->nvFixedTable[n].nvAddress,
               nmp->nvFixedTable[n].nvLength);
        tsaSendParamPtr->apduSize = nmp->nvFixedTable[n].nvLength + i + 1;
        apduRespPtr->code.allBits = NM_resp_success | NM_NV_FETCH;
        EnQueue(tsaOutQPtr);

    }
    else
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
    }
}

/*******************************************************************************
Function:  HandleNMQuerySIData
Returns:   None
Reference: None
Purpose:   Handle incoming NM Query SI Data message.
Comments:  None.
*******************************************************************************/
void HandleNMQuerySIData(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{
    Queue             *tsaOutQPtr;
    TSASendParam      *tsaSendParamPtr;
    APDU              *apduRespPtr;
    uint16             offset;
    uint8              count;

    tsaOutQPtr = &gp->tsaRespQ;

    /* Fail if message is not 4 bytes long */
    if (appReceiveParamPtr->pduSize != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Decode offset and count */
    offset = apduPtr->data[0];
    offset = (offset << 8) | apduPtr->data[1];
    count  = apduPtr->data[2];
    /* Check if we have enough space to respond for this message */
    if (count + 1 > gp->tsaRespBufSize)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Send response */
    tsaSendParamPtr               = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    apduRespPtr                   = (APDU *)(tsaSendParamPtr + 1);
    apduRespPtr->code.allBits     = NM_resp_success | NM_QUERY_SNVT;
    tsaSendParamPtr->apduSize     = 1 + count;
    memcpy(apduRespPtr->data,  offset + (char *)&(nmp->snvt), count);
    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
Function:  HandleNMWink
Returns:   None
Reference: None
Purpose:   Handle incoming NM Wink message.
Comments:  None.
*******************************************************************************/
void HandleNMWink(APPReceiveParam *appReceiveParamPtr,
                  APDU            *apduPtr)
{
    Queue             *tsaQueuePtr;
    TSASendParam      *tsaSendParamPtr;
    APDU              *apduRespPtr;
    int8               subcmd;
    int8               niIndex;

    subcmd = 0;

    if (appReceiveParamPtr->pduSize > 1)
    {
        subcmd  = apduPtr->data[0];
    }

    if (appReceiveParamPtr->pduSize > 2)
    {
        niIndex = apduPtr->data[1];
    }

    if (appReceiveParamPtr->pduSize > 3)
    {
        /* Incorrect size */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    if (appReceiveParamPtr->pduSize <= 1 || subcmd == 0)
    {
        if (appReceiveParamPtr->service != REQUEST)
        {
            /* Any service except request/response */
            Wink();  /* Simple Wink */
        }
        else
        {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        }
        return;
    }

    /* must be requesting SEND_ID_INFO. */
    if (appReceiveParamPtr->service != REQUEST)
    {
        return; /* The message should be a request. */
    }

    tsaQueuePtr                           = &gp->tsaRespQ;
    tsaSendParamPtr                = QueueTail(tsaQueuePtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    /* Note: This implementation only has one NI Interface. i.e 0 */
    /* Send response. */
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = FALSE;
    apduRespPtr                   = (APDU *)(tsaSendParamPtr + 1);

    if (niIndex == 0 &&
            UNIQUE_NODE_ID_LEN + ID_STR_LEN + 2 <= gp->tsaRespBufSize )
    {
        tsaSendParamPtr->apduSize      = ID_STR_LEN + UNIQUE_NODE_ID_LEN + 2;
        apduRespPtr->data[0]           = 0; /* Interface not down. */
        apduRespPtr->code.allBits      = NM_resp_success | NM_WINK;
        memcpy(&apduRespPtr->data[1], eep->readOnlyData.uniqueNodeId,
               UNIQUE_NODE_ID_LEN);
        memcpy(&(apduRespPtr->data[1+UNIQUE_NODE_ID_LEN]),
               eep->readOnlyData.progId, ID_STR_LEN);
    }
    else
    {
        tsaSendParamPtr->apduSize  = 1;
        apduRespPtr->code.allBits  = NM_resp_failure | NM_WINK;
    }
    EnQueue(tsaQueuePtr);
}


/*******************************************************************************
Function:  HandleNmeQueryVersion
Purpose:   Handle incoming NME Query Version
*******************************************************************************/
void HandleNmeQueryVersion(APPReceiveParam *appReceiveParamPtr,
 					       APDU            *apduPtr)
{
	struct 
	{
		Byte subcommand;
		Byte version;
		Byte capabilitiesHi;
		Byte capabilitiesLo;
	} queryVersion;

	queryVersion.subcommand = apduPtr->data[0];
	queryVersion.version = 2;
	queryVersion.capabilitiesHi = 0;
	queryVersion.capabilitiesLo = NMV_OMA | NMV_PROXY | NMV_SSI;
	SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(queryVersion), (Byte*)&queryVersion);
}


/*******************************************************************************
Function:  HandleNmeUpdateDomain
Purpose:   Handle incoming NME Join Domain No Key
*******************************************************************************/
void HandleNmeUpdateDomain(APPReceiveParam *appReceiveParamPtr,
						   APDU            *apduPtr)
{
    Status sts = FAILURE;
    if (appReceiveParamPtr->pduSize >= 3 + sizeof(DomainStruct) - AUTH_KEY_LEN)
    {
        sts = UpdateDomain((DomainStruct *)&apduPtr->data[2], apduPtr->data[1], false);
        RecomputeChecksum();
    }
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}


/*******************************************************************************
Function:  HandleNmeReportDomain
Purpose:   Handle incoming NME Report Domain No Key
*******************************************************************************/
void HandleNmeReportDomain(APPReceiveParam *appReceiveParamPtr,
						   APDU            *apduPtr)
{
	DomainStruct *p = AccessDomain(apduPtr->data[1]);
	struct
	{
		Byte subcommand;
		LogicalAddress address;
	} reportDomain;

	if (appReceiveParamPtr->pduSize < 3)
	{
	    NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
	}
	else if (p == NULL)
	{
        LCS_RecordError(INVALID_DOMAIN);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
	}
	else
	{
		reportDomain.subcommand = apduPtr->data[0];
		memcpy(&reportDomain.address, p, sizeof(reportDomain.address));
		SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(reportDomain), (Byte*)&reportDomain);
	}
}


/*******************************************************************************
Function:  HandleNmeReportKey
Purpose:   Handle incoming NME Report Key
*******************************************************************************/
void HandleNmeReportKey(APPReceiveParam *appReceiveParamPtr,
						   APDU            *apduPtr)
{
	uint8 i;
	struct
	{
		Byte subcommand;
		Byte key[OMA_KEY_LEN];
	} reportKey;

	for (i=0; i<2; i++)
	{
		DomainStruct *p = AccessDomain(i);
		if (p == NULL)
		{
		    NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
			break;
		}
		memcpy(&reportKey.key[i*AUTH_KEY_LEN], p->key, AUTH_KEY_LEN);
	}
	if (i==2)
	{
		reportKey.subcommand = apduPtr->data[0];
		SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(reportKey), (Byte*)&reportKey);
	}
}


/*******************************************************************************
Function:  HandleNmeUpdateKey
Purpose:   Handle incoming NME Update Key
*******************************************************************************/
void HandleNmeUpdateKey(APPReceiveParam *appReceiveParamPtr,
						   APDU            *apduPtr)
{
	Status sts = FAILURE;
	if (appReceiveParamPtr->pduSize >= 1+OMA_KEY_LEN && AccessDomain(1) != NULL)
	{
		uint8 i;
		Boolean increment = apduPtr->data[1] == 1;
		Byte* pKey = &apduPtr->data[2];

		for (i=0; i<2; i++)
		{
			int j;
			DomainStruct *p = AccessDomain(i);
			if (!increment)
			{
				memset(p->key, 0, sizeof(p->key));
			}
			for (j=0; j<AUTH_KEY_LEN; j++)
			{
				p->key[j] += *pKey++;
			}
			sts = UpdateDomain(p, i, true);
			if (sts == FAILURE)
			{
				// This should never happen...
				break;
			}
		}
	    RecomputeChecksum();
	}
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}


/*******************************************************************************
Function:  HandleNMExpanded
Purpose:   Handle incoming NM Expanded
*******************************************************************************/
void HandleNMExpanded(APPReceiveParam *appReceiveParamPtr,
                      APDU            *apduPtr)
{
	if (appReceiveParamPtr->pduSize < 2)
	{
		// All expanded commands must at least include a sub-command
	    NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
	}
	else
	{
		switch (apduPtr->data[0])
		{
			case NME_QUERY_VERSION:
				HandleNmeQueryVersion(appReceiveParamPtr, apduPtr);
				break;
			case NME_UPDATE_DOMAIN_NO_KEY:
				HandleNmeUpdateDomain(appReceiveParamPtr, apduPtr);
				break;
			case NME_REPORT_DOMAIN_NO_KEY:
				HandleNmeReportDomain(appReceiveParamPtr, apduPtr);
				break;
			case NME_REPORT_KEY:
				HandleNmeReportKey(appReceiveParamPtr, apduPtr);
				break;
			case NME_UPDATE_KEY:
				HandleNmeUpdateKey(appReceiveParamPtr, apduPtr);
				break;
			default:
			    NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
				break;
		}
	}
}

/*******************************************************************************
Function:  HandleNMQueryId
Returns:   None
Reference: None
Purpose:   Handle incoming NM Query ID message.
Comments:  The message must be a request. There must be space in
           response queue. See HandleNM (We do these checks first).
*******************************************************************************/
void HandleNMQueryId(APPReceiveParam *appReceiveParamPtr,
                     APDU            *apduPtr)
{
    NMQueryIdRequest  *pid;
    char              *memp;
    Queue             *tsaOutQPtr;
    TSASendParam      *tsaSendParamPtr;
    APDU              *apduRespPtr;
    Boolean            allowed;
    uint16             offset;

    /* Fail if message does not have the correct size. Should be 2 or 6+n */
    if (appReceiveParamPtr->pduSize != 2 && appReceiveParamPtr->pduSize < 6)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    tsaOutQPtr = &gp->tsaRespQ;

    /* Init some fields here. Assume we may fail. */
    tsaSendParamPtr               = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    apduRespPtr                   = (APDU *)(tsaSendParamPtr + 1);
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    tsaSendParamPtr->apduSize     = 1;
    /* Since there are a lot of fail cases, init code to indicate failed
       response. */
    apduRespPtr->code.allBits     = NM_resp_failure | NM_QUERY_ID;

    pid = (NMQueryIdRequest *)&apduPtr->data;
    offset = hton16(pid->offset);

    /* if optional fields are present, check that the data field has
       sufficient bytes. */
    if (appReceiveParamPtr->pduSize > 2 &&
            appReceiveParamPtr->pduSize != (6 + pid->count))
    {
        /* The message does not have sufficient data or it has too much data. */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    switch (pid->selector)
    {
    case UNCONFIGURED:
        if (!NodeUnConfigured())
        {
            /* Not unconfigured - don't respond. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        break;
    case SELECTED:
        if (!gp->selectQueryFlag)
        {
            /* Not selected - don't respond. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        break;
    case SELECTED_UNCFG:   /* must be selected and unconfigured */
        if (!gp->selectQueryFlag)
        {
            /* Not selected - don't respond. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        if (!NodeUnConfigured())
        {
            /* Not unconfigured - don't respond */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        break;
    default:
        EnQueue(tsaOutQPtr); /* Failed response. */
        return;
    }

    /* If memory matching is present, check memory match */
    if (appReceiveParamPtr->pduSize > 2)
    {
        switch (pid->mode)
        {
        case ABSOLUTE_MEM_ADDR:
            memp = (char *)nmp;
            if (offset >= 0xF000)
            {
                memp = (char *)eep - 0xF000;
            }
            break;
        case CONFIG_RELATIVE:
            memp = (char *)&(eep->configData);
            break;
        case READ_ONLY_RELATIVE:
            memp = (char *)&(eep->readOnlyData);
            break;
        default:
            EnQueue(tsaOutQPtr);
            return;   /* Failed response. */
        }

        memp += offset;

        /* Absolute addressing to read snvt is not possible */
        allowed = (memp >= (char *)&eep->readOnlyData &&
                   memp + apduPtr->data[3] < (char *)&eep->domainTable[0]);
        if (!allowed)
        {
            EnQueue(tsaOutQPtr);
            return; /* Failed response. */
        }

        if (memcmp(pid->data, memp, pid->count) != 0)
        {
            /* Compare failed - don't reply. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
    }

    /* Send response */
    if (1 + UNIQUE_NODE_ID_LEN + ID_STR_LEN <= gp->tsaRespBufSize )
    {
        tsaSendParamPtr->apduSize = 1 + UNIQUE_NODE_ID_LEN + ID_STR_LEN;
        apduRespPtr->code.allBits = NM_resp_success | NM_QUERY_ID;
        memcpy(apduRespPtr->data, eep->readOnlyData.uniqueNodeId,
               UNIQUE_NODE_ID_LEN);
        memcpy(&(apduRespPtr->data[UNIQUE_NODE_ID_LEN]),
               eep->readOnlyData.progId, ID_STR_LEN);
    }

    EnQueue(tsaOutQPtr);
}


/*******************************************************************************
Purpose:   Handle incoming NM Update Group Addr message.
*******************************************************************************/
void HandleNMUpdateGroupAddr(APPReceiveParam *appReceiveParamPtr,
                             APDU *apduPtr)
{
    uint16           addrIndex;
    GroupAddrMode   *groupStrPtr;
    AddrTableEntry  *ap;

    /* This message must be delivered with group addressing and is
       updated based on the domain in which it was received. Hence,
       flex domain is not allowed. */
    if (appReceiveParamPtr->srcAddr.addressMode != MULTICAST ||
            appReceiveParamPtr->srcAddr.dmn.domainIndex == FLEX_DOMAIN)
    {
        /* This message should be sent in MULTICAST. Fail */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    if (appReceiveParamPtr->pduSize != 1 + sizeof(AddrTableEntry))
    {
        /* Incorrect size */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* For accessing the corresponding address table entry,
       let us use the domainIndex in which it was received and
       the group in which it was received. It makes sense to use
       the domainIndex in which the message was received rather than
       the domain index in the packet(which will be same for all
       recipients) as it may be different for different nodes. */
    groupStrPtr = (GroupAddrMode *)&apduPtr->data[0];
    if (groupStrPtr->groupFlag == 1)
    {
        addrIndex = AddrTableIndex(appReceiveParamPtr->srcAddr.dmn.domainIndex,
                                   appReceiveParamPtr->srcAddr.group);
    }

    /* Make sure we got a good index. */
    if (groupStrPtr->groupFlag != 1 || addrIndex == 0xFF)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    ap = AccessAddress(addrIndex); /* ap cannot be NULL */
    /* Only group size and timer values should be changed */
    ap->groupEntry.groupSize  = groupStrPtr->groupSize;
    ap->groupEntry.rptTimer   = groupStrPtr->rptTimer;
    ap->groupEntry.retryCount = groupStrPtr->retryCount;
    ap->groupEntry.rcvTimer   = groupStrPtr->rcvTimer;
    ap->groupEntry.txTimer    = groupStrPtr->txTimer;
    RecomputeChecksum();
    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}


/*******************************************************************************
Function:  HandleNMQueryDomain
Returns:   None
Reference: None
Purpose:   Handle incoming NM QueryDomain message.
Comments:  Must be a Request.
*******************************************************************************/
void HandleNMQueryDomain(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{
	DomainStruct	  *p = AccessDomain(apduPtr->data[0]);

    /* Fail if message does not have the correct size. */
    if (appReceiveParamPtr->pduSize != 2)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* If domain index is other than 0 or 1 or if the node is in only one
       domain and the index is 1, then fail. */
    if (p == NULL)
    {
        /* Domain index is bad. */
        LCS_RecordError(INVALID_DOMAIN);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Send response */
	SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_QUERY_DOMAIN, sizeof(DomainStruct), (Byte*)p);
}

/*******************************************************************************
Function:  HandleNMUpdateNvConfig
Purpose:   Handle incoming NM Update NV Config message.
*******************************************************************************/
void HandleNMUpdateNvConfig(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
    uint16           n; /* for nv index */
    uint8            pduSize;
    NVStruct        *np;

    if (appReceiveParamPtr->pduSize < 5)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Decode index */

    n = apduPtr->data[0];
    if (n == 255)
    {
        n = (uint16)apduPtr->data[1];
        n = (n << 8) | apduPtr->data[2];
        if (n < nmp->nvTableSize)
        {
            pduSize = sizeof(NVStruct) + 4; /* escaped regular update */
        }
        else
        {
            /* Escaped alias update. Assume that host_primary is
               absent for now. */
            pduSize = (sizeof(AliasStruct) - 2) + 4;
        }
        np = (NVStruct *)(&apduPtr->data[3]);
    }
    else
    {
        if (n < nmp->nvTableSize)
        {
            pduSize = sizeof(NVStruct) + 2; /* regular update */
        }
        else
        {
            /* Alias update. Assume that host_primary is absent for now. */
            /* last 2 is for index + code */
            pduSize = (sizeof(AliasStruct) - 2) + 2;
        }
        np = (NVStruct *)(&apduPtr->data[1]);
    }

    /* Update nv config or alias table */
    if (n < nmp->nvTableSize)
    {
        if (appReceiveParamPtr->pduSize >= pduSize)
        {
            memcpy(&eep->nvConfigTable[n], np, sizeof(NVStruct));
        }
        else
        {
            /* Incorrect size */
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    }
    else if (n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        n = n - nmp->nvTableSize; /* Alias table index */
        /* Check for various forms of alias update */
        if (((AliasStruct *)np)->primary == 0xFF &&
                appReceiveParamPtr->pduSize == pduSize)
        {
            /* host_primary missing. default to 0xffff. Null alias update. */
            ((AliasStruct *)np)->hostPrimary = 0xffff;
        }
        else if (((AliasStruct *)np)->primary == 0xFF)
        {
            /* escaped alias. hostPrimary is present */
            pduSize += 2;
        }
        /* Update the nv alias table */
        if (appReceiveParamPtr->pduSize >= pduSize)
        {
            memcpy(&eep->nvAliasTable[n], np, sizeof(AliasStruct));
        }
        else
        {
            /* Incorrect size */
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    }
    else
    {
        /* Invalid nv table index */
        LCS_RecordError(INVALID_NV_INDEX);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Recompute checksum and send response */
    RecomputeChecksum();
    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}


/*******************************************************************************
Function:  HandleNMSetNodeMode
Returns:   None
Reference: None
Purpose:   Handle NM SetNodeMode message
Comments:
*                                                          Possible
* Description                   State  Mode   Service LED  in ref imp?
* --------------------------------------------------------------------
* Applicationless, unconfigured   3     -     On           NO
* Unconfigured (w/application)    2     -     Flashing     YES
* Configured, Hard Offline        6     -     Off          NO
* Configured                      4     1     Off          YES
* Configured, Soft offline        4     0     Off          YES
*
* The NM_SET_NODE_MODE message encompasses a lot of functionality,
* and impacts some other areas of the implementation.
* 1) Mode is not maintained in EEPROM
* 2) A node that is soft-offline will go on-line when it is reset
* 3) The hard-offline state is preserved across reset
* 4) For either hard or soft offline, the scheduler is disabled
* 5) When soft-offline:
*    A) Polling an NV will return NULL data
*    B) Incomming network variable updates are handled normally
*    C) But nv_update_occurs events will be lost
* 6) In all other states except configured:
*    A) No response is returned on NV polls
*    B) Incomming NV updates are discarded
* 7) If a node is in a non-configured state, is reset and then issued
*    a command to go configured, it will come up soft offline
* 8) If a set node mode message changes the mode to offline or online
*    the approprite task (if any) is executed
* 9) Changing the node state recomputes the configuration checksum
*******************************************************************************/
void HandleNMSetNodeMode(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
    if (appReceiveParamPtr->pduSize < 2 || (apduPtr->data[0] == 3 && appReceiveParamPtr->pduSize < 3))
    {
        NMNDRespond(NM_MESSAGE, FAILURE,appReceiveParamPtr, apduPtr);
        return;
    }

    switch (apduPtr->data[0])
    {
    case 0:  /* Go to soft offline state */
        if (AppPgmRuns())
        {
            OfflineEvent();  /* Indicate to application program. */
        }
        eep->readOnlyData.nodeState = CNFG_ONLINE;
        gp->appPgmMode   = OFF_LINE;
        gp->ioOutputPin1 = FALSE; /* LED off */
        /* No response given as the message is not a request. */
        break;
    case 1:  /* Go on-line */
        OnlineEvent(); /* Indicate to application program. */
        eep->readOnlyData.nodeState = CNFG_ONLINE;
        gp->appPgmMode   = ON_LINE;
        gp->ioOutputPin1 = FALSE; /* LED off */
        /* No response given as the message is not a request. */
        break;
    case 2:  /* Application reset */
        gp->resetNode   = TRUE;
        nmp->resetCause = SOFTWARE_RESET; /* Software reset. */
        /* No response since the node is being reset. */
        break;
    case 3:  /* Change State */
        /* Fail if message is not 3 bytes long. */
        if (appReceiveParamPtr->pduSize != 3)
        {
            NMNDRespond(NM_MESSAGE, FAILURE,
                        appReceiveParamPtr, apduPtr);
            break;
        }
        eep->readOnlyData.nodeState = apduPtr->data[1];
        /* Preserve the state of appPgmMode except for
           NO_APPL_UNCNFG. */
        if (eep->readOnlyData.nodeState == NO_APPL_UNCNFG)
        {
            gp->appPgmMode = NOT_RUNNING;
        }
        RecomputeChecksum();
        /* Respond with success if the message was a request. */
        NMNDRespond(NM_MESSAGE, SUCCESS,
                    appReceiveParamPtr, apduPtr);
        break;
    default:
        /* Let us reset the node for this case */
        gp->resetNode   = TRUE;
        nmp->resetCause = SOFTWARE_RESET;
        break;
    }
}


/*******************************************************************************
Function:  HandleNMReadMemory
Returns:   None
Reference: None
Purpose:   Handle incomming NM ReadMemory message
Comments:  None.
*******************************************************************************/
void HandleNMReadMemory(APPReceiveParam *appReceiveParamPtr,
                        APDU            *apduPtr)
{
    Queue               *tsaOutQPtr;
    TSASendParam        *tsaSendParamPtr;
    APDU                *apduRespPtr;
    char                *memp;
    uint16               offset;
    Boolean              allowed;

    tsaOutQPtr = &gp->tsaRespQ;

    if (appReceiveParamPtr->pduSize < 5 ||
		apduPtr->data[3] > gp->tsaRespBufSize)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    offset = ((uint16)apduPtr->data[1] << 8) | apduPtr->data[2];

    /* Assemlbe response */
    tsaSendParamPtr               = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    tsaSendParamPtr->apduSize     = 1 + apduPtr->data[3];

    apduRespPtr = (APDU *)(tsaSendParamPtr + 1);

    apduRespPtr->code.allBits = NM_resp_success | NM_READ_MEMORY;

    switch (apduPtr->data[0])
    {
    case ABSOLUTE_MEM_ADDR:
        memp = (char *)nmp;
        if (offset >= 0xF000)
        {
            memp = (char *)eep - 0xF000;
        }
        break;
    case READ_ONLY_RELATIVE:
    default:
        memp = (char *)&(eep->readOnlyData);
        break;
    case CONFIG_RELATIVE:
        memp = (char *)&(eep->configData);
        break;
    case STAT_RELATIVE:
        memp = (char *)&(nmp->stats);
        break;
    }

    memp += offset;

    /* If readWriteProtect flag is on, then only readonly data structure,
       snvt structures, and configuration structure can be read.
       The one byte read of location 0 (firmware number) is also allowed.
       Reference implementation does not support snvt reading through
       absolute addressing. readOnlyData.snvtStruct is 0xFFFF */


    if (eep->readOnlyData.readWriteProtect == TRUE)
    {
        allowed = (memp >= (char *)&eep->readOnlyData &&
                   memp + apduPtr->data[3] < (char *)&eep->domainTable[0]) ||
                  (memp == (char *)&nmp && apduPtr->data[3] == 1);

        if (!allowed)
        {
            tsaSendParamPtr->apduSize = 1;
            apduRespPtr->code.allBits = NM_resp_failure | NM_READ_MEMORY;
            EnQueue(tsaOutQPtr);
            return;
        }
    }

    memcpy(apduRespPtr->data, memp, apduPtr->data[3]);

    if (memp == (char *)nmp && apduPtr->data[3] == 1)
    {
        /* trap for absolute read of location 0 and write
           version number */
        apduRespPtr->data[0] = BASE_FIRMWARE_VERSION;
    }
    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
Function:  HandleNMWriteMemory
Returns:   None
Reference: None
Purpose:   Handle incoming NM WriteMemory message.
Comments:  None.
*******************************************************************************/
void HandleNMWriteMemory(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{
    Queue                *tsaOutQPtr;
    TSASendParam         *tsaSendParamPtr;
    APDU                 *apduRespPtr;
    NMWriteMemoryRequest *pr;
    char                 *memp;
    uint16                offset;
    Boolean               allowed = true;

    tsaOutQPtr = &gp->tsaRespQ;

    /* Fail if message is not at least 6 bytes long */
    if (appReceiveParamPtr->pduSize < 6)
    {
        NMNDRespond(NM_MESSAGE, FAILURE,appReceiveParamPtr,apduPtr);
        return;
    }

    /* Pointer to struct describing memory request */
    pr = (NMWriteMemoryRequest *)&apduPtr->data[0];

    offset = hton16(pr->offset);

    /* Fail if message length doesn't match count. Poke can use 16 data bytes.
       Allow that. Note that the code takes one byte. */
    if (appReceiveParamPtr->pduSize != 6 + pr->count &&
            appReceiveParamPtr->pduSize != 17)
    {
        NMNDRespond(NM_MESSAGE, FAILURE,appReceiveParamPtr,apduPtr);
        return;
    }

    if (appReceiveParamPtr->service == REQUEST)
    {
        /* Assemble response */
        tsaSendParamPtr               = QueueTail(tsaOutQPtr);
        tsaSendParamPtr->altPathOverride = FALSE;
        tsaSendParamPtr->service      = RESPONSE;
        tsaSendParamPtr->nullResponse = FALSE;
		tsaSendParamPtr->flexResponse = FALSE;
        tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
        tsaSendParamPtr->apduSize     = 1;
        apduRespPtr = (APDU *)(tsaSendParamPtr + 1);
    }

    switch (pr->mode)
    {
    case ABSOLUTE_MEM_ADDR:
        memp = (char *)nmp;
        if (offset >= 0xF000)
        {
            memp = (char *)eep;
			offset -= 0xF000;
        }
        break;
    case CONFIG_RELATIVE:
        memp = (char *)&(eep->configData);
        break;
    case STAT_RELATIVE:
        memp = (char *)&(nmp->stats);
        break;
    case READ_ONLY_RELATIVE:
        memp = (char *)&(eep->readOnlyData);
        break;
    default:
        /* Invalid Mode */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    memp += offset;

    /* Check if the range of memory cells written is good. */
    allowed = (memp >= (char *)nmp &&
               (memp + pr->count) <= (char *)(nmp+1)) ||
              (memp >= (char *)eep &&
               (memp + pr->count) <= (char *)(eep+1));
    if (! allowed)
    {

        /* Send failure response if the message was a request */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* If readwrite flag is on, then only config structure can be written. */
    if (eep->readOnlyData.readWriteProtect == TRUE)
    {
        allowed = (memp >= (char *)&eep->configData &&
                   memp + pr->count < (char *)&eep->domainTable[0]);

        if (! allowed)
        {
            /* Send failure response if the message was a request */
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    }

    /* Need to copy only the data array in NM_Req structure.
       The header is 5 bytes long */
    /* We have to assume that pr->count is good. Max is 255 */
    /* Reference implementation has no application check sum.
       Only config checksum */
    memcpy(memp, apduPtr->data+5, pr->count);

    if (pr->form & CNFG_CS_RECALC) {
        RecomputeChecksum();
    }
    if (pr->form & ACTION_RESET) {
        gp->resetNode   = TRUE;
        nmp->resetCause = SOFTWARE_RESET;
    }

    /* There is no harm in responding even when the node is reset. It will
       be lost anyway */
    if (appReceiveParamPtr->service == REQUEST)
    {
        apduRespPtr->code.allBits = NM_resp_success | NM_WRITE_MEMORY;
        EnQueue(tsaOutQPtr);
    }
}



/*******************************************************************************
Function:  HandleProxyResponse
Returns:   None
Reference: None
Purpose:   Handle Proxy Response Message.
Comments:  None.
*******************************************************************************/
void HandleProxyResponse(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{
	if (appReceiveParamPtr->proxyDone || SendResponse(appReceiveParamPtr->tag, apduPtr->code.allBits, appReceiveParamPtr->pduSize-1, apduPtr->data) == SUCCESS)
	{
		DeQueue(&gp->appInQ);
	}
}

/*******************************************************************************
Function:  HandleNDQueryStatus
Returns:   none
Reference: None
Purpose:   Handle Network Diagnostics Query Status Message.
Comments:  None.
*******************************************************************************/
void HandleNDQueryStatus(APPReceiveParam *appReceiveParamPtr,
                         APDU *apduPtr, Bool useFlexDomain)
{
    NDQueryStat    ndq;
    Queue         *tsaOutQPtr;
    TSASendParam  *tsaSendParamPtr;
    APDU          *apduRespPtr;

    tsaOutQPtr = &gp->tsaRespQ;
    if (((apduPtr->code.allBits & 0x0F) == ND_QUERY_STATUS) &&
            appReceiveParamPtr->pduSize != 1)
    {
        /* Incorrect size. Fail. */
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (((apduPtr->code.allBits & 0x0F) == ND_PROXY_COMMAND) &&
            appReceiveParamPtr->pduSize != 2)
    {
        /* Incorrect size. Fail. */
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

	memcpy(ndq.stats, nmp->stats.stats, sizeof(ndq.stats));
	ndq.resetCause         = nmp->resetCause;
    if (eep->readOnlyData.nodeState == CNFG_ONLINE &&
            gp->appPgmMode == OFF_LINE)
    {
        ndq.nodeState = SOFT_OFFLINE;
    }
    else
    {
        ndq.nodeState = eep->readOnlyData.nodeState;
    }
    ndq.versionNumber = FIRMWARE_VERSION;
    ndq.errorLog      = eep->errorLog;
    ndq.modelNumber   = MODEL_NUMBER;
    /* Send response */
    tsaSendParamPtr               = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service      = RESPONSE;
    tsaSendParamPtr->nullResponse = FALSE;
	tsaSendParamPtr->flexResponse = useFlexDomain;
    tsaSendParamPtr->reqId        = appReceiveParamPtr->reqId;
    tsaSendParamPtr->apduSize     = 1 + sizeof(NDQueryStat);
	apduRespPtr                   = (APDU *)(tsaSendParamPtr + 1);
    /* The response code is formed using apduPtr->code.allBits as the
       actual could be a proxy command and the response code will be
       different. Thus, using apduPtr->code.allBits works for both
       native query status command as well as proxy based query status */
    if (tsaSendParamPtr->apduSize <= gp->tsaRespBufSize)
    {
        apduRespPtr->code.allBits     =
            ND_resp_success | (apduPtr->code.allBits & 0x0F);
        memcpy(apduRespPtr->data, &ndq,  sizeof(NDQueryStat));
    }
    else
    {
        apduRespPtr->code.allBits     =
            ND_resp_failure | (apduPtr->code.allBits & 0x0F);
        tsaSendParamPtr->apduSize     = 1;
    }
    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
Function:  HandleNDProxyCommand
Returns:   none
Reference: None
Purpose:   Handle network diagnostics proxy request message.
Comments:  None.
*******************************************************************************/
Status HandleNDProxyCommand(APPReceiveParam *appReceiveParamPtr,
                            APDU            *apduPtr)
{
    Status          sts = SUCCESS;
    Queue         *tsaOutQPtr;
    TSASendParam  *tsaSendParamPtr;
    APDU          *apduSendPtr;

    /* See if the proxy command is to be forwarded or it is for us
       to process. If the address information in the packet is missing,
       then there is no forwarding information and hence we respond directly */
    if (appReceiveParamPtr->pduSize == 2)
    {
        /* Address portion is missing. This node is the proxy target. */
        tsaOutQPtr = &gp->tsaRespQ;
        /* Send the response directly back to the sender */
        switch (apduPtr->data[0])
        {
        case 0:
            HandleNDQueryUnconfig(appReceiveParamPtr, apduPtr);
            break;
        case 1:
            HandleNDQueryStatus(appReceiveParamPtr, apduPtr, false);
            break;
        case 2:
            HandleNDTransceiverStatus(appReceiveParamPtr, apduPtr);
            break;
        default:
            /* Invalid sub_command. Send failure response */
            NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        }
        return sts;
    }

    /* Send failure response if we are proxy agent and the message received is on flex domain
       or it does not have correct size */
    if (appReceiveParamPtr->srcAddr.dmn.domainIndex == FLEX_DOMAIN ||
        (apduPtr->data[1] == UNIQUE_NODE_ID &&
         appReceiveParamPtr->pduSize !=
         (2 + sizeof(AddrTableEntry) + UNIQUE_NODE_ID_LEN) ) ||
        (apduPtr->data[1] != UNIQUE_NODE_ID &&
         appReceiveParamPtr->pduSize != (2 + sizeof(AddrTableEntry)) )
    )
    {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return sts;
    }


    /* We need to forward the proxy now. Set the target queue ptrs */
    if (appReceiveParamPtr->priority)
    {
        tsaOutQPtr = &gp->tsaOutPriQ;
    }
    else
    {
        tsaOutQPtr = &gp->tsaOutQ;
    }

    /* Check if the target queue has space for forwarding this request. */
    if (QueueFull(tsaOutQPtr))
    {
        // Failure indicates we didn't process the message so don't free it!
        return FAILURE;
    }

    /* Generate request message */
    tsaSendParamPtr                = QueueTail(tsaOutQPtr);

    /* First copy the msg_out_addr info in the proxy command */
    memcpy(&tsaSendParamPtr->destAddr, &apduPtr->data[1], sizeof(MsgOutAddr) + UNIQUE_NODE_ID_LEN);

    /* Set the domain index to be the one in which it was received.
       Note that this cannot be flex domain. Transport or session
       will use this instead of the one in the destAddr (i.e msg_out_addr) */
    tsaSendParamPtr->dmn.domainIndex   = appReceiveParamPtr->srcAddr.dmn.domainIndex;
    tsaSendParamPtr->service  = REQUEST;
    tsaSendParamPtr->auth     = FALSE;
	// The tag for a proxy request is the original request's reqId.  When the response comes back,
	// the response is delivered using that tag in order to vector it back to the original receive transaction.
	// See HandleProxyResponse().
    tsaSendParamPtr->tag      = appReceiveParamPtr->reqId;
    /* Proxy relays the message, the altpath bit should be same as the one
       used for the proxy message received */
    tsaSendParamPtr->altPathOverride = TRUE;
    tsaSendParamPtr->altPath = appReceiveParamPtr->altPath;
    tsaSendParamPtr->apduSize = 2; /* Always two bytes: code + sub_command */
	tsaSendParamPtr->priority = appReceiveParamPtr->priority;
	tsaSendParamPtr->proxy	  = TRUE;
	tsaSendParamPtr->proxyDone = FALSE;
	tsaSendParamPtr->proxyCount= 0;
	tsaSendParamPtr->txInherit = FALSE;
	tsaSendParamPtr->txTimerDeltaLast = 0;

    apduSendPtr  = (APDU *)(tsaSendParamPtr + 1);
    /* Copy the code as it is */
    apduSendPtr->code.allBits = apduPtr->code.allBits;
    apduSendPtr->data[0] = apduPtr->data[0];
    EnQueue(tsaOutQPtr);
    return sts;
}


/*******************************************************************************
Function:  HandleNDClearStatus
Returns:   none
Reference: None
Purpose:   Handle network diagnostics clear status message.
Comments:  None.
*******************************************************************************/
void HandleNDClearStatus(APPReceiveParam *appReceiveParamPtr,
                         APDU            *apduPtr)
{

    /* Check for proper size of the message */
    if (appReceiveParamPtr->pduSize != 1)
    {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Clear Status */
    memset(&nmp->stats, 0, sizeof(nmp->stats));
    nmp->resetCause                 = CLEARED;
    eep->errorLog                   = NO_ERRORS;  /* Cleared */

    /* NMNDRespond will send response only if the msg is REQUEST */
    NMNDRespond(ND_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
Function:  HandleNDQueryXcvrBidir
Returns:   none
Reference: None
Purpose:   Handle network diagnostics bidirectional signal strength measurement
Comments:  None.
*******************************************************************************/
void HandleNDQueryXcvrBidir(APPReceiveParam *appReceiveParamPtr,
							APDU            *apduPtr)
{
    if (appReceiveParamPtr->pduSize == 0x00)
    {
        /* Incorrect size. Fail. */
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

	SendResponse(appReceiveParamPtr->reqId, ND_resp_success |ND_QUERY_XCVR_BIDIR, NUM_COMM_PARAMS, appReceiveParamPtr->xcvrParams.data);
}
 
/*******************************************************************************
Function:  HandleNDGetFullVersion
Returns:   none
Reference: None
Purpose:   Handle network diagnostics get full version command
Comments:  None.
*******************************************************************************/
void HandleNDGetFullVersion(APPReceiveParam *appReceiveParamPtr,
							APDU            *apduPtr)
{
    Byte fullVersion[2];
	fullVersion[0] = FIRMWARE_VERSION;
	fullVersion[1] = FIRMWARE_BUILD;
  	SendResponse(appReceiveParamPtr->reqId, ND_resp_success |ND_GET_FULL_VERSION, sizeof(fullVersion), fullVersion);
}


/*******************************************************************************
Function:  HandleND
Returns:   None
Reference: None
Purpose:   Handle incomming network diagnostic message.
Comments:  None.
*******************************************************************************/
void HandleND(APPReceiveParam *appReceiveParamPtr,
              APDU            *apduPtr)
{
    Status sts = SUCCESS;

    /* It is not legal for a response to be an ND command */
    if (appReceiveParamPtr->service != RESPONSE)
    {
        /* If network diagnostics messages need authentication
            and the message did not pass authentication and
            the node is not unconfigured
            then discard those messages that should be discarded and return. */
        if (!NodeUnConfigured() && eep->configData.nmAuth &&
                !appReceiveParamPtr->auth &&
                (apduPtr->code.nd.ndCode != ND_QUERY_STATUS && 
				 apduPtr->code.nd.ndCode != ND_PROXY_COMMAND &&
				 apduPtr->code.nd.ndCode != ND_QUERY_STATUS_FLEX &&
				 apduPtr->code.nd.ndCode != ND_QUERY_XCVR_BIDIR &&
				 apduPtr->code.nd.ndCode != ND_GET_FULL_VERSION))
        {
            LCS_RecordError(AUTHENTICATION_MISMATCH);
            NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        }
        else
        {
            /* Handle various network diagnostic message codes */
            switch (apduPtr->code.nd.ndCode)
            {
            case ND_QUERY_STATUS:
                HandleNDQueryStatus(appReceiveParamPtr, apduPtr, false);
				break;
            case ND_PROXY_COMMAND:
                sts = HandleNDProxyCommand(appReceiveParamPtr, apduPtr);
				break;
            case ND_CLEAR_STATUS:
                HandleNDClearStatus(appReceiveParamPtr, apduPtr);
				break;
            case ND_QUERY_XCVR:
                HandleNDTransceiverStatus(appReceiveParamPtr, apduPtr);
				break;
			case ND_QUERY_STATUS_FLEX:
			    HandleNDQueryStatus(appReceiveParamPtr, apduPtr, true);
				break;			
			case ND_QUERY_XCVR_BIDIR:
			    HandleNDQueryXcvrBidir(appReceiveParamPtr, apduPtr);
				break;				
			case ND_GET_FULL_VERSION:
			    HandleNDGetFullVersion(appReceiveParamPtr, apduPtr);
				break;
            default:
                /* Discard unrecognized diagnostic command */
                NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
				break;
            }
        }
    }
    if (sts == SUCCESS)
    {
        // Proxy may not actually process the message so we don't dequeue in that case
        DeQueue(&gp->appInQ);
    }
}

/*******************************************************************************
Function:  HandleNM
Returns:   None
Reference: None
Purpose:   Handle incomming network management messages.
Comments:  None.
*******************************************************************************/
void HandleNM(APPReceiveParam *appReceiveParamPtr,
              APDU            *apduPtr)
{
    if (appReceiveParamPtr->service == RESPONSE)
    {
        /* It is not legal for a response to be an NM command. */
        DeQueue(&gp->appInQ);
        return;
    }

    /* If network management messages need authentication
       and the message did not pass authentication and
       the node is not unconfigured
       then discard those messages that should be discarded and return. */
    if (!NodeUnConfigured() && eep->configData.nmAuth && !appReceiveParamPtr->auth)
    {
        /* Only a few messages are allowed. Others should be discarded */
		Byte subCommand = apduPtr->data[0];
        if (apduPtr->code.nm.nmCode != NM_QUERY_ID &&
                apduPtr->code.nm.nmCode != NM_RESPOND_TO_QUERY &&
				(apduPtr->code.nm.nmCode != NM_EXPANDED || subCommand != NME_QUERY_VERSION))
        {
            LCS_RecordError(AUTHENTICATION_MISMATCH);
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            DeQueue(&gp->appInQ);
            return;
        }
    }

    /* Handle various network mgmt message codes */
    switch (apduPtr->code.nm.nmCode)
    {
	case NM_EXPANDED:
		HandleNMExpanded(appReceiveParamPtr, apduPtr);
		break;
    case NM_QUERY_ID:
        HandleNMQueryId(appReceiveParamPtr,apduPtr);
        break;
    case NM_RESPOND_TO_QUERY:
        /* Fail if message is not 2 bytes long or the byte is bad. */
        if (appReceiveParamPtr->pduSize != 2 ||
                (apduPtr->data[0] != 0 && apduPtr->data[0] != 1) )
        {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        }
		else
		{
	        gp->selectQueryFlag = apduPtr->data[0];
		    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr,apduPtr);
		}
		break;
    case NM_UPDATE_DOMAIN:
        HandleNMUpdateDomain(appReceiveParamPtr, apduPtr);
        break;
    case NM_LEAVE_DOMAIN:
        HandleNMLeaveDomain(appReceiveParamPtr, apduPtr);
        break;
    case NM_UPDATE_KEY:
        HandleNMUpdateKey(appReceiveParamPtr, apduPtr);
        break;
    case NM_UPDATE_ADDR:
        HandleNMUpdateAddr(appReceiveParamPtr, apduPtr);
        break;
    case NM_QUERY_ADDR:
        HandleNMQueryAddr(appReceiveParamPtr, apduPtr);
        break;
    case NM_QUERY_NV_CNFG:
        HandleNMQueryNvCnfg(appReceiveParamPtr, apduPtr);
        return;
    case NM_UPDATE_GROUP_ADDR:
        HandleNMUpdateGroupAddr(appReceiveParamPtr,apduPtr);
        break;
    case NM_QUERY_DOMAIN:
        HandleNMQueryDomain(appReceiveParamPtr,apduPtr);
        break;
    case NM_UPDATE_NV_CNFG:
        HandleNMUpdateNvConfig(appReceiveParamPtr, apduPtr);
        break;
    case NM_SET_NODE_MODE:
        HandleNMSetNodeMode(appReceiveParamPtr, apduPtr);
        break;
    case NM_READ_MEMORY:
        HandleNMReadMemory(appReceiveParamPtr, apduPtr);
        break;
    case NM_WRITE_MEMORY:
        HandleNMWriteMemory(appReceiveParamPtr, apduPtr);
        break;
    case NM_CHECKSUM_RECALC:
        // We only have config checksum.
        RecomputeChecksum();
        NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
        break;
    case NM_WINK:   /* Same as NM_INSTALL */
        HandleNMWink(appReceiveParamPtr,apduPtr);
        break;
    case NM_MEMORY_REFRESH:
#pragma message ("REMINDER: need to implement refresh")
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        break;
    case NM_QUERY_SNVT:
        HandleNMQuerySIData(appReceiveParamPtr, apduPtr);
        break;
    case NM_NV_FETCH:
        HandleNMNVFetch(appReceiveParamPtr, apduPtr);
        break;
    case NM_MANUAL_SERVICE_REQUEST:
        /* This is unsolicited message from a node. Reference
           implementation ignores manual service request message from other nodes */
        break;
    default:
        /* This is where any message that is not taken care of should be
           handled. An example is product query command. For now, we treat
           everything else as unrecognized network management message. */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        break;
    }

	// Persist any changes to NVM
	LCS_WriteNvm();

    DeQueue(&gp->appInQ);
}

/******************************************************************************/
