/*
 * lcs_custom.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Customization Constants and Global Data
 * Purpose: Provides constant definitions and a global data structure
 *          to customize the LON Stack for a specific device.
 * Notes:   This file can be modified to customize the LON Stack
 *          for a specific device.  The constants defined in this
 *          file can be changed to set default values for
 *          ReadOnlyData, ConfigData, Domain Table, and Address Table
 *          members.  The global data structure 'customDataGbl' can be
 *          used to hold any implementation-specific values.
 * 
 *          Some or all of these values will be overwritten by
 *          network management tools when this device is loaded.
 * 
 *          See ISO/IEC 14908-1 for details on the LON protocol parameters.
 */

#include "lcs/lcs_custom.h"

CustomData customDataGbl[NUM_STACKS];
CustomData *cp;
