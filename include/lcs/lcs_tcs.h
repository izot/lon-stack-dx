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
          File:        lcs_tcs.h

       Version:        1

       Purpose:        Interface for Transaction Control Sublayer.
                       Outgoing Sequencing
                       Incoming Sequencing and Duplicate Detection.

          Note:        For assigning TIds, a table is used. We
                       remember the last TID for each unique dest
                       addr. When a new TId is requested for a
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
                       dest addr, we get rid of one which has
                       remained more than 24 sec. If no such entry,
                       then we fail to allocate the new trans ID.
                       The table size is configurable.

         To Do:        None

*********************************************************************/
#ifndef _TCS_H
#define _TCS_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <lcs_eia709_1.h>
#include <lcs_node.h>

/*-------------------------------------------------------------------
Section: Constant Definitions
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Type Definitions
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Globals
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Function Prototypes
-------------------------------------------------------------------*/
void TCSReset(void);

void   TransDone(Boolean  priorityIn);
void OverrideTrans(Boolean   priorityIn, TransNum num);

/* Return Values:      SUCCESS or FAILURE  */
Status NewTrans(Boolean   priorityIn, DestinationAddress addrIn,
                TransNum *transNumOut);

/* Return Values: TRAN_CURRENT or TRAN_NOT_CURRENT */
TransStatus ValidateTrans(Boolean  priorityIn, TransNum transNumIn);

#endif
/*------------------------End of tcs.h------------------------*/
