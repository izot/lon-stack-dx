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
          File:  lcs_node.c

       Version:  1

     Reference:  Technology Device Data Book.

       Purpose:  Configuration Data Strutures that contain
                 information about this node.
                 Also, define all type defintions needed.

          Note:  cStack supports any number of stacks.
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
         To Do:  None
*******************************************************************************/
/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lcs_eia709_1.h"
#include "lcs_custom.h"
#include "lcs_node.h"
#include "lcs_timer.h"

/*-------------------------------------------------------------------
Section: Constant Definitions
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Type Definitions
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Globals
-------------------------------------------------------------------*/
EEPROM            *eep; /* actual structure is in eeprom.c */
NmMap             *nmp;
NmMap              nm[NUM_STACKS] = {{0}};
ProtocolStackData *gp;
ProtocolStackData  protocolStackDataGbl[NUM_STACKS];

/*-------------------------------------------------------------------
Section: Local Globals
-------------------------------------------------------------------*/
/* See Tech Device Data Book for the tables with these values */

/* Table on page 9-9. Field Value 1 is missing.
   We assume the value 20 for 1 */
static uint16 bufSizeCodeLGbl[16] =
{
    255,20,20,21,22,24,26,30,34,42,50,66,82,114,146,210
};
/* Table is on page 9-10. Field value 1 is missing.
   We assume the value 1 for 1 */
static uint16 bufCntCodeLGbl[16] =
{
    0,1,1,2,3,5,7,11,15,23,31,47,63,95,127,191
};
static uint16 rptTimerCodeLGbl[16] =
{
    16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072
};
static uint16 rcvTimerCodeLGbl[16] =
{
    128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,
    16384,24576
};

/*-------------------------------------------------------------------
Section: Local Function Prototypes
-------------------------------------------------------------------*/
/* None */

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

DomainStruct *AccessDomain(uint8 indexIn)
{
    if (indexIn <= eep->readOnlyData.twoDomains)
    {
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
Status UpdateDomain(DomainStruct *domainInp, uint8 indexIn, Boolean includeKey)
{
    Status sts = SUCCESS;
    int nDomains = eep->readOnlyData.twoDomains ? MAX_DOMAINS : 1;
    if (indexIn < nDomains)
    {
		memcpy(&eep->domainTable[indexIn], domainInp, includeKey ? sizeof(DomainStruct) : sizeof(DomainStruct) - AUTH_KEY_LEN);
    }
    else
    {
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
AddrTableEntry *AccessAddress(uint16 indexIn)
{
    if (indexIn < NUM_ADDR_TBL_ENTRIES)
    {
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
Status UpdateAddress(AddrTableEntry *addrEntryInp, uint16 indexIn)
{
    Status sts = SUCCESS;
    if (indexIn < NUM_ADDR_TBL_ENTRIES)
    {
        eep->addrTable[indexIn] = *addrEntryInp;
    }
    else
    {
        LCS_RecordError(INVALID_ADDR_TABLE_INDEX);
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
Boolean IsGroupMember(Byte domainIndexIn, uint8 groupIn,
                      uint8 *groupMemberOut)
{
    uint16 i;

    for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++)
    {
        if (eep->addrTable[i].addrFormat >= 128)
        {
            /* Group Format */
            if (eep->addrTable[i].groupEntry.groupID == groupIn &&
                    eep->addrTable[i].groupEntry.domainIndex == domainIndexIn)
            {
                break;
            }
        }
    }
    if (i == NUM_ADDR_TBL_ENTRIES)
    {
        return(FALSE); /* Not Found */
    }
    if (groupMemberOut)
    {
        *groupMemberOut = eep->addrTable[i].groupEntry.member;
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
uint16 AddrTableIndex(uint8 domainIndexIn, uint8 groupIn)
{
    uint16 i;

    for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++)
    {
        if (eep->addrTable[i].addrFormat >= 128)
        {
            /* Group Format */
            if (eep->addrTable[i].groupEntry.groupID == groupIn &&
                    eep->addrTable[i].groupEntry.domainIndex == domainIndexIn)
            {
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
uint16  DecodeBufferSize(uint8 bufSizeIn)
{
    if (bufSizeIn <= 15)
    {
        return(bufSizeCodeLGbl[bufSizeIn]);
    }
    ErrorMsg("DecodeBufferSize: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeBufferCnt
Returns:   Actual Buffer Count
Reference: Tech Device Data Rev 1 p.9-10
Purpose:   To compute the actual buffer count from code
Comments:  None
******************************************************************/
uint16  DecodeBufferCnt(uint8 bufCntIn)
{
    if (bufCntIn <= 15)
    {
        return(bufCntCodeLGbl[bufCntIn]);
    }
    ErrorMsg("DecodeBufferCnt: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeRptTimer
Returns:   Actual timer value in ms
Reference: Tech Device Data Rev 1 p.9-17
Purpose:   To compute the actual rpt timer value from code
Comments:  None
******************************************************************/
uint16 DecodeRptTimer(uint8 rptTimerIn)
{
    if (rptTimerIn <= 15)
    {
        return(rptTimerCodeLGbl[rptTimerIn]);
    }
    ErrorMsg("DecodeRptTimer: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeRcvTimer
Returns:   Actual Receive Timer value in ms
Reference: Tech Device Data Rev 1 p.9-17
Purpose:   To compute the actual rcv timer value in ms from code
Comments:  None
******************************************************************/
uint16 DecodeRcvTimer(uint8 rcvTimerIn)
{
    if (rcvTimerIn <= 15)
    {
        return(rcvTimerCodeLGbl[rcvTimerIn]);
    }
    ErrorMsg("DecodeRcvTimer: Invalid code.\n");
    return(0);
}

/*****************************************************************
Function:  DecodeTxTimer
Returns:   Actual Transmit Timer Value in ms
Reference: Tech Device Data Rev 1 p.9-17
Purpose:   To compute the actual transmit timer value from code
Comments:  None
******************************************************************/
uint16 DecodeTxTimer(uint8  txTimerIn, Boolean longTimer)
{
	int v = 16;
	if (longTimer)
	{
		txTimerIn += 16;
	}
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
NVStruct *AccessNV(uint16 indexIn)
{
    if (indexIn < nmp->nvTableSize)
    {
        return(&eep->nvConfigTable[indexIn]);
    }
    ErrorMsg("AccessNV: Invalid index.\n");
    return(NULL);
}

/*****************************************************************
Function:  UpdateNV
Returns:   None
Reference: Tech Device Data Rev 1 p.9-18
Purpose:   To update an entry in NV Config Table
Comments:  None
******************************************************************/
void UpdateNV(NVStruct *nvStructInp, uint16 indexIn)
{
    if (nvStructInp && indexIn < nmp->nvTableSize)
    {
        eep->nvConfigTable[indexIn] = *nvStructInp;
        return;
    }
    if (nvStructInp)
    {
        ErrorMsg("UpdateNV: Invalid index.\n");
    }
    else
    {
        ErrorMsg("UpdateNV: NULL nvStructInp.\n");
    }
}

/*****************************************************************
Function:  NVTableIndex
Returns:   index of NV Config Table
Reference: Tech Device Data Rev 1 p.9-18.
Purpose:   To retrieve the index corresponding to a network varname.
Comments:  We don't need this fn for Ref Imp. as the app pgm already
           has the index for all variables.
******************************************************************/
uint16 NVTableIndex(char varNameIn[])
{
    return(0);
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
#ifdef _DEBUG_LCS
void ErrorMsg(char errMessageIn[])
{
    printf(errMessageIn);
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
void *AllocateStorage(uint16 sizeIn)
{
    Byte *ptr;

    if (gp->mallocUsedSize + sizeIn > MALLOC_SIZE)
    {
	    assert(0);
        LCS_RecordError(MEMORY_ALLOC_FAILURE);
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
void NodeReset(Boolean firstReset)
{
    void APPReset(void), TCSReset(void), TSAReset(void),
    NWReset(void),  LKReset(void),  AppReset(void);

    void (*resetFns[])(void) =
        {APPReset, TCSReset, TSAReset, NWReset, LKReset, AppReset};
    uint8 fnNum, fnsCnt;

#ifdef INCLUDE_PHYSICAL
    if (!firstReset)
    {
        PHYDisableSPMIsr();
    }
#endif

    /* Init variables that are not in EEPROM */
    memset(&nmp->stats, 0, sizeof(StatsStruct));
    gp->prevPinState[0]  = 0;

    /* A node in soft off-line state should go on-line state */
    if (eep->readOnlyData.nodeState == CNFG_ONLINE && gp->appPgmMode == OFF_LINE)
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
            return;
        }
    }

#ifdef INCLUDE_PHYSICAL
    PHYInitSPM(firstReset);
#endif

    if (firstReset)
    {
        memset(gp->prevChallenge, 0, sizeof(gp->prevChallenge));
    }

    if (nmp->resetCause == EXTERNAL_RESET || nmp->resetCause == POWER_UP_RESET)
    {
        MsTimerSet(&gp->tsDelayTimer, TS_RESET_DELAY_TIME);
    }
    gp->resetNode        = FALSE;
}

/*****************************************************************
Function:  InitEEPROM
Returns:   None
Reference: None
Purpose:   To initialize the EEPROM data items based on constants
           in custom.h and values set in custom.c
Comments:  Incomplete Initialization. Make sure it has the var you
           want or else add it here or in custom.h or custom.c
                depending on where it fits.
******************************************************************/
void    InitEEPROM(void)
{
    int i;
    char *p;

	// We first get the persistent data from NVM.
	if (LCS_ReadNvm() != ECHERR_OK)
	{
	  	/* Init the entire readOnlyData to 0 first. */
		memset(&eep->readOnlyData, 0, sizeof(eep->readOnlyData));
		/* Init the entire configData to 0 first. */
		memset(&eep->configData, 0, sizeof(eep->configData));

		/* Init Based on custom.h and default values */
		eep->readOnlyData.modelNum         = MODEL_NUM;
		eep->readOnlyData.minorModelNum    = MINOR_MODEL_NUM;
		eep->readOnlyData.checkSum         = 0;

		eep->readOnlyData.nvFixed[0] = 0xFF; /* not useful */
		eep->readOnlyData.nvFixed[1] = 0xFF;

		eep->readOnlyData.runWhenUnconf    = RUN_WHEN_UNCONF;
		eep->readOnlyData.nvCount          = 0;
		/* MIP uses 0xFFFF for snvtStruct field. p 9-8 */
		eep->readOnlyData.snvtStruct[0]    = 0xFF;
		eep->readOnlyData.snvtStruct[1]    = 0xFF;
		eep->readOnlyData.nodeState        = CNFG_ONLINE;
		/* NUM_ADDR_TBL_ENTRIES can be larger than 15, but
		   addressCnt is set to min(15, NUM_ADDR_TBL_ENTRIES).
		   The remaining entries are not seen by the lonbuilder tool */
		eep->readOnlyData.addressCnt       =
			(NUM_ADDR_TBL_ENTRIES <= 15)?NUM_ADDR_TBL_ENTRIES:15;
		eep->readOnlyData.receiveTransCnt  =
			(RECEIVE_TRANS_COUNT < 16)?RECEIVE_TRANS_COUNT-1:15;
		eep->readOnlyData.appOutBufSize    = APP_OUT_BUF_SIZE;
		eep->readOnlyData.appInBufSize     = APP_IN_BUF_SIZE;
		eep->readOnlyData.nwOutBufSize     = NW_OUT_BUF_SIZE;
		eep->readOnlyData.nwInBufSize      = NW_IN_BUF_SIZE;
		eep->readOnlyData.nwOutBufPriCnt   = NW_OUT_PRI_Q_CNT;
		eep->readOnlyData.appOutBufPriCnt  = APP_OUT_PRI_Q_CNT;
		eep->readOnlyData.appOutBufCnt     = APP_OUT_Q_CNT;
		eep->readOnlyData.appInBufCnt      = APP_IN_Q_CNT;
		eep->readOnlyData.nwOutBufCnt      = NW_OUT_Q_CNT;
		eep->readOnlyData.nwInBufCnt       = NW_IN_Q_CNT;
		eep->readOnlyData.msgTagCnt        = 0;

		eep->readOnlyData.readWriteProtect = READ_WRITE_PROTECT;
		eep->readOnlyData.txByAddress      = 0;
		eep->readOnlyData.aliasCnt         = 0; /* Host based node */

		/* Initialize configData */
		eep->configData.channelId          = 0;
		eep->configData.commClock          = 3;
		eep->configData.inputClock         = 5;
		eep->configData.commType           = SPECIAL_PURPOSE;
		eep->configData.commPinDir         = 0x1E; /* 0x17 if wake-up pin is input */
		eep->configData.reserved[0]        = 0x00; /* for special purpose mode. */
		eep->configData.reserved[1]        = 0x3F; /* packet_cycle */
		eep->configData.reserved[2]        = 0xA6; /* beta2 control */
		eep->configData.reserved[3]        = 0x77; /* xmit_interpacket */
		eep->configData.reserved[4]        = 0x67; /* recv_interpacket */
		eep->configData.nodePriority       = 1; /* 0-255. 0 => no priority slot. */
		eep->configData.channelPriorities  = 8; /* 0-255 */
		eep->configData.param.xcvrParams[0] = 0x0e;
		eep->configData.param.xcvrParams[1] = 0x01;
		eep->configData.param.xcvrParams[2] = 0;
		eep->configData.param.xcvrParams[3] = 0;
		eep->configData.param.xcvrParams[4] = 0;
		eep->configData.param.xcvrParams[5] = 0;
		eep->configData.param.xcvrParams[6] = 0;
		/* dirParams only used for direct mode not special purpose mode */
		/* eep->configData.param.dirParams.bitSyncThreshHold = 1; */
		eep->configData.nonGroupTimer       = NON_GROUP_TIMER;
		eep->configData.nmAuth              = NM_AUTH;
		eep->configData.preemptionTimeout   = 0;

		/* Initialization based on custom.c */
		memcpy(eep->readOnlyData.uniqueNodeId, cp->uniqueNodeId, UNIQUE_NODE_ID_LEN);
		eep->readOnlyData.twoDomains = cp->twoDomains;
		memcpy(eep->readOnlyData.progId, cp->progId, ID_STR_LEN);
		memcpy(eep->configData.location, cp->location, LOCATION_LEN);
		for (i = 0; i <= cp->twoDomains; i++)
		{
			eep->domainTable[i].len = cp->len[i];
			memcpy(eep->domainTable[i].domainId, cp->domainId[i],
				   cp->len[i]);
			eep->domainTable[i].subnet = cp->subnet[i];
			eep->domainTable[i].node   = cp->node[i];
			eep->domainTable[i].cloneDomain = 1;
			memcpy(eep->domainTable[i].key, cp->key[i], AUTH_KEY_LEN);
		}
		/* Init Address Table based on custom.c */
		for (i = 0; i < NUM_ADDR_TBL_ENTRIES; i++)
		{
			memcpy(&eep->addrTable[i], &cp->addrTbl[i], 5);
		}

		/* Init Alias Table based in custom.c */
		/* Since C initializes missing elements with 0 we use
		   any non-zero value for hostPrimary field to indicate that
		   we did initialize an entry. We don't need 0 anyway for
		   hostPrimary as we can use primary for such entries. */

		for (i = 0; i < NV_ALIAS_TABLE_SIZE; i++)
		{
			memcpy(&eep->nvAliasTable[i], &cp->aliasTbl[i], 6);
		}

		nmp->nvTableSize  = 0;

		/* Initialize Alias Tables that are not initialized in custom.h */
		for (i = 0; i < NV_ALIAS_TABLE_SIZE; i++)
		{
			/* Init only those that are not given meaningful values
			   in custom.h */
			if (eep->nvAliasTable[i].hostPrimary != 0)
			{
				continue; /* Skip this as it was initialized in custom.c */
			}

			p = (char *)&eep->nvAliasTable[i];
			*p        = (char) 0x70;
			*(p + 1)  = (char) 0x00;
			*(p + 2)  = (char) 0x0F;
			*(p + 3)  = (char) 0xFF;
			*(p + 4)  = (char) 0xFF;
			*(p + 5)  = (char) 0xFF;
		}
	}
}

/*****************************************************************
Function:  GetPrimaryIndex
Returns:   The primary index of the given variable.
Reference: None
Purpose:   To compute the primary index
Comments:  Given index can be either primary or alias.
******************************************************************/
int16 GetPrimaryIndex(int16 nvIndex)
{
    int16   primaryIndex;

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
        primaryIndex = eep->nvAliasTable[nvIndex].primary;
        if (primaryIndex == 0xFF)
        {
            primaryIndex = eep->nvAliasTable[nvIndex].hostPrimary;
        }
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
NVStruct *GetNVStructPtr(int16 nvIndexIn)
{
    if (nvIndexIn < 0 || nvIndexIn >= nmp->nvTableSize + NV_ALIAS_TABLE_SIZE)
    {
        return(NULL); /* Bad index value. */
    }

    if (nvIndexIn < nmp->nvTableSize)
    {
        return(&eep->nvConfigTable[nvIndexIn]);
    }

    return(&eep->nvAliasTable[nvIndexIn - nmp->nvTableSize].nvConfig);
}

/*****************************************************************
Function:  CheckSum4
Returns:   4 bit checksum of a given data.
Reference: None
Purpose:   To Compute the checksum of an array of bytes of
           a given length. The check sum is the successive
           application of exclusive or of successive 4 bits.
Comments:  None
******************************************************************/
uint8   CheckSum4(void *dataIn, uint16 lengthIn)
{
    unsigned char *p;
    uint16 i;
    uint8 result = 0; /* Final checksum in low order 4 bits. */

    p = dataIn;
    for (i = 0; i < lengthIn; i++)
    {
        result = result ^ (*p >> 4); /* exclusive or with high order
                                      4 bits */
        result = result ^ (*p & 0x0F); /* With low order 4 bits */
        p++;
    }
    return(result);
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
uint8   CheckSum8(void *dataIn, uint16 lengthIn)
{
    unsigned char *p;
    uint16 i;
    uint8 result = 0; /* Final checksum */

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
uint8 ComputeConfigCheckSum(void)
{
    uint8 checkSum;
    uint16 size;

    size = (char*)&eep->configCheckSum - (char *)&eep->configData;
    checkSum = CheckSum8((char *)&eep->configData, size);
    return(checkSum);
}

/*****************************************************************
Function:  IOChanges
Returns:   TRUE if the state of input pin changed.
Reference: None
Purpose:   To determine whether there is a state change in input
           Pin 0.
Comments:  None
******************************************************************/
Boolean IOChanges(uint8 pinNumberIn)
{
    if (pinNumberIn != 0)
    {
        return(FALSE); /* Only Input Pin 0 is supported for now */
    }

    if (gp->prevPinState[0] == 0 && gp->ioInputPin0)
    {
        /* Prev state = released  curstate = pressed */
        gp->prevPinState[0] = 1;
        return(TRUE);
    }
    if (gp->prevPinState[0] == 1 && !gp->ioInputPin0)
    {
        /* prevstate = pressed and curstate = released */
        gp->prevPinState[0] = 0;
        return(TRUE);
    }
    return(FALSE);
}

Boolean IsTagBound(uint8 tagIn)
{
    return(tagIn < nmp->snvt.mtagCount &&
           tagIn < NUM_ADDR_TBL_ENTRIES &&
           eep->addrTable[tagIn].addrFormat != UNBOUND);
}

/****************************************************************
Function: IsNvBound
Returns:  TRUE if the variable is bound. FALSE otherwise.
Purpose:  To determine if a primary variable is bound or not.
Comment:  A variable is bound if its address index is not 0xF
          or there is an alias attached to it whose address
          index is not 0xF.
****************************************************************/
Boolean IsNVBound(int16 nvIndexIn)
{
    uint16 i;
    int16 primaryIndex;
    uint16 addrIndex;

    if (nvIndexIn < 0 || nvIndexIn >= nmp->nvTableSize)
    {
        return(FALSE); /* not an index of a primary network variable */
    }

    /* If the primary has a valid address table index and the address
       table entry is not unbound, then the variable is bound */
    addrIndex = eep->nvConfigTable[nvIndexIn].nvAddrIndex;
    if ( addrIndex != 0x0F &&
            (eep->addrTable[addrIndex].addrFormat != UNBOUND ||
             eep->addrTable[addrIndex].turnaEntry.turnaround == 1) )
    {
        return(TRUE);
    }

    /* Primary is not bound. See if there is an alias for this variable
       that is bound. */
    for (i = 0; i < NV_ALIAS_TABLE_SIZE; i++)
    {
        primaryIndex = GetPrimaryIndex((int16)(i + nmp->nvTableSize));
        addrIndex    = eep->nvAliasTable[i].nvConfig.nvAddrIndex;
        /* If the alias matches the primary, has a valid address table
           index and the address table entry is not UNBOUND, then
           the primary variable is bound */
        if (primaryIndex == nvIndexIn &&
                addrIndex != 0x0F         &&
                (eep->addrTable[addrIndex].addrFormat != UNBOUND ||
                 eep->addrTable[addrIndex].turnaEntry.turnaround == 1) )
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
Boolean AppPgmRuns(void)
{
    /* Normal Mode. Configured and running. */
    if (eep->readOnlyData.nodeState == CNFG_ONLINE &&
            gp->appPgmMode == ON_LINE)
    {
        return(TRUE);
    }

    /* Unconfigured and running. */
    if (eep->readOnlyData.nodeState == APPL_UNCNFG &&
            eep->readOnlyData.runWhenUnconf            &&
            gp->appPgmMode == ON_LINE)
    {
        return(TRUE);
    }

    return(FALSE);
}

/*******************************************************************************
Function: NodeConfigured
Returns:  TRUE if the node configured is valid.
Purpose:  To determine whether currently the node is configured.
*******************************************************************************/
Boolean NodeConfigured(void)
{
    return(eep->readOnlyData.nodeState == CNFG_ONLINE  ||
           eep->readOnlyData.nodeState == CNFG_OFFLINE);
}

/*******************************************************************************
Function: NodeUnConfigured
Returns:  TRUE if the node is configured.
Purpose:  To determine whether currently node is unconfigured.
*******************************************************************************/
Boolean NodeUnConfigured(void)
{
    return(eep->readOnlyData.nodeState == APPL_UNCNFG  ||
           eep->readOnlyData.nodeState == NO_APPL_UNCNFG);
}

/*******************************************************************************
Function: RecordError
Returns:  void
Purpose:  Log an error to the error log.
*******************************************************************************/
void LCS_RecordError(LcsErrorLog err)
{
  	// To avoid wearing out the NVM in case there is repeated logging of the same
    // error, check for a change first.
	if (eep->errorLog != err)
	{
    	eep->errorLog = err;
		LCS_WriteNvm();
	}
}

/*************************End of node.c*************************/
