//
// lcs_node.c
//
// Copyright (C) 2022-2025 EnOcean
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
     Reference:  ISO/IEC 14908-1

       Purpose:  Configuration data strutures and type definitions.

          Note:  The LON DX Stack supports any number of stacks.
                 A global structure called ProtocolStackData
                 is defined in node.h. An array of such
                 structures is used so that each stack has
                 its own data that it works on. A global
                 pointer gp points to the right structure
                 before the stack code is executed. This
                 is done by the scheduler.

                 The support for multiple-stacks does not include support
                 for mac layer to handle multiple stacks or multiple application
                 programs. A true multi-stack system needs some extra coding.
*******************************************************************************/

/*------------------------------------------------------------------------------
  Section: Includes
  ------------------------------------------------------------------------------*/
#include "lcs/lcs_node.h"

/*-------------------------------------------------------------------
  Section: Constant Definitions
  -------------------------------------------------------------------*/
static const Dimensions dimensions =
{
    MAX_DOMAINS,
    NUM_ADDR_TBL_ENTRIES,
    NV_TABLE_SIZE,
    NV_ALIAS_TABLE_SIZE
};

/*-------------------------------------------------------------------
  Section: Type Definitions
  -------------------------------------------------------------------*/

/*-------------------------------------------------------------------
  Section: Globals
  -------------------------------------------------------------------*/
EEPROM            *eep; /* actual structure is in eeprom.c */
NmMap             *nmp;
NmMap              nm[NUM_STACKS];
ProtocolStackData *gp;
ProtocolStackData  protocolStackDataGbl[NUM_STACKS];

SNVTCapabilityInfo *snvt_capability_info;
SIHeaderExt        *si_header_ext;
SNVTCapabilityInfo  capability_info;
SIHeaderExt         header_ext;
IzotDpProperty      izot_dp_prop[NV_TABLE_SIZE];

extern IzotByte DataPointCount;
extern IzotByte AliasTableCount;
extern uint16_t BindableMTagCount;
extern IzotByte NmAuth;

/*-------------------------------------------------------------------
  Section: Local Globals
  -------------------------------------------------------------------*/
static const IzotUbits16 bufSizeCodeLGbl[16] = {255, 20, 20, 21, 22, 24, 26, 30, 34, 42, 50, 66, 82, 114, 146, 210};
static const IzotUbits16 bufCntCodeLGbl[16] = {0, 1, 1, 2, 3, 5, 7, 11, 15, 23, 31, 47, 63, 95, 127, 191};
static const IzotUbits16 rptTimerCodeLGbl[16] = 
{16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072};
static const IzotUbits16 rcvTimerCodeLGbl[16] = 
{128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576};
static IzotBool do_reset = FALSE;

/*-------------------------------------------------------------------
Section: Function Prototypes
-------------------------------------------------------------------*/

/*-------------------------------------------------------------------
Section: Function Definitions
-------------------------------------------------------------------*/

/*****************************************************************
Function:  AccessDomain
Returns:   Address of structure corresponding to given index
Purpose:   To return the address of the structure that has domain
           information for this node
Comments:  If an invalid index is given, log error message.
******************************************************************/

IzotDomain *AccessDomain(IzotByte indexIn)
{
    if (indexIn <= IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS)) {
        return(&eep->domainTable[indexIn]);
    }
    return(NULL);
}

/*****************************************************************
Function:  UpdateDomain
Returns:   Status
Purpose:   To Change the domain table entry with given structure.
Comments:  If an invalid index is given, log error message.
******************************************************************/
Status UpdateDomain(const IzotDomain *domainInp, IzotByte indexIn, 
IzotByte includeKey)
{
    Status sts = SUCCESS;
    int nDomains = IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS) ? MAX_DOMAINS : 1;
    if (indexIn < nDomains) {
        memcpy(&eep->domainTable[indexIn], domainInp, includeKey ? sizeof(IzotDomain) : 
        sizeof(IzotDomain) - IZOT_AUTHENTICATION_KEY_LENGTH);
    } else {
        sts = FAILURE;
    }
    return sts;
}

/*****************************************************************
Function:  AccessAddress
Returns:   Address of structure at given index
Reference: Tech Device Data Book Rev 1 p.9-12
Purpose:   To access address table entry
Comments:  None
******************************************************************/
IzotAddress *AccessAddress(IzotUbits16 indexIn)
{
    if (indexIn < eep->readOnlyData.Extended) {
        return(&eep->addrTable[indexIn]);
    }
    return(NULL);
}

/*****************************************************************
Function:  UpdateAddress
Returns:   Status
Reference: Tech Device Data Rev 1 p.9-12
Purpose:   To update an address table entry.
Comments:  None
******************************************************************/
Status UpdateAddress(const IzotAddress *addrEntryInp, IzotUbits16 indexIn)
{
    Status sts = SUCCESS;

    if (indexIn < eep->readOnlyData.Extended) {
        eep->addrTable[indexIn] = *addrEntryInp;
    } else {
        LCS_RecordError(IzotInvalidAddrTableIndex);
        sts = FAILURE;
    }

    return sts;
}

/*****************************************************************
Function:  IsGroupMember
Returns:   TRUE if this node belongs to given group. FALSE, else.
Reference: None
Purpose:   To check if a node belongs to a given group in the
           given domain. If it does, also get the member number.
Comments:  If groupMemberOut is NULL, then it is not used.
******************************************************************/
IzotByte IsGroupMember(IzotByte domainIndexIn, IzotByte groupIn,
                      IzotByte *groupMemberOut)
{
    IzotUbits16 i;

    for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++)
    {
        if (IZOT_GET_ATTRIBUTE(eep->addrTable[i].Group, IZOT_ADDRESS_GROUP_TYPE) == 1) {

            /* Group Format */
            if (eep->addrTable[i].Group.Group == groupIn && IZOT_GET_ATTRIBUTE(eep->addrTable[i].Group, 
            IZOT_ADDRESS_GROUP_DOMAIN) == domainIndexIn) {
                break;
            }
        }
    }
    if (i == NUM_ADDR_TBL_ENTRIES) {
        return(FALSE); /* Not Found */
    }
    if (groupMemberOut) {
        *groupMemberOut = IZOT_GET_ATTRIBUTE(eep->addrTable[i].Group, IZOT_ADDRESS_GROUP_MEMBER);
    }
    return(TRUE); /* Found */
}

/*****************************************************************
Function:  AddrTableIndex
Returns:   The index of the address table for the given domain
           and group. 0xFF if not found.
Reference: None
Purpose:   To get the addr table index for a given group and domain.
           If there is no such entry in the addr table, return 0xff.
Comments:  None
******************************************************************/
IzotUbits16 AddrTableIndex(IzotByte domainIndexIn, IzotByte groupIn)
{
    IzotUbits16 i;

    for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++) {
        if (IZOT_GET_ATTRIBUTE(eep->addrTable[i].Group, IZOT_ADDRESS_GROUP_TYPE) == 1) {
            /* Group Format */
            if (eep->addrTable[i].Group.Group == groupIn && IZOT_GET_ATTRIBUTE(eep->addrTable[i].Group, 
            IZOT_ADDRESS_GROUP_DOMAIN) == domainIndexIn) {
                return(i);
            }
        }
    }

    return(0xFF); /* Not Found */
}


/*****************************************************************
Function:  DecodeBufferSize
Returns:   Actual Buffer Size
Reference: Tech Device Data Rev 1 p9-9
Purpose:   To compute the actual buffer size from code
Comments:  None
******************************************************************/
IzotUbits16  DecodeBufferSize(IzotByte bufSizeIn)
{
    if (bufSizeIn <= 15)
    {
        return(bufSizeCodeLGbl[bufSizeIn]);
    }
    DBG_vPrintf(TRUE, "DecodeBufferSize: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeBufferCnt
Returns:   Actual Buffer Count
Reference: Tech Device Data Rev 1 p.9-10
Purpose:   To compute the actual buffer count from code
Comments:  None
******************************************************************/
IzotUbits16  DecodeBufferCnt(IzotByte bufCntIn)
{
    if (bufCntIn <= 15)
    {
        return(bufCntCodeLGbl[bufCntIn]);
    }
    DBG_vPrintf(TRUE, "DecodeBufferCnt: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeRptTimer
Returns:   Actual timer value in ms
Reference: Tech Device Data Rev 1 p.9-17
Purpose:   To compute the actual rpt timer value from code
Comments:  None
******************************************************************/
IzotUbits16 DecodeRptTimer(IzotByte rptTimerIn)
{
    if (rptTimerIn <= 15)
    {
        return(rptTimerCodeLGbl[rptTimerIn]);
    }
    DBG_vPrintf(TRUE, "DecodeRptTimer: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeRcvTimer
Returns:   Actual Receive Timer value in ms
Reference: Tech Device Data Rev 1 p.9-17
Purpose:   To compute the actual rcv timer value in ms from code
Comments:  None
******************************************************************/
IzotUbits16 DecodeRcvTimer(IzotByte rcvTimerIn)
{
    if (rcvTimerIn <= 15)
    {
        return(rcvTimerCodeLGbl[rcvTimerIn]);
    }
    DBG_vPrintf(TRUE, "DecodeRcvTimer: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeTxTimer
Returns:   Actual Transmit Timer Value in ms
Reference: Tech Device Data Rev 1 p.9-17
Purpose:   To compute the actual transmit timer value from code
Comments:  None
******************************************************************/
IzotUbits16 DecodeTxTimer(IzotByte  txTimerIn
#ifdef IZOT_PROXY
, IzotByte longTimer
#endif
)
{
    int v = 16;
#ifdef IZOT_PROXY
    if (longTimer)
    {
        txTimerIn += 16;
    }
#endif
    v <<= txTimerIn/2;
    if (txTimerIn&1)
    {
        v += v/2;
    }
    return v;
}


/*****************************************************************
Function:  AccessNV
Returns:   Address of NV conf table entry
Reference: Tech Device Data Rev 1 p.9-18
Purpose:   To Access the NV Config Table Entry given the index
Comments:  None
******************************************************************/
IzotDatapointConfig *AccessNV(IzotUbits16 indexIn)
{
    if (indexIn < nmp->nvTableSize)
    {
        return(&eep->nvConfigTable[indexIn]);
    }
    DBG_vPrintf(TRUE, "AccessNV: Invalid index.\n");
    return(NULL);
}

/*****************************************************************
Function:  AccessAlias
Returns:   Address of Alias conf table entry
Reference:
Purpose:   To Access the Alias Config Table Entry given the index
Comments:  None
******************************************************************/
IzotAliasConfig *AccessAlias(IzotUbits16 indexIn)
{
    if (indexIn < NV_ALIAS_TABLE_SIZE)
    {
        return(&eep->nvAliasTable[indexIn]);
    }
    DBG_vPrintf(TRUE, "AccessAlias: Invalid index.\n");
    return(NULL);
}

/*****************************************************************
Function:  UpdateNV
Returns:   None
Reference: Tech Device Data Rev 1 p.9-18
Purpose:   To update an entry in NV Config Table
Comments:  None
******************************************************************/
void UpdateNV(IzotDatapointConfig *nvStructInp, IzotUbits16 indexIn)
{
    if (nvStructInp && indexIn < nmp->nvTableSize)
    {
        eep->nvConfigTable[indexIn] = *nvStructInp;
        return;
    }
    if (nvStructInp)
    {
        DBG_vPrintf(TRUE, "UpdateNV: Invalid index.\n");
    }
    else
    {
        DBG_vPrintf(TRUE, "UpdateNV: NULL nvStructInp.\n");
    }
}

/*****************************************************************
Function:  UpdateAlias
Returns:   None
Reference:
Purpose:   To update an entry in Alias Config Table
Comments:  None
******************************************************************/
void UpdateAlias(IzotAliasConfig *aliasStructInp, IzotUbits16 indexIn)
{
    if (aliasStructInp && indexIn < NV_ALIAS_TABLE_SIZE)
    {
        eep->nvAliasTable[indexIn] = *aliasStructInp;
        return;
    }
    if (aliasStructInp)
    {
        DBG_vPrintf(TRUE, "UpdateAlias: Invalid index.\n");
    }
    else
    {
        DBG_vPrintf(TRUE, "UpdateAlias: NULL aliasStructInp.\n");
    }
}

/*****************************************************************
Function:  ErrorMsg
Returns:   None
Reference: None
Purpose:   To store error msgs produced by these functions.
Comments:  It is just a sequence of Bytes that is large.
           If there is no more space, it wraps around and
           logs. Thus, if there are too many error logs,
           we will only have the latest ones.
           Each Log is automatically given a number.
           The output has log number followed by message.
******************************************************************/
#if DEBUG_LCS
void ErrorMsg(char errMessageIn[])
{
    printf(errMessageIn);
    printf("\n\r");
}

/*****************************************************************
Function:  DebugMsg
Returns:   None
Reference: None
Purpose:   To Print Debugging Messages for stacks.
Comments:  Actually not recorded anywhere. One needs to set
           breakpoint at the end of this fn and print temp
           to see the msg.
******************************************************************/
void DebugMsg(char debugMsgIn[])
{
    printf(debugMsgIn);
    printf("\n\r");
}
#endif

/*****************************************************************
Function:  AllocateStorage
Returns:   Pointer to data storage allocated or NULL
Reference: None
Purpose:   A Simple version of storage allocator similar to malloc.
           A Global array is used to allocate the srorage.
           If no more space, NULL is returned.
Comments:  There is no function similar to free. There is no need
           for such a funcion in the Reference Implementation.
******************************************************************/
void *AllocateStorage(IzotUbits16 sizeIn)
{
    IzotByte *ptr;

    if (gp->mallocUsedSize + sizeIn > MALLOC_SIZE)
    {
        LCS_RecordError(IzotMemoryAllocFailure);
        return(NULL); /* No space for requested size */
    }

    ptr = gp->mallocStorage + gp->mallocUsedSize;
    gp->mallocUsedSize += sizeIn;
    return(ptr);
}

/*****************************************************************
Function:  NodeReset
Returns:   None
Reference:
Purpose:   Initialization of node data structures.
Comments:
******************************************************************/
void NodeReset(IzotByte firstReset)
{

    void APPReset(void), TCSReset(void), TSAReset(void), NWReset(void), LsUDPReset(void);

    void (*resetFns[])(void) = {APPReset, TCSReset, TSAReset, NWReset,  LsUDPReset};

    IzotByte fnNum, fnsCnt;

    if (!firstReset)
    {
        // Just do an actual reset of the device.
    }

    /* Init variables that are not in EEPROM */
    memset(&nmp->stats, 0, sizeof(StatsStruct));
    gp->prevServiceLedState  = 0xFF;
    gp->preServiceLedPhysical = 0xFF;

    /* A node in soft off-line state should go on-line state */
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotConfigOnLine && 
    gp->appPgmMode == OFF_LINE)
    {
        gp->appPgmMode = ON_LINE; /* Normal state. on-line. */
    }

    /* If a node is reset while in unconfigured state, it will come back in
       offline mode when asked to go configured later. */
    if (NodeUnConfigured())
    {
        gp->appPgmMode = OFF_LINE;
    }

    /* First, Let each layer determine the address of all its
       data strcutures */
    gp->mallocUsedSize = 0;

    /* Call all the Reset functions */
    fnsCnt = sizeof(resetFns)/sizeof(FnType);
    for (fnNum = 0; fnNum < fnsCnt; fnNum++)
    {
        resetFns[fnNum](); /* Call the Reset function. */
        if (!gp->resetOk)
        {
            DBG_vPrintf(TRUE, "NodeReset: Failure");
            return;
        }
    }

#if LINK_IS(MIP)
    PHYInitSPM(firstReset);
#endif // LINK_IS(MIP)

    if (firstReset)
    {
        memset(gp->prevChallenge, 0, sizeof(gp->prevChallenge));
    }

    if (nmp->resetCause == IzotExternalReset || nmp->resetCause == IzotPowerUpReset)
    {
        SetLonTimer(&gp->tsDelayTimer, TS_RESET_DELAY_TIME);
    }
    gp->resetNode        = FALSE;
    
    IzotReset(NULL);
}

/*****************************************************************
Function:  InitEEPROM
Returns:   Status
Reference: None
Purpose:   To initialize the EEPROM data items based on constants
           in custom.h and values set in custom.c
Comments:  Incomplete Initialization. Make sure it has the var you
           want or else add it here or in custom.h or custom.c
                depending on where it fits.
******************************************************************/
Status    InitEEPROM(uint32_t signature)
{
    Status sts = SUCCESS;
    int i;

    // We first get the persistent data from NVM.
    if (!gp->initialized)
    {
        EchErr err;

        // Init all of NVM
        memset(eep, 0, sizeof(*eep));

        err = LCS_ReadNvm();
        if (err == ECHERR_INVALID_PARAM)
        {
            // This can occur if the NVM image has grown too large for the max PAL size
            sts = FAILURE;
        }
        else if (err != ECHERR_OK || 
        memcmp(&eep->dimensions, &dimensions, sizeof(dimensions)) || eep->signature != signature)
        {
            // Re-init all of NVM
            memset(eep, 0, sizeof(*eep));

            IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE, 
            IzotApplicationUnconfig);

            /* Initialize configData */
            IZOT_SET_UNSIGNED_WORD(eep->configData.ChannelId, 0);
            IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_COMM_CLOCK, 3);
            IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_COMM_TYPE, 
            SPECIAL_PURPOSE);
            IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_COMM_PINDIR, 0x1E); /* 0x17 if wake-up pin is input */
            eep->configData.PreambleLength      = 0x00; /* for special purpose mode. */
            eep->configData.PacketCycle         = 0x3F; /* packet_cycle */
            eep->configData.Beta2Control        = 0xA6; /* beta2 control */
            eep->configData.TransmitInterpacket = 0x77; /* xmit_interpacket */
            eep->configData.ReceiveInterpacket  = 0x67; /* recv_interpacket */
            eep->configData.NodePriority        = 1; /* 0-255. 0 => no priority slot. */
            eep->configData.ChannelPriorities   = 8; /* 0-255 */
            eep->configData.CommunicationParameters.TransceiverParameters[0] = 0x0e;
            eep->configData.CommunicationParameters.TransceiverParameters[1] = 0x01;
            eep->configData.CommunicationParameters.TransceiverParameters[2] = 0;
            eep->configData.CommunicationParameters.TransceiverParameters[3] = 0;
            eep->configData.CommunicationParameters.TransceiverParameters[4] = 0;
            eep->configData.CommunicationParameters.TransceiverParameters[5] = 0;
            eep->configData.CommunicationParameters.TransceiverParameters[6] = 0;
            /* dirParams only used for direct mode not special purpose mode */
            /* eep->configData.param.dirParams.bitSyncThreshHold = 1; */
            IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_NONGRPRCV, NON_GROUP_TIMER);
            IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_NMAUTH, NmAuth);
            IZOT_SET_ATTRIBUTE(eep->configData, IZOT_CONFIG_PREEMPT, 0);

            /* Initialization based on custom.c */
            memcpy(eep->configData.Location, cp->location, LOCATION_LEN);
            for (i = 0; i <= cp->twoDomains; i++)
            {
                IZOT_SET_ATTRIBUTE(eep->domainTable[i], IZOT_DOMAIN_ID_LENGTH, cp->len[i]);
                memcpy(eep->domainTable[i].Id, cp->domainId[i], cp->len[i]);
                eep->domainTable[i].Subnet = cp->subnet[i];
                IZOT_SET_ATTRIBUTE(eep->domainTable[i], IZOT_DOMAIN_NODE, cp->node[i]);
                IZOT_SET_ATTRIBUTE(eep->domainTable[i], IZOT_DOMAIN_NONCLONE, cp->clone[i]);
                memcpy(eep->domainTable[i].Key, cp->key[i], IZOT_AUTHENTICATION_KEY_LENGTH);
            }
            LCS_InitAddress();
            nmp->nvTableSize  = 0;
            LCS_InitAlias();
            LCS_WriteNvm();
        }
        else
        {
            IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE, eep->nodeState);
        }

        // Record the signature in case the application interface changes.
        eep->signature = signature;

        // Read only data we always set.  Note that this means if you modify these things
        // over the network, they'll get wiped out after a reset.  The reason for this is
        // to ensure they get updated on an upgrade.
        // First set stuff from the custom.c values.
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TWO_DOMAINS, cp->twoDomains);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_ADDRESS_CNT ,cp->addressCnt);
        IzotGetUniqueId(&eep->readOnlyData.UniqueNodeId);
        memcpy(eep->readOnlyData.ProgramId, cp->progId, IZOT_PROGRAM_ID_LENGTH);
        
        /* Init remainder based on custom.h and default values */
        eep->readOnlyData.ModelNum         = MODEL_NUM;
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_MINORNUM, 
        MINOR_MODEL_NUM);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_CHECKSUM,0);

        eep->readOnlyData.DatapointFixed[0] = 0xFF; /* not useful */
        eep->readOnlyData.DatapointFixed[1] = 0xFF;

        // Must be one for NodeUtil to conclude this is a host node
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, 
        IZOT_READONLY_DATAPOINT_PROCESSINGOFF, 0);
        // Just say this is true.  
        // Not sure why it needs to be in the XIF but ok...
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_MSG_PROCESS,1); 
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_RUN_UNCONFIG, 
        RUN_WHEN_UNCONF);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_DATAPOINT_COUNT, 0);
        /* MIP uses 0xFFFF for snvtStruct field. p 9-8 */
        eep->readOnlyData.SnvtStruct[0]    = 0xFF;
        eep->readOnlyData.SnvtStruct[1]    = 0xFF;
        /* NUM_ADDR_TBL_ENTRIES can be larger than 15, but
           addressCnt is set to min(15, NUM_ADDR_TBL_ENTRIES).
           The remaining entries are not seen by the lonbuilder tool */
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_ADDRESS_CNT , 
        (cp->addressCnt <= 15) ? cp->addressCnt : 15);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_REC_TRANSCNT, 
        (RECEIVE_TRANS_COUNT < 16) ? RECEIVE_TRANS_COUNT - 1 : 15);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_OUTBUF_SIZE, APP_OUT_BUF_SIZE);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_INBUF_SIZE, APP_IN_BUF_SIZE);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUTBUF_SIZE, NW_OUT_BUF_SIZE);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_INBUF_SIZE, NW_IN_BUF_SIZE);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUT_PRICNT, NW_OUT_PRI_Q_CNT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_OUT_PRICNT, APP_OUT_PRI_Q_CNT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_OUTBUF_CNT, APP_OUT_Q_CNT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_INBUF_CNT, APP_IN_Q_CNT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUTBUF_CNT, NW_OUT_Q_CNT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_INBUF_CNT, NW_IN_Q_CNT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_MSG_TAG_CNT, BindableMTagCount);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_RESERVED10, 0x8);
        
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_RW_PROTECT, READ_WRITE_PROTECT);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_TX_BY_ADDRESS ,0);
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_ALIAS_CNT, 0); /* Host based node */
        eep->readOnlyData.AliasCount      = AliasTableCount; /* Host based node */
        eep->readOnlyData.DatapointCount  = DataPointCount;
        eep->readOnlyData.Extended          = cp->addressCnt;
        // Record the dimensions uses for NVM.  If these change, we'll reset all the NVM
        #if LON_DMF_ENABLED
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_DMF, 1);
        #endif
        #ifdef SECURITY_II
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_SEC_II, 1);
		#endif
        eep->dimensions = dimensions;
        
        si_header_ext->node_sd_text_length = hton16(strlen(cp->szSelfDoc) + 1);
        si_header_ext->static_nv_count = hton16(DataPointCount);
        
        snvt_capability_info->length = hton16(sizeof(capability_info));
        snvt_capability_info->ver_struct = 1;
        snvt_capability_info->ver_nm_min = 0;
        snvt_capability_info->ver_nm_max = 0;
        snvt_capability_info->ver_binding = 0;
        memset(snvt_capability_info->ext_cap_flags, 0, NUM_EXTCAP_BYTES);
        snvt_capability_info->domain_capacity = hton16(cp->twoDomains + 1);
        snvt_capability_info->address_capacity = (cp->addressCnt <= 15) ? hton16(cp->addressCnt) : hton16(15);
        snvt_capability_info->static_mtag_capacity = hton16(BindableMTagCount);
        snvt_capability_info->mcnv_capacity = 0;
        snvt_capability_info->mcp_capacity = 0;
        snvt_capability_info->mcs_capacity = 0;
        snvt_capability_info->max_mc_desc_length = 0;
        snvt_capability_info->mcnv_current_count = 0;
        snvt_capability_info->mcnv_max_index = 0;
        snvt_capability_info->dyn_fb_capacity = 0;
        snvt_capability_info->eat_address_capacity = cp->addressCnt;
    }
    return sts;
}

/*****************************************************************
Function:  GetPrimaryIndex
Returns:   The primary index of the given variable.
Reference: None
Purpose:   To compute the primary index
Comments:  Given index can be either primary or alias.
******************************************************************/
IzotBits16 GetPrimaryIndex(IzotBits16 nvIndex)
{
    IzotBits16   primaryIndex;

    if (nvIndex < 0 || nvIndex >= nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        return(-1); /* Bad index value. */
    }

    if (nvIndex < nmp->nvTableSize)
    {
        primaryIndex = nvIndex; /* Primary index itself. */
    }
    else
    {
        nvIndex = nvIndex - nmp->nvTableSize; /* Get alias table index. */
        /* Compute the primary index. */
        primaryIndex = eep->nvAliasTable[nvIndex].Primary; 

        if (primaryIndex >= nmp->nvTableSize)
        {
            return(-1); /* Bad index in alias structure. */
        }
    }
    return(primaryIndex);
}

/*****************************************************************
Function:  GetNVStructPtr
Returns:   Pointer to the network variable structure.
Reference: None
Purpose:   To compute the pointer to the network variable structure.
Comments:  The given index can be either primary or alias.
******************************************************************/
IzotDatapointConfig *GetNVStructPtr(IzotBits16 nvIndexIn)
{
    if (nvIndexIn < 0 || nvIndexIn >= nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        return(NULL); /* Bad index value. */
    }

    if (nvIndexIn < nmp->nvTableSize)
    {
        return(&eep->nvConfigTable[nvIndexIn]);
    }

    return(&eep->nvAliasTable[nvIndexIn - nmp->nvTableSize].Alias);
}

/*****************************************************************
Function:  CheckSum8
Returns:   8 bit checksum of a given data
Reference: None
Purpose:   To compute the checksum of an array of bytes of
           a given length. The check sum is the successive
           application of exclusive or of successive 4 bits.
Comments:  None
******************************************************************/
IzotByte CheckSum8(void *dataIn, IzotUbits16 lengthIn)
{
    unsigned char *p;
    IzotUbits16 i;
    IzotByte result = 0; /* Final checksum */

    p = dataIn;
    for (i = 0; i < lengthIn; i++)
    {
        result = result ^ *p;
        p++;
    }
    return(result);
}

/*****************************************************************
Function:  ComputeConfigCheckSum
Returns:   The configuration checksum.
Reference: None
Purpose:   To compute the configuration checksum.
******************************************************************/
IzotByte ComputeConfigCheckSum(void)
{
    IzotByte checkSum;
    IzotUbits16 size;

    size = (char*)&eep->configCheckSum - (char *)&eep->configData;
    checkSum = CheckSum8((char *)&eep->configData, size);
    return(checkSum);
}

/****************************************************************
Function: IsTagBound
Returns:  TRUE if the tag is bound. FALSE otherwise.
Purpose:  To determine if a tag is bound or not.
Comment:  A tag is bound if its address index is not 0xF
          or there is an alias attached to it whose address
          index is not 0xF.
****************************************************************/
IzotByte IsTagBound(IzotByte tagIn)
{
    return(tagIn < nmp->snvt.mtagCount && tagIn < NUM_ADDR_TBL_ENTRIES &&
    eep->addrTable[tagIn].SubnetNode.Type != IzotAddressUnassigned);
}

/****************************************************************
Function: IsNvBound
Returns:  TRUE if the variable is bound. FALSE otherwise.
Purpose:  To determine if a primary variable is bound or not.
Comment:  A variable is bound if its address index is not 0xF
          or there is an alias attached to it whose address
          index is not 0xF.
****************************************************************/
IzotByte IsNVBound(IzotBits16 nvIndexIn)
{
    IzotUbits16 i;
    IzotBits16 primaryIndex;
    IzotByte addrIndex;

    if (nvIndexIn < 0 || nvIndexIn >= nmp->nvTableSize)
    {
        return(FALSE); /* not an index of a primary network variable */
    }

    /* If the primary has a valid address table index and the address
       table entry is not unbound, then the variable is bound */
    addrIndex = ADDR_INDEX(IZOT_GET_ATTRIBUTE(eep->nvConfigTable[nvIndexIn], IZOT_DATAPOINT_ADDRESS_HIGH), 
    IZOT_GET_ATTRIBUTE(eep->nvConfigTable[nvIndexIn], IZOT_DATAPOINT_ADDRESS_LOW));
    //Changed as per Extended Address table doc requirement
    if ( addrIndex != 0xFF && (eep->addrTable[addrIndex].SubnetNode.Type != IzotAddressUnassigned ||
    eep->addrTable[addrIndex].Turnaround.Turnaround == 1))
    {
        return(TRUE);
    }

    /* Primary is not bound. See if there is an alias for this variable
       that is bound. */
    for (i = 0; i < NV_ALIAS_TABLE_SIZE; i++)
    {
        primaryIndex = GetPrimaryIndex((IzotBits16)(i + nmp->nvTableSize));
        addrIndex    = ADDR_INDEX(IZOT_GET_ATTRIBUTE(eep->nvAliasTable[i].Alias, IZOT_DATAPOINT_ADDRESS_HIGH), 
        IZOT_GET_ATTRIBUTE(eep->nvAliasTable[i].Alias, IZOT_DATAPOINT_ADDRESS_LOW));
        /* If the alias matches the primary, has a valid address table
           index and the address table entry is not IzotAddressUnassigned, then
           the primary variable is bound */
        if (primaryIndex == nvIndexIn && addrIndex != 0xFF &&
        (eep->addrTable[addrIndex].SubnetNode.Type != IzotAddressUnassigned ||
        eep->addrTable[addrIndex].Turnaround.Turnaround == 1) )
        {
            return(TRUE);
        }
    }
    return(FALSE); /* No alias entry for this primary that is bound. */
}

/*******************************************************************************
Function: AppPgmRuns
Returns:  TRUE if the application program is running on the node.
          FALSE otherwise.
Purpose:  To determine whether the application program is running or not.
          This is used to determine whether to deliver messages, responses,
          and events to the application. Also used to determine whether
          to call DoApp or not.
*******************************************************************************/
IzotByte AppPgmRuns(void)
{
    /* Normal Mode. Configured and running. */
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotConfigOnLine && 
    gp->appPgmMode == ON_LINE)
    {
        return(TRUE);
    }

    /* Unconfigured and running. */
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == 
    IzotApplicationUnconfig && IZOT_GET_ATTRIBUTE(eep->readOnlyData, 
    IZOT_READONLY_RUN_UNCONFIG) && gp->appPgmMode == ON_LINE)
    {
        return(TRUE);
    }

    return(FALSE);
}

/*******************************************************************************
Function: NodeConfigured
Returns:  TRUE if the node configuration is valid.
Purpose:  To determine whether currently the node is configured.
*******************************************************************************/
IzotByte NodeConfigured(void)
{
    return (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) & IS_CONFIGURED) ? true:false;
}

/*******************************************************************************
Function: NodeUnConfigured
Returns:  TRUE if the node is configured.
Purpose:  To determine whether currently node is unconfigured.
### WARNING ###
NodeUnConfigured() is not just !NodeConfigured() and vice versa.  The reason
is that we support certain hybrid states where the device's configuration is
considered valid but the application must not run.
### WARNING ###
*******************************************************************************/
IzotByte NodeUnConfigured(void)
{
    return IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) != IzotConfigOnLine;
}

/*******************************************************************************
Function: RecordError
Returns:  void
Purpose:  Log an error to the error log.
*******************************************************************************/
void LCS_RecordError(IzotSystemError err)
{
      // To avoid wearing out the NVM in case there is repeated logging of 
    // the same error, check for a change first.
    if (eep->errorLog != err)
    {
        eep->errorLog = err;
        LCS_WriteNvm();
    }
}

/*******************************************************************************
Function: LCS_LogRxStat
Returns:  void
Purpose:  Log an RX stat.  See above.
*******************************************************************************/
void    LCS_LogRxStat(AltPathFlags altPath, RxStatType type)
{
    int channel = (altPath&ALT_CHANNEL) ? 1 : 0;
    nmp->rxStat.rx[channel][altPath&ALT_PATH][type&1]++;
}

/*******************************************************************************
Function: LCS_InitAddress
Returns:  void
Purpose:  Clear out address table.
*******************************************************************************/
void LCS_InitAddress(void)
{
    int i;

    /* Init Address Table based on custom.c */
    for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++)
    {
        memset(&eep->addrTable[i], 0, 5);
    }
}

/*******************************************************************************
Function: LCS_InitAlias
Returns:  void
Purpose:  Clear out alias table.
*******************************************************************************/
void LCS_InitAlias(void)
{
    int i;
    /* Init Alias Table based in custom.c */
    /* Since C initializes missing elements with 0 we use
       any non-zero value for hostPrimary field to indicate that
       we did initialize an entry. We don't need 0 anyway for
       hostPrimary as we can use primary for such entries. */

    for (i = 0; i < NV_ALIAS_TABLE_SIZE; i++)
    {
        memset(&eep->nvAliasTable[i], 0, 6);
    }

    /* Initialize Alias Tables that are not initialized in custom.h */
    for (i = 0; i < NV_ALIAS_TABLE_SIZE; i++)
    {
        char *p;
        /* Init only those that are not given meaningful values
           in custom.h */

        p = (char *)&eep->nvAliasTable[i];
        *p        = (char) 0x7F;
        *(p + 1)  = (char) 0xFF;
        *(p + 2)  = (char) 0x0F;
        *(p + 3)  = (char) 0xFF;
        *(p + 4)  = (char) 0xFF;
        *(p + 5)  = (char) 0xFF;
    }
}

/****************************************************************************
 *
 * NAME: AppInit
 *
 * DESCRIPTION: Called by LCS init to init Application layer
 *
 * PARAMETERS: None
 *
 ****************************************************************************/
Status AppInit(void) 
{
#ifdef SECURITY_II
	LtSecurityII_Init();
#endif
    return(SUCCESS);
};

/****************************************************************************
 *
 * NAME: MsgCompletes
 *
 * DESCRIPTION: You can call this function when Message successfully sent
 *              or failed.
 *
 ****************************************************************************/
void MsgCompletes(Status status, MsgTag tag)
{
    IzotBool stat = status ? FALSE : TRUE;
    unsigned Tag =  (unsigned)tag;
    
    if (!IzotFilterMsgCompleted(Tag, stat))
    {
        IzotMsgCompleted(Tag, stat);
    }
}

void DoApp(Bool isOnline) 
{
    MsgIn* msg_in = NULL;
    RespIn* rsp_in = NULL;
    
    if (MsgReceive(&msg_in))
    {
        if (!IzotFilterMsgArrived(&msg_in->addr, (IzotCorrelator)&msg_in->reqId, 0, msg_in->service, 
        msg_in->authenticated, msg_in->code, msg_in->data, msg_in->len))
        {
            IzotMsgArrived(&msg_in->addr, (IzotCorrelator)&msg_in->reqId, 0, msg_in->service, msg_in->authenticated, 
            msg_in->code, msg_in->data, msg_in->len); 
        }
        MsgFree();
    }
    if (RespReceive(&rsp_in))
    {
        if (!IzotFilterResponseArrived(&rsp_in->addr, rsp_in->tag, rsp_in->code, rsp_in->data, rsp_in->len))
        {
            IzotResponseArrived(&rsp_in->addr, rsp_in->tag, rsp_in->code, rsp_in->data, rsp_in->len);
        }
        RespFree();
    }
}

/****************************************************************************
 *
 * NAME: izot_get_device_state
 *
 * DESCRIPTION: This function is for informing the webpages of the current
 *              state.
 *
 ****************************************************************************/
uint8_t izot_get_device_state(void)
{
    return IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE);
}

/****************************************************************************
 *
 * NAME: izot_get_device_mode
 *
 ****************************************************************************/
uint8_t izot_get_device_mode(void)
{
    return gp->appPgmMode;
}

/****************************************************************************
 *
 * NAME: izot_get_program_id
 *
 ****************************************************************************/
void izot_get_program_id(uint8_t *pId)
{
	memcpy(pId, cp->progId, IZOT_PROGRAM_ID_LENGTH);
}

/****************************************************************************
 *
 * NAME: izot_send_service_pin_wrapper
 *
 ****************************************************************************/
void izot_send_service_pin_wrapper(void)
{
    ManualServiceRequestMessage();
}

/****************************************************************************
 *
 * NAME: izot_device_reset
 *
 ****************************************************************************/
void izot_device_reset(void)
{
    gp->resetNode = TRUE;
    nmp->resetCause = IzotSoftwareReset; /* Software reset. */
}

void izot_get_cpm_sdk_version(char *cpm_sdk_version)
{
    char version[10];
    snprintf(version, 10, "%d.%d.%02d", FIRMWARE_VERSION, FIRMWARE_MINOR_VERSION, FIRMWARE_BUILD);
    strcpy(cpm_sdk_version, version);
}

IzotBool IsPhysicalResetRequested(void)
{
    return do_reset;
}

void PhysicalResetRequested(void)
{
	do_reset = TRUE;
}

/*************************End of node.c*************************/
