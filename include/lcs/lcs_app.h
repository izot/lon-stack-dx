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

/*******************************************************************************

          File:        lcs_app.h

       Version:        1

       Purpose:        App Layer
                       Data Structures for App Layer.
                       (See app.c for more information.)

          Note:        None

         To Do:        None

*******************************************************************************/
#ifndef _LCS_APP_H
#define _LCS_APP_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <lcs_eia709_1.h>
#include <lcs_queue.h>
#include <lcs_tsa.h>
#include <lcs_network.h>
#include <lcs_api.h>

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/

/*******************************************************************************
Define tags for use by the application layer for the messages it generates.
There are several types of messages the application layer generates. Proxy,
ManualServiceRequest, network variable updates, and network variable polls. The tag
for these messages will be set up in such a way that we can tell what type of
message got completed when the application layer receives completion indication
from transport/session layer. Since tags are also used by the application
program and they are all >= 0, we will use negative tags for the application
layer. For tags for the application layer are used as follows:

0xFFFF   ==> tag for manual service request.
For all NV tags,
bit15 is 1 (negative tag)
bit14 is 1 => nv update 0 => nv poll
bit13 is 1 => last tag.
bit12-bit0 is the actual primary index of network variable.

The tag for which bit13 is set is a special tag value that is recognized
by transport layer. In this case, the transport layer sends an indication
right away with that tag. No message is sent by the transport layer.

Network variable updates and polls are scheduled sequentially. When the
completion event for the last tag is received, completion event is
generated.
*******************************************************************************/
#define MANUAL_SERVICE_REQ_TAG_VALUE ((MsgTag) 0xFFFF)
#define NV_UPDATE_LAST_TAG_VALUE ((MsgTag) 0xE000)
#define NV_POLL_LAST_TAG_VALUE   ((MsgTag) 0xA000)

#define MANUAL_SERVICE_REQUEST_TAG(tag)  (tag == MANUAL_SERVICE_REQ_TAG_VALUE)

#define NV_UPDATE_TAG(tag)    ((tag & 0xC000) == 0xC000)
#define NV_POLL_TAG(tag)      ((tag & 0xC000) == 0x8000)
#define NV_INDEX_OF_TAG(tag)  (tag & 0x1FFF)
#define NV_LAST_TAG(tag)      ((tag & 0xA000) == 0xA000)

#define GET_NV_UPDATE_TAG(index)      (0xC000 | index)
#define GET_NV_POLL_TAG(index)        (0x8000 | index)

/* Explicit application message codes */
#define APPL_MSG_OFFLINE       0x3F
#define FOREIGN_FRAME_OFFLINE  0x4F
/*------------------------------------------------------------------------------
Section: Type Definitions
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/
void APPInit(void);
void APPReset(void);
void APPSend(void);
void APPReceive(void);
Status SendResponse(RequestId reqId, Byte code, int len, Byte *pData);
Status SendNullResponse(RequestId reqId);

#endif  // _LCS_APP_H
