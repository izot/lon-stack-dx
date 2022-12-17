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
  File:           lcs_node.h

  Version:        1

  Purpose:        To define all the type definitions needed by all
                  the upper layers of the stack.
                  To also define all function to interface with
                  some of these data structures as specified in
                  the Device Data Book.

  Note:           None

  To Do:          None.
*********************************************************************/
#ifndef _NODE_H
#define _NODE_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include "lcs_eia709_1.h"
#include "lcs_api.h"
#include "lcs_queue.h"

/*-------------------------------------------------------------------
Section: Constant Definitions
-------------------------------------------------------------------*/
/* Define the size of the table maintained by the transaction control
   sublayer that keeps track of each possible destination address
   in packets sent to make sure that we don't assign the same tid as in the
   last transaction to that destination. This constant is needed here
   so that we can use it in the type definition of protocol stack data. */
#define TID_TABLE_SIZE 10

/* Given a valid primary index of a network variable, get its address */
#define NV_ADDRESS(i) (nmp->nvFixedTable[i].nvAddress)

/* Given a valid primary index of a network variable, get its length */
#define NV_LENGTH(i)  (nmp->nvFixedTable[i].nvLength)

/* Given a valid primary index of a network variable, check if it is sync */
#define NV_SYNC(i)    (nmp->nvFixedTable[i].nvSync)

/*-------------------------------------------------------------------
Section: Type Definitions
-------------------------------------------------------------------*/
/* Reference: Tech Device Data: Page 9-6. */

/* Turn off alignment by compiler to make sure that the structure sizes
   are as we intend them to be with no padding */

#pragma pack(push, 1)

typedef struct
{
    Byte  uniqueNodeId[UNIQUE_NODE_ID_LEN]; /* 48-bit unique ID.              */
    Byte  modelNum;                  /* Model Number for Ref. Impl.           */
    BITS2(checkSum         , 4,      /* For Unique Node ID.                   */
          minorModelNum    , 4)      /* 0-128.                                */
    Byte  nvFixed[2];                /* Location of nv fixed table.           */
    BITS3(readWriteProtect , 1,      /* read+write protect flag.              */
          runWhenUnconf    , 1,      /* 1=> Application runs.                 */
          nvCount          , 6)      /* 0 for reference implementation.       */
    Byte  snvtStruct[2];             /* 0xFFFF for reference implementation.  */
    char  progId[ID_STR_LEN];        /* Program Id string.                    */
    BITS6(r1               , 1,      /* nvProcessingOff not used in Ref. Impl.*/
          twoDomains       , 1,      /* 1 if node is in 2 domains.            */
          r2               , 1,      /* explicitAddr not used in Ref. Impl.   */
          r3               , 1,      /* Unused.                               */
          r4               , 1,      /* Unused.                               */
          nodeState        , 3)      /* Node State. See eia709_1.h            */
    BITS2(addressCnt       , 4,      /* # of entries in address table.        */
          r5               , 4)      /* Unused.                               */
    BITS2(r6               , 4,      /* Unused.                               */
          receiveTransCnt  , 4)      /* RR Cnt = this field + 1               */
    BITS2(appOutBufSize    , 4,      /* Special Size Encoding.                */
          appInBufSize     , 4)      /* Special Size Encoding.                */
    BITS2(nwOutBufSize     , 4,      /* Special Size Encoding.                */
          nwInBufSize      , 4)      /* Special Size Encoding.                */
    BITS2(nwOutBufPriCnt   , 4,      /* Special Count Enconding.              */
          appOutBufPriCnt  , 4)      /* Special Count Enconding.              */
    BITS2(appOutBufCnt     , 4,      /* Special Count Enconding.              */
          appInBufCnt      , 4)      /* Special Count Enconding.              */
    BITS2(nwOutBufCnt      , 4,      /* Special Count Enconding.              */
          nwInBufCnt       , 4)      /* Special Count Enconding.              */
    Byte  reserved0;                 /* Unused                                */
    Byte  reserved1[2];              /* Unused.                               */
    Byte  reserved2[3];              /* Unused.                               */
    BITS3(r7               , 6,      /* Unused.                               */
          txByAddress      , 1,      /* 0 in reference implementation.        */
          r8               , 1)      /* Unused.                               */
    BITS2(r9               , 2,      /* Unused.                               */
          aliasCnt          , 6)      /* 0 in reference implementation.        */
    BITS2(msgTagCnt        , 4,      /* 0 in reference implementation.        */
          r10              , 4)      /* Unused.                               */
    Byte reserved3[3];               /* Unused.                               */
} ReadOnlyDataStruct;

typedef struct
{
    Byte  domainId[DOMAIN_ID_LEN];
    Byte  subnet;
    BITS2(cloneDomain , 1,
          node        , 7)
    BITS4(invalid	  , 1,
	      unused      , 2,
		  authType    , 2,			// 1 => OMA (0 and 3 mean standard - the 3 is for backward compatibility with tools that wrote all ones to the domain length byte)
		  len         , 3)
} LogicalAddress;
// REMINDER - Want DomainStruct to be a function of LogicalAddress but that means a lot of changes...
/* Reference: Tech Device Data Book. Page 9-11. */
typedef struct
{
    Byte  domainId[DOMAIN_ID_LEN];
    Byte  subnet;
    BITS2(cloneDomain , 1,
          node        , 7)
    BITS4(invalid	  , 1,
	      unused      , 2,
		  authType    , 2,			// 1 => OMA (0 and 3 mean standard - the 3 is for backward compatibility with tools that wrote all ones to the domain length byte)
		  len         , 3)
    Byte  key[AUTH_KEY_LEN];
} DomainStruct;
// authType literals
#define AUTH_STD		0
#define AUTH_OMA		1
#define AUTH_STD_OLD	3

typedef struct
{
    AddrMode  addrMode;             /* UNBOUND (0) => turnaround.   */
    Byte      turnaround;           /* 1=>turnaround; 0=>not in use.*/
    BITS2(    unacdkRptTimer  , 4,  /* unackd_rpt timer.            */
              retryCount      , 4)  /* retry count.                 */
    BITS2(    unused          , 4,  /* UNUSED.                      */
              txTimerIndex    , 4)  /* transmit timer index.        */
} TurnaroundStruct;

/* Other types used in the following structure are defined in api.h. */
typedef union
{
    uint8             addrFormat; /* Format of entry. */
    GroupAddrMode     groupEntry;
    SNodeAddrMode     snodeEntry;
    BcastAddrMode     bcastEntry;
    TurnaroundStruct  turnaEntry;
} AddrTableEntry;

/* Reference: Tech Device Data: Page 9-24 */
typedef struct
{
    BITS4(collisionDetect    ,1,
          bitSyncThreshHold  ,2, /* Encoded. See page 9-28 */
          filter             ,2,
          hysteresis         ,3)
    BITS3(unUsed             ,6,
          cdTail             ,1,
          cdPreamble         ,1)
} DirectParamStruct;

typedef struct
{
    int16       channelId;              /* Id of channel assigned */
    Byte        location[LOCATION_LEN]; /* 6 byte ascii string    */
    BITS2(      commClock    ,5,  /* bit rate ratio    */
                inputClock   ,3)  /* input clk osc freq     */
    BITS2(      commType     ,3,        /* type of receiver       */
                commPinDir   ,5)        /* dir comm port pins     */
    Byte        reserved[5];
    Byte        nodePriority;           /* priority slot used     */
    Byte        channelPriorities;      /* # of priority slots    */
    union {
        Byte               xcvrParams[NUM_COMM_PARAMS];
        DirectParamStruct  dirParams;
    } param;
    BITS3(      nonGroupTimer     ,4,
                nmAuth            ,1, /* NetMgt msgs needs auth? */
                preemptionTimeout ,3) /* free buffer wait time in
                                           preemption mode         */
} ConfigDataStruct;

/* Reference: Tech Device Data: Page 9-17 */
/* Network Variable Tables */
typedef struct
{
    BITS3(nvPriority     ,1,  /* NV uses priority messaging       */
          nvDirection    ,1,  /* 1 => output var                  */
          nvSelectorHi   ,6)  /* Hi & Lo form NV selector         */
    Byte  nvSelectorLo     ;  /* Range 0-0x3FFF                   */
    BITS4(nvTurnaround   ,1,  /* 1 ==> bound to a nv in this node */
          nvService      ,2,  /* ACKD or UNACKD_RPT or UNACKD     */
          nvAuth         ,1,  /* 1 => NV used auth transactions   */
          nvAddrIndex    ,4)  /* index into addr table. 15 is spcl */
} NVStruct;

typedef struct
{
    NVStruct nvConfig;
    uint8    primary;   /* index into NV Cfg tbl. 0xFF => next fld */
    uint16   hostPrimary; /* NV cfg tbl index. For host nodes */
} AliasStruct;

typedef struct
{
    BITS3(nvSync    ,1, /* 1 ==> Synchronous network var */
          unused    ,2, /* unused */
          nvLength  ,5) /* # of bytes in the Network Var */
    void *nvAddress;   /* Ptr to variable's data */
} NVFixedStruct;

typedef struct
{
  	UInt8  stats[LcsNumStats*2];
    uint8  eepromLock;
 } StatsStruct;

#pragma pack(pop)

/* yes: Yes, error is implemented in reference implementation */
/* na:  Not applicable (not implemented) in ref. imp.         */
typedef enum
{
    NO_ERRORS                        = 0,    /* yes */
    BAD_EVENT                        = 129,  /* na  */
    NV_LENGTH_MISMATCH               = 130,  /* yes */
    NV_MSG_TOO_SHORT                 = 131,  /* yes */
    EEPROM_WRITE_FAIL                = 132,  /* na  */
    BAD_ADDRESS_TYPE                 = 133,  /* yes */
    PREEMPTION_MODE_TIMEOUT          = 134,  /* na  */
    ALREADY_PREEMPTED                = 135,  /* na  */
    SYNC_NV_UPDATE_LOST              = 136,  /* na  */
    INVALID_RESP_ALLOC               = 137,  /* na  */
    INVALID_DOMAIN                   = 138,  /* yes */
    READ_PAST_END_OF_MSG             = 139,  /* na  */
    WRITE_PAST_END_OF_MSG            = 140,  /* na  */
    INVALID_ADDR_TABLE_INDEX         = 141,  /* yes */
    INCOMPLETE_MSG                   = 142,  /* na  */
    NV_UPDATE_ON_OUTPUT_NV           = 143,  /* na  */
    NO_MSG_AVAIL                     = 144,  /* na  */
    ILLEGAL_SEND                     = 145,  /* na  */
    UNKNOWN_PDU                      = 146,  /* yes */
    INVALID_NV_INDEX                 = 147,  /* yes */
    DIVIDE_BY_ZERO                   = 148,  /* na  */
    INVALID_APPL_ERROR               = 149,  /* na  */
    MEMORY_ALLOC_FAILURE             = 150,  /* yes */
    WRITE_PAST_END_OF_NET_BUFFER     = 151,  /* na  */
    APPL_CS_ERROR                    = 152,  /* na  */
    CNFG_CS_ERROR                    = 153,  /* na  */
    INVALID_XCVR_REG_ADDR            = 154,  /* na  */
    XCVR_REG_TIMEOUT                 = 155,  /* na  */
    WRITE_PAST_END_OF_APPL_BUFFER    = 156,  /* na  */
    IO_READY                         = 157,  /* na  */
    SELF_TEST_FAILED                 = 158,  /* na  */
    SUBNET_ROUTER                    = 159,  /* na  */
    AUTHENTICATION_MISMATCH          = 160,  /* yes */
    SELF_INST_SEMAPHORE_SET          = 161,  /* na  */
    READ_WRITE_SEMAPHORE_SET         = 162,  /* na  */
    APPL_SIGNATURE_BAD               = 163,  /* na  */
    ROUTER_FIRMWARE_VERSION_MISMATCH = 164   /* na  */
} LcsErrorLog;

/*-------------------------------------------------------------------
  NWSendParam is used to process Send Request to Network Layer
  from other layers. When other layers deposit a new PDU into the
  Output Queue, they should also fill in the details of this
  data structure to indicate the kind of request.

  altPath is set by transport/session layers for the last 2 retries.
  destAddr has the domainIndex which is used to determine the
  domainId, source subnet/node. The possible values for the
  domainIndex are 0 or 1 or 2. 0 or 1 ==> use the corresponding
  domainTable. 2 ==> use the flex domain.
-------------------------------------------------------------------*/
typedef struct
{
    DestinationAddress    destAddr; /* To give dest addr  */
    PDUType               pduType;  /* APDU, SPDU, TPDU, AuthPDU */
    MsgTag                tag;      /* used only for APDU */
    uint8                 deltaBL;  /* Upper Layers supply this */
    Boolean               altPath;
    uint16                pduSize;  /* Size of the PDU sent */

    Boolean               dropIfUnconfigured; /* drop the packet if the
                           node is unconfigured. */
} NWSendParam;

/*-------------------------------------------------------------------
  NWReceiveParam: Is used by Link Layer when it supplies an incoming
  NPDU to the network layer. This data structure contains information
  regarding the incoming NPDU.
*******************************************************************/
typedef struct
{
    Boolean               priority;   /* Was it a priority message? */
    Boolean               altPath;
    uint16                pduSize;    /* Size of NPDU in queue */
	XcvrParam			  xcvrParams;
} NWReceiveParam;

/* Type Definitions for Application Layer */
typedef struct
{
    DestinType code;
    Byte data[MAX_DATA_SIZE];
} APDU;

typedef APDU *APDUPtr;

/* APPSendParam is used by API to give info to app layer
   regarding APDU that is going out. Most info is available
   in msg_out in the Application Out Buffers */

/* APPSendParam has everything in MsgOut except code and data. */
typedef struct
{
    MsgTag      tag;           /* to correlate completion codes  */
    uint8       len;           /* message length in app data     */
    Boolean     authenticated; /* TRUE if to be authenticated    */
    ServiceType service;       /* service type used to send msg  */
    RequestId   reqId;         /* Request ID for responses */
    MsgOutAddr  addr;          /* destination address (see above)*/
    Boolean     nullResponse;  /* For responses                  */
} APPSendParam;

/* Types of messages that are received by application layer */
typedef enum {
    MESSAGE    = 0,
    COMPLETION = 1,  /* Indication by Transport/Session layers */
} APIndType;


/* APPReceiveParam is used by Application Layer to figure out
   information regarding incoming APDU. */
typedef struct
{
    APIndType             indication; /* Type of APDU received */
    Boolean               success;    /* Used if indication ==
                                        COMPLETION */
    MsgTag                tag;        /* Tag for indication
                                        or for matching response */
    SourceAddress         srcAddr;    /* Who sent it?            */
    ServiceType           service;    /* Which service type?     */
    Boolean               priority;   /* Was it a priority msg?  */
    Boolean               altPath;    /* Was it sent in alt path? */
    uint16                pduSize;    /* Size of incoming APDU   */
    uint16                taIndex;    /* Turnaround var Index    */
    Boolean               auth;       /* Was it authenticated?   */
    RequestId             reqId;      /* Assigned by session to match
                                        responses later         */
	Boolean				  proxy;	  // 1=>Proxy 
	Boolean				  proxyDone;  // 1=>Proxy transaction completed
	uint8				  proxyCount; // Original proxy hop count.
	XcvrParam			  xcvrParams; // Transceiver parameters
} APPReceiveParam;

/* Type Definition for Transaction Control SubLayer */
typedef struct
{
    TransNum    transNum;       /* initial value = 0. Range 0..15 */
    Boolean     inProgress;     /* Is the transaction in progress */
} TransCtrlRecord;

/* Type definition for table used while assigning new TId. */
/* For more information, see tcs.h or tcs.c */
typedef struct
{
    Byte  domainId[DOMAIN_ID_LEN];
    uint8 len; /* domain length */
    AddrMode              addressMode;
    union
    {
        SubnetAddress         subnetNode;
        MulticastAddress      group;   /* group number of multicast */
        BroadcastAddress      subnet;  /* 0 if domainwide broadcast */
        Byte                  uniqueNodeId[UNIQUE_NODE_ID_LEN];
    } addr;
    MsTimer                  timer;
    TransNum                 tid;    /* Last TID used for this addr */
} TIDTableEntry;

/* Type Definitions for transport, Session, Auth Layers */

typedef enum
{
    TRANS_CURRENT,
    TRANS_NOT_CURRENT,
    TRANS_NEW,
    TRANS_DUPLICATE
} TransStatus;

typedef enum
{
    UNUSED_TX,        /* Transmit Record is unused */
    SESSION_TX,       /* Transmit Record is used by Session */
    TRANSPORT_TX      /* Transmit Record is used by Transport */
} TXStatus;

typedef enum
{
    UNUSED_RR,       /* Receive Record is unused */
    SESSION_RR,      /* Receive Record is used by Session */
    TRANSPORT_RR     /* Receive Record is used by Transport */
} RRStatus;

typedef enum
{
    JUST_RECEIVED,   /* Msg just received. Nothing has been done.  
						Could indicate failure to get an app buffer */
    DELIVERED,       /* Msg has been delivered to app layer. Receive
                       Timer has not expired yet. Waiting for Resp */
    DONE,            /* Msg delivered & a null resp received.
                       Or Msg delivered and Server received has received
                       the response.
                       Timer has not expired yet */
    AUTHENTICATED,   /* Msg has been authenticated. Not delivered   */
    AUTHENTICATING,  /* Msg is being authenticated. reply expected  */
    RESPONDED,       /* Rsp has been recvd from app layer & responded
                       at least once. Recv Timer not expired yet   */
} TransactionState;  /* Constant used by Transport & Session */

typedef struct
{
	Boolean				 altKey;	// TRUE => use alternate authentication key
	uint8				 altKeyValue[2][DOMAIN_ID_LEN]; // Key used when altKey is true
} AltKey;

typedef struct
{
    TXStatus           status;               /* Who owns it? if not free  */
    DestinationAddress nwDestAddr;           /* Destination Address */
    Boolean            ackReceived[MAX_GROUP_NUMBER+1];
    /* Array[0..MAX_GROUP_NUMBER] of Boolean   */
    uint8              destCount;            /* Number of destinations    */
    uint8              ackCount;             /* Or respCount              */
    TransNum           transNum;
    uint16             xmitTimerValue;
    MsTimer            xmitTimer;            /* Transmit Timer            */
    uint8              retriesLeft;          /* How many left?            */
    APDU              *apdu;                 /* APDU transmitted          */
    uint16             apduSize;             /* Size of APDU              */
    Boolean            auth;                 /* Does this msg need auth?  */
	uint16			   txTimerDeltaLast;	 // Time to add to last retry timer
	AltKey			   altKey;				 // Alternate authentication info.
} TransmitRecord;

typedef struct
{
    RRStatus             status;         /* used? Who is using?    */
    SourceAddress        srcAddr;        /* Who sent this?         */
    TransNum             transNum;
    RequestId            reqId;          /* For matching response  */
    MsTimer              recvTimer;      /* receive timer          */
    TransactionState     transState;     /* What state is it in    */
    Boolean              priority;
    Boolean              altPath;        /* Was alt path used?     */
    Boolean              auth;           /* TRUE if auth succeeded */
    Boolean              needAuth;       /* Need authentication    */
    ServiceType          serviceType;    /* What type of service   */
    Byte                 rand[8];        /* For authentication     */
    APDU                *response;       /* Store the response     */
    uint16               rspSize;
    APDU                *apdu;          /* Store the APDU received */
    uint16               apduSize;
	XcvrParam			 xcvrParams;
} ReceiveRecord;

/********************************************************************
   TSASendParam is used to provide the necessary information
   needed to process the transaction.
   MsgOutAddr has all the necessary information for forming the
   destination address. It also has the domainIndex that can be
   used to determine the domainId, source subnet/node.
   However to facilitate sending of messages even when a node
   is not in any domain (such as sending ManualServiceRequest) this structure
   provides a way to specify the domainIndex with different
   possible values.
   domainIndex = 0 or 1  --> use the corresponding domain table
   domainIndex = 2 ---> use the flexDomain given
   domainIndex = 3 ---> use the domain table as given in destAddr.
   For response messages, several fields such as the destAddr or
   domainIndex is not needed.They are determined from the
   corresponding request message in the receive records.
********************************************************************/
typedef struct
{
    MsgOutAddr           destAddr;  /* Whom to send? Need Timers too */
	Domain				 dmn;
    ServiceType          service;   /* What type of service? */
    Boolean              auth;      /* Need Authentication?  */
    uint16               apduSize;  /* Size of APDU to be sent */
    MsgTag               tag;       /* For service indication  */
    Boolean              nullResponse; /* TRUE => no resp goes out */
	Boolean				 flexResponse; /* TRUE => send response on flex domain */
    RequestId            reqId;     /* Set if service=RESPONSE */

    Boolean              altPathOverride;
    Boolean              altPath; /* Used only if altPathOverride is true */
	Boolean				 priority;  // TRUE => priority
	Boolean				 proxy;		// TRUE => proxy
	Boolean			     proxyDone; // 1=>Proxy transaction completed
	Boolean				 txInherit; // 1=>Inherit TX# from proxy source.  Only valid for proxy
	uint8				 proxyCount;// Original proxy hop count.
	uint16				 txTimerDeltaLast; // Amount to add to the last retry timer.  Only valid for proxy
	AltKey				 altKey;	// Alternate authentication key info
} TSASendParam;

/********************************************************************
   TSAReceiveParam is used to receive the necessary
   information to process incoming PDU (TPDU or SPDU or AuthPDU).
   Receive Timer is set based on info in srcAddr.
   srcAddr has the source subnet/node, domainIndex used, group if
   any etc.
********************************************************************/
typedef struct
{
    SourceAddress         srcAddr;    /* Who send it? */
    Boolean               priority;   /* Was it a priority msg?  */
    uint16                pduSize;    /* Size of incoming PDU */
    PDUType               pduType;    /* What type of PDU? */
    Boolean               altPath;    /* Was it sent in alt path? */
	XcvrParam			  xcvrParams; /* Transceiver Parameters */
} TSAReceiveParam;

typedef struct
{
    uint8   deltaBL;  /* What is the backlog generated by this msg? */
    Boolean altPath;  /* Should altPath be used? */
    uint16  pduSize;  /* Size of NPDU */
} LKSendParam;

/* SNVT data structures */

/* See comment in app.c in APPReset and AddNV for a description */
/* of how the SNVT area is layed out and managed.               */

typedef struct
{
    BITS8(extRec            , 1,
          nvSync            , 1,
          nvPolled          , 1,
          nvOffline         , 1,
          nvServiceConfig   , 1,
          nvPriorityConfig  , 1,
          nvAuthConfig      , 1,
          nvConfigClass     , 1)
    uint8 snvtTypeIndex;
} SNVTdescStruct;

typedef struct
{
    BITS3(        bindingII    , 1,
                  queryStats   , 1,
                  aliasCount   , 6)
    uint16        hostAlias;     // Warning - stored in host order so not for internal consumption
} AliasField;

typedef struct
{
    uint16          length;          // Warning - stored in host order so not for internal consumption
    uint8           numNetvars;
    uint8           version;
    uint8           msbNumNetvars;
    uint8           mtagCount;
    char            sb[SNVT_SIZE];
    SNVTdescStruct  *descPtr; /* Point to next SNVTdesc entry in sb */
    /* Actually, it points Node Self-Doc
       String. Just before that is the
       self-id info for the last netwar */
    AliasField      *aliasPtr; /* Point to alias structure in sb */
} SNVTstruct;

typedef struct
{
    BITS6(mre  , 1,    /* Maximum rate    */
          re   , 1,    /* Rate            */
          nm   , 1,    /* Name            */
          sd   , 1,    /* Self Doc String */
          nc   , 1,    /* Array Count     */
          rsvd , 3)    /* Unused          */
} SNVTextension;

/* Partial list of SNVT type index values */
/* Unused in Ref Impl. Given for illustration */
typedef enum
{
    SNVT_STR_ASC  = 36,
    SNVT_LEV_CONT = 21,
    SNVT_LEV_DISC = 22,
    SNVT_COUNT_F  = 51,
} SNVTType;

typedef struct
{
    int16 nvIndex; /* Base index of array network variables */
    int16 dim;     /* The dimension of the array */
} NVArrayTbl;

/* Type Definition for Protocol Stack Data */
typedef struct
{
    /* Number of bytes used so far */
    uint16  mallocUsedSize;

    /* Array of storage space for dynamic allocation of buffers etc */
    Byte mallocStorage[MALLOC_SIZE];

    /* Variables for Transaction Control Sublayer */
    TransCtrlRecord priTransCtrlRec;

    TransCtrlRecord nonpriTransCtrlRec;

    TransNum       priTransID;
    TransNum       nonpriTransID;

    TIDTableEntry priTbl[TID_TABLE_SIZE];
    TIDTableEntry nonpriTbl[TID_TABLE_SIZE];
    /* # entries currently used */
    uint16        priTblSize;
    uint16        nonpriTblSize;

    /* Timer to delay Transport/Session layers after an external or
       power-up reset. */
    MsTimer tsDelayTimer;

    /* Transmit and Receive Records */
    TransmitRecord xmitRec;
    TransmitRecord priXmitRec;

    ReceiveRecord  *recvRec;  /* Pool of records */
    uint16 recvRecCnt;        /* How many Records allocated? */

    RequestId reqId; /* Running count for request numbers */
    Byte      prevChallenge[8]; /* Used in generation of new challenge. */

    /* Various Queues */
    /**************************************************************
      There are 3 queues associated with Each Layer except Physical.
      1. Input  Queue 2. Output Queue 3. Output Priority Queue

      Each item in a queue has appropriate Param Structure
      followed by appropriate PDU.

      Buffer Sizes and Count Values are available in readOnlyData
      ************************************************************/
    /* Input Queue For App Layer */
    Queue      appInQ;
    uint16     appInBufSize;
    uint16     appInQCnt;

    /* Output Queue For APP Layer */
    Queue      appOutQ;
    uint16     appOutBufSize;
    uint16     appOutQCnt;

    /* Output Priority Queue for APP Layer */
    Queue      appOutPriQ;
    uint16     appOutPriBufSize;
    uint16     appOutPriQCnt;

    /* Input Queue For Transport, Session, Authentication Layers */
    Queue         tsaInQ;
    uint16        tsaInBufSize;
    uint16        tsaInQCnt;

    /* Output Queue For Transport, Session, Authentication Layers */
    Queue         tsaOutQ;
    uint16        tsaOutBufSize;
    uint16        tsaOutQCnt;

    /* Output Priority Queue For Transport, Session, Auth Layers */
    Queue         tsaOutPriQ;
    uint16        tsaOutPriBufSize;
    uint16        tsaOutPriQCnt;

    /* Output Queue for Responses. Just one queue is sufficient.
       Priority or Non-priority is determined based on request */
    Queue         tsaRespQ;
    uint16        tsaRespBufSize;
    uint16        tsaRespQCnt;

    /* Input Queue For network Layer */
    Queue     nwInQ;
    uint16    nwInBufSize;
    uint16    nwInQCnt;

    /* Temporary queue pointers */
    Queue       *nwCurrent;
    Queue       *lkCurrent;

    /* Output Queue For network Layer */
    Queue     nwOutQ;
    uint16    nwOutBufSize;
    uint16    nwOutQCnt;

    /* Output Priority Queue For network Layer */
    /* Buffer size is same as the buffer size for Output Queue */
    Queue     nwOutPriQ;
    uint16    nwOutPriBufSize;
    uint16    nwOutPriQCnt;

    /* Input Queue For Link Layer */
    Byte         *lkInQ;
    uint16        lkInBufSize; /* Size of buffer in lkInPDUQ */
    uint16        lkInQCnt;    /* # of Buffers allocated. */
    Byte         *lkInQHeadPtr;
    Byte         *lkInQTailPtr;

    /* Output Queue For Link Layer */
    Queue         lkOutQ;
    uint16        lkOutBufSize;
    uint16        lkOutQCnt;

    /* Output Priority Queue For Link Layer */
    /* Buffer size is same as the buffer size for Output Queue */
    Queue         lkOutPriQ;
    uint16        lkOutPriBufSize;
    uint16        lkOutPriQCnt;

#ifndef INCLUDE_PHYSICAL
    /* Output Queue For Physical Layer */
    Byte      *phyOutQ; /* Not a regular Queue unlike others */
    uint16     phyOutBufSize;
    uint16     phyOutQCnt;
    Byte      *phyOutQHeadPtr;
    Byte      *phyOutQTailPtr;

    /* Output Priority Queue For Physical Layer */
    /* Buffer size is same as the buffer size for Output Queue */
    Byte      *phyOutPriQ; /* Not a regular Queue unlike others */
    uint16     phyOutPriBufSize;
    uint16     phyOutPriQCnt;
    Byte      *phyOutPriQHeadPtr;
    Byte      *phyOutPriQTailPtr;
#endif

    /* API Flags */
    Boolean msgReceive;    /* TRUE when data in gp->msgIn  */
    Boolean respReceive;   /* TRUE when data in gp->respIn */

    Boolean callMsgFree;   /* Flag to help implicit call to msg_free after DoApp */
    Boolean callRespFree;  /* Flag to help implicit call to resp_free after DoApp */

    /* API Variables */
    RespIn  respIn;
    RespOut respOut;
    MsgIn   msgIn;
    MsgOut  msgOut;

    /* Flag to determine if selected for Net Mgmt queries */
    Boolean selectQueryFlag;

    /* Unbound selector counter for automatic assignement in AddNV. */
    uint16 unboundSelector;

    /* Table to keep track of array network variables and dim */
    NVArrayTbl nvArrayTbl[MAX_NV_ARRAYS];
    uint16     nvArrayTblSize;

    /* Queue of nvIndex for network output variables.
       This queue stores the indices of network variables
       (primary or alias) that are scheduled for sending out
       NVUpdate messages. Each index takes 2 bytes. The index
       is optionally followed by the value of the variable to
       be used with the NVUpdate message. This is done for sync
       network output variables so that we use the value when the
       request was made rather than the current value.

       Update to one network output variable may involve zero or one (primary)
       and zero or more alias indices. Thus we have a collection of indices to be
       scheduled. Alias entries can have different service type or priority
       attributes. To make scheduling work properly, we will have only one
       queue for storing these indices. After scheduling all the alias
       indices, if any, for the primary, we will add an extra entry
       that is equal to NV_UPDATE_LAST_TAG_VALUE to indicate end of
       one scheduling of a primary. When these indices are actually processed,
       we will process one at a time. i.e. we don't send NV message unless
       the previous one is completed.

       An NVUpdate for a primary succeeds if all the transactions for
       scheduled for the NVUpdates succeed. We keep track of this in
       the flag nvOutStatus. The flag nvOutSchedule is set to TRUE if
       scheduling can be done, FALSE otherwise. This is reset to TRUE at
       the completion of the primary and all alias schedules. */

    Queue         nvOutIndexQ;
    uint16        nvOutIndexQCnt;
    uint16        nvOutIndexBufSize;
    Status        nvOutStatus;      /* Used to give NVUpdateCompletes for Propagate. */
    Boolean       nvOutCanSchedule; /* TRUE --> can continue to schedule. */
    int16         nvOutIndex;       /* current primary index scheduled.   */

    /* Queue of nvIndex for network input variables.
       This queue stores the input variables that scheduled
       to be polled. Each item exactly 2 bytes to store the index
       of the variable scheduled. Also, see the comment above for the
       nvOutIndexQ. The comment is valid for this queue for the poll
       messages too.

       Polling one network input variable may involve zero or one (primary)
       and zero or more alias indices. Thus we have a collection of indices to be
       scheduled. The alias can have different service type or priority.

       A poll succeeds if both nvInDataStatus and nvInTranStatus are true. */

    Queue         nvInIndexQ;
    uint16        nvInIndexQCnt;
    Status        nvInDataStatus; /* true if any valid data from external node
                                    is received or nv is turnaround only. */
    Status        nvInTranStatus; /* true if all transactions for the poll succeeded. */
    Boolean       nvInCanSchedule; /* TRUE --> can continue to schedule. */
    int16         nvInIndex;       /* current primary index scheduled.   */

    int16               nvArrayIndex;
    NvInAddr            nvInAddr;

    /* Flag to indicate scheduler whether reset is needed or not */
    Boolean resetNode; /* TRUE ==> reset needed */
    Boolean resetPinPrevState;  /* For PHYIO's use */

    /* To Check if reset was successful */
    Boolean resetOk;

    /* To Check if the manual service request button was pressed or not */
    Boolean manualServiceRequest;
    Boolean manualServiceRequestPrevState;  /* For PHYIO's use */

    /* To Check if generic io input pin 0 is pressed or not */
    Boolean ioInputPin0;
    Boolean ioInputPin0PrevState; /* For PHYIO's use */

    /* To set generic io output pin 0 */
    Boolean ioOutputPin0;

    /* To set generic io output pin 1 */
    Boolean ioOutputPin1;

    /* To Remember prev state of the buttons. For IOChanges's use */
    Boolean prevPinState[1]; /* Currently only one pin is supported */

    /* Represents the mode for the application program in configured state. */
    Byte appPgmMode;  /* Possible Values: OFF_LINE OFF_LINE NOT_RUNNING */

    /* For Msg Tag assignments */
    uint16 nextBindableMsgTag;
    uint16 nextNonbindableMsgTag;

	MsTimer ledTimer;		/* To flash service LED */
    MsTimer checksumTimer;	/* How often to checksum */
} ProtocolStackData;

#pragma pack(push, 1)

typedef struct
{
    ReadOnlyDataStruct  readOnlyData;
    ConfigDataStruct    configData;
    DomainStruct        domainTable[MAX_DOMAINS];
    AddrTableEntry      addrTable[NUM_ADDR_TBL_ENTRIES];
    NVStruct            nvConfigTable[NV_TABLE_SIZE];
    AliasStruct         nvAliasTable[NV_ALIAS_TABLE_SIZE];
    /* Checksum for config structure */
    Byte   configCheckSum; /* Exclusive or of successive bytes in
                             config structure */
    LcsErrorLog    		errorLog;
} EEPROM;

#pragma pack(pop)

typedef struct
{
     /* RAM starts here */
    StatsStruct   stats;
    SNVTstruct    snvt;
    uint8         resetCause;
    NVFixedStruct       nvFixedTable[NV_TABLE_SIZE];
    uint16              nvTableSize; /* Config or Fixed */
} NmMap; /* Memory Map */

/*-------------------------------------------------------------------
Section: Global Variables
-------------------------------------------------------------------*/
/* Node Data Structures */
extern   ProtocolStackData   protocolStackDataGbl[NUM_STACKS];
extern   ProtocolStackData  *gp; /* Pointer to current Structure */
extern   EEPROM              eeprom[NUM_STACKS];
extern   EEPROM             *eep; /* Pointer to current eeprom str */
extern   NmMap               nm[NUM_STACKS];
extern   NmMap              *nmp;

/*-------------------------------------------------------------------
Section: Function Prototypes
-------------------------------------------------------------------*/
DomainStruct *AccessDomain(uint8 indexIn);
Status  UpdateDomain(DomainStruct *domainInp, uint8 indexIn, Boolean includeKey);
AddrTableEntry *AccessAddress(uint16 indexIn);
Status  UpdateAddress(AddrTableEntry *addrEntryInp, uint16 indexIn);
uint16  AddrTableIndex(Byte domainIndexIn, uint8 groupIn);
Boolean IsGroupMember(Byte domainIndex, uint8 groupIn,
                      uint8 *groupMemberOut);
uint16  DecodeBufferSize(uint8 bufSizeIn);
uint16  DecodeBufferCnt(uint8 bufCntIn);
uint16  DecodeRptTimer(uint8 rptTimerIn);
uint16  DecodeRcvTimer(uint8 rcvTimerIn);
uint16  DecodeTxTimer(uint8  txTimerIn, Boolean longTimer);
NVStruct *AccessNV(uint16 indexIn);
void    UpdateNV(NVStruct *nvStructInp, uint16 indexIn);
AliasStruct *AccessAlias(uint16 indexIn);
void    UpdateAlias(AliasStruct *aliasStructInp, uint16 indexIn);
uint16  NVTableIndex(char varNameIn[]);
uint16  AliasTableIndex(char varNameIn[]);
void   *AllocateStorage(uint16 size);
void    NodeReset(Boolean firstReset);
void    InitEEPROM(void);
Boolean IOChanges(uint8 pinNumberIn);
uint8   CheckSum4(void *data, uint16 lengthIn);
uint8   CheckSum8(void *data, uint16 lengthIn);
uint8   ComputeConfigCheckSum(void);
int16   GetPrimaryIndex(int16 nvIndexIn);
NVStruct *GetNVStructPtr(int16 nvIndexIn);
Boolean IsTagBound(uint8 tagin);
Boolean IsNVBound(int16 nvIndexIn);
Boolean AppPgmRuns(void);
Boolean NodeConfigured(void);
Boolean NodeUnConfigured(void);

// APIs that follow the AREA_<Name> convention:
void	LCS_RecordError(LcsErrorLog err);
void	LCS_WriteNvm(void);
EchErr	LCS_ReadNvm(void);

#ifdef _DEBUG_LCS
void    DebugMsg(char debugMsg[]);
void    ErrorMsg(char errMessageIn[]);
#else
#define DebugMsg(x) //
#define ErrorMsg(x) //
#endif

#endif   /* #ifndef _NODE_H */
/*******************************node.h*******************************/
