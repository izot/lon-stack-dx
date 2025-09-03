//
// lcs_node.h
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

/*********************************************************************
  Purpose:        To define the type definitions needed by the upper
                  layers of the LON stack and to define interface
                  functions for some of these data structures.
*********************************************************************/

#ifndef _NODE_H
#define _NODE_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotTypes.h"
#include "common/EchelonStandardDefinitions.h"
#include "isi/isi_int.h"
#include "abstraction/IzotEndian.h"
#include "lcs/lcs_api.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_queue.h"
#include "lcs/lcs_timer.h"

#ifdef LCS_DEBUG
	#define DBG_vPrintf(format,args...)		wmprintf(args)
#else
	#define DBG_vPrintf(format,args...)		;
#endif

#ifdef  __cplusplus
extern "C" {
#endif

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
#define NV_SYNC(i)    0  //Sync datapoints are not supported in DX stack

#define ADDR_INDEX(hi,lo) (hi<<4 | lo)
/*Used to correct the responses in the NmQuerySiData() function*/
#define OFFSET_OF_SI_DATA_BUFFER_IN_SNVT_STRUCT 6

/*-------------------------------------------------------------------
  Section: Type Definitions
  -------------------------------------------------------------------*/

/* Turn off alignment by compiler to make sure that the structure sizes
   are as we intend them to be with no padding */

#pragma pack(push, 1)

typedef struct __attribute__((__packed__))
{
    IzotByte  domainId[DOMAIN_ID_LEN];
    IzotByte  subnet;
    BITS2(cloneDomain , 1,
          node        , 7)
    BITS4(invalid	  , 1,
	      unused      , 2,
		  authType    , 2,			// 1 => OMA (0 and 3 mean standard - the 3 is for backward compatibility with tools that wrote all ones to the domain length byte)
		  len         , 3)
} LogicalAddress;

#define LsProtocolModeLegacy		 				0		/*  */
#define LsProtocolModeEncapsulatedIp				1		/*  */
#define LsProtocolModeEnhanced						2		/*  */

// authType literals
#define AUTH_STD		0
#define AUTH_OMA		1
#define AUTH_STD_OLD	3

typedef struct __attribute__((__packed__))
{
    BITS4(collisionDetect    ,1,
          bitSyncThreshHold  ,2, /* Encoded. See page 9-28 */
          filter             ,2,
          hysteresis         ,3)
    BITS3(unUsed             ,6,
          cdTail             ,1,
          cdPreamble         ,1)
} DirectParamStruct;

typedef struct __attribute__((__packed__))
{
    BITS2(      commClock    ,5,  /* bit rate ratio    */
                inputClock   ,3)  /* input clk osc freq     */
    BITS2(      commType     ,3,        /* type of receiver       */
                commPinDir   ,5)        /* dir comm port pins     */
    IzotByte        reserved[5];
    IzotByte        nodePriority;           /* priority slot used     */
    IzotByte        channelPriorities;      /* # of priority slots    */
    union {
        IzotByte               xcvrParams[IZOT_COMMUNICATIONS_PARAMETER_LENGTH];
        DirectParamStruct  dirParams;
    } param;
} CommParams;

/* Network Variable Tables */
typedef struct __attribute__((__packed__))
{
    BITS3(nvPriority     ,1,  /* NV uses priority messaging       */
          nvDirection    ,1,  /* 1 => output var                  */
          nvSelectorHi   ,6)  /* Hi & Lo form NV selector         */
    IzotByte  nvSelectorLo     ;  /* Range 0-0x3FFF                   */
    BITS4(nvTurnaround   ,1,  /* 1 ==> bound to a nv in this node */
          nvService      ,2,  /* IzotServiceAcknowledged or UNACKD_RPT or IzotServiceUnacknowledged     */
          nvAuth         ,1,  /* 1 => NV used auth transactions   */
          nvAddrIndex    ,4)  /* index into addr table. 15 is spcl */
} NVStruct;

typedef struct __attribute__((__packed__))
{
    IzotUbits16 node_sd_text_length;
    IzotUbits16 static_nv_count; // static NV count 
} SIHeaderExt;

#define NUM_EXTCAP_BYTES 6
typedef struct __attribute__((__packed__))
{
    IzotUbits16  length;        /* length of structure, including the length field. */
    IzotByte     ver_struct;    /* version number of structure.  Current version is 1 */
    IzotByte     ver_nm_min;    /* 0 = no ext cmd; 1 = ext cmd set.  Minimum nm version. */
    IzotByte     ver_nm_max;    /* 0 = no ext cmd; 1 = ext cmd set*/
    IzotByte     ver_binding;   /* 0/1 = see binding_ii; 2, or 3 */
    IzotByte     ext_cap_flags[NUM_EXTCAP_BYTES];   /* using EXT_CAP_XXX */
    IzotUbits16  domain_capacity;  /* maximum number of domain entries */
    IzotUbits16  address_capacity; /* maximum number of address table entries available using
                                       tradition methods or ECS.  On non ECS devices, this should
                                       be a maximum of 15. Note that non-ECS devices that support
                                       EAT may actually support more address table entries,
                                       as defined by the eat_address_capacity table below. */
    IzotUbits16  static_mtag_capacity; /* maximum number of static message tags */
    IzotUbits16  mcnv_capacity;        /* maximum number of monitor/control NVs */
    IzotUbits16  mcp_capacity;         /* maximum number of monitor/control points */
    IzotUbits16  mcs_capacity;         /* maximum number of monitor/control sets */
    IzotUbits16  max_mc_desc_length;   /* maximum size of each monitor description data */
    IzotUbits16  mcnv_current_count;   /* current number of monitor/control NVs defined */
    IzotUbits16  mcnv_max_index;       /* highest monitor/control NV index defined */

        // The following fields or optional and were added 8/11/03 with XIF 4.401
    IzotUbits16  dyn_fb_capacity;      /* Dynamic function block capacity. */

        /* The following fields are optional and were added 9/30/10 with XIF 4.402 */
    IzotByte    eat_address_capacity;  /* EAT (extended) address capacity, 0-255.
                                       Number of address table entries available using EAT
                                       network management.  On non-EAT devices, this should
                                       be the minimum of the total address capacity and 15. */
} SNVTCapabilityInfo;

typedef struct __attribute__((__packed__))
{
    NVStruct nvConfig;
    IzotByte    primary;   /* index into NV Cfg tbl. 0xFF => next fld */
    IzotUbits16   hostPrimary; /* NV cfg tbl index. For host nodes */
} AliasStruct;

typedef struct __attribute__((__packed__))
{
	IzotByte nvLength;		// sync field removed!! However it is not supported by IZOT-DX
    void *nvAddress;   /* Ptr to variable's data */
} NVFixedStruct;

typedef struct
{
  	IzotByte  stats[LcsNumStats*2];
    IzotByte  eepromLock;
 } StatsStruct;

#pragma pack(pop)

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
// Downlink/uplink alt path flags
#define ALT_PATH		0x01	// The traditional alt path bit - sent/received on the alternate carrier frequency (AKA secondary carrier)
#define ALT_RETRY		0x02	// This is a retry - downlink only
#define ALT_CHANNEL		0x04	// Send/receive on the alternate chanenel if an alternate channel is available.
#define ALT_CHANNEL_LOCK 0x08	// 1 => Transmission is locked to the primary/alternate channel based on previous bit.
typedef IzotByte AltPathFlags;

typedef struct __attribute__((__packed__))
{
    DestinationAddress	destAddr; /* To give dest addr  */
    PDUType				pduType;  /* APDU, SPDU, TPDU, AuthPDU */
    MsgTag				tag;      /* used only for APDU */
    IzotByte			deltaBL;  /* Upper Layers supply this */
    AltPathFlags		altPath;	/* See alt path flags above */
    IzotUbits16			pduSize;  /* Size of the PDU sent */
    IzotByte			dropIfUnconfigured; /* drop the packet if the
                           node is unconfigured. */
	IzotByte			proxy;	/* Message is a proxy message */
	BITS2(unused		,6,
          version		,2)			  /* version */
} NWSendParam;

/*-------------------------------------------------------------------
  NWReceiveParam: Is used by Link Layer when it supplies an incoming
  NPDU to the network layer. This data structure contains information
  regarding the incoming NPDU.
*******************************************************************/
typedef struct
{
    IzotByte		priority;   /* Was it a priority message? */
	AltPathFlags	altPath;    /* See alt path flags above */
	IzotUbits16		pduSize;
} NWReceiveParam;

/* Type Definitions for Application Layer */
typedef struct __attribute__((__packed__))
{
    DestinType code;
    IzotByte data[MAX_DATA_SIZE];
} APDU;

typedef APDU *APDUPtr;

/* APPSendParam is used by API to give info to app layer
   regarding APDU that is going out. Most info is available
   in msg_out in the Application Out Buffers */

/* APPSendParam has everything in MsgOut except code and data. */
typedef struct
{
    MsgTag			tag;           /* to correlate completion codes  */
    IzotUbits16		len;           /* message length in app data     */
    IzotByte		authenticated; /* TRUE if to be authenticated    */
    IzotServiceType	service;       /* service type used to send msg  */
    RequestId		reqId;         /* Request ID for responses */
    IzotSendAddress	addr;          /* destination address (see above)*/
    IzotByte		nullResponse;  /* For responses                  */
} APPSendParam;

/* Types of messages that are received by application layer */
typedef enum __attribute__((__packed__)){
    MESSAGE    = 0,
    COMPLETION = 1,  /* Indication by Transport/Session layers */
} APIndType;

// Define flags used to control packet transmission
#define PKT_PRIORITY		0x0001
#define PKT_ALTPATH			0x0002
#define PKT_PROXY			0x0004
typedef IzotByte PktCtrl;

/* APPReceiveParam is used by Application Layer to figure out
   information regarding incoming APDU. */
typedef struct __attribute__((__packed__))
{
    APIndType			indication; /* Type of APDU received */
    IzotByte			success;    /* Used if indication ==
                                        COMPLETION */
    MsgTag				tag;        /* Tag for indication
                                        or for matching response */
    SourceAddress		srcAddr;    /* Who sent it?            */
    IzotServiceType		service;    /* Which service type?     */
    IzotByte			priority;   /* Was it a priority msg?  */
    IzotByte			altPath;    /* Was it sent in alt path? */
    IzotUbits16			pduSize;    /* Size of incoming APDU   */
    IzotUbits16			taIndex;    /* Turnaround var Index    */
    IzotByte			auth;       /* Was it authenticated?   */
    RequestId			reqId;      /* Assigned by session to match
                                        responses later         */
	IzotByte			proxy;	  // 1=>Proxy
	IzotByte			proxyDone;  // 1=>Proxy transaction completed
	IzotByte			proxyCount; // Original proxy hop count.
	XcvrParam			xcvrParams; // Transceiver parameters
} APPReceiveParam;

/* Type Definition for Transaction Control SubLayer */
typedef struct
{
    TransNum	transNum;       /* initial value = 0. Range 0..15 */
    IzotByte	inProgress;     /* Is the transaction in progress */
} TransCtrlRecord;

/* Type definition for table used while assigning new TId. */
/* For more information, see tcs.h or tcs.c */
typedef struct
{
    IzotByte	domainId[DOMAIN_ID_LEN];
    IzotByte	len; /* domain length */
    AddrMode	addressMode;
    union
    {
        IzotReceiveSubnetNode subnetNode;
        IzotReceiveGroup      group;   /* group number of multicast */
        IzotReceiveBroadcast  subnet;  /* 0 if domainwide broadcast */
        IzotByte              uniqueNodeId[UNIQUE_NODE_ID_LEN];
    } addr;
    LonTimer                  timer;
    TransNum                  tid;     /* Last TID used for this addr */
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

typedef enum  __attribute__((__packed__))
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
	IzotByte           altKey;	// TRUE => use alternate authentication key
	IzotByte           altKeyValue[2][DOMAIN_ID_LEN]; // Key used when altKey is true
} AltKey;

typedef struct
{
    TXStatus           status;               /* Who owns it? if not free  */
    DestinationAddress nwDestAddr;           /* Destination Address */
    IzotByte           ackReceived[MAX_GROUP_NUMBER+1];
    /* Array[0..MAX_GROUP_NUMBER] of IzotByte   */
    IzotByte           destCount;            /* Number of destinations    */
    IzotByte           ackCount;             /* Or respCount              */
    TransNum           transNum;
    IzotUbits16        xmitTimerValue;
    LonTimer            xmitTimer;            /* Transmit Timer            */
    IzotByte           retriesLeft;          /* How many left?            */
    APDU              *apdu;                 /* APDU transmitted          */
    IzotUbits16        apduSize;             /* Size of APDU              */
    IzotByte           auth;                 /* Does this msg need auth?  */
	IzotUbits16        txTimerDeltaLast;	 // Time to add to last retry timer
	AltKey			   altKey;				 // Alternate authentication info.
	BITS2(unused	  , 6,
          version     , 2)			  /* version */
} TransmitRecord;

typedef struct  __attribute__((__packed__))
{
    RRStatus             status;         /* used? Who is using?    */
    SourceAddress        srcAddr;        /* Who sent this?         */
    TransNum             transNum;
    RequestId            reqId;          /* For matching response  */
    LonTimer              recvTimer;      /* receive timer          */
    TransactionState     transState;     /* What state is it in    */
    IzotByte              priority;
    IzotByte              altPath;        /* Was alt path used?     */
    IzotByte              auth;           /* TRUE if auth succeeded */
    IzotByte              needAuth;       /* Need authentication    */
    IzotServiceType      serviceType;    /* What type of service   */
    IzotByte             rand[8];        /* For authentication     */
    APDU                *response;       /* Store the response     */
    IzotUbits16               rspSize;
    APDU                *apdu;          /* Store the APDU received */
    IzotUbits16               apduSize;
	XcvrParam			 xcvrParams;
	BITS2(unused		,6,
          version     , 2)			  /* version */
} ReceiveRecord;

/********************************************************************
   TSASendParam is used to provide the necessary information
   needed to process the transaction.
   IzotSendAddress has all the necessary information for forming the
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
typedef struct __attribute__((__packed__))
{
    IzotSendAddress		destAddr;  /* Whom to send? Need Timers too */
	Domain				dmn;
    IzotServiceType		service;   /* What type of service? */
    IzotByte			auth;      /* Need Authentication?  */
    IzotUbits16			apduSize;  /* Size of APDU to be sent */
    MsgTag				tag;       /* For service indication  */
    IzotByte			nullResponse; /* TRUE => no resp goes out */
	IzotByte			flexResponse; /* TRUE => send response on flex domain */
    RequestId			reqId;     /* Set if service=IzotServiceResponse */
    IzotByte			altPathOverride;
    IzotByte			altPath; /* Used only if altPathOverride is true */
	IzotByte			priority;  // TRUE => priority
	IzotByte			proxy;		// TRUE => proxy
	IzotByte			proxyDone; // 1=>Proxy transaction completed
	IzotByte			txInherit; // 1=>Inherit TX# from proxy source.  Only valid for proxy
	IzotByte			proxyCount;// Original proxy hop count.
	IzotUbits16			txTimerDeltaLast; // Amount to add to the last retry timer.  Only valid for proxy
	AltKey				altKey;	// Alternate authentication key info
} TSASendParam;

/********************************************************************
   TSAReceiveParam is used to receive the necessary
   information to process incoming PDU (TPDU or SPDU or AuthPDU).
   Receive Timer is set based on info in srcAddr.
   srcAddr has the source subnet/node, domainIndex used, group if
   any etc.
********************************************************************/
typedef struct __attribute__((__packed__))
{
    SourceAddress	srcAddr;    /* Who send it? */
    IzotByte		priority;   /* Was it a priority msg?  */
    IzotUbits16		pduSize;    /* Size of incoming PDU */
    PDUType			pduType;    /* What type of PDU? */
    IzotByte		altPath;    /* Was it sent in alt path? */
	XcvrParam		xcvrParams; /* Transceiver Parameters */
	BITS2(unused	,6,
          version	,2)			  /* version */
} TSAReceiveParam;

typedef struct
{
	IzotByte		deltaBL;  /* What is the backlog generated by this msg? */
	AltPathFlags	altPath;  /* Should altPath be used or is this a retry? */
    IzotUbits16		pduSize;  /* Size of NPDU */
	IzotByte		DomainIndex; /* Channels sent on */
} LKSendParam;

/* SNVT data structures */

/* See comment in app.c in APPReset and AddNV for a description */
/* of how the SNVT area is layed out and managed.               */

typedef struct __attribute__((__packed__))
{
    BITS8(extRec            , 1,
          nvSync            , 1,
          nvPolled          , 1,
          nvOffline         , 1,
          nvServiceConfig   , 1,
          nvPriorityConfig  , 1,
          nvAuthConfig      , 1,
          nvConfigClass     , 1)
    IzotByte snvtTypeIndex;
} SNVTdescStruct;

typedef struct __attribute__((__packed__))
{
    BITS3(        bindingII    , 1,
                  queryStats   , 1,
                  aliasCount   , 6)
    IzotUbits16   hostAlias;     // Warning - stored in host order so not for internal consumption
} AliasField;

typedef struct
{
    IzotUbits16		length;          // Warning - stored in host order so not for internal consumption
    IzotByte		numNetvars;
    IzotByte		version;
    IzotByte		msbNumNetvars;
    IzotByte		mtagCount;
    char            *sb;
    SNVTdescStruct  *descPtr; /* Point to next SNVTdesc entry in sb */
    /* Actually, it points Node Self-Doc
       String. Just before that is the
       self-id info for the last netwar */
    AliasField      *aliasPtr; /* Point to alias structure in sb */
} SNVTstruct;

typedef struct __attribute__((__packed__))
{
    BITS6(mre  , 1,    /* Maximum rate    */
          re   , 1,    /* Rate            */
          nm   , 1,    /* Name            */
          sd   , 1,    /* Self Doc String */
          nc   , 1,    /* Array Count     */
          rsvd , 3)    /* Unused          */
} SNVTextension;

typedef struct
{
    IzotBits16 nvIndex; /* Base index of array network variables */
    IzotBits16 dim;     /* The dimension of the array */
} NVArrayTbl;

typedef struct
{
	IzotBool8   downloading;	// true => doing a download
	IzotBool8   switchoverFailure;// true => switchover failed
	IzotBool8	wrapPending;	// true => a wrap is about to occur.
	uint32_t    imageOffset;	// Current 64K base offset being written to
} DownloadState;

#define CP_WRITE 0x01
#define CP_RESET 0x02
typedef IzotByte CpWrite;

typedef struct
{
	DownloadState downloadState;	// Used for tracking download state - kept persistent
	CommParams 	  ftxlCps;			// Used for tracking application comm parameters
	CpWrite		  cpWrite;
} NvmMisc;

/* Type definition for LON DX Stack data */
typedef struct
{
	/* Track if this stack is initialized */
	IzotBool        initialized;

    /* Number of bytes used so far */
    IzotUbits16     mallocUsedSize;

    /* Array of storage space for dynamic allocation of buffers etc */
    IzotByte        mallocStorage[MALLOC_SIZE];

    /* Variables for Transaction Control Sublayer */
    TransCtrlRecord priTransCtrlRec;

    TransCtrlRecord nonpriTransCtrlRec;

    TransNum        priTransID;
    TransNum        nonpriTransID;

    TIDTableEntry   priTbl[TID_TABLE_SIZE];
    TIDTableEntry   nonpriTbl[TID_TABLE_SIZE];
    /* # entries currently used */
    IzotUbits16     priTblSize;
    IzotUbits16     nonpriTblSize;

    /* Timer to delay Transport/Session layers after an external or
       power-up reset. */
    LonTimer        tsDelayTimer;

    /* Transmit and Receive Records */
    TransmitRecord  xmitRec;
    TransmitRecord  priXmitRec;

    ReceiveRecord  *recvRec;  /* Pool of records */
    IzotUbits16     recvRecCnt;        /* How many Records allocated? */

    RequestId       reqId; /* Running count for request numbers */
    IzotByte        prevChallenge[8]; /* Used in generation of new challenge. */

    /* Various Queues */
    /**************************************************************
      There are 3 queues associated with Each Layer except Physical.
      1. Input  Queue 2. Output Queue 3. Output Priority Queue

      Each item in a queue has appropriate Param Structure
      followed by appropriate PDU.

      Buffer Sizes and Count Values are available in readOnlyData
      ************************************************************/
    /* Input Queues For App Layer */
    Queue           appInQ;
	Queue	        appCeRspInQ;	// Completion Event and Response queue
    IzotUbits16     appInBufSize;
    IzotUbits16     appInQCnt;

    /* Output Queue For APP Layer */
    Queue           appOutQ;
    IzotUbits16     appOutBufSize;
    IzotUbits16     appOutQCnt;

    /* Output Priority Queue for APP Layer */
    Queue           appOutPriQ;
    IzotUbits16     appOutPriBufSize;
    IzotUbits16     appOutPriQCnt;

    /* Input Queue For Transport, Session, Authentication Layers */
    Queue           tsaInQ;
    IzotUbits16     tsaInBufSize;
    IzotUbits16     tsaInQCnt;

    /* Output Queue For Transport, Session, Authentication Layers */
    Queue           tsaOutQ;
    IzotUbits16     tsaOutBufSize;
    IzotUbits16     tsaOutQCnt;

    /* Output Priority Queue For Transport, Session, Auth Layers */
    Queue           tsaOutPriQ;
    IzotUbits16     tsaOutPriBufSize;
    IzotUbits16     tsaOutPriQCnt;

    /* Output Queue for Responses. Just one queue is sufficient.
       Priority or Non-priority is determined based on request */
    Queue           tsaRespQ;
    IzotUbits16     tsaRespBufSize;
    IzotUbits16     tsaRespQCnt;

    /* Input Queue For network Layer */
    Queue           nwInQ;
    IzotUbits16     nwInBufSize;
    IzotUbits16     nwInQCnt;

    /* Temporary queue pointers */
    Queue          *nwCurrent;
    Queue          *lkCurrent;

    /* Output Queue For network Layer */
    Queue           nwOutQ;
    IzotUbits16     nwOutBufSize;
    IzotUbits16     nwOutQCnt;

    /* Output Priority Queue For network Layer */
    /* Buffer size is same as the buffer size for Output Queue */
    Queue           nwOutPriQ;
    IzotUbits16     nwOutPriBufSize;
    IzotUbits16     nwOutPriQCnt;
#if LINK_IS(MIP)
    /* Input Queue For Link Layer */
    IzotByte       *lkInQ;
    IzotUbits16     lkInBufSize; /* Size of buffer in lkInPDUQ */
    IzotUbits16     lkInQCnt;    /* # of Buffers allocated. */
    IzotByte       *lkInQHeadPtr;
    IzotByte       *lkInQTailPtr;
#endif // LINK_IS(MIP)
    /* Output Queue For Link Layer */
    Queue           lkOutQ;
    IzotUbits16     lkOutBufSize;
    IzotUbits16     lkOutQCnt;

    /* Output Priority Queue For Link Layer */
    /* Buffer size is same as the buffer size for Output Queue */
    Queue           lkOutPriQ;
    IzotUbits16     lkOutPriBufSize;
    IzotUbits16     lkOutPriQCnt;

#if LINK_IS(MIP)
    /* Output Queue For Physical Layer */
    IzotByte       *phyOutQ; /* Not a regular Queue unlike others */
    IzotUbits16     phyOutBufSize;
    IzotUbits16     phyOutQCnt;
    IzotByte       *phyOutQHeadPtr;
    IzotByte       *phyOutQTailPtr;

    /* Output Priority Queue For Physical Layer */
    /* Buffer size is same as the buffer size for Output Queue */
    IzotByte       *phyOutPriQ; /* Not a regular Queue unlike others */
    IzotUbits16     phyOutPriBufSize;
    IzotUbits16     phyOutPriQCnt;
    IzotByte       *phyOutPriQHeadPtr;
    IzotByte       *phyOutPriQTailPtr;
#endif // LINK_IS(MIP)

    /* API Flags */
    IzotByte msgReceive;    /* TRUE when data in gp->msgIn  */
    IzotByte respReceive;   /* TRUE when data in gp->respIn */

    IzotByte callMsgFree;   /* Flag to help implicit call to msg_free after DoApp */
    IzotByte callRespFree;  /* Flag to help implicit call to resp_free after DoApp */

    /* API Variables */
    RespIn  respIn;
    RespOut respOut;
    MsgIn   msgIn;
    MsgOut  msgOut;

    /* Flag to determine if selected for Net Mgmt queries */
    IzotByte selectQueryFlag;

    /* Unbound selector counter for automatic assignement in AddNV. */
    IzotUbits16 unboundSelector;

    /* Table to keep track of array network variables and dim */
    NVArrayTbl     nvArrayTbl[MAX_NV_ARRAYS];
    IzotUbits16    nvArrayTblSize;

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

    Queue          nvOutIndexQ;
    IzotUbits16    nvOutIndexQCnt;
    IzotUbits16    nvOutIndexBufSize;
    Status         nvOutStatus;      /* Used to give NVUpdateCompletes for Propagate. */
    IzotByte       nvOutCanSchedule; /* TRUE --> can continue to schedule. */
    IzotBits16     nvOutIndex;       /* current primary index scheduled.   */

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

    Queue          nvInIndexQ;
    IzotUbits16    nvInIndexQCnt;
    Status         nvInDataStatus; /* true if any valid data from external node
                                    is received or nv is turnaround only. */
    Status         nvInTranStatus; /* true if all transactions for the poll succeeded. */
    IzotByte       nvInCanSchedule; /* TRUE --> can continue to schedule. */
    IzotBits16     nvInIndex;       /* current primary index scheduled.   */

    IzotBits16     nvArrayIndex;
    IzotReceiveAddress	nvInAddr;

    /* Flag to indicate scheduler whether reset is needed or not */
    IzotByte resetNode; /* TRUE ==> reset needed */

    /* To Check if reset was successful */
    IzotByte resetOk;

    /* To Check if the manual service request button was pressed or not */
    IzotByte manualServiceRequest;

    IzotByte serviceLedPhysical;
    IzotByte preServiceLedPhysical;
    
    IzotByte serviceLedState;
    IzotByte prevServiceLedState;

    /* Represents the mode for the application program in configured state. */
    IzotByte appPgmMode;  /* Possible Values: OFF_LINE OFF_LINE NOT_RUNNING */

    /* For Msg Tag assignments */
    IzotUbits16 nextBindableMsgTag;
    IzotUbits16 nextNonbindableMsgTag;

	LonTimer ledTimer;		/* To flash service LED */
    LonTimer checksumTimer;	/* How often to checksum */

	LonTimer proxyBufferWait;

//	ErrorSim errorSim[2][2];		// One for each channel (primary, alternate) and frequency (0 primary, 1 secondary)

//	LcsBlockingMode blockingMode;

	NvmMisc nvm;					// NVM miscellaneous

	void (*clearStatsCallback)(void);
} ProtocolStackData;

#pragma pack(push, 1)

typedef struct
{
	IzotByte            domain;
	IzotByte            address;
	IzotByte            nv;
	IzotByte            alias;
} Dimensions;

typedef struct
{
    IzotReadOnlyData    readOnlyData;
    IzotConfigData      configData;
    IzotDomain          domainTable[MAX_DOMAINS];
    IzotAddress         addrTable[NUM_ADDR_TBL_ENTRIES];
    IzotDatapointConfig nvConfigTable[NV_TABLE_SIZE];
    IzotAliasConfig     nvAliasTable[NV_ALIAS_TABLE_SIZE];
    /* Checksum for config structure */
    IzotByte            configCheckSum; /* Exclusive or of successive bytes in
                             config structure */
    IzotSystemError     errorLog;
	Dimensions          dimensions;
	IzotByte            nvInitCount;
	IzotByte            nodeState;
	uint32_t            signature;
} EEPROM;

#pragma pack(pop)


typedef struct
{
	IzotUbits16 rx[2][2][NUM_RX_TYPES];	// Alt path is first dimension, solicited/unsolicited is second
} RxStats;

typedef struct
{
     /* RAM starts here */
    StatsStruct         stats;
    SNVTstruct          snvt;
    IzotByte            resetCause;
    NVFixedStruct       nvFixedTable[NV_TABLE_SIZE];
    IzotUbits16         nvTableSize; /* Config or Fixed */
	RxStats             rxStat;
} NmMap; /* Memory Map */

#define IZOT_DATAPOINT_PERSIST_MASK  	    0x01    /* Added for to store persistent flag */
#define IZOT_DATAPOINT_PERSIST_SHIFT 	    0
#define IZOT_DATAPOINT_PERSIST_FIELD 	    Attribute

#define IZOT_DATAPOINT_CHANGEABLE_TYPE_MASK  0x02    /* Added for to store changeble type flag */
#define IZOT_DATAPOINT_CHANGEABLE_TYPE_SHIFT 1
#define IZOT_DATAPOINT_CHANGEABLE_TYPE_FIELD Attribute
typedef struct __attribute__ ((packed))
{
    const IzotByte     *ibolSeq;
    IzotByte            Attribute;
} IzotDpProperty;
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
extern   SNVTCapabilityInfo  capability_info;
extern   SNVTCapabilityInfo *snvt_capability_info;
extern   SIHeaderExt         header_ext;
extern   SIHeaderExt        *si_header_ext;
extern   IzotDpProperty      izot_dp_prop[NV_TABLE_SIZE];

/*-------------------------------------------------------------------
  Section: Function Prototypes
  -------------------------------------------------------------------*/
IzotDomain  *AccessDomain(IzotByte indexIn);
Status   UpdateDomain(const IzotDomain *domainInp, IzotByte indexIn, IzotByte includeKey);
IzotAddress *AccessAddress(IzotUbits16 indexIn);
Status   UpdateAddress(const IzotAddress *addrEntryInp, IzotUbits16 indexIn);
IzotUbits16  AddrTableIndex(IzotByte domainIndexIn, IzotByte groupIn);
IzotByte IsGroupMember(IzotByte domainIndex, IzotByte groupIn,
                      IzotByte *groupMemberOut);
IzotUbits16  DecodeBufferSize(IzotByte bufSizeIn);
IzotUbits16  DecodeBufferCnt(IzotByte bufCntIn);
IzotUbits16  DecodeRptTimer(IzotByte rptTimerIn);
IzotUbits16  DecodeRcvTimer(IzotByte rcvTimerIn);
IzotUbits16  DecodeTxTimer(IzotByte  txTimerIn);
// Deleted from DecodeTxTimer: "", IzotByte longTimer"

IzotDatapointConfig *AccessNV(IzotUbits16 indexIn);
void     UpdateNV(IzotDatapointConfig *nvStructInp, IzotUbits16 indexIn);
IzotAliasConfig *AccessAlias(IzotUbits16 indexIn);
void     UpdateAlias(IzotAliasConfig *aliasStructInp, IzotUbits16 indexIn);
IzotUbits16 AliasTableIndex(char varNameIn[]);
void    *AllocateStorage(IzotUbits16 size);
void     NodeReset(IzotByte firstReset);
void	 NodeReset_wrapper(void);
Status   InitEEPROM(uint32_t signature);
IzotByte CheckSum8(void *data, IzotUbits16 lengthIn);
IzotByte ComputeConfigCheckSum(void);
IzotBits16 GetPrimaryIndex(IzotBits16 nvIndexIn);
IzotDatapointConfig *GetNVStructPtr(IzotBits16 nvIndexIn);
IzotByte IsTagBound(IzotByte tagin);
IzotByte IsNVBound(IzotBits16 nvIndexIn);
IzotBool AppPgmRuns(void);
void     MsgCompletes(Status status, MsgTag tag);
IzotByte NodeConfigured(void);		    // Node is honoring its configuration
IzotByte NodeUnConfigured(void);		// Node is not running and not honoring configuration (not necessarily the same as !NodeConfigured())
void	 LCS_LogRxStat(AltPathFlags altPath, RxStatType type);
void	 NM_Init(void);
IzotBool IsPhysicalResetRequested(void);
void     PhysicalResetRequested(void);

// Workaround EchErr definition problem
#ifndef ECHERR_DEFINED
#define ECHERR_DEFINED
typedef uint16_t EchErr;
#endif

// APIs that follow the AREA_<Name> convention:
void	 LCS_RecordError(IzotSystemError err);
void	 LCS_WriteNvm(void);
EchErr	 LCS_ReadNvm(void);
void     LCS_WriteNvs(void);
EchErr   LCS_ReadNvs(void);
void	 LCS_InitAddress(void);
void 	 LCS_InitAlias(void);
IzotByte izot_get_device_state(void);
IzotByte izot_get_service_pin_mode(void);
uint8_t	 izot_get_device_mode(void);

extern IzotApiError IzotGetUniqueId(IzotUniqueId* const pId);
extern void IzotMsgArrived(const IzotReceiveAddress* const pAddress,
                   const IzotCorrelator correlator,
                   const IzotBool priority,
                   const IzotServiceType serviceType,
                   const IzotBool authenticated,
                   const IzotByte code,
                   const IzotByte* const pData, const unsigned dataLength);
extern void IzotResponseArrived(const IzotResponseAddress* const pAddress,
                        const unsigned tag,
                        const IzotByte code,
                        const IzotByte* const pData, const unsigned dataLength);
extern IzotBool IzotFilterMsgArrived(const IzotReceiveAddress* const pAddress, const IzotCorrelator correlator,
									const IzotBool priority, const IzotServiceType serviceType, const IzotBool authenticated,
									const IzotByte code, const IzotByte* const pData, const unsigned dataLength);
extern IzotBool IzotFilterResponseArrived(const IzotResponseAddress* const pAddress, const unsigned tag,
								const IzotByte code, const IzotByte* const pData, const unsigned dataLength);									

#if DEBUG_LCS
void    DebugMsg(char debugMsg[]);
void    ErrorMsg(char errMessageIn[]);
#else
#define DebugMsg(x)
#define ErrorMsg(x)
#endif

#ifdef  __cplusplus
}
#endif

#endif   /* #ifndef _NODE_H */
/*******************************node.h*******************************/
