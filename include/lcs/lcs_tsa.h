//
// lcs_tsa.h
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
  Reference:        ISO/IEC 14908-1, Sections 8, 9

    Purpose:        Interfaces for LON transport, session, and
                    authentication layers in tsa.c
*********************************************************************/

#ifndef _TSA_H
#define _TSA_H

/*------------------------------------------------------------------------------
  Section: Includes
  ------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "abstraction/IzotConfig.h"
#include "izot/IzotApi.h"
#include "izot/IzotTypes.h"
#include "lcs/lcs_app.h" /* For TAG related macros and constants */
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_netmgmt.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "lcs/lcs_queue.h"
#include "lcs/lcs_tcs.h"


/*-------------------------------------------------------------------
  Section: Constant Definitions
  -------------------------------------------------------------------*/

/*-------------------------------------------------------------------
  Section: Type Definitions
  -------------------------------------------------------------------*/

/*-------------------------------------------------------------------
  Section: Globals
  -------------------------------------------------------------------*/

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

Status TSA_AddressConversion(IzotSendAddress* pSrc, DestinationAddress *pDst);

#endif
/*------------------------End of tsa.h------------------------*/
