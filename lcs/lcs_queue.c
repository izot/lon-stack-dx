//
// lcs_queue.c
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
       Purpose:        Handle queue operations. See queue.h for
                       details of these operations.
*********************************************************************/

/*------------------------------------------------------------------------------
  Section: Includes
  ------------------------------------------------------------------------------*/
#include "lcs_queue.h"

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
  Section: Local Function Prototypes
  -------------------------------------------------------------------*/

/*-------------------------------------------------------------------
  Section: Function Definitions
  -------------------------------------------------------------------*/

/*****************************************************************
Function:  QueueSize
Returns:   The current size (# of items) of the queue.
Reference: None
Purpose:   To determine the number of items currently in queue.
Comments:  None
******************************************************************/
IzotUbits16 QueueSize(Queue *qInp)
{
    return(qInp->queueSize);
}

/*****************************************************************
Function:  QueueCnt
Returns:   The capacity of the queue.
Reference: None
Purpose:   To get the max # of items that can be stored in queue.
Comments:  None
******************************************************************/
IzotUbits16 QueueCnt(Queue *qInp)
{
    return(qInp->queueCnt);
}

/*****************************************************************
Function:  QueueItemSize
Returns:   The size of each item in the queue.
Reference: None
Purpose:   To get the size of items in the queue.
Comments:  None
******************************************************************/
IzotUbits16  QueueItemSize(Queue *qInp)
{
    return(qInp->itemSize);
}

/*****************************************************************
Function:  QueueFull
Returns:   TRUE if the queue is full, FALSE otherwise
Reference: None
Purpose:   To check whether the queue is full or not.
Comments:  None
******************************************************************/
IzotByte QueueFull(Queue *qInp)
{
    return(qInp->queueCnt == qInp->queueSize);
}

/*****************************************************************
Function:  QueueEmpty
Returns:   TRUE if the queue is empty, FALSE otherwise
Reference: None
Purpose:   To check whether a queue is empty or not.
Comments:  None
******************************************************************/
IzotByte QueueEmpty(Queue *qInp)
{
    return(qInp->queueSize == 0);
}

/*****************************************************************
Function:  DeQueue
Returns:   None
Reference: None
Purpose:   To remove an item from the queue.
Comments:  If the queue is empty, an error message is printed and
           nothing is done on the queue.
******************************************************************/
void DeQueue(Queue *qInOut)
{
    if (qInOut->queueSize == 0)
    {
        DBG_vPrintf(TRUE, "DeQueue: Queue is empty.\n");
        return;
    }
    qInOut->queueSize--;
    qInOut->head = qInOut->head + qInOut->itemSize;
    /* Wrap around if the ptr goes past the array */
    if (qInOut->head ==
            (qInOut->data + qInOut->itemSize * qInOut->queueCnt))
    {
        qInOut->head = qInOut->data;
    }
}

/*****************************************************************
Function:  EnQueue
Returns:   None
Reference: None
Purpose:   To add an item to the queue.
Comments:  The item added is directly placed in queue before calling
           this function. Thus, this function does not handle the item.
           If the queue is full, then an error message is printed
           and nothing is done.
******************************************************************/
void EnQueue(Queue *qInOut)
{
    if (qInOut->queueSize == qInOut->queueCnt)
    {
    	DBG_vPrintf(TRUE, "EnQueue: Queue is full.\n");
        return;
    }
    qInOut->queueSize++;
    qInOut->tail = qInOut->tail + qInOut->itemSize;
    /* Wrap around if the ptr goes past the array. */
    if (qInOut->tail ==
            (qInOut->data + qInOut->itemSize * qInOut->queueCnt))
    {
        qInOut->tail = qInOut->data;
    }

}

/*****************************************************************
Function:  QueueHead
Returns:   The ptr to the head of the queue.
Reference: None
Purpose:   To access the head of the queue directly from queue.
           Clients can examine the item directly from queue without
           removing it first.
Comments:  None
******************************************************************/
void *QueueHead(Queue *qInp)
{
    return(qInp->head);
}

/*****************************************************************
Function:  QueueTail
Returns:   The ptr to the head of the queue.
Reference: None
Purpose:   To access the tail of the queue. The returned pointer
           can be used by the client to form the new item
           in the queue directly. Client should make sure that the
           queue is not full before filling the new item.
Comments:  None
******************************************************************/
void *QueueTail(Queue *qInp)
{
    return(qInp->tail);
}

/*****************************************************************
Function:  QueueInit
Returns:   Status the operation: SUCCESS or FAILURE
Reference: None
Purpose:   To initialize the queue by allocating storage for data
           and recording the item size and cnt values (capacity).
Comments:  None
******************************************************************/
Status QueueInit(Queue *qOut, IzotUbits16 itemSizeIn, IzotUbits16 qCntIn)
{
    qOut->itemSize  = itemSizeIn;
    qOut->queueCnt  = qCntIn;

    qOut->data = AllocateStorage((IzotUbits16)(itemSizeIn * qCntIn));
    if (qOut->data == NULL)
    {
        return(FAILURE);
    }

    /* Initialize other fields */
    qOut->head      = qOut->data;
    qOut->tail      = qOut->data;
    qOut->queueSize = 0;

    return(SUCCESS);
}

/*************************End of queue.c***************************/
