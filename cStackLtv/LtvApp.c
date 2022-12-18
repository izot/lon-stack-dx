//
// ltvapp.c
//
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

//
// This application is used for the validation of LonTalk implementations.  The
// unit under test must have this application loaded before running the script
// called ltv.scr.  This program must be ported as appropriate for host
// applications.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <node.h>

#include "Api.h"

//#pragma app_buf_in_size 255
//#pragma app_buf_out_size 255
//#pragma net_buf_in_size 255
//#pragma net_buf_out_size 255

//#pragma net_buf_in_count 7

LonTimer lightTimer;
LonTimer actionTimer;

//#pragma num_alias_table_entries 10
//#pragma num_addr_table_entries 15
//#pragma enable_sd_nv_names

MsgTag eventTag;
MsgTag M1,M2,M3,M4,M5,M6;

int changes = 0;

typedef struct {
    char d[31];
} Maxdata;

nint firstnvo;
NVDefinition firstnvoDef =
{
    FALSE, NV_OUTPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(nint), 0x80, 0x30, 0, 0, 0,
    0, "firstnvo", "firstnvo doc", &firstnvo
};

int16 secondnvo;
NVDefinition secondnvoDef =
{
    FALSE, NV_OUTPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(int16), 0x80, 0x30, 0, 0, 0,
    0, "secondnvo", "secondnvo doc", &secondnvo
};

float thirdnvo;
NVDefinition thirdnvoDef =
{
    FALSE, NV_OUTPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(float), 0x80, 0x30, 0, 0, 0,
    0, "thirdnvo", "thirdnvo doc", &thirdnvo
};

Maxdata fourthnvo;
NVDefinition fourthnvoDef =
{
    FALSE, NV_OUTPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(Maxdata), 0x80, 0x30, 0, 0, 0,
    0, "fourthnvo", "fourthnvo doc", &fourthnvo
};

nint arraynvo[10];
NVDefinition arraynvoDef =
{
    FALSE, NV_OUTPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    1, sizeof(nint), 0x80, 0x30, 0, 0, 0,
    10, "arraynvo", "arraynvo doc", arraynvo
};

nint firstnvi;
NVDefinition firstnviDef =
{
    FALSE, NV_INPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(nint), 0x80, 0x30, 0, 0, 0,
    0, "firstnvi", "firstnvi doc", &firstnvi
};

int16 secondnvi;
NVDefinition secondnviDef =
{
    FALSE, NV_INPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(int16), 0x80, 0x30, 0, 0, 0,
    0, "secondnvi", "secondnvi doc", &secondnvi
};

float thirdnvi;
NVDefinition thirdnviDef =
{
    FALSE, NV_INPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(float), 0x80, 0x30, 0, 0, 0,
    0, "thirdnvi", "thirdnvi doc", &thirdnvi
};

Maxdata fourthnvi;
NVDefinition fourthnviDef =
{
    FALSE, NV_INPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(Maxdata), 0x80, 0x30, 0, 0, 0,
    0, "fourthnvi", "fourthnvi doc", &fourthnvi
};

nint arraynvi[10];
NVDefinition arraynviDef =
{
    FALSE, NV_INPUT, 0, 0, 1, FALSE, ACKD, FALSE,
    0, sizeof(nint), 0x80, 0x30, 0, 0, 0,
    10, "arraynvi", "arraynvi doc", arraynvi
};

char scratch[256];

typedef enum {
    NVUPDATE,
    NVSUCCESS,
    NVFAILURE,
    MSGARRIVES,
    MSGSUCCESS,
    MSGFAILURE,
    RESPARRIVES,
} EventCode;

void HandleActionTimer(void);
void HandleMsg(void);
void HandleResp(void);
void delay(int16 v);
void reportEventData(EventCode eventCode, int hdr, int code, char *pData, int len);
void reportEvent(EventCode eventCode, int index);

void delay(int16 amount)
{
    int16 i;
    volatile float f = 1;
    for (i=0; i<amount; i++)
    {
        f = f*10;
    }
}

void reportEventData(EventCode eventCode, int hdr, int code, char *pData, int len)
{
    msg_out.tag = eventTag;
    msg_out.code = 0x40;
    msg_out.service = UNACKD;
    msg_out.data[0] = eventCode;
    msg_out.data[1] = hdr;
    msg_out.data[2] = code;
    if (len)
        memcpy(&msg_out.data[3], pData, len);
    msg_send();
}

void reportEvent(EventCode eventCode, int index)
{
    // Since events are unackd, delay a bit to avoid too rapid fire delivery
    delay(4000);
    msg_out.tag = eventTag;
    msg_out.code = 0x40;
    msg_out.data[0] = eventCode;
    msg_out.data[1] = index;
    msg_out.service = UNACKD;
    msg_send();
}

typedef struct {
    unsigned :3;
    unsigned len:5;
    char *addr;
} NvFixed;

struct msgParams {
    int action;
    int pri;
    int tag;
    int svc;
    int code;
    int len;
    int data[1];
};
struct nvParams {
    int action;
    int index;
    int len;
    int data[1];
};

typedef union
{
    int action;
    struct msgParams m;
    struct nvParams n;
} ActionItem;

typedef struct
{
    int iterations;
    uint16 sleepTime;
    int count;
    ActionItem d;
} Action;

Action actionBuf;

void HandleActionTimer(void)
{
    ActionItem *action;
    int i;
    action = &actionBuf.d;
    for (i=0; i<actionBuf.count; i++)
    {
        int adjust;
        switch (action->action)
        {
            case 0:
                // Send out an explicit message.
                msg_out.priority_on = action->m.pri;
                msg_out.tag = action->m.tag;
                msg_out.service = action->m.svc;
                msg_out.code = action->m.code;
                if (action->m.len)
                {
                    memcpy(msg_out.data, action->m.data, action->m.len);
                }
                adjust = action->m.len + sizeof(struct msgParams)-1;
                msg_send();
                break;
            case 1:
            {
                // Do an NV update.  Use NV fixed table to get at address
                int16 index;
                index = action->n.index;
                memcpy(NV_ADDRESS(index), action->n.data, NV_LENGTH(index));
                PropagateNV(index);
                adjust = action->n.len + sizeof(struct nvParams)-1;
                break;
            }
        }
        action = (ActionItem *)(((char *)action) + adjust);
    }
    if (--actionBuf.iterations)
    {
        SetLonTimer(&actionTimer, actionBuf.sleepTime);
    }
}

void NVUpdateCompletes(Status status, int16 nvIndex, int16 nvArrayIndex)
{
    reportEvent(status==SUCCESS?NVSUCCESS:NVFAILURE, nvIndex+nvArrayIndex);
}

void NVUpdateOccurs(int16 nvIndex, int16 nvArrayIndex)
{
    reportEvent(NVUPDATE, nvIndex);
}

void MsgCompletes(Status stat, MsgTag tag)
{
    if (tag)
    {
        reportEvent(stat==SUCCESS?MSGSUCCESS:MSGFAILURE, tag);
    }
}

void HandleMsg(void)
{
    int code;
    code = msg_in.code;
    if (code == 0x40)
    {
        // Special directives to perform actions
        memcpy(&actionBuf, msg_in.data, msg_in.len);
        SetLonTimer(&actionTimer, 1);
    }
    else
    {
        int hdr;
        int len;
        len = msg_in.len;
        hdr = msg_in.authenticated | (msg_in.service<<1);
        memcpy(scratch, msg_in.data, msg_in.len);
        if (msg_in.service == REQUEST)
        {
            resp_out.code = code;
            if (len)
            {
                memcpy(resp_out.data, scratch, len);
            }
            resp_send();
        }
        reportEventData(MSGARRIVES, hdr, code, scratch, len);
    }
}

void HandleResp(void)
{
    int code;
    code = resp_in.code;
    memcpy(scratch, resp_in.data, resp_in.len);
    reportEventData(RESPARRIVES, 0, code, scratch, resp_in.len);
}

Status  AppInit(void)
{
   /* Register Network variables */
   AddNV(&firstnvoDef);
   AddNV(&secondnvoDef);
   AddNV(&thirdnvoDef);
   AddNV(&fourthnvoDef);
   AddNV(&arraynvoDef);
   AddNV(&firstnviDef);
   AddNV(&secondnviDef);
   AddNV(&thirdnviDef);
   AddNV(&fourthnviDef);
   AddNV(&arraynviDef);

   eventTag = NewMsgTag(BINDABLE);
   M1 = NewMsgTag(BINDABLE);
   M2 = NewMsgTag(BINDABLE);
   M3 = NewMsgTag(BINDABLE);
   M4 = NewMsgTag(BINDABLE);
   M5 = NewMsgTag(BINDABLE);
   M6 = NewMsgTag(BINDABLE);
	return SUCCESS;
}

void  AppReset(void)
{
}

void OfflineEvent()
{
}

void OnlineEvent()
{
}

void Wink(void)
{
    gp->ioOutputPin1 = 1;
    SetLonTimer(&lightTimer, 1000);
}

void DoApp(void)
{
    if (LonTimerExpired(&lightTimer))
    {
        gp->ioOutputPin1 = FALSE;
    }
    if (IOChanges(0))
    {
        if (++changes == 2) GoUnconfigured();
    }
    if (LonTimerExpired(&actionTimer))
    {
        HandleActionTimer();
    }
    if (msg_receive())
    {
        HandleMsg();
    }
    if (resp_receive())
    {
        HandleResp();
    }
}
