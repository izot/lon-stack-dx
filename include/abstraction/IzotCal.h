/*
 * IzotCal.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   IP Connectivity Abstraction Layer
 * Purpose: Defines portable functions and types for communicating
 *          with IP sockets.
 * Notes:   IP sockets are required for LON/IP and are not required
 * 			for native LON.
 */

#include "izot/IzotPlatform.h"

#if !defined(DEFINED_IZOTCAL_H)
#define DEFINED_IZOTCAL_H

#define IPV4_ADDRESS_LEN 			4
#define IP_ADDRESS_CHECK_INTERVAL	60000

#if LINK_IS(WIFI)
	#define LINK_CHECK_INTERVAL	300000
#else
	#define LINK_CHECK_INTERVAL	  3000
#endif	// LINK_IS(WIFI)

#ifdef CAL_DEBUG
	#define CAL_Printf(format,args...) wmprintf(args)
#else
	#define CAL_Printf(format,args...)	;
#endif

/*****************************************************************
 * Section: Globals
 *****************************************************************/
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
extern IzotByte	ownIpAddress[IPV4_ADDRESS_LEN]; 
                // Buffer to store the IP address
extern IzotBool is_connected;
                // Flag to report IP link connectivity
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
/*
 * Starts the IP link.
 * Parameters:
 *   None
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code
 *   on failure.
 */
extern IzotApiError CalStart(void);

/*
 * Sets the current IP address.
 * Parameters:
 *   None
 * Returns:
 *   TRUE if the IP address changed since the last call
 */
extern IzotBool SetCurrentIP(void);

/*
 * Initializes IP sockets and adds a MAC filter for broadcast messages.
 * Parameters:
 *   port: UDP port to open
 * Returns:
 *   0 on success, or a negative error code on failure
 * Notes:
 *   Priority and non-priority sockets are initialized to the same port.
 *   Implementation of this function is required for LON/IP support.
 */
extern int InitSocket(int port);

/*
 * Removes membership of the specified address from a multicast group.
 * Parameters:
 *   addr: Address to remove
 * Returns:
 *   None
 * Notes:
 *   Implementation of this function is required for LON/IP support.
 */
extern void RemoveIPMembership(uint32_t addr);

/*
 * Adds membership for the specified address to a multicast group.
 * Parameters:
 *   addr: Address to add
 * Returns:
 *   None
 * Notes:
 *   Implementation of this function is required for LON/IP support.
 */
extern void AddIpMembership(uint32_t addr);

/*
 * Sends data on a UDP socket.
 * Parameters:
 *   port: UDP port to send to
 *   addr: IP address to send to
 *   pData: Pointer to data to send
 *   dataLength: Length of data to send
 * Returns:
 *   None
 */
extern void CalSend(uint32_t port, IzotByte* addr, IzotByte* pData, 
				uint16_t dataLength);

/*
 * Receives data from a UDP socket.  
 * Parameters:
 *   pData: Pointer to buffer to receive data
 *   pSourceAddr: Pointer to buffer to receive source IP address
 * Returns:
 *   Number of bytes received, or a negative error code on failure
 * Notes:
 *   Keep the DUP socket non-blockable.  Implementation of this function is 
 *   required for LON/IP support.
 */
extern int CalReceive(IzotByte* pData, IzotByte* pSourceAddr);

/*
 * Checks for a change of status for the data link. 
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes: 
 *   Handle an unexpected loss or recovery of a data link.
 */
extern void CheckNetworkStatus(void);

#endif  /* defined(DEFINED_IZOTCAL_H) */
