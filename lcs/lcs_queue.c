/*
 * lcs_queue.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Queue and Ring Buffer Operations
 * Purpose: Defines functions to handle queue and ring buffer operations.
 * Notes:   This module provides a simple queue and ring buffer implementation
 *          using dynamic memory allocation. Queues are used for storing items
 *          of fixed size, while ring buffers are used for byte streams.
 *          Both data structures support basic operations such as
 *          initialization, adding/removing items, and querying size/capacity.
 *          For queues, the user must specify the size of each item and the
 *          total capacity when initializing.  For ring buffers, the user 
 *          specifies the total byte capacity.  The data storage for both 
 *          structures is allocated within the initialization functions.
 */

#include "lcs/lcs_queue.h"

/*****************************************************************
 * Section: Queue Function Definitions
 *****************************************************************/
/*
 * Initializes a queue with the specified capacity and entry size.
 * Parameters:
 *   queue_out: Pointer to the queue to initialize.
 *   entry_size: Size of each entry in the queue in bytes.
 *   queue_capacity: Capacity of the queue in entries.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 * Notes:
 *  The data storage for the queue is allocated within this function.
 *  After allocating the storage, the head and tail pointers are
 *  initialized to point to the start of the data storage, the
 *  queue size is initialized to zero, and the entry size is stored.
 */
LonStatusCode QueueInit(Queue *queue_out, size_t entry_size, size_t queue_capacity)
{
    queue_out->itemSize  = entry_size;
    queue_out->queueCnt  = queue_capacity;

    queue_out->data = OsalAllocateMemory((size_t)(entry_size * queue_capacity));

    if (queue_out->data == NULL) {
        return(LonStatusNoMemoryAvailable);
    }

    // Initialize other fields
    queue_out->head      = queue_out->data;
    queue_out->tail      = queue_out->data;
    queue_out->queueSize = 0;

    return(LonStatusNoError);
}

/*
 * Returns the capacity (max items) of a queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   The capacity (max # of items) of the queue.
 */
size_t QueueCapacity(Queue *queue_in)
{
    return(queue_in->queueCnt);
}

/*
 * Returns the available space in a queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   Number of queue entries available for writing.
 */
size_t QueueAvail(Queue *queue_in)
{
    return(queue_in->queueCnt - queue_in->queueSize);
}

/* 
 * Returns the current size (# of items) of a queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   The current size (# of items) of the queue.
 */
size_t QueueSize(Queue *queue_in)
{
    return(queue_in->queueSize);
}

/*
 * Returns TRUE if the specified queue is full, FALSE otherwise.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   TRUE if the queue is full, FALSE otherwise.
 */
IzotBool QueueFull(Queue *queue_in)
{
    return(queue_in->queueCnt == queue_in->queueSize);
}

/*
 * Returns TRUE if the specified queue is empty, FALSE otherwise.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   TRUE if the queue is empty, FALSE otherwise.   
 */
IzotBool QueueEmpty(Queue *queue_in)
{
    return(queue_in->queueSize == 0);
}

/*
 * Returns the size of each entry in a queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   The size of each entry in the queue.
 */
size_t QueueEntrySize(Queue *queue_in)
{
    return(queue_in->itemSize);
}

/*
 * Removes an entry from the head of the specified queue.
 * Parameters:
 *   queue_in_out: Pointer to the queue.
 * Returns:
 *   None.
 * Notes:
 *   An error message is logged and nothing is done if the queue is empty.
 */
void QueueDropHead(Queue *queue_in_out)
{
    if (queue_in_out->queueSize == 0) {
        OsalPrintDebug(LonStatusNoError, "QueueDropHead: Queue is empty");
        return;
    }
    queue_in_out->queueSize--;
    queue_in_out->head = queue_in_out->head + queue_in_out->itemSize;
    // Wrap around if the ptr goes past the array
    if (queue_in_out->head ==
            (queue_in_out->data + queue_in_out->itemSize * queue_in_out->queueCnt)) {
        queue_in_out->head = queue_in_out->data;
    }
}

/*
 *  Adds an entry to the specified queue.
 *  Parameters:
 *    queue_in_out: Pointer to the queue.
 *  Returns:
 *    None.
 *  Notes:
 *    The entry to be added must already be placed in the queue entry
 *    before calling this function.  The entry is written to the tail
 *    of the queue.  If the queue is full, no entry is added and an 
 *    error message is logged.
 */
void QueueWrite(Queue *queue_in_out)
{
    if (queue_in_out->queueSize == queue_in_out->queueCnt) {
    	OsalPrintError(LonStatusNoBufferAvailable, "QueueWrite: Queue is full");
        return;
    }
    queue_in_out->queueSize++;
    queue_in_out->tail = queue_in_out->tail + queue_in_out->itemSize;
    // Wrap around if the ptr goes past the array
    if (queue_in_out->tail ==
            (queue_in_out->data + queue_in_out->itemSize * queue_in_out->queueCnt)) {
        queue_in_out->tail = queue_in_out->data;
    }

}

/*
 * Returns a pointer to the head (next out) of the specified queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   A pointer to the head of the queue is returned without
 *   modifying the queue.
 */
void *QueuePeek(Queue *queue_in)
{
    return(queue_in->head);
}

/*
 * Returns a pointer to the tail (next in) of the specified queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   A pointer to the tail (next in) of the queue is returned without
 *   modifying the queue.
 */
void *QueueTail(Queue *queue_in)
{
    return(queue_in->tail);
}

/*****************************************************************
 * Section: Ring Buffer Function Definitions
 *****************************************************************/
/*
 * Initializes a ring buffer with the specified capacity.
 * Parameters:
 *   rb: Pointer to the ring buffer to initialize.
 *   capacity: Capacity of the ring buffer in bytes.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode RingBufferInit(RingBuffer *rb, size_t capacity)
{
    if (!rb || capacity == 0) return LonStatusInvalidParameter;
    if (capacity > RING_BUFFER_MAX_CAPACITY) return LonStatusInvalidParameter;
    rb->size = capacity;
    rb->head = rb->tail = rb->count = 0;
    return LonStatusNoError;
}

/*
 * Returns the available space in a ring buffer.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 * Returns:
 *   Number of free bytes available for writing.
 */
size_t RingBufferAvail(const RingBuffer *rb)
{
    return rb ? (rb->size - rb->count) : 0;
}

/*
 * Returns the number of bytes currently stored in a ring buffer.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 * Returns:
 *   Number of bytes currently stored.
 */
size_t RingBufferSize(const RingBuffer *rb)
{
    return rb ? rb->count : 0;
}

/*
 * Clears a ring buffer, resetting head, tail, and count.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 * Returns:
 *   None
 */
void RingBufferClear(RingBuffer *rb)
{
    if (rb) {
        rb->head = rb->tail = rb->count = 0;
    }
}

/*
 * Writes data to a ring buffer.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 *   src: Pointer to the source data buffer.
 *   len: Number of bytes to write.
 * Returns:
 *   Number of bytes actually written.
 */
size_t RingBufferWrite(RingBuffer *rb, const uint8_t *src, size_t len)
{
    if (!rb || !src || len == 0) return 0;
    size_t space = RingBufferAvail(rb);
    size_t to_write = (len > space) ? space : len;
    size_t written = 0;
    while (written < to_write) {
        size_t space_end = rb->size - rb->head;
        size_t chunk = to_write - written;
        if (chunk > space_end) chunk = space_end;
        memcpy(&rb->data[rb->head], src + written, chunk);
        rb->head = (rb->head + chunk) % rb->size;
        rb->count += chunk;
        written += chunk;
    }
    return written;
}

/*
 * Peeks data from a ring buffer without removing it.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 *   dst: Pointer to the destination data buffer.
 *   len: Number of bytes to peek.
 * Returns:
 *   Number of bytes actually peeked.
 */
size_t RingBufferPeek(const RingBuffer *rb, uint8_t *dst, size_t len)
{
    if (!rb || !dst || len == 0) return 0;
    size_t to_read = (len > rb->count) ? rb->count : len;
    size_t read = 0;
    size_t tail = rb->tail;
    while (read < to_read) {
        size_t avail_end = rb->size - tail;
        size_t chunk = to_read - read;
        if (chunk > avail_end) chunk = avail_end;
        memcpy(dst + read, &rb->data[tail], chunk);
        tail = (tail + chunk) % rb->size;
        read += chunk;
    }
    return read;
}

/*
 * Reads data from the ring buffer, removing it.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 *   dst: Pointer to the destination data buffer.
 *   len: Number of bytes to read.
 * Returns:
 *   Number of bytes actually read.
 */
size_t RingBufferRead(RingBuffer *rb, uint8_t *dst, size_t len)
{
    if (!rb || !dst || len == 0) return 0;
    size_t got = RingBufferPeek(rb, dst, len);
    rb->tail = (rb->tail + got) % rb->size;
    rb->count -= got;
    return got;
}
