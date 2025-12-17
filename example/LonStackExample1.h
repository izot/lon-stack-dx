/*
 * LonStackExample1.c
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Example 1
 * Purpose: Provides a simple example application for the LON Stack.
 */

#include <stdlib.h>
#include <string.h>

#include "izot/IzotPlatform.h"       // Project-specific configuration
#include "izot/IzotApi.h"            // LON Stack API definition

#ifdef  __cplusplus
extern "C" {
#endif

// Domain definition.  
#define EXAMPLE_DOMAIN_LENGTH 1
#define EXAMPLE_DOMAIN_ID 0x51
#define EXAMPLE_SUBNET 0x23
#define EXAMPLE_NODE 12

// Example target device definition.  The target device is for a self-installed
// connection to a second device running the same application.  The target device
// must be in the same domain, and must have a different subnet and/or node ID.
// When building the application image for the second device, assign the
// EXAMPLE_TARGET_SUBNET and EXAMPLE_TARGET_NODE values for the first device
// to the EXAMPLE_SUBNET and EXAMPLE_NODE values of the second device, and
// change the EXAMPLE_TARGET_SUBNET and EXAMPLE_TARGET_NODE values for the
// second device.
#define EXAMPLE_TARGET_SUBNET 0x23
#define EXAMPLE_TARGET_NODE 15

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
extern LonStatusCode SetUpExample1(void);
extern LonStatusCode LoopExample1(void);

#ifdef  __cplusplus
}
#endif
