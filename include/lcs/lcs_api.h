/*
 * lcs_api.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Application Layer API
 * Purpose: Implements the LON application layer (Layer 7) API.
 * Notes:   See ISO/IEC 14908-1, Section 10 for LON protocol details.
 *          See Section 10.6 for Application Protocol State Variables.
 * 
 *          The functions in lcs_api.c are called by the application program
 *          to perform various operations related to LON messaging and network
 *          variables. 
 *
 *          lcs_api.c implements the API functions used by the application
 *          program to interact with the LON stack application layer to send
 *          and receive network variable updates and LON messages. Some of
 *          the main functions are:
 *          MsgAlloc():     Allocates a message buffer for the application
 *                          program to create a message to be sent out.
 *          MsgSend():      Sends the message created by the application
 *                          program using the MsgAlloc() function.
 *          MsgReceive():   Receives messages destined to the application
 *                          program.
 *          RespAlloc():    Allocates a response buffer for the application
 *                          program to create a response to be sent out.
 *          RespSend():     Sends the response created by the application
 *                          program using the RespAlloc() function.
 *          RespReceive():  Receives responses destined to the application
 *                          program.
 *          AddNV():        Adds a network variable to the application
 *                          program with the given properties.
 *          Propagate():    Sends all output network variables in the device.
 *          PropagateNV():  Sends a specific output network variable.
 *          Poll():         Polls all input network variables in the device.
 *          PollNV():       Polls a specific input network variable.
 *          GoOffline():    Puts the application offline.
 *          GoUnconfigured(): Puts the application unconfigured.
 *          NewMsgTag():    Returns a new message tag for the application program
 *                          to use when sending messages.
 *          ManualServiceRequestMessage(): Sends a manual service request message.
 */

#ifndef _LCS_API_H
#define _LCS_API_H

#include "lcs/lcs_custom.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

typedef union __attribute__ ((packed))
{
    struct __attribute__ ((packed))
    {
        BITS2(apFlag,       2,
              apCode,     6)
    } ap;
    struct __attribute__ ((packed))
    {
        BITS3(nvFlag,       1,
              nvDir,      1,
              nvCode,     6)
    } nv;
    struct __attribute__ ((packed))
    {
        BITS2(nmFlag,       3,
              nmCode,     5)
    } nm;
    struct __attribute__ ((packed))
    {
        BITS2(ndFlag,       4,
              ndCode,     4)
    } nd;
    struct __attribute__ ((packed))
    {
        BITS2(ffFlag,       4,
              ffCode,     4)
    } ff;
    IzotByte allBits;
} DestinType;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

/* Message Declarations ****************************************** */

typedef struct
{
    IzotByte			code; /* message code                      */
    IzotUbits16			len;  /* length of message data            */
    IzotByte			data[MAX_DATA_SIZE];  /* message data      */
    IzotByte			authenticated; /* TRUE if msg was authenticated */
    IzotServiceType		service;       /* Service used to send the msg  */
    RequestId			reqId;         /* Request ID to match responses   */
    IzotReceiveAddress	addr;
} MsgIn;

typedef struct
{
    IzotByte			priorityOn;    /* TRUE if a priority message     */
    MsgTag				tag;           /* to correlate completion codes  */
    IzotUbits16			len;           /* message length in data array below */
    IzotByte			code;          /* message code                   */
    IzotByte			data[MAX_DATA_SIZE];  /* message data            */
    IzotByte			authenticated; /* TRUE if to be authenticated    */
    IzotServiceType		service;       /* service type used to send msg  */
    IzotSendAddress		addr;          /* destination address (see above)*/
} MsgOut;

typedef struct
{
    MsgTag               tag;                   /* To match req    */
    IzotByte             code;                  /* message code    */
    IzotUbits16          len;                   /* message length  */
    IzotByte             data[MAX_DATA_SIZE];   /* message data    */
    IzotResponseAddress  addr;  /* destination address (see above) */
} RespIn;             /* struct for receiving responses  */

typedef struct
{
    RequestId	reqId;         /* Request ID to match responses */
    IzotByte	nullResponse;  /* TRUE => no resp goes out */
    IzotByte	code;          /* message code */
    IzotUbits16	len;           /* message length  */
    IzotByte	data[MAX_DATA_SIZE];   /* message data */
} RespOut;                   /* structure for sending responses */


/*******************************************************************
NVDefinition is used to describe network variable properties.
These are used to create a new network variable with proper
attributes in network variable tables and SNVT structures.
*******************************************************************/
typedef struct
{
	IzotByte  priority;		// 1 => priority
	IzotByte  direction;		// IzotDatapointDirectionIsOutput or IzotDatapointDirectionIsInput
	IzotUbits16    selector;		// Present only for non-bindable
	IzotByte  bind;
	IzotByte  turnaround;		// 1 => turnaround
	IzotByte  service;			// IzotServiceAcknowledged, IzotServiceRepeated, IzotServiceUnacknowledged
	IzotByte  auth;			// 1 => authenticated
    IzotByte  persist;      // 1 => persist datapoints
	IzotByte  explodeArray;	// 1 => explode arrays in SNVT structure
	IzotByte  nvLength;		// length of NV in bytes.  For arrays, give the size of each item
    IzotByte  snvtDesc;         /* snvtDesc_struct in byte form. Big-endian */
    IzotByte  snvtExt;          /* Extension record. Big-endian. */
    IzotByte  snvtType;         /* 0 => non-SNVT variable. */
    IzotByte  rateEst;
    IzotByte  maxrEst;
    IzotUbits16    arrayCnt;         /* 0 for simple variables. dim for arrays. */
    const char  *nvName;           /* Name of the network variable */
    const char  *nvSdoc;           /* Sel-doc string for the variable */
    void const volatile *varAddr;          /* Address of the variable. */
    const IzotByte *ibol;
    IzotByte changeable;
} NVDefinition;

typedef enum
{
	RX_SOLICITED,					// Messages expected by node such as ack, response, challenge
	RX_UNSOLICITED,					// Basically anything else including request, ackd, reminder, reply, unackd

	NUM_RX_TYPES
} RxStatType;

//
// Blocking modes
//
// Blocking modes are intended to prevent transmission on certain channels for multi-channel devices.
//
typedef enum
{
	BM_NONE,
	BM_RF,
	BM_PL,

	BM_COUNT
} LcsBlockingMode;

// DirectionFlags used for error rate simulation or other directional things
#define DIRECTION_RX	0x01		// Apply to RX path
#define DIRECTION_TX	0x02		// Apply to TX path
typedef IzotByte DirectionFlags;

/* API Functions ***************************************************/
IzotByte MsgAlloc(MsgOut **p);    /* Returns TRUE if msg allocated and returns pointer to MsgOut */
IzotByte MsgAllocPriority(MsgOut **p);  /* Returns TRUE if msg allocated and returns pointer to MsgOut */
void     MsgSend(void);           /* Reads msgOut, sends message   */
void     MsgCancel(void);         /* Cancels MsgAlloc() or         */
								  /*   msgAllocPriority()          */
void     MsgFree(void);           /* Releases data in msgIn        */
IzotByte MsgReceive(MsgIn **p);   /* TRUE if there is msg is available and returns pointer to MsgIn */

IzotByte RespAlloc(RespOut **p);  /* Returns TRUE if resp allocated and returns pointer to RespOut */
void     RespSend(void);          /* Reads respOut, sends response */
void     RespCancel(void);        /* Cancels RespAlloc()           */
void     RespFree(void);          /* Releases data in respIn       */
IzotByte RespReceive(RespIn **p); /* Returns TRUE if resp is available and returns pointer to RespIn */

/********************************************************************
   Add a network variable with the given info.  Returns the index of
   the network variable that should be used with other functios such
   as SendNV, GetNVAddr.  For array variables, only one index value is
   returned, however each element is considered like a separate
   network variable
*********************************************************************/
IzotBits16    AddNV(NVDefinition *);

/* To send all network output variables in the node.
   Polled or not, Use Propagate function. */
void Propagate(void);

/* To send one simple network variable or a whole array */
void PropagateNV(IzotBits16 nvIndex);

/* To send an array element or any other simple variable.*/
void PropagateArrayNV(IzotBits16 arrayNVIndex, IzotBits16 index);

/* To poll all input network variables */
void  Poll(void);

/* To poll a specific simple input network variable or an array */
void  PollNV(IzotBits16 nvIndex);

/* Poll a specific array element or any other simple variable. */
void PollArrayNV(IzotBits16 arrayNVIndex, IzotBits16 index);

/* Application can call this fn to put itself offline */
void GoOffline(void);

/* Appplication can call this fn to put itself unconfigured */
void GoUnconfigured(void);

/* To get a new message tag. NewMsgTag(BIND) or NewMsgTag(NOBIND) */
MsgTag NewMsgTag(BindNoBind bindStatusIn);

/* To send a manual service request message  */
LonStatusCode ManualServiceRequestMessage(void);

/* Functions that must be defined in applications using this API */
extern LonStatusCode AppInit(void);             /* Application initialization      */
void DoApp(IzotBool isOnline);      /* Application processing          */
void LCS_RegisterClearStatsCallback(void (*cb)(void));
extern void setMem(const unsigned addr, const unsigned size);
extern void RecomputeChecksum(void);

/*
 * Function: SetAppSignature
 * Set the application signature.
 */
extern void SetAppSignature(unsigned appSignature);
extern void readIupPersistData(void);

#endif   /* #ifndef _LCS_API_H */
