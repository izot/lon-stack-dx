
/*
 * isi_int.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   ISI Macro and Type Definitions
 * Purpose: Defines macros and types used internally by the ISI engine.
 * Notes:   This file is included by isi/isi.c and other ISI source files.
 *          It is not included by application code.
 */

#ifndef __ISIINT_H__
#define	__ISIINT_H__

#include <string.h>
#include "izot/IzotIsiApi.h"

#ifdef  __cplusplus
extern "C" {
#endif

// Define ISI_COMPACT for the small version (will be FULL if COMPACT is not defined)
#ifdef IsiCompactAuto
#define ISI_COMPACT
#endif
#ifdef IsiCompactManual
#define ISI_COMPACT
#endif

#ifdef ISI_COMPACT
#ifdef IsiCompactManual
#define ISI_SUPPORT_MANUAL_CONNECTIONS
#else
#define ISI_SUPPORT_AUTOMATIC_CONNECTIONS
#endif
#else  // Not compact (aka FULL):
#define ISI_SUPPORT_HEARTBEATS
#define ISI_SUPPORT_CONNECTION_REMOVAL
#define ISI_SUPPORT_ALIAS
#define ISI_SUPPORT_DIAGNOSTICS                 // Enable IsiUpdateDiagnostist
#define ISI_SUPPORT_TIMG
#define ISI_SUPPORT_MANUAL_CONNECTIONS
#define ISI_SUPPORT_AUTOMATIC_CONNECTIONS
#define ISI_SUPPORT_DADAS
#define ISI_SUPPORT_CONTROLLED_CONNECTIONS
#endif  //  ISI_COMPACT

// Common defines
#define ISI_PROTOCOL_VERSION        3u
#define ISI_IMPLEMENTATION_VERSION  4u
#define ISI_DEFAULT_DEVICECOUNT     32u
#define ISI_MINIMUM_DEVICECOUNT     4u
#define ISI_MAX_CONNECTION_COUNT    256
#define ISI_WIDTH_PER_CONNTAB       4u			// Certain code paths assume this value (look for "ASSUMES")
#define ISI_SELECTOR_MASK           0x2FFFul
#define ISI_MESSAGE_CODE            0x3Du
#define ISI_PRIMARY_DOMAIN_INDEX    0u			// Certain code paths assume this value (look for "ASSUMES")
#define ISI_SECONDARY_DOMAIN_INDEX  1u
#define ISI_SECONDARY_SUBNET_ID     1u
#define ISI_SECONDARY_NODE_ID       1u
#define ISI_NOT_ACCEPTABLE         255u			// Certain code paths assume this value (look for "ASSUMES")
#define ISI_ADPU_OFFSET             11u
#define ISI_MAX_ADDRESS_TABLE_SIZE	254u        // Was 15u
#define ISI_MAX_ALIAS_COUNT         254u
#define ISI_MAX_NV_COUNT            254u
#define	ISI_ALIAS_UNUSED			0xFFFFu
#define ISI_NO_ADDRESS              0xFFFFu
#define ISI_T_ACQ                   (5ul*60ul*ISI_TICKS_PER_SECOND)
#define ISI_T_CSMR_PAUSE            15          // Minimum hesitation after DIDCF in seconds
#define ISI_T_CSMR                  (60u*ISI_TICKS_PER_SECOND)
#define ISI_T_AUTO                  (30u*ISI_TICKS_PER_SECOND)
#define ISI_T_TIMG                  (60u*ISI_TICKS_PER_SECOND)
#define ISI_T_ENROLL                ISI_T_ACQ
#define ISI_T_CSMO                  (5u * ISI_TICKS_PER_SECOND)
#define ISI_T_CSME					ISI_T_CSMO

#define ISI_T_RM                    (5u*ISI_TICKS_PER_SECOND)
#define ISI_DIDRQ_RETRIES           20u
#define ISI_DIDRQ_PAUSE             (5u*ISI_T_RM)
#define ISI_T_COLL                  ((3ul*ISI_TICKS_PER_SECOND)/2u)
#define ISI_T_CF                    (ISI_T_ACQ/5u)
#define ISI_DIDRM_RETRIES           3
#define ISI_T_QDR                   (1+ISI_TICKS_PER_SECOND)
#define ISI_T_UDR                   (2+ISI_TICKS_PER_SECOND)

#define ISI_NV_UPDATE_RETRIES		3u
#define ISI_SUBNET_BUCKET_SIZE      64u
#define ISI_SUBNET_START_TPFT       64u
#define ISI_SUBNET_START_PL20       128u
#define ISI_SUBNET_START_OTHER		192u
#define	ISI_MESSAGE_HEADROOM		4u	        // Bytes before the start of the IsiMessage in a message buffer

#define IZOT_SERVICE_PIN_MESSAGE    0x7Fu
#define IZOT_WINK_MESSAGE           0x70u
#define IZOT_QUERY_DOMAIN_MESSAGE   0x6A
#define IZOT_QUERY_DOMAIN_SUCCESS   0x2A
#define IZOT_QUERY_DOMAIN_FAILURE   0x0A

#define IZOT_UPDATE_DOMAIN_MESSAGE  0x63
#define IZOT_UPDATE_DOMAIN_SUCCESS  0x23
#define IZOT_UPDATE_DOMAIN_FAILURE  0x03

#define ISI_WINK_REPEATS            3
#define ISI_QUERY_DOMAIN_RETRIES    3
#define ISI_UPDATE_DOMAIN_RETRIES   3
#define ISI_NVHB_REPEATS            1
#define ISI_CTR_RETRIES             3
#define ISI_RDC_RETRIES             3

#define NEURON_ID_LEN               IZOT_UNIQUE_ID_LENGTH

#define MAX_DOMAINS                 2           // Maximum # of domains allowed
#define MAX_CONNECTION_TBL_ENTRIES  255 
#define NUM_ADDR_TBL_ENTRIES    	254         // Number of entries in address table
#define NV_TABLE_SIZE				254         // Number of entries in NV table
#define NUM_ADDR_TBL_SIZE
#define ISI_MESSAGE_TAG             0x0F

#define DOMAIN_ID_LEN               6
#define AUTH_KEY_LEN                6
#define ID_STR_LEN                  8

#ifndef WIN32
#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif
#define max(a, b) MAX(a, b)
#define min(a, b) MIN(a, b)
#endif

// Initialization opitions
typedef	enum {
    isiReboot = 0,
	isiReset,
	isiRestart
} 	IsiBootType;

// Persistent data loss reason
typedef enum {
	LT_CORRUPTION				= 0x00,		// Image checksum invalid
	LT_PROGRAM_ID_CHANGE		= 0x01,		// Program ID changed
	LT_SIGNATURE_MISMATCH		= 0x02,		// Image signature mismtach.  Could be corruption
											// or change to the persistent data format
	LT_PROGRAM_ATTRIBUTE_CHANGE = 0x03,		// Number of NVs, aliases, address or domain
											// entries changed
	LT_PERSISTENT_WRITE_FAILURE = 0x04,		// Could not write the persistence file
	LT_NO_PERSISTENCE			= 0x05,		// No persistence found
	LT_RESET_DURING_UPDATE		= 0x06,		// Reset or power cycle occurred while
											// configuration changes were in progress
	LT_VERSION_NOT_SUPPORTED	= 0x07,		// Version number not supported

	LT_PERSISTENCE_OK			= -1
} LtPersistenceLossReason;

// ISI variables fall into two sections, persistent ones, and not persistent
// ones. Both are held in one structure each. The structures are defined here
// (and implemented in isi_vars.c. The defaults are also set there)
typedef struct {
#ifdef  ISI_SUPPORT_TIMG
    IzotByte    Devices;            // Latest device count
#endif  // ISI_SUPPORT_TIMG
    IzotByte    Nuid;               // Non-unique device ID
    IzotBits16 Serial;              // Running serial number of CIDs
	// The next field is an enumeration that indicates the amount of initialization work
	// required. The field is initialized to "isiReset", because the Neuron Exporter already
	// gives us cleared-out NV, alias, and address tables. For a normal application download,
	// this significantly reduces the time the node is not responsive.  ReturnToFactoryDefaults()
    // resets this field to isiReboot, causing a reinitialization of all tables - the right thing
    // to do when transitioning back from a managed to a self-installed network. When the 
    // initialization function has done its job one way or another, it changes this field to 
    // isiRestart so that a subsequent reset only causes normal reset operations, but no wiping
    // out of system tables, etc.
	IsiBootType BootType;
	IzotByte	RepeatCount;	    // Repeat count used for NV updates (i.e. the address table); 
                                    // must be 1,2, or 3
} IsiPersist;

extern IsiPersist _isiPersist;
extern IsiType gIsiType;
extern IsiFlags gIsiFlags;
extern unsigned gConnectionsTableSz;
extern unsigned gDIDLen;
extern IzotDomainId gDomainID;
extern unsigned gRepeatCount;
extern unsigned char globalExtended;
extern unsigned char gIsiDerivableAddr;  // Flag to indicate that the IP address is derivabled or not

// Structure for volatile (RAM) data.
typedef enum {
    isiChannelTpFt    = 0x04,
    isiChannelPl20A   = 0x0F,
    isiChannelPl20C   = 0x10,
    isiChannelPl20N   = 0x11,
    isiChannelIp852   = 0x9A,
    isiChannelIp852_1 = 0x00
} IsiChannelType;

typedef struct {
    unsigned    RepeatTimer;        // Encoded (and pre-shifted into normal address table location)
    unsigned    TransmitTimer;      // Encoded
    unsigned    GroupRcvTimer;      // Encoded
    unsigned    NonGroupTimer;      // Encoded
    unsigned    BaseSubnet;
    unsigned    TicksPerSlot;       // Width of ISI broadcast slots in ticks; must be >>
                                    // SpreadingInterval
    unsigned    SpreadingInterval;  // Width of the spreading interval; must be >= 2 x JitterInterval;
                                    // JitterInterval is fixed at +- 1
} IsiTransport;

typedef enum {
    isiPeriodicTypeDrum = 0,        // isiPeriodicTypeDRUM *must* be zero
    isiPeriodicTypeCsmr,
    isiPeriodicTypeCsmi,
    isiPeriodicTypeNvHb,
    isiPeriodicTypeApplication,
    isiPeriodicTypeTimg,
    //  Insert new mode before this comment. Make sure there can only by a total of 8 modes, starting with
    //  isiPeriodicTypeDrum (0) and ending with isiPeriodicTypeXYZ (7). For more modes, Ticks.c must be
    //  restructured to meet the requirement that at least every 8th periodic message must be a DRUM message.
    isiPeriodicTypes
} _IsiPeriodicType;

typedef struct {
    unsigned    drumPause;   // use _IsiPeriodicType
	enum {
		slotCsmr, slotCsmi, slotNvHb, slotAppl, slotTimg
	}			slotUsage;
	unsigned	lastConnectionIdx;
} _IsiPeriodic;

// These enumerations are powers of 2 so that they can be checked using
// bit operations.  If there are more than 8 non-zero states required, then 
// additional state bytes must be defined.
typedef enum {
    isiStateNormal			= 0x00,	            // Must be zero
    // Enrollment states
    isiStateInviting		= 0x01u,	        // About to become a host, not heard of anybody back yet
    isiStatePlannedParty	= 0x02u,	        // About to become a host, have at least one guest
    isiStateInvited			= 0x04u,	        // Have been invited but not accepted yet
    isiStateAccepted		= 0x08u,	        // Have been invited and accepted invitation
    // Device and domain acquisition states
    isiStateAwaitDidrx      = 0x10u,            // ISI-DA: wait for DIDRM, ISI-DAS: wait for DIDRQ
    isiStateAwaitConfirm    = 0x20u,            // ISI-DA: wait for DIDCF, ISI-DAS: wait for IsiStartDeviceAcquisition()
    isiStateCollect         = 0x40u,            // ISI-DA: collect DIDRM, ISI-DAS: collect service pin 1 and 2
    isiStateAwaitQdr        = (int)0x80u,       // ISI-DAS only: await query domain response
    isiStatePause           = isiStateAwaitQdr  // ISI-DA: Wait before issueing new DIDRM
} IsiState;

#ifdef  ISI_SUPPORT_DADAS
//  DAS devices support an extended set of state flags - these only exist on DAS devices
//  and are used to keep track of the substates of the domain and device acquisition
typedef enum {
    isiDasNormal                = 0x00,    // no special state modifier values
    isiDasAutoDeviceAcquisition = 0x01,    // automatic device acquisition following a domain fetch process; see msgdas.c for details.
    isiDasFetchDomain           = 0x02,    // regular DAS state bits refer to a normal FetchDomain process (obtain remote domain ID)
    isiDasFetchDevice_Query     = 0x04,    // regular DAS state bits refer to a FetchDevice process (set remote domain ID): obtain remote domain id
    isiDasFetchDevice_Confirm   = 0x08,    // regular DAS state bits refer to a FetchDevice process (set remote domain ID): await positive response to assigning remote ID
    isiDasAwaitDidrx            = 0x10,
    isiDasAwaitConfirm          = 0x20,
    isiDasCollect               = 0x40,
    isiDasAwaitQdr              = (int)0x80u
} IsiDasExtendedStates;
extern IsiDasExtendedStates isiDasExtState;
#endif  //  ISI_SUPPORT_DADAS

#define HOST_STATES			(isiStateInviting|isiStatePlannedParty)
#define GUEST_STATES		(isiStateInvited|isiStateAccepted)
#define CONNECTION_STATES	(HOST_STATES | GUEST_STATES)
#define ACQUISITION_STATES  (isiStateAwaitDidrx | isiStateAwaitConfirm | isiStateCollect | isiStateAwaitQdr)

typedef struct {
    IzotBool         Running;
    IsiFlags        Flags;      // public enumeration IsiFlags plus ISI_FLAG_* macros
    IsiState        State;
    IsiChannelType  ChannelType;    // channel type, used to determine subnet bucket and IsiTransport pointer
    IsiTransport    Transport;    // current transport parameter record
    unsigned long   Wait;       // number of ticks Tick*() is currently waiting before anything happens
    unsigned long   Startup;    // number of ticks since start, stops at 0xFFFF. Used to determine Tcsmr and Tauto events
    unsigned long   Timeout;    // number of ticks, counting down from some timeout. 1==due, 0=off
    unsigned        ShortTimer;
	unsigned		Group;		// the group ID for a pending connection. re-used during domain/device acquisition.
    unsigned        Spreading;  // tick counter used for spreading
    _IsiPeriodic    Periodic;
    unsigned        ConnectionTableSize;
    unsigned        pendingConnection;
    unsigned long   specialDrum;  // if the broadcaster starts too late, we might issue a single DRUM before the broadcaster actually starts
} IsiVolatile;
extern IsiVolatile _isiVolatile;
extern IsiMessage isi_out;							// the global ISI message buffer

#ifdef	ISI_SUPPORT_TIMG
#define PersistDevices _isiPersist.Devices
#else
#define PersistDevices
#endif

// TBD: Move the following typedefs to a more appropriate file
						
// Network management response structures (from netmgmt.h)
typedef struct NM_query_domain_response {
    IzotByte	    id[ DOMAIN_ID_LEN ];
    IzotByte	    subnet;
    IzotByte	    must_be_one    : 1;
    IzotByte	    node	   : 7;
    IzotByte	    len;
    IzotByte	    key[ AUTH_KEY_LEN ];
} NM_query_domain_response;

typedef struct NM_update_domain_request {
    IzotByte	    domain_index;
    IzotByte	    id[ DOMAIN_ID_LEN ];
    IzotByte	    subnet;
    IzotByte	    must_be_one : 1;	// this bit must be set to 1
    IzotByte	    node	: 7;
    IzotByte	    len;
    IzotByte	    key[ AUTH_KEY_LEN ];
} NM_update_domain_request;

typedef struct NM_service_pin_msg{
    IzotByte	    neuron_id[ NEURON_ID_LEN ];
    IzotByte	    id_string[ ID_STR_LEN ];
} NM_service_pin_msg;

/* NM_query_domain */
typedef struct NM_query_domain_request {
    IzotByte        code;
    IzotByte        domain_index;
} NM_query_domain_request;

#ifdef MSC
#pragma pack(1)
#endif

// TBD: Move the following comment
/*
 ****************************************************************************
 * Application buffer structures for sending and receiving messages to and
 * from a network interface.  The 'ExpAppBuffer' and 'ImpAppBuffer'
 * structures define the application buffer structures with and without
 * explicit addressing.  These structures have up to four parts:
 *
 *   Network Interface Command (NI_Hdr)                        (2 Bytes)
 *   Message Header (MsgHdr)                                   (3 Bytes)
 *   Network Address (ExplicitAddr)                            (11 Bytes)
 *   Data (MsgData)                                            (varies)
 *
 * Network Interface Command (NI_Hdr):
 *
 *   The network interface command is always present.  It contains the
 *   network interface command and queue specifier.  This is the only
 *   field required for local network interface commands such as 
 *   LonNiResetDeviceCmd.
 *
 * Message Header (MsgHdr: union of NetVarHdr and ExpMsgHdr):
 *
 *   This field is present if the buffer is a data transfer or a completion
 *   event.  The message header describes the type of IZOT message
 *   contained in the data field.
 *
 *   NetVarHdr is used if the message is a network variable message and
 *   network interface selection is enabled.
 *
 *   ExpMsgHdr is used if the message is an explicit message, or a network
 *   variable message and host selection is enabled (this is the default
 *   for the SLTA).
 *
 * Network Address (ExplicitAddr:  SendAddrDtl, RcvAddrDtl, or RespAddrDtl)
 *
 *   This field is present if the message is a data transfer or completion
 *   event, and explicit addressing is enabled.  The network address
 *   specifies the destination address for downlink application buffers,
 *   or the source address for uplink application buffers.  Explicit
 *   addressing is the default for the SLTA.
 *
 *   SendAddrDtl is used for outgoing messages or NV updates.
 *
 *   RcvAddrDtl is used  for incoming messages or unsolicited NV updates.
 *   RespAddrDtl is used for incoming responses or NV updates solicited
 *   by a poll.
 *
 * Data (MsgData: union of UnprocessedNV, ProcessedNV, and ExplicitMsg)
 *
 *   This field is present if the message is a data transfer or completion
 *   event.
 *
 *   If the message is a completion event, then the first two Bytes of the
 *   data are included.  This provides the NV index, the NV selector, or the
 *   message code as appropriate.
 *
 *   UnprocessedNV is used if the message is a network variable update, and
 *   host selection is enabled. It consists of a two-Byte header followed by
 *   the NV data.
 *   ProcessedNV is used if the message is a network variable update, and
 *   network interface selection is enabled. It consists of a two-Byte header
 *   followed by the NV data.
 *
 *   ExplicitMsg is used if the message is an explicit message.  It consists
 *   of a one-Byte code field followed by the message data.
 *
 * Note - the fields defined here are for a little-endian (Intel-style)
 * host processor, such as the 80x86 processors used in PC compatibles.
 * Bit fields are allocated right-to-left within a Byte.
 * For a big-endian (Motorola-style) host, bit fields are typically
 * allocated left-to-right.  For this type of processor, reverse
 * the bit fields within each Byte.  Compare the NEURON C include files
 * ADDRDEFS.H and MSG_ADDR.H, which are defined for the big-endian NEURON
 * CHIP.
 */

/*
 *  Typedef: IsiCid2
 *  Structure representing the rev 2 format of the unique connection ID.  
 *  See the IsiCid typedef in IsiTypes.h for the rev1 format of the unique 
 *  connection ID. 
 *  In this rev2, the uniqueID is constructed based on the 6-byte neuronID 
 *  (it's no longer in the compressed form) and leaving the Serial Number with 1-byte.
 *  It's currently used internally during the creation of the new unique CID.
 *
 */
typedef IZOT_STRUCT_BEGIN(IsiCid2) 
{
    IzotUniqueId UniqueId;    // host's unique ID (copy from Neuron ID)
    IzotByte     SerialNumber;
} IZOT_STRUCT_END(IsiCid2);

/*
 *  Typedef: IsiUniqueCid
 *  Structure representing the union of the rev 1 and rev 2 format of the unique connection ID.
 *  It's currently used internally as a convenient access to the connection ID.   
 */
typedef IZOT_UNION_BEGIN(IsiUniqueCid) 
{        
    IsiCid rev1Cid;
    IsiCid2 rev2Cid;
} IZOT_UNION_END(IsiUniqueCid);

extern IzotReadOnlyData read_only_data;
extern IzotConfigData config_data;
extern IzotDatapointConfig nv_config;
extern IzotAliasConfig alias_config;

extern IzotDomain domainTable[MAX_DOMAINS];  // retrieve using IzotQueryDomainConfig(const unsigned index, IzotDomain* const pDomain);
extern IzotAddress addrTable;    // [NUM_ADDR_TBL_ENTRIES];    // retrieve using IzotQueryAddressConfig(const unsigned index, IzotAddress* const pAddress);

//
// Used to check for valid data pointer.
// Note it is sufficient to just check the high byte of the pointer since the first
// page of Neuron memory is always constant system image.
//
#define VALID_DATA_PTR(p)	(p != NULL)      

extern unsigned LonIsiNvCount(void);
extern unsigned int LonIsiAliasCount(void);
extern unsigned int LonIsiAddressTableCount(void);
extern LonStatusCode LonIsiUpdateDomainConfig(const IzotDomain* domainConfig,
                int domainIndex, int nonCloneValue, IzotBool bUpdateID);
extern IsiType LonIsiType();
extern void LonIsiSetType(IsiType type);

#ifdef  __cplusplus
}
#endif

#endif	//	!defined __ISIINT_H__
