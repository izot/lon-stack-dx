/*
 * lcs_tcs.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Transaction Control Sublayer
 * Purpose: Implements outgoing sequencing, incoming sequencing,
 *          and duplicate detection for the LON Stack transaction
 *          control sublayer (TCS).
 * Notes:   See ISO/IEC 14908-1, Sections 8 and 9 for LON protocol details.
 * 
 *          The LON Stack transaction control sublayer (TCS) provides
 *          outgoing sequencing, incoming sequencing, and duplicate
 *          detection services shared by the LON Stack transport layer
 *          (Layer 4) and session layer (Layer 5).
 * 
 *          TCS implements a table for assigning transaction IDs and to
 *          remember the last transaction ID (TID) for each unique 
 *          destination address. When a new TID is requested for a
 *          destination, TCS searches this table for that destination. 
 *          If found, a different TID is assigned for that destination.
 *          If the destination is not found, a new entry is created in 
 *          the table.
 * 
 *          There is an entry in the table for each subnet/node, group,
 *          broadcast, subnet broadcast, unique node id, and table
 *          entry timestamp. Table entries older than 24 seconds are
 *          discarded if required to make space for a new entry.
 *          Allocation of a new TID fails if there is no space in the
 *          table for a new entry.  The table size is configurable.
 */

#ifndef _TCS_H
#define _TCS_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"

/*-------------------------------------------------------------------
  Section: Function Prototypes
  -------------------------------------------------------------------*/
void TCSReset(void);

void   TransDone(IzotByte  priorityIn);
void OverrideTrans(IzotByte   priorityIn, TransNum num);

/* Return Values:      <LonStatusCode>  */
LonStatusCode NewTrans(IzotByte   priorityIn, DestinationAddress addrIn,
                TransNum *transNumOut);

/* Return Values: TRAN_CURRENT or TRAN_NOT_CURRENT */
TransStatus ValidateTrans(IzotByte  priorityIn, TransNum transNumIn);

#endif
