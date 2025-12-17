/*
 * lcs_queue.h
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

#ifndef _LCS_QUEUE_H
#define _LCS_QUEUE_H

#include <stdio.h>
#include <stddef.h>

#include "izot/IzotPlatform.h"
#include "izot/IzotApi.h"
#include "izot/lon_types.h"
#include "lcs/lcs_eia709_1.h"   /* To get Byte, Boolean & Status         */
#include "lcs/lcs_timer.h"      /* For LonTimer required by lcs_node.h   */

// Maximum static capacity for any RingBuffer instance. Can be overridden
// before including this header (e.g., via compiler flag) if larger bursts are needed.
#ifndef RING_BUFFER_MAX_CAPACITY
#define RING_BUFFER_MAX_CAPACITY 2048
#endif

typedef struct RingBuffer {
    size_t   size;      // active capacity in bytes (<= RING_BUFFER_MAX_CAPACITY)
    size_t   head;      // write position
    size_t   tail;      // read position
    size_t   count;     // bytes currently stored
    uint8_t  data[RING_BUFFER_MAX_CAPACITY]; // internal storage
} RingBuffer;

/*****************************************************************
 * Section: Queue Operations Function Declarations
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
LonStatusCode QueueInit(Queue *queue_out, size_t entry_size, size_t queue_capacity);

/* 
 * Returns the current size of a queue.
 * Parameters:
 *   queue_in: Pointer to the queue.
 * Returns:
 *   The current size (# of items) of the queue.
 */
size_t QueueSize(Queue *queue_in);

/* QueueCapacity returns the capacity (i.e max items) of the queue. */
size_t QueueCapacity(Queue *queue_in);

/* QueueEntrySize returns the size of each item in the queue. */
size_t QueueEntrySize(Queue *queue_in);

/* QueueFull returns TRUE if the queue is full and FALSE otherwise. */
IzotBool QueueFull(Queue *queue_in);

/* QueueEmpty returns TRUE if the queue is empty and FALSE else. */
IzotBool QueueEmpty(Queue *queue_in);

/* QueueDropHead removes an item (i.e advances head) from the queue. */
void QueueDropHead(Queue *queue_in_out);

/* Enqueue adds an item (i.e advances tail) to queue. */
void QueueWrite(Queue *queue_in_out);

/* QueuePeek returns the pointer to the head of the queue so that
   client can examine the queue's first item without actually
   removing it. If needed it can be removed with QueueDropHead. */
void *QueuePeek(Queue *queue_in);

/* QueueTail returns the pointer to the tail of the queue (i.e free
   space) so that a new item can be formed directly in the queue
   before calling QueueWrite. It is important to make sure that the
   Queue is not Full before filling an item. */
void *QueueTail(Queue *queue_in);

/*****************************************************************
 * Section: Ring Buffer Operations Function Declarations
 *****************************************************************/
/*
 * Initializes a ring buffer with the specified capacity.
 * Parameters:
 *   rb: Pointer to the ring buffer to initialize.
 *   capacity: Capacity of the ring buffer in bytes.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code if unsuccessful.
 */
LonStatusCode RingBufferInit(RingBuffer *rb, size_t capacity);

/*
 * Returns the available space in a ring buffer.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 * Returns:
 *   Number of bytes available for writing.
 */
size_t RingBufferAvail(const RingBuffer *rb);

/*
 * Returns the number of bytes currently stored in a ring buffer.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 * Returns:
 *   Number of bytes currently stored.
 */
size_t RingBufferSize(const RingBuffer *rb);

/*
 * Clears a ring buffer, removing all stored data.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 */
void RingBufferClear(RingBuffer *rb);

/*
 * Writes data to a ring buffer.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 *   src: Pointer to the source data buffer.
 *   len: Number of bytes to write.
 * Returns:
 *   Number of bytes actually written.
 */
size_t RingBufferWrite(RingBuffer *rb, const uint8_t *src, size_t len);

/*
 * Peeks data from a ring buffer without removing it.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 *   dst: Pointer to the destination data buffer.
 *   len: Number of bytes to peek.
 * Returns:
 *   Number of bytes actually peeked.
 */
size_t RingBufferPeek(const RingBuffer *rb, uint8_t *dst, size_t len);

/*
 * Reads data from the ring buffer, removing it.
 * Parameters:
 *   rb: Pointer to the ring buffer.
 *   dst: Pointer to the destination data buffer.
 *   len: Number of bytes to read.
 * Returns:
 *   Number of bytes actually read.
 */
size_t RingBufferRead(RingBuffer *rb, uint8_t *dst, size_t len);

#endif  // _LCS_QUEUE_H
