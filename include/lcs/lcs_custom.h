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
  File:           lcs_custom.h

  Version:        1

  References:     Protocol Spec.

  Purpose:        Contains constant definitions that can be
                  used to customize the characteristics of the
                  node running the reference implementation.

  Note:           None.

  To Do:          None.

*********************************************************************/
#ifndef _LCS_CUSTOM_H
#define _LCS_CUSTOM_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include "lcs_eia709_1.h"  /* To get constants and types (e.g. Byte) */

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
/* ReadOnlyData. Reference: Tech Device Data Book. Revision 3. p.9-6 */
#define MODEL_NUM             128
#define MINOR_MODEL_NUM       0
#define READ_WRITE_PROTECT    0
#define RUN_WHEN_UNCONF       0 /* Set to 1 if applicaiton needs to run
even if the node is unconfigured */

/*********************************************************************
   NUM_ADDR_TBL_ENTRIES:
   The address table in reference implementation supports more than
   15 entries. However, network management tools might support only
   upto 15 entries. So, other entries should be managed by the
   application program itself. The maximum supported value is 255.
**********************************************************************/
#define NUM_ADDR_TBL_ENTRIES    5    /* # of entries in addr tbl    */

#define RECEIVE_TRANS_COUNT     5    /* Can be > 16 for Ref. Impl */

#define NV_TABLE_SIZE          20    /* Check management tool for any restriction on maximum size */

#define NV_ALIAS_TABLE_SIZE    10    /* Check management tool for any restriction on maximum size */

#define SNVT_SIZE             200    /* Maximum allowed storage space for SNVT structures */

    /*********************************************************************
    Warning!!! The following constants are encoded. So, don't use the
    actual values
    *********************************************************************/
#define LCS_BUF_SIZE		 12
#define APP_OUT_BUF_SIZE     LCS_BUF_SIZE
#define APP_IN_BUF_SIZE      LCS_BUF_SIZE
#define NW_OUT_BUF_SIZE      LCS_BUF_SIZE
#define NW_IN_BUF_SIZE       LCS_BUF_SIZE

#define APP_OUT_Q_CNT         3
#define APP_OUT_PRI_Q_CNT     3    /* 3 ==> 2  8 ==> 15             */
#define APP_IN_Q_CNT          3

#define NW_OUT_Q_CNT          3
#define NW_OUT_PRI_Q_CNT      3
#define NW_IN_Q_CNT           3

#define NGTIMER_SPCL_VAL   8192

#define NON_GROUP_TIMER 8

    /* Reference: Tech Device Data Book. Revision 3. p9-27. */
#define NM_AUTH 0

    /* Node Self Documentation String */
#define NODE_DOC  "Ref. Impl. Pgm."

    /* If the node is a member of group A(say), then typical 709.1 applications
       set group size to 1 more than actual group size if node is not
       a member of group A. This is done so that the number of
       acknowledgements to be expected is always (groupsize - 1).
       In reference implementation, there is an option
       to set this to the true group size and let the transport
       and session layers take care of this. Comment the following
       line to do this. For backward compatibility, uncomment it.
    */
#define GROUP_SIZE_COMPATIBILITY

    /* Maximum number of array network variables allowed in
       the application program. This constant is used to allocate
       space that keeps track of all arrays and their dimension */
#define MAX_NV_ARRAYS 10

    /* Maximum number of network output variables that can
       be scheduled to be sent out at any point in time */
#define MAX_NV_OUT     5

    /* To implement synchronous variables, the values of the
       variables are to be stored along with index in the queue.
       Define the maximum size (in bytes) of a network variable
       in the application program. This is used for storage allocation. */
#define MAX_NV_LENGTH 50

    /* Maximum number of network input variables that can
       be scheduled to be polled at any point in time */
#define MAX_NV_IN     50

    /* Maximum number of bytes in data array for msg_in, msg_out, resp_in etc.
       This value is indepedent of application buffer sizes mentioned
       earlier. Clearly it does not make sense for this value to be
       larger than application buffer size (out or in). */
#define MAX_DATA_SIZE 114

    /* Maximum size of message on the wire (approximate, might be over sized a byte or two for safety.) */
#define MAX_PDU_SIZE (MAX_DATA_SIZE+21)

    /* The following constant is used to delay transport and session
       layers on an external or power-up reset for sending messages
       to make sure that messages sent after a reset are not discarded
       as duplicates. The dealy should be small enough to ensure that
       there will be no message in receive transaction records of
       target nodes. Normal default is 2 seconds. The amount is given
       in milliseconds.
       The timer duration would be set based on the maximum expected receive
       timer value in all target nodes. */
#define TS_RESET_DELAY_TIME 2000

    /*******************************************************************************
       Protocol Stack Implementation uses an array to allocate storage
       space dynamically. The size of the array used for this allocation
       is determined by this constant. If it is too low, it may be
       impossible to allocate necessary buffers or data structures.
       If it is too high, some memory is unused.
       Set to some high value, run program, stop, and check
       gp->mallocUsedSize to determine the current usage.
       This array space is allocated during Reset of all layers.
       Tracing through the Reset code of all layers will indicate
       the approximate size of this array necessary.
       If AllocateStorage function in node.c is rewritten to use malloc, then
       this constant will be of no use.
    *******************************************************************************/
#define MALLOC_SIZE     4500

    /*******************************************************************************
    Section: Type Definitions
    *******************************************************************************/
    typedef Byte DomainId[DOMAIN_ID_LEN];
typedef Byte AuthKey[AUTH_KEY_LEN];

/* Other values that can be initialized are in the following structure.
   The actual values are given in custom.c file. */
typedef struct
{
    /* ReadOnlyData Members */
    Byte uniqueNodeId[UNIQUE_NODE_ID_LEN];
    Byte twoDomains;
    char progId[ID_STR_LEN];

    /* ConfigData Members */
    char location[LOCATION_LEN];

    /* Domain Table Members */
    DomainId domainId[MAX_DOMAINS];
    Byte len[MAX_DOMAINS];
    Byte subnet[MAX_DOMAINS]; /* One for each domain */
    Byte node[MAX_DOMAINS];
    AuthKey key[MAX_DOMAINS];  /* 6 byte authentication key */

    /* Address Table Info. Enter all 5 byte values */
    /* Note: LonBuilder will overwrite the first 15 entries during
       Load/Start. This was used for only for testing purpose */
    /* Alias tables are not handled by some management tools
       and hence this initialization might be useful. */
    Byte addrTbl[NUM_ADDR_TBL_ENTRIES][5];
    Byte aliasTbl[NV_ALIAS_TABLE_SIZE][6];

} CustomData;

extern CustomData  customDataGbl[NUM_STACKS];
extern CustomData *cp;


#endif   /* #ifndef _LCS_CUSTOM_H */

