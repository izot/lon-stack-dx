/*
 * IzotConfig.h
 *
 * Copyright (c) 2023-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Configuration File
 * Purpose: Defines product, platform, processor, link, self-
 *          installation (ISI), and security conditional test macros
 *          and project-specific IDs to use with the macros.
 * Notes:   You can customize this file for a project.  To add new IDs,
 *          add them to the LON Stack Configuration IDs section.  The 
 *          value 0 for each set is typically used as the default
 *          value for the set.  To override a default, add a #define
 *          for your project in the LON Stack Configuration Overrides
 *          section at the end of this file.
 *
 *          If you do not assign a PLATFORM_ID in this file,
 *          IzotPlatform.h may determine the platform ID based on
 *          pre-defined platform macros for different platforms.  To 
 *          enable automatic platform detection, remove the PLATFORM_ID
 *          assignment from the LON Stack Configuration Overrides
 *          section.  For example, if PLATFORM_ID is not defined 
 *          prior to including IzotPlatform.h, the platform will
 *          be set to PLATFORM_ID_LINUX64_ARM_GCC if the __aarch64__
 *          macro is defined, to PLATFORM_ID_RPI if the __ARMEL__
 *          macro is defined, or to PLATFORM_ID_FRTOS_ARM_EABI if the 
 *          __FREERTOS_ARM_EABI__ macro is defined.  If PLATFORM_ID is 
 *          defined, the platform detection code will not override it.
 * 
 *          The LON Stack uses the xxx_IS(yyy) macros to control conditional 
 *          compilation.  For example:
 * 
 *              #if LINK_IS(USB_MIP)
 *                  <USB-only code>
 *              #endif
 * 
 *          If the conditional code is in the middle of a function,
 *          you can use the macro as a real expression and let
 *          optimization remove unused code.  For example:
 * 
 *              if (LINK_IS(USB_MIP)) {
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
 *              #if ISI_IS(SIMPLE)
 *              #if ISI_IS(SIMPLE) || ISI_IS(DA)
 *              #if IUP_IS(V1)
 *              #if LINK_IS(SPI_MIP)
 *              #if LINK_IS(USB_MIP)
 *              #if LINK_IS(MULTIPLE_USB_MIPS)
 *              #if PHYSICAL_IS(WIFI)
 *              #if PROCESSOR_IS(ARM64)
 *              #if PROTOCOL_IS(LON_IPV4)
 *              #if SECURITY_IS(V2)
 */
#if !defined(_IZOT_CONFIG_H)
#define _IZOT_CONFIG_H
 
// Conditional test macros
#define PLATFORM_IS(platid) (PLATFORM_ID == PLATFORM_ID_ ## platid)
#define ISI_IS(isiid) (ISI_ID == ISI_ID_ ## isiid)
#define IUP_IS(iupid) (IUP_ID == IUP_ID_ ## iupid)
#define LINK_IS(linkid) (LINK_ID == LINK_ID_ ## linkid)
#define OS_IS(osid) (OS_ID == OS_ID_ ## osid)
#define PHYSICAL_IS(phyid) (PHYSICAL_ID == PHYSICAL_ID_ ## phyid)
#define PROCESSOR_IS(procid) (PROCESSOR_ID == PROCESSOR_ID_ ## procid)
#define PRODUCT_IS(prodid) (PRODUCT_ID == PRODUCT_ID_ ## prodid)
#define PROTOCOL_IS(protid) (PROTOCOL_ID == PROTOCOL_ID_ ## protid)
#define SECURITY_IS(secid) (SECURITY_ID == SECURITY_ID_ ## secid)
#define USB_SERVICE_IS(usbid) (USB_SERVICE_ID == USB_SERVICE_ID_ ## usbid)
#define USB_UPLINK_IS(uplinkid) (USB_UPLINK_ID == USB_UPLINK_ID_ ## uplinkid)

/*****************************************************************
 * Section: LON Stack Configuration IDs
 *****************************************************************/
// You can add new IDs to this list.  The value 0 is typically used
// for the default value of each.  Any undefined IDs are set in
// IzotPlatform.h.  To override a default IDs, set it in the next
// section.

// Physical Layer (PHY) IDs -- default is LON/FT
#define PHYSICAL_ID_LON_FT           0  // LON/FT
#define PHYSICAL_ID_LON_PL           1  // LON/PL (without proxy repeating)
#define PHYSICAL_ID_LON_PL_PROXY     2  // LON/PL with proxy repeating
#define PHYSICAL_ID_LON_RS_485       3  // LON/RS-485
#define PHYSICAL_ID_LON_TP_78        4  // LON/TP-78
#define PHYSICAL_ID_LON_TP_1250      5  // LON/TP-1250
#define PHYSICAL_ID_ETHERNET         6  // Ethernet
#define PHYSICAL_ID_WIFI             7  // Wi-Fi
#define PHYSICAL_ID_MIXED          100  // Multiple links with different types

// Platform IDs
#define PLATFORM_ID_LINUX64_ARM_GCC  0  // Linux 64 ARM (LINUX64_ARM_GCC)
#define PLATFORM_ID_RPI              1  // Raspberry Pi ARM (__ARMEL__)
#define PLATFORM_ID_ARM_EABI_GCC     2  // Generic ARM EABI GCC (ARM_NONE_EABI_GCC)
#define PLATFORM_ID_FRTOS_ARM_EABI   3  // FreeRTOS ARM EABI GCC
#define PLATFORM_ID_LINUX32_ARM_GCC  4  // Linux 32 ARM (LINUX32_GCC)
#define PLATFORM_ID_WIN32_X86        5  // Windows 32 x86 (WIN32)
#define PLATFORM_ID_SIM		         6  // Windows simulator --> WIN32_X86
#define PLATFORM_ID_IAR_ARM7         7  // IAR ARM7 (ARM7_IAR)
#define PLATFORM_ID_AVR_TINY13       8  // AVR Tiny-13 (AVR_TINY13)
#define PLATFORM_ID_HITECH           9  // Hi-Tech C (_HITECH)
#define PLATFORM_ID_COSMIC          10  // Cosmic C (_COSMIC)
#define PLATFORM_ID_NIOS2_LE        11  // Altera NIOS II GCC Little Endian (GCC_NIOS)
#define PLATFORM_ID_RPI_PICO        12  // Raspberry Pi Pico (RP2040) ARM (__ARM64__)

/// ISI IDs -- default is no LON Interoperable Self-Installation (ISI)
#define ISI_ID_NO_ISI                0  // ISI disabled
#define ISI_ID_SIMPLE                1  // ISI-S
#define ISI_ID_DA                    2  // ISI-DA (with domain address server)

// IUP IDs -- default is no LON Image Update Protocol (IUP)
#define IUP_ID_NO_IUP                0  // IUP disabled
#define IUP_ID_V1                    1  // IUP V1 enabled

// Link Layer IDs -- default is LON/IP Ethernet
#define LINK_ID_USB_MIP              0  // U60-compatible USB MIP data link
#define LINK_ID_MULTIPLE_USB_MIPS    1  // Multiple USB MIP data links
#define LINK_ID_SPI_MIP              2  // SPI MIP data link
#define LINK_ID_UDP                  3  // UDP data link

// Operating System IDs -- default is Linux
#define OS_ID_LINUX                  0  // Linux or POSIX-compliant OS
#define OS_ID_LINUX_KERNEL           1  // Linux Kernel
#define OS_ID_FREERTOS               2  // FreeRTOS
#define OS_ID_ZEPHYR                 3  // Zephyr
#define OS_ID_WINDOWS                4  // Windows
#define OS_ID_BARE_METAL             5  // No OS, bare metal
         
// Processor IDs -- default is ARM64
#define PROCESSOR_ID_ARM64           0  // 64-bit ARM
#define PROCESSOR_ID_ARM32           1  // 32-bit ARM
#define PROCESSOR_ID_ARM7            2  // 32-bit ARM7
#define PROCESSOR_ID_X64             3  // 64-bit x64
#define PROCESSOR_ID_X86             4  // 32-bit x86
#define PROCESSOR_ID_MC200           5  // Marvell MC200 ARM Cortex M3
#define PROCESSOR_ID_STM32           6  // STMicroelectronics STM32 ARM Cortex

// Product IDs -- default is unspecified
#define PRODUCT_ID_NA                0  // Product not specified
#define PRODUCT_ID_SLB               1  // Echelon Street Light Bridge

// Protocol IDs -- for application, session, transport, and network layers; default is LON native V0
#define PROTOCOL_ID_LON_NATIVE_V0    0  // LON native as defined by ISO/IEC 14908-1
#define PROTOCOL_ID_LON_NATIVE_V2    1  // LON native with expanded transaction records
#define PROTOCOL_ID_LON_IPV4         2  // LON/IP with IPv4 transport as defined by ISO/IEC 14908-1 + EN 14908-7
#define PROTOCOL_ID_LON_IPV6         3  // LON/IP with IPv6 transport as defined by ISO/IEC 14908-1 + EN 14908-7

// Security IDs -- default is LON Security V1 
#define SECURITY_ID_V1               0  // LON security V1 (authentication only)
#define SECURITY_ID_V2               1  // LON security V2 (AES encryption)

// USB Service Type IDs -- default is none
#define USB_SERVICE_ID_NA            0  // No USB service
#define USB_SERVICE_ID_PUMP          1  // USB service via LON Stack event pump function
#define USB_SERVICE_ID_THREAD        2  // USB service via separate thread

// USB Uplink Type IDs -- default is unspecified
#define USB_UPLINK_ID_NA             0  // USB uplink type not specified
#define USB_UPLINK_ID_POLLING        1  // Polled USB uplink
#define USB_UPLINK_ID_INTERRUPT      2  // Interrupt-driven USB uplink

/*****************************************************************
 * Section: LON Stack Configuration Overrides
 *****************************************************************/
// Overrides have been moved to CMake configuration

#endif  // defined(_IZOT_CONFIG_H) 
