//
// IzotCal.h
//
// Copyright (C) 2022 EnOcean
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

/*
 * Title: IP Connectivity Abstaction Layer header file
 *
 * Abstract:
 * This file contains the Izot Connectivity layer socket APIs.
 */ 

#include "IzotConfig.h"
#include "IzotPlatform.h"

#if !defined(DEFINED_IZOTCAL_H)
#define DEFINED_IZOTCAL_H


/*------------------------------------------------------------------------------
Section: Macros
------------------------------------------------------------------------------*/
#define TRUE             1
#define IPV4_ADDRESS_LEN 4

#ifdef CAL_DEBUG
	#define CAL_Printf(format,args...) wmprintf(args)
#else
	#define CAL_Printf(format,args...)	;
#endif

/*------------------------------------------------------------------------------
Section: Global
------------------------------------------------------------------------------*/
// IP addresse defined on the device 
extern IzotByte	ownIpAddress[IPV4_ADDRESS_LEN]; 
extern IzotBool is_connected;

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/
/*
 * Function: CalStart
 * Start the a WLAN connection for the FreeRTOS.
 */
extern int CalStart(void);

/*
 * Function: ReportNewIP
 *
 * Report a new IP address.
 */
extern void ReportNewIP(void);

/*
 * Function: InitSocket
 *
 * Open priority and non-priority sockets and add a MAC filter for
 * broadcast messages.
 */
extern int InitSocket(int port);

/*
 * Function: RemoveIPMembership
 *
 * Remove membership of given address in multicast group.
 */
extern void RemoveIPMembership(uint32_t addr);

/*
 * Function: AddIpMembership
 *
 * Add membership of the specified IP address in a multicast group.
 */
extern void AddIpMembership(uint32_t addr);

/*
 * Function: CalSend
 *
 * Send data on a UDP socket.
 */
extern void CalSend(
uint32_t port, IzotByte* addr, IzotByte* pData, uint16_t dataLength
);

/*
 * Function: CalReceive
 *
 * Receive data from a UDP socket.  Keep the DUP socket non-blockable.
 */
extern int CalReceive(IzotByte* pData, IzotByte* pSourceAddr);

extern void CheckNetworkStatus(void);

#endif  /* defined(DEFINED_IZOTCAL_H) */
