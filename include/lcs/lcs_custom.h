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
  File:           lcs_custom.h

  References:     ISO/IEC 14908-1

  Purpose:        Contains constant definitions that can be
                  used to customize the characteristics of the
                  node running the LON DX Stack.
*********************************************************************/
#ifndef _LCS_CUSTOM_H
#define _LCS_CUSTOM_H

/*------------------------------------------------------------------------------
 Section: Includes
 ------------------------------------------------------------------------------*/
#include <stddef.h>
#include "abstraction/IzotConfig.h"
#include "izot/IzotPlatform.h"
#include "izot/IzotTypes.h"
// #include "lcs/lcs_eia709_1.h" 

/*------------------------------------------------------------------------------
  Section: Constant Definitions
  ------------------------------------------------------------------------------*/
/* ReadOnlyData. Reference: Tech Device Data Book. Revision 3. p.9-6 */
#define MODEL_NUM             MODEL_NUMBER
#define MINOR_MODEL_NUM       0
#define READ_WRITE_PROTECT    0
#define RUN_WHEN_UNCONF       0 /* Set to 1 if applicaiton needs to run
even if the node is unconfigured */


/*********************************************************************
    Warning!!! The following constants are encoded. So, don't use the
    actual values
*********************************************************************/
#define LCS_BUF_SIZE		 12		// This is the encoded byte count
#define APP_OUT_BUF_SIZE     0
#define APP_IN_BUF_SIZE      0
#define RECV_REC_APDU_SIZE   LCS_BUF_SIZE
#define NW_OUT_BUF_SIZE      0
#define NW_IN_BUF_SIZE       0
#define LK_OUT_BUF_SIZE      LCS_BUF_SIZE
#define LK_SEND_BUF_SIZE     0
#define LK_RECEIVE_BUF_SIZE  0
#define CAL_RECEIVE_BUF_SIZE 0
#define TSA_IN_BUF_SIZE      0
#define TSA_OUT_BUF_SIZE     0
#define RECV_REC_RESP_SIZE   LCS_BUF_SIZE
#define TSA_RESP_BUF_SIZE    0
#define LCS_MAX_BUF_SIZE     0


#define APP_OUT_Q_CNT         3
#define APP_OUT_PRI_Q_CNT     2    /* 3 ==> 2  8 ==> 15             */
#define APP_IN_Q_CNT          3

#define NW_OUT_Q_CNT          3
#define NW_OUT_PRI_Q_CNT      2
#define NW_IN_Q_CNT           3

#define NGTIMER_SPCL_VAL   8192

#define NON_GROUP_TIMER 8

/* If the node is a member of group A(say), then typical 709.1 applications
    set group size to 1 more than actual group size if node is not
    a member of group A. This is done so that the number of
    acknowledgements to be expected is always (groupsize - 1).
    In reference implementation, there is an option
    to set this to the true group size and let the transport
    and session layers take care of this. Comment the following
    line to do this. For backward compatibility, uncomment it. */
#define GROUP_SIZE_COMPATIBILITY

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
 Section: Type Definitions
 *******************************************************************************/
typedef IzotByte DomainId[DOMAIN_ID_LEN];
typedef IzotByte AuthKey[AUTH_KEY_LEN];

/* Other values that can be initialized are in the following structure.
   The actual values are given in custom.c file. */
typedef struct
{
    /* ReadOnlyData Members */
    IzotUniqueId  UniqueNodeId;
    IzotByte twoDomains;
	IzotByte addressCnt;
    char progId[ID_STR_LEN];

	char* szSelfDoc;
	
    /* ConfigData Members */
    char location[LOCATION_LEN];

    /* Domain Table Members */
    DomainId domainId[MAX_DOMAINS];
    IzotByte len[MAX_DOMAINS];
    IzotByte subnet[MAX_DOMAINS]; /* One for each domain */
    IzotByte node[MAX_DOMAINS];
	IzotBool8 clone[MAX_DOMAINS];	/* This is really NOT clone */
    AuthKey key[MAX_DOMAINS];  /* 6 byte authentication key */

    /* Address Table Info. Enter all 5 byte values */
    /* Alias tables are not handled by some management tools
       and hence this initialization might be useful. */
} CustomData;

extern CustomData  customDataGbl[NUM_STACKS];
extern CustomData *cp;


#endif   /* #ifndef _LCS_CUSTOM_H */
