/*
 * lcs_timer.h
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Timer Functions
 * Purpose: Provides interfaces for managing LON timers.
 * Notes:   Timers are used for various timing operations within the LON stack.
 */

#ifndef _TIMER_H
#define _TIMER_H

#include <stddef.h>
#include <stdbool.h>

#include "izot/IzotPlatform.h"
#include "izot/lon_types.h"
#include "lcs/lcs_eia709_1.h"

// Set LON timer maximum duration to 1/2 maximum full range for a 32-bit
// millisecond timer which is about 24 days
#define LON_TIMER_MAX_DURATION 0x7FFFFFFF

void SetLonTimer(LonTimer *timerOut, uint32_t initValueIn);
void SetLonRepeatTimer(LonTimer *timerOut, uint32_t initValueIn, uint32_t repeatValueIn);
bool LonTimerExpired(LonTimer *timerInOut); // Returns true once upon expiration
bool LonTimerRunning(LonTimer *timerInOut); // Returns true as long as timer is running
uint32_t LonTimerRemaining(LonTimer *timerInOut);

#endif // _TIMER_H