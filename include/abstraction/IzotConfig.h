/*
 * IzotConfig.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Configuration File
 * Purpose: Defines product, platform, processor, link, self-
 *          installation (ISI), and security conditional test macros
 *          and project-specific IDs to use with the macros.
 * Notes:   You can customize this file for a project.  To add new IDs,
 *          add them after "Available IDs".  To select an ID for a
 *          project, add a #define for your ID after including
 *          IzotConfig.h and prior to including IzotPlatform.h.
 *          IzotConfig.h. If a value is not defined for PLATFORM_ID, 
 *          IzotPlatform.h may determine the platform ID based on
 *          pre-defined platform macros for different platforms.  To 
 *          enable automatic platform detection, undefine PLATFORM_ID
 *          after including IzotConfig.h and prior to including
 *          IzotPlatform.h.  For example, if PLATFORM_ID is undefined, 
 *          prior to including IzotPlatform.h, the platform will
 *          be set to PLATFORM_ID_LINUX64_ARM_GCC if the __aarch64__
 *          macro is defined, to PLATFORM_ID_RPI if the __ARMEL__
 *          macro is defined, or to PLATFORM_ID_FRTOS_ARM_EABI if the 
 *          __FREERTOS_ARM_EABI__ macro is defined.  If PLATFORM_ID is 
 *          defined, the platform detection code will not override it.
 * 
 *          For example, to define the platform as 64-bit Linux on ARM
 *          using the GCC compiler, the link as a USB interface, and
 *          the protocol as LON native, add the following three
 *          definitions after including IzotConfig.h and prior to 
 *          including IzotPlatform.h:
 * 
 *              #define PLATFORM_ID PLATFORM_ID PLATFORM_ID_LINUX64_ARM_GCC
 *              #define LINK_ID LINK_ID_USB
 *              #define PROTOCOL_ID PROTOCOL_ID_LON_NATIVE
 * 
 *          The LON Stack uses the xxx_IS(yyy) macros to control conditional 
 *          compilation.  For example:
 * 
 *              #if LINK_IS(USB)
 *                  <USB-only code>
 *              #endif
 * 
 *          If the conditional code is in the middle of a function,
 *          you can use the macro as a real expression and let
 *          optimization remove unused code.  For example:
 * 
 *              if (LINK_IS(USB)) {
 *                  <USB-only code>
 *              }
 * 
 *          This technique will only work if all the options are 
 *          linkable when they are not optimized away.  The IDs
 *          are in the form xxx_ID_yyy N.  For example:
 * 
 *              #define LINK_ID_USB 3
 * 
 *          Here are a few example conditional tests:
 * 
 *              #if PLATFORM_IS(LINUX64_ARM_GCC)
 *              #if PLATFORM_IS(RPI)
 *              #if PLATFORM_IS(RPI_PICO)
 *              #if DEBUG_IS(LCD)
 *              #if DEBUG_IS(SERIAL)
 *              #if ISI_IS(SIMPLE)
 *              #if ISI_IS(SIMPLE) || ISI_IS(DA)
 *              #if IUP_IS(V1)
 *              #if LINK_IS(MIP)
 *              #if LINK_IS(USB)
 *              #if LINK_IS(WIFI)
 *              #if PROCESSOR_IS(ARM64)
 *              #if PROTOCOL_IS(LON_IP)
 *              #if SECURITY_IS(V2)
 */
#if !defined(_IZOT_CONFIG_H)
#define _IZOT_CONFIG_H
 
// Conditional test macros
#define PLATFORM_IS(platid) (PLATFORM_ID == PLATFORM_ID_ ## platid)
#define DEBUG_IS(dbgid) (DEBUG_ID == DEBUG_ID_ ##dbgid)
#define ISI_IS(isiid) (ISI_ID == ISI_ID_ ## isiid)
#define IUP_IS(iupid) (IUP_ID == IUP_ID_ ## iupid)
#define LINK_IS(linkid) (LINK_ID == LINK_ID_ ## linkid)
#define OS_IS(osid) (OS_ID == OS_ID_ ## osid)
#define PROCESSOR_IS(procid) (PROCESSOR_ID == PROCESSOR_ID_ ## procid)
#define PRODUCT_IS(prodid) (PRODUCT_ID == PRODUCT_ID_ ## prodid)
#define PROTOCOL_IS(protid) (PROTOCOL_ID == PROTOCOL_ID_ ## protid)
#define SECURITY_IS(secid) (SECURITY_ID == SECURITY_ID_ ## secid)

// Available IDs.  You can add new IDs to this list.  The value 0 is
// typically used for the default value of each.  See default
// assignments following this list.

// Platform IDs -- default is 64-bit Linux ARM GCC
#define PLATFORM_ID_LINUX64_ARM_GCC  0  // Linux 64 ARM (LINUX64_ARM_GCC)
#define PLATFORM_ID_RPI              1  // Raspberry Pi ARM (__ARMEL__)
#define PLATFORM_ID_ARM_EABI_GCC     2  // Generic ARM EABI GCC (ARM_NONE_EABI_GCC)
#define PLATFORM_ID_FRTOS_ARM_EABI   3  // FreeRTOS ARM EABI GCC
#define PLATFORM_ID_LINUX32_ARM_GCC  4  // Linux 32 ARM (LINUX32_GCC)
#define PLATFORM_ID_WIN32_X86        5  // Windows 32 x86 (WIN32)
#define PLATFORM_ID_SIM		         5  // Windows simulator --> WIN32_X86
#define PLATFORM_ID_IAR_ARM7         6  // IAR ARM7 (ARM7_IAR)
#define PLATFORM_ID_AVR_TINY13       7  // AVR Tiny-13 (AVR_TINY13)
#define PLATFORM_ID_HITECH           8  // Hi-Tech C (_HITECH)
#define PLATFORM_ID_COSMIC           9  // Cosmic C (_COSMIC)
#define PLATFORM_ID_NIOS2_LE        10  // Altera NIOS II GCC Little Endian (GCC_NIOS)
#define PLATFORM_ID_RPI_PICO        11  // Raspberry Pi Pico (RP2040) ARM (__ARM64__)

// Debug IDs -- default is no debug output
#define DEBUG_ID_NONE           0   // Debug output disabled
#define DEBUG_ID_SERIAL         1   // Debug output to serial console
#define DEBUG_ID_LCD            2   // Debug output to LCD

// ISI IDs -- default is no LON Interoperable Self-Installation (ISI)
#define ISI_ID_NO_ISI           0   // ISI disabled
#define ISI_ID_SIMPLE           1   // ISI-S
#define ISI_ID_DA               2   // ISI-DA (with domain address server)

// IUP IDs -- default is no LON Image Update Protocol (IUP)
#define IUP_ID_NO_IUP           0   // IUP disabled
#define IUP_ID_V1               1   // IUP V1 enabled

// Link IDs -- default is LON/IP Ethernet
#define LINK_ID_ETHERNET        0   // LON/IP Ethernet data link
#define LINK_ID_WIFI            1   // LON/IP Wi-Fi data link
#define LINK_ID_MIP             2   // Neuron MIP data link
#define LINK_ID_USB             3   // U60 FT-compatible USB data link

// Operating System IDs -- default is Linux
#define OS_ID_LINUX             0   // Linux or POSIX-compliant OS
#define OS_ID_FREERTOS          1   // FreeRTOS
#define OS_ID_WINDOWS           2   // Windows
#define OS_ID_BARE_METAL        3   // No OS, bare metal

// Processor IDs -- default is ARM64
#define PROCESSOR_ID_ARM64      0   // 64-bit ARM
#define PROCESSOR_ID_ARM32      1   // 32-bit ARM
#define PROCESSOR_ID_ARM7       2   // 32-bit ARM7
#define PROCESSOR_ID_X64        3   // 64-bit x64
#define PROCESSOR_ID_X86        4   // 32-bit x86
#define PROCESSOR_ID_MC200      5   // Marvell MC200 ARM Cortex M3

// Product IDs -- default is unspecified
#define PRODUCT_ID_NA           0   // Product not specified
#define PRODUCT_ID_SLB          1   // Echelon Street Light Bridge

// Protocol IDs -- default is LON native
#define PROTOCOL_ID_LON_IP      0   // LON/IP as defined by ISO/IEC 14908-1 + EN 14908-7
#define PROTOCOL_ID_LON_NATIVE  1   // LON native as defined by ISO/IEC 14908-1
                                    
// Security IDs -- default is LON Security V1 
#define SECURITY_ID_V1          0   // LON Security V1 (authentication only)
#define SECURITY_ID_V2          1   // LON Security V2 (AES encryption)

// Default IDs -- to change any of these for a project, add a #define
// your ID prior to including IzotConfig.h.
#if !defined(PLATFORM_ID)
#define PLATFORM_ID   PLATFORM_ID_LINUX64_ARM_GCC
#endif  // !defined(PLATFORM_ID)

#if !defined(DEBUG_ID)
#define DEBUG_ID      DEBUG_ID_NONE
#endif  // !defined(DEBUG_ID)

#if !defined(ISI_ID)
#define ISI_ID 		  ISI_ID_NO_ISI
#endif  // !defined(ISI_ID)

#if !defined(IUP_ID)
#define IUP_ID        IUP_ID_NO_IUP
#endif  // !defined(IUP_ID)

#if !defined(LINK_ID)
#define LINK_ID       LINK_ID_ETHERNET
#endif  // !defined(LINK_ID)

#if !defined(OS_ID)
#define OS_ID         OS_ID_LINUX
#endif  // !defined(OS_ID)

#if !defined(PROCESSOR_ID)
#define PROCESSOR_ID  PROCESSOR_ID_ARM64
#endif  // !defined(PROCESSOR_ID)

#if !defined(PRODUCT_ID)
#define PRODUCT_ID 	  PRODUCT_ID_NA
#endif  // !defined(PRODUCT_ID)

#if !defined(PROTOCOL_ID)
#define PROTOCOL_ID   PROTOCOL_ID_LON_IP
#endif  // !defined(PROTOCOL_ID)

#if !defined(SECURITY_ID)
#define SECURITY_ID   SECURITY_ID_V1
#endif  // !defined(SECURITY_ID)

#endif  // defined(_IZOT_CONFIG_H) 
