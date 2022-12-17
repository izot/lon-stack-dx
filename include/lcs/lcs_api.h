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
       File:      lcs_api.h

    Version:      1

  References:     Protocol Spec:
                  Section 10. Application Layer
                  Section 10.6. Application Protocol State
                  Variables

  Purpose:        API: Application Program Interface
                  This is the interface file for the API.

  Note:           The API is described in the Reference Implementation
                  Overview document.

  To Do:          None.
*********************************************************************/

#ifndef _API_H
#define _API_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include "lcs_eia709_1.h"
#include "lcs_custom.h"
#include "lcs_timer.h"

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
/* None */

/*********************************************************************
Section: Type Definitions
*********************************************************************/

#pragma pack(push, 1)

typedef union
{
    struct
    {
        BITS2(apFlag,       2,
              apCode,     6)
    } ap;
    struct
    {
        BITS3(nvFlag,       1,
              nvDir,      1,
              nvCode,     6)
    } nv;
    struct
    {
        BITS2(nmFlag,       3,
              nmCode,     5)
    } nm;
    struct
    {
        BITS2(ndFlag,       4,
              ndCode,     4)
    } nd;
    struct
    {
        BITS2(ffFlag,       4,
              ffCode,     4)
    } ff;
    uint8 allBits;
} DestinType;

/* Addresses used by API message structures */
typedef struct
{
    BITS2(  groupFlag,              1,      // 1 => group
            groupSize,              7)      // group size (0 => huge group)
    BITS2(  domainIndex,            1,
            member,                 7)
    BITS2(  rptTimer,               4,      // unackd_rpt timer
            retryCount,             4)
    BITS2(  rcvTimer,               4,      // receive timer index
            txTimer,                4)      // transmit timer index
    Byte        groupID;
} GroupAddrMode;

typedef struct
{
	BITS6(  mbz,                    1,
		    keyOverride,            1,
			unused1,	            1,
		    longTimer,				1,
			synczero,               1,
			addrMode,               3)
    BITS2(  domainIndex,            1,
            node,                   7)
    BITS2(  rptTimer,               4,      // unackd_rpt timer
            retryCount,             4)
    BITS2(  unused,                 4,
            txTimer,                4)      // transmit timer index
    Byte     subnetID;                      // subnet ID
} SNodeAddrMode;

typedef struct
{
	BITS6(  mbz,                    1,
		    keyOverride,            1,
			broadcastGroup,         1,
		    longTimer,				1,
			synczero,               1,
			addrMode,               3)
    BITS3(  domainIndex,            1,
            reserved,               1,
            backlog,                6)      // backlog override value
    BITS2(  rptTimer,               4,      // unackd_rpt timer
            retryCount,             4)
    BITS2(  maxResponses,           4,      // maximum responses to deliver to app
            txTimer,                4)      // transmit timer index
    Byte     subnetID;                      // subnet ID
} BcastAddrMode;

typedef struct
{
	BITS6(  mbz,                    1,
		    keyOverride,            1,
			unused1,	            1,
		    longTimer,				1,
			synczero,               1,
			addrMode,               3)
    BITS2(  domainIndex,            1,
            reserved,               7)
    BITS2(  rptTimer,               4,      // unackd_rpt timer
            retryCount,             4)
    BITS2(  unused,                 4,
            txTimer,                4)      // transmit timer index
    Byte     subnetID;                       // subnet ID
    uint8    uniqueId[UNIQUE_NODE_ID_LEN]; /* unique node ID      */
} UniqueNodeIdAddrMode;

typedef union
{
    AddrMode          noAddress;  /* UNBOUND: 0 if no address    */
    GroupAddrMode     group;
    SNodeAddrMode     snode;
    UniqueNodeIdAddrMode uniqueNodeId;
    BcastAddrMode     bcast;
} MsgOutAddr;

/* Typedef for 'resp_in_addr', type of the field 'resp_in.addr'  */
typedef struct
{
    BITS2(  domain,     1,
            flexDomain, 1)
    struct
    {
        uint8    subnet;
        BITS2(  snodeFlag,  1,      // 0=> group response; 1=> snode response
                node,       7)
    } srcAddr;
    union
    {
        struct
        {
            uint8    subnet;
            BITS2(  reserved,       1,
                    node,           7)
        } snode;
        struct
        {
            uint8    subnet;
            BITS2(  reserved1,      1,
                    node,           7)
            uint8    group;
            BITS2(  reserved2,      2,
                    member,         6)
        } group;
    } destAddr;
} RespInAddr;

typedef struct
{
    BITS3(domain,           1,
          flexDomain,     1,
          format,         6)      // 0 => Broadcast 1 => group
    // 2 => subnet/node 3 => Unique Node Id
    struct
    {
        uint8    subnet;
        BITS2(  reserved,       1,
                node,           7)
    } srcAddr;
    union
    {
        uint8  bcastSubnet;  /* bcast dest address */
        uint8  group;        /* group destination */
        struct
        {
            uint8    subnet;
            BITS2(  reserved,       1,
                    node,           7)
        } snode;
        struct
        {
            uint8 subnet;
            Byte  uniqueId[UNIQUE_NODE_ID_LEN];
        } uniqueNodeId;
    } destAddr;
} MsgInAddr;

typedef struct
{
    BITS3(  domain,         1,
            flexDomain,     1,
            format,         6)  // 0 => Broadcast 1 => group
    // 2 => subnet/node 3 => Unique Node Id
    // 4 => turnaround
    struct
    {
        uint8    subnet;
        BITS2(  reserved,   1,
                node,       7)
    } srcAddr;
    struct
    {
        uint8  group;
    } destAddr;
} NvInAddr;

#pragma pack(pop)

/* Message Declarations ****************************************** */

typedef struct
{
    uint8         code; /* message code                      */
    uint8         len;  /* length of message data            */
    uint8         data[MAX_DATA_SIZE];  /* message data      */
    Boolean       authenticated; /* TRUE if msg was authenticated */
    ServiceType   service;       /* Service used to send the msg  */
    RequestId     reqId;         /* Request ID to match responses   */
    MsgInAddr     addr;
} MsgIn;

typedef struct
{
    Boolean     priorityOn;    /* TRUE if a priority message     */
    MsgTag      tag;           /* to correlate completion codes  */
    uint8       len;           /* message length in data array below */
    uint8       code;          /* message code                   */
    uint8       data[MAX_DATA_SIZE];  /* message data            */
    Boolean     authenticated; /* TRUE if to be authenticated    */
    ServiceType service;       /* service type used to send msg  */
    MsgOutAddr  addr;          /* destination address (see above)*/
} MsgOut;

typedef struct
{
    MsgTag      tag;                   /* To match req    */
    uint8       code;                  /* message code    */
    uint8       len;                   /* message length  */
    uint8       data[MAX_DATA_SIZE];   /* message data    */
    RespInAddr  addr;  /* destination address (see above) */
} RespIn;             /* struct for receiving responses  */

typedef struct
{
    RequestId  reqId;         /* Request ID to match responses */
    Boolean    nullResponse;  /* TRUE => no resp goes out */
    uint8      code;          /* message code */
    uint8      len;           /* message length  */
    uint8      data[MAX_DATA_SIZE];   /* message data */
} RespOut;                   /* structure for sending responses */


/*******************************************************************
NVDefinition is used to describe network variable properties.
These are used to create a new network variable with proper
attributes in network variable tables and SNVT structures.
*******************************************************************/
typedef struct
{
	uint8  priority;		// 1 => priority
	uint8  direction;		// NV_OUTPUT or NV_INPUT
	uint16 selector;		// Present only for non-bindable
	uint8  bind;
	uint8  turnaround;		// 1 => turnaround
	uint8  service;			// ACKD, UNACK_RPT, UNACKD
	uint8  auth;			// 1 => authenticated
	uint8  explodeArray;	// 1 => explode arrays in SNVT structure
	uint8  nvLength;		// length of NV in bytes.  For arrays, give the size of each item
    uint8  snvtDesc;         /* snvtDesc_struct in byte form. Big_Endian */
    uint8  snvtExt;          /* Extension record. Big_Endian. */
    uint8  snvtType;         /* 0 => non-SNVT variable. */
    uint8  rateEst;
    uint8  maxrEst;
    uint16 arrayCnt;         /* 0 for simple variables. dim for arrays. */
    char  *nvName;           /* Name of the network variable */
    char  *nvSdoc;           /* Sel-doc string for the variable */
    void  *varAddr;          /* Address of the variable. */
} NVDefinition;

/* API Functions ***************************************************/
Boolean  MsgAlloc(void);          /* Returns TRUE if msg allocated */
Boolean  msg_alloc(void);
Boolean  MsgAllocPriority(void);  /* Returns TRUE if msg allocated */
Boolean  msg_alloc_priority(void);
void     MsgSend(void);           /* Reads msgOut, sends message   */
void     msg_send(void);
void     MsgCancel(void);         /* Cancels MsgAlloc() or         */
/*   msgAllocPriority()          */
void     msg_cancel(void);
void     MsgFree(void);           /* Releases data in msgIn        */
void     msg_free(void);
Boolean  msgReceive(void);        /* TRUE if there is msg          */
Boolean  msg_receive(void);

Boolean  RespAlloc(void);         /* Returns TRUE if resp allocated*/
Boolean  resp_alloc(void);
void     RespSend(void);          /* Reads respOut, sends response */
void     resp_send(void);
void     RespCancel(void);        /* Cancels RespAlloc()           */
void     resp_cancel(void);
void     RespFree(void);          /* Releases data in respIn       */
void     resp_free(void);
Boolean  RespReceive(void);
Boolean  resp_receive(void);

/********************************************************************
   Add a network variable with the given info.  Returns the index of
   the network variable that should be used with other functios such
   as SendNV, GetNVAddr.  For array variables, only one index value is
   returned, however each element is considered like a separate
   network variable
*********************************************************************/
int16    AddNV(NVDefinition *);

/* To send all network output variables in the node.
   Polled or not, Use Propagate function. */
void Propagate(void);

/* To send one simple network variable or a whole array */
void PropagateNV(int16 nvIndex);

/* To send an array element or any other simple variable.*/
void PropagateArrayNV(int16 arrayNVIndex, int16 index);

/* To poll all input network variables */
void  Poll(void);

/* To poll a specific simple input network variable or an array */
void  PollNV(int16 nvIndex);

/* Poll a specific array element or any other simple variable. */
void PollArrayNV(int16 arrayNVIndex, int16 index);

/* Application can call this fn to put itself offline */
void GoOffline(void);

/* Appplication can call this fn to put itself unconfigured */
void GoUnconfigured(void);

/* To get a new message tag. NewMsgTag(BIND) or NewMsgTag(NOBIND) */
MsgTag NewMsgTag(BindNoBind bindStatusIn);

/* To send a manual service request message  */
Boolean ManualServiceRequestMessage(void);

/* Functions that must be defined in the application program.       */
Status AppInit(void);             /* Application initialization      */
void  AppReset(void);            /* Code after a reset              */
void  DoApp(void);               /* Application processing          */
/* MsgCompletes is called when an explicit message has completed. */
void  MsgCompletes(Status stat, MsgTag tag);
/* NVUpdateCompletes is called when an nv update or nv poll completes. The 2nd
   parameter is the array index for array variables, 0 for simple variables. */
void  NVUpdateCompletes(Status stat, int16 nvIndex, int16 nvArrayIndex);
/* NVUpdateOccurs is called when an input nv has been changed. The 2nd
   parameter is the array index for array variables, 0 for simple variables. */
void  NVUpdateOccurs(int16 nvIndex, int16 nvArrayIndex);
void  Wink(void);                /* Call to app wink clause         */
void  OfflineEvent(void);        /* Going Application Offline       */
void  OnlineEvent(void);         /* Going Application Online        */
/* End of Functions that must be defined in the application program.*/


#endif   /* #ifndef _API_H */
/********************************api.h*******************************/
