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
// This file defines product, platform, processor, link, ISI, and security
// conditional test macros and project-specific IDs to use with the macros.
// You can customize this file for a project.  To add new IDs, add them
// after "Available IDs".  To select an ID for a project, add the appropriate
// macro name after "Project-specific IDs".  The default values for all IDs is 0. 
// For example, the default data link is LINK_ID 0 which is LON/IP over Ethernet.
// To change the data link to LON/IP over Wi-Fi, change the LINK_ID definition
// under "Project-specific IDs" to the following:
//
// #define LINK_ID LINK_ID_WIFI
//
// Use the xxx_IS(yyy) macros to control conditional compilation.
// For example:
//
// #if PRODUCT_IS(XYZ)
// 		XYZ-only code
// #endif
//
// If the conditional code is in the middle of a function,
// you can use the macro as a real expression and let
// and let optimization remove unused code.  For example:
//
// if (PRODUCT_IS(XYZ)) {
// 		XYZ-only code
// }
//
// This technique will only work if all the options are linkable when they are not optimized away.
//
// The IDs are in the form xxx_ID_yyy N.  For example:
//
// #define PRODUCT_ID_SLB 1
//
// Here are a few example conditional tests:
//
// #if PRODUCT_IS(SLB)
// #if PROCESSOR_IS(MC200)
// #if LINK_IS(WIFI)
// #if LINK_IS(MIP)
// #if ISI_IS(SIMPLE)
// #if ISI_IS(SIMPLE) || ISI_IS(DA)
// #if SECURITY_IS(V2)

#if !defined(DEFINED_IZOTCONFIG_H)
#define DEFINED_IZOTCONFIG_H
 

//
// Section: Macros
//

// Conditional test macros.

#define PRODUCT_IS(prodid) (PRODUCT_ID == PRODUCT_ID_ ## prodid)
#define PLATFORM_IS(platid) (PLATFORM_ID == PLATFORM_ID_ ## platid)
#define PROCESSOR_IS(procid) (PROCESSOR_ID == PROCESSOR_ID_ ## procid)
#define LINK_IS(linkid) (LINK_ID == LINK_ID_ ## linkid)
#define ISI_IS(isiid) (ISI_ID == ISI_ID_ ## isiid)
#define SECURITY_IS(secid) (SEC_ID == SEC_ID ## secid)
#define ENCRYPTION_IS(encid) (ENC_ID == ENC_ID ## encid)


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

// Security IDs -- default is LON Security V1 (authentication only)
#define SECURITY_ID_V2 1	// LON Security V2 (AES encryption)


// Project-specific IDs -- to change any of these for a project,
// change the "0" to a macro name defined above with a matching
// prefix.
#define PRODUCT_ID 	  0
#define PLATFORM_ID   0
#define PROCESSOR_ID  0
#define LINK_ID 	  0
#define ISI_ID 		  0
#define SECURITY_ID   0



#endif  /* defined(DEFINED_IZOTCONFIG_H) */
