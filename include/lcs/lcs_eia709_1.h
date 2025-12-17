/*
 * lcs_eia709_1.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Protocol Constants and Types
 * Purpose: Define constants and types for the ISO/IEC 14908-1 LON protocol.
 * Notes:   This file is included by many LON Stack source files.
 */

#ifndef _EIA709_1_H
#define _EIA709_1_H

#include "lcs/lcs_platform.h"

#ifndef MIN
   #define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#ifndef MAX
   #define MAX(x,y) (((x)>(y))?(x):(y))
#endif

/* Statistics */
typedef enum __attribute__ ((packed))
{
  	LcsTxError,
	LcsTxFailure,
   LcsRxError,
	LcsRxTxFull,
	LcsLost,
	LcsMissed,
	LcsL2Rx,
	LcsL3Rx,
	LcsL3Tx,
	LcsRetry,
	LcsBacklogOverflow,
	LcsLateAck,
	LcsCollision,

	LcsNumStats
} LcsStatistic;

#define INCR_STATS(x) IncrementStat(x)
void IncrementStat(LcsStatistic x);

typedef IzotBits16 MsgTag;

typedef enum __attribute__ ((packed)) {
   NOBIND = 0,
   NON_BINDABLE = 0,   /* Same as NOBIND */
   BIND   = 1,
   BINDABLE = 1       /* Same as BIND   */
} BindNoBind;

/* Address Types */
typedef enum __attribute__ ((packed)) {
   UNBOUND           = 0,
   AM_SUBNET_NODE    = 1,
   AM_UNIQUE_NODE_ID = 2,
   AM_BROADCAST      = 3,
   AM_MULTICAST      = 4,
   AM_MULTICAST_ACK  = 5
} AddrMode;

// Node state masks
#define IS_APPLESS			0x01
#define IS_HARDOFFLINE		0x02
#define IS_CONFIGURED		0x04

/* These constants are used to represent the mode when the device is configured */
typedef enum __attribute__ ((packed)) {
   OFF_LINE    = 0,  /* For soft off-line */
   ON_LINE     = 1,  /* For normal mode   */
   NOT_RUNNING = 2   /* For hard-offline  */
} ConfigMode;

/* The order of the first 4 items is important as it is used by
   network layer to determine the type of PDU */
typedef enum __attribute__ ((packed)) {
   /* Do not change the order. The values are sent across the network */
   TPDU_TYPE,
   SPDU_TYPE,
   AUTHPDU_TYPE,
   APDU_TYPE,

   /* Something extra for internal use of the protocol stack */
   NPDU_TYPE,
   LPDU_TYPE
} PDUType;

/* Tranceiver types. Only the constants are used */
typedef enum __attribute__ ((packed)) {
   BLANK            = 0,
   SINGLE_ENDED     = 1,
   SPECIAL_PURPOSE  = 2,
   DIFFERENTIAL     = 5
} TranceiverType;

typedef IzotUbits16  TransNum; /* For transaction numbers */
typedef IzotUbits16 RequestId; /* For matching responses with requests */

#pragma pack(push, 1)

typedef struct {
   IzotReceiveGroup group;
   IzotByte member;
} GroupAddress;

typedef struct __attribute__ ((packed)) {
   IzotReceiveSubnetNode subnetAddr;
   GroupAddress  groupAddr;   /* Acknowledging group member */
} MulticastAckAddress;

typedef struct __attribute__ ((packed)) {
   IzotByte domainIndex;  /* 0 or 1 or FLEX_DOMAIN (2) */
	IzotByte domainLen;
	IzotByte domainId[DOMAIN_ID_LEN];
} Domain;

/*******************************************************************************
   DestinationAddress is used to indicate network layer which address
   mode is used to send the message. The destination address
   is always one of the five possibilities.
   domainIndex indicates the domain table to use to determine
   domain length and domain id.
   If domainIndex = FLEX_DOMAIN, then it is flexdomain.
   In this case, src subnet/node is 0/0.
*******************************************************************************/
typedef struct __attribute__ ((packed)) {
	Domain dmn;
    AddrMode addressMode;
    union {
        IzotReceiveBroadcast	addr0;
        IzotReceiveGroup		addr1;
        IzotReceiveSubnetNode	addr2a;
        MulticastAckAddress		addr2b;
        IzotReceiveUniqueId		addr3;
    } addr;
} DestinationAddress;

/*******************************************************************************
   SourceAddress is used by network layer to indicate to upper levels
   who sent the message and what mode was used.
   Thus this structure is used only for receiving messages.
   domainIndex is used to respond back in the domain in which
   the message was received.
*******************************************************************************/
typedef struct __attribute__ ((packed)) {
   IzotReceiveSubnetNode	subnetAddr;   /* Subnet of source node */
   AddrMode				addressMode;  /* What mode used */
	Domain					dmn;
    /* group is used only if addressMode is AM_MULTICAST.
       It is the group of the source node sending this message. */
    IzotReceiveGroup		group;
    /* ackNode is used only if addressMode is AM_MULTICAST_ACK.
       It is the destSubnet used and the group of the node sending the ack
       or response. */
    MulticastAckAddress		ackNode;
    /* Destination subnet for broadcast messages */
    IzotByte				broadcastSubnet;
} SourceAddress;

/*******************************************************************************
   OmaAddress is used for open media authentication.  Unused fields must be all ones
*******************************************************************************/

typedef union {
	struct {
		IzotByte   uniqueId[UNIQUE_NODE_ID_LEN];
	} physical;
	struct {
		IzotByte   domainId[DOMAIN_ID_LEN];
		IzotByte   domainLen;
		union {
			IzotReceiveSubnetNode snode;	// selField must be 0
			IzotByte group;
		} addr;
	} logical;
} OmaAddress;

//
// XcvrParam - the transceiver specific registers.  May contain fixed info,
// trend info, per packet info or some combination of these.
//
typedef struct {
	IzotByte data[NUM_COMM_PARAMS];
} XcvrParam;

#pragma pack(pop)

typedef void (*FnType)(void); /* Type of Reset, Send and Receive functions */

#endif   /* #ifndef _EIA709_1 */
