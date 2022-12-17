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
          File:        lcs_tcs.c

       Version:        1

       Purpose:        Interface file for transaction control sublayer.
                       Outgoing sequencing.
                       Incoming sequencing and duplicate detection.

          Note:        For assigning TIds, a table is used. We
                       remember the last TID for each unique destination
                       address. When a new TId is requested for a
                       destination, this table is searched for that
                       destination. If found, we make sure that we
                       don't assign the same TId used for that
                       destination. If the destination is not
                       found, we make a new entry in the table.

                       We have an entry in the table for each
                       subnet/node, group, broadcast, subnet
                       broadcast, unique node id. When a table entry
                       is assigned, we remember the time stamp too.
                       If the table does not have space for a new
                       destination address, we get rid of one which has
                       remained more than 24 seconds. If there is no
                       such entry, then we fail to allocate the new
                       transaction ID. The table size is configurable.

         To Do:        None
*********************************************************************/

/* *** START INFORMATIVE - Transaction ID Allcation *** */
/* These functions represent an example means of allocating transaction IDs.
 * There are in fact several valid mechanisms for allocating transaction IDs.
 * In addition to the mechanism below, there are at least two other accepted means
 * for allocating transaction IDs.
 * 1. Allocate a transaction ID per unique destination address.  This method should
 *    not be used if acknowledged or request/response using unique ID or broadcast
 *    addressing are using in time proximity with the other addressing modes.
 *    Note that such combinations can be accomplished in this scheme if guardbands
 *    are placed around expected arrivals of acks/responses per transaction ID.
 * 2. Allocate all transaction IDs from a single transaction ID space without
 *    conflict checking.  If this simple scheme is used, it is recommended that
 *    conflict checking be performed by the application. */

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <lcs_eia709_1.h>
#include <lcs_node.h>
#include <lcs_tcs.h>

/*--------------------------------------------------------------------
Section: Constant Definitions
--------------------------------------------------------------------*/
/* Minimum amount of time in seconds a record in the priTbl or
   non-priTbl should stay before it can be replaced with a new
   entry. i.e., if the table is full and a new entry is needed,
   we look for an entry which has remained in the table for
   at least MIN_TABLE_TIME seconds. */
#define MIN_TABLE_TIME 24

/*--------------------------------------------------------------------
Section: Type Definitions
--------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Globals
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Function Prototypes
-------------------------------------------------------------------*/
/*****************************************************************
Function:  TCSReset
Returns:   None
Reference: Section 7, Protocol Specification.
Purpose:   To initialize all globals to proper values.
Comments:  None.
******************************************************************/
void TCSReset(void)
{
    gp->priTransID    = 0; /* On node reset, transaction id 0 is used. */
    gp->nonpriTransID = 0;
    gp->priTransCtrlRec.inProgress    = FALSE;
    gp->nonpriTransCtrlRec.inProgress = FALSE;
    /* Reset the tables that keep track of (destination address
       transaction id) pairs only duing powerup or external reset.
       When resetCause is software reset or cleared, we keep this
       table to ensure that we don't send a message to a destination
       with tid same as the one used last time for that destination.
       For power-up or external reset, we also need to ensure that
       using some other technique. We will delay transport or session
       layer sends by a small amount so that no messages are pending in
       target nodes. If we don't follow these guidelines, the target
       node may throw away messages sent after a reset as duplicates. */
    if (nmp->resetCause == POWER_UP_RESET || nmp->resetCause == EXTERNAL_RESET)
    {
        gp->priTblSize       = 0;
        gp->nonpriTblSize    = 0;
    }
}

/*****************************************************************
Function:  NewTrans
Returns:   SUCCESS if a transaction id can be assigned.
           FAILURE if it is not possible to assign an id.
Purpose:   To get a new transaction id.
Comments:  This function implements a new algorithm to assign the
           transaction id. It does not use the one in protocol specification.
Alg Idea:  For each of the following categories, we have an
           entry in the table.
           1. Subnet/Node
           2. group
           3. broadcast (domainwide or subnet)
           4. unique node id
           When a new id is requested, we increment the tid from
           the single space. We then search the table for this
           entry. If a matching entry is found, then we check
           if that tid was used last time for the same destination.
           If so, we bump it up by one. If not we use it. In
           either case, we record this tid in the table.
           If there was no such entry, we create a new one.
           If there is no space for the new entry, we release one
           that has remained more than MIN_TABLE_TIME seconds.
           If no such entry, then we fail to assign a tid.
******************************************************************/
Status NewTrans(Boolean   priorityIn, DestinationAddress addrIn,
                TransNum *transNumOut)
{
    uint16           i;
    TransCtrlRecord *transRecPtr;
    TransNum        *transNumPtr;
    TIDTableEntry   *tbl;
    uint16          *tblSize;
    Boolean          found;

    /* Point to the appropriate control record & table. */
    if (priorityIn)
    {
        transRecPtr = &gp->priTransCtrlRec;
        transNumPtr = &gp->priTransID;
        tbl         = gp->priTbl;
        tblSize     = &gp->priTblSize;
    }
    else
    {
        transRecPtr = &gp->nonpriTransCtrlRec;
        transNumPtr = &gp->nonpriTransID;
        tbl         = gp->nonpriTbl;
        tblSize     = &gp->nonpriTblSize;
    }

    /* Check if transaction already in progress. */
    if (transRecPtr->inProgress)
    {
        /* We can't allow a new transaction. Return failure. */
        return(FAILURE);
    }

    /* We can allow the transaction. Allocate a new TID. */
    transRecPtr->transNum = *transNumPtr;

    /* Make sure that this dest did not use this TID last time.
       If it did, increment the TID. */
    /* Note: addrIn.addressMode can never be MULTICAST_ACK
             for transactions initiated by a node. */
    found = FALSE;
    for (i = 0; i < *tblSize; i++)
    {
        /* If domainId does not match, skip entry. */
        if (addrIn.dmn.domainIndex != FLEX_DOMAIN &&
                (eep->domainTable[addrIn.dmn.domainIndex].invalid ||
                 tbl[i].len != eep->domainTable[addrIn.dmn.domainIndex].len ||
                 memcmp(eep->domainTable[addrIn.dmn.domainIndex].domainId,
                        tbl[i].domainId,
                        eep->domainTable[addrIn.dmn.domainIndex].len) != 0)
           )
        {
            continue; /* Not flex domain but domain mismatch. */
        }

        if (addrIn.dmn.domainIndex == FLEX_DOMAIN &&
                (tbl[i].len != addrIn.dmn.domainLen ||
                 memcmp(addrIn.dmn.domainId,
                        tbl[i].domainId,
                        addrIn.dmn.domainLen) != 0)
           )
        {
            continue; /* Flex domain but domain mismatch. */
        }

        switch (addrIn.addressMode)
        {
        case SUBNET_NODE:
            if (tbl[i].addressMode == SUBNET_NODE &&
                    memcmp(&tbl[i].addr.subnetNode, &addrIn.addr.addr2a,
                           sizeof(SubnetAddress)) == 0)
            {
                found = TRUE;
            }
            break;
        case UNIQUE_NODE_ID:
            if (tbl[i].addressMode == UNIQUE_NODE_ID &&
                    memcmp(tbl[i].addr.uniqueNodeId,
                           addrIn.addr.addr3.uniqueId,
                           UNIQUE_NODE_ID_LEN) == 0)
            {
                found = TRUE;
            }
            break;
        case MULTICAST:
            if (tbl[i].addressMode == MULTICAST &&
                    tbl[i].addr.group  == addrIn.addr.addr1)
            {
                found = TRUE;
            }
            break;
        case BROADCAST:
            if (tbl[i].addressMode == BROADCAST &&
                    tbl[i].addr.subnet == addrIn.addr.addr0)
            {
                found = TRUE;
            }
            break;
        default:
            ErrorMsg("NewTrans: Unexpected addressMode.\n");
            /* Should not come here. */
        }
        if (found)
        {
            break; /* Need to leave for loop with matched i value. */
        }
    }

    if (found)
    {
        /* Found a match. Check if last TID is same or not. */
        /* We can reuse this entry and reinitialize timer.  */
        if (tbl[i].tid == *transNumPtr)
        {
            /* Increment TID. */
            (*transNumPtr)++;
            if (*transNumPtr == 16)
            {
                *transNumPtr = 1; /* Wrap around. */
            }
            transRecPtr->transNum = *transNumPtr;
        }
        tbl[i].tid   = *transNumPtr;
        MsTimerSet(&tbl[i].timer,
                   (uint16)(MIN_TABLE_TIME * 1000));
        *transNumOut = *transNumPtr;
        transRecPtr->inProgress = TRUE;
        return(SUCCESS);
    }

    /* No match. Make a new entry. If no space. get a space. */
    /* All the timers must have been updated in the for loop above. */
    if (*tblSize == TID_TABLE_SIZE)
    {
        /* Table is full. See if any entry can be replaced. */
        found = FALSE;
        for (i = 0; i < *tblSize; i++)
        {
            if (MsTimerExpired(&tbl[i].timer))
            {
                found = TRUE;
                break;
            }
        }
        if (found)
        {
            /* Replace this entry with last entry of table. */
            (*tblSize)--;
            tbl[i] = tbl[*tblSize];
            /* Fall through to code below. */
        }
        else
        {
            /* Unable to find an entry. */
            return(FAILURE);
        }
    }

    /* Now we have space for an entry. Add new entry. */
    /* Store the domain len and domain id in table */
    if (addrIn.dmn.domainIndex == FLEX_DOMAIN)
    {
        memcpy(tbl[*tblSize].domainId,
               addrIn.dmn.domainId,
               addrIn.dmn.domainLen);
        tbl[i].len = addrIn.dmn.domainLen;
    }
    else
    {
        memcpy(tbl[*tblSize].domainId,
               eep->domainTable[addrIn.dmn.domainIndex].domainId,
               eep->domainTable[addrIn.dmn.domainIndex].len);
        tbl[i].len = eep->domainTable[addrIn.dmn.domainIndex].len;
    }

    tbl[*tblSize].addressMode = addrIn.addressMode;
    if (addrIn.addressMode == MULTICAST)
    {
        tbl[*tblSize].addr.group = addrIn.addr.addr1;
    }
    else if (addrIn.addressMode == SUBNET_NODE)
    {
        tbl[*tblSize].addr.subnetNode = addrIn.addr.addr2a;
    }
    else if (addrIn.addressMode == UNIQUE_NODE_ID)
    {
        memcpy(tbl[*tblSize].addr.uniqueNodeId,
               addrIn.addr.addr3.uniqueId,
               UNIQUE_NODE_ID_LEN);
    }
    else if (addrIn.addressMode == BROADCAST)
    {
        tbl[*tblSize].addr.subnet = addrIn.addr.addr0;
    }
    else
    {
        /* Should not come here as addressMode was checked before too. */
        ErrorMsg("NewTrans: Invalid addressMode at unexpected place\n");
    }
    MsTimerSet(&tbl[*tblSize].timer, (uint16)(MIN_TABLE_TIME * 1000));
    tbl[*tblSize].tid = *transNumPtr;
    *transNumOut      = *transNumPtr;
    (*tblSize)++;
    transRecPtr->inProgress = TRUE;
    return(SUCCESS);
}

/*****************************************************************
Function:  OverrideTrans
Returns:   None
Reference: 
Purpose:   Override the TX# chosen by NewTrans.
Comments:  None
******************************************************************/
void OverrideTrans(Boolean   priorityIn, TransNum num)
{
    if (priorityIn)
    {
	    gp->priTransID = num;
		gp->priTransCtrlRec.transNum = num;
    }
    else
    {
	    gp->nonpriTransID = num;
		gp->nonpriTransCtrlRec.transNum = num;
    }
}

/*****************************************************************
Function:  TransDone
Returns:   None
Reference: Section 7 Protocol Spec.
Purpose:   To release the transaction record for future assignments.
Comments:  None
******************************************************************/
void TransDone(Boolean  priorityIn)
{
    TransCtrlRecord *transRecPtr;
    TransNum        *transNumPtr;

    /* Point to the appropriate control record & table. */
    if (priorityIn)
    {
        transRecPtr = &gp->priTransCtrlRec;
        transNumPtr = &gp->priTransID;
    }
    else
    {
        transRecPtr = &gp->nonpriTransCtrlRec;
        transNumPtr = &gp->nonpriTransID;
    }

    /* Mark transaction as available. */
    transRecPtr->inProgress = FALSE;

    /* Increment the corresponding transaction id. */
    (*transNumPtr)++;
    if (*transNumPtr == 16)
    {
        *transNumPtr = 1; /* Wrap Around to 1. */
    }
}

/*****************************************************************
Function:  ValidateTrans
Returns:   TRANS_CURRENT if the transNumIn matches transaction
           in progress.
           TRANS_NOT_CURRENT othewise.
Reference: Section 7, Protocol Specification.
Purpose:   To check if a given transNumIn is current or not.
Comments:  None
******************************************************************/
TransStatus ValidateTrans(Boolean  priorityIn,
                          TransNum transNumIn)
{
    TransCtrlRecord *transRecPtr;

    /* Point to the appropriate control record & table. */
    if (priorityIn)
    {
        transRecPtr = &gp->priTransCtrlRec;
    }
    else
    {
        transRecPtr = &gp->nonpriTransCtrlRec;
    }

    if (transRecPtr->inProgress &&
            transRecPtr->transNum == transNumIn)
    {
        return(TRANS_CURRENT);
    }
    else
    {
        return(TRANS_NOT_CURRENT);
    }
}

/* *** END INFORMATIVE - Transaction ID Allcation *** */

/*------------------------End of tcs.c------------------------*/
