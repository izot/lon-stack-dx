/*
 * ipv4_to_lon_udp.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   IPv4 to LON/IP UDP Conversion
 * Purpose: Provides functions to convert between IPv4 UDP packets
 *          and LON/IP UDP packets, and manage address mappings.
 */

#ifndef _IPV4_LS_TO_UDP_H
#define _IPV4_LS_TO_UDP_H

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif  /* __cplusplus */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "izot/IzotPlatform.h"
#include "izot/lon_types.h"
#include "abstraction/IzotCal.h"
#include "lcs/lcs.h"

#ifndef USE_UIP
#define USE_UIP
#endif

#ifdef USE_UIP
#define IPV4_INCLUDE_LTVX_LSUDP_TRANSLATION 1  
#define IPV4_SUPPORT_ARBITRARY_ADDRESSES    1
#endif

#ifdef LSUDP_DEBUG
	#define LSUDP_PRINTF(format,args...) wmprintf(args)
#else
	#define LSUDP_PRINTF(format,args...)	;
#endif

#define NODE_ID_MASK                   0x7F

// Time To Live for IPV4 Multicast.
// Restricted to the same site, organization or department
#define IPV4_MC_TTL_FOR_IPV4           32 

// LonTalk Service multicast type.  This is found in offset 14 of an
// LS MC address
#define IPV4_LS_MC_ADDR_TYPE_BROADCAST 0
#define IPV4_LS_MC_ADDR_TYPE_GROUP     1

// UDP port used for LS/UDP
// Note that this port was originaly allocated for use 
// by LNS remote lightweight clients.  However LNS only uses this port for TCP, 
// so LS/UDP can use this port for UDP.
#define IPV4_LS_UDP_PORT               2541

// The IPV4_LTVX_NPDU_IDX_ definitions represent the byte offset of the first 
// several fields in a version 0 NPDU.
//
// The following definitions are used to access fields within a LTVx NPDU
//     
// | 1 |   1   |   6   | 2 | 2 | 2 |   2   |  2  |    8    |1|  7    |Variable|0/8/24/48|Variable|
// |===|=======|=======|===|===|===|=======|=====|=========|=|=======|========|=========|========|
// |Pri|AltPath|DeltaBl|Ver|PDU|Fmt|AddrFmt|DmLen|SrcSubnet|f|SrcNode|DestAddr| Domain  |EnclPDU |
// |==============================================================================================
//
// DestAddr has one of the following forms
//
//  Broacast (f = 1):  Group (f = 1):   Subnet/Node (f = 1)   
//      |  8   |          |  8   |       |  8   |1| 7  |      
//      |======|          |======|       |======|=|====|      
//      |subnet|          |group |       |subnet|1|Node|      
//      ========          ========       ===============      
//
//  Subnet/Node (f = 0) - for group responses   NeuronID (f = 1)
//   |  8   |1| 7  |   8   |   8    |            |  8   |   48   |  
//   |======|=|====|=======|========|            |======|========|  
//   |subnet|1|Node|GroupID|GroupMbr|            |subnet|NeuronID|  
//   ================================            =================  


#define IPV4_LTVX_NPDU_IDX_PRIDELTA       0
#define IPV4_LTVX_NPDU_IDX_TYPE           1
#define IPV4_LTVX_NPDU_IDX_SOURCE_SUBNET  2
#define IPV4_LTVX_NPDU_IDX_SOURCE_NODE    3
#define IPV4_LTVX_NPDU_IDX_DEST_ADDR      4
#define IPV4_LTVX_NPDU_IDX_DEST_SUBNET    IPV4_LTVX_NPDU_IDX_DEST_ADDR
#define IPV4_LTVX_NPDU_IDX_DEST_NODE      (IPV4_LTVX_NPDU_IDX_DEST_ADDR + 1)
#define IPV4_LTVX_NPDU_IDX_DEST_GROUP     IPV4_LTVX_NPDU_IDX_DEST_ADDR
#define IPV4_LTVX_NPDU_IDX_DEST_NEURON_ID (IPV4_LTVX_NPDU_IDX_DEST_ADDR + 1)

#define IPV4_LTVX_NPDU_IDX_DEST_NODE_MASK 0x7f
// The group ID contained in a response (subnet/node address)
#define IPV4_LTVX_NPDU_IDX_RESP_GROUPID  (IPV4_LTVX_NPDU_IDX_DEST_ADDR+2) 
// The group member contained in a response     
#define IPV4_LTVX_NPDU_IDX_RESP_GROUPMBR (IPV4_LTVX_NPDU_IDX_RESP_GROUPID+1)    

#define IPV4_LTVX_NPDU_DEST_NEURON_ID_LEN   6


// The following definitions are used to access fields within a LTVx NPDU.  
// Typically a bit position (BITPOS) and mask value are provided for each field
// Byte 0 - IPV4_LTVX_NPDU_IDX_PRIDELTA
// | 1 |   1   |   6   |
// |===|=======|=======|
// |Pri|AltPath|DeltaBl|
// |====================
#define IPV4_LTVX_NPDU_BITPOS_DELTA_BACKLOG 0
#define IPV4_LTVX_NPDU_MASK_DELTA_BACKLOG   \
                                  (0x3f << IPV4_LTVX_NPDU_BITPOS_DELTA_BACKLOG)
#define IPV4_LTVX_NPDU_BITPOS_PRIORITY     7
#define IPV4_LTVX_NPDU_MASK_PRIORITY       (1 << IPV4_LTVX_NPDU_BITPOS_PRIORITY)

// Byte 1 - IPV4_LTVX_NPDU_IDX_TYPE

// | 2 | 2 | 2 |   2   |  2  | 
// |===|===|===|=======|=====|
// |Ver|PDU|Fmt|AddrFmt|DmLen|
// ===========================
#define IPV4_LTVX_NPDU_BITPOS_DOMAINLEN     0
#define IPV4_LTVX_NPDU_BITPOS_ADDRTYPE      2
#define IPV4_LTVX_NPDU_BITPOS_PDUFMT        4
#define IPV4_LTVX_NPDU_BITPOS_VER           6
#define IPV4_LTVX_NPDU_MASK_DOMAINLEN  (0x03 << IPV4_LTVX_NPDU_BITPOS_DOMAINLEN)
#define IPV4_LTVX_NPDU_MASK_ADDRTYPE   (0x03 << IPV4_LTVX_NPDU_BITPOS_ADDRTYPE)
#define IPV4_LTVX_NPDU_MASK_PDUFMT     (0x03 << IPV4_LTVX_NPDU_BITPOS_PDUFMT) 
#define IPV4_LTVX_NPDU_MASK_VER        (0x03 << IPV4_LTVX_NPDU_BITPOS_VER) 

#define IPV4_GET_ADDRESS_FORMAT_FROM_NPDU(npdu)  \
    (((npdu)[IPV4_LTVX_NPDU_IDX_TYPE] & IPV4_LTVX_NPDU_MASK_ADDRTYPE) >> IPV4_LTVX_NPDU_BITPOS_ADDRTYPE)
#define IPV4_GET_PDU_FORMAT_FROM_NPDU(npdu)  \
    (((npdu)[IPV4_LTVX_NPDU_IDX_TYPE] & IPV4_LTVX_NPDU_MASK_PDUFMT) >> IPV4_LTVX_NPDU_BITPOS_PDUFMT)

#define ENCLOSED_PDU_TYPE_TPDU 0
#define ENCLOSED_PDU_TYPE_SPDU 1
#define ENCLOSED_PDU_TYPE_AUTH 2
#define ENCLOSED_PDU_TYPE_APDU 3

// Supported LT versions

// 4 bit transaction IDs
#define IPV4_LT_VER_LEGACY          0
// Arbitrary IP traffic on a native LonTalk link
#define IPV4_LT_VER_ENCAPSULATED_IP 1
// 12 bit transaction IDs.
#define IPV4_LT_VER_ENHANCED        2

// Some LonTalk links compress arbitrary UDP packets usng LS enhanced mode
#define IPV4_LT_VER_ARB_UDP         IPV4_LT_VER_ENHANCED  

#define IPV4_LT_VER_MATCHES(value, ver) \
    ((value & IPV4_LTVX_NPDU_MASK_VER) == ((ver) << IPV4_LTVX_NPDU_BITPOS_VER))
#define IPV4_LT_IS_VER_LS_LEGACY_MODE(value) \
    IPV4_LT_VER_MATCHES(value, IPV4_LT_VER_LEGACY)
#define IPV4_LT_IS_VER_LS_ENHANCED_MODE(value) \
    IPV4_LT_VER_MATCHES(value, IPV4_LT_VER_ENHANCED)
#define IPV4_LT_IS_VER_LS_ENCAPSULATED_IP(value) \
    IPV4_LT_VER_MATCHES(value, IPV4_LT_VER_ENCAPSULATED_IP)
#define IPV4_LT_IS_VER_ARB_UDP(value) \
    IPV4_LT_VER_MATCHES(value, IPV4_LT_VER_ARB_UDP)

#define IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE  4
#define IPV4_LTVX_NPDU_MASK_SERVICE_TYPE   \
    (3 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)

// TPDU types
#define IPV4_LTVX_NPDU_TPDU_TYPE_ACKD      00
#define IPV4_LTVX_NPDU_TPDU_TYPE_REPEATED \
    (0x01 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)
#define IPV4_LTVX_NPDU_TPDU_TYPE_ACK      \
    (0x02 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)
#define IPV4_LTVX_NPDU_TPDU_TYPE_REMINDER \
    (0x04 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)
#define IPV4_LTVX_NPDU_TPDU_TYPE_REMMSG   \
    (0x05 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)

// SPDU types
#define IPV4_LTVX_NPDU_SPDU_TYPE_REQUEST  00
#define IPV4_LTVX_NPDU_SPDU_TYPE_RESPONSE \
    (0x02 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)
#define IPV4_LTVX_NPDU_SPDU_TYPE_REMINDER \
    (0x04 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)
#define IPV4_LTVX_NPDU_SPDU_TYPE_REMMSG   \
    (0x05 << IPV4_LTVX_NPDU_BITPOS_SERVICE_TYPE)


// The first two bytes of the LIFT link layer header have the 
// following format for V1 packets
// | 8 | 2 |    6    |
// |===|===|=========|
// |00 | 1 | PktType |
// ===================

#define IPV4_LTV1_LINKHDR_MASK_PKTTYPE 0x3f
#define IPV4_GET_LTV1_LINKHDR_PKTTYPE(pHdr) \
    (pHdr[1] & IPV4_LTV1_LINKHDR_MASK_PKTTYPE)

#define IPV4_LTV1_LINKHDR_PKTTYPE_IPV4 0
#define IPV6_LTV1_LINKHDR_PKTTYPE_IPV6 1

#if UIP_CONF_IPV6
#define IPV6_LTV1_LINKHDR_PKTTYPE_MYIP IPV6_LTV1_LINKHDR_PKTTYPE_IPV6
#else
#define IPV4_LTV1_LINKHDR_PKTTYPE_MYIP IPV4_LTV1_LINKHDR_PKTTYPE_IPV4
#endif
#define IPV4_LTV1_LINKHDR_PKT_HEADER_VALID(pHdr) \
    (pHdr[0] == 0 && IPV4_GET_LTV1_LINKHDR_PKTTYPE(pHdr) == IPV4_LTV1_LINKHDR_PKTTYPE_MYIP)

// Byte 1 - IPV4_LTVX_NPDU_IDX_TYPE

// | 2 |   2  |   2   |  2  |
// |===|======|=======|=====|
// |Ver|PDUFmt|AddrFmt|DmLen|
// ==========================


// The following definitions are used to access fields within a LS/UDP NPDU
//     
// |  4   |  4  |   4   | 1 | 1 |   2   | 0 or 16 | 0 or 8 | 0 or 8  | 0 or 48|variable|
// |======|=====|=======|===|===|=======|=========|========|=========|========|========|
// |UdpVer|Flags|AddrFmt|MCR|Pri|PDU Fmt| BlInfo  | Group  | Grp Mbr |NeuronId|Encl.PDU|
// |===================================================================================|
//
 
// Byte 0 
// |  4   |  3  | 1| 
// |======|=====|==|
// |UdpVer|Flags|SF|
// |================
//
#define IPV4_LSUDP_UDP_VER_LS_LEGACY         0
#define IPV4_LSUDP_UDP_VER_LS_ENHANCED       1
#define IPV4_LSUDP_UDP_VER_CURRENT           IPV4_LSUDP_UDP_VER_LS_ENHANCED

#define IPV4_LSUDP_NPDU_BITPOS_ARB_SOURCE    0   
#define IPV4_LSUDP_NPDU_BITPOS_FLAGS         1
#define IPV4_LSUDP_NPDU_BITPOS_UDPVER        4
#define IPV4_LSUDP_NPDU_MASK_ARB_SOURCE      \
    (0x01 << IPV4_LSUDP_NPDU_BITPOS_ARB_SOURCE)
#define IPV4_LSUDP_NPDU_MASK_FLAGS           \
    (0x07 << IPV4_LSUDP_NPDU_BITPOS_FLAGS)
#define IPV4_LSUDP_NPDU_MASK_UDPVER          \
    (0x0f << IPV4_LSUDP_NPDU_BITPOS_UDPVER)

// Byte 1 
// |   4   | 1 | 1 |   2   | 
// |=======|===|===|=======|
// |AddrFmt|MCR|Pri|PDU Fmt| 
// =========================
//
#define IPV4_LSUDP_NPDU_BITPOS_PDUFMT        0
#define IPV4_LSUDP_NPDU_BITPOS_PRIORITY      2
#define IPV4_LSUDP_NPDU_BITPOS_MCR           3
#define IPV4_LSUDP_NPDU_BITPOS_ADDRFMT       4
#define IPV4_LSUDP_NPDU_MASK_PDUFMT          \
    (0x03 << IPV4_LSUDP_NPDU_BITPOS_PDUFMT)
#define IPV4_LSUDP_NPDU_MASK_PRIORITY        \
    (0x01 << IPV4_LSUDP_NPDU_BITPOS_PRIORITY)
#define IPV4_LSUDP_NPDU_MASK_MCR             \
    (0x01 << IPV4_LSUDP_NPDU_BITPOS_MCR)
#define IPV4_LSUDP_NPDU_MASK_ADDRFMT         \
    (0x0f << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)

// The BlInfo field - Bytes 2 and 3 (optional)
// | 2  |   6   |   8   |
// |====|=======|=======|
// |Rsvd|DeltaBl|RspTime|
// ======================
#define IPV4_LSUDP_NPDU_IDX_BLINFO 2
#define IPV4_LSUDP_NPDU_MASK_DELTA_BACKLOG IPV4_LTVX_NPDU_MASK_DELTA_BACKLOG

// The arbitrary source adderess, appears after the BlInfo record.  Present if
// IPV4_LSUDP_NPDU_MASK_ARB_SOURCE is set.
//
// |   8    |1|   7  |
// |========|=|======|
// |SubnetId|0|NodeId|
// |==================
// 
// |   8    |1|   7  |  6 |  2  |variable|
// |========|=|======|====|=====|========|
// |SubnetId|1|NodeId|Rsvd|DmLen|DomainID|
// |======================================
// Offsets relative to begining of arbitrary source address
#define IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_SUBNET    0
#define IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE      \
    (IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_SUBNET + 1)
#define IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMFLAG    \
    (IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE + 0)
#define IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN     \
    (IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE + 1)
#define IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DM        \
    (IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN + 1)

// At offset IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_NODE
#define IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_NODE     0x7f
// At offset IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMFLAG
#define IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_DMFLG    0x80
// At offset IPV4_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN
#define IPV4_LSUDP_NPDU_MASK_ARB_SOURCE_DMLEN    3

// LSUDP address formats
#define IPV4_LSUDP_NPDU_ADDR_FMT_NEURON_ID           \
    (0 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_NODE         \
    (1 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_DOMAIN_BROADCAST    \
    (2 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_SUBNET_BROADCAST    \
    (3 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_GROUP               \
    (4 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_GROUP_RESP          \
    (5 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_BROADCAST_NEURON_ID \
    (6 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)
#define IPV4_LSUDP_NPDU_ADDR_FMT_EXP_SUBNET_NODE     \
    (7 << IPV4_LSUDP_NPDU_BITPOS_ADDRFMT)

#if UIP_CONF_IPV6
// Offsets within IPV6 unicast addresses

// Subnet/node address
// |<---- 48 ---->| 8  |<----  8 ---->|<----- 56 ----->|<---- 8 --->|
// ==================================================================
// |    DomainId  | 00 | LS subnet ID | 00000000000000 | LS Node ID |
// ==================================================================
#define IPV6_LSIP_UCADDR_OFF_SUBNET 7
#define IPV6_LSIP_UCADDR_OFF_NODE   15
#define IPV6_LSIP_UCADDR_OFF_DOMAIN 0
// The size of the domain in the LTVX packet
#define IPV6_LSIP_LTVx_DOMAIN_LEN         6
// The encoded size of the domain in the LTVx packet
#define IPV6_LSIP_LTVx_DOMAIN_LEN_ENCODED 3
// The size of the domain in the IP address
#define IPV6_LSIP_IPADDR_DOMAIN_LEN       6
#else

// Offsets within IPV4 unicast addresses

// Unicast addressing in LS/IPV4 uses a 2 byte prefix based on the domain,
// followed by the LS subnet and LS node.
//
// |<------ 16 ------>|<----  8 ---->|<--- 56 --->|
// ================================================
// |   Domain Prefix  | LS subnet ID | LS Node ID |
// ================================================
//
// LS/IPV4 supports 3 domain lengths, 0, 1 and 3.  However, the third
// byte of 3 byte domain ID must always be 0 and 
// is ommitted from the IP address.
// Let "d1" and "d2" represent the first and second bytes of the domain ID, 
// "s" the LS subnet ID and "n" the LS node ID.  The format of the IP address
// for each  supported domain length is:
//
//      Domain len      Format
//          0           192.168.s.n
//          1           10.d1.s.n
//          3           d1.d2.s.n
//

#define IPV4_LSIP_UCADDR_OFF_SUBNET 2
#define IPV4_LSIP_UCADDR_OFF_NODE   3
#define IPV4_LSIP_UCADDR_OFF_DOMAIN 0
// Support domain lengths 0, 1 and 3.
// The encoded size of the zero len domain.  Translates to 192.168.x.x
#define IPV4_LSIP_LTVX_DOMAIN_LEN_0_ENCODED 0
// The encoded size of a 1 byte domain.  Translates to D1.D1.x.x  
#define IPV4_LSIP_LTVX_DOMAIN_LEN_1_ENCODED 1
// The encoded size of a 3 byte domain.  Last byte MBZ.  Translates to D1.D2.x.x
#define IPV4_LSIP_LTVX_DOMAIN_LEN_3_ENCODED 2
// The size of the domain in the IP address - lsb is 0.
#define IPV4_LSIP_IPADDR_DOMAIN_LEN       2
#endif

#if UIP_CONF_IPV6
/* 
// // Neuron ID address
// |<---- 48 ---->|  8  |      8    |<------------ 64 ------------>|
// ================================================================= 
// | LS Domain ID | 00  | LS Subnet | EUI-64 derived from NeuronId |
// =================================================================
//  
//        |0      0|       1|1      2|2      3|3      3|4      4|
//        |0      7|       5|6      3|4      1|2      9|0      7|
//        +--------+--------+--------|--------+--------+--------+
//        |mmmmmmmm|ssssssss|ssssssss|ssssssss|ssssssss|bbbbbbbb| Neuron ID
//        +--------+--------+--------+--------+--------+--------+
//         || /        /        /          \         \       \
//         ||/        /        /            \         \       \
//         ||        /        /              \         \       \
//         ||       /        /                \         \       \
//        /||      /        /                  \         \       \
//       / ||     /        /                    \         \       \
//   |0 /  ||    /   1|1   |          3|3        \     4|4 \       \    6|
//   |0v   vv   v    5|6   v          1|2         v    7|8  v       v   3|
//   +----------------+----------------+----------------+----------------+
//   |mmmmmmugssssssss|ssssssss11111111|11111110ssssssss|ssssssssbbbbbbbb| IID
//   +----------------+----------------+----------------+----------------+
//          ^^
//      u---+|
//      g----+
*/  
#define IPV6_LSIP_UCADDR_OFF_NIDHI  8
#define IPV6_LSIP_UCADDR_OFF_NIDLO  13
#define IPV6_LSIP_UCADDR_NID_HILEN  3
#define IPV6_LSIP_UCADDR_NID_LOLEN  3

/* Offsets within IPV6 unicast addresses
//
//  |  16  |<-- 48  -->|<---- 48 ---->|<---- 8 ---->|<-------- 8 ------->|
//  ======================================================================
//  | FF18 | Domain ID | 4C5349505636 | AddressType | LS Subnet or Group |
//  ======================================================================
*/
#define IPV6_LSIP_MCADDR_OFF_ADDR_TYPE 14
#define IPV6_LSIP_MCADDR_OFF_SUBNET 15
#define IPV6_LSIP_MCADDR_OFF_GROUP  15
#define IPV6_LSIP_MCADDR_OFF_DOMAIN 2
#else
#define IPV4_LSIP_MCADDR_OFF_ADDR_TYPE 2
#define IPV4_LSIP_MCADDR_OFF_SUBNET 3
#define IPV4_LSIP_MCADDR_OFF_GROUP  3
#endif

// These are used on neuron to turn off certain warnings...
#ifndef NEURON_IPV4_WARNOFF_NO_EFFECT
#define NEURON_IPV4_WARNOFF_NO_EFFECT
#define NEURON_IPV4_WARNON_NO_EFFECT
#endif

#define IPV4_ADDRESS_LEN 4
#define IPV6_ADDRESS_LEN 16
#if UIP_CONF_IPV6
#define IPV6_MAX_IP_ADDRESS_LEN IPV6_ADDRESS_LEN
#else
#define IPV4_MAX_IP_ADDRESS_LEN IPV4_ADDRESS_LEN
#endif

#define IPV4_MAX_ARBITRARY_SOURCE_ADDR_LEN 9

// Allow room for subent/node address, 6 byte domain and 2 byte msg code.
#define IPV4_MAX_LTVX_UNICAST_ARB_ANNOUNCE_LEN \
    (IPV4_LTVX_NPDU_IDX_DEST_NODE+1+6+2)

// Allow room for 6 byte domain broadcast address and 2 byte msg code.
#define IPV4_MAX_LTVX_BROADCAST_ARB_ANNOUNCE_LEN \
    (IPV4_LTVX_NPDU_IDX_DEST_SUBNET+1+6+2)


// Announcment message.  The first byte is IPV4_EXP_MSG_CODE.
#define IPV4_EXP_MSG_CODE                             0x60
// The sub-command code.
// Announce LS address. Content doesn't matter, source addr format does.
#define IPV4_EXP_DEVICE_LS_ADDR_MAPPING_ANNOUNCEMENT  0x15
// Announce subnets using LS derived IP addresses
#define IPV4_EXP_SUBNETS_LS_ADDR_MAPPING_ANNOUNCEMENT 0x16
// Announcement period, Throtlle, Aging Period
#define IPV4_EXP_SET_LS_ADDR_MAPPING_ANNOUNCEMENT_PARAM 0x17

/*
 *
 * V0/V2 Arbitrary UDP packet Compresssion 
 *
 */

// The following definitions are used to access fields within the APDU of a 
// compressed arbitrary UPD packet
//     
// | 8 | 1 | 3 | 3 | 1 | 0-128 | 0-128 | 0/16  |   16  | Variable    |
// |===|===|===|===|===|=======|=======|=======|=======|=============|
// | 4F|MBZ|SAC|DAC|SPE|SrcAddr|DstAddr|SrcPort|DstPort| UDP Payload |
// ===================================================================
//

// This message code is used by the LonTalk applicaion to send and receive 
// UDP messages.  They are presented as LonTalk application messages using
// the Ipv4UdpAppMsgHdr, followed by the UDP payload.
#define IPV4_UDP_APP_MSG_CODE           0x4f    

// The following define the commpression bits
// The bit position of the Source Address Compression value
#define IPV4_ARB_UDP_SAC_BITPOS         4
// The bit position of the Desitination Address Compression value
#define IPV4_ARB_UDP_DAC_BITPOS         1
// The bit position of the Source Port Elided Flag
#define IPV4_ARB_UDP_SPE_BITPOS         0

#define IPV4_ARB_UDP_SAC_MASK           (0x7 << IPV4_ARB_UDP_SAC_BITPOS)
#define IPV4_ARB_UDP_DAC_MASK           (0x7 << IPV4_ARB_UDP_DAC_BITPOS)
#define IPV4_ARB_UDP_SPE_MASK           (0x1 << IPV4_ARB_UDP_SPE_BITPOS)

// The maximum sized NPDU header for a compressed arbitrary UPD packet. 
// Addr mode is subnet/node (2 byte dest)
// or broadcast (1 byte dest).
#define IPV4_MAX_COMPRESSED_ARB_UDP_NPDU_HDR \
                (2 + /* Priority/Delta + Ver/pduFm/addrFmt/dmLen */\
                 4 + /* Source subnet/node, dest subnet/node */ \
                 6   /* 6 byte domain ID */)

// The maximum for the UDP header in a compressed packet, assuming nothing is elided */
#define IPV4_MAX_COMPRESSED_ARB_UDP_HDR_LEN \
                            (2 + /* msg code + compression flags*/ \
                             2 * IPV4_MAX_IP_ADDRESS_LEN + \
                             4  /* 2 ports */)

// The maximum size of the NPDU header + the UDP header 
#define IPV4_MAX_COMPRESSED_ARB_UDP_OVERHEAD \
    (IPV4_MAX_COMPRESSED_ARB_UDP_NPDU_HDR + IPV4_MAX_COMPRESSED_ARB_UDP_HDR_LEN)

/*
 *
 * External data
 *
 */
#if UIP_CONF_IPV6
// The Lontalk services multicast prefix which appears at offsets 8-13 of
// an LS MC adderess
extern const IzotByte ipv6_ls_multicast_prefix[6];
// Pointer to my IP prefix
uip_ds6_prefix_t *pIpv6LsDomainUipPrefix;

#else
#define IPV4_DOMAIN_LEN_1_PREFIX 10
#define IPV4_DOMAIN_LEN_0_PREFIX_0 192
#define IPV4_DOMAIN_LEN_0_PREFIX_1 168
#endif 


/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/

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
extern void Ipv4GenerateLsMacAddr(IzotByte type, 
#if UIP_CONF_IPV6
const IzotByte *pDomainId, IzotByte domainLen, 
#endif
IzotByte subnetOrGroup, IzotByte *pAddr
);

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
extern void Ipv4GenerateLsSubnetNodeAddr(const IzotByte *pDomainId, 
IzotByte domainLen, IzotByte subnetId, IzotByte nodeId, IzotByte *pAddr
);

/* 
 *  Function: SendAnnouncement
 *  Send a LON/IP address announcement to the network
 *
 *  Returns:
 *  <void>.   
 */
extern void SendAnnouncement(void);

/* 
 *  Callback: UdpInit
 *  Initialize the Izot Framework with the relevant details
 *
 *
 *  Returns:
 *  LonApiError on unsuccessful exit LonApiNoError otherwise.
 *
 */
extern int UdpInit(void);

/* 
 *  Callback: SetLsAddressFromIpAddr
 *  Set the LS address from the IP address
 *
 *
 *  Returns:
 *  <void>.   
 *
 */
extern void SetLsAddressFromIpAddr(void);

/*
 * Function:   UpdateMapping
 * Update the mapping table based on the information in announcement 
 * received
 * 
 * pDomainId:        	Domain id of device
 * domainLen:        	Domain length of device 
 * subnetId:			Subnet id of device
 * nodeId:				Node Id of device
 * addr:				Absolute Ip address of device from which
 * 					    announcement is received
 *
 */
extern void UpdateMapping(IzotByte *pDomainId, IzotByte domainLen, 
IzotByte subnetId, IzotByte nodeId, const IzotByte *addr);

#if IPV4_INCLUDE_LTVX_LSUDP_TRANSLATION
#if IPV4_SUPPORT_ARBITRARY_ADDRESSES
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
extern IzotByte Ipv4GetArbitrarySourceAddress(void *lsMappingHandle,
IzotByte *pSourceIpAddress, const IzotByte *pDomainId, 
int domainIdLen, IzotByte *pEnclosedSource);

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
extern IzotByte Ipv4GetArbitraryDestAddress(
void *lsMappingHandle,
const IzotByte *pDomainId, 
IzotByte domainLen, 
IzotByte subnetId, 
IzotByte nodeId, 
IzotByte ipv1AddrFmt,
IzotByte *pDestIpAddress, 
IzotByte *pEnclosedDest
);

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
);

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
);

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
);

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
);

#endif // IPV4_SUPPORT_ARBITRARY_ADDRESSES
#endif

/*
 * Function:   ClearMapping
 * clear the mapping tabel after aging period expires
 * 
 * Parameters: None
 *
 */
extern void ClearMapping(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
