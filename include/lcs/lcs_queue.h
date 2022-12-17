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
          File:        lcs_queue.h

       Version:        1

     Reference:        None

       Purpose:        To handle queue operations. A dynamic array
                       is used for circular implementation of queue.
                       The size of a queue item and the queue capacity
                       are chosen when the queue is created. It is
                       up to the user of the queue to interpret
                       the contents of the queue item.

          Note:        None

         To Do:        None
*********************************************************************/
#ifndef _LCS_QUEUE_H
#define _LCS_QUEUE_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include "lcs_eia709_1.h"

/*-------------------------------------------------------------------
Section: Constant Definitions
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Type Definitions
-------------------------------------------------------------------*/
typedef struct
{
    uint16 queueCnt;   /* Max number of items in queue. i.e capacity */
    uint16 queueSize;  /* Number of items currently in queue         */
    uint16 itemSize;   /* Number of bytes for each item in queue     */
    Byte *head;        /* Pointer to the head item of the queue      */
    Byte *tail;        /* Pointer to the tail item of the queue      */
    Byte *data;        /* Array of items -- Allocated during Init    */
} Queue;

/*-------------------------------------------------------------------
Section: Globals
-------------------------------------------------------------------*/
/* None */

/*-------------------------------------------------------------------
Section: Function Prototypes
-------------------------------------------------------------------*/
/* QueueSize returns the current size of the queue. */
uint16    QueueSize(Queue *qInp);

/* QueueCnt returns the capacity (i.e max items) of the queue. */
uint16    QueueCnt(Queue *qInp);

/* QueueItemSize returns the size of each item in the queue. */
uint16    QueueItemSize(Queue *qInp);

/* QueueFull returns TRUE if the queue is full and FALSE otherwise. */
Boolean   QueueFull(Queue *qInp);

/* QueueEmpty returns TRUE if the queue is empty and FALSE else. */
Boolean   QueueEmpty(Queue *qInp);

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
Status    QueueInit(Queue *qOut, uint16 itemSize, uint16 qCnt);

#endif  // _LCS_QUEUE_H
