/*
 * lcs_proxy.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Enhanced Proxy (LTEP) Repeating Implementation
 * Purpose: Implements the LON Enhanced Proxy (LTEP) repeating functions.
 * Notes:   LTEP repeating allows a LON Stack DX device to function
 *          as a proxy repeater for LON Enhanced Proxy messages.
 *          LTEP repeating is only used in systems using LON power line
 *          communication that require the extended range provided
 *          by LTEP repeating.
 */

#ifndef __PROXY_H
#define __PROXY_H

#ifdef ENABLE_PROXY_REPEATING

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_api.h"
#include "lcs/lcs_app.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_netmgmt.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"

#pragma pack(push, 1)

// Following the proxy message code are N subnet/node definitions
typedef struct ProxyNeuronIdAddress {
    unsigned char     subnet;           // routing subnet
    unsigned char     nid[6];         // Neuron ID
} ProxyNeuronIdAddress;

typedef struct ProxyNeuronIdAddressCompact {
    // Routing subnet of 0 is used.
    unsigned char     nid[6];         // Neuron ID
} ProxyNeuronIdAddressCompact;

typedef struct ProxySubnetNodeAddress {
    unsigned char subnet;
	BITS2(path,			1,
		  node,			7)
} ProxySubnetNodeAddress;

typedef struct ProxySubnetNodeAddressCompact {
    // Subnet used is that of agent
	BITS2(path,			1,
		  node,			7)
} ProxySubnetNodeAddressCompact;

typedef struct ProxyGroupAddress {
    unsigned char     group;
    unsigned char     size;
} ProxyGroupAddress;

typedef struct ProxyGroupAddressCompact {
    // Size of 0 is used (for unackd and unackd/rpt)
    unsigned char     group;
} ProxyGroupAddressCompact;

typedef struct ProxyBroadcastAddress {
    unsigned char     subnet;
    unsigned char     backlog;
} ProxyBroadcastAddress;

// Note that this is defined as a union.  However, the actual
// size of data shipped is the minimum size needed to convey
// the enclosed structure.
typedef union ProxyTargetAddress {
    ProxyNeuronIdAddress nid;
    ProxyNeuronIdAddressCompact nidc;
    ProxySubnetNodeAddress sn;
    ProxySubnetNodeAddressCompact snc;
    ProxyGroupAddress gp;
    ProxyGroupAddressCompact gpc;
    ProxyBroadcastAddress bc;
} ProxyTargetAddress;

typedef struct ProxyHeader {
	BITS5(uniform_by_dest,	1,
		  long_timer,		1,
		  all_agents,		1,
		  uniform_by_src,	1,
		  count,			4)
} ProxyHeader;

typedef struct ProxyTxCtrl {
	BITS2(retry,			4,
		  timer,			4)
} ProxyTxCtrl;

typedef enum {
    PROXY_AGENT_MODE_NORMAL = 0,
    PROXY_AGENT_MODE_ZERO_SYNC = 1,
    PROXY_AGENT_MODE_ALTKEY = 2,
    PROXY_AGENT_MODE_ATTENUATE = 3
} ProxySicbMode;

//
//
// This structure differs from the normal SICB in the following ways:
// 1. No auth field is needed - this is inherited.
// 2. No tag field is needed - we correlate using the rcvtx index.
// 3. No length field is needed - this comes from the message length.
//
typedef struct ProxySicb {
    // The following fields are used by the agent to talk
    // to the target.
	BITS4(type,			3,			// ProxyAddressType
		  path,			1,			// Primary or alternate path
		  service,		2,			// Service Type
		  mode,			2)			// ProxySicbMode		 
    ProxyTxCtrl       txctrl;
} ProxySicb;

#define AUTH_STD    0
#define AUTH_OMA    1

typedef struct ProxyAuthKey {
    // if "altkey" is zero
	BITS2(mbz,			6,
		  type,			2)			// AUTH_STD
		  
    unsigned char     key[6];         // 48 bit key
} ProxyAuthKey;

typedef struct ProxyOmaKey {
    // if "altkey" is one
	BITS2(mbz,			6,
		  type,			2)				// AUTH_OMA		  
    unsigned char     key[12];        // 96 bit key
} ProxyOmaKey;

#define MAX_PROXY_DATA 102

typedef struct ProxyTargetApdu {
    unsigned char     code;
    unsigned char     data[MAX_PROXY_DATA];      // message data where
    // length is determined by length of remainder of packet.
} ProxyTargetApdu;

typedef enum {
    PX_GROUP = 0,
    PX_SUBNET_NODE = 1,
    PX_NEURON_ID = 2,
    PX_BROADCAST = 3,
    PX_GROUP_COMPACT = 4,
    PX_SUBNET_NODE_COMPACT_SRC = 5,
    PX_NEURON_ID_COMPACT = 6,
    PX_SUBNET_NODE_COMPACT_DEST = 7,

    PX_ADDRESS_TYPES
} ProxyAddressType;

#pragma pack(pop)

#define LT_ENHANCED_PROXY_SUCCESS		0x4D
#define LT_ENHANCED_PROXY_FAILURE		0x4C

LonStatusCode ProcessLTEP(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr);
LonStatusCode ProcessLtepCompletion(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr, LonStatusCode status);

#endif // ENABLE_PROXY_REPEATING

#endif // __PROXY_H
