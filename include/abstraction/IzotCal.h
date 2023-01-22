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
#if !defined(DEFINED_IZOTCAL_H)
#define DEFINED_IZOTCAL_H

#include "IzotPlatform.h"

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
 * This function starts the wlan connection for the FreeRtos

 */
extern int CalStart(void);

/*
 * Function: SetCurrentIP
 *
 * This function will send the announcement whenever new ip is
 * assigned to the device
 */
extern void SetCurrentIP(void);

/*
 * Function: InitSocket
 *
 * Open priority and non priority sockets and 
 * also add mac filter for broadcast message
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
 * Add membership of given address in multicast group.
 */
extern void AddIpMembership(uint32_t addr);

/*
 * Function: CalSend
 *
 * This API is used to send data on udp socket
 */
extern void CalSend(
uint32_t port, IzotByte* addr, IzotByte* pData, uint16_t dataLength
);

/*
 * Function: CalReceive
 *
 * This API is used to receive data on udp socket.
 * Keep the udp socket non blockable.
 */
extern int CalReceive(IzotByte* pData, IzotByte* pSourceAddr);

extern void CheckNetworkStatus(void);

#endif  /* defined(DEFINED_IZOTCAL_H) */
/* end of file. */
