/*
 * lcs_tsa.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Transport and Session Layers
 * Purpose: Implements the transport and session layers of the LON Stack.
 * Notes:   See ISO/IEC 14908-1, Sections 8 and 9 for LON protocol details.
 * 
 *          This module handles message transport and session management
 *          as specified in the LON protocol.  This module also implements
 *          an authentication sublayer that provides authentication services
 *          for both the transport and session layers.  The transport layer,
 *          session layer, and authentication sublayer are collectively
 *          referred to as the TSA (Transport/Session/Authentication) module.
 *          The transport layer provides reliable message delivery with
 *          acknowledgements and retransmissions, as well as segmentation
 *          and reassembly of messages that exceed the maximum PDU size.
 *          The session layer provides request/response message services,
 *          including matching responses to requests, and retransmitting
 *          requests when responses are not received within a specified timeout
 *          period.  The authentication sublayer provides challenge/response
 *          authentication services for both the transport and session layers.
 *          The TSA module uses the LON Stack transaction control sublayer (TCS)
 *          to assign transaction IDs to outgoing messages, and to detect
 *          duplicate incoming messages.
 */

#ifndef _TSA_H
#define _TSA_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_app.h" /* For TAG related macros and constants */
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_netmgmt.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "lcs/lcs_queue.h"
#include "lcs/lcs_tcs.h"

/*-------------------------------------------------------------------
  Section: Function Prototypes
  -------------------------------------------------------------------*/
void TSAReset(void);

void TPSend(void);
void TPReceive(void);

void SNSend(void);
void SNReceive(void);

void AuthSend(void);
void AuthReceive(void);

LonStatusCode TSA_AddressConversion(IzotSendAddress* pSrc, DestinationAddress *pDst);

#endif
