/*
 * lon_usb_link.h
 *
 * Copyright (c) 2017-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON USB Network Driver Link
 * Purpose: Provides a LON link-layer interface with a U10, U20,
 * 			U60, or U70 LON USB network interface device.
 */

#if !defined(_LON_USB_LINK_H)
#define _LON_USB_LINK_H

#include "izot/IzotPlatform.h"
#include "izot/lon_types.h"
#include "abstraction/IzotCal.h"
#include "lcs/lcs.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_link.h"
#include "lcs/lcs_queue.h"

// Maximum time to wait for a LON interface unique ID (UID) response (milliseconds)
#ifndef UID_WAIT_TIME
#define UID_WAIT_TIME 500
#endif

// Maximum time to wait for an uplink message (milliseconds)
#ifndef MSG_WAIT_TIME
#define MSG_WAIT_TIME 500
#endif

// Maximum time to wait for an uplink acknowledgment (milliseconds)
#ifndef ACK_WAIT_TIME
#define ACK_WAIT_TIME 500
#endif

// Maximum time to wait for an uplink acknowledgment during startup (milliseconds)
#ifndef STARTUP_ACK_WAIT_TIME
#define STARTUP_ACK_WAIT_TIME 1000
#endif

// Maximum time to wait for the network interface to accept a downlink message (milliseconds)
#ifndef DOWNLINK_WAIT_TIME
#define DOWNLINK_WAIT_TIME 10000
#endif

// Maximum time to wait for a unique ID request after opening the LON USB
// interface (milliseconds)
#ifndef STARTUP_DELAY
#define STARTUP_DELAY 60000
#endif

// Maximum number of attempts to read the LON interface UID
#ifndef MAX_UID_RETRIES
#define MAX_UID_RETRIES 3
#endif

// Maximum bytes to read from the USB network interface per event loop cycle
#ifndef MAX_BYTES_PER_USB_READ
#define MAX_BYTES_PER_USB_READ 512
#endif

// Maximum bytes to parse from the USB receive ring buffer per event loop cycle
#ifndef MAX_BYTES_PER_USB_PARSE_CHUNK
#define MAX_BYTES_PER_USB_PARSE_CHUNK 128
#endif

// Maximum bytes to parse from the USB receive ring buffer per parse window
#ifndef MAX_BYTES_PER_USB_PARSE_WINDOW
#define MAX_BYTES_PER_USB_PARSE_WINDOW 512
#endif

// Maximum number of LON USB interfaces supported
#ifndef MAX_IFACE_STATES
#define MAX_IFACE_STATES 4
#endif

// Maximum LON MAC layer message size (bytes) for non-expanded non-extended
// and extended messages,and expanded extended messages with framesync
// byte-stuffing; the LON MAC layer can carry ISO/IEC 14908-1 payloads up
// to 228 bytes, or UDP payloads up to 1280 bytes
#define MAX_LON_MSG_NON_EX_LEN		240
#define MAX_LON_MSG_EX_LEN			1280
#define MAX_EXP_LON_MSG_EX_LEN		(2*MAX_LON_MSG_EX_LEN+4)

// Maximum entries in the LON downlink and uplink buffer queues
#define MAX_LON_DOWNLINK_BUFFERS	16
#define MAX_LON_UPLINK_BUFFERS		16

// LON USB interface mode enumeration
typedef enum {
	LON_USB_OPEN_LAYER5,				// LON protocol layer 5 (for host apps)
	LON_USB_OPEN_LAYER2,				// LON protocol layer 2 (for LON stacks)
	LON_USB_OPEN_UNKNOWN = -1
} LonUsbOpenMode;

// LON uplink statistics structure
typedef struct LonUplinkStats {
	size_t packets_received;			// Total packets received
	size_t bytes_received;      		// Total bytes received
	size_t rx_checksum_errors;			// Receive checksum errors
	size_t rx_code_packet_failures;		// Uplink CpFail received (implies downlink checksum error)
	size_t rx_crc_errors;				// Receive CRC errors
	size_t rx_frame_errors;				// Receive frame errors
	size_t rx_timeout_errors;			// Receive timeout errors
	size_t rx_ack_timeout_errors;		// Receive ACK timeout errors
	size_t rx_duplicates;				// Duplicate packets received
} LonUplinkStats;

// LON downlink statistics structure
typedef struct LonDownlinkStats {
	size_t packets_sent;				// Total packets sent
	size_t bytes_sent;					// Total bytes sent
	size_t tx_aborted_errors;			// Incomplete downlink transmit errors
	size_t tx_rejects;			// Downlink message rejects by network interface
} LonDownlinkStats;

// USB RX staging statistics structure; tracks ring-buffer receive staging activity
typedef struct LonUsbRxStats {
	size_t bytes_fed;					// Total bytes accepted into ring
	size_t bytes_read;					// Total bytes drained & parsed
	size_t bytes_dropped;				// Bytes rejected due to insufficient ring capacity
	size_t max_occupancy;				// High-water mark of ring usage in bytes
	size_t capacity;					// Configured ring capacity
} LonUsbRxStats;

// LON USB interface statistics structure
typedef struct LonStats {
	size_t size;         			 	// Set to sizeof(LonStats);
	LonUplinkStats uplink;				// LON uplink statistics
	LonDownlinkStats downlink;			// LON downlink statistics
	LonUsbRxStats usb_rx;				// LON USB receive staging statistics
	int reference_count;				// open() reference count
	int reset_count;					// NI reset count
	int tx_id;							// Reported on NI reset
	int l2_l5_mode;						// Reported on NI reset
} LonStats;

// Increment 'x' and peg at 0xFFFFFFFF on overflow
#define Increment32(x)	{if(x != 0xFFFFFFFF) x++;}

// Add 'inc' to 'x' and peg at 0xFFFFFFFF on overflow
#define Add32(x, inc)                                    \
	do {                                                         \
		uint32_t _orig = (uint32_t)(x);                          \
		uint32_t _add  = (uint32_t)(inc);                        \
		uint32_t _res  = _orig + _add;                           \
		(x) = (_res < _orig) ? 0xFFFFFFFFu : _res;               \
	} while (0)

// Portable packed-structure annotation targeting GCC/Clang style compilers.
#ifndef LON_PACKED
#define LON_PACKED __attribute__((packed))
#endif

/*
 * Compile-time layout verification for packed wire-format structures.
 * These asserts will fail if packing is lost or field ordering changes.
 */
#if defined(__cplusplus)
#define LON_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define LON_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

typedef struct LonUsbConfig {
	uint32_t in_transfer_size;			// Input transfer size in bytes
	uint32_t read_timeout;				// Read timeout in milliseconds
	uint32_t write_timeout;			// Write timeout in milliseconds
	uint32_t uplink_container_limit;	// Uplink container limit in packets
	uint32_t llp_timeout;				// LLP timeout in milliseconds
} LonUsbConfig;

// Downlink state machine states
typedef enum {
	DOWNLINK_START = 0,				// Starting state, downlink initialization pending
	DOWNLINK_IDLE,					// Idle state, nothing in progress
	DOWNLINK_CP_ACK_WAIT,			// Waiting for a code packet ACK
	DOWNLINK_MSG_ACK_WAIT,			// Waiting for a message ACK
	DOWNLINK_CP_MSG_REQ_ACK_WAIT,	// Waiting for a message request ACK
	DOWNLINK_CP_RESPONSE_WAIT		// Waiting for uplink local command response
} DownlinkState;

// Uplink state machine states
typedef enum {
	UPLINK_IDLE = 0,				// Waiting for frame sync byte indicating start of frame
	UPLINK_FRAME_CODE,				// Waiting for frame code byte; always 0 for MIP/U61
	UPLINK_FRAME_PARAMETER,			// Waiting for frame parameter byte; N/A for MIP/U61
	UPLINK_CODE_PACKET_CHECKSUM,	// Waiting for code packet checksum byte; N/A for MIP/U61
	UPLINK_MESSAGE,					// Message streaming
	UPLINK_ESCAPED_DATA				// Message streaming, escaped data
} UplinkState;

// Message priority levels
typedef enum {
	NORMAL_PRIORITY = 0,
	HIGH_PRIORITY,
	ALL_PRIORITIES
} MessagePriorityLevel;

// LON extended message alternate structure
#define EXT_LENGTH	0xFF			// Extended message length indicator
typedef struct LON_PACKED LdvExtendedMessage {
	uint8_t cmd;					// Network interface command (use LonNiCommand value)
	uint8_t ext_flag;				// Will be set to EXT_LENGTH
	uint16_t ext_length;
	uint8_t ext_pdu[MAX_LON_MSG_EX_LEN]; // Size is based on ext_length
} LdvExtendedMessage;

// Uplink and downlink buffer sizes (including expansions)
#define UPLINK_BUF_LEN  ((MAX_LON_MSG_EX_LEN+4)*2)    // Size of ul serial buffer (including expansions)
#define DOWNLINK_BUF_LEN  ((MAX_LON_MSG_EX_LEN+4)*2)  // Size of dl serial buffer (including expansions)

typedef struct LON_PACKED UsbNiExtendedMessage {
	uint8_t cmd;					// Network interface command (use LonNiCommand value)
	uint8_t ext_flag;				// Will be set to EXT_LENGTH
	uint8_t ext_length;				// PDU size in bytes
	uint8_t ext_pdu[MAX_LON_MSG_EX_LEN];
} UsbNiExtendedMessage;

// LON USB frame header type enumeration
typedef enum {
	FRAME_SYNC_ONLY,
	FRAME_CODE_PACKET
} LonUsbFrameHeaderType;

// LON USB frame commands -- must match the MIP/U50 implementation
typedef enum {
	NULL_FRAME_CMD			= 0,
	FAIL_FRAME_CMD			= 1,
	MSG_FRAME_CMD			= 2,
	MSG_REQ_FRAME_CMD		= 3,
	MSG_ACK_FRAME_CMD		= 4,

	MSG_QUEUE_CMD_COUNT,	// Messages prior to this one can be queued.

	MSG_REJECT_FRAME_CMD  	= 5,
	SHORT_NI_CMD_FRAME_CMD	= 6,
	NI_RESYNC_FRAME_CMD 	= 7,
	NI_CMD_PASSW_FRAME_CMD	= 8,
	NI_CALLBACK_FRAME_CMD	= 9,
	INVALID_FRAME_CMD		= 14,
	ESCAPE_FRAME_CMD		= 15,

	FRAME_CMD_COUNT			= 16
} LonUsbFrameCommand;

// LON frame code packet structure; member of LON frame header
// TBD: little-endian vs big-endian bit-field ordering is compiler-dependent
//      and must match the MIP/U50 implementation
typedef struct LON_PACKED LonFrameCode {
	unsigned int sequence_num : 3;	// Sequence number for duplicate detection; set to 0 for MIP/U61
	bool ack : 1;					// Acknowledgment flag; set to 0 for MIP/U61
	unsigned int frame_cmd : 4;		// Frame command; set to 0 for MIP/U61
} LonFrameCode;

// LON frame header structure; for MIP/U50 this is a code packet;
// for MIP/U61 this is always FRAME_SYNC followed by a 0 byte only
typedef struct LON_PACKED LonFrameHeader {
	uint8_t frame_sync;				// Always FRAME_SYNC (0x7E)
	LonFrameCode frame_code;		// Frame code with sequence number, ack, and frame command
	uint8_t parameter;				// Parameter for the frame command; zero if none; not used for MIP/U61
	uint8_t checksum;				// Negative modulo 256 sum of the frame contents; not used for MIP/U61
} LonFrameHeader;

// The full message element as stored in this driver for non-extended messages
typedef struct LON_PACKED LonUsbQueueBuffer {
	size_t buf_size;				// Buffer size in bytes, with variable size PDU
	MessagePriorityLevel priority;	// Message priority level
	LonFrameHeader frame_header;	// Two (frame sync and zero only) or four-byte (code packet) frame header
	L2Frame usb_ni_message;	// Underlying USB NI message
} LonUsbQueueBuffer;

// LON USB interface type enumeration
typedef enum {
	LON_USB_INTERFACE_U50,
	LON_USB_INTERFACE_U61
} LonUsbIfaceType;

// LON USB interface model enumeration
typedef enum {
	U10_FT_AB,
	U10_FT_C,
	U20_PL,
	U60_FT,
	U60_TP_1250,
	U70_PL,
	RF_900
} LonUsbIfaceModel;

#define MAX_IFACE_MODELS 6

// LON USB link-layer interface configuration structure
typedef struct LonUsbIfaceConfig {
	LonUsbIfaceType iface_type;
	short usb_line_discipline;
	LonUsbFrameHeaderType frame_header_type;
	bool supports_link_layer_sequencing;
	bool supports_short_local_cmds;
	bool supports_code_packet_acks;
} LonUsbIfaceConfig;

// LON USB interface configurations indexed by LonUsbIfaceModel
static LonUsbIfaceConfig lon_usb_iface_configs[] = {
	// U10 FT Rev A/B
	{LON_USB_INTERFACE_U61, 27, FRAME_SYNC_ONLY, false, false, false},
	// U10 FT Rev C
	{LON_USB_INTERFACE_U50, 28, FRAME_CODE_PACKET, true, true, true},
	// U20 PL
	{LON_USB_INTERFACE_U61, 27, FRAME_SYNC_ONLY, false, false, false},
	// U60 FT
	{LON_USB_INTERFACE_U50, 28, FRAME_CODE_PACKET, true, true, true},
	// U60 TP-1250
	{LON_USB_INTERFACE_U61, 27, FRAME_SYNC_ONLY, false, false, false},
	// U70 PL
	{LON_USB_INTERFACE_U61, 27, FRAME_SYNC_ONLY, false, false, false}
};

// LON USB link state structure
typedef struct LonUsbLinkState {
	OsalLockType state_lock;			// Mutex or spinlock for this structure
	OsalLockType queue_lock;			// Mutex or spinlock for queue operations
	bool assigned;						// True if this entry is in use
										// Set on start to ALL_PRIORITIES to clear all queues
										// Set on restart to NORMAL_PRIORITY to clear normal queue only
	bool wait_for_uid;					// True if waiting for UID response
	int uid_retries;					// Remaining LON NI UID read retries
	bool have_uid;						// True if LON NI UID acquired
    volatile bool shutdown;				// True to terminate any threads
	int iface_index;
	char lon_dev_name[FILENAME_MAX];
	char usb_dev_name[DEVICE_NAME_MAX];
	int usb_fd;							// USB device file descriptor

	// LON USB interface model
	LonUsbIfaceModel lon_usb_iface_model;

	// LON settings and statistics
	uint8_t uid[IZOT_UNIQUE_ID_LENGTH];	// LON NI unique ID (MAC ID or Neuron ID)
	LonUsbOpenMode iface_mode;			// LON NI mode (Layer 2 or Layer 5)
	LonStats lon_stats;					// Last copy of LON stats

	// USB parameters
	LonUsbConfig usb_params;

	// Timestamps
	OsalTickCount start_time;			// Time of interface startup
	OsalTickCount last_timeout;			// Time of last uplink ack timeout

	// Uplink packet state machine
	UplinkState uplink_state;
	OsalTickCount uplink_msg_timer;		// Uplink wait for message timeout timer
	// OsalTickCount uplink_ack_timer;	// Uplink wait for ack timeout timer
	bool uplink_frame_error;			// True if last uplink had a frame error
	bool uplink_duplicate;				// True if last uplink frame was a duplicate
	LonUsbQueueBuffer uplink_buffer;		// Current uplink buffer being built
	uint8_t uplink_msg[UPLINK_BUF_LEN];	// Uplink packets built here
	int uplink_msg_index;				// Index into uplink_msg[]
	int uplink_msg_length;				// Length or extended length
	int uplink_seq_number;				// Uplink sequence number
	bool uplink_tail_escaped;			// True if last byte of a message was a frame sync byte
	OsalTickCount uplink_ack_timer;		// Start time in ms ticks if waiting for an uplink ack
										// to a downlink packet; 0 if not waiting
	OsalTickCount uplink_ack_timeout;	// Uplink ack timeout duration
	int uplink_ack_timeouts;			// Uplink ack timeout count
	int uplink_ack_timeout_phase;		// Uplink ack timeout phase, incremented on repeated timeouts
	LonNiCommand uplink_expected_rsp;	// Expected uplink response command

	// Downlink packet state machine
	DownlinkState downlink_state;		// Current downlink state
	OsalTickCount downlink_reject_timer;// Downlink reject timeout timer;
										// Cleared when downlink message is acknowledged
	LonUsbQueueBuffer downlink_buffer;		// Current downlink buffer being built
	bool downlink_cp_requested[MSG_QUEUE_CMD_COUNT];  // True if downlink code packet requested indexed by LonUsbFrameCommand
	int downlink_seq_number;			// Downlink sequence number
	bool downlink_ack_required;			// True if an ack is required to be sent downlink

	// Buffer queues for parsed messages; each queue holds LonUsbQueueBuffer entries
	// Implemented using the generic Queue type from lcs_queue.h
	Queue lon_usb_downlink_normal_queue;		// Downlink normal priority buffer queue
	Queue lon_usb_downlink_priority_queue;		// Downlink high priority buffer queue
	Queue lon_usb_uplink_normal_queue;			// Uplink normal priority buffer queue
	Queue lon_usb_uplink_priority_queue;		// Uplink high priority buffer queue

	// Uplink ring buffer for staging raw bytes received from the LON USB interface
	// before parsing into messages
	RingBuffer lon_usb_uplink_ring_buffer;
} LonUsbLinkState;

// Verify packed layout for 4-byte LonUsbFrameHeaderType
LON_STATIC_ASSERT(sizeof(LonUsbFrameHeaderType) == 4 * sizeof(uint8_t), "LonFrameHeader size mismatch");

// IPV4 ICMP "poll" (ping) definitions - 
//    ============================================================================
//    | 8                | 8                | 16                                 |
//    ============================================================================
//  0 | Version/IHL      | Type of service  | Length                             |
//  4 | Identification                      | Flags & offset                     |
//  8 | TTL              | Protocol         | Header Checksum                    |
// 12 | Source IP address                                                        |
// 16 | Destination IP address                                                   |
//    ==ICMP Header===============================================================
// 20 | Type of message  | Code             | Checksum                           |
//    ============================================================================
#define IPV4_START			2					// inc. BL & LTV2 bytes
#define IPV4_TOS			(IPV4_START+1)		// 0
#define IPV4_PROTO			(IPV4_START+9)		// 1:ICMP
#define IPV4_DEST_ADDR		(IPV4_START+16)
#define IPV4_ICMP_TYPE		(IPV4_START+20)		// 8:ping
#define IPV4_ICMP_CODE		(IPV4_START+21)		// 0

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
/*
 * Opens a LON USB network interface.
 * Parameters:
 *   lon_dev_name: Logical name for the LON interface, for example "lon0"
 *   usb_dev_name: Host USB interface name, for example "/dev/ttyUSB0"
 *   iface_index: Interface index to be used in subsequent read, write, 
 *   		   and close calls for the interface
 *	 iface_mode: LON interface iface_mode, set to LON_USB_OPEN_LAYER5
 *			   for layer 5 (host apps) or LON_USB_OPEN_LAYER2 for
 *			   layer 2 (LON stacks)
 *   line_discipline: USB line discipline number, set to LDISCS_50 for
 *			   the U10 FT Rev C and U60 FT, and LDISCS_61 for the U10 FT
 *			   Rev A and B, U20 PL, U60 TP-1250, and U70 PL
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode OpenLonUsbLink(char *lon_dev_name, char *usb_dev_name,
					int *iface_index, LonUsbOpenMode iface_mode,
					LonUsbIfaceModel lon_usb_iface_model);

/*
 * Writes a downlink message to the LON USB interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   in_msg: pointer to xLdvMessage or LdvExtendedMessage structure
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode WriteLonUsbMsg(int iface_index, const L2Frame* in_msg);

/*
 * Reads an uplink message from the LON USB interface, if available.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   out_msg: pointer to xLdvMessage or LdvExtendedMessage structure
 * Returns:
 *   LonStatusNoError if a message is successfully read;
 *   LonStatusNoMessageAvailable if no full message is available;
 *   LonStatusCode error code if unsuccessful
 * Notes:
 *   Non-blocking; reads available bytes and returns a single message if
 *   available. If no full message is available, tests for timeout waiting
 *   for the LON interface unique ID (UID) and retries the UID read request.
 */
LonStatusCode ReadLonUsbMsg(int iface_index, L2Frame *out_msg);

/*
 * Feeds received bytes into the RX ring buffer for a LON USB interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   data: pointer to buffer of received bytes
 *   len: number of bytes in data buffer
 * Returns:
 *   Number of bytes successfully fed into the RX ring buffer
 * Notes:
 *   Called by an optional asynchronous OS-specific LON USB interface
 *   receive handler when data is received from the LON USB network 
 *   interface.  For example, this function can be called by a 
 *   platform-specific interrupt handler for the USB interface.  
 *   The data is copied into the RX ring buffer for later processing
 *   by ReadLonUsbMsg().
 */
size_t LonUsbFeedRx(int iface_index, const uint8_t *data, size_t len);

/*
 * Closes a LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode CloseLonUsbLink(int iface_index);

#endif	// !defined(_LON_USB_LINK_H)