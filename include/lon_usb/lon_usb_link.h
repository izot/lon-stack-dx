/*
 * lon_usb_link.h
 *
 * Copyright (c) 2017-2026 EnOcean
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

// Maximum time to wait for an uplink reset on startup and a layer mode change (milliseconds)
#ifndef NI_RESET_WAIT_TIME
#define NI_RESET_WAIT_TIME 800
#endif

// Maximum time to wait for a NULL frame response on startup (milliseconds)
#ifndef NI_NULL_WAIT_TIME
#define NI_NULL_WAIT_TIME 700
#endif

// Resynchronization delay to synchronize uplink and downlink state machines after a reset (milliseconds)
#ifndef NI_RESYNC_DELAY
#define NI_RESYNC_DELAY 300
#endif

// Maximum time to wait for a LON interface unique ID (UID) response (milliseconds)
#ifndef NI_UID_WAIT_TIME
#define NI_UID_WAIT_TIME 350
#endif

// Maximum time to wait for a LON interface status response (milliseconds)
#ifndef NI_STATUS_WAIT_TIME
#define NI_STATUS_WAIT_TIME 350
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
#define MAX_UID_RETRIES 4
#endif

// Maximum number of attempts to read the LON interface status
#ifndef MAX_STATUS_RETRIES
#define MAX_STATUS_RETRIES 4
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

// Maximum entries in the LON downlink and uplink buffer queues
#define MAX_LON_DOWNLINK_BUFFERS	16
#define MAX_LON_UPLINK_BUFFERS		16

// Unknown LON interface status value
#define LON_NI_STATUS_UNKNOWN -1

// LON USB interface mode enumeration
typedef enum {
	LON_IFACE_MODE_LAYER5 = 0,			// LON protocol layer 5 (for host apps)
	LON_IFACE_MODE_LAYER2,				// LON protocol layer 2 (for LON stacks)
	LON_IFACE_MODE_UNKNOWN = -1
} LonUsbInterfaceMode;

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
	size_t tx_rejects;					// Downlink message rejects by network interface
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
	int reset_count;					// LON interface reset count
	int channel_type_id;				// Channel type reported on NI reset
										// See stdxcvr.xml for type IDs
										// Channel type 30 is the Custom channel type
	LonUsbInterfaceMode l2_l5_mode;		// Reported on NI reset and by MIP status response
	uint8_t ni_model_id;				// LON interface model ID reported by MIP status response
	uint8_t ni_fw_version;				// LON interface firmware version
										// reported by MIP status response encoded
										// as (Value / 10); example: 0xA0 = 160,
										// interpreted as version 16.0
	int ni_status;						// LON interface status reported by MIP status response
										// 0 is ready; -1 is unknown; other values are LON interface specific
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
	uint32_t write_timeout;				// Write timeout in milliseconds
	uint32_t uplink_container_limit;	// Uplink container limit in packets
	uint32_t llp_timeout;				// LLP timeout in milliseconds
} LonUsbConfig;

// Downlink state machine states
typedef enum {
	/* --- RESTART --- */
	DOWNLINK_START = 0,					// Restart initialization after receiving a reset
										// command or detecting a reset condition on the link
	/* --- INITIAL BOOT & RESYNC --- */
	DOWNLINK_WAIT_STARTUP_RESET,		// Wait for uplink reset message during startup
	DOWNLINK_WAIT_STARTUP_RESYNC_ACK,	// Wait for uplink null frame indicating LON interface
										// restart after startup resync
	DOWNLINK_WAIT_RESYNC_RESET,			// Wait for uplink reset message after startup resync
	/* --- LAYER 5 TRANSITION & VERIFICATION --- */
	DOWNLINK_LAYER_5_REQUEST,			// Send request to set LON interface mode to layer 5
	DOWNLINK_WAIT_LAYER_5_ACK,			// Wait for uplink null frame indicating LON
										// interface restart after network interface
	DOWNLINK_WAIT_L5_RESYNC_ACK,		// Wait for uplink null frame indicating layer 5
										// resync complete
	DOWNLINK_WAIT_L5_RESET,				// Wait for uplink reset message after change to
										// layer 5 mode
#if 0
// TBD: delete
	DOWNLINK_RESYNC_DELAY,				// Waiting for resynchronization delay to
										// synchronize uplink and downlink state machines
#endif
	/* --- LON INTERFACE UNIQUE ID READ --- */
	DOWNLINK_NI_UNIQUE_ID_REQUEST,		// Send read memory request for the LON interface unique ID
	DOWNLINK_WAIT_NI_UNIQUE_ID,			// Wait for uplink memory read response with unique ID
	/* --- LAYER 2 TRANSITION & VERIFICATION --- */
	DOWNLINK_LAYER_2_REQUEST,			// Send request to set LON interface mode to layer 2
	DOWNLINK_WAIT_LAYER_2_ACK,			// Wait for uplink null frame indicating network
										// interface restart after change to layer 2 mode
	DOWNLINK_WAIT_L2_RESYNC_ACK,		// Wait for uplink null frame indicating layer 2
										// resync complete
	DOWNLINK_WAIT_L2_RESET,				// Wait for uplink reset message after change to
										// layer 2 mode
	/* --- LON INTERFACE MIP STATUS READ --- */
	DOWNLINK_MIP_STATUS_REQUEST,		// Send MIP status request
	DOWNLINK_WAIT_MIP_STATUS,			// Wait for uplink MIP status response after sending MIP status request
	/* --- NORMAL OPERATIONS --- */
	DOWNLINK_IDLE_START,				// Initial idle state	
	DOWNLINK_IDLE,						// Idle state, nothing in progress
	/* --- OPERATIONAL WAITS --- */
	DOWNLINK_WAIT_CP_ACK,				// Wait for a code packet ACK
	DOWNLINK_WAIT_MSG_ACK,				// Wait for a message ACK
	DOWNLINK_WAIT_CP_MSG_REQ_ACK,		// Wait for a message request ACK
	DOWNLINK_WAIT_CP_RESPONSE,			// Wait for uplink local command response
	DOWNLINK_INVALID					// Invalid state
} DownlinkState;

// Downlink state machine state names
static const char* downlink_state_names[] = {
	"DOWNLINK_START",
	"DOWNLINK_WAIT_STARTUP_RESET",
	"DOWNLINK_WAIT_STARTUP_RESYNC_ACK",
	"DOWNLINK_WAIT_RESYNC_RESET",
	"DOWNLINK_LAYER_5_REQUEST",
	"DOWNLINK_WAIT_LAYER_5_ACK",
	"DOWNLINK_WAIT_L5_RESYNC_ACK",
	"DOWNLINK_WAIT_L5_RESET",
	"DOWNLINK_NI_UNIQUE_ID_REQUEST",
	"DOWNLINK_WAIT_NI_UNIQUE_ID",
	"DOWNLINK_LAYER_2_REQUEST",
	"DOWNLINK_WAIT_LAYER_2_ACK",
	"DOWNLINK_WAIT_L2_RESYNC_ACK",
	"DOWNLINK_WAIT_L2_RESET",
	"DOWNLINK_MIP_STATUS_REQUEST",
	"DOWNLINK_WAIT_MIP_STATUS",
	"DOWNLINK_IDLE_START",
	"DOWNLINK_IDLE",
	"DOWNLINK_WAIT_CP_ACK",
	"DOWNLINK_WAIT_MSG_ACK",
	"DOWNLINK_WAIT_CP_MSG_REQ_ACK",
	"DOWNLINK_WAIT_CP_RESPONSE",
	"DOWNLINK_INVALID"
};

// Uplink state machine states
typedef enum {
	UPLINK_IDLE = 0,				// Waiting for frame sync byte indicating start of frame
	UPLINK_FRAME_CODE,				// Waiting for frame code byte; always 0 for MIP/U61
	UPLINK_FRAME_PARAMETER,			// Waiting for frame parameter byte; N/A for MIP/U61
	UPLINK_CODE_PACKET_CHECKSUM,	// Waiting for code packet checksum byte; N/A for MIP/U61
	UPLINK_MESSAGE,					// Message streaming
	UPLINK_ESCAPED_DATA,			// Message streaming, escaped data
	UPLINK_INVALID					// Invalid state
} UplinkState;

// Uplink state machine state names
static const char* uplink_state_names[] = {
	"UPLINK_IDLE",
	"UPLINK_FRAME_CODE",
	"UPLINK_FRAME_PARAMETER",
	"UPLINK_CODE_PACKET_CHECKSUM",
	"UPLINK_MESSAGE",
	"UPLINK_ESCAPED_DATA",
	"UPLINK_INVALID"
};

// Message priority levels
typedef enum {
	NORMAL_PRIORITY = 0,
	HIGH_PRIORITY,
	ALL_PRIORITIES
} MessagePriorityLevel;

// LON extended message alternate structure
#define EXT_LENGTH	0xFF					 // Extended message length indicator
typedef struct LdvExtendedMessage {
	uint8_t cmd;							 // Network interface command (use LonNiCommand value)
	uint8_t ext_flag;				 		 // Will be set to EXT_LENGTH
	uint16_t ext_length;
	uint8_t ext_pdu[MAX_LON_MSG_EX_LEN]; 	 // Size is based on ext_length
} LdvExtendedMessage;

// Uplink and downlink buffer sizes (including expansions)
#define UPLINK_BUF_LEN  ((MAX_LON_MSG_EX_LEN+4)*2)    // Size of ul serial buffer (including expansions)
#define DOWNLINK_BUF_LEN  ((MAX_LON_MSG_EX_LEN+4)*2)  // Size of dl serial buffer (including expansions)

// LON NI frame structure used for non-extended and extended frames as
// transmitted on the USB link; use the LonNiExtendedFrame structure to
// access the extended length field
typedef struct LonNiFrame {
	uint8_t short_pdu_length;				 // PDU length if < EXT_LENGTH; else EXT_LENGTH
	uint8_t ni_command;				 		 // Network interface command--use LonNiCommand values
	uint8_t pdu[MAX_LON_MSG_EX_LEN];
} LonNiFrame;

// LON NI frame structure used for extended frames as transmitted on the
// USB link; the first two bytes match the LonNiFrame structure; the
// ext_length_be field is in network byte order (big-endian) format
typedef struct LonNiExtendedFrame {
	uint8_t ext_flag;				 		 // Set to EXT_LENGTH
	uint8_t ni_command;				 		 // Network interface command--use LonNiCommand values
	uint16_t ext_length_be;					 // Big-endian PDU length in bytes
	uint8_t ext_pdu[MAX_LON_MSG_EX_LEN - 2];
} LonNiExtendedFrame;

// LON USB frame header type enumeration
typedef enum {
	FRAME_SYNC_ONLY,
	FRAME_CODE_PACKET,
	FRAME_INVALID_TYPE = 0xFFFFFFFF		// Unused constant forcing a 4-byte wide enum
										// to prevent a compilation error on an STM32	
} LonUsbFrameHeaderType;

// LON USB frame codes -- must match the MIP/U50 implementation
// The frame code values are in the lower 4 bits of the frame command
// byte for MIP/U50; for MIP/U61, the frame command byte is always 0
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

/*
 * LON frame header structure; for MIP/U50 this is a code packet;
 * for MIP/U61 this is always FRAME_SYNC followed by a 0 byte only
 */

// Macros to access the seq field in LonFrameHeader.frame_code field;
// sequence number for duplicate detection; set to 0 for MIP/U61
#define IZOT_U50_FRAME_CODE_SEQ_MASK  0xE0
#define IZOT_U50_FRAME_CODE_SEQ_SHIFT 5
#define IZOT_U50_FRAME_CODE_SEQ_FIELD frame_code

// Macros to access the ack field in LonFrameHeader.frame_code field;
// acknowledgment flag; set to 0 for MIP/U61
#define IZOT_U50_FRAME_CODE_ACK_MASK  0x10
#define IZOT_U50_FRAME_CODE_ACK_SHIFT 4
#define IZOT_U50_FRAME_CODE_ACK_FIELD frame_code

// Macros to access the cmd field in LonFrameHeader.frame_code field;
// frame command; set to 0 for MIP/U61
#define IZOT_U50_FRAME_CODE_CMD_MASK  0x0F
#define IZOT_U50_FRAME_CODE_CMD_SHIFT 0
#define IZOT_U50_FRAME_CODE_CMD_FIELD frame_code

typedef IZOT_STRUCT_BEGIN(LonFrameHeader) {
    uint8_t frame_sync;	   // Always FRAME_SYNC (0x7E)
    uint8_t frame_code;    // Frame code with sequence number, ack, and
                           // frame command, use IZOT_U50_FRAME_CODE_*
                           // macros for MIP/U50; always 0 for MIP/U61
    uint8_t parameter;	   // Parameter for the frame command; zero if
                           // none; not used for MIP/U61
    uint8_t checksum;	   // Negative modulo 256 sum of the frame contents;
                           // not used for MIP/U61
} IZOT_STRUCT_END(LonFrameHeader);
LON_STATIC_ASSERT(sizeof(LonFrameHeader) == 4, "LonFrameHeader size incorrect");

// The full message element as stored in this driver for non-extended messages
typedef struct LonUsbQueueBuffer {
	size_t buf_size;				// Buffer size in bytes, with variable size PDU
	size_t data_frame_size;			// Variable size PDU in bytes
	MessagePriorityLevel priority;	// Message priority level
	LonFrameHeader frame_header;	// Two (frame sync and zero) or four-byte (code packet) frame header
	LonNiFrame usb_ni_data_frame;   // Underlying USB NI message
} LonUsbQueueBuffer;

// LON USB interface type enumeration
typedef enum {
	LON_USB_INTERFACE_U50,
	LON_USB_INTERFACE_U61
} LonUsbIfaceType;

// LON USB interface model enumeration
#if LINK_IS(USB)
	typedef enum {
		U60_FT
	} LonUsbIfaceModel;
	#define MAX_IFACE_MODELS 1
#else
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
#endif // LINK_IS(USB)

// LON USB link-layer interface configuration structure
typedef struct LonUsbIfaceConfig {
	LonUsbIfaceType iface_type;
	short usb_line_discipline;
	LonUsbFrameHeaderType frame_header_type;
	bool supports_link_layer_sequencing;
	bool supports_short_local_cmds;
	bool supports_code_packet_acks;
} LonUsbIfaceConfig;

// Special sequence numbers for LonUsbLinkState.downlink_ack_seq_number to indicate special conditions
#define NO_ACK_REQUIRED -2				// No acknowledgment required
#define ACK_WITH_NEXT_SEQ_NUM -1		// Next acknowledgment sequence number to use

// Line discipline for LON USB interfaces; -1 specifies default line discipline
#if OS_IS(LINUX_KERNEL)
#define LON_USB_LDISC_MIP_U50  28
#define LON_USB_LDISC_MIP_U61  27
#else
#define LON_USB_LDISC_MIP_U50  -1
#define LON_USB_LDISC_MIP_U61  -1
#endif	// OS_IS(LINUX_KERNEL)

// LON USB interface configurations indexed by LonUsbIfaceModel
#if LINK_IS(USB)
	__attribute__((used)) static LonUsbIfaceConfig lon_usb_iface_configs[] = {
		// U60 FT and U10 FT Rev C
		{LON_USB_INTERFACE_U50, LON_USB_LDISC_MIP_U50, FRAME_CODE_PACKET, true, true, true}
	};
#else
	static LonUsbIfaceConfig lon_usb_iface_configs[] = {
		// U10 FT Rev A/B
		{LON_USB_INTERFACE_U61, LON_USB_LDISC_MIP_U61, FRAME_SYNC_ONLY, false, false, false},
		// U10 FT Rev C
		{LON_USB_INTERFACE_U50, LON_USB_LDISC_MIP_U50, FRAME_CODE_PACKET, true, true, true},
		// U20 PL
		{LON_USB_INTERFACE_U61, LON_USB_LDISC_MIP_U61, FRAME_SYNC_ONLY, false, false, false},
		// U60 FT
		{LON_USB_INTERFACE_U50, LON_USB_LDISC_MIP_U50, FRAME_CODE_PACKET, true, true, true},
		// U60 TP-1250
		{LON_USB_INTERFACE_U61, LON_USB_LDISC_MIP_U61, FRAME_SYNC_ONLY, false, false, false},
		// U70 PL
		{LON_USB_INTERFACE_U61, LON_USB_LDISC_MIP_U61, FRAME_SYNC_ONLY, false, false, false}
	};
#endif // LINK_IS(USB)

// LON USB link state structure
typedef struct LonUsbLinkState {
	OsalLockType state_lock;				// Mutex or spinlock for this structure
	OsalLockType queue_lock;				// Mutex or spinlock for queue operations
	bool assigned;							// True if this entry is in use
											// Set on start to ALL_PRIORITIES to clear all queues
											// Set on restart to NORMAL_PRIORITY to clear normal queue only
	bool wait_for_reset;					// True if waiting for reset command to be received on uplink
	bool have_reset;						// True if an uplink reset message has been received
	bool wait_for_null;						// True if waiting for null frame response on uplink
	bool have_null;							// True if an uplink null frame has been received
	bool wait_for_uid;						// True if waiting for UID response
	int uid_retries;						// Remaining LON interface unique ID read retries
	bool have_uid;							// True if LON interface unique ID acquired
	bool wait_for_status;					// True if waiting for MIP status response
	int status_retries;						// Remaining MIP status request retries
	bool have_status;						// True if MIP status response received
	bool ready;								// True if link layer is ready for normal operation
    volatile bool shutdown;					// True to terminate any threads
	int iface_index;
	char lon_dev_name[FILENAME_MAX];
	char usb_dev_name[DEVICE_NAME_MAX];
	int usb_fd;								// USB device file descriptor

	// LON USB interface model and type (MIP/U50 vs MIP/U61)
	LonUsbIfaceModel lon_usb_iface_model;
	LonUsbIfaceType lon_usb_iface_type;

	// LON settings and statistics
	uint8_t uid[IZOT_UNIQUE_ID_LENGTH];		   // LON NI unique ID (MAC ID or Neuron ID)
	LonUsbInterfaceMode configured_iface_mode; // LON NI mode (Layer 2 or Layer 5)
	LonUsbInterfaceMode current_iface_mode;    // Current desired LON NI mode; may
											   // differ from configured_iface_mode
											   // during startup and mode changes
	LonStats lon_stats;						   // Last copy of LON stats

	// USB parameters
	LonUsbConfig usb_params;

	// Timestamps
	OsalTickCount start_time;				// Time of interface startup
	OsalTickCount last_timeout;				// Time of last uplink ack timeout

	// Uplink packet state machine
	UplinkState uplink_state;
	OsalTickCount reset_wait_timer;			// Network interface reset timeout timer
	OsalTickCount null_wait_timer;			// NULL frame response timeout timer
	OsalTickCount resync_delay_timer;		// Resynchronization delay timer to synchronize
											// uplink and downlink state machines
	OsalTickCount uid_wait_timer;			// UID response timeout timer
	OsalTickCount status_wait_timer;		// MIP status response timeout timer
	OsalTickCount uplink_msg_timer;			// Uplink wait for message timeout timer
	// OsalTickCount uplink_ack_timer;		// Uplink wait for ack timeout timer
	bool uplink_frame_error;				// True if last uplink had a frame error
	bool uplink_duplicate;					// True if last uplink frame was a duplicate
	LonUsbQueueBuffer uplink_buffer;		// Current uplink buffer being built
	uint8_t uplink_msg[UPLINK_BUF_LEN];		// Uplink packets built here
	int uplink_msg_index;					// Index into uplink_msg[]
	int uplink_msg_length;					// Length or extended length
	int uplink_seq_number;					// Uplink sequence number
	bool uplink_tail_escaped;				// True if last byte of a message was a frame sync byte
	OsalTickCount uplink_ack_timer;			// Start time in ms ticks if waiting for an uplink ack
											// to a downlink packet; 0 if not waiting
	OsalTickCount uplink_ack_timeout;		// Uplink ack timeout duration
	int uplink_ack_timeouts;				// Uplink ack timeout count
	int uplink_ack_timeout_phase;			// Uplink ack timeout phase, incremented on repeated timeouts
	LonNiCommand uplink_expected_rsp;		// Expected uplink response command

	// Downlink packet state machine
	DownlinkState downlink_state;			// Current downlink state
	OsalTickCount downlink_reject_timer;	// Downlink reject timeout timer;
											// Cleared when downlink message is acknowledged
	LonUsbQueueBuffer downlink_buffer_staging;	
											// Staging downlink buffer used to build a message
											// before adding the message to the downlink queue;
											// this is shared across calls but only one message
											// can be processed at a time due to the state lock
	LonUsbQueueBuffer downlink_buffer;		// Current downlink buffer from the head of the downlink
											// queue; this buffer is expanded to
											// exp_downlink_ni_data_frame[] in WriteDownlinkMessage()
	uint8_t exp_downlink_ni_data_frame[MAX_EXP_LON_MSG_EX_LEN];	
											// Expanded downlink data frame used to build the message
											// data frame to be written to the USB interface
	#if 0
	// TBD: delete
	bool downlink_cp_requested[MSG_QUEUE_CMD_COUNT];  // True if downlink code packet requested indexed by LonUsbFrameCommand
	#endif
	int downlink_seq_number;				// Downlink sequence number
	int downlink_ack_seq_number;			// Downlink sequence number to be acknowledged
											// Set to NO_ACK_REQUIRED if no ack is pending
											// Set to ACK_WITH_NEXT_SEQ_NUM if ack is required
											// with the next sequence number; used for both 
											// code packet and message acks
	// TBD: delete next
	// bool downlink_ack_required;			// True if an ack is required to be sent downlink

	// Buffer queues for parsed messages; each queue holds LonUsbQueueBuffer entries
	// Implemented using the generic Queue type from lcs_queue.h
	Queue lon_usb_downlink_normal_queue;	// Downlink normal priority buffer queue
	Queue lon_usb_downlink_priority_queue;	// Downlink high priority buffer queue
	Queue lon_usb_uplink_normal_queue;		// Uplink normal priority buffer queue
	Queue lon_usb_uplink_priority_queue;	// Uplink high priority buffer queue

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
 * Processes retry and downlink requests for a specified LON USB network interface.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   This function processes retry attempts for failed messages and downlink
 *   messages from the downlink queue and writes them to the USB interface. It
 *   is called periodically to handle downlink traffic.
 */
LonStatusCode ProcessDownlinkRequests(int iface_index);

/*
 * Opens a LON USB network interface.
 * Parameters:
 *   lon_dev_name: Logical name for the LON interface, for example "lon0"
 *   usb_dev_name: Host USB interface name, for example "/dev/ttyUSB0"
 *   iface_index: Interface index to be used in subsequent read, write, 
 *   		   and close calls for the interface
 *	 configured_iface_mode: LON interface configured_iface_mode, set to LON_IFACE_MODE_LAYER5
 *			   for layer 5 (host apps) or LON_IFACE_MODE_LAYER2 for
 *			   layer 2 (LON stacks)
 *   line_discipline: USB line discipline number, set to LDISCS_50 for
 *			   the U10 FT Rev C and U60 FT, and LDISCS_61 for the U10 FT
 *			   Rev A and B, U20 PL, U60 TP-1250, and U70 PL
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode OpenLonUsbLink(char *lon_dev_name, char *usb_dev_name,
					int *iface_index, LonUsbInterfaceMode configured_iface_mode,
					LonUsbIfaceModel lon_usb_iface_model);

/*
 * Checks if the LON USB link is ready.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   true if the interface is ready; false otherwise
 */
bool LonUsbLinkReady(int iface_index);

/*
 * Writes a downlink message to the LON USB interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   in_msg: pointer to xLdvMessage or LdvExtendedMessage structure
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode WriteLonUsbMsg(int iface_index, const LonDataFrame* in_msg);

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
LonStatusCode ReadLonUsbMsg(int iface_index, LonDataFrame *out_msg);

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