//
// IPv4ToLsUdp.c
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

#if PLATFORM_IS(FRTOS)
#include <wm_net.h>
#include <wlan.h>
#include <wmtime.h>
#include <wm_os.h>
#include <arch/sys.h>
#include <dhcp-server.h>
#endif

#include "IzotTypes.h"
#include "lcs.h"
#include "lcs_queue.h"
#include "err.h"
#include "IPv4ToLsUdp.h"
#include <lcs_node.h>
#include "IzotCal.h"
#include "lcs_api.h"
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------------
Section: Macros
------------------------------------------------------------------------------*/
#ifdef USE_UIP
// Access to Contiki global buffer.
# define UIP_IP_BUF  ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
# define UIP_UDP_BUF ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#endif

#define MAX_DATA_LEN  255
/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
#ifdef USE_UIP
// Address Formats are based on those found in the LonTalk packet.
// These are used for incoming addresses.  The first 4 match
// the address formats used in the LonTalk packet.
typedef enum
{
    LT_AF_BROADCAST          = 0,
    LT_AF_GROUP              = 1,
    LT_AF_SUBNET_NODE        = 2,
    LT_AF_UNIQUE_ID          = 3,
    LT_AF_GROUP_ACK          = 4,
    LT_AF_TURNAROUND         = 5,
    LT_AF_NONE               = 6,    // indicates no address available
} LtAddressFormat;
#endif

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
void *ls_mapping;
IzotUbits16 AnnounceTimer         = 60;    // in seconds
IzotUbits16 AddrMappingAgingTimer = 0;    // in seconds


#if UIP_CONF_IPV6
    const IzotByte ipv6_ls_multicast_prefix[] = {0x4C, 0x53, 0x49, 0x50, 0x56, 0x36};
    uip_ds6_prefix_t *pIpv6LsDomainUipPrefix; // Pointer to my IP prefix
#else
    const IzotByte ipv4_ls_multicast_prefix[] = {0xEF, 0xC0};
    // The 2 byte IP prefix used to represent the 0 length domain
    const IzotByte ipv4_zero_len_domain_prefix[] = {IPV4_DOMAIN_LEN_0_PREFIX_0, IPV4_DOMAIN_LEN_0_PREFIX_1};
#endif

/*------------------------------------------------------------------------------
Section: Statics
------------------------------------------------------------------------------*/
// Encoded domain length
static const IzotByte domainLengthTable[4] = { 0, 1, 3, 6 }; 
static LonTimer        AnnouncementTimer;
static LonTimer        AgingTimer;

/*
 * SECTION: FUNCTIONS
 *
 */

/* 
 *  Callback: Ipv4GenerateLsPrefix
 *  Generate a LS prefix from a LS domain and subnet
 *
 *  Parameters:
 *    pDomainId:  Pointer to the domain ID
 *    domainLen:  Length of the domain (0 to 6)
 *    subnet:     LS subnet ID
 *    pAddr:      Pointer to IPV4 address to store the prefix
 *
 *  Returns:
 *  <void>.   
 *
 */
static void Ipv4GenerateLsPrefix(const IzotByte *pDomainId, IzotByte domainLen, IzotByte subnet, IzotByte *pAddr)
{
#if UIP_CONF_IPV6
    memset(pAddr, 0, sizeof(uip_ipaddr_t));
    if (domainLen <= 6) {
        memcpy(pAddr, pDomainId, domainLen);
        pAddr += 7;
        *pAddr++ = subnet;
    }
#else
    memset(pAddr, 0, IPV4_ADDRESS_LEN);

    if (domainLen > 6) {
        // An invalid domain. Set node and subnet to 0.  
        // Will use the zero length domain below.
        domainLen = 0;  
    }
    if (domainLen == 0) {
        memcpy(pAddr, ipv4_zero_len_domain_prefix, sizeof(ipv4_zero_len_domain_prefix));
    } else {
        if (domainLen == 1) {
            pAddr[0] = IPV4_DOMAIN_LEN_1_PREFIX;
            pAddr[1] = pDomainId[0];
        } else {
            pAddr[0] = pDomainId[0];
            pAddr[1] = pDomainId[1];
        }
    }
    pAddr[IPV4_LSIP_UCADDR_OFF_SUBNET] = subnet;
#endif
}

/* 
 *  Callback: getDomainLenEncoding
 *  Get domain length from encoded value
 *
 *  Parameters:
 *    domainLen:  Length of the domain 
 *
 *  Returns:
 *  <IzotByte>.   
 *
 */
static IzotByte getDomainLenEncoding(int domainLen)
{
    IzotByte encodedLen;
    for (encodedLen = 0; encodedLen < sizeof(domainLengthTable)/sizeof(domainLengthTable[0]); encodedLen++) {
        if (domainLen == domainLengthTable[encodedLen]) {
            return encodedLen;
        }
    }
    return 0;  // Whoops.
}

/* 
 *  Callback: RestoreIpMembership
 *  Scan Address table at boot time and add membership of
 *  multicast address if present
 *
 *  Returns:
 *  <void>.   
 *
 */
static void RestoreIpMembership(void)
{
    IzotByte i;
    uint32_t mc_addr = 0xEFC00100;
    uint32_t bc_addr = 0XEFC00000;
    
    // Group multicast membership
    for (i = 0; i < eep->readOnlyData.Extended; i++) {
        //Indicates the group entry in Address table
        if (IZOT_GET_ATTRIBUTE(eep->addrTable[i].Group, IZOT_ADDRESS_GROUP_TYPE) == 1) {
            mc_addr |= eep->addrTable[i].Group.Group;
            AddIpMembership(mc_addr);
        }
    }
    
    // Domain Broadcast membership
    AddIpMembership(bc_addr);
    
    // Subnet broadcast membership
    if (eep->domainTable[0].Subnet != 0)    {
        bc_addr |= eep->domainTable[0].Subnet;
        AddIpMembership(bc_addr);
    }
}

#if IPV4_INCLUDE_LTVX_LSUDP_TRANSLATION

/* 
 *  Callback: Ipv4ConvertLTVXtoLsUdp
 *  Convert the LonTalk V0 or V2 NPDU to LS/UDP format.   
 *
 *  Parameters:
 *   pNpdu:              On input, pointer to the LTV0 or LTV2 NPDU.  
 *                       This gets overwriten by the LS/UDP payload.
 *   pduLen:             The size, in bytes of the LTVX NPDU
 *   pSourceAddr:        Pointer to recieve the IP source addresss, 
 *                       calculated from the address in the LTVX NPDU.
 *   pSourcePort:        Pointer to recieve the source port.
 *   pDestAddr:          Pointer to recieve the IP destination addresss, 
 *                       calculated from the address in the LTVX NPDU.
 *   pDestPort:          Pointer to recieve the dest port.
 *   lsMappingHandle:    A handle used for LS mapping 
 *
 *  Returns:
 *  <void>.   
 *
 */
static uint16_t Ipv4ConvertLTVXtoLsUdp(IzotByte *pNpdu, uint16_t pduLen, IzotByte *pSourceAddr, uint16_t *pSourcePort, 
IzotByte *pDestAddr, uint16_t *pDestPort
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
    , void *lsMappingHandle
#endif
)
{
    IzotByte domainOffset = 0;
    LtAddressFormat lsVxAddrFmt = IPV4_GET_ADDRESS_FORMAT_FROM_NPDU(pNpdu);
    IzotByte domainLen = domainLengthTable[pNpdu[IPV4_LTVX_NPDU_IDX_TYPE] & IPV4_LTVX_NPDU_MASK_DOMAINLEN];
    IzotByte lsUdpHdrLen = 2; // Size of the LS/UDP HDR
    
    // Second byte of LS/UDP header
    IzotByte lsUdpHdrByte1 = IPV4_GET_PDU_FORMAT_FROM_NPDU(pNpdu);  
    IzotByte lsUdpEnclosedAddr[7];
    IzotByte lsUdpEnclosedAddrLen = 0;
    IzotByte failed = 0;

    if (!IPV4_LT_IS_VER_LS_LEGACY_MODE(pNpdu[IPV4_LTVX_NPDU_IDX_TYPE]) &&
    !IPV4_LT_IS_VER_LS_ENHANCED_MODE(pNpdu[IPV4_LTVX_NPDU_IDX_TYPE])) {
        failed = 1; // Version is not supported;
    } else {
        switch (lsVxAddrFmt) {
        case LT_AF_BROADCAST:
        case LT_AF_GROUP:
            {
                domainOffset = IPV4_LTVX_NPDU_IDX_DEST_ADDR + 1;
                Ipv4GenerateLsMacAddr(lsVxAddrFmt == LT_AF_BROADCAST ? 
                    IPV4_LS_MC_ADDR_TYPE_BROADCAST : IPV4_LS_MC_ADDR_TYPE_GROUP,
#if UIP_CONF_IPV6
                   &pNpdu[IPV6_LTVX_NPDU_IDX_DEST_ADDR + 1], domainLen,
#endif
                pNpdu[IPV4_LTVX_NPDU_IDX_DEST_ADDR],  pDestAddr);
                
                if ((lsUdpHdrByte1 == ENCLOSED_PDU_TYPE_TPDU || lsUdpHdrByte1 == ENCLOSED_PDU_TYPE_SPDU) &&
                ((pNpdu[4 + domainLen] & IPV4_LTVX_NPDU_MASK_SERVICE_TYPE) == 0)) {
                    // Either an ackd or request service.  Include 
                    lsUdpHdrByte1 |= IPV4_LSUDP_NPDU_MASK_MCR;
                    lsUdpHdrLen += 1;  // Add room for backlog  info.
                }

                if (lsVxAddrFmt == LT_AF_BROADCAST) {
                    if (pNpdu[IPV4_LTVX_NPDU_IDX_DEST_SUBNET] == 0) {
                        lsUdpHdrByte1 |= IPV4_LSUDP_NPDU_ADDR_FMT_DOMAIN_BROADCAST;
                    } else {
                        lsUdpHdrByte1 |= IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_BROADCAST;
                        lsUdpEnclosedAddrLen = 1;    // add room for subnetId
                        lsUdpEnclosedAddr[0] = pNpdu[IPV4_LTVX_NPDU_IDX_DEST_ADDR];
                    }
                } else {
                    lsUdpEnclosedAddrLen = 1;        // add room for groupID
                    lsUdpHdrByte1 |= IPV4_LSUDP_NPDU_ADDR_FMT_GROUP;
                    lsUdpEnclosedAddr[0] = pNpdu[IPV4_LTVX_NPDU_IDX_DEST_ADDR];
                }
            }
            break;
        case LT_AF_SUBNET_NODE:
            {
                IzotByte lsUdpAddrFmt;
                if (pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE] & 0x80) {
                    lsUdpAddrFmt = IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_NODE;
                    domainOffset = IPV4_LTVX_NPDU_IDX_DEST_NODE + 1;
                } else {
                    lsUdpAddrFmt = IPV4_LSUDP_NPDU_ADDR_FMT_GROUP_RESP;
                    domainOffset = IPV4_LTVX_NPDU_IDX_DEST_NODE + 3;
                }

                Ipv4GenerateLsSubnetNodeAddr(&pNpdu[domainOffset], domainLen, 
                pNpdu[IPV4_LTVX_NPDU_IDX_DEST_SUBNET], pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NODE], pDestAddr);
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
                if (Ipv4GetArbitraryDestAddress(lsMappingHandle, &pNpdu[domainOffset], domainLen, 
                pNpdu[IPV4_LTVX_NPDU_IDX_DEST_SUBNET], pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NODE],
                lsUdpAddrFmt, pDestAddr, lsUdpEnclosedAddr)) {
                    lsUdpEnclosedAddrLen += 2;
                    lsUdpAddrFmt = IPV4_LSUDP_NPDU_ADDR_FMT_EXP_SUBNET_NODE;
                }
#endif
                if ((pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE] & 0x80) == 0) {
                    // Group response.  Add in the grop and member.
                    lsUdpEnclosedAddr[lsUdpEnclosedAddrLen++] = pNpdu[IPV4_LTVX_NPDU_IDX_RESP_GROUPID];
                    lsUdpEnclosedAddr[lsUdpEnclosedAddrLen++] = pNpdu[IPV4_LTVX_NPDU_IDX_RESP_GROUPMBR];
                }
                lsUdpHdrByte1 |= lsUdpAddrFmt; 
            }
            break;
        case LT_AF_UNIQUE_ID:
            domainOffset = IPV4_LTVX_NPDU_IDX_DEST_NEURON_ID + IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN;
#if UIP_CONF_IPV6
            if (pNpdu[IPV6_LTVX_NPDU_IDX_DEST_SUBNET]) {
                // Unicast neuron ID addressing
                lsUdpHdrByte1 |= IPV6_LSUDP_NPDU_ADDR_FMT_NEURON_ID;
#if UIP_CONF_IPV6
                ipv6_gen_ls_neuronid_addr(&pNpdu[domainOffset], domainLen, 
                pNpdu[IPV6_LTVX_NPDU_IDX_DEST_SUBNET], &pNpdu[IPV6_LTVX_NPDU_IDX_DEST_NEURON_ID], pDestAddr);
#endif
            } else
#endif
            {
                // Subnet is 0. This is a neuron ID addressed message 
                // that floods the network. Use the broadcast address and 
                // include the NEURON ID in the payload
                lsUdpHdrByte1 |= IPV4_LSUDP_NPDU_ADDR_FMT_BROADCAST_NEURON_ID;
                Ipv4GenerateLsMacAddr(IPV4_LS_MC_ADDR_TYPE_BROADCAST, 
#if UIP_CONF_IPV6
                                   &pNpdu[domainOffset], domainLen,
#endif
                                   0,  pDestAddr);

                // add room for subnetID and neuronID
                lsUdpEnclosedAddrLen = IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN + 1;
                lsUdpEnclosedAddr[0] = pNpdu[IPV4_LTVX_NPDU_IDX_DEST_SUBNET];
                memcpy(&lsUdpEnclosedAddr[1], &pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NEURON_ID], 
                                IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN);
            }
            break;
        default:
            // Unsupported adress type
            failed = 1;
        };
    }

    if (!failed) {
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
        IzotByte arbitrarySourceAddressLen;
#endif
        // Copy the enclosed PDU following the LS/UDP header
        pduLen -= domainOffset+domainLen;
        if (pSourceAddr != NULL) {
            Ipv4GenerateLsSubnetNodeAddr(&pNpdu[domainOffset], domainLen, pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_SUBNET], 
            pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE], pSourceAddr);
        }

        if (pNpdu[0] & IPV4_LTVX_NPDU_MASK_PRIORITY) {
            lsUdpHdrByte1 |= (1 << IPV4_LSUDP_NPDU_BITPOS_PRIORITY);
        }
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
        {
            IzotByte arbitrarySourceAddress[IPV4_MAX_ARBITRARY_SOURCE_ADDR_LEN];
            arbitrarySourceAddressLen = Ipv4GetArbitrarySourceAddress(lsMappingHandle, pSourceAddr, 
                                                &pNpdu[domainOffset], domainLen, arbitrarySourceAddress);
                                              
            memmove(&pNpdu[lsUdpHdrLen + lsUdpEnclosedAddrLen + arbitrarySourceAddressLen], 
            &pNpdu[domainOffset + domainLen], pduLen);
            
            if (arbitrarySourceAddressLen) {
                memcpy(&pNpdu[lsUdpHdrLen], arbitrarySourceAddress, arbitrarySourceAddressLen);
                lsUdpHdrLen += arbitrarySourceAddressLen;
            }
        }
#else
        memmove(&pNpdu[lsUdpHdrLen], &pNpdu[domainOffset + domainLen], pduLen);
#endif
        if (lsUdpHdrByte1 & IPV4_LSUDP_NPDU_MASK_MCR) {
            pNpdu[IPV4_LSUDP_NPDU_IDX_BLINFO] = pNpdu[0] & IPV4_LTVX_NPDU_MASK_DELTA_BACKLOG;  // Copy delta backlog.
        }

        // Set the version to use LS legacy or 
        // enhanced mode based on the LT version.
        if (IPV4_LT_IS_VER_LS_LEGACY_MODE(pNpdu[IPV4_LTVX_NPDU_IDX_TYPE])) {
            pNpdu[0] = IPV4_LSUDP_UDP_VER_LS_LEGACY << IPV4_LSUDP_NPDU_BITPOS_UDPVER;
        } else {
            pNpdu[0] = IPV4_LSUDP_UDP_VER_LS_ENHANCED << IPV4_LSUDP_NPDU_BITPOS_UDPVER;
        }

#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
        if (arbitrarySourceAddressLen) {
            pNpdu[0] |= IPV4_LSUDP_NPDU_MASK_ARB_SOURCE;
        }
#endif
        pNpdu[1] = lsUdpHdrByte1;
        memcpy(&pNpdu[lsUdpHdrLen], lsUdpEnclosedAddr, lsUdpEnclosedAddrLen);
        lsUdpHdrLen += lsUdpEnclosedAddrLen;
    } else {
        pduLen = 0;
        lsUdpHdrLen = 0;
    }
    
    return pduLen + lsUdpHdrLen;
}

#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
/* 
 *  Callback: Ipv4SendAnnouncement
 *  Send the announcement
 *
 *  Parameters:
 *    msg:  Pointer to the buffer
 *    len:  Length of the message
 *
 *  Returns:
 *  <void>.   
 *
 */
static void Ipv4SendAnnouncement(IzotByte *msg, IzotByte len)
{
    LKSendParam  *lkSendParamPtr; /* Param in lkOutQ or lkPriOutQ.   */
    IzotByte     *npduPtr;        /* Pointer to NPDU being formed.   */
    
    if (!QueueFull(&gp->lkOutQ)) {
        lkSendParamPtr  = QueueTail(&gp->lkOutQ);
    } else {
        return;
    }
    
    // ptr to NPDU constructed.
    npduPtr = (IzotByte *)(lkSendParamPtr + 1);

    // Write the parameters for the link layer.
    lkSendParamPtr->deltaBL = 0;
    lkSendParamPtr->altPath = 0;
    lkSendParamPtr->pduSize = len - 1;
    lkSendParamPtr->DomainIndex = 0;
    
    // Copy the pdu
    memcpy(npduPtr, &msg[1], len - 1);
    
    EnQueue(&gp->lkOutQ);
    INCR_STATS(LcsL3Tx);
}
#endif

/* 
 *  Callback: Ipv4ConvertLsUdptoLTVX
 *  Convert the LS/UDP packet found in the Contiki global uip_buf to an LTV0
 *  or LTV2 NPDU and return it in the buffer provided.
 * 
 *  Parameters:
 *   ipv6:               True if this is IPV6, false if IPV4
 *   pUdpPayload:        Pointer to the UDP data
 *   udpLen:             The length of the UDP data
 *   pSourceAddr:        Pointer to source address, in *network* order
 *   sourcePort:         Source port in *host* order
 *   pDestAddr:          Pointer to destination address, in *network* order
 *   destPort:           Destination port in *host* order
 *   pNpdu:              A pointer to a buffer to write the LTVX npdu.
 *   pLtVxLen:           A pointer to the size in bytes of the resulting NPDU.
 *   lsMappingHandle:    A handle used for LS mapping 
 *
 *  Returns:
 *  <void>.   
 *
 */
static void Ipv4ConvertLsUdptoLTVX(IzotByte ipv6, IzotByte *pUdpPayload, uint16_t udpLen, const IzotByte *pSourceAddr, 
uint16_t sourcPort,const IzotByte *pDestAddr, uint16_t destPort,
IzotByte *pNpdu, uint16_t *pLtVxLen
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
    , void *lsMappingHandle
#endif
)
{
    IzotByte *pLsUdpPayload = pUdpPayload;
    IzotByte *p = pNpdu;
    IzotByte npduHdr;
    IzotByte lsUdpHdr0 = 0; // First byte of LS/UDP header.
    IzotByte lsUdpHdr1;     // Second byte of LS/UDP header
    IzotByte domainLen;
    const IzotByte *pDomain;
    IzotByte failed = 0;

    if (ipv6) {
        failed = 1;
    } else {
        pDomain = pSourceAddr;

        if ((*pLsUdpPayload & IPV4_LSUDP_NPDU_MASK_UDPVER) > 
        (IPV4_LSUDP_UDP_VER_CURRENT << IPV4_LSUDP_NPDU_BITPOS_UDPVER)) {
            // Unsupported version.  Drop it.
            failed = 1;
        }
#if !IPV4_SUPPORT_ARBITRARY_ADDRESSES
        else if (*pLsUdpPayload & IPV4_LSUDP_NPDU_MASK_ARB_SOURCE) {
            // Unsupported version.  Drop it.
            failed = 1;
        }
#endif
        else {
            lsUdpHdr0 = *pLsUdpPayload;
            pLsUdpPayload++;
            lsUdpHdr1 = *pLsUdpPayload++;
            *p = (lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PRIORITY) ? IPV4_LTVX_NPDU_MASK_PRIORITY : 0;
                                              
            if (lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_MCR) {
                *p |= *pLsUdpPayload++ & IPV4_LSUDP_NPDU_MASK_DELTA_BACKLOG;    
            }
        }
    }

    if (!failed) {
        // Set version (0), pdu format and domain length
        npduHdr = ((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PDUFMT) << IPV4_LTVX_NPDU_BITPOS_PDUFMT);
        if ((lsUdpHdr0 & IPV4_LSUDP_NPDU_MASK_UDPVER) == 
        (IPV4_LSUDP_UDP_VER_LS_ENHANCED << IPV4_LSUDP_NPDU_BITPOS_UDPVER)) {
            // Whoops, need to set the LT version to enhanced mode.
            npduHdr |= (IPV4_LT_VER_ENHANCED << IPV4_LTVX_NPDU_BITPOS_VER);
        }
        domainLen = 0xff;
        if (lsUdpHdr0 & IPV4_LSUDP_NPDU_MASK_ARB_SOURCE) {
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
            if (pLsUdpPayload[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMFLAG] & IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_DMFLG) {
                domainLen = domainLengthTable[
                pLsUdpPayload[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN] & IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_DMLEN];
                pDomain = &pLsUdpPayload[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DM];
            } else {
                pDomain = pSourceAddr;
            }
#else
            failed = 1;
#endif
        } else if ((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_ADDRFMT) == IPV4_LSUDP_NPDU_ADDR_FMT_EXP_SUBNET_NODE) {
            pDomain = pSourceAddr;
        }

        if (domainLen == 0xff) {
            // The domain is not included in arbitrary source address.  
            // Need to extract it from source or dest addr.
            if (memcmp(pDomain, ipv4_zero_len_domain_prefix, sizeof(ipv4_zero_len_domain_prefix)) == 0) {
                domainLen = 0;
            } else if (pDomain[0] == IPV4_DOMAIN_LEN_1_PREFIX) {
                domainLen = 1;
                pDomain++;  // Skip first byte...
            } else {
                domainLen = 3;
            }
        }
        npduHdr |= getDomainLenEncoding(domainLen);
        p += 2;  // Skip over delta backlog and npuHdr;

#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
        if (lsUdpHdr0 & IPV4_LSUDP_NPDU_MASK_ARB_SOURCE) {
            // Update arbitrary address info
            Ipv4SetArbitraryAddressMapping(lsMappingHandle, pSourceAddr, pDomain, domainLen, pLsUdpPayload[0], 
                                    pLsUdpPayload[1] & NODE_ID_MASK);

            *p++ = pLsUdpPayload[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_SUBNET];
            *p++ = 0x80 | pLsUdpPayload[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE];
            // Skip source address.
            if (pLsUdpPayload[IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMFLAG] & IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_DMFLG) {
                pLsUdpPayload += domainLen + IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DM;
            } else {
                pLsUdpPayload += IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN;
            }
        } else
#endif
        {
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
            Ipv4SetDerivedAddressMapping(lsMappingHandle, pDomain, domainLen, pSourceAddr[IPV4_LSIP_UCADDR_OFF_SUBNET], 
            pSourceAddr[IPV4_LSIP_UCADDR_OFF_NODE] & NODE_ID_MASK);
#endif
            *p++ = pSourceAddr[IPV4_LSIP_UCADDR_OFF_SUBNET];
            *p++ = 0x80 | pSourceAddr[IPV4_LSIP_UCADDR_OFF_NODE];
        }

        switch(lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_ADDRFMT) {
#if UIP_CONF_IPV6
        case IPV6_LSUDP_NPDU_ADDR_FMT_NEURON_ID:
            npduHdr |= (ADDR_FORMAT_NEURONID << IPV6_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = UIP_IP_BUF->destipaddr.u8[IPV6_LSIP_UCADDR_OFF_SUBNET];
            memcpy(p, &UIP_IP_BUF->destipaddr.u8[IPV6_LSIP_UCADDR_OFF_NIDHI], IPV6_LSIP_UCADDR_NID_HILEN);
                   
            *p = (*p << 2) | (*p >> 6);
            p += IPV6_LSIP_UCADDR_NID_HILEN;
            memcpy(p, &UIP_IP_BUF->destipaddr.u8[IPV6_LSIP_UCADDR_OFF_NIDLO], IPV6_LSIP_UCADDR_NID_LOLEN);
            p += IPV6_LSIP_UCADDR_NID_LOLEN;
            break;
#endif
        case IPV4_LSUDP_NPDU_ADDR_FMT_BROADCAST_NEURON_ID:
            npduHdr |= (LT_AF_UNIQUE_ID << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = *pLsUdpPayload++;
            memcpy(p, pLsUdpPayload, IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN);
            p += IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN;
            pLsUdpPayload += IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN;
            break;
        case IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_NODE:
        case IPV4_LSUDP_NPDU_ADDR_FMT_GROUP_RESP:
            npduHdr |= (LT_AF_SUBNET_NODE << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = eep->domainTable[0].Subnet;
            *p++ = 0x80 | IZOT_GET_ATTRIBUTE(eep->domainTable[0], IZOT_DOMAIN_NODE);
            if ((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_ADDRFMT) == IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_NODE) {
                break; 
            }
            // Strip hi bit to indicate group response
            pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE] = pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE] & NODE_ID_MASK;  
            *p++ = *pLsUdpPayload++;
            *p++ = *pLsUdpPayload++;
            break; 
        case IPV4_LSUDP_NPDU_ADDR_FMT_DOMAIN_BROADCAST:
            npduHdr |= (LT_AF_BROADCAST << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = 0;
           break;
        case IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_BROADCAST:
            npduHdr |= (LT_AF_BROADCAST << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = *pLsUdpPayload++;
            break;
        case IPV4_LSUDP_NPDU_ADDR_FMT_GROUP:
            npduHdr |= (LT_AF_GROUP << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = *pLsUdpPayload++; // Note that this should be the same as UIP_IP_BUF->destipaddr.u8[IPV4_LSIP_MCADDR_OFF_GROUP]
            break;
        case IPV4_LSUDP_NPDU_ADDR_FMT_EXP_SUBNET_NODE:
            npduHdr |= (LT_AF_SUBNET_NODE << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE);
            *p++ = *pLsUdpPayload++;           // Subnet ID
            *p++ = 0x80 | *pLsUdpPayload;      // Node ID
            if (*pLsUdpPayload++ & 0x80) {
                // Strip hi bit to indicate group response
                pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE] &= NODE_ID_MASK;  
                *p++ = *pLsUdpPayload++;   // Group ID
                *p++ = *pLsUdpPayload++;   // Group member
            }
            break;
        default:
            // Unknown address type
             failed = 1;
             break;
        }
    }

    if (failed) {
        *pLtVxLen = 0;
    } else {
        uint16_t pduLen;
        // Calculate the pduLen by subtracting the UDP payload and 
        // LS/UDP headers from udplen.
        pduLen = udpLen - (pLsUdpPayload - pUdpPayload);
        pNpdu[IPV4_LTVX_NPDU_IDX_TYPE] = npduHdr;
        memcpy(p, pDomain, domainLen);
#if !UIP_CONF_IPV6
        // IPV4 address doesn not include the LSB of the domain, which MBZ
        if (domainLen > IPV4_LSIP_IPADDR_DOMAIN_LEN) {
            memset(p + IPV4_LSIP_IPADDR_DOMAIN_LEN, 0, domainLen - IPV4_LSIP_IPADDR_DOMAIN_LEN);
        }
#endif
        p += domainLen;
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
        if (((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_ADDRFMT) == IPV4_LSUDP_NPDU_ADDR_FMT_EXP_SUBNET_NODE) &&
        (((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PDUFMT) == ENCLOSED_PDU_TYPE_APDU) || 
        (((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PDUFMT) == ENCLOSED_PDU_TYPE_TPDU) && 
        ((*pLsUdpPayload & IPV4_LTVX_NPDU_MASK_SERVICE_TYPE) == IPV4_LTVX_NPDU_TPDU_TYPE_REPEATED)))) {
            // An unacked or repeated message that uses LS subnet node 
            // addressing, but includes the subnet/node address expliicitly. 
            // We won't be sending an ack or response, so even if there
            // is a better address to use the sending device won't learn it. 

            // Note that it should be sufficient to determine whether a unicast 
            // or multicast address  was used.  However, sockets doesn't really 
            // provide that information, so we can't always tell. So generate 
            // the ls derived IP address, and see if it is supported, and send 
            // the announcement in that case as well.

#if UIP_CONF_IPV6
            IzotByte lsDerivedAddr[16];
#else
            IzotByte lsDerivedAddr[4];
#endif
            // we know that the Vx message uses subnet node address, 
            // so we know where the domain ID is.
            Ipv4GenerateLsSubnetNodeAddr(&pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NODE + 1], domainLen, 
            pNpdu[IPV4_LTVX_NPDU_IDX_DEST_SUBNET], pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NODE], lsDerivedAddr);

            if (Ipv4IsUnicastAddressSupported(lsMappingHandle, lsDerivedAddr)) {
                IzotByte msg[IPV4_MAX_LTVX_UNICAST_ARB_ANNOUNCE_LEN];
                IzotByte len = 0;
                msg[len++] = 0;     // Pri, altpath backlog

                // version, pdu fmt, addfmt, domain len
                msg[len++] = (ENCLOSED_PDU_TYPE_APDU << IPV4_LTVX_NPDU_BITPOS_PDUFMT) |
                (LT_AF_SUBNET_NODE << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE) |
                (pNpdu[IPV4_LTVX_NPDU_IDX_TYPE] & IPV4_LTVX_NPDU_MASK_DOMAINLEN);

                // souce address
                msg[len++] = pNpdu[IPV4_LTVX_NPDU_IDX_DEST_SUBNET];
                msg[len++] = pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NODE] | 0x80;
                // dest address
                msg[len++] = pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_SUBNET];
                msg[len++] = pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE];
                // domain ID
                memcpy(&msg[len], &pNpdu[IPV4_LTVX_NPDU_IDX_DEST_NODE+1], domainLen);
                len += domainLen;
                msg[len++] = IPV4_EXP_MSG_CODE;  
                msg[len++] = IPV4_EXP_DEVICE_LS_ADDR_MAPPING_ANNOUNCEMENT;
                Ipv4SendAnnouncement(msg, len);                
            }
        }

        // Check for IPV4_EXP_SUBNETS_LS_ADDR_MAPPING_ANNOUNCEMENT
        if ((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PDUFMT) == ENCLOSED_PDU_TYPE_APDU && 
        pLsUdpPayload[0] == IPV4_EXP_MSG_CODE && pduLen >= (3 + 32) && pLsUdpPayload[1] == 
        IPV4_EXP_SUBNETS_LS_ADDR_MAPPING_ANNOUNCEMENT) {
            // This is IPV4_EXP_SUBNETS_LS_ADDR_MAPPING_ANNOUNCEMENT 
            // announcement;
            Ipv4SetDerivedSubnetsMapping(lsMappingHandle, p - domainLen, domainLen, pLsUdpPayload[2], &pLsUdpPayload[3]);
            
            // Do not need to send this packet to the network layer
            *pLtVxLen = 0;
            return;
        }
        
        if ((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PDUFMT) == ENCLOSED_PDU_TYPE_APDU && 
        pLsUdpPayload[0] == IPV4_EXP_MSG_CODE && pduLen >= 2 && pLsUdpPayload[1] == 
        IPV4_EXP_DEVICE_LS_ADDR_MAPPING_ANNOUNCEMENT) {
            // Announcement received, update mapping table if necessary
            UpdateMapping(p - domainLen, // Domain id
                domainLen, // Domain Length
                pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_SUBNET], // Subnet Id
                pNpdu[IPV4_LTVX_NPDU_IDX_SOURCE_NODE] & NODE_ID_MASK, // Node Id
                pSourceAddr); // Source ip Address
            
            // Do not need to send this packet to the network layer
            *pLtVxLen = 0;
            return;
        }
        
        if ((lsUdpHdr1 & IPV4_LSUDP_NPDU_MASK_PDUFMT) == ENCLOSED_PDU_TYPE_APDU && 
        pLsUdpPayload[0] == IPV4_EXP_MSG_CODE && pduLen >= 12 && pLsUdpPayload[1] == 
        IPV4_EXP_SET_LS_ADDR_MAPPING_ANNOUNCEMENT_PARAM) {
            // Stop the timer first
            SetLonTimer(&AnnouncementTimer, 0);
            SetLonTimer(&AgingTimer, 0);
            
            AnnounceTimer = pLsUdpPayload[2] << 24 || pLsUdpPayload[3] << 16 || 
            pLsUdpPayload[4] << 8 || pLsUdpPayload[5];
            AddrMappingAgingTimer = pLsUdpPayload[8] << 24 || pLsUdpPayload[9] << 16 || 
            pLsUdpPayload[10] << 8 || pLsUdpPayload[11];
                                    
            // Set the new timer
            SetLonTimer(&AnnouncementTimer, AnnounceTimer * 1000);
            SetLonTimer(&AgingTimer, AddrMappingAgingTimer * 1000);
        }
#endif
        memcpy(p, pLsUdpPayload, pduLen);

        // LTVX len is pduLen + NPDU header len.
        *pLtVxLen = pduLen + (p-pNpdu);
    }
}

#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
/* 
 *  Callback: Ipv4SendMulticastAnnouncement
 *  Send a multicast announcement that this device is using an arbitrary
 *  IP address.  This function contructs the message and then calls the 
 *  utility function Ipv4SendAnnouncement to do the actual send.
 *
 *  Parameters:
 *    lsSenderHandle:     A handle used for sending messages 
 *    pDesiredIpAddress:  Pointer LS derived IP address that this device should
 *                        ideally use.
 *
 *  Returns:
 *  <void>.   
 *
 */
static void Ipv4SendMulticastAnnouncement(const IzotByte *pDesiredIpAddress)
{
    IzotByte msg[IPV4_MAX_LTVX_BROADCAST_ARB_ANNOUNCE_LEN] = {0};
    IzotByte len = 0;
    IzotByte encodedDomainLen;
    const IzotByte *pDomainId = pDesiredIpAddress;
    msg[len++] = 0;     // Pri, altpath backlog

    // The domain is not included in arbitrary source address.  
    // Need to extract it from source or dest addr.
#if UIP_CONF_IPV6
    encodedDomainLen = 3;
#else
    if (memcmp(pDesiredIpAddress, ipv4_zero_len_domain_prefix, 
    sizeof(ipv4_zero_len_domain_prefix)) == 0) {
        encodedDomainLen = 0;
    } else if (pDesiredIpAddress[0] == IPV4_DOMAIN_LEN_1_PREFIX) {
        encodedDomainLen = 1;   // 1 byte domain
        pDomainId++;  // Skip the first byte.
    } else {
        encodedDomainLen = 2;   // 3 byte domain
    }
#endif
    // version, pdu fmt, addfmt, domain len
    msg[len++] = (ENCLOSED_PDU_TYPE_APDU << IPV4_LTVX_NPDU_BITPOS_PDUFMT) |
                 (LT_AF_BROADCAST << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE) | encodedDomainLen;

    // source address
    msg[len++] = pDesiredIpAddress[IPV4_LSIP_UCADDR_OFF_SUBNET];
    msg[len++] = pDesiredIpAddress[IPV4_LSIP_UCADDR_OFF_NODE] | 0x80;
    // dest subnet - domain wide broadcast uses 0.
    msg[len++] = 0;
    // domain ID
#if UIP_CONF_IPV6
    memcpy(&msg[len], pDesiredIpAddress, IPV6_LSIP_IPADDR_DOMAIN_LEN);
    len += IPV6_LSIP_IPADDR_DOMAIN_LEN;
#else
    // it just so happens that for IPV4 the encoded domain LEN is 
    // also equal to the number of bytes to copy.
    memcpy(&msg[len], pDomainId, encodedDomainLen);
    len += encodedDomainLen;
    if (encodedDomainLen == 2) {
        msg[len++] = 0; // Last byte is 0.
    }
#endif
    msg[len++] = IPV4_EXP_MSG_CODE;
    msg[len++] = IPV4_EXP_DEVICE_LS_ADDR_MAPPING_ANNOUNCEMENT; 
    Ipv4SendAnnouncement(msg, len);    
}
#endif
#endif

/* 
 *  Callback: Ipv4GenerateLsMacAddr
 *  Generate a multicast address for a LS broadcast or group address
 *
 *  Parameters:
 *    type:           The multicast group type: IPV4_LS_MC_ADDR_TYPE_BROADCAST 
 *                    or IPV4_LS_MC_ADDR_TYPE_GROUP
 *    pDomainId:      Pointer to the domain ID (IPV6 only)
 *    domainLen:      Length of the domain (0 to 6) (IPV6 only)
 *    subnetOrGroup:  LS subnet ID or group ID
 *    pAddr:      Pointer to IPV4 address to store the prefixturns:
 *
 *  Return:
 *  <void>.   
 *
 */
void Ipv4GenerateLsMacAddr(IzotByte type, 
#if UIP_CONF_IPV6
    const IzotByte *pDomainId, IzotByte domainLen, 
#endif
IzotByte subnetOrGroup, IzotByte *pAddr)
{
#if UIP_CONF_IPV6
    memset(pAddr, 0, sizeof(uip_ipaddr_t));
    if (domainLen > IPV6_LSIP_IPADDR_DOMAIN_LEN) {
        domainLen = 0;  // No domain...
    }
    *pAddr++ = 0xff;
    *pAddr++ = 0x18;
    memcpy(pAddr, pDomainId, domainLen);
    pAddr += 6;
#endif
    memcpy(pAddr, ipv4_ls_multicast_prefix, sizeof(ipv4_ls_multicast_prefix));
    pAddr += sizeof(ipv4_ls_multicast_prefix);
    *pAddr++ = type;
    *pAddr++ = subnetOrGroup;
}

/* 
 *  Callback: Ipv4GenerateLsSubnetNodeAddr
 *  Generate a unicast address for a LS subent/node address
 *
 *  Parameters:
 *     pDomainId:      Pointer to the domain ID
 *     domainLen:      Length of the domain (0 to 6)
 *     subnetId:       LS subnet ID 
 *     nodeId:         LS node ID
 *     pAddr:          Pointer to a buffer to store the IPV4 address
 * 
 *  Returns:
 *  <void>.   
 *
 */
void Ipv4GenerateLsSubnetNodeAddr(const IzotByte *pDomainId, IzotByte domainLen, 
IzotByte subnetId, IzotByte nodeId, IzotByte *pAddr
)
{                                     
#if UIP_CONF_IPV6
    Ipv4GenerateLsPrefix(pDomainId, domainLen, subnetId, pAddr);
    pAddr[15] = nodeId & NODE_ID_MASK;
#else
    nodeId &= NODE_ID_MASK ;
    if (domainLen > 6) {
        // An invalid domain. Set node  to 0.  
        // Will use the zero length domain below.
        domainLen = 0;  
        nodeId = 0;
    }
    if (subnetId == 0 || nodeId == 0) {
        domainLen = 0;
    }
    Ipv4GenerateLsPrefix(pDomainId, domainLen, subnetId, pAddr);
    pAddr[IPV4_LSIP_UCADDR_OFF_NODE] = nodeId;
#endif
}


/* 
 *  Callback: LsUDPReset
 *  To allocate space for link layer queues.
 *
 *  Parameters:
 *
 *  Returns:
 *  <void>.   
 *
 */
void LsUDPReset(void)
{
    IzotUbits16 queueItemSize;
    
    // Allocate and initialize the output queue.
    gp->lkOutBufSize  = DecodeBufferSize(LK_OUT_BUF_SIZE); //1280
    gp->lkOutQCnt     = DecodeBufferCnt((IzotByte)IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUTBUF_CNT));
    queueItemSize    = gp->lkOutBufSize + sizeof(LKSendParam) + 21;

    if (QueueInit(&gp->lkOutQ, queueItemSize, gp->lkOutQCnt) != SUCCESS) {
        DBG_vPrintf(TRUE, "LsUDPReset: Unable to init the output queue.\r\n");
        gp->resetOk = FALSE;
        return;
    }
    
    // Allocate and initialize the priority output queue.
    gp->lkOutPriBufSize = gp->lkOutBufSize; //1280
    gp->lkOutPriQCnt    = DecodeBufferCnt((IzotByte)IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NW_OUT_PRICNT));
    queueItemSize       = gp->lkOutPriBufSize + sizeof(LKSendParam);

    if (QueueInit(&gp->lkOutPriQ, queueItemSize, gp->lkOutPriQCnt) != SUCCESS) {
        DBG_vPrintf(TRUE, "LsUDPReset: Unable to init the priority output queue.\r\n");
        gp->resetOk = FALSE;
        return;
    }

    SetLonTimer(&AnnouncementTimer, AnnounceTimer*1000);
    SetLonTimer(&AgingTimer, AddrMappingAgingTimer*1000);

    return;
}

/* 
 *  Callback: LsUDPSend
 *  To take the NPDU from link layer's output queue and put it
 *  in the queue for the physical layer.
 *
 *  Parameters:
 *
 *  Returns:
 *  <void>.   
 *
 */
void LsUDPSend(void)
{
    LKSendParam   *lkSendParamPtr;
    Queue         *lkSendQueuePtr;
    IzotByte      *npduPtr;
    IzotByte       priority;
    IzotByte       LtVx2lsUdpPayload[MAX_PDU_SIZE]; //1280
    IzotByte       SourceAddr[IPV4_ADDRESS_LEN];// buffer to store source ip
    IzotByte       DestAddr[IPV4_ADDRESS_LEN];    // buffer to store dest ip
    uint16_t       lsUdpLen;                    // size of lsudp payload formed

    // Check for the announcement timer
    if (LonTimerExpired(&AnnouncementTimer)) {
        SendAnnouncement();
        // Set the announcement timer again
        SetLonTimer(&AnnouncementTimer, AnnounceTimer * 1000);
    }
    
    // Check for the LS/IP address mapping aging
    if (LonTimerExpired(&AgingTimer)) {
        ClearMapping();
        // Set the aging timer again
        SetLonTimer(&AgingTimer, AddrMappingAgingTimer * 1000);
    }
    
    
    // First, make variables point to the right queue.
    if (!QueueEmpty(&gp->lkOutPriQ)) {
        priority        = TRUE;
        lkSendQueuePtr  = &gp->lkOutPriQ;
    } else if (!QueueEmpty(&gp->lkOutQ)) {
        priority        = FALSE;
        lkSendQueuePtr  = &gp->lkOutQ;
    } else {
        return; // Nothing to send.
    }

    lkSendParamPtr = QueueHead(lkSendQueuePtr);
    npduPtr        = (IzotByte *) (lkSendParamPtr + 1);

    memset(&LtVx2lsUdpPayload[0], 0, MAX_PDU_SIZE); //1280
    LtVx2lsUdpPayload[0] = ((priority << 7) & 0x80)| lkSendParamPtr->deltaBL;
    
    IzotDomain *temp = &eep->domainTable[lkSendParamPtr->DomainIndex];
    
    // Prepare the LS derived source ip address
    Ipv4GenerateLsSubnetNodeAddr(temp->Id, (IzotByte)IZOT_GET_ATTRIBUTE_P(temp, IZOT_DOMAIN_ID_LENGTH), 
    temp->Subnet, (IzotByte)IZOT_GET_ATTRIBUTE_P(temp, IZOT_DOMAIN_NODE), SourceAddr);
    
    // Copy the NPDU.
    if (lkSendParamPtr->pduSize <= MAX_PDU_SIZE) {
        memcpy(&LtVx2lsUdpPayload[1], npduPtr, lkSendParamPtr->pduSize);
    } else {
        DeQueue(lkSendQueuePtr);
        return;
    }
    
    // Convert the LTVX payload into LSUDP payload and 
    // Set the destination IP address
    lsUdpLen = Ipv4ConvertLTVXtoLsUdp(
                LtVx2lsUdpPayload, // Ptr to LTVX PDU to be sent
                lkSendParamPtr->pduSize + 1, // Size of LTVX PDU to be sent
                SourceAddr, NULL, // LS derived source IP address
                DestAddr, NULL, // Destination IP address to be used
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
                &ls_mapping    // Mapping Handle
#endif
    );
    
    if (lsUdpLen == 0) {
        DeQueue(lkSendQueuePtr);
        return;
    }
    
    CalSend(IPV4_LS_UDP_PORT, DestAddr, LtVx2lsUdpPayload, lsUdpLen);
    
    DeQueue(lkSendQueuePtr);
    return;
}

/* 
 *  Callback: LsUDPReceive
 *  To receive the incoming LPDUs and process them.
 *
 *  Parameters:
 *
 *  Returns:
 *  <void>.   
 *
 */
void LsUDPReceive(void)
{
    NWReceiveParam      *nwReceiveParamPtr;
    IzotByte            *npduPtr;
    IzotByte             priority;
    int                  lsudpLen;
    IzotByte             SourceAddr[IPV4_ADDRESS_LEN] = {0};
    uint16_t             LtVxLen = 0;    // ltv0 payload length
    
    if (!QueueFull(&gp->nwInQ)) {
        nwReceiveParamPtr = QueueTail(&gp->nwInQ);
        npduPtr           = (IzotByte *)(nwReceiveParamPtr + 1);
    } else {
        return;
    }
    
    // Receive the udp data from UDP port
    lsudpLen = CalReceive(npduPtr, SourceAddr);
    
    if (lsudpLen > 0 && lsudpLen < 3) {
        INCR_STATS(LcsTxError);
        return;
    }
    
    // Do nothing
    if (lsudpLen <= 0) {
        return;
    }
    
    // Got a good packet.
    INCR_STATS(LcsL2Rx); 

    // Get the priority bit from LSUDP packet
    priority = npduPtr[1] & IPV4_LSUDP_NPDU_MASK_PRIORITY;
    
    // We need to receive this message
    if (QueueFull(&gp->nwInQ)) {
        // We are losing this packet
        INCR_STATS(LcsMissed);
    } else {
        // buffer to store LTVX payload
        IzotByte    LtVxPayload[MAX_PDU_SIZE];
         
        memset(&LtVxPayload[0], 0, MAX_PDU_SIZE);
        
        // Convert the LTV1 payload into LTV0 payload
        Ipv4ConvertLsUdptoLTVX(0, npduPtr, // Ptr to lsudp packet received
                    lsudpLen, // size of lsudp packet received
                    SourceAddr, // Source IP address
                    0, // Source Port
                    NULL, 0, // Destination IP address and Port
                    LtVxPayload, // Buffer to store LTVX PDU to be formed
                    &LtVxLen, //will be the size of LTVX pdu
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
                    &ls_mapping // Mapping handle
#endif
        );
        
        // Return if LtVxLen set to zero
        if (LtVxLen == 0) {
            return;
        }
#ifdef LSUDP_DEBUG
        int k;
        LSUDP_PRINTF("LTVX: %d byte recv: ", LtVxLen);
        for (k = 0; k < LtVxLen; k++) {
            LSUDP_PRINTF("%02X ", LtVxPayload[k]);
        }
        LSUDP_PRINTF("\r\n");wmstdio_flush();
#endif    
        nwReceiveParamPtr->priority = priority;
        nwReceiveParamPtr->altPath  = 0;
        nwReceiveParamPtr->pduSize  = LtVxLen - 1;
        
        // Copy the NPDU.
        // if it was in link layer's queue, then the size should be
        // sufficient in network layer's queue as they differ by 3.
        // However, let us play safe by checking the size first.
        if (nwReceiveParamPtr->pduSize <= gp->nwInBufSize) {
            memcpy(npduPtr, &LtVxPayload[1], nwReceiveParamPtr->pduSize);
            EnQueue(&gp->nwInQ);
        } else {
            ErrorMsg("LsUDPReceive: LSUDP packet size seems too large.\n");
            // We are losing this packet.
            INCR_STATS(LcsMissed);
        }
    }
    
    return;
}

/* 
 *  Callback: SendAnnouncement
 *  Sends the announcement in the network
 *
 *
 *  Returns:
 *  <void>.   
 *
 */
void SendAnnouncement(void)
{
    IzotDomain *temp = &eep->domainTable[0];
    SetCurrentIP();
    
    IzotByte    ls_derived_src_ip[IPV4_ADDRESS_LEN];
    Ipv4GenerateLsSubnetNodeAddr(
            (IzotByte *)temp->Id, 
                (IzotByte)IZOT_GET_ATTRIBUTE_P(temp, IZOT_DOMAIN_ID_LENGTH), 
                    (IzotByte)temp->Subnet, 
                        (IzotByte)IZOT_GET_ATTRIBUTE_P(temp, IZOT_DOMAIN_NODE), 
                            ls_derived_src_ip
                                );
    Ipv4SendMulticastAnnouncement(ls_derived_src_ip);
}

/* 
 *  Callback: SetLsAddressFromIpAddr
 *  Set the LS address from the IP address
 *
 *
 *  Returns:
 *  <void>.   
 *
 */
void SetLsAddressFromIpAddr(void)
{
    IzotDomain domain;
    
    memset(&domain, 0, sizeof(IzotDomain));
    
    if (ownIpAddress[0] == IPV4_DOMAIN_LEN_0_PREFIX_0 && ownIpAddress[1] == IPV4_DOMAIN_LEN_0_PREFIX_1) {
        IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_ID_LENGTH, 0);
    } else if (ownIpAddress[0] == IPV4_DOMAIN_LEN_1_PREFIX) {
        IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_ID_LENGTH, 1); // 1 byte domain
        domain.Id[0] = ownIpAddress[1];   // 1 byte domain
    } else {
        IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_ID_LENGTH, 3);  // 3 byte domain
        domain.Id[0] = ownIpAddress[0];
        domain.Id[1] = ownIpAddress[1];
        domain.Id[2] = 0;
    }
    
    if ((ownIpAddress[2] >= 1 && ownIpAddress[2] <= 255) && (ownIpAddress[3] >= 1 && ownIpAddress[3] <= 127)) {
        domain.Subnet = ownIpAddress[2];
        // 3 byte domain
        IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_NODE, ownIpAddress[3]);  
        IZOT_SET_ATTRIBUTE(domain, IZOT_DHCP_FLAG, 1);
        IZOT_SET_ATTRIBUTE(domain, IZOT_LS_MODE, 0);
    } else {
        domain.Subnet = (rand() % 254) + 1;
        IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_NODE, (rand() % 126) + 1);
        IZOT_SET_ATTRIBUTE(domain, IZOT_DHCP_FLAG, 0);
        IZOT_SET_ATTRIBUTE(domain, IZOT_LS_MODE, 1);
    }

    IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_INVALID, 0);
    IZOT_SET_ATTRIBUTE(domain, IZOT_DOMAIN_NONCLONE, 1);

    IZOT_SET_ATTRIBUTE(domain, IZOT_DHCP_FLAG, 1);
    IZOT_SET_ATTRIBUTE(domain, IZOT_AUTH_TYPE, AUTH_OMA);
    
    UpdateDomain(&domain, 0, 0);
    RecomputeChecksum();
    LCS_WriteNvm();
}

/* 
 *  Callback: UdpInit
 *  Initialize the Izot Framework with the relevat details
 *
 *
 *  Returns:
 *  LonApiError on unsuccessful exit LonApiNoError otherwise.
 *
 */
int UdpInit(void)
{
    int ret = IzotApiNoError;
    int err;
    
#if PROCESSOR_IS(MC200)
    // Init the debug UART
    wmstdio_init(UART0_ID, 0);
    
    // Init the wlan service
    err = wm_wlan_init();
    if (err != IzotApiNoError) {
        return err;
    }
#endif  // PROCESSOR_IS(MC200)

    // Init the LCS Stack
    LCS_Init(IzotPowerUpReset);
    
#if PROCESSOR_IS(MC200)
    // Start the wlan
    ret = CalStart();
    if (ret != IzotApiNoError) {
        return ret;
    }

    // Init the udp socket for communication
    ret = InitSocket(IPV4_LS_UDP_PORT); 
    if (ret < 0) {
        DBG_vPrintf(TRUE, "Sockets not created\r\n");
        return IzotApiNoIpAddress;
    }
    DBG_vPrintf(TRUE, "Sockets created\r\n");
#endif  // PROCESSOR_IS(MC200)
  
    // restore multicast membership
    RestoreIpMembership();
    
    return ret;
}
