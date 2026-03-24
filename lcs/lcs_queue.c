/*
 * lcs_queue.c
 *
 * Copyright (c) 2022-2026 EnOcean
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
 *   queue_name: Optional name of the queue (for debugging)
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
LonStatusCode QueueInit(Queue *queue_out, char *queue_name, size_t entry_size, size_t queue_capacity)
{
    // Allocate memory for the queue name and data
    if (queue_name != NULL) {
        queue_out->queueName = OsalAllocateMemory((size_t)(strlen(queue_name) + 1));
        strcpy(queue_out->queueName, queue_name);
    } else {
        queue_out->queueName = NULL;
    }
    queue_out->data = OsalAllocateMemory((size_t)(entry_size * queue_capacity));
    if (queue_out->data == NULL) {
        OsalPrintLog(ERROR_LOG, LonStatusNoMemoryAvailable, "QueueInit: Memory allocation failed");
        return(LonStatusNoMemoryAvailable);
    }

    // Initialize other fields
    queue_out->entrySize = entry_size;
    queue_out->queueCapacity = queue_capacity;
    queue_out->queueEntries = 0;
    queue_out->head = queue_out->data;
    queue_out->tail = queue_out->data;
    queue_out->headIndex = 0;
    queue_out->tailIndex = 0;
    queue_out->emptyCountReports = 0;
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
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueCapacity: Invalid queue");
        return 0;
    }
    return(queue_in->queueCapacity);
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
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueAvail: Invalid queue");
        return 0;
    }
    return(queue_in->queueCapacity - queue_in->queueEntries);
}

/* 
 * Returns the current size (# of entries) of a queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   The current size (# of entries) of the queue.
 */
size_t QueueEntries(Queue *queue_in)
{
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueEntries: Invalid queue");
        return 0;
    }
    return(queue_in->queueEntries);
}

/*
 * Returns TRUE if the specified queue is full or invalid.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   TRUE if the queue is full or invalid, FALSE otherwise.
 */
IzotBool QueueFull(Queue *queue_in)
{
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueFull: Invalid queue");
        return TRUE;
    }
    return(queue_in->queueEntries >= queue_in->queueCapacity);
}

/*
 * Returns TRUE if the specified queue is valid and empty.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   TRUE if the queue is valid and empty, FALSE otherwise.   
 */
IzotBool QueueEmpty(Queue *queue_in)
{
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueEmpty: Invalid queue");
        return FALSE;
    }
    return(queue_in->queueEntries == 0);
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
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueEntrySize: Invalid queue");
        return 0;
    }
    return(queue_in->entrySize);
}

/*
 * Removes an entry from the head of the specified queue.
 * Parameters:
 *   queue_in_out: Pointer to the queue.
 * Returns:
 *   None.
 * Notes:
 *   An error message is logged and nothing is done if the queue is invalid or empty.
 */
void QueueDropHead(Queue *queue_in_out)
{
    if ((queue_in_out == NULL) || (queue_in_out->data == NULL)
            || (queue_in_out->head == NULL) || (queue_in_out->tail == NULL)
            || (queue_in_out->queueCapacity == 0) || (queue_in_out->entrySize == 0)
            || (queue_in_out->queueEntries > queue_in_out->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueDropHead: Invalid queue");
        return;
    }
    if (queue_in_out->queueEntries == 0) {
        if (queue_in_out->emptyCountReports < 20) {
            OsalPrintLog(PACKET_TRACE_LOG, LonStatusNoError, "QueueDropHead: %s queue is empty", queue_in_out->queueName);
            queue_in_out->emptyCountReports++;
        }
        return;
    }
    queue_in_out->queueEntries--;
    queue_in_out->head = queue_in_out->head + queue_in_out->entrySize;
    queue_in_out->headIndex++;
    // Wrap around if the head pointer goes past the end of the array
    if (queue_in_out->head ==
            (queue_in_out->data + queue_in_out->entrySize * queue_in_out->queueCapacity)) {
        queue_in_out->head = queue_in_out->data;
        queue_in_out->headIndex = 0;
    }
    OsalPrintLog(PACKET_TRACE_LOG, LonStatusNoError, 
            "QueueDropHead: Dropped entry from %s queue, new size %d, head index %d (%p), tail index %d (%p)",
            queue_in_out->queueName, queue_in_out->queueEntries,
            queue_in_out->headIndex, queue_in_out->head,
            queue_in_out->tailIndex, queue_in_out->tail);
}

/*
 * Flushes all entries in the specified queue.
 * Parameters:
 *   queue_in_out: Pointer to the queue.
 * Returns:
 *   None.
 * Notes:
 *   An error message is logged and nothing is done if no queue is provided
 *   or if the queue is invalid.
 */
void QueueFlush(Queue *queue_in_out)
{
    if ((queue_in_out == NULL) || (queue_in_out->data == NULL)
            || (queue_in_out->head == NULL) || (queue_in_out->tail == NULL)
            || (queue_in_out->queueCapacity == 0) || (queue_in_out->entrySize == 0)
            || (queue_in_out->queueEntries > queue_in_out->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueFlush: Invalid queue");
        return;
    }
    queue_in_out->head = queue_in_out->data;
    queue_in_out->tail = queue_in_out->data;
    queue_in_out->headIndex = 0;
    queue_in_out->tailIndex = 0;
    queue_in_out->queueEntries = 0;
    queue_in_out->emptyCountReports = 0;
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
    if ((queue_in_out == NULL) || (queue_in_out->data == NULL)
            || (queue_in_out->head == NULL) || (queue_in_out->tail == NULL)
            || (queue_in_out->queueCapacity == 0) || (queue_in_out->entrySize == 0)
            || (queue_in_out->queueEntries > queue_in_out->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueWrite: Invalid queue");
        return;
    }
    if (queue_in_out->queueEntries >= (queue_in_out->queueCapacity - 1)) {
    	OsalPrintLog(ERROR_LOG, LonStatusNoBufferAvailable, "QueueWrite: Queue is full");
        return;
    }
    uint8_t *new_entry = queue_in_out->tail;
    queue_in_out->queueEntries++;
    // Increment queue tail to next entry
    queue_in_out->tail = queue_in_out->tail + queue_in_out->entrySize;
    queue_in_out->tailIndex++;
    // Wrap around if the tail pointer goes past the end of the array
    if (queue_in_out->tail ==
            (queue_in_out->data + queue_in_out->entrySize * queue_in_out->queueCapacity)) {
        queue_in_out->tail = queue_in_out->data;
        queue_in_out->tailIndex = 0;
    }
    OsalPrintLog(PACKET_TRACE_LOG, LonStatusNoError, 
            "QueueWrite: Wrote entry to %s queue, new entry count %d, head index %d (%p), tail index %d (%p)",
            queue_in_out->queueName, queue_in_out->queueEntries, queue_in_out->headIndex, queue_in_out->head,
            queue_in_out->tailIndex, queue_in_out->tail);
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
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueuePeek: Invalid queue");
        return NULL;
    }
    void *head;
    head = queue_in->queueEntries ? queue_in->head : NULL;
    if (head) {
        OsalPrintLog(PACKET_TRACE_LOG, LonStatusNoError, 
            "QueuePeek: Peeked entry from %s queue, size %d, head index %d (%p), tail index %d (%p)",
            queue_in->queueName, queue_in->queueEntries, queue_in->headIndex,
            queue_in->head, queue_in->tailIndex, queue_in->tail);
    }
    return(head);
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
    if ((queue_in == NULL) || (queue_in->data == NULL)
            || (queue_in->head == NULL) || (queue_in->tail == NULL)
            || (queue_in->queueCapacity == 0) || (queue_in->entrySize == 0)
            || (queue_in->queueEntries > queue_in->queueCapacity)) {
        OsalPrintLog(ERROR_LOG, LonStatusInvalidParameter, "QueueTail: Invalid queue");
        return NULL;
    }
    return((queue_in->queueEntries < queue_in->queueCapacity) ? queue_in->tail : NULL);
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
