//
// LsMappingUtil.c
//
// Copyright (C) 2023-2025 EnOcean
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

#include <string.h>
#include "IzotConfig.h"
#include "IzotApi.h"
#include "IPv4ToLsUdp.h"
#include "IzotCal.h"

/*------------------------------------------------------------------------------
Section: Macros
------------------------------------------------------------------------------*/
#define TRUE  1
#define FALSE 0

// These definitions are used to define the state of a LsMappingInfo entry
#define LS_MAP_STATE_AVAILABLE     0  // The entry is not being used
#define LS_MAP_STATE_DERIVED       1  // The IP address should be derived from 
                                      // the LS address
#define LS_MAP_STATE_ARBITRARY     2  // The IP address is in the 
                                      // arbitraryIdAddress array

// The maximum number of entries. This should be based on the maximum number 
// of address table entries plus some more to support responding to devices 
// that send messages to this one.
#define MAX_LS_MAP_INFO 50  


/*------------------------------------------------------------------------------
Section: Global
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
Section: Definitions
------------------------------------------------------------------------------*/
// The LsMappingInfo structure contains an entry for each LS address that we 
// know about. Note that the organinization of this mapping is not very 
// efficient, as it includes the domain ID in every entry, but the device is 
// likely to only support two domain IDs of its own and maybe a flex domainID.
// An improvement would be to have a short list of domains and have this 
// structure just indicate which domain by index or something of that nature. 
// But for this example, I decided to keep it simple.
typedef struct
{
    IzotByte state;         // The state of the address: LS_MAP_ADDR_TYPE_*
    IzotByte domainId[6];   // The LS domain ID
    IzotByte domainLen;     // The length of the domain (0, 1, 3 or 6)
    IzotByte subnetId;      // The LS subnet ID
    IzotByte nodeId;        // The LS node 
    IzotByte ageCount;      // A count of the number of times the age timer has
                            // expired since the address was last refreshed.
                            // If the state is LS_MAP_STATE_ARBITRARY and
                            // the ageCount is over a specified limit, the
                            // entry should be deleted by setting its state back
                            // to LS_MAP_STATE_ARBITRARY

    // The arbitrary IP addr - valid only if the state = LS_MAP_STATE_ARBITRARY
    IzotByte arbitraryIdAddress[IPV4_ADDRESS_LEN];  
} LsMappingInfo;

/*------------------------------------------------------------------------------
Section: Static
------------------------------------------------------------------------------*/
// The mapping array.  Entries get added via the 
// Ipv4SetArbitraryAddressMapping and Ipv4SetDerivedAddressMapping 
// calbacks as message are processed by ipv4_convert_ls_v1_to_v0
static LsMappingInfo lsMapInfo[MAX_LS_MAP_INFO];

// The number of entries in the map -used to limit searches.
static int numMapEntries = 0;

/*
 *
 * SECTION: FUNCTIONS
 *
 */

/*
 * Function:   findMappingInfo
 * Find the mapping entry for the specified LS address.  
 *
 * Parameters:
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * subnetId:               The LS subnet ID
 * nodeId:                 The LS node ID
 *
 * Returns:
 * A pointer to the entry, or NULL.
 * 
 * 
 */
static LsMappingInfo *findMappingInfo(
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte subnetId, 
IzotByte nodeId
)
{
    LsMappingInfo *pMapInfo = NULL;
    int i;

    // Find the entry for this address.  
    // If it doesn't exist, we don't know anything
    // about the destination.
    if (domainLen == 3) {
        domainLen = 2;  // Only compare the first two bytes
    }
    
    // Scan the map for a match
    for (i = 0; pMapInfo == NULL && i < numMapEntries; i++) {
        if (lsMapInfo[i].state != LS_MAP_STATE_AVAILABLE &&
            lsMapInfo[i].domainLen == domainLen && 
            memcmp(&lsMapInfo[i].domainId, pDomainId, domainLen) == 0 &&
            lsMapInfo[i].subnetId == subnetId && 
            lsMapInfo[i].nodeId == (nodeId&NODE_ID_MASK)) {
            pMapInfo = &lsMapInfo[i];
        }
    }
    return pMapInfo;
}

/*
 * Function:   findAvailMappingInfo
 * Find an available entry and fill in the LS information.  If the map is full,
 * just start over. A performance improvement would be to remove entries on a
 * least recently used basis
 * 
 * Parameters:
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * subnetId:               The LS subnet ID
 * nodeId:                 The LS node ID
 *
 * Returns:
 *  A pointer to the new entry.
 * 
 * 
 */
static LsMappingInfo *findAvailMappingInfo(
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte subnetId, 
IzotByte nodeId)
{
    LsMappingInfo *pMapInfo = NULL;
    // Find an available one;
    int i;

    if (domainLen == 3) {
        domainLen = 2;  // Only compare the first two bytes
    }

    // Scan the map for an available entry
    for (i = 0; pMapInfo == NULL && i < numMapEntries; i++) {
        if (lsMapInfo[i].state == LS_MAP_STATE_AVAILABLE) {
            pMapInfo = &lsMapInfo[i];
        }
    }    

    if (pMapInfo == NULL) {
        // There was none. Will need to grow the map to accomadate the
        // new entry
        if (numMapEntries >= MAX_LS_MAP_INFO) {
            // Whoops, can't make it bigger. Just forget everyting and 
            // start over. Could definately be smarter about this, using 
            // an LRU algorithm for example to select a good candidate.
            numMapEntries = 0;
        }
        // Grow the map to accomodate the new entry
        pMapInfo = &lsMapInfo[numMapEntries++];
    }

    // Set the LS addressing information.  Note that the caller needs to 
    // set the state.
    memset(pMapInfo, 0, sizeof(*pMapInfo));
    memcpy(pMapInfo->domainId, pDomainId, domainLen);
    pMapInfo->domainLen = domainLen;
    pMapInfo->subnetId = subnetId;
    pMapInfo->nodeId = (nodeId&NODE_ID_MASK);

    return pMapInfo;
}

/*
 * Function:   Ipv4GetArbitrarySourceAddress
 * This callback is used to retrieve arbitrary IP address information 
 * for a given source address.  
 * 
 * Parameters:
 * lsMappingHandle:        A handle used for LS mapping 
 * pSourceIpAddress:       On input, a pointer the desired (LS derived) source
 *                         IP address.  If this IP address cannot be used, 
 *                         pSourceIpAddress will be updated with the arbitrary
 *                         IP address to be used instead.
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * pEnclosedSource:        Pointer to a buffer to receive the necessary LS
 *                         source addressing information (in V1 format) to be 
 *                         added to the UDP payload, if any
 *
 * Returns:
 *  The length of the additional enclosed source address information
 * 
 */
IzotByte Ipv4GetArbitrarySourceAddress(
void *lsMappingHandle,
IzotByte *pSourceIpAddress, 
const IzotByte *pDomainId, 
int domainIdLen,
IzotByte *pEnclosedSource
)
{
    IzotByte enclosedSourceLen = 0;
    IzotByte foundAddress = FALSE;
    const IzotByte *pArbitraryAddress = NULL;
    IzotByte includeDomain = FALSE;

#if UIP_CONF_IPV6
    if (domainIdLen == 6)
#else
    if (domainIdLen <= 1 || (domainIdLen == 3 && pDomainId[2] == 0))
#endif
    {
        // See if we have a bound socket for this source IP address.
        foundAddress = 
        Ipv4IsUnicastAddressSupported(lsMappingHandle, pSourceIpAddress); 
        
        if (!foundAddress) {
            // Can't use this address. 
            // See if we can find a domain match at least!
            IzotByte domainIpPrefix[4];
            includeDomain = TRUE;
            Ipv4GenerateLsSubnetNodeAddr(
            pDomainId, domainIdLen, 1, 1, domainIpPrefix);
            const IzotByte *pSourceAddr = ownIpAddress;
            if (pArbitraryAddress == NULL) {
                pArbitraryAddress = pSourceAddr; // First available
            }
            if (memcmp(pSourceAddr, domainIpPrefix, IPV4_LSIP_IPADDR_DOMAIN_LEN) 
            == 0) {
                // This matches the domain, so its a good arbitrary IP address.
                includeDomain = FALSE;
                enclosedSourceLen = IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE+1;
                pArbitraryAddress = pSourceAddr;
            }
        }
    }

    if (includeDomain) {
        int encodedDomainLen = 0;
        // Need to include the domain plus the source subnet/node
        enclosedSourceLen = IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DM + domainIdLen;
        switch (domainIdLen) {
        case 1: encodedDomainLen = 1; break;
        case 3: encodedDomainLen = 2; break;
        case 6: encodedDomainLen = 3; break;
        }
        pEnclosedSource[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN] = encodedDomainLen;
        memcpy(&pEnclosedSource[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DM], 
        pDomainId, domainIdLen);
    }

    if (pArbitraryAddress != NULL) {
        // Using an arbitrary address. Include the source subnet/node
        pEnclosedSource[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_SUBNET] = 
                                pSourceIpAddress[IPV4_LSIP_UCADDR_OFF_SUBNET];
        pEnclosedSource[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE] = 
                            pSourceIpAddress[IPV4_LSIP_UCADDR_OFF_NODE] & NODE_ID_MASK;
        if (enclosedSourceLen > (IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE+1)) {
            // Domain is included, so set the flag
            pEnclosedSource[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMFLAG] |= 
                                          IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_DMFLG;
        }

        // Copy the arbitrary source IP address
        memcpy(pSourceIpAddress, pArbitraryAddress, IPV4_ADDRESS_LEN);
    }

    return enclosedSourceLen;
}

/*
 * Function:   Ipv4GetArbitraryDestAddress
 * This callback is used to used by the ls to udp translation layers to retrieve
 * arbitrary IP address information for a given destination address.
 * 
 * Parameters:
 * lsMappingHandle:        A handle used for LS mapping 
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * subnetId:               The LS destination subnet ID
 * nodeId:                 The LS destination node ID
 * ipv1AddrFmt:            The LS/IP address format
 * pDestIpAddress:         Pointer to a buffer to receive the destination IP
 *                         address to be used.
 * pEnclosedDest:          Pointer to a buffer to receive additional LS
 *                         destination address information enclosed in the
 *                         PDU, if any.
 * 
 * Returns:
 *  The length of the additional enclosed destination address information
 * 
 */
IzotByte Ipv4GetArbitraryDestAddress(
void *lsMappingHandle,
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte subnetId, 
IzotByte nodeId, 
IzotByte ipv1AddrFmt,
IzotByte *pDestIpAddress, 
IzotByte *pEnclosedDest
)
{
    LsMappingInfo *pMapInfo = 
                        findMappingInfo(pDomainId, domainLen, subnetId, nodeId);
    int enclosedDestLen = 0;
    if (pMapInfo == NULL || pMapInfo->state == LS_MAP_STATE_ARBITRARY) {
        // Not using a derived address, so we need to include the destination LS
        // address in the payload.
        pEnclosedDest[enclosedDestLen++] = subnetId;
        pEnclosedDest[enclosedDestLen++] = nodeId & NODE_ID_MASK;
        if (ipv1AddrFmt == IPV4_LSUDP_NPDU_ADDR_FMT_GROUP_RESP) {
            pEnclosedDest[enclosedDestLen] |= 0x80;
        }

        if (pMapInfo == NULL) {
            // It's not an arbitrary address, 
            // so we need to use the subnet broadcast address.
            Ipv4GenerateLsMacAddr(IPV4_LS_MC_ADDR_TYPE_BROADCAST, 
#if UIP_CONF_IPV6
                                pDomainId, domainLen, 
#endif
                                subnetId, pDestIpAddress);

        } else {
            // Use the arbitrary address
            memcpy(pDestIpAddress, pMapInfo->arbitraryIdAddress, 
                                         sizeof(pMapInfo->arbitraryIdAddress));
        }
    }

    return enclosedDestLen;

}

/*
 * Function:   Ipv4SetArbitraryAddressMapping
 * This callback is used by the ls to udp translation layers to 
 * inform the LS/IP mapping layers that a given LS address uses an
 * arbitrary IP address.
 * 
 * Parameters:
 * lsMappingHandle:        A handle used for LS mapping 
 * pArbitraryIpAddr:       The arbitrary IP address to use when addressing
 *                         the LS device.
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * subnetId:               The LS subnet ID
 * nodeId:                 The LS node ID
 * 
 * Returns:
 * void
 * 
 */
void Ipv4SetArbitraryAddressMapping(
void *lsMappingHandle, 
const IzotByte *pArbitraryIpAddr, 
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte subnetId, 
IzotByte nodeId
)
{
    LsMappingInfo *pMapInfo = 
                       findMappingInfo(pDomainId, domainLen, subnetId, nodeId);
                       
    if (pMapInfo == NULL) {
        pMapInfo = findAvailMappingInfo(pDomainId, domainLen, subnetId, nodeId);
    }
    
    pMapInfo->state = LS_MAP_STATE_ARBITRARY;
    memcpy(pMapInfo->arbitraryIdAddress, pArbitraryIpAddr, 
                                          sizeof(pMapInfo->arbitraryIdAddress));
    pMapInfo->ageCount = 0;
}

/*
 * Function:   Ipv4SetDerivedAddressMapping
 * This callback is used by the ls to udp translation layers to 
 * inform the LS/IP mapping layers that a given LS address uses an
 * LS derived IP address.  
 * 
 * Parameters:
 * lsMappingHandle:        A handle used for LS mapping 
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * subnetId:               The LS subnet ID
 * nodeId:                 The LS node ID
 * 
 * Returns:
 * void
 * 
 */
void Ipv4SetDerivedAddressMapping(
void *lsMappingHandle, 
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte subnetId, 
IzotByte nodeId
)
{
    LsMappingInfo *pMapInfo = 
                        findMappingInfo(pDomainId, domainLen, subnetId, nodeId);

    if (pMapInfo == NULL) {
        pMapInfo = findAvailMappingInfo(pDomainId, domainLen, subnetId, nodeId);
    }
    
    pMapInfo->state = LS_MAP_STATE_DERIVED;
    pMapInfo->ageCount = 0;
}

/*
 * Function:   Ipv4SetDerivedSubnetsMapping
 * This callback is used by the ls to udp translation layers when an 
 * SubnetsAddrMapping message is received.
 * 
 * Parameters:
 * lsMappingHandle:        A handle used for LS mapping 
 * pDomainId:              The LS domain ID.
 * domainIdLen:            The length (in bytes) of the LS domain ID
 * set:                    True to set the derived mapping entries, clear to
 *                         clear the derived mapping entries.
 * pSubneteMap:            Pointer to a bit map of subnets to set or clear.
 * 
 * Returns:
 * void
 * 
 */
void Ipv4SetDerivedSubnetsMapping(
void *lsMappingHandle, 
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte set, 
const IzotByte *pSubnets
)
{
    // This example does not process derived subnets mapping. It makes sense to
    // process this message only if the device keeps track of large numbers 
    // of devices. For example, the DS/EX stack maintains a bitmap of nodes 
    // using derived addresses, since it can support hundreds of LS addresses.
    IzotByte i, j, k;
    
    // Check for the mask bits and update the mapping table
    for (i = 0; i < 32; i++) { // Check for 32 byte one by one
        for (j = 0; j < 8; j++) { // Take the first byte
            // check for the affected bit in one byte
            if ((IzotByte)pSubnets[i] & (0x80 >> j)) {
                for (k = 0; k < numMapEntries; k++) {
                    if (lsMapInfo[k].subnetId == ((i * 8) + j)) {
                        if (set) {
                            lsMapInfo[k].state = LS_MAP_STATE_DERIVED;
                        } else {
                            lsMapInfo[k].state = LS_MAP_STATE_AVAILABLE;
                        }
                        break;
                    }
                }
            }
        }
    }
}

/*
 * Function:   Ipv4IsUnicastAddressSupported
 * This callback is used by the ls to udp translation layers to
 * determmine whether or not the specified IP address can be used by this
 * device as a source address.
 * 
 * Parameters:
 * lsMappingHandle:        A handle used for LS mapping 
 * ipAddress:              The LS domain ID.
 * 
 * 
 */
IzotByte Ipv4IsUnicastAddressSupported(
void *lsMappingHandle, 
IzotByte *ipAddress
)
{
    if (memcmp(ownIpAddress, ipAddress, sizeof(ownIpAddress)) == 0) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Function:   UpdateMapping
 * Update the mapping table based on the information in announcement 
 * received
 * 
 * pDomainId:           Domain id of device
 * domainLen:           Domain length of device 
 * subnetId:            Subnet id of device
 * nodeId:              Node Id of device
 * addr:                Absolute Ip address of device from which
 *                      announcement is received
 *
 */
void UpdateMapping(
IzotByte *pDomainId, IzotByte domainLen, 
IzotByte subnetId, IzotByte nodeId, 
const IzotByte *addr
)
{
    LSUDP_PRINTF(
    "Announcement received from subnet: "
    "%d, node %d, domain: %X %X 00, ip addr: %d.%d.%d.%d -->", 
    subnetId, 
    nodeId, 
    (domainLen == 3 ? pDomainId[0] : (domainLen == 1 ? pDomainId[1] : 0)), 
    (domainLen == 3 ? pDomainId[1] : 0), 
    addr[0], addr[1], addr[2], addr[3]
    );
    
    LsMappingInfo *pMapInfo = findMappingInfo(pDomainId, domainLen, subnetId, nodeId);
    
    if (pMapInfo == NULL) {
        pMapInfo = findAvailMappingInfo(pDomainId, domainLen, subnetId, nodeId);
    }
    
    if (addr[IPV4_LSIP_UCADDR_OFF_SUBNET] == subnetId && addr[IPV4_LSIP_UCADDR_OFF_NODE] == nodeId) {
        if ((domainLen == 0 && addr[0] == 0xc0 && addr[1] == 0xa8) || (domainLen == 1 && addr[0] == 0x0A) || 
        (domainLen == 3 && addr[0] == pDomainId[0] && addr[1] == pDomainId[1])) {
            LSUDP_PRINTF("mapping updated to the ls derived\r\n");
            pMapInfo->state = LS_MAP_STATE_DERIVED;
        } else {
            pMapInfo->state = LS_MAP_STATE_ARBITRARY;
            LSUDP_PRINTF("mapping updated to the arbitrary\r\n");        
        }
    } else {
        pMapInfo->state = LS_MAP_STATE_ARBITRARY;
        LSUDP_PRINTF("mapping updated to the arbitrary\r\n");
    }
        
    memcpy(pMapInfo->arbitraryIdAddress, addr, sizeof(pMapInfo->arbitraryIdAddress));
    pMapInfo->ageCount = 0;
}

/*
 * Function:   ClearMapping
 * clear the mapping tabel after aging period expires
 * 
 * Parameters: None
 *
 */
void ClearMapping(void)
{
    int i;

    for (i = 0; i < numMapEntries; i++) {
        memset(&lsMapInfo[i], 0, sizeof(LsMappingInfo));
    }
}
