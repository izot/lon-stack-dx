//
// lcs_netmgmt.c
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
 Reference: Section 10, ISO/IEC 14908-1

 Purpose:   LON App Layer/Network Management.  The functions in this file 
 handle the LON network management messages (HandleNM()) and LON network
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
#if PROCESSOR_IS(MC200)
#include <wmstdio.h>
#endif

#include <stdio.h>
#include <string.h>
#include "IzotApi.h"
#include "lcs_eia709_1.h"
#include "lcs_node.h"
#include "lcs_app.h"
#include "lcs_api.h"
#include "lcs_netmgmt.h"
#include "iup.h"
#include "IzotCal.h"
#include "endian.h"
#include "err.h"

#ifdef SECURITY_II
#include "SecNmMsgs.h"
#endif

/*------------------------------------------------------------------------------
 Section: Constant Definitions
 ------------------------------------------------------------------------------*/
#define BROADCAST_PREFIX       0xEFC00000
#define IP_ADDRESS_LEN         4
#define MAX_NV_LEN_SUPPORTED   228
#define IBOL_FINISH            0xFF

/*------------------------------------------------------------------------------
 Section: Type Definitions
 ------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Section: Globals
 ------------------------------------------------------------------------------*/
extern IzotUbits16 AnnounceTimer;
extern IzotUbits16 AddrMappingAgingTimer;

/*------------------------------------------------------------------------------
 Section: Static
 ------------------------------------------------------------------------------*/
static EEPROM save;
static unsigned    DmfWindowAddress;
static unsigned    DmfWindowSize;

/*------------------------------------------------------------------------------
 Section: Function Prototypes.
 ------------------------------------------------------------------------------*/

/* External function from physical layer to get the transceiver status. */
extern void IzotWink(void);
extern void IzotOffline(void);
extern void IzotOnline(void);
extern IzotApiError IzotMemoryRead(
const unsigned address, const unsigned size, void* const pData
);
extern IzotApiError IzotMemoryWrite(
const unsigned address, const unsigned size, const void* const pData
);

/*------------------------------------------------------------------------------
 Section: Function Definitions
 ------------------------------------------------------------------------------*/

/*******************************************************************************
 Function:  inRange
 Purpose:   To check the address within the DMF window range.
 Comments:  None.
 *******************************************************************************/
static int inRange(unsigned addr, unsigned size) 
{
    return (addr >= DmfWindowAddress && 
    addr + size <= DmfWindowAddress + DmfWindowSize);
}

/*******************************************************************************
 Function:  setMem
 Purpose:   To set the DMF window address and size.
 Comments:  None.
 *******************************************************************************/
void setMem(const unsigned addr, const unsigned size) 
{
    DmfWindowAddress = addr;
    DmfWindowSize = size;
}

/*******************************************************************************
 Function:  RecomputeChecksum
 Returns:   None.
 Reference: None
 Purpose:   To compute the configuration checksum and store.
 Comments:  None.
 *******************************************************************************/
void RecomputeChecksum(void) 
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
IzotByte ManualServiceRequestMessage(void) {
    NWSendParam *nwSendParamPtr;
    APDU *apduRespPtr;

    if (QueueFull(&gp->nwOutQ)) {
        DBG_vPrintf(TRUE, "No room for service pin in NW queue");
        return (FALSE); /* Can't send it now. Try later. */
    }

    /* Send unack domain wide broadcast message. */
    nwSendParamPtr = QueueTail(&gp->nwOutQ);
    nwSendParamPtr->pduSize = 
                            1 + IZOT_UNIQUE_ID_LENGTH + IZOT_PROGRAM_ID_LENGTH;
    if (nwSendParamPtr->pduSize > gp->nwOutBufSize) {
        return (FALSE); /* Do not have sufficient space to send the message. */
    }
    apduRespPtr = (APDU *) (nwSendParamPtr + 1);
    apduRespPtr->code.allBits = 0x7F; /* Manual Service Request. */
    nwSendParamPtr->destAddr.dmn.domainIndex = FLEX_DOMAIN;
    nwSendParamPtr->destAddr.dmn.domainLen = 0;
    nwSendParamPtr->pduType = APDU_TYPE;
    nwSendParamPtr->destAddr.addressMode = AM_BROADCAST;
    nwSendParamPtr->destAddr.addr.addr0.SubnetId = 0; /* Domain wide broadcast. */
    nwSendParamPtr->deltaBL = 0;
    nwSendParamPtr->altPath = 0; /* don't use alternate path. */
    nwSendParamPtr->tag = MANUAL_SERVICE_REQ_TAG_VALUE;
    nwSendParamPtr->version = 0;

    memcpy(apduRespPtr->data, eep->readOnlyData.UniqueNodeId,
            IZOT_UNIQUE_ID_LENGTH);
    memcpy(&(apduRespPtr->data[IZOT_UNIQUE_ID_LENGTH]), 
                eep->readOnlyData.ProgramId, IZOT_PROGRAM_ID_LENGTH);
    EnQueue(&gp->nwOutQ);
    gp->manualServiceRequest = FALSE;
    return (TRUE);
}

/*******************************************************************************
 Function:  NMNDRespond
 Returns:   None
 Reference: None
 Purpose:   Respond with success or failure code to current message.
 Can be called either for ND or NM messages.
 Comments:  None
 *******************************************************************************/
void NMNDRespond(NtwkMgmtMsgType msgType, Status success, 
APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotByte code;
    IzotByte subCode;
    int len = 0;
    IzotByte data[1];

    /* If service type is not request, nothing to do. */
    if (appReceiveParamPtr->service != IzotServiceRequest) {
        /* Does not make sense to respond if the original is not a request. */
        return;
    }

    if (msgType == NM_MESSAGE) {
        subCode = apduPtr->code.nm.nmCode;
        code = (success == SUCCESS ? NM_resp_success : NM_resp_failure)
                | subCode;
    } else {
        subCode = apduPtr->code.nd.ndCode;
        code = (success == SUCCESS ? ND_resp_success : ND_resp_failure)
                | subCode;
    }
    // Look for expanded commands.  
    // The response to these includes the sub-command
    if (subCode == 0) {
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
void HandleNDQueryUnconfig(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    /* Check for proper size of the message */
    if (appReceiveParamPtr->pduSize != 2) {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (!NodeUnConfigured()) {
        /* Not unconfigured - don't respond. */
        SendNullResponse(appReceiveParamPtr->reqId);
    } else {
        IzotByte code = ND_resp_success | (apduPtr->code.allBits & 0x0F);
        IzotByte data[IZOT_UNIQUE_ID_LENGTH + IZOT_PROGRAM_ID_LENGTH];
        memcpy(data, eep->readOnlyData.UniqueNodeId, IZOT_UNIQUE_ID_LENGTH);
        memcpy(&data[IZOT_UNIQUE_ID_LENGTH], eep->readOnlyData.ProgramId, 
        IZOT_PROGRAM_ID_LENGTH);
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
APDU *apduPtr) {
    XcvrParam transceiverStatus;
    IzotByte n;
    IzotByte response = ND_resp_success | (apduPtr->code.allBits & 0x0F);

    /* Check for proper size of the message. Regular pdusize is 1 byte.
     If this fn is called due to proxy request, then it is 2 bytes. */
    if (appReceiveParamPtr->pduSize > 2) {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    } else if (appReceiveParamPtr->pduSize == 2) {
        /* Make sure it is a proxy request. Or else, length is wrong. */
        if (apduPtr->code.nd.ndFlag == 0x5 && apduPtr->code.nd.ndCode
                == ND_PROXY_COMMAND && apduPtr->data[0] == 2) {
            ; /* It is indeed a proxy command. Length ok. Proceed. */
        } else {
            NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    }

    if (IZOT_GET_ATTRIBUTE(eep->configData, IZOT_CONFIG_COMM_TYPE) == 
    SPECIAL_PURPOSE) {
        transceiverStatus = appReceiveParamPtr->xcvrParams;
        n = IZOT_COMMUNICATIONS_PARAMETER_LENGTH;
    } else {
        n = 0;
        response = ND_resp_failure | (apduPtr->code.allBits & 0x0F);
    }

    SendResponse(appReceiveParamPtr->reqId, response, n, 
    transceiverStatus.data);
}

/*******************************************************************************
 Purpose:   Handle incoming NM Update Domain message.
 ******************************************************************************/
void HandleNMUpdateDomain(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotDomain *p = NULL, *pDomain = NULL;
    Status sts = FAILURE;
    
    if (appReceiveParamPtr->pduSize >= 2 + sizeof(IzotDomain)) {
        p = AccessDomain(apduPtr->data[0]);
        pDomain = (IzotDomain *) &apduPtr->data[1];
        sts = UpdateDomain(pDomain, apduPtr->data[0], true);
        RecomputeChecksum();
    }
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
        
    if (sts == SUCCESS && apduPtr->data[0] == 0) 
    {
        if (pDomain->Subnet != 0 && p->Subnet != pDomain->Subnet) 
        {
            uint32_t oldaddr = AM_BROADCAST_PREFIX | p->Subnet;
            uint32_t newaddr = AM_BROADCAST_PREFIX | pDomain->Subnet;
            RemoveIPMembership(oldaddr);
            AddIpMembership(newaddr);
        }
    }
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
 ******************************************************************************/
void HandleNMLeaveDomain(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    /* Fail if message is not 2 bytes long */
    if (appReceiveParamPtr->pduSize != 2) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* If the domain index is bad, fail */
    if (apduPtr->data[0] != 0 && apduPtr->data[0] != 1) {
        LCS_RecordError(IzotInvalidDomain);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Leave the domain */
    memset(&eep->domainTable[apduPtr->data[0]], 0xFF,
            sizeof(eep->domainTable[0]));
    memcpy(eep->domainTable[apduPtr->data[0]].Id, "gmrdwf", DOMAIN_ID_LEN);
    eep->domainTable[apduPtr->data[0]].Subnet = 0;
    IZOT_SET_ATTRIBUTE(eep->domainTable[apduPtr->data[0]], IZOT_DOMAIN_NODE, 0);
    
    /* Recompute the configuration checksum */
    RecomputeChecksum();

    /* If message not received on domain just left, then respond */
    if (apduPtr->data[0] != appReceiveParamPtr->srcAddr.dmn.domainIndex) {
        NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
    }

    /* If not a member of any domain, go unconfigured and reset. */
    if ((!IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS) 
        && IZOT_GET_ATTRIBUTE(eep->domainTable[0], IZOT_DOMAIN_INVALID))
        || (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS) 
        && IZOT_GET_ATTRIBUTE(eep->domainTable[0], IZOT_DOMAIN_INVALID)
        && IZOT_GET_ATTRIBUTE(eep->domainTable[1], IZOT_DOMAIN_INVALID))) {
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, 
                            IZOT_READONLY_NODE_STATE, IzotApplicationUnconfig);
        nmp->resetCause = IzotSoftwareReset;
        gp->resetNode = TRUE; /* Scheduler will reset the node */
    }
}

/*******************************************************************************
 Purpose:   Handle incoming NM Update Address message.
 *******************************************************************************/
void HandleNMUpdateKey(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    int i;

    /* Fail if message is not of correct length or domain index is bad. */
    if (appReceiveParamPtr->pduSize != 2 + IZOT_AUTHENTICATION_KEY_LENGTH) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    if (apduPtr->data[0] != 0 && apduPtr->data[0] != 1) {

        LCS_RecordError(IzotInvalidDomain);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    for (i = 0; i < IZOT_AUTHENTICATION_KEY_LENGTH; i++) {
        eep->domainTable[apduPtr->data[0]].Key[i] += apduPtr->data[i + 1];
    }
    RecomputeChecksum();
    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Purpose:   Handle incoming NM Update Address message.
 ******************************************************************************/
void HandleNMUpdateAddr(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Status sts = FAILURE;
    uint32_t oldaddr = 0, newaddr;
    IzotByte removeFlag = 0;
    IzotUbits16 indexIn;
    
    /* Check for incorrect size. Not sure why IzotAddressUnassigned tolerates 
    a missing last byte.  Probably legacy from a LB bug. */
    if (appReceiveParamPtr->pduSize >= 
    (apduPtr->data[1] == IzotAddressUnassigned ? 6 : 7)) {
        indexIn = apduPtr->data[0];
        
        //Backup old group id
        if (IZOT_GET_ATTRIBUTE(eep->addrTable[indexIn].Group, 
        IZOT_ADDRESS_GROUP_TYPE) == 1) 
        {
            oldaddr = 0xEFC00100 | eep->addrTable[indexIn].Group.Group;
            removeFlag = 1;
        }
        
        sts = UpdateAddress((const IzotAddress *)&apduPtr->data[1], 
                                                            apduPtr->data[0]);
            
        // Get the new group id
        newaddr = 0xEFC00100 | eep->addrTable[indexIn].Group.Group;
        if (IZOT_GET_ATTRIBUTE(eep->addrTable[indexIn].Group, 
        IZOT_ADDRESS_GROUP_TYPE) == 1 && oldaddr != newaddr) 
        {
            if (removeFlag) 
            {
                RemoveIPMembership(oldaddr);
            }
            AddIpMembership(newaddr);
        }
        
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
 ******************************************************************************/
void HandleNMQueryAddr(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    if (appReceiveParamPtr->pduSize < 2) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Fail if the address table index is bad and set statistics. */
    if (apduPtr->data[0] >= eep->readOnlyData.Extended) {
        LCS_RecordError(IzotInvalidAddrTableIndex);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Send response */
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_QUERY_ADDR,
            sizeof(IzotAddress), (IzotByte*) AccessAddress(apduPtr->data[0]));
}

/*******************************************************************************
 Function:  HandleNMQueryNvCnfg
 Returns:   None
 Reference: None
 Purpose:   Handle incoming NM Query Netvar Config message.
 Comments:  None.
 ******************************************************************************/
void HandleNMQueryNvCnfg(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Queue *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduRespPtr;
    IzotUbits16 n;
    AliasStruct          alias_config;
    
    /* Fail if the request does not have correct size */
    if (appReceiveParamPtr->pduSize != 2 && appReceiveParamPtr->pduSize != 4) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    n = apduPtr->data[0];
    if (n == 255 && appReceiveParamPtr->pduSize != 4) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Decode index */

    /* In implementations which handle a maximum number of network variables that is less
     * than 255, it is not necessary to check for network variable escapes of 255. */
    if (n == 255) {
        n = (IzotUbits16) apduPtr->data[1];
        n = (n << 8) | apduPtr->data[2];
    }

    /* Fail if there is insufficient space to send the response */
    if ((n < nmp->nvTableSize && (1 + sizeof(NVStruct)) > gp->tsaRespBufSize)
            || (n >= nmp->nvTableSize && n < nmp->nvTableSize
                    + NV_ALIAS_TABLE_SIZE && (1 + sizeof(AliasStruct))
                    > gp->tsaRespBufSize)) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Send response */
    tsaOutQPtr = &gp->tsaRespQ;
    tsaSendParamPtr = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);
    apduRespPtr->code.allBits = NM_resp_success | NM_QUERY_NV_CNFG;
    if (n < nmp->nvTableSize) {
        /* Copy the NVStruct entry */
        tsaSendParamPtr->apduSize = 1 + sizeof(NVStruct);
        memcpy(apduRespPtr->data, &(eep->nvConfigTable[n]), sizeof(NVStruct));

    } else if (n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE) {
        /* Copy the alias table entry. */
        tsaSendParamPtr->apduSize = 1 + sizeof(AliasStruct);
        n = n - nmp->nvTableSize;
        memcpy(&alias_config.nvConfig, &eep->nvAliasTable[n].Alias, 
        sizeof(NVStruct));
        alias_config.primary                = eep->nvAliasTable[n].Primary;
        alias_config.hostPrimary            = 0xFFFF;
        
        memcpy(apduRespPtr->data, &alias_config, sizeof(AliasStruct));
    } else {
        LCS_RecordError(IzotInvalidDatapointIndex);
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
void HandleNMNVFetch(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Queue *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduRespPtr;
    IzotUbits16 n;
    IzotByte i;

    /* Check if the message has correct size. If not, fail. */
    if (appReceiveParamPtr->pduSize != 2 && appReceiveParamPtr->pduSize != 4) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    n = apduPtr->data[0];
    if (n == 255 && appReceiveParamPtr->pduSize != 4) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    tsaOutQPtr = &gp->tsaRespQ;
    /* Send response */
    tsaSendParamPtr = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);

    if (n == 255) {
        n = apduPtr->data[1];
        n = (n << 8) | apduPtr->data[2];
        memcpy(apduRespPtr->data, apduPtr->data, 3); /* copy the index */
        i = 3; /* index where value of nv is copied */
    } else {
        apduRespPtr->data[0] = (char) n;
        i = 1;
    }
    if (n < nmp->nvTableSize) {
        /* Make sure there is sufficient space for the response. Else, fail. */
        if (nmp->nvFixedTable[n].nvLength + i + 1 > gp->tsaRespBufSize) {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }

        IzotByte ndi[nmp->nvFixedTable[n].nvLength];
        IzotPrepareNetworkData(ndi, n, nmp->nvFixedTable[n].nvLength, (IzotByte *)nmp->nvFixedTable[n].nvAddress);
        memcpy(&apduRespPtr->data[i], ndi, nmp->nvFixedTable[n].nvLength);
        tsaSendParamPtr->apduSize = nmp->nvFixedTable[n].nvLength + i + 1;
        apduRespPtr->code.allBits = NM_resp_success | NM_NV_FETCH;
        EnQueue(tsaOutQPtr);

    } else {
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
void HandleNMQuerySIData(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Queue *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduRespPtr;
    IzotUbits16 offset;
    IzotByte count;

    tsaOutQPtr = &gp->tsaRespQ;

    /* Fail if message is not 4 bytes long */
    if (appReceiveParamPtr->pduSize != 4) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Decode offset and count */
    offset = apduPtr->data[0];
    offset = (offset << 8) | apduPtr->data[1];
    count = apduPtr->data[2];
    /* Check if we have enough space to respond for this message */
    if (count + 1 > gp->tsaRespBufSize) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Send response */
    tsaSendParamPtr = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);
    apduRespPtr->code.allBits = NM_resp_success | NM_QUERY_SNVT;
    tsaSendParamPtr->apduSize = 1 + count;
    
    if((offset + count) < OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT)
    {
        memcpy(apduRespPtr->data, offset + (char *) &(nmp->snvt), count);
    }
    else if(offset < OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT)
    {
        memcpy(apduRespPtr->data, offset + (char *) &(nmp->snvt), 
                            (OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT - offset));
        memcpy(apduRespPtr->data + 
                OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT - offset, 
                (char *) (nmp->snvt.sb), 
                (count - (OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT - offset)));
    }
    else
    {
        memcpy(apduRespPtr->data, (char *) ((nmp->snvt.sb) +  
                   (offset - OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT)), count);
    }
    
    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
 Function:  HandleNMWink
 Returns:   None
 Reference: None
 Purpose:   Handle incoming NM Wink message.
 Comments:  None.
 *******************************************************************************/
void HandleNMWink(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Queue *tsaQueuePtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduRespPtr;
    IzotBits8 subcmd;
    IzotBits8 niIndex = 0;
    IzotUbits16 offset = 0;
    IzotByte count = 0;
    subcmd = 0;

    if (appReceiveParamPtr->pduSize > 1) {
        subcmd = apduPtr->data[0];
    }

    if (appReceiveParamPtr->pduSize > 2) {
        niIndex = apduPtr->data[1];
    }

    if (appReceiveParamPtr->pduSize > 5) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    
    if (appReceiveParamPtr->pduSize <= 1 || subcmd == 0) {
        if (appReceiveParamPtr->service != IzotServiceRequest) {
            /* Any service except request/response */
            IzotWink(); /* Simple Wink */
        } else {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        }
        return;
    }

    if (appReceiveParamPtr->pduSize == 5 || subcmd == 7) {
        if (appReceiveParamPtr->service != IzotServiceRequest) 
        {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
        
        offset = (IzotUbits16) apduPtr->data[1];
        offset = (offset << 8) | apduPtr->data[2];
        count = apduPtr->data[3];
        
        tsaQueuePtr = &gp->tsaRespQ;
        tsaSendParamPtr = QueueTail(tsaQueuePtr);
        tsaSendParamPtr->altPathOverride = FALSE;
        tsaSendParamPtr->service = IzotServiceResponse;
        tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
        tsaSendParamPtr->nullResponse = FALSE;
        tsaSendParamPtr->flexResponse = FALSE;
        apduRespPtr = (APDU *) (tsaSendParamPtr + 1);
        
        if(count + 1 <= gp->tsaRespBufSize)
        {
            IzotByte data[sizeof(AliasField) + sizeof(SIHeaderExt) + 
            sizeof(SNVTCapabilityInfo)] = {0};
            
            memcpy(&data[0], (IzotByte *)nmp->snvt.aliasPtr, 
                                                         sizeof(AliasField));
            memcpy(&data[sizeof(AliasField)], (IzotByte *) si_header_ext,
                                                        sizeof(SIHeaderExt));
            memcpy(&data[sizeof(AliasField) + sizeof(SIHeaderExt)], 
              (IzotByte *) snvt_capability_info, sizeof(SNVTCapabilityInfo));              
            tsaSendParamPtr->apduSize = count + 1;
            apduRespPtr->code.allBits = NM_resp_success | NM_WINK;
            memcpy(&apduRespPtr->data[0], &data[offset], count);
        }
        else
        {
            tsaSendParamPtr->apduSize = 1;
            apduRespPtr->code.allBits = NM_resp_failure | NM_WINK;
        }
        EnQueue(tsaQueuePtr);
        return;
    }
    
    /* must be requesting SEND_ID_INFO. */
    if (appReceiveParamPtr->service != IzotServiceRequest) {
        return; /* The message should be a request. */
    }

    tsaQueuePtr = &gp->tsaRespQ;
    tsaSendParamPtr = QueueTail(tsaQueuePtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    /* Note: This implementation only has one NI Interface. i.e 0 */
    /* Send response. */
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = FALSE;
    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);

    if (niIndex == 0 && IZOT_UNIQUE_ID_LENGTH + IZOT_PROGRAM_ID_LENGTH + 2
            <= gp->tsaRespBufSize) {
        tsaSendParamPtr->apduSize = 
                        IZOT_PROGRAM_ID_LENGTH + IZOT_UNIQUE_ID_LENGTH + 2;
        apduRespPtr->data[0] = 0; /* Interface not down. */
        apduRespPtr->code.allBits = NM_resp_success | NM_WINK;
        memcpy(&apduRespPtr->data[1], eep->readOnlyData.UniqueNodeId,
                IZOT_UNIQUE_ID_LENGTH);
        memcpy(&(apduRespPtr->data[1 + IZOT_UNIQUE_ID_LENGTH]),
                eep->readOnlyData.ProgramId, IZOT_PROGRAM_ID_LENGTH);
    } else {
        tsaSendParamPtr->apduSize = 1;
        apduRespPtr->code.allBits = NM_resp_failure | NM_WINK;
    }
    EnQueue(tsaQueuePtr);
}

/*******************************************************************************
 Function:  HandleNMRefresh
 Purpose:   Handle incoming NM Refresh
 *******************************************************************************/
void HandleNMRefresh(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotUbits16 offset = 
                    (IzotUbits16) apduPtr->data[0] * 256 + apduPtr->data[1];
    IzotByte offchip = apduPtr->data[3];
    IzotByte result = NM_resp_failure;

    // The best thing to do here would be to send a request to the Neuron, get it's response
    // and then relay that.  That's a pretty big deviation from how things work so we'll just
    // hardcode the limit now based on the known Neuron size of 4K and page size of 16.
    if (!offchip && offset * 16 < 4096) {
        result = NM_resp_success;
    }

    SendResponse(appReceiveParamPtr->reqId, 
    result | NM_MEMORY_REFRESH, 0, NULL);
}

/*******************************************************************************
 Function:  HandleNmeQueryVersion
 Purpose:   Handle incoming NME Query Version
 *******************************************************************************/
void HandleNmeQueryVersion(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    struct {
        IzotByte subcommand;
        IzotByte version;
        IzotByte capabilitiesHi;
        IzotByte capabilitiesLo;
    } queryVersion;

    queryVersion.subcommand = apduPtr->data[0];
    queryVersion.version = 2;
    queryVersion.capabilitiesHi = NMV_LS_MODE_COMPATIBILITY_OR_ENHANCED | NMV_LSIP_ADDR_MAPPING_ANNOUNCEMENTS;
    queryVersion.capabilitiesLo = NMV_OMA | NMV_PROXY | NMV_SSI | NMV_INITCONFIG | NMV_UPDATE_NV_BY_INDEX;
#ifdef SECURITY_II
	queryVersion.capabilitiesLo |= NVM_SECURITY_II;
#endif
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(queryVersion), 
    (IzotByte*) &queryVersion);
}

/*******************************************************************************
 Function:  HandleNmeUpdateDomain
 Purpose:   Handle incoming NME Join Domain No Key
 *******************************************************************************/
void HandleNmeUpdateDomain(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Status sts = FAILURE;
    if (appReceiveParamPtr->pduSize >= 
    3 + sizeof(IzotDomain) - IZOT_AUTHENTICATION_KEY_LENGTH) {
        sts = UpdateDomain((IzotDomain *) &apduPtr->data[2],
                apduPtr->data[1], false);
        RecomputeChecksum();
    }
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleNmeReportDomain
 Purpose:   Handle incoming NME Report Domain No Key
 *******************************************************************************/
void HandleNmeReportDomain(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotDomain *p = AccessDomain(apduPtr->data[1]);
    struct {
        IzotByte subcommand;
        LogicalAddress address;
    } reportDomain;

    if (appReceiveParamPtr->pduSize < 3) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
    } else if (p == NULL) {
        LCS_RecordError(IzotInvalidDomain);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
    } else {
        reportDomain.subcommand = apduPtr->data[0];
        memcpy(&reportDomain.address, p, sizeof(reportDomain.address));
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED,
                sizeof(reportDomain), (IzotByte*) &reportDomain);
    }
}

/*******************************************************************************
 Function:  HandleNmeReportKey
 Purpose:   Handle incoming NME Report Key
 *******************************************************************************/
void HandleNmeReportKey(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotByte i;
    struct {
        IzotByte subcommand;
        IzotByte key[OMA_KEY_LEN];
    } reportKey;

    for (i = 0; i < 2; i++) {
        IzotDomain *p = AccessDomain(i);
        if (p == NULL) {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            break;
        }
        memcpy(&reportKey.key[i * IZOT_AUTHENTICATION_KEY_LENGTH], 
        p->Key, IZOT_AUTHENTICATION_KEY_LENGTH);
    }
    if (i == 2) {
        reportKey.subcommand = apduPtr->data[0];
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED,
                sizeof(reportKey), (IzotByte*) &reportKey);
    }
}

/*******************************************************************************
 Function:  HandleNmeUpdateKey
 Purpose:   Handle incoming NME Update Key
 *******************************************************************************/
void HandleNmeUpdateKey(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Status sts = FAILURE;
    if (appReceiveParamPtr->pduSize >= 3 + OMA_KEY_LEN && AccessDomain(1)
            != NULL) {
        IzotByte i;
        IzotByte increment = apduPtr->data[1] == 1;
        IzotByte* pKey = &apduPtr->data[2];

        for (i = 0; i < 2; i++) {
            int j;
            IzotDomain *p = AccessDomain(i);
            if (!increment) {
                memset(p->Key, 0, sizeof(p->Key));
            }
            for (j = 0; j < IZOT_AUTHENTICATION_KEY_LENGTH; j++) {
                p->Key[j] += *pKey++;
            }
            sts = UpdateDomain(p, i, true);
            if (sts == FAILURE) {
                // This should never happen...
                break;
            }
        }
        RecomputeChecksum();
    }
    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleNmeInitConfig
 Purpose:   Handle incoming NME Init Config
 *******************************************************************************/
void HandleNmeInitConfig(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    int i;
    UInt16 selectorVal = 0x3FFF;
    Status sts = SUCCESS;

    if (appReceiveParamPtr->pduSize <= 2) {
        sts = FAILURE;
    } else {
        int len = apduPtr->data[1];
        IzotByte nvAuth = false;

        // Initialize configuration as follows:
        // 1. Init all address table entries to unbound
        // 2. Set all aliases to unbound
        // 3. Set all NV selectors to unbound
        // 4. Set NV auth property based on passed in data as follows:
        //  a. If length is 0, all are un-auth
        //  b. If length is non-zero and bit is defined, use that.
        //  c. Otherwise use the last defined bit.
        LCS_InitAddress();
        LCS_InitAlias();

        for (i = 0; i < nmp->nvTableSize; i++, selectorVal--) {
            IzotDatapointConfig *p = &eep->nvConfigTable[i];
            IZOT_SET_ATTRIBUTE_P(p, IZOT_DATAPOINT_SELHIGH, 
            (IzotByte) (selectorVal >> 8));
            p->SelectorLow = selectorVal & 0xFF;
            if (i / 8 < len) {
                nvAuth = (apduPtr->data[2 + (i / 8)] & (1 << (i % 8))) ? true
                        : false;
            }
            IZOT_SET_ATTRIBUTE_P(p, IZOT_DATAPOINT_AUTHENTICATION, nvAuth);
        }

        RecomputeChecksum();
    }

    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleNmeUpdateNvCnfg
 Purpose:   Handle incoming NM Expanded update NV Config message
 *******************************************************************************/
void HandleNmeUpdateNvCnfg(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotUbits16           n; /* for nv index */
    IzotByte            pduSize;
    IzotDatapointConfig    *np;
    if (appReceiveParamPtr->pduSize < 6)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
         return;
    }

    /* Decode index */
    n = (IzotUbits16)apduPtr->data[1];
    n = (n << 8) | apduPtr->data[2];
    pduSize = sizeof(IzotDatapointConfig) + 2; /* regular update */
    np = (IzotDatapointConfig *)(&apduPtr->data[3]);
    /* Update nv config or alias table */
    if (n < nmp->nvTableSize)
    {
        if (appReceiveParamPtr->pduSize >= pduSize)
        {
            memcpy(&eep->nvConfigTable[n], np, sizeof(IzotDatapointConfig));
        }
        else
        {
            /* Incorrect size */
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
             return;
        }
    } else
    {
    /* Invalid nv table index */
        LCS_RecordError(IzotInvalidDatapointIndex);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
         return;
    }
    /* Recompute checksum and send response */
    RecomputeChecksum();
    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleNmeQueryNvCnfg
 Purpose:   Handle incoming NM Expanded query NV config message
 *******************************************************************************/
void HandleNmeQueryNvCnfg(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotUbits16            n;

    struct 
    {
        IzotByte             subcommand;
        IzotDatapointConfig    nv_config;
    } query_nv_config;
    
    /* Fail if the request does not have correct size */
    if ((appReceiveParamPtr->pduSize) != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    n = (IzotUbits16)apduPtr->data[1];
    n = (n << 8) | apduPtr->data[2];

    /* Fail if there is insufficient space to send the response */
    if (n < nmp->nvTableSize && 
    (1 + sizeof(IzotDatapointConfig)) > gp->tsaRespBufSize )
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (n < nmp->nvTableSize)
    {
        query_nv_config.subcommand = apduPtr->data[0];
        memcpy(&query_nv_config.nv_config, &eep->nvConfigTable[n], 
        sizeof(IzotDatapointConfig));
        
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, 
        sizeof(query_nv_config), (IzotByte*)&query_nv_config);
    }
    else
    {
        LCS_RecordError(IzotInvalidDatapointIndex);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
    }
}

/*******************************************************************************
 Function:  HandleNmeUpdateNvAliasCnfg
 Purpose:   Handle incoming NM Expanded update alias message
 *******************************************************************************/
void HandleNmeUpdateNvAliasCnfg(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotUbits16            n,pduSize;
        
    if (appReceiveParamPtr->pduSize < 10)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    
    n = (IzotUbits16)apduPtr->data[1];
    n = (n << 8) | apduPtr->data[2];
    
    pduSize = sizeof(IzotAliasConfig) + 4;
    
    if (n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        /* Update the nv alias table */
        if (appReceiveParamPtr->pduSize >= pduSize)
        {
            memcpy(&eep->nvAliasTable[n], 
            (IzotAliasConfig *)(&apduPtr->data[3]), sizeof(IzotAliasConfig));
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
        LCS_RecordError(IzotInvalidDatapointIndex);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Recompute checksum and send response */
    RecomputeChecksum();
    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleNmeQueryNvAliasCnfg
 Purpose:   Handle incoming NM Expanded query alias message
 *******************************************************************************/
void HandleNmeQueryNvAliasCnfg(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotUbits16 n;

    struct 
    {
        IzotByte           subcommand;
        IzotAliasConfig    alias_config;
    } query_alias_config;
    
    /* Fail if the request does not have correct size */
    if (appReceiveParamPtr->pduSize != 4)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    n = (IzotUbits16) apduPtr->data[1];
    n = (n << 8) | apduPtr->data[2];
    
    /* Fail if there is insufficient space to send the response */
    if ((n >= nmp->nvTableSize && n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE &&
                (1 + sizeof(IzotAliasConfig)) > gp->tsaRespBufSize )
    )
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    if (n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        /* Copy the alias table entry. */
        query_alias_config.subcommand = apduPtr->data[0];
        memcpy(&query_alias_config.alias_config, &eep->nvAliasTable[n], 
        sizeof(IzotAliasConfig));
        
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, 
        sizeof(query_alias_config), (IzotByte*)&query_alias_config);
    }
    else
    {
        LCS_RecordError(IzotInvalidDatapointIndex);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
    }
}

/*******************************************************************************
 Function:  HandleNmeQueryLsAddrMapping
 Purpose:   Handle incoming query mapping table message
 *******************************************************************************/
void HandleNmeQueryLsAddrMapping(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    struct {
        IzotByte subcommand;
        uint32_t AnnouncePeriod;
        uint32_t AgePeriod;
    } reportLsAddrMapping;
    
    /* Fail if the request does not have correct size */
    if ((appReceiveParamPtr->pduSize) != 2)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    reportLsAddrMapping.subcommand = apduPtr->data[0];
    reportLsAddrMapping.AnnouncePeriod = AnnounceTimer;
    reportLsAddrMapping.AgePeriod = AddrMappingAgingTimer;
    
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED,
    sizeof(reportLsAddrMapping), (IzotByte*) &reportLsAddrMapping);
}

/*******************************************************************************
 Function:  HandleNmeQueryIpAddress
 Purpose:   Handle incoming query IP address message
 *******************************************************************************/
void HandleNmeQueryIpAddress(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    struct {
        IzotByte subcommand;
        IzotByte ipAddress[IP_ADDRESS_LEN];
    } reportIpAddress;
    
    /* Fail if the request does not have correct size */
    if ((appReceiveParamPtr->pduSize) != 2)
    {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
     }
    
    reportIpAddress.subcommand = apduPtr->data[0];
    reportIpAddress.ipAddress[0] = ownIpAddress[0];
    reportIpAddress.ipAddress[1] = ownIpAddress[1];
    reportIpAddress.ipAddress[2] = ownIpAddress[2];
    reportIpAddress.ipAddress[3] = ownIpAddress[3];
    
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED,
                sizeof(reportIpAddress), (IzotByte*) &reportIpAddress);
}

/*******************************************************************************
 Function:  HandleNmeUpdateNvByIndex
 Purpose:   Handle incoming Update NV by index message
 *******************************************************************************/
void HandleNmeUpdateNvByIndex(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
    IzotUbits16          index = -1;
    IzotUbits16          dataLength, matchingDataLength;
    IzotUbits16          matchingPrimaryIndex;
    IzotDatapointConfig *matchingNVStrPtr;
    IzotByte             authOK;
    IzotByte             err = NM_resp_success;
    struct {
        unsigned char code;     // 0x20 pass // 0x00 fail
        unsigned char subcode;  // 0x02
    } update_nv_response;

    update_nv_response.subcode =  NME_UPDATE_NV_BY_INDEX;
    
    if (appReceiveParamPtr->pduSize < 5) {
        // The message does not have any correct size or data field.
        LCS_RecordError(IzotDatapointMsgTooShort);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotNoApplicationUnconfig) {
        err = NM_resp_failure;
    }

    dataLength = appReceiveParamPtr->pduSize - 4;       // data length in message
    index = (apduPtr->data[1] << 8) | apduPtr->data[2]; //Decode index

    matchingPrimaryIndex = GetPrimaryIndex(index);
    matchingDataLength   = NV_LENGTH(matchingPrimaryIndex);
    // If the data size does not match, don't update. ignore.
    if (dataLength != matchingDataLength) {
        LCS_RecordError(IzotDatapointLengthMismatch);
        err = NM_resp_failure;
    }
    
    matchingNVStrPtr = GetNVStructPtr(matchingPrimaryIndex);
    if (err == NM_resp_success && IZOT_GET_ATTRIBUTE_P(matchingNVStrPtr, IZOT_DATAPOINT_DIRECTION) == 
    IzotDatapointDirectionIsOutput) {
        LCS_RecordError(IzotDatapointUpdateOnOutput);
        err = NM_resp_failure;
    }

    if (appReceiveParamPtr->auth || !IZOT_GET_ATTRIBUTE_P(matchingNVStrPtr, IZOT_DATAPOINT_AUTHENTICATION)) {
        authOK = TRUE;
    } else {
        authOK = FALSE;
    }

    if (!authOK) {
        LCS_RecordError(IzotAuthenticationMismatch);
        err = NM_resp_failure;
    }
    
    if(err == NM_resp_success) {
        IzotUbits16 dplength = dataLength;
        IzotByte hdi[MAX_NV_LEN_SUPPORTED];

        if (izot_dp_prop[matchingPrimaryIndex].ibolSeq) {
            int byte_index = 0;
            const IzotByte *ibol_seq = izot_dp_prop[matchingPrimaryIndex].ibolSeq;

            IzotNdiToHdi(&apduPtr->data[3], hdi, ibol_seq);
            dplength = 0; //Back to zero , updating as per original structured size
            while(ibol_seq[byte_index] != IBOL_FINISH) {
                if(ibol_seq[byte_index] & 0x80) {
                    byte_index++;
                }
                byte_index++;
                dplength += ibol_seq[byte_index++] * ibol_seq[byte_index++];
            }
            memcpy(&apduPtr->data[3], &hdi, dplength);
        }

        memcpy(NV_ADDRESS(matchingPrimaryIndex), &apduPtr->data[3], dplength);

        if (AppPgmRuns()) {
            DBG_vPrintf(TRUE, "ProcessNVUpdate: Notify Application Program\n");
            // Notify application program only if it is running.
            IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_DOMAIN, appReceiveParamPtr->srcAddr.dmn.domainIndex);
            IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_FLEX, (appReceiveParamPtr->srcAddr.dmn.domainIndex == 
            FLEX_DOMAIN));
            
            memcpy(&gp->nvInAddr.Source, &appReceiveParamPtr->srcAddr.subnetAddr, sizeof(gp->nvInAddr.Source));

            switch (appReceiveParamPtr->srcAddr.addressMode) {
            case AM_BROADCAST:
                IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_FORMAT, 0);
            break;
            case AM_MULTICAST:
                IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_FORMAT, 1);
                gp->nvInAddr.Destination.Group.GroupId = appReceiveParamPtr->srcAddr.group.GroupId;
            break;
            case AM_SUBNET_NODE:
                IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_FORMAT, 2);
            break;
            case AM_UNIQUE_NODE_ID:
                IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_FORMAT, 3);
            break;
            default:
                // should not come here
                IZOT_SET_ATTRIBUTE(gp->nvInAddr, IZOT_RECEIVEADDRESS_FORMAT, 5); // unknown
            }

            IzotDatapointUpdateOccurred(matchingPrimaryIndex, &(gp->nvInAddr));
            if (IZOT_GET_ATTRIBUTE(izot_dp_prop[matchingPrimaryIndex], IZOT_DATAPOINT_PERSIST)) {
                LCS_WriteNvs();
            }
        }
    }
    SendResponse(appReceiveParamPtr->reqId, err | NM_EXPANDED, sizeof(update_nv_response), 
    (IzotByte*) &update_nv_response);
}

/*******************************************************************************
 Function:  HandleNMExpanded
 Purpose:   Handle incoming NM Expanded
 *******************************************************************************/
void HandleNMExpanded(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    if (appReceiveParamPtr->pduSize < 2) {
        // All expanded commands must at least include a sub-command
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
    } else {
        switch (apduPtr->data[0]) {
        case NME_QUERY_VERSION:
            HandleNmeQueryVersion(appReceiveParamPtr, apduPtr);
            break;
        case NME_UPDATE_NV_BY_INDEX:
            HandleNmeUpdateNvByIndex(appReceiveParamPtr, apduPtr);
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
        case NME_INIT_CONFIG:
            HandleNmeInitConfig(appReceiveParamPtr, apduPtr);
            break;
        case NME_UPDATE_NV_CONFIG:
            HandleNmeUpdateNvCnfg(appReceiveParamPtr, apduPtr);
            break;
        case NME_QUERY_NV_CONFIG:
            HandleNmeQueryNvCnfg(appReceiveParamPtr, apduPtr);
            break;
        case NME_UPDATE_NV_ALIAS_CONFIG:
            HandleNmeUpdateNvAliasCnfg(appReceiveParamPtr, apduPtr);
            break;
        case NME_QUERY_NV_ALIAS_CONFIG:
            HandleNmeQueryNvAliasCnfg(appReceiveParamPtr, apduPtr);
            break;
        case NME_QUERY_LS_ADDR_MAPPING_ANNOUNCEMENT:
            HandleNmeQueryLsAddrMapping(appReceiveParamPtr, apduPtr);
            break;
        case NME_QUERY_IP_ADDRESS:
            HandleNmeQueryIpAddress(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_INIT:
            HandleNmeIupInit(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_TRANSFER:
            HandleNmeIupTransfer(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_CONFIRM:
            HandleNmeIupConfirm(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_VALIDATE:
            HandleNmeIupValidate(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_SWITCHOVER:
            HandleNmeIupSwitchOver(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_STATUS:
            HandleNmeIupStatus(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_COMMIT:
            HandleNmeIupCommit(appReceiveParamPtr, apduPtr);
            break;
        case NME_IUP_ACK_TRANSFER:
			HandleNmeIupTransferAck(appReceiveParamPtr, apduPtr);
			break;
#ifdef SECURITY_II
		case NMEXP_SEC_ENROLL:
		case NMEXP_SEC_REENROLL:
			HandleNmeSecEnroll(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_SET_BAKP:
			HandleNmeSecSetBAKP(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_QUERY_BAKP:
			HandleNmeSecQueryBAKP(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_RELEASE_DEVICE:
			HandleNmeSecReleaseDevice(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_START_REKEY:
			HandleNmeSecStartReKey(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_REKEY_COMPLETE:
			HandleNmeSecRekeyComplete(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_REVOKE_KEY:
			HandleNmeSecRevokeKey(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_DEACTIVATE:
			HandleNmeSecDeactivate(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_QUERY_DEVICE_CERT:
			HandleNmeSecQueryDeviceCert(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_QUERY_SECURITY_STATE:
			HandleNmeSecQuerySecurityState(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_QUERY_SECURITY_STATS:
			HandleNmeSecQuerySecurityStats(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_CLEAR_SECURITY_STATS:
			HandleNmeSecClearSecurityStats(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_QUERY_IV_STATE:
			HandleNmeSecQueryIVState(appReceiveParamPtr, apduPtr);
			break;
		case NMEXP_SEC_QUERY_REPLAY:
			HandleNmeSecQueryReply(appReceiveParamPtr, apduPtr);
			break;
#endif
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
void HandleNMQueryId(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    NMQueryIdRequest *pid;
    char *memp;
    Queue *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduRespPtr;
    IzotByte allowed;
    IzotUbits16 offset;

    /* Fail if message does not have the correct size. Should be 2 or 6+n */
    if (appReceiveParamPtr->pduSize != 2 && appReceiveParamPtr->pduSize < 6) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    tsaOutQPtr = &gp->tsaRespQ;

    /* Init some fields here. Assume we may fail. */
    tsaSendParamPtr = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    tsaSendParamPtr->apduSize = 1;
    /* Since there are a lot of fail cases, init code to indicate failed
     response. */
    apduRespPtr->code.allBits = NM_resp_failure | NM_QUERY_ID;

    pid = (NMQueryIdRequest *) &apduPtr->data;
    offset = hton16(pid->offset);

    /* if optional fields are present, check that the data field has
     sufficient bytes. */
    if (appReceiveParamPtr->pduSize > 2 && appReceiveParamPtr->pduSize != (6
            + pid->count)) {
        /* The message does not have sufficient data or it has too much data. */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    switch (pid->selector) {
    case UNCONFIGURED:
        if (!NodeUnConfigured()) {
            /* Not unconfigured - don't respond. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        break;
    case SELECTED:
        if (!gp->selectQueryFlag) {
            /* Not selected - don't respond. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        break;
    case SELECTED_UNCFG: /* must be selected and unconfigured */
        if (!gp->selectQueryFlag) {
            /* Not selected - don't respond. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
        if (!NodeUnConfigured()) {
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
    if (appReceiveParamPtr->pduSize > 2) {
        switch (pid->mode) {
        case ABSOLUTE_MEM_ADDR:
            memp = (char *) nmp;
            if (offset >= 0xF000) {
                memp = (char *) eep - 0xF000;
            }
            break;
        case CONFIG_RELATIVE:
            memp = (char *) &(eep->configData);
            break;
        case READ_ONLY_RELATIVE:
            memp = (char *) &(eep->readOnlyData);
            break;
        default:
            EnQueue(tsaOutQPtr);
            return; /* Failed response. */
        }

        memp += offset;

        /* Absolute addressing to read snvt is not possible */
        allowed = (memp >= (char *) &eep->readOnlyData && memp
                + apduPtr->data[3] < (char *) &eep->domainTable[0]);
        if (!allowed) {
            EnQueue(tsaOutQPtr);
            return; /* Failed response. */
        }

        if (memcmp(pid->data, memp, pid->count) != 0) {
            /* Compare failed - don't reply. */
            tsaSendParamPtr->nullResponse = TRUE;
            EnQueue(tsaOutQPtr);
            return;
        }
    }

    /* Send response */
    if (1 + IZOT_UNIQUE_ID_LENGTH + IZOT_PROGRAM_ID_LENGTH <= 
    gp->tsaRespBufSize) {
        tsaSendParamPtr->apduSize = 
                             1 + IZOT_UNIQUE_ID_LENGTH + IZOT_PROGRAM_ID_LENGTH;
        apduRespPtr->code.allBits = NM_resp_success | NM_QUERY_ID;
        memcpy(apduRespPtr->data, eep->readOnlyData.UniqueNodeId,
                IZOT_UNIQUE_ID_LENGTH);
        memcpy(&(apduRespPtr->data[IZOT_UNIQUE_ID_LENGTH]),
                eep->readOnlyData.ProgramId, IZOT_PROGRAM_ID_LENGTH);
    }

    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
 Purpose:   Handle incoming NM Update Group Addr message.
 *******************************************************************************/
void HandleNMUpdateGroupAddr(APPReceiveParam *appReceiveParamPtr, 
APDU *apduPtr) {
    IzotByte addrIndex = 0;
    IzotAddressTableGroup *groupStrPtr;
    IzotAddress *ap;

    /* This message must be delivered with group addressing and is
     updated based on the domain in which it was received. Hence,
     flex domain is not allowed. */
    if (appReceiveParamPtr->srcAddr.addressMode != AM_MULTICAST
            || appReceiveParamPtr->srcAddr.dmn.domainIndex == FLEX_DOMAIN) {
        /* This message should be sent in AM_MULTICAST. Fail */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    if (appReceiveParamPtr->pduSize != 1 + sizeof(IzotAddress)) {
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
    groupStrPtr = (IzotAddressTableGroup *) &apduPtr->data[0];
    if (IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_TYPE) == 1) {
        addrIndex = AddrTableIndex(appReceiveParamPtr->srcAddr.dmn.domainIndex,
                appReceiveParamPtr->srcAddr.group.GroupId);
    }

    /* Make sure we got a good index. */
    if (IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_TYPE) != 
    1 || addrIndex == 0xFF) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    ap = AccessAddress(addrIndex); /* ap cannot be NULL */
    /* Only group size and timer values should be changed */
    IZOT_SET_ATTRIBUTE(ap->Group, IZOT_ADDRESS_GROUP_SIZE, 
    IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_SIZE));
    IZOT_SET_ATTRIBUTE(ap->Group, IZOT_ADDRESS_GROUP_REPEAT_TIMER, 
    IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_REPEAT_TIMER));
    IZOT_SET_ATTRIBUTE(ap->Group, IZOT_ADDRESS_GROUP_RETRY, 
    IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_RETRY));
    IZOT_SET_ATTRIBUTE(ap->Group, IZOT_ADDRESS_GROUP_RECEIVE_TIMER, 
    IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_RECEIVE_TIMER));
    IZOT_SET_ATTRIBUTE(ap->Group, IZOT_ADDRESS_GROUP_TRANSMIT_TIMER, 
    IZOT_GET_ATTRIBUTE_P(groupStrPtr, IZOT_ADDRESS_GROUP_TRANSMIT_TIMER));
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
void HandleNMQueryDomain(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotDomain *p = AccessDomain(apduPtr->data[0]);

    /* Fail if message does not have the correct size. */
    if (appReceiveParamPtr->pduSize != 2) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* If domain index is other than 0 or 1 or if the node is in only one
     domain and the index is 1, then fail. */
    if (p == NULL) {
        /* Domain index is bad. */
        LCS_RecordError(IzotInvalidDomain);
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }
    /* Send response */
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_QUERY_DOMAIN,
            sizeof(IzotDomain), (IzotByte*) p);
}

/*******************************************************************************
 Function:  HandleNMUpdateNvConfig
 Purpose:   Handle incoming NM Update NV Config message.
 *******************************************************************************/
void HandleNMUpdateNvConfig(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    IzotUbits16 n; /* for nv index */
    IzotByte pduSize;
    NVStruct *np;

    if (appReceiveParamPtr->pduSize < 5) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Decode index */

    n = apduPtr->data[0];
    if (n == 255) {
        n = (IzotUbits16) apduPtr->data[1];
        n = (n << 8) | apduPtr->data[2];
        if (n < nmp->nvTableSize) {
            pduSize = sizeof(NVStruct) + 4; /* escaped regular update */
        } else {
            /* Escaped alias update. Assume that host_primary is
             absent for now. */
            pduSize = (sizeof(AliasStruct) - 2) + 4;
        }
        np = (NVStruct *) (&apduPtr->data[3]);
    } else {
        if (n < nmp->nvTableSize) {
            pduSize = sizeof(NVStruct) + 2; /* regular update */
        } else {
            /* Alias update. Assume that host_primary is absent for now. */
            /* last 2 is for index + code */
            pduSize = (sizeof(AliasStruct) - 2) + 2;
        }
        np = (NVStruct *) (&apduPtr->data[1]);
    }

    /* Update nv config or alias table */
    if (n < nmp->nvTableSize) {
        if (appReceiveParamPtr->pduSize >= pduSize) {
            memcpy(&eep->nvConfigTable[n], np, sizeof(NVStruct));
            IZOT_SET_ATTRIBUTE(eep->nvConfigTable[n], IZOT_DATAPOINT_ADDRESS_HIGH, 0x0);
            IZOT_SET_ATTRIBUTE(eep->nvConfigTable[n], IZOT_DATAPOINT_AES, 0x0);
        } else {
            /* Incorrect size */
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    } else if (n < nmp->nvTableSize + NV_ALIAS_TABLE_SIZE) {
        n = n - nmp->nvTableSize; /* Alias table index */
        /* Check for various forms of alias update */
        if (((AliasStruct *) np)->primary == 0xFF
                && appReceiveParamPtr->pduSize == pduSize) {
            /* host_primary missing. default to 0xffff. Null alias update. */
            ((AliasStruct *) np)->hostPrimary = 0xffff;
        } else if (((AliasStruct *) np)->primary == 0xFF) {
            /* escaped alias. hostPrimary is present */
            pduSize += 2;
        }
        /* Update the nv alias table */
        if (appReceiveParamPtr->pduSize >= pduSize) {
            memcpy(&eep->nvAliasTable[n].Alias, &((AliasStruct *)np)->nvConfig, sizeof(NVStruct));
            IZOT_SET_ATTRIBUTE(eep->nvAliasTable[n].Alias, IZOT_DATAPOINT_ADDRESS_HIGH, 0x00);
            IZOT_SET_ATTRIBUTE(eep->nvAliasTable[n].Alias, IZOT_DATAPOINT_AES, 0x00);
            eep->nvAliasTable[n].Primary = (IzotUbits16)(((AliasStruct *)np)->primary);
        } else {
            /* Incorrect size */
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            return;
        }
    } else {
        /* Invalid nv table index */
        LCS_RecordError(IzotInvalidDatapointIndex);
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
 *    B) Incoming network variable updates are handled normally
 *    C) But nv_update_occurs events will be lost
 * 6) In all other states except configured:
 *    A) No response is returned on NV polls
 *    B) Incoming NV updates are discarded
 * 7) If a node is in a non-configured state, is reset and then issued
 *    a command to go configured, it will come up soft offline
 * 8) If a set node mode message changes the mode to offline or online
 *    the approprite task (if any) is executed
 * 9) Changing the node state recomputes the configuration checksum
 *******************************************************************************/
void HandleNMSetNodeMode(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    if (appReceiveParamPtr->pduSize < 2 || (apduPtr->data[0] == 3
            && appReceiveParamPtr->pduSize < 3)) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    switch (apduPtr->data[0]) {
    case 0: /* Go to soft offline state */
        if (AppPgmRuns()) {
            IzotOffline(); /* Indicate to application program. */
        }
        gp->appPgmMode = OFF_LINE;
        /* No response given as the message is not a request. */
        break;
    case 1: /* Go on-line */
        IzotOnline(); /* Indicate to application program. */
        gp->appPgmMode = ON_LINE;
        /* No response given as the message is not a request. */
        break;
    case 2: /* Application reset */
        gp->resetNode = TRUE;
        nmp->resetCause = IzotSoftwareReset; /* Software reset. */
        /* No response since the node is being reset. */
        break;
    case 3: /* Change State */
        /* Fail if message is not 3 bytes long. */
        if (appReceiveParamPtr->pduSize != 3) {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            break;
        }
        if ((IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) & 
        IS_APPLESS) && !(apduPtr->data[1] & IS_APPLESS)) 
        {
            // We're attempting to leave the appless state.  
            // Did we have a switchover failure?
            if (gp->nvm.downloadState.switchoverFailure) {
                // Yes, fail the download.
                NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
                break;
            }
        }

        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE, 
        apduPtr->data[1]);
        /* Preserve the state of appPgmMode except for
         IzotNoApplicationUnconfig. */
        if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == 
        IzotNoApplicationUnconfig || IZOT_GET_ATTRIBUTE(eep->readOnlyData, 
        IZOT_READONLY_NODE_STATE) == IzotStateInvalid_7) 
        {
            gp->appPgmMode = NOT_RUNNING;
        }
        RecomputeChecksum();
        /* Respond with success if the message was a request. */
        NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
        break;
	case 6: /* New Physical Reset Sub Command */
		PhysicalResetRequested();
		break;
    default:
        /* Let us reset the node for this case */
        gp->resetNode = TRUE;
        nmp->resetCause = IzotSoftwareReset;
        break;
    }
}

/*******************************************************************************
 Function:  HandleNMReadMemory
 Returns:   None
 Reference: None
 Purpose:   Handle incoming NM ReadMemory message
 Comments:  None.
 *******************************************************************************/
void HandleNMReadMemory(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    Queue        *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU         *apduRespPtr;
    char         *memp = NULL;
    IzotUbits16   offset;
    IzotByte      allowed;
    IzotByte      clockSave = 
                   IZOT_GET_ATTRIBUTE(eep->configData, IZOT_CONFIG_INPUT_CLOCK);

    tsaOutQPtr = &gp->tsaRespQ;

    if (appReceiveParamPtr->pduSize < 5 || apduPtr->data[3]
            > gp->tsaRespBufSize) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    offset = ((IzotUbits16) apduPtr->data[1] << 8) | apduPtr->data[2];

    /* Assemble response */
    tsaSendParamPtr = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = FALSE;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    tsaSendParamPtr->apduSize = 1 + apduPtr->data[3];

    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);

    apduRespPtr->code.allBits = NM_resp_success | NM_READ_MEMORY;

    switch (apduPtr->data[0]) {
    case ABSOLUTE_MEM_ADDR:
        if (inRange(offset, apduPtr->data[3]))
        {
            if (IzotMemoryRead(offset, apduPtr->data[3], apduRespPtr->data))
            {
                tsaSendParamPtr->apduSize = 1;
                apduRespPtr->code.allBits = NM_resp_failure | NM_READ_MEMORY;
            }
            EnQueue(tsaOutQPtr);
            return;
        } else
        {
            memp = (char *) nmp;
            if (offset >= 0xF000) {
                memp = (char *) eep - 0xF000;
            }
        }
        break;
    case READ_ONLY_RELATIVE:
    default:
        memp = (char *) &(eep->readOnlyData);
        break;
    case CONFIG_RELATIVE:
        memp = (char *) &(eep->configData);
        break;
    case STAT_RELATIVE:
        memp = (char *) &(nmp->stats);
        break;

#if    ENABLE_STACKTRACE
        case DBG_RELATIVE:
        memp = (char *)STK_GetSnapshot();
        break;
#endif    // ENABLE_STACKTRACE
    case MFG_RELATIVE: {
        EchErr sts = 0;
        if (sts != ECHERR_OK) {
            // Return whatever we read but also return an error.
            apduRespPtr->code.allBits = NM_resp_failure | NM_READ_MEMORY;
        }
    }
    }

    memp += offset;

    /* If readWriteProtect flag is on, then only readonly data structure,
     snvt structures, and configuration structure can be read.
     The one byte read of location 0 (firmware number) is also allowed.
     Reference implementation does not support snvt reading through
     absolute addressing. readOnlyData.snvtStruct is 0xFFFF */

    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_RW_PROTECT) == 
    TRUE)
    {
        allowed = (memp >= (char *) &eep->readOnlyData && memp
                + apduPtr->data[3] < (char *) &eep->domainTable[0]) || (memp
                == (char *) &nmp && apduPtr->data[3] == 1);

        if (!allowed) {
            tsaSendParamPtr->apduSize = 1;
            apduRespPtr->code.allBits = NM_resp_failure | NM_READ_MEMORY;
            EnQueue(tsaOutQPtr);
            return;
        }
    }

    // The switchover occurs during the checksum recalc which loaders base 
    // their timeout on the input clock so we fake this out as being slow.
    // We don't just permanently tweak the comm params for fear of messing 
    // up the MIP.
    IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_INPUT_CLOCK, 0);

    memcpy(apduRespPtr->data, memp, apduPtr->data[3]);

    // Undo above fake-out
    IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_INPUT_CLOCK, clockSave);

    if (memp == (char *) nmp && apduPtr->data[3] == 1) {
        /* trap for absolute read of location 0 and write
         version number */
        apduRespPtr->data[0] = BASE_FIRMWARE_VERSION;
    }

    EnQueue(tsaOutQPtr);
}

/*******************************************************************************
 Function:  HandleNMRecalcChecksum
 Returns:   None
 Reference: None
 Purpose:   Handle incoming NM RecalcChecksum message.
 Comments:  None.
 *******************************************************************************/
void HandleNMChecksumRecalc(APPReceiveParam *appReceiveParamPtr, 
APDU *apduPtr) {
    Status sts = SUCCESS;
    // Re-config checksum.
    RecomputeChecksum();

    NMNDRespond(NM_MESSAGE, sts, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleNMWriteMemory
 Returns:   None
 Reference: None
 Purpose:   Handle incoming NM WriteMemory message.
 Comments:  None.
 *******************************************************************************/
void HandleNMWriteMemory(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    NMWriteMemoryRequest *pr;
    char *memp;
    IzotUbits16 offset;
    IzotByte allowed = true;

    /* Fail if message is not at least 6 bytes long */
    if (appReceiveParamPtr->pduSize < 6) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    /* Pointer to struct describing memory request */
    pr = (NMWriteMemoryRequest *) &apduPtr->data[0];

    offset = hton16(pr->offset);

    /* Fail if message length doesn't match count. Poke can use 16 data bytes.
     Allow that. Note that the code takes one byte. */
    if (appReceiveParamPtr->pduSize != 6 + pr->count
            && appReceiveParamPtr->pduSize != 17) {
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    switch (pr->mode) {
    case ABSOLUTE_MEM_ADDR:
        if (inRange(offset, pr->count))
        {
            if (!IzotMemoryWrite(offset, pr->count, pr->data))
            {
                SetPersistentDataType(2);
                IzotPersistentAppSegmentHasBeenUpdated();
                if (pr->form & CNFG_CS_RECALC) {
                    RecomputeChecksum();
                }
                if (pr->form & ACTION_RESET) {
                    gp->resetNode = TRUE;
                    nmp->resetCause = IzotSoftwareReset;
                }
                NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
            }
            else
            {
                NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            }
            return;
        } else
        {
            memp = (char *) nmp;
            if (offset >= 0xF000) {
                memp = (char *) eep;
                offset -= 0xF000;
            }
        }
        break;
    case CONFIG_RELATIVE:
        memp = (char *) &(eep->configData);
        break;
    case STAT_RELATIVE:
        memp = (char *) &(nmp->stats);
        break;
    case READ_ONLY_RELATIVE:
        memp = (char *) &(eep->readOnlyData);
        break;
    default:
        /* Invalid Mode */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    memp += offset;

    /* Check if the range of memory cells written is good. */
    allowed
            = (memp >= (char *) nmp && (memp + pr->count) <= (char *) (nmp + 1))
                    || (memp >= (char *) eep && (memp + pr->count)
                            <= (char *) (eep + 1));
    if (!allowed) {

        /* Send failure response if the message was a request */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (&memp[pr->count] >= (char*) &eep->configData.Clock && memp
            < (char*) (&eep->configData.Config_1)) {
        // This is a comm parameter write.  Update the actual device
        CpWrite cw = CP_WRITE;
        if (pr->form & ACTION_RESET) {
            cw |= CP_RESET;
        }
    }

    /* If readwrite flag is on, then only config structure can be written. */
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_RW_PROTECT) == 
    TRUE) 
    {
        allowed = (memp >= (char *) &eep->configData && memp + pr->count
                < (char *) &eep->domainTable[0]);

        if (!allowed) {
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
    memcpy(memp, apduPtr->data + 5, pr->count);

    if (pr->form & CNFG_CS_RECALC) {
        RecomputeChecksum();
    }
    if (pr->form & ACTION_RESET) {
        gp->resetNode = TRUE;
        nmp->resetCause = IzotSoftwareReset;
    }

    NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
}

/*******************************************************************************
 Function:  HandleProxyResponse
 Returns:   None
 Reference: None
 Purpose:   Handle Proxy Response Message.
 Comments:  None.
 *******************************************************************************/
void HandleProxyResponse(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    if (appReceiveParamPtr->proxyDone || SendResponse(appReceiveParamPtr->tag,
            apduPtr->code.allBits, appReceiveParamPtr->pduSize - 1,
            apduPtr->data) == SUCCESS) {
        DeQueue(&gp->appCeRspInQ);
    }
}

/*******************************************************************************
 Function:  HandleNDQueryStatus
 Returns:   none
 Reference: None
 Purpose:   Handle Network Diagnostics Query Status Message.
 Comments:  None.
 *******************************************************************************/
void HandleNDQueryStatus(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr,
        Bool useFlexDomain) {
    NDQueryStat ndq;
    Queue *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduRespPtr;

    tsaOutQPtr = &gp->tsaRespQ;
    if (((apduPtr->code.allBits & 0x0F) == ND_QUERY_STATUS)
            && appReceiveParamPtr->pduSize != 1) {
        /* Incorrect size. Fail. */
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    if (((apduPtr->code.allBits & 0x0F) == ND_PROXY_COMMAND)
            && appReceiveParamPtr->pduSize != 2) {
        /* Incorrect size. Fail. */
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return;
    }

    memcpy(ndq.stats, nmp->stats.stats, sizeof(ndq.stats));
    ndq.resetCause = nmp->resetCause;
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == 
    IzotConfigOnLine && gp->appPgmMode  == OFF_LINE) {
        ndq.nodeState = IzotSoftOffLine;
    } else {
        ndq.nodeState = 
               IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE);
    }
    ndq.versionNumber = FIRMWARE_VERSION;
    ndq.errorLog = eep->errorLog;
    ndq.modelNumber = MODEL_NUMBER;
    /* Send response */
    tsaSendParamPtr = QueueTail(tsaOutQPtr);
    tsaSendParamPtr->altPathOverride = FALSE;
    tsaSendParamPtr->service = IzotServiceResponse;
    tsaSendParamPtr->nullResponse = FALSE;
    tsaSendParamPtr->flexResponse = useFlexDomain;
    tsaSendParamPtr->reqId = appReceiveParamPtr->reqId;
    tsaSendParamPtr->apduSize = 1 + sizeof(NDQueryStat);
    apduRespPtr = (APDU *) (tsaSendParamPtr + 1);
    /* The response code is formed using apduPtr->code.allBits as the
     actual could be a proxy command and the response code will be
     different. Thus, using apduPtr->code.allBits works for both
     native query status command as well as proxy based query status */
    if (tsaSendParamPtr->apduSize <= gp->tsaRespBufSize) {
        apduRespPtr->code.allBits = ND_resp_success | (apduPtr->code.allBits
                & 0x0F);
        memcpy(apduRespPtr->data, &ndq, sizeof(NDQueryStat));
    } else {
        apduRespPtr->code.allBits = ND_resp_failure | (apduPtr->code.allBits
                & 0x0F);
        tsaSendParamPtr->apduSize = 1;
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
APDU *apduPtr) {
    Status sts = SUCCESS;
    Queue *tsaOutQPtr;
    TSASendParam *tsaSendParamPtr;
    APDU *apduSendPtr;

    /* See if the proxy command is to be forwarded or it is for us
     to process. If the address information in the packet is missing,
     then there is no forwarding information and hence we respond directly */
    if (appReceiveParamPtr->pduSize == 2) {
        /* Address portion is missing. This node is the proxy target. */
        tsaOutQPtr = &gp->tsaRespQ;
        /* Send the response directly back to the sender */
        switch (apduPtr->data[0]) {
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

    /* Send failure response if we are proxy agent and the message received is 
    on flex domain or it does not have correct size */
    if (appReceiveParamPtr->srcAddr.dmn.domainIndex == FLEX_DOMAIN
            || (apduPtr->data[1] == AM_UNIQUE_NODE_ID
                    && appReceiveParamPtr->pduSize != (2
                            + sizeof(IzotAddress) + IZOT_UNIQUE_ID_LENGTH))
            || (apduPtr->data[1] != AM_UNIQUE_NODE_ID
                    && appReceiveParamPtr->pduSize != (2
                            + sizeof(IzotAddress)))) {
        NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        return sts;
    }

    /* We need to forward the proxy now. Set the target queue ptrs */
    if (appReceiveParamPtr->priority) {
        tsaOutQPtr = &gp->tsaOutPriQ;
    } else {
        tsaOutQPtr = &gp->tsaOutQ;
    }

    /* Check if the target queue has space for forwarding this request. */
    if (QueueFull(tsaOutQPtr)) {
        // Failure indicates we didn't process the message so don't free it!
        return FAILURE;
    }

    /* Generate request message */
    tsaSendParamPtr = QueueTail(tsaOutQPtr);

    /* First copy the msg_out_addr info in the proxy command */
    memcpy(&tsaSendParamPtr->destAddr, &apduPtr->data[1], 
                              sizeof(IzotSendAddress) + IZOT_UNIQUE_ID_LENGTH);

    /* Set the domain index to be the one in which it was received.
     Note that this cannot be flex domain. Transport or session
     will use this instead of the one in the destAddr (i.e msg_out_addr) */
    tsaSendParamPtr->dmn.domainIndex
            = appReceiveParamPtr->srcAddr.dmn.domainIndex;
    tsaSendParamPtr->service = IzotServiceRequest;
    tsaSendParamPtr->auth = FALSE;
    // The tag for a proxy request is the original request's reqId.  When the 
    // response comes back, the response is delivered using that tag in order 
    // to vector it back to the original receive transaction.
    // See HandleProxyResponse().
    tsaSendParamPtr->tag = appReceiveParamPtr->reqId;
    /* Proxy relays the message, the altpath bit should be same as the one
     used for the proxy message received */
    tsaSendParamPtr->altPathOverride = TRUE;
    tsaSendParamPtr->altPath = appReceiveParamPtr->altPath;
    tsaSendParamPtr->apduSize = 2; /* Always two bytes: code + sub_command */

    // Replace: Added check for setting �tsaSendParamPtr->priority� since 
    //          priority messages get stuck in the tsaPriOutQ and endlessly 
    //          sends. This is because when DeQueuing the NV, it will DeQueue
    //          from the tsaOutQ and not the tsaPriOutQ since there is nothing 
    //          sets the priority.
    tsaSendParamPtr->priority = appReceiveParamPtr->priority;
    tsaSendParamPtr->proxy = TRUE;
    tsaSendParamPtr->proxyDone = FALSE;
    tsaSendParamPtr->proxyCount = 0;
    tsaSendParamPtr->txInherit = FALSE;
    tsaSendParamPtr->txTimerDeltaLast = 0;

    apduSendPtr = (APDU *) (tsaSendParamPtr + 1);
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
void HandleNDClearStatus(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    /* Clear Status */
    memset(&nmp->stats, 0, sizeof(nmp->stats));
    nmp->resetCause = IzotResetCleared;
    eep->errorLog = IzotNoError; /* Cleared */

    gp->clearStatsCallback();
    
    /* NMNDRespond will send response only if the msg is IzotServiceRequest */
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
APDU *apduPtr) {
    if (appReceiveParamPtr->pduSize == 1) {
        // Default data parameter to none
        apduPtr->data[0] = 0;
    }

    SendResponse(appReceiveParamPtr->reqId, ND_resp_success
            | ND_QUERY_XCVR_BIDIR, IZOT_COMMUNICATIONS_PARAMETER_LENGTH,
            appReceiveParamPtr->xcvrParams.data);
}

/*******************************************************************************
 Function:  HandleNDGetFullVersion
 Returns:   none
 Reference: None
 Purpose:   Handle network diagnostics get full version command
 Comments:  None.
 *******************************************************************************/
void HandleNDGetFullVersion(APPReceiveParam *appReceiveParamPtr, 
APDU *apduPtr) {
    ND_get_full_version_response fullVersion = { 0 };
	fullVersion.version = FIRMWARE_VERSION;
    fullVersion.variant = 0;
    fullVersion.rom_version = 0;
    fullVersion.minor_version = FIRMWARE_MINOR_VERSION;
    fullVersion.build_number = FIRMWARE_BUILD;
    fullVersion.code = 0;
    SendResponse(appReceiveParamPtr->reqId, ND_resp_success
            | ND_GET_FULL_VERSION, sizeof(fullVersion), (IzotByte*) &fullVersion);
}

/*******************************************************************************
 Function:  HandleND
 Returns:   None
 Reference: None
 Purpose:   Handle incoming network diagnostic message.
 Comments:  None.
 *******************************************************************************/
void HandleND(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    memcpy(&save, eep, sizeof(save));
    Status sts = SUCCESS;
    /* It is not legal for a response to be an ND command */
    if (appReceiveParamPtr->service != IzotServiceResponse) {
        /* If network diagnostics messages need authentication
         and the message did not pass authentication and
         the node is not unconfigured
         then discard those messages that should be discarded and return. */
        if (NodeConfigured() && IZOT_GET_ATTRIBUTE(eep->configData, IZOT_CONFIG_NMAUTH)
                && !appReceiveParamPtr->auth && (apduPtr->code.nd.ndCode
                != ND_QUERY_STATUS && apduPtr->code.nd.ndCode
                != ND_PROXY_COMMAND && apduPtr->code.nd.ndCode
                != ND_QUERY_STATUS_FLEX && apduPtr->code.nd.ndCode
                != ND_QUERY_XCVR_BIDIR && apduPtr->code.nd.ndCode
                != ND_GET_FULL_VERSION)) {
            LCS_RecordError(IzotAuthenticationMismatch);
            NMNDRespond(ND_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        } else {
            /* Handle various network diagnostic message codes */
            switch (apduPtr->code.nd.ndCode) {
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

    // Persist any changes to NVM
    if (memcmp(&save, eep, sizeof(save))) {
        LCS_WriteNvm();
    }

    if (sts == SUCCESS) {
        // Proxy may not actually process the message 
        // so we don't dequeue in that case
        DeQueue(&gp->appInQ);
    }
}

/*******************************************************************************
 Function:  HandleNM
 Returns:   None
 Reference: None
 Purpose:   Handle incoming network management messages.
 Comments:  None.
 *******************************************************************************/
void HandleNM(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) {
    memcpy(&save, eep, sizeof(save));
    /* If network management messages need authentication
     and the message did not pass authentication and
     the node is not unconfigured
     then discard those messages that should be discarded and return. */

    if (NodeConfigured() && IZOT_GET_ATTRIBUTE(eep->configData, 
    IZOT_CONFIG_NMAUTH) && !appReceiveParamPtr->auth
#ifdef SECURITY_II
&& securityIIAuthenticationRequired()
#endif
    ) {
        /* Only a few messages are allowed. Others should be discarded */
        IzotByte subCommand = apduPtr->data[0];
        if (apduPtr->code.nm.nmCode != NM_QUERY_ID && apduPtr->code.nm.nmCode != NM_RESPOND_TO_QUERY && 
			(apduPtr->code.nm.nmCode != NM_EXPANDED || (subCommand != NME_QUERY_VERSION && 
			subCommand != NME_QUERY_IP_ADDRESS
#ifdef SECURITY_II
           && subCommand != NMEXP_SEC_REENROLL && subCommand != NMEXP_SEC_QUERY_BAKP && 
           subCommand != NMEXP_SEC_RELEASE_DEVICE && subCommand != NMEXP_SEC_START_REKEY && 
           subCommand != NMEXP_SEC_REKEY_COMPLETE && subCommand != NMEXP_SEC_DEACTIVATE && 
           subCommand != NMEXP_SEC_QUERY_DEVICE_CERT && subCommand != NMEXP_SEC_QUERY_SECURITY_STATE && 
           subCommand != NMEXP_SEC_QUERY_SECURITY_STATS && subCommand != NMEXP_SEC_QUERY_IV_STATE && 
           subCommand != NMEXP_SEC_QUERY_REPLAY
#endif
    ))) 
		{
            LCS_RecordError(IzotAuthenticationMismatch);
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
            DeQueue(&gp->appInQ);
            return;
        }
    }

    /* Handle various network mgmt message codes */
    switch (apduPtr->code.nm.nmCode) {
    case NM_EXPANDED:
        HandleNMExpanded(appReceiveParamPtr, apduPtr);
        break;
    case NM_QUERY_ID:
        HandleNMQueryId(appReceiveParamPtr, apduPtr);
        break;
    case NM_RESPOND_TO_QUERY:
        /* Fail if message is not 2 bytes long or the byte is bad. */
        if (appReceiveParamPtr->pduSize != 2 || (apduPtr->data[0] != 0
                && apduPtr->data[0] != 1)) {
            NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        } else {
            gp->selectQueryFlag = apduPtr->data[0];
            NMNDRespond(NM_MESSAGE, SUCCESS, appReceiveParamPtr, apduPtr);
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
        break;
    case NM_UPDATE_GROUP_ADDR:
        HandleNMUpdateGroupAddr(appReceiveParamPtr, apduPtr);
        break;
    case NM_QUERY_DOMAIN:
        HandleNMQueryDomain(appReceiveParamPtr, apduPtr);
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
        HandleNMChecksumRecalc(appReceiveParamPtr, apduPtr);
        break;
    case NM_WINK: /* Same as NM_INSTALL */
        HandleNMWink(appReceiveParamPtr, apduPtr);
        break;
    case NM_MEMORY_REFRESH:
        HandleNMRefresh(appReceiveParamPtr, apduPtr);
        break;
    case NM_QUERY_SNVT:
        HandleNMQuerySIData(appReceiveParamPtr, apduPtr);
        break;
    case NM_NV_FETCH:
        HandleNMNVFetch(appReceiveParamPtr, apduPtr);
        break;
    case NM_MANUAL_SERVICE_REQUEST:
        /* This is unsolicited message from a node. Reference implementation 
        ignores manual service request message from other nodes */
        break;
    default:
        /* This is where any message that is not taken care of should be
         handled. An example is product query command. For now, we treat
         everything else as unrecognized network management message. */
        NMNDRespond(NM_MESSAGE, FAILURE, appReceiveParamPtr, apduPtr);
        break;
    }

    // Persist any changes to NVM
    if (memcmp(&save, eep, sizeof(save))) {
        LCS_WriteNvm();
    }

    DeQueue(&gp->appInQ);
}

// dummy callback for when app doesn't register one
void dummy(void) {
}

/*******************************************************************************
 Function:  NM_Init
 Returns:   None
 Reference: None
 Purpose:   Handle network management initialization
 Comments:  None.
 *******************************************************************************/
void NM_Init(void) {
    // Set up a dummy callback
    gp->clearStatsCallback = dummy;
}

/*******************************************************************************
 Function:  LCS_RegisterClearStatsCallback
 Returns:   None
 Reference: None
 Purpose:   Register a callback for clear stats
 Comments:  None.
 *******************************************************************************/
void LCS_RegisterClearStatsCallback(void(*cb)(void)) {
    gp->clearStatsCallback = cb;
}

/******************************************************************************/
