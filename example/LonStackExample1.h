//
// LonStackExample1.h
//
// Copyright (C) 2023 EnOcean
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

// Title: LON Stack Example 1 Header File
//
// Abstract:
// Definitions for aimple example application for the LON Stack.

#include <stdlib.h>
#include <string.h>

#include "abstraction/IzotConfig.h"         // Project-specific configuration
#include "izot/IzotApi.h"            // LON Stack API definition

#ifdef  __cplusplus
extern "C" {
#endif

// Function prototypes.

extern IzotApiError SetUpExample1(void);
extern IzotApiError LoopExample1(void);

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


#ifdef  __cplusplus
}
#endif
