//
// IzotConfig.h
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

//
// Title: LON Stack configuration file
//
// Abstract:
// This file defines the product, platform, processor, link, and ISI IDs
// for a project.  This file can be customized for a project.  Add new
// IDs in the first part of the file and then select IDs for a project
// in the last part.  The default values for all IDs is 0.  See 
// EchelonStandardDefinitions.h for explanation of use.

#if !defined(DEFINED_IZOTCONFIG_H)
#define DEFINED_IZOTCONFIG_H
 

//
// Section: Macros
//

// Available IDs.  You can add new IDs to this list.  Don't use 0 for
// the value of new IDs--0 is reserved for the default value of each

// Product IDs -- default is unspecified
#define PRODUCT_ID_SLB 1

// Platform IDs -- default is unspecified
#define PLATFORM_ID_SIM		1  // Windows simulator
#define PLATFORM_ID_FRTOS   2  // FreeRTOS

// Processor IDs -- default is unspecified
#define PROCESSOR_ID_MC200  1   // Marvell MC200 ARM Cortex M3

// Link IDs -- default is LON/IP Ethernet
#define LINK_ID_WIFI   1	// LON/IP Wi-Fi data link
#define LINK_ID_MIP    2   	// Neuron MIP data link

// ISI IDs -- default is no ISI
#define ISI_ID_SIMPLE  1   // ISI-S
#define ISI_ID_DA      2   // ISI-DA (with domain address server)

// Encryption IDs -- default is no encryption
#define ENCRYPTION_ID_AES	1	// AES encryption

// Project-specific IDs -- to change any of these for a project,
// change the "0" to a macro name defined above with a matching
// prefix.
#define PRODUCT_ID 	  0
#define PLATFORM_ID   0
#define PROCESSOR_ID  0
#define LINK_ID 	  0
#define ISI_ID 		  0
#define ENCRYPTION_ID 0


#endif  /* defined(DEFINED_IZOTCONFIG_H) */
