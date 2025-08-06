//
// lcs_api.h
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

/*********************************************************************
  References:     ISO/IEC 14908-1
                  Section 10. Application Layer
                  Section 10.6. Application Protocol State Variables

  Purpose:        LON DX Stack Application Program Interface
                  This is the interface file for the LON DX Stack API.
*********************************************************************/
#ifndef _LCS_API_H
#define _LCS_API_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include "lcs/lcs_custom.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
/* None */

/*********************************************************************
Section: Type Definitions
*********************************************************************/

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
IzotByte ManualServiceRequestMessage(void);

/* Functions that must be defined in applications using this API */
extern Status AppInit(void);             /* Application initialization      */
void DoApp(IzotBool isOnline);      /* Application processing          */
void LCS_RegisterClearStatsCallback(void (*cb)(void));
extern void setMem(const unsigned addr, const unsigned size);
extern void RecomputeChecksum(void);

/*
 * Function: setAppSignature
 * Set the application signature.

 */
extern void setAppSignature(unsigned appSignature);
extern void readIupPersistData(void);

#endif   /* #ifndef _LCS_API_H */
/********************************api.h*******************************/
