//
// lcs_link.h
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

/*******************************************************************************
     Reference:        ISO/IEC 14908-1, Data Link Layer

       Purpose:        Prototypes for the LON Data Link layer implemented on a
                       Neuron processor with MIP firmware.  These interfaces
                       are not used with an IP data link.
*******************************************************************************/
#ifndef _LINK_H
#define _LINK_H

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"

typedef short LinkHandle;

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/
void LKReset(void);
void LKSend(void);
void LKReceive(void);

void LK_UpdateCommParams(CpWrite cpWrite);
void LK_RefreshNeuron(IzotUbits16 offset, IzotByte count, IzotByte offchip);
#endif

/******************************* End of link.h ********************************/
