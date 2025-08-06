//
// lcs_queue.h
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
       Purpose:        Handle queue operations. Use a dynamic array
                       for circular implementation of a queue.
                       Choose the size of a queue item and the queue
                       capacity when the queue is created. It is
                       up to the user of the queue to interpret
                       the contents of the queue item.
*********************************************************************/

#ifndef _LCS_QUEUE_H
#define _LCS_QUEUE_H

/*------------------------------------------------------------------------------
  Section: Includes
  ------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stddef.h>
#include "abstraction/IzotConfig.h"
#include "izot/IzotApi.h"
#include "izot/IzotTypes.h"
#include "lcs/lcs_eia709_1.h"   /* To get Byte, Boolean & Status         */
#include "lcs/lcs_timer.h"      /* For LonTimer required by lcs_node.h   */
#include "lcs/lcs_node.h"       /* To get AllocateStorage.               */


/*-------------------------------------------------------------------
  Section: Function Prototypes
  -------------------------------------------------------------------*/
/* QueueSize returns the current size of the queue. */
IzotUbits16    QueueSize(Queue *qInp);

/* QueueCnt returns the capacity (i.e max items) of the queue. */
IzotUbits16    QueueCnt(Queue *qInp);

/* QueueItemSize returns the size of each item in the queue. */
IzotUbits16    QueueItemSize(Queue *qInp);

/* QueueFull returns TRUE if the queue is full and FALSE otherwise. */
IzotByte   QueueFull(Queue *qInp);

/* QueueEmpty returns TRUE if the queue is empty and FALSE else. */
IzotByte   QueueEmpty(Queue *qInp);

/* DeQueue removes an item (i.e advances head) from the queue. */
void      DeQueue(Queue *qInOut);

/* Enqueue adds an item (i.e advances tail) to queue. */
void      EnQueue(Queue *qInOut);

/* QueueHead returns the pointer to the head of the queue so that
   client can examine the queue's first item without actually
   removing it. If needed it can be removed with DeQueue. */
void     *QueueHead(Queue *qInp);

/* QueueTail returns the pointer to the tail of the queue (i.e free
   space) so that a new item can be formed directly in the queue
   before calling EnQueue. It is important to make sure that the
   Queue is not Full before filling an item. */
void     *QueueTail(Queue *qInp);

/* QueueInit is used to initialize a queue. Client specifies the
   size of each item in queue and the count (capacity) of queue. */
Status    QueueInit(Queue *qOut, IzotUbits16 itemSize, IzotUbits16 qCnt);

#endif  // _LCS_QUEUE_H
