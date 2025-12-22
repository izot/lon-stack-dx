/*
 * lon_usb_link.c
 *
 * Copyright (c) 2017-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON USB Network Driver Link
 * Purpose: Provides a LON link-layer interface with a U10, U20,
 * 			U60, or U70 LON USB network interface device.
 * Notes:   Concurrency model (per-interface):
 * 			- Each LonUsbLinkState has a mutex lock guarding two shared structures:
 * 			  (1) lon_usb_uplink_ring_buffer (staging ring for raw bytes)
 * 			  (2) uplink_queue (singly-linked list of parsed messages)
 * 			- Producers:
 * 			  • LonUsbFeedRx() may run in an ISR/deferred thread to push bytes into
 * 			    lon_usb_uplink_ring_buffer
 * 			  • ProcessUplinkBytes() (invoked by ReadLonUsbMsg) produces messages
 * 			    into uplink_queue
 * 			- Consumers:
 * 			  • ReadLonUsbMsg() drains lon_usb_uplink_ring_buffer and pops from uplink_queue
 * 			    to return a message to the caller
 * 			- Locking rules:
 * 			  • Always OsalLockMutex(&state->queue_lock) before reading/writing
 * 			    lon_usb_uplink_ring_buffer or uplink_queue, including related stats updates
 * 			  • Keep the critical section minimal; perform parsing and other CPU
 * 			    work outside the lock
 * 			  • For uplink_queue pop, copy the node’s payload inside the lock,
 * 			    then free the node after unlocking to shorten the critical section
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "izot/IzotApi.h"
#include "lon_usb/lon_usb_link.h"

// LON frame sync byte value
#define FRAME_SYNC					0x7E	// Start of frame indicator; escaped when in data

// LON frame code masks
#define FRAME_CODE_SEQ_NUM_MASK		0xE0	// Sequence number mask (bits 5-7)
#define FRAME_CODE_SEQ_NUM_INC		0x20	// Sequence number increment (add to current value; wrap at 7)
#define FRAME_CODE_ACK_MASK			0x10	// Acknowledgment flag mask (bit 4)
#define FRAME_CODE_CMD_MASK			0x0F	// Frame command mask (bits 0-3)

// Sequence number mask
#define SEQ_NUM_MASK				0x03	// Sequence number mask (bits 0-2)

// USB driver parameters and defaults
#define DEFAULT_IN_TRANSFER_SIZE	4096
#define DEFAULT_READ_TIMEOUT		500
#define DEFAULT_WRITE_TIMEOUT		500
#define DEFAULT_UPLINK_LIMIT		200
#define DEFAULT_LLP_TIMEOUT			1000

// Default LON USB interface configuration
static const LonUsbConfig default_lon_usb_config = {
	DEFAULT_IN_TRANSFER_SIZE,
	DEFAULT_READ_TIMEOUT,
	DEFAULT_WRITE_TIMEOUT,
	DEFAULT_UPLINK_LIMIT,
	DEFAULT_LLP_TIMEOUT,
};

// LON network diagnostic messages used by the driver
// TBD: should this be in the format of the LONVXD_Buffer struct - length 1st; response will be CMD/QUE of 0x16
static const L2Frame MsgReportStatus[] = {NI_LOCAL_NM_CMD,	// Network interface command
		MSG_CODE_OFFSET+(1),	// 15 PDU length (is +1 required?)
		0x6F,					// 0 msg_type, 3 st (request), 0 auth, 15 tag (private)
		0x08,					// 0 priority, 0 path, 0 2-bit cmpl_code, 1 addr_mode, 0 alt_path, 0 pool, 0 response
		(1),					// 1 msg length
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Network address
		IzotNdQueryStatus
};

// LON network management messages used by the driver
static const L2Frame MsgClear[] = {NI_CLEAR, 0};
static const L2Frame MsgIdentify[] = {NI_IDENTIFY_CMD, 0};
static const L2Frame MsgModeL2[]	= {NI_LAYER_MODE_CMD, 1, 1};
static const L2Frame MsgModeL5[]	= {NI_LAYER_MODE_CMD, 1, 0};
static const L2Frame MsgStatus[] = {NI_S_STATUS, 0};
static const L2Frame MsgUniqueId[] = {NI_LOCAL_NM_CMD,	// Network interface command
		MSG_CODE_OFFSET+(5),	// 19 PDU length
		0x70,					// 0 msg_type, 3 2-bit st (request), 1 auth, 0 4-bit tag
		0x00,					// 0 priority, 0 path, 0 2-bit cmpl_code, 0 addr_mode, 0 alt_path, 0 pool, 0 response
		(5),					// 5 msg length
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Network address
		0x6d,					// Read memory network management command
		0x01,					// Read-only memory relative address space
		0x00, 0x00,				// 0 offset
		0x06 					// 6 bytes (48-bits)
};

/*****************************************************************
 * Section: Globals
 *****************************************************************/
bool linkInitialized = false; 	// Flag to indicate if LON USB
								// link is initialized
static LonUsbLinkState iface_state[MAX_IFACE_STATES];
								// Interface state array

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
// LON USB link management
static LonStatusCode StartLonUsbLink(int iface_index, LonUsbOpenMode iface_mode);
static LonStatusCode ResetUplinkState(int iface_index);
static LonStatusCode RequestNiUid(int iface_index);
static LonStatusCode SetNiLayerMode(int iface_index);

// LON downlink processing
static LonStatusCode WriteDownlinkMessage(int iface_index);
static LonStatusCode WriteByteDownlink(int iface_index, uint8_t msg_byte);
static LonStatusCode WriteDownlinkLocalNiCmd(int iface_index);
static LonNiCommand GetExpectedResponse(uint8_t cmd);
static LonStatusCode WriteDownlinkReportStatusCmd(int iface_index);
static LonStatusCode WriteDownlinkCodePacket(int iface_index, LonUsbFrameCommand frame_cmd, uint8_t parameter);
static LonStatusCode CreateFrameHeader(LonFrameHeader* frame_header,
				uint8_t frame_command, uint8_t parameter,
				uint8_t sequence_num, bool ack);
static uint8_t ComputeChecksum(uint8_t *pkt, size_t len);
static LonStatusCode IncrementDownlinkSequence(int iface_index);
static LonStatusCode StartAckTimer(int iface_index, DownlinkState next_state);
static LonStatusCode WriteDownlinkCodePacket(int iface_index, LonUsbFrameCommand frame_cmd, uint8_t parameter);
static LonStatusCode ResetDownlinkState(int iface_index);
static LonStatusCode StopAckTimer(int iface_index);
static LonStatusCode InitiateDownlinkCodePackets(int iface_index);

// LON uplink processing
static LonStatusCode ProcessUplinkBytes(int iface_index, uint8_t *chunk, size_t chunk_size);
static LonStatusCode ProcessUplinkCodePacket(int iface_index);
static LonStatusCode ClearUplinkTransaction(int iface_index);
static LonStatusCode CheckUplinkCompleted(int iface_index, bool *completed);
static LonStatusCode QueueUplinkMessage(int iface_index, L2Frame *msg);
static int TotalMessageLength(L2Frame *msg);

// Interface state array management
static LonStatusCode InitIfaceStates(void);
static int AssignIfaceState(void);
static LonUsbLinkState* GetIfaceState(int iface_index);
static int CountFreeIfaceStates(void);

// Downlink queue management
static LonStatusCode PeekDownlinkBuffer(int iface_index, LonUsbQueueBuffer *dst);
static LonStatusCode ReadDownlinkBuffer(int iface_index, LonUsbQueueBuffer *dst);
static LonStatusCode FlushDownlinkQueue(int iface_index, MessagePriorityLevel priority);
static LonStatusCode QueueDownlinkBuffer(int iface_index, LonUsbQueueBuffer *src);
static size_t GetDownlinkBufferCount(int iface_index);
static bool IsDownlinkQueueEmpty(int iface_index);

// Uplink queue management
static LonStatusCode PeekUplinkBuffer(int iface_index, LonUsbQueueBuffer *dst);
static LonStatusCode ReadUplinkBuffer(int iface_index, LonUsbQueueBuffer *dst);
static LonStatusCode QueueUplinkBuffer(int iface_index, LonUsbQueueBuffer *src);
static size_t GetUplinkBufferCount(int iface_index);
static bool IsUplinkQueueEmpty(int iface_index);

// Debugging support functions
static void PrintMessage(const uint8_t *msg, size_t length);
static void PrintUplinkCodePacket(int iface_index);

// Implementation-specific LON NI functions
static void IdentifyLonNi(int iface_index);

/*****************************************************************
 * Section: Initialization Function Definitions
 *****************************************************************/
/*
 * Opens a LON USB network interface.
 * Parameters:
 *   lon_dev_name: Logical name for the LON interface, for example "lon0"
 *   usb_dev_name: Host USB interface name, for example "/dev/ttyUSB0"
 *   iface_index: LON interface index to be used in subsequent read, write,
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
					LonUsbIfaceModel lon_usb_iface_model)
{
	if (!lon_dev_name || !usb_dev_name || !iface_index
			|| (iface_mode != LON_USB_OPEN_LAYER2 && iface_mode != LON_USB_OPEN_LAYER5)
			|| (lon_usb_iface_model < 0 || lon_usb_iface_model >= MAX_IFACE_MODELS)) {
		return LonStatusInvalidParameter;
	}
	// Initialize link interface states if this is the first LON USB open
	if (!linkInitialized) {
		LonStatusCode code;
		code = InitIfaceStates();
		if (code != LonStatusNoError) {
			*iface_index = -1;
			OsalPrintError(code, "Failed to inititialize LON USB link interface states");
			return code;
		}
		linkInitialized = TRUE;
	}
	// Assign available link interface state
	*iface_index = AssignIfaceState();
	if ((*iface_index < 0) || (*iface_index >= MAX_IFACE_STATES)) {
		*iface_index = -1;
		return LonStatusNoMoreCapableDevices;
	}
	// Get pointer to assigned interface state
	LonUsbLinkState *state = GetIfaceState(*iface_index);
	if (state == NULL) {
		ResetUplinkState(*iface_index);
		*iface_index = -1;
		return LonStatusOpenFailed;
	}
	// Copy LON device name to state and ensure nul-termination
    strncpy(state->lon_dev_name, lon_dev_name, sizeof(state->lon_dev_name) - 1);
    state->lon_dev_name[sizeof(state->lon_dev_name) - 1] = '\0';

    // Copy USB device name to state and ensure nul-termination
    strncpy(state->usb_dev_name, usb_dev_name, sizeof(state->usb_dev_name) - 1);
    state->usb_dev_name[sizeof(state->usb_dev_name) - 1] = '\0';

	// Store LON USB interface model
	state->lon_usb_iface_model = lon_usb_iface_model;

	// Initialize USB link and get the USB device file descriptor
	int fd = HalOpenUsb(usb_dev_name, lon_usb_iface_configs[state->lon_usb_iface_model].usb_line_discipline);
    if (fd < 0) {
		ResetUplinkState(*iface_index);
		*iface_index = -1;
        return LonStatusOpenFailed;
    }
	state->usb_fd = fd;
	
	// Start LON USB link
	LonStatusCode code = StartLonUsbLink(*iface_index, iface_mode);
	if (code != LonStatusNoError) {
		HalCloseUsb(fd);
		ResetUplinkState(*iface_index);
		*iface_index = -1;
	}
	state->start_time = state->last_timeout = OsalGetTickCount();
	return code;
};

/*
 * Starts the link for a specified LON USB network interface.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 *   iface_mode: LON interface iface_mode, set to
 *             LON_USB_OPEN_LAYER5 for layer 5 (host apps) or
 *             LON_USB_OPEN_LAYER2 for layer 2 (LON stacks)
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Called at end of OpenLonUsbLink() and ProcessDownlinkRequests().
 */
static LonStatusCode StartLonUsbLink(int iface_index, LonUsbOpenMode iface_mode)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	L2Frame *mode_msg;

	OsalPrintDebug(code, "Link start: interface index %d, interface mode %d", iface_index, iface_mode);

	// Reset LON USB link state
	code = ResetUplinkState(iface_index);
	if (code != LonStatusNoError) {
		OsalPrintError(code, "Failed to reset uplink state for interface index %d", iface_index);
		return code;
	}
    state->iface_mode = iface_mode;

	// MIP/U50 specific startup commands
	if (state->lon_usb_iface_model == LON_USB_INTERFACE_U50) {
		// Set LON interface layer mode specified by iface_mode; Layer 2 is required for LON stacks
		// and Layer 5 is typically used for host applications
		mode_msg = (iface_mode == LON_USB_OPEN_LAYER5) ? (L2Frame*) MsgModeL5 : (L2Frame*) MsgModeL2;
		code = WriteLonUsbMsg(iface_index, mode_msg);
		if (code != LonStatusNoError) {
			OsalPrintError(code, "Failed to send network interface mode command to MIP/U50 for interface index %d", iface_index);
			return code;
		}
		// Send status request
		code = WriteLonUsbMsg(iface_index, (const L2Frame*) MsgStatus);
		if (code != LonStatusNoError) {
			OsalPrintError(code, "Failed to send status command to MIP/U50 for interface index %d", iface_index);
			return code;
		}
		// Send clear command to clear any pending faults
		code = WriteLonUsbMsg(iface_index, (const L2Frame*) MsgClear);
		if (code != LonStatusNoError) {
			OsalPrintError(code, "Failed to send clear command to MIP/U50 for interface index %d", iface_index);
			return code;
		}
	}
	// Send read memory request for the network interface unique ID
	state->wait_for_uid = TRUE;
	state->uid_retries = MAX_UID_RETRIES;
	state->have_uid = FALSE;
	state->shutdown = FALSE; // Now started
	return RequestNiUid(iface_index);
}

/*
 * Resets uplink state for the start of a new packet.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Called by:
 *	   - InitIfaceStates() which is called by OpenLonUsbLink()
 *	   - StartLonUsbLink() which is called by OpenLonUsbLink()
 *	   - AssignIfaceState() for a newly assigned state which is
 *	     called by OpenLonUsbLink()
 *	   - ProcessUplinkBytes() after a framing error which is called
 *	     by ReadLonUsbMsg()
 *	   - CheckUplinkCompleted() on success which is called by 
 *		 ProcessUplinkBytes()
 *	   - AssignIfaceState() which is called by OpenLonUsbLink()
 */
static LonStatusCode ResetUplinkState(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	state->uplink_ack_timer = 0;
	state->uplink_ack_timeout = ACK_WAIT_TIME;
	state->uplink_ack_timeouts = 0;
	state->uplink_ack_timeout_phase = 0;
	state->uplink_duplicate = false;
	state->uplink_frame_error = false;
	state->uplink_msg_index = 0;
	state->uplink_msg_length = 0;
	state->uplink_msg_timer = 0;
	state->uplink_state = UPLINK_IDLE;
	state->uplink_tail_escaped = false;
	return LonStatusNoError;
}

/*
 * Sends a read memory request for the LON interface unique ID.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode RequestNiUid(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	if (state->uid_retries <= 0) {
		// UID read failed after max retries; stop waiting for UID
		state->wait_for_uid = FALSE;
		OsalPrintError(LonStatusTimeout, "Failed to read LON interface %d unique ID after %d attempts",
				iface_index, MAX_UID_RETRIES);
		state->uid_retries = 0;
		// Set the LON interface layer mode and return
		return SetNiLayerMode(iface_index);
	}
	code = WriteLonUsbMsg(iface_index, (const L2Frame*) MsgUniqueId);
	if (code != LonStatusNoError) {
		return code;
	}
	state->uid_retries--;
	state->wait_for_uid = TRUE;
	state->have_uid = FALSE;
	state->uplink_msg_timer = OsalGetTickCount(); // Start UID wait timer
	OsalPrintDebug(LonStatusNoError, "Sent read memory request for the LON interface %d device unique ID, %d retries left",
			iface_index, state->uid_retries);
	return code;
}

/*
 * Sets the LON interface layer mode to layer 2 or layer 5.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode SetNiLayerMode(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	if (state->iface_mode == LON_USB_OPEN_LAYER5) {
		code = WriteLonUsbMsg(iface_index, (const L2Frame*) MsgModeL5);
	} else {
		code = WriteLonUsbMsg(iface_index, (const L2Frame*) MsgModeL2);
	}
	OsalPrintDebug(LonStatusNoError, "Sent LON interface layer mode request for interface index %d", iface_index);
	return code;
}

/*****************************************************************
 * Section: Downlink Function Definitions
 *****************************************************************/
/*
 * Writes a downlink message to the LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   in_msg: pointer to L2Frame or LdvExtendedMessage structure
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Typically called by Layer 2 of a LON stack with the network interface
 *   in layer 2 mode.  May also be called by a host application with the 
 *   network interface in layer 5 mode.
 */
LonStatusCode WriteLonUsbMsg(int iface_index, const L2Frame* in_msg)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoBufferAvailable;

	OsalLockMutex(&state->state_lock);
	if (GetDownlinkBufferCount(iface_index) < MAX_LON_DOWNLINK_BUFFERS) {
		// Build downlink buffer -- the sequence number and checksum
		// will be updated when the buffer is written to the interface
		code = CreateFrameHeader(&state->downlink_buffer.frame_header,
				MSG_FRAME_CMD, 0, 0, TRUE);
		if (code == LonStatusNoError) {
			if (in_msg->len == EXT_LENGTH) {
				// This is an extended message (length > 254)
				const LdvExtendedMessage *in_ext_msg = (LdvExtendedMessage *)in_msg;		// Source
				UsbNiExtendedMessage *ni_ext_msg = (UsbNiExtendedMessage *) &state->downlink_buffer.usb_ni_message.cmd;	// Dest
				if(in_ext_msg->ext_length <= MAX_LON_MSG_EX_LEN) {
					memcpy(&ni_ext_msg->ext_pdu, &in_ext_msg->ext_pdu, in_ext_msg->ext_length);
					ni_ext_msg->ext_flag = EXT_LENGTH;
					ni_ext_msg->ext_length = EndianSwap16(in_ext_msg->ext_length + 1);
					ni_ext_msg->cmd = in_ext_msg->cmd;
					code = LonStatusNoError;
				} else {
					code = LonStatusInvalidBufferLength;
				}
			} else {
				LonNiCommand ni_command = in_msg->cmd;
				L2Frame *ni_msg = &state->downlink_buffer.usb_ni_message;				// Dest
				// Test for network interface layer mode command from the client
				if (ni_command == NI_LAYER_MODE_CMD && in_msg->len) {
					ni_msg->len = 0;
					ni_msg->cmd = (in_msg->pdu[0] == 0) ? NI_L5_MODE_CMD : NI_L2_MODE_CMD;
					// Set the interface mode used for this link after NI reset
					state->iface_mode = (in_msg->pdu[0] == 0) ? LON_USB_OPEN_LAYER5 : LON_USB_OPEN_LAYER2;
				} else {
					ni_msg->len = in_msg->len + 1;	// Bump to include cmd
					ni_msg->cmd = in_msg->cmd;
				}
				memcpy(ni_msg->pdu, in_msg->pdu, in_msg->len);
				code = LonStatusNoError;
			}
		}
		if (code == LonStatusNoError) {
			code = QueueDownlinkBuffer(iface_index, &state->downlink_buffer);
		}
	}
	OsalUnlockMutex(&state->state_lock);
	return code;
}

/*
 * Writes a downlink code packet to the LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   frame_command: frame command value
 *   parameter: frame parameter value
 *   sequence_num: frame sequence number
 *   ack: acknowledgment flag
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode WriteCodePacket(int iface_index, unsigned int frame_command,
				unsigned int parameter, unsigned int sequence_num, bool ack)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoBufferAvailable;

	OsalLockMutex(&state->state_lock);
	if (GetDownlinkBufferCount(iface_index) < MAX_LON_DOWNLINK_BUFFERS) {
		// Build downlink buffer -- the sequence number and checksum
		// will be updated when the buffer is written to the interface
		code = CreateFrameHeader(&state->downlink_buffer.frame_header,
				frame_command, parameter, sequence_num, ack);
		if (code == LonStatusNoError) {
			state->downlink_buffer.usb_ni_message.len = 0;	// No payload
			state->downlink_buffer.usb_ni_message.cmd = 0;	// No NI command
			code = QueueDownlinkBuffer(iface_index, &state->downlink_buffer);
		}
	}
	OsalUnlockMutex(&state->state_lock);
	return code;
}

/*
 * Creates a LON frame header.
 * Parameters:
 *   frame_header: pointer to LonFrameHeader structure to populate
 *   frame_command: frame command value
 *   sequence_num: frame sequence number
 *   ack: acknowledgment flag
 *   parameter: frame parameter value
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode CreateFrameHeader(LonFrameHeader* frame_header,
				uint8_t frame_command, uint8_t parameter,
				uint8_t sequence_num, bool ack)
{
	frame_header->frame_sync = FRAME_SYNC;
	frame_header->frame_code.frame_cmd = frame_command;
	frame_header->parameter = parameter;
	frame_header->frame_code.sequence_num = sequence_num;
	frame_header->frame_code.ack = ack;
	frame_header->checksum = -ComputeChecksum((uint8_t *)frame_header, sizeof(LonFrameHeader) - 1);
	return LonStatusNoError;
}

/*
 * Computes the checksum for a LON USB packet.
 * Parameters:
 *   pkt: pointer to the packet data
 *   len: length of the packet data
 * Returns:
 *   Computed checksum byte
 * Notes:
 *   Checksum is computed as the 8-bit sum of all bytes in the packet.
 *   The packet may be the frame header or the full message payload.
 *   When computing a checksum for a packet, the checksum byte is negated
 *   and appended to the end of the packet.  When verifying a packet checksum,
 *   the checksum byte is included in the sum and the result should be zero.
 *   Called by WriteLonUsbMsg() and ProcessUplinkBytes().
 */
static uint8_t ComputeChecksum(uint8_t *pkt, size_t len)
{
	size_t i;
	uint8_t sum = 0;

	for(i=0; i<len; i++) {
		sum += pkt[i];
	}
	return sum;
}


static bool ValidateChecksum(int iface_index, uint8_t *pkt, size_t len)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return false;
	}
	bool is_valid = ComputeChecksum(pkt, len) == 0;
	if (!is_valid) {
		OsalPrintError(LonStatusChecksumError, "Invalid checksum computed for %zu-byte packet",len);
		Increment32(state->lon_stats.uplink.rx_checksum_errors);
	}
	return is_valid;
}

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
LonStatusCode ProcessDownlinkRequests(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	DownlinkState starting_downlink_state = state->downlink_state;

	switch(state->downlink_state) {
	case DOWNLINK_START:
		// Flush any pending non-priority downlink messages and start or restart the downlink
		OsalPrintDebug(code, "Processing downlink start for interface index %d", iface_index);
		FlushDownlinkQueue(iface_index, NORMAL_PRIORITY);
		StartLonUsbLink(iface_index, state->iface_mode);
		state->downlink_state = DOWNLINK_IDLE;
		break;

	case DOWNLINK_CP_RESPONSE_WAIT:
	case DOWNLINK_CP_ACK_WAIT:
	case DOWNLINK_MSG_ACK_WAIT:
	case DOWNLINK_CP_MSG_REQ_ACK_WAIT:
		// Waiting for an ACK or response
		if (OsalGetTickCount() - state->uplink_ack_timer < state->uplink_ack_timeout) {
			// Timer has not timed out
			break;
		}
		// Wait timed out; handle timeout
		state->downlink_state = DOWNLINK_IDLE;
		Increment32(state->lon_stats.uplink.rx_ack_timeout_errors);
		// Look for runt CPs in the RX path that could clog up things
		if (state->uplink_state == UPLINK_IDLE && state->uplink_msg_index) {
			ResetUplinkState(iface_index);
		}
		if (++state->uplink_ack_timeouts >= 2) {
			if (state->uplink_ack_timeout_phase < 5) {
				// Many ack timeouts occurred; re-sync by requesting ND Status
				IncrementDownlinkSequence(iface_index);
				// Overwrite the downlink message at state->downlink_buffer.usb_ni_message
				memcpy(&state->downlink_buffer.usb_ni_message, &MsgReportStatus, sizeof(MsgReportStatus));
				WriteDownlinkMessage(iface_index);
				if (++state->uplink_ack_timeout_phase == 0) {
					state->uplink_ack_timeout_phase--;
				}
				OsalPrintTrace(LonStatusTimeout, "Interface %d ACK timeout phase %d, sequence #%02X, requesting status",
					iface_index, state->uplink_ack_timeout_phase, state->downlink_seq_number);
				OsalPrintTrace(LonStatusTimeout, "Interface %d checksum errors: %d  code packet failures: %d",
					iface_index, state->lon_stats.uplink.rx_checksum_errors, state->lon_stats.uplink.rx_code_packet_failures);
				OsalPrintTrace(LonStatusTimeout, "Interface %d packets received: %d, bytes received: %d",
					iface_index, state->lon_stats.uplink.packets_received, state->lon_stats.uplink.bytes_received);
				OsalPrintTrace(LonStatusTimeout, "Interface %d TX state %d, RX state %d",
					iface_index, starting_downlink_state, state->uplink_state);
			} else {		// Do this once
				OsalTickCount current_time = OsalGetTickCount();
				// If we have a MAC address and it's been more than 60 seconds since startup or the last timeout
				// then panic and reset the interface
				if (!state->wait_for_uid && (current_time - state->last_timeout) > STARTUP_DELAY) {
					OsalPrintError(LonStatusTimeout, "Interface [%d] ACK timeout limit #%d, restart now", iface_index, state->uplink_ack_timeout_phase);
					state->last_timeout = current_time;		// Don't keep doing this too rapidly
				} else {	// Try again
					state->uplink_ack_timeout_phase = 0;
				}
			}
			state->uplink_ack_timeouts = 0;
		} else {
			OsalPrintTrace(LonStatusTimeout, "Interface %d ACK timeout (first case)", iface_index);
		}
		// Fall into the next case
	case DOWNLINK_IDLE:
		// Look for downlink buffers to process
		code = ReadDownlinkBuffer(iface_index, &state->downlink_buffer);
		if (code != LonStatusNoError) {
			break;
		}
		// Message is available; process it
		size_t pdu_size = state->downlink_buffer.usb_ni_message.len;
		if (pdu_size == 1) {
			// Process local NI command (PDU size of 1 means there is no payload)
			ResetDownlinkState(iface_index);
			WriteDownlinkLocalNiCmd(iface_index);
			break;
		}
		bool drop_it = false;
		LonNiCommand cmd = state->downlink_buffer.usb_ni_message.cmd;
		uint8_t *pdu = state->downlink_buffer.usb_ni_message.pdu;
		if ((state->iface_mode == LON_USB_OPEN_LAYER2)
				&& ((cmd == (NI_COMM|NI_START_TXN)) || (cmd == (NI_COMM|NI_PRIORITY_TXN)))
				&& (((pdu[1]) & 0xC0) == 0x40)) {
			// Drop LonTalk V1 packet except for ICMP ping (see AP-2549)
			if (pdu[IPV4_TOS] == 0 && pdu[IPV4_PROTO] == 1) {					// ICMP
				if (pdu[IPV4_ICMP_TYPE] == 8 && pdu[IPV4_ICMP_CODE] == 0) {		// ICMP ping
					OsalPrintTrace(code, "Received ICMP ping message");
				} else {
					drop_it = true;
					OsalPrintTrace(code, "Dropping ICMP message");
				}
			} else {
				drop_it = true;
				OsalPrintTrace(code, "Dropping LonTalk V1 message with %d bytes", pdu_size);
				PrintMessage(pdu, pdu_size);
			}
		}
		if (drop_it) {
			// Drop the message
			ResetDownlinkState(iface_index);
		} else {
			WriteDownlinkMessage(iface_index);
		}
		break;

	default:
		// Unused or undefined state; reset to idle
		state->downlink_state = DOWNLINK_IDLE;
		break;
	}	// End of switch

	// Send any code packets requested by the uplink processing, plus possibly an ack
	InitiateDownlinkCodePackets(iface_index);

	if (state->downlink_ack_required) {
		// Downlink ack requested but not yet sent; send a null CP to send the ack
		WriteDownlinkCodePacket(iface_index, NULL_FRAME_CMD, 0);
	}
	return code;
}

/*
 * Increments the downlink sequence number modulo 8, skipping 0.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode IncrementDownlinkSequence(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	state->downlink_seq_number += 1;
	state->downlink_seq_number &= SEQ_NUM_MASK;	// Modulo 8
	if(state->downlink_seq_number == 0) {
		state->downlink_seq_number = 1;	// Sequence number 0 is not used
	}
	return LonStatusNoError;
}

/*
 * Writes the downlink message to the LON USB interface with byte expansion.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Sends a code packet, expands the message by stuffing a FRAME_SYNC byte
 *   after any embedded FRAME_SYNC bytes, computes and adds a checksum,
 *   and writes the expanded message to the USB interface.  Called by
 *   ProcessDownlinkRequests().
 */
static LonStatusCode WriteDownlinkMessage(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	uint8_t *dest_exp_message_ptr, *source_message_ptr, message_byte;
 	uint16_t unexpanded_length, remaining;
	static uint8_t dest_exp_message[MAX_EXP_LON_MSG_EX_LEN];
	static size_t expanded_length;
 	uint8_t checksum;
	L2Frame source_message = state->downlink_buffer.usb_ni_message;
	
	// Send code packet prior to the message packet
	WriteDownlinkCodePacket(iface_index, MSG_FRAME_CMD, 1);
	
	// Build the downlink message packet with byte expansion for FRAME_SYNC bytes
 	source_message_ptr = (uint8_t *)&(source_message.cmd);
	unexpanded_length = expanded_length = remaining = TotalMessageLength(&source_message);
	dest_exp_message_ptr = dest_exp_message;
	checksum = -ComputeChecksum(source_message_ptr, unexpanded_length);
	while(remaining--) {
		message_byte = *dest_exp_message_ptr++ = *source_message_ptr++;
		if(message_byte == FRAME_SYNC) {
			*dest_exp_message_ptr++ = FRAME_SYNC;
			expanded_length++;
		}
		if(expanded_length >= sizeof(dest_exp_message) - 2) {
			break;
		}
	}
	if((*dest_exp_message_ptr++ = checksum) == FRAME_SYNC) {
		*dest_exp_message_ptr++ = FRAME_SYNC;
		expanded_length++;
	}
	expanded_length++;
	code = StartAckTimer(iface_index, DOWNLINK_MSG_ACK_WAIT);
	if (code != LonStatusNoError) {
		return code;
	}
	// Write the expanded message to the USB interface
	size_t bytes_written = 0;
	code = HalWriteUsb(state->usb_fd, dest_exp_message, expanded_length, &bytes_written);
	if (code != LonStatusNoError || bytes_written != expanded_length) {
		OsalPrintDebug(code, "Wrote %zu of %d bytes to LON USB interface", bytes_written, expanded_length);
		Increment32(state->lon_stats.downlink.tx_aborted_errors);
		return (code == LonStatusNoError) ? LonStatusWriteFailed : code;
	}
	// Update statistics
	Increment32(state->lon_stats.downlink.packets_sent);
	Add32(state->lon_stats.downlink.bytes_sent, bytes_written);
	OsalPrintTrace(code, "Downlink write succeeded with 0x%02X message code and %zu bytes", source_message.cmd, bytes_written);
	PrintMessage(state->downlink_buffer.usb_ni_message.pdu, unexpanded_length);
	return code;
}

/*
 * Starts the acknowledgment timer for the uplink.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 *   next_state: next downlink state to set
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode StartAckTimer(int iface_index, DownlinkState next_state)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	state->uplink_ack_timer = OsalGetTickCount();
	state->uplink_ack_timeouts = 0;
	state->downlink_state = next_state;
	return LonStatusNoError;
}

/*
 * Writes a downlink code packet to the LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   frame_cmd: frame command value
 *   parameter: frame parameter value
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode WriteDownlinkCodePacket(int iface_index, LonUsbFrameCommand frame_cmd, uint8_t parameter)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	LonFrameHeader frame_header;

	// Build the frame header
	memset(&frame_header, 0, sizeof(frame_header));
	frame_header.frame_sync = FRAME_SYNC;
	frame_header.frame_code.frame_cmd = frame_cmd;
	if (frame_cmd == MSG_FRAME_CMD) {
		frame_header.frame_code.sequence_num = state->downlink_seq_number;
	}
	if (state->downlink_ack_required) {
		if (frame_cmd != FAIL_FRAME_CMD) {
			// Frame command is not reporting a failure; set the ack flag
			frame_header.frame_code.ack = 1;
		}
		state->downlink_ack_required = FALSE;
	}
	frame_header.parameter = parameter;
	// Compute the checksum and negate it; don't include the checksum byte itself
	frame_header.checksum = -ComputeChecksum((uint8_t *)&frame_header, sizeof(frame_header)-1);

	// Send the code packet
	size_t bytes_written = 0;
	code = HalWriteUsb(state->usb_fd, (uint8_t *)&frame_header, sizeof(frame_header), &bytes_written);
	if (code != LonStatusNoError) {
		OsalPrintError(code, "Failed to write downlink code packet with command 0x%02X, parameter 0x%02X, sequence #%02X",
				frame_cmd, parameter, state->downlink_seq_number);
		return code;
	}
	if (bytes_written != sizeof(frame_header)) {
		OsalPrintError(LonStatusLniWriteFailure, "Failed to write downlink code packet with command 0x%02X, parameter 0x%02X, sequence #%02X",
				frame_cmd, parameter, state->downlink_seq_number);
		return LonStatusLniWriteFailure;
	}
	OsalPrintTrace(LonStatusNoError, "Sent downlink code packet with command 0x%02X, parameter 0x%02X, sequence #%02X",
			frame_cmd, parameter, state->downlink_seq_number);
	return code;
}

/*
 * Writes a downlink local NI command to the LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode WriteDownlinkLocalNiCmd(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	uint8_t cmd = state->downlink_buffer.usb_ni_message.cmd;
	state->uplink_expected_rsp = GetExpectedResponse(cmd);
	DownlinkState next_downlink_state = state->uplink_expected_rsp ? DOWNLINK_CP_RESPONSE_WAIT : DOWNLINK_CP_ACK_WAIT;
	code = WriteDownlinkCodePacket(iface_index, SHORT_NI_CMD_FRAME_CMD, cmd);
	if (code != LonStatusNoError) {
		OsalPrintError(code, "Failed to write downlink local NI command 0x%02X for interface index %d", cmd, iface_index);
		return code;
	}
	code = StartAckTimer(iface_index, next_downlink_state);
	if (code != LonStatusNoError) {
		OsalPrintError(code, "Failed to start ACK timer for local NI command 0x%02X for interface index %d", cmd, iface_index);
		return code;
	}
	OsalPrintDebug(LonStatusNoError, "Processed local NI command 0x%02X for interface index %d", cmd, iface_index);
	return code;
}

/*
 * Gets the expected response NI command for a given NI command.
 * Parameters:
 *   cmd: NI command byte
 * Returns:
 *   Expected response NI command; NI_CLEAR if no response is expected
 * Notes:
 *   Some versions of the MIP firmware have the following behavior:
 *   1. The host sends a SHORT_NI_CMD_FRAME_CMD code packet that requires a response.
 *   2. The MIP responds with a code packet ACK, but the ACK is in response to a
 *      previous command, not the current one.  For example, the MIP may respond
 *      with an ACK to a previous command because the host has not yet acknowledged
 *      a previously pending uplink short command.
 *   3. The host sends another SHORT_NI_CMD_FRAME_CMD code packet overwriting the
 *      command sent in number 1.
 *   To prevent losing the first command, a downlink short command is not acknowledged
 *   if there is another downlink short command pending.  Instead, the host waits for
 *   the expected response to the command before sending an ACK.
 *   Called by WriteDownlinkLocalNiCmd().
 */
static LonNiCommand GetExpectedResponse(uint8_t cmd)
{
	LonNiCommand expected_response = NI_CLEAR;
	switch(cmd) {
		case NI_S_STATUS:
		case NI_L5_MODE_CMD:
		case NI_L2_MODE_CMD:
			expected_response = cmd;
			break;
		case NI_INITIATE:
			expected_response = NI_CHALLENGE;
			break;
	}
	return expected_response;
}

/*
 * Resets the downlink state for a specified LON USB network interface.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode ResetDownlinkState(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	OsalLockMutex(&state->state_lock);
	if (state->downlink_buffer.buf_size > 0) {
		memset(&state->downlink_buffer, 0, sizeof(state->downlink_buffer));
		state->downlink_reject_timer = 0;
	}
	OsalUnlockMutex(&state->state_lock);
	// Clear the ack timer and change state to idle
	code = StopAckTimer(iface_index);
	return code;
}

/*
 * Stops the acknowledgment timer for the uplink and sets the downlink state to idle.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode StopAckTimer(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	state->downlink_state = DOWNLINK_IDLE;
	state->uplink_ack_timer = 0;
	state->uplink_ack_timeout = ACK_WAIT_TIME;
	state->uplink_ack_timeout_phase = 0;
	state->uplink_ack_timeouts = 0;
	return LonStatusNoError;
}

/*
 * Initiates downlink code packets for a specified LON USB network interface.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode InitiateDownlinkCodePacket(int iface_index, LonUsbFrameCommand frame_command)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	if (frame_command >= MSG_QUEUE_CMD_COUNT) {
		return LonStatusInvalidParameter;
	}
	state->downlink_cp_requested[frame_command] = TRUE;
	return LonStatusNoError;
}

/*
 * Initiates all requested downlink code packets for a specified LON USB network interface.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode InitiateDownlinkCodePackets(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	for (int i = 0; i < MSG_QUEUE_CMD_COUNT; i++) {
		if (state->downlink_cp_requested[i]) {
			WriteDownlinkCodePacket(iface_index, (LonUsbFrameCommand)i, 0);
			state->downlink_cp_requested[i] = FALSE;
		}
	}
	return LonStatusNoError;
}

// TBD: verify that length stuffing is correct for extended messages (see the U61 code);
//      zero out the downlink buffer before use to avoid garbage in unused fields;
//      verify sequence number handling; verify layer 2 vs layer 5 operation;
//      verify handling of local NI commands; verify handling of LON protocol V1

/*****************************************************************
 * Section: Uplink Function Definitions
 *****************************************************************/
/*
 * Reads an uplink message from the LON USB interface, if available.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   out_msg: pointer to L2Frame or LdvExtendedMessage structure
 * Returns:
 *   LonStatusNoError if a message is successfully read;
 *   LonStatusNoMessageAvailable if no full message is available;
 *   LonStatusCode error code if unsuccessful
 * Notes:
 *   Non-blocking; reads available bytes and returns a single message if
 *   available.  If no full message is available, tests for timeout waiting
 *   for the LON interface unique ID (UID) and retries the UID read request.
 *   TBD: this implementation is for the MIP/U61 interface; refactor for U50.
 */
LonStatusCode ReadLonUsbMsg(int iface_index, L2Frame *out_msg)
{
	if (!out_msg) {
		return LonStatusInvalidParameter;
	}
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;

	// Stage 1: attempt a non-blocking read into a temporary buffer
	int fd = state->usb_fd;
	// Check available ring capacity under queue_lock to avoid over-pulling from USB
	size_t rb_avail = 0;
	// Lock ring to safely check available capacity; another thread may feed concurrently
	// Protect queue head access; other threads may enqueue/dequeue concurrently
	OsalLockMutex(&state->queue_lock);
	rb_avail = RingBufferAvail(&state->lon_usb_uplink_ring_buffer);
	OsalUnlockMutex(&state->queue_lock); // Keep critical section minimal
	if (fd >= 0 && rb_avail > MAX_BYTES_PER_USB_READ) {
		uint8_t temp_read_buf[MAX_BYTES_PER_USB_READ];
		ssize_t bytes_read = 0;
		code = HalReadUsb(fd, temp_read_buf, sizeof(temp_read_buf), &bytes_read);
		if (code == LonStatusNoError && bytes_read > 0) {
			// Lock ring while writing staged bytes and updating staging stats
			OsalLockMutex(&state->queue_lock);
			size_t written = RingBufferWrite(&state->lon_usb_uplink_ring_buffer, temp_read_buf, (size_t)bytes_read);
			size_t occ = RingBufferSize(&state->lon_usb_uplink_ring_buffer);
			state->lon_stats.usb_rx.bytes_fed += written;
			if (occ > state->lon_stats.usb_rx.max_occupancy) state->lon_stats.usb_rx.max_occupancy = occ;
			if (written < (size_t)bytes_read) {
				size_t dropped = (size_t)bytes_read - written;
				state->lon_stats.usb_rx.bytes_dropped += dropped;
				OsalUnlockMutex(&state->queue_lock); // Release before logging
				OsalPrintError(LonStatusOverflow, "RX ring overflow: dropped %zu of %zd bytes", dropped, bytes_read);
			} else {
				OsalUnlockMutex(&state->queue_lock); // Leave lock before returning
			}
		} else if (code != LonStatusNoError && code != LonStatusNoMessageAvailable) {
			// Propagate hard error (timeout/device/read failure)
			return code;
		}
	}

	// Stage 2: drain any accumulated bytes from the ring buffer in modest
	// chunks and feed the parser--this allows smoothing of bursty UART/USB
	// input and reduces immediate stack usage; skip if no space in uplink buffer;
	// lock ring to snapshot count--parser will process outside of lock;
	// protect pop of head node to avoid races with concurrent writers/readers
	OsalLockMutex(&state->queue_lock);
	size_t ring_count = RingBufferSize(&state->lon_usb_uplink_ring_buffer);
	OsalUnlockMutex(&state->queue_lock); // Free outside of lock to keep critical section small
	if (ring_count > 0 && GetUplinkBufferCount(iface_index) < MAX_LON_UPLINK_BUFFERS) {
		uint8_t parse_chunk[MAX_BYTES_PER_USB_PARSE_CHUNK];
		size_t processed_in_window = 0;
		for (;;) {
			// Lock ring for atomic read of a chunk--keep scope tight;
			// protect tail walk and append under lock--single-linked O(n) by design
			OsalLockMutex(&state->queue_lock);
			size_t cur_count = RingBufferSize(&state->lon_usb_uplink_ring_buffer);
			if (cur_count == 0) {
				OsalUnlockMutex(&state->queue_lock);
				break;
			}
			size_t chunk_size = RingBufferRead(&state->lon_usb_uplink_ring_buffer, parse_chunk, sizeof(parse_chunk));
			OsalUnlockMutex(&state->queue_lock);
			if (chunk_size == 0) break; // Should not happen
			ProcessUplinkBytes(iface_index, parse_chunk, chunk_size);
			// Update staging stats under lock to avoid races with feeder
			// Protect O(n) count traversal to avoid concurrent mutation
			OsalLockMutex(&state->queue_lock);
			state->lon_stats.usb_rx.bytes_read += chunk_size;
			OsalUnlockMutex(&state->queue_lock);
			processed_in_window += chunk_size;
			// Break early if a message has been queued to minimize per-call latency
			if (GetUplinkBufferCount(iface_index) > 0) break;
			// Guard against monopolizing CPU if ring is very full; parse only a window per call
			if (processed_in_window > MAX_BYTES_PER_USB_PARSE_WINDOW) break; // Arbitrary processing budget
		}
	}

	// Stage 3: if a message is available, return it
	static LonUsbQueueBuffer buffer;
	code = ReadUplinkBuffer(iface_index, &buffer);
	if (code != LonStatusNoError && code != LonStatusNoBufferAvailable) {
		return code;
	}
	if (code != LonStatusNoBufferAvailable) {
		// Message is available; copy to output buffer
		memset(out_msg, 0, sizeof(*out_msg));
		out_msg->cmd = buffer.usb_ni_message.cmd;
		out_msg->len = (uint8_t)(buffer.buf_size - 1);
		if (out_msg->len > sizeof(out_msg->pdu)) {
			out_msg->len = (uint8_t) sizeof(out_msg->pdu);
		}
		memcpy(out_msg->pdu, buffer.usb_ni_message.pdu, out_msg->len);
		return LonStatusNoError;
	}

	// Stage 4: no message available; read LON interface UID if waiting
	// and the uplink timer expired
	if (state->wait_for_uid 
			&& ((OsalGetTickCount() - state->uplink_msg_timer) > UID_WAIT_TIME)) {
		// UID wait timer expired; send another LON interface UID read request
		code = RequestNiUid(iface_index);
		if (code != LonStatusNoError) {
			return code;
		} else {
			// New request for UID sent; no message available
			return LonStatusNoBufferAvailable;
		}
	}
	// No uplink message available
	return LonStatusNoBufferAvailable;
}

/* 
 * Processes uplink bytes received from the LON USB interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   chunk: pointer to buffer of received bytes
 *   chunk_size: number of bytes in chunk
 * Returns:
 *   None
 * Notes:
 *   Called by ReadLonUsbMsg() after reading available bytes from the
 *   LON USB interface into a temporary buffer.  Calls CheckUplinkCompleted()
 *   to check for completed uplink messages.
 * 
 *   Code packets contain the following, with byte-stuffing of FRAME_SYNC (0x7E)
 *   bytes in the message data:
 *   - Byte 0: FRAME_SYNC (0x7E)
 *   - Byte 1: 0x00 for MIP/U61 (U10 FT Rev A/B, U20 PL, U60 TP-1250, U70 PL)
 * 			   <SSSA:CCCC> for MIP/U50 (U10 FT Rev C, U60 FT) where:
 * 			   -- SSS = 3-bit sequence number; 0xE0 mask; increments per message;
 * 				  used only for messages, all other set to 0
 * 			   -- A = 1-bit ACK/NAK flag; 0x10 mask; set to 1 for ACK, 0 for NAK
 * 			   -- CCCC = 4-bit frame code; 0x0F mask; see U50_PACKET_CODE enumeration
 *   - Byte 2: For MIP/U50 only: code packet parameter, for example:
 * 			   -- 0: for CpNull (0), CpNiResync (7) (see U50LinkStart)
 * 			   -- 1: for CpMsg (2)
 * 			   -- <command>: for CpNiCmdShort (6); nicbRESET (0x50) (see smpDlNiCmdLocal)
 *   - Bytes 3: For MIP/U50 only: checksum byte (negated sum of all bytes modulo 256)
 *   - Bytes 4+: PDU data bytes (if any)
 * 
 *   For MIP/U50: 
 *   - Send CpNiCmdShort/nicbMODE_L2 (0xD1), CpNiCmdShort/nicbSSTATUS (0xE0),
 *     and CpNiCmdShort/0 commands to set layer 2 mode
 *   - Send nicbRESET (0x50) command to reset the NIC; create a ResetNi()
 *     function to reset the network interface when the NI no longer accepts
 *     outgoing messages, for example after a timeout waiting for an ACK; process incoming
 *   - Send CpMsg (2) code packets for downlink messages
 *   - Send CpNiResync (7) followed by a 0 parameter to resync
 *   - Send CpNiCmdShort/nicbSSTATUS (0xE0) to get MIP version and layer mode
 *   - Respond to nicbSSTATUS (0xE0), nicbMODE_L5 (0xD0), and nicbMODE_L2 (0xD1)
 *     commands with the same command
 *   - Respond to nicbMODE_SSI (0x42) command with nicbMODE (0x40) command
 *   - Respond to nicbINITIATE (0x51) command with nicbCHALLENGE (0x52) command
 *   - Ignore nicbACK (0xC0) and nicbNACK (0xC1) commands
 *   - Use nicbERROR (0x30) to process runt packets
 * 
 * TBD:
 *   - See smpCodePacketRxd() in U50Link.c; reviewed through line 900
 *   - Add new uplink_state values for U50_LINK.
 *   - Update ProcessUplinkBytes() and CheckUplinkCompleted() to handle
 *     U50_LINK parsing.
 *   - Add line discipline mapping for the U50_LINK and U61_LINK types.
 */
static LonStatusCode ProcessUplinkBytes(int iface_index, uint8_t *chunk, size_t chunk_size)
{
	bool completed = false;
	LonStatusCode code = LonStatusNoError;
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	while (chunk_size && !state->shutdown && code == LonStatusNoError) {
		switch(state->uplink_state) {
		case UPLINK_IDLE:
			// Looking for frame sync byte
			if (*chunk == FRAME_SYNC) {
				state->uplink_state = UPLINK_FRAME_CODE;
				state->uplink_frame_error = FALSE;
			} else if (!state->uplink_frame_error) {
				// Frame sync expected but not received; increment error count
				// and stay in UPLINK_IDLE to continue looking for frame sync
				Increment32(state->lon_stats.uplink.rx_frame_errors);
				OsalPrintError(LonStatusFrameError, "Uplink frame error (1) - %02X", *chunk);
				state->uplink_frame_error = TRUE; // TBD: check this
			}
			chunk++; chunk_size--;
			break;
		case UPLINK_FRAME_CODE:
			if (state->lon_usb_iface_model == LON_USB_INTERFACE_U50) {
				// MIP/U50: save frame code byte fields
				state->uplink_buffer.frame_header.frame_code.ack = *chunk & 0x10 ? 1 : 0;
				state->uplink_buffer.frame_header.frame_code.sequence_num = (*chunk >> 5) & 0x07;
				state->uplink_buffer.frame_header.frame_code.frame_cmd = *chunk & 0x0F;
				state->uplink_state = UPLINK_FRAME_PARAMETER;
			} else {
				// MIP/U61: frame code byte is always 0
				if (*chunk == 0) {
					chunk++; chunk_size--;
					state->uplink_msg_timer = OsalGetTickCount();
					state->uplink_state = UPLINK_MESSAGE;
				} else {
					Increment32(state->lon_stats.uplink.rx_frame_errors);
					OsalPrintError(LonStatusFrameError,
							"Uplink frame error (2) - %02X", *chunk);
					state->uplink_state = UPLINK_IDLE;
					state->uplink_frame_error = TRUE;
				}
			}
			break;
		case UPLINK_FRAME_PARAMETER:
			// For MIP/U50 only: save frame parameter byte
			state->uplink_buffer.frame_header.parameter = *chunk;
			chunk++; chunk_size--;
			state->uplink_state = UPLINK_CODE_PACKET_CHECKSUM;
			break;
		case UPLINK_CODE_PACKET_CHECKSUM:
			// For MIP/U50 only: the checksum byte is the final byte of a code packet;
			// save frame checksum and process the code packet
			state->uplink_buffer.frame_header.checksum = *chunk;
			if (!ValidateChecksum(iface_index, (uint8_t *) &state->uplink_buffer.frame_header,
					sizeof(state->uplink_buffer.frame_header))) {
				// Code packet checksum is invalid; flag frame error and go back to idle
				state->uplink_frame_error = TRUE;
				state->uplink_state = UPLINK_IDLE;
			} else {
				// Process the code packet
				code = ProcessUplinkCodePacket(iface_index);
				if (code != LonStatusNoError) {
					return code;
				}
				if (state->uplink_buffer.frame_header.frame_code.frame_cmd == MSG_FRAME_CMD) {
					// Message frame; receive uplink message
					state->uplink_state = UPLINK_MESSAGE;
				} else {
					// Not a message frame; go back to idle
					state->uplink_state = UPLINK_IDLE;
				}
			}
			chunk++; chunk_size--;
			break;
		case UPLINK_MESSAGE:
			while (chunk_size) {
				uint8_t msg_byte = state->uplink_msg[state->uplink_msg_index++] = *chunk++;
				chunk_size--;
				if (msg_byte == FRAME_SYNC) {
					state->uplink_state = UPLINK_ESCAPED_DATA;
					break;
				}
				code = CheckUplinkCompleted(iface_index, &completed);
				if (code != LonStatusNoError) {
					return code;
				}
				if (completed) {
						break;
				}
			}
			if (state->uplink_state != UPLINK_IDLE
					&& (OsalGetTickCount() - state->uplink_msg_timer) > state->usb_params.llp_timeout) {
				Increment32(state->lon_stats.uplink.rx_timeout_errors);
				OsalPrintDebug(LonStatusFrameTimeout, "Uplink frame timeout");
				code = ResetUplinkState(iface_index);
			}
			break;
		case UPLINK_ESCAPED_DATA:
			state->uplink_state = UPLINK_MESSAGE;
			if (*chunk != FRAME_SYNC) {
				Increment32(state->lon_stats.uplink.rx_frame_errors);
				OsalPrintError(LonStatusFrameError, "Uplink frame error (data)");
				if (*chunk == 0) {
					state->uplink_msg_index = 0;
				} else {
					ResetUplinkState(state->iface_index);
				}
			}
			chunk++; chunk_size--;
			code = CheckUplinkCompleted(iface_index, &completed);
			break;
		}
	}
	return code;
}

static LonStatusCode ProcessUplinkCodePacket(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	PrintUplinkCodePacket(iface_index);
	LonFrameHeader *frame_header = (LonFrameHeader *)&state->uplink_buffer.frame_header;
	LonUsbFrameCommand frame_cmd = frame_header->frame_code.frame_cmd;
	uint8_t parameter = frame_header->parameter;
	bool ack_flag = frame_header->frame_code.ack != 0;
	uint8_t sequence_number = frame_header->frame_code.sequence_num;
	bool cp_fail_or_reject = ((frame_cmd == FAIL_FRAME_CMD) || (frame_cmd == MSG_REJECT_FRAME_CMD));
	if (frame_cmd) {
		if (state->uplink_seq_number == sequence_number && !cp_fail_or_reject) {
			// Duplicate sequence number; process it partially to receive and
			// dump any following message into the uplink buffer without queuing it
			state->uplink_duplicate = true;
			Increment32(state->lon_stats.uplink.rx_duplicates);
		}
		state->uplink_seq_number = sequence_number;
	}
	if(ack_flag && !cp_fail_or_reject && (state->downlink_state != DOWNLINK_CP_RESPONSE_WAIT)) {
			// Process ACK flag for good code packets that are not local request/response commands
			ClearUplinkTransaction(iface_index);
	}
	switch(frame_cmd) {
	case MSG_FRAME_CMD:
		// Message frame header received; change state to receive message in CheckUplinkCompleted()
		OsalPrintTrace(code, "Received uplink message frame with sequence #%02X", sequence_number);
		state->uplink_state = UPLINK_MESSAGE;
		break;
	case SHORT_NI_CMD_FRAME_CMD:
		// Short NI command received; process the command
		OsalPrintTrace(code, "Received uplink short NI command 0x%02X", parameter);
		state->downlink_ack_required = true;
		if (!state->uplink_duplicate) {
			if (parameter == NI_ACK || parameter == NI_NACK) {
				// Downlink ACK not required for these commands
				break;
			}
			if(parameter == NI_RESET_DEV_CMD) {
				// Reset transmit state and force a pending downlink flush
				state->downlink_seq_number = 0;
				state->downlink_state = DOWNLINK_START;
			}
			// Translate short NI command to a local NI command and queue it uplink
			state->uplink_buffer.usb_ni_message.cmd = parameter;
	        if((parameter & 0xF0) == NI_ERROR) {
				state->uplink_buffer.usb_ni_message.len = 5;
				// Zero payload fields for an error message
				memset(state->uplink_buffer.usb_ni_message.pdu, 0, 4);
			} else {
      			state->uplink_buffer.usb_ni_message.len = 1;
			}
			code = QueueUplinkMessage(iface_index, &state->uplink_buffer.usb_ni_message);
			if (code != LonStatusNoError) {
				OsalPrintError(code, "Failed to queue uplink short NI command 0x%02X", parameter);
				return code;
			}
		}
		// Clear uplink state
		ResetUplinkState(iface_index);
		break;
	case MSG_REJECT_FRAME_CMD:
		// No room downlink; start or check a reject timer
		Increment32(state->lon_stats.downlink.tx_rejects);
		if(state->downlink_state == DOWNLINK_MSG_ACK_WAIT || state->downlink_state == DOWNLINK_CP_ACK_WAIT) {
			// Stop the ACK timer and set downlink state to idle
			StopAckTimer(iface_index);
			// This timer is stopped when an ACK is received
			if (!state->downlink_reject_timer) {
				state->downlink_reject_timer = OsalGetTickCount();
			} else if (OsalGetTickCount() - state->downlink_reject_timer > DOWNLINK_WAIT_TIME) {
				state->downlink_reject_timer = 0;
				// Downlink reject timer limit reached; reset the network interface
				code = LonStatusLniWriteFailure;
				OsalPrintError(code, "Downlink reject timer expired; resetting network interface %d", iface_index);
				WriteDownlinkCodePacket(iface_index, SHORT_NI_CMD_FRAME_CMD, NI_RESET_DEV_CMD);
			}
			// TBD: ensure sequence number is cycled for DOWNLINK_MSG_ACK_WAIT
			// because the code packet was accepted
		} else {
			// This isn't necessarily an error
			OsalPrintTrace(code, "Unknown CP or message reject cause for interface %d in state %d.", iface_index, state->downlink_state);
		}
		break;
	case FAIL_FRAME_CMD:
		ClearUplinkTransaction(iface_index); //Don't keep repeating a failed message
		StopAckTimer(iface_index);
		Increment32(state->lon_stats.uplink.rx_code_packet_failures);
		code = LonStatusFrameError;
		OsalPrintError(code, "Received uplink failure frame with parameter 0x%02X",
				state->uplink_buffer.frame_header.parameter);
		break;
	case NULL_FRAME_CMD:
		break;
	default:
		// Unknown frame command; log and increment error count
		Increment32(state->lon_stats.uplink.rx_frame_errors);
		code = LonStatusFrameError;
		OsalPrintError(code, "Received uplink frame with unknown command 0x%02X",
				frame_cmd);
		break;
	}
	return code;
}

// TBD: verify message processing after ProcessUplinkCodePacket()

/*
 * Clears any pending uplink transaction for a specified LON USB network interface.
 * Parameters:
 *   iface_index: LON interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Called to complete or abort any pending uplink transaction before starting
 *   a new uplink transaction.
 */
static LonStatusCode ClearUplinkTransaction(int iface_index)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode code = LonStatusNoError;
	if (state->downlink_state == DOWNLINK_CP_ACK_WAIT || state->downlink_state == DOWNLINK_CP_RESPONSE_WAIT) {
		state->downlink_buffer.usb_ni_message.cmd = 0;
		code = StopAckTimer(iface_index);
	} else if (state->downlink_state == DOWNLINK_MSG_ACK_WAIT) {
		code = ResetDownlinkState(iface_index);
	}
	if (code != LonStatusNoError) {
		return code;
	}
	code = IncrementDownlinkSequence(iface_index);
	state->uplink_expected_rsp = NI_CLEAR;
	state->uplink_ack_timeout_phase = 0;
	return code;
}

/*
 * Checks for completed uplink (read) messages from the LON USB
 * network interface.
 * Parameters:
 *   state: pointer to LonUsbLinkState structure for the interface
 *   completed: pointer to BOOL set to TRUE if an uplink message
 * 		   is complete and has been processed; FALSE if not yet complete
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Called by ReadLonUsbMsg() during incoming byte stream parsing. 
 *   If an uplink message is complete, it is processed and queued for
 *   the LON stack or application using QueueUplinkMessage().  If an
 *   uplink message is not yet complete, the function returns FALSE.
 *   The message length is determined from the first one or three bytes
 *   of the message, depending on whether the message is an expanded or
 *   non-expanded message.  The length is in the first byte
 *   of a non-expanded uplink message as follows:
 *     <[7E][00]>[LEN][CMD][...]
 *   and is in the second and third bytes of an expanded uplink
 *   message as follows:
 *     <[7E][00]>[FF][LHI][LLO][CMD][...]
 */
static LonStatusCode CheckUplinkCompleted(int iface_index, bool *completed)
{
	*completed = false;
	LonStatusCode code = LonStatusNoError;
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
	if (state->uplink_msg_index == 1) {		// Length
		state->uplink_msg_length = state->uplink_msg[0];
	} else if (state->uplink_msg_length == EXT_LENGTH && state->uplink_msg_index == 3){
		// Get the extended length; state->uplink_msg[0] is still EXT_LENGTH
		state->uplink_msg_length = ((state->uplink_msg[1] << 8) | state->uplink_msg[2]);
	} else if (state->uplink_msg_index && (state->uplink_msg_index > state->uplink_msg_length)) {
		uint8_t cmd;
		L2Frame *msg = (L2Frame *)state->uplink_msg;
		if (state->uplink_msg[0] == EXT_LENGTH){
			// Re-arrange the first 4 bytes; endian swap the length
			// Output: [CMD][FF][LHI][LLO][...]
			cmd = state->uplink_msg[3];
			state->uplink_msg[1] = EXT_LENGTH;
			state->uplink_msg_length--;		// Decrement for cmd
			state->uplink_msg[2] = (uint8_t)(state->uplink_msg_length & 0xFF);
			state->uplink_msg[3] = (uint8_t)(state->uplink_msg_length >> 8);
			state->uplink_msg[0] = cmd;
		} else {
			// Re-arrange the first two bytes (length & cmd)
			cmd = state->uplink_msg[1];
			state->uplink_msg_length--;		// Decrement for cmd
			state->uplink_msg[1] = (uint8_t)state->uplink_msg_length;
			state->uplink_msg[0] = cmd;
		}
		// Filter any NI_DRIVER_CMD commands
		if ((cmd & NI_CMD_MASK) == NI_DRIVER_CMD || cmd == NI_LAYER_MODE_CMD) {
			OsalPrintTrace(LonStatusNoError,
					"Processed 0x%02X uplink network interface command", cmd);
			if (cmd == NI_LAYER_MODE_CMD) {
				OsalPrintDebug(LonStatusNoError, "NI Layer Mode setting received: %d",
						msg->pdu[0]);
			}
		} else {
			OsalPrintTrace(LonStatusNoError, 
					"Received 0x%02X network interface command with %d bytes",
					cmd, state->uplink_msg_length);
			PrintMessage(state->uplink_msg, state->uplink_msg_length);

			// Process and shrink any NI_RESET_DEV_CMD data
			if (cmd == NI_RESET_DEV_CMD) {
				if (msg->len) {
					state->lon_stats.tx_id = msg->pdu[1];
					state->lon_stats.l2_l5_mode = msg->pdu[0];
					OsalPrintDebug(LonStatusNoError, 
						"NI Reset received, TXID: %d, MODE: %d",
						state->lon_stats.tx_id, state->lon_stats.l2_l5_mode);
				} else {
					OsalPrintDebug(LonStatusNoError, "NI Reset received, no data");
				}
				msg->len = 0;
			} else if (cmd == NI_CRC_ERROR) {
				Increment32(state->lon_stats.uplink.rx_crc_errors);
            } else if (cmd == NI_INCOMING_CMD && (msg->len > (3+11)) && (msg->pdu[MSG_CODE_OFFSET] == IzotNmWink)) {
                // Process any uplink network Wink messages
				IdentifyLonNi(state->iface_index);
			}
			if (cmd == NI_FLUSH_COMPLETE) {
				OsalPrintTrace(LonStatusNoError, "NI Flush Complete message received");
			}
            // Trashed uplink data can result in 0xFF error value in the len field
            if (((unsigned int)state->uplink_msg_length + 2) > (sizeof(L2Frame) + 2)) {
                OsalPrintDebug(LonStatusNoError,
						"Uplink message had invalid length: %d",
						state->uplink_msg_length);
            } else {
                // Queue it
                code = QueueUplinkMessage(iface_index, msg);
				if (code != LonStatusNoError) {
					return code;
				}
            }
		}
		code = ResetUplinkState(iface_index);
		if (code != LonStatusNoError) {
			return code;
		}
		*completed = true;
	}
	return code;
}

/*
 * Queues an uplink (read) message from the LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   msg: Pointer to L2Frame structure containing uplink message
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *  Called by CheckUplinkCompleted() which is called via ReadLonUsbMsg()
 *  parsing.  If the have_uid flag is set and the message is a read unique ID
 *  memory response, the unique ID is copied to the LON interface state.
 *  If the wait_for_uid flag is set and the message is not a read unique ID
 *  memory response, the message is dropped. Otherwise the message is queued 
 *  for upstream processing.
 */
static LonStatusCode QueueUplinkMessage(int iface_index, L2Frame *msg)
{
	LonStatusCode code = LonStatusNoError;
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL || state->shutdown) {
		return LonStatusInvalidInterfaceId;
	}
    size_t copy_length = TotalMessageLength(msg) + 1;
    if (state->wait_for_uid) {
        uint8_t *buf = (uint8_t *)msg;
		if (copy_length >= 23 && buf[0] == 0x16 && buf[16] == 0x2d) { 
			// Shortcut parsing for 23 byte read memory response
			// to read Unique ID memory (0x2d = 45 decimal)
			int i;
			for (i=0; i < 6; i++) {
				state->uid[i] = buf[i+17];
			}
			state->wait_for_uid = FALSE;
            state->have_uid = TRUE;
			OsalPrintDebug(LonStatusNoError, 
					"LON interface %d unique ID %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
					state->iface_index,
                	state->uid[0], state->uid[1], state->uid[2],
                	state->uid[3], state->uid[4], state->uid[5]);
			code = SetNiLayerMode(iface_index);
		}
    } else {
		Increment32(state->lon_stats.uplink.packets_received);
		Add32(state->lon_stats.uplink.bytes_received, copy_length);
        code = QueueUplinkBuffer(iface_index, (LonUsbQueueBuffer *)msg);
		OsalPrintTrace(code, "QueueUplinkMessage: Queued uplink message with %d bytes", copy_length);
		PrintMessage((uint8_t *)msg, copy_length);
    }
	return code;
}

/*
 * Returns the total length of a LON USB network interface message,
 * including length fields.
 * Parameters:
 *   msg: Pointer to L2Frame or LdvExtendedMessage structure
 * Returns:
 *   Total message length including length fields
 */
static int TotalMessageLength(L2Frame *msg)
{
	if (!msg) {
		return 0;
	} else if (msg->len != EXT_LENGTH) {
		return msg->len + 1;
	} else {
		LdvExtendedMessage *ext_msg = (LdvExtendedMessage *)msg;
		return EndianSwap16(ext_msg->ext_length) + 1 + 2;
	}
}

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
size_t LonUsbFeedRx(int iface_index, const uint8_t *data, size_t len) 
{
	if (!data || len == 0) {
		return 0;
	}
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (!state || state->shutdown) {
		return 0;
	}
	// Feed thread may run concurrently: lock around ring write and stats
	// Protect emptiness check; head pointer may be updated concurrently
	OsalLockMutex(&state->queue_lock);
	size_t written = RingBufferWrite(&state->lon_usb_uplink_ring_buffer, data, len);
	size_t occ = RingBufferSize(&state->lon_usb_uplink_ring_buffer);
	state->lon_stats.usb_rx.bytes_fed += written;
	if (occ > state->lon_stats.usb_rx.max_occupancy) {
		state->lon_stats.usb_rx.max_occupancy = occ;
	}
	if (written < len) {
		state->lon_stats.usb_rx.bytes_dropped += (len - written);
	}
	OsalUnlockMutex(&state->queue_lock);
	return written;
}

/*
 * Closes a LON USB network interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
LonStatusCode CloseLonUsbLink(int iface_index)
{
	LonStatusCode code = LonStatusCloseFailed;
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		code = LonStatusCloseFailed;
		OsalPrintDebug(code, "Error closing interface index %d", iface_index);
	} else {
		HalCloseUsb(state->usb_fd);
		ResetUplinkState(iface_index);
		state->shutdown = TRUE;
		state->assigned = FALSE;
		code = LonStatusNoError;
		OsalPrintDebug(code, "Close interface index %d", iface_index);
	}
	return code;
}

/*****************************************************************
 * Section: State Array Management Function Definitions
 *****************************************************************/
/* 
 * Initializes all iface_state entries to default values.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   Called once at first OpenLonUsbLink() to initialize all states.
 *   Calls ResetUplinkState() for each state.
 */
static LonStatusCode InitIfaceStates(void) 
{
    LonStatusCode code = LonStatusNoError;
    for (int iface_index = 0; iface_index < MAX_IFACE_STATES && code == LonStatusNoError; iface_index++) {
		LonUsbLinkState *state = &iface_state[iface_index];
		OsalInitMutex(&state->queue_lock);
		state->iface_index = iface_index;
		state->assigned = false;					// Not yet assigned; set to true in AssignIfaceState()
		state->shutdown = true; 					// Not yet started; set to false in OpenLonUsbLink()
		state->wait_for_uid = true;
		state->have_uid = false;
		state->lon_dev_name[0] = '\0';
		state->usb_dev_name[0] = '\0';
		state->usb_fd = -1;
		state->usb_params = default_lon_usb_config;
		memset(&state->uid, 0, sizeof(state->uid));
		state->iface_mode = LON_USB_OPEN_LAYER2;	// Default to Layer 2 mode
		state->start_time = state->last_timeout = 0;

		// Initialize downlink state
		state->downlink_state = DOWNLINK_IDLE;
		state->downlink_reject_timer = 0;
		// state->downlink_msg_index = 0;
		// state->downlink_msg_length = 0;
		state->downlink_ack_required = false;
		state->downlink_seq_number = 0;
		memset(&state->downlink_buffer, 0, sizeof(state->downlink_buffer));
		memset(&state->downlink_cp_requested, 0, sizeof(state->downlink_cp_requested));
		code = ResetDownlinkState(iface_index);
		if (code != LonStatusNoError) {
			OsalPrintError(code, "Failed to reset link state for index %d", iface_index);
			return code;
		}

		// Initialize uplink state
		state->uplink_state = UPLINK_IDLE;
		state->uplink_frame_error = false;
		state->uplink_duplicate = false;
		state->uplink_ack_timer = 0;
		state->uplink_ack_timeouts = 0;
		state->uplink_ack_timeout_phase = 0;
		state->uplink_seq_number = ~FRAME_CODE_SEQ_NUM_MASK; // Force mismatch on first frame
		code = ResetUplinkState(iface_index);
		if (code != LonStatusNoError) {
			OsalPrintError(code, "Failed to reset link state for index %d", iface_index);
			return code;
		}
		// Increase ack wait time set in ResetUplinkState() for startup only,
		// overwriting the timeout set in ResetUplinkState(); this allows more
		// time for the LON interface to reset after the first startup command
		state->uplink_ack_timeout = STARTUP_ACK_WAIT_TIME;

	    // Allocate and initialize LON USB downlink queues
		if (((code = QueueInit(&state->lon_usb_downlink_normal_queue, sizeof(LonUsbQueueBuffer), MAX_LON_DOWNLINK_BUFFERS)) != LonStatusNoError)
				|| ((code = QueueInit(&state->lon_usb_downlink_priority_queue, sizeof(LonUsbQueueBuffer), MAX_LON_DOWNLINK_BUFFERS)) != LonStatusNoError)) {
			OsalPrintError(LonStatusStackInitializationFailure, "InitIfaceStates: Failed to initialize LON USB downlink queue");
			return code;
		}

	    // Allocate and initialize LON USB uplink queues
		if (((code = QueueInit(&state->lon_usb_uplink_normal_queue, sizeof(LonUsbQueueBuffer), MAX_LON_UPLINK_BUFFERS)) != LonStatusNoError)
				|| ((code = QueueInit(&state->lon_usb_uplink_priority_queue, sizeof(LonUsbQueueBuffer), MAX_LON_UPLINK_BUFFERS)) != LonStatusNoError)) {
			OsalPrintError(LonStatusStackInitializationFailure, "InitIfaceStates: Failed to initialize LON USB uplink queue");
			return code;
		}
		
		// Initialize the LON USB uplink ring buffer (capacity set to full internal max)
		code = RingBufferInit(&state->lon_usb_uplink_ring_buffer, RING_BUFFER_MAX_CAPACITY);
		if (code != LonStatusNoError) {
			OsalPrintError(code, "InitIfaceStates: Failed to initialize LON USB uplink ring buffer");
			return code;
		}
		RingBufferClear(&state->lon_usb_uplink_ring_buffer);
		state->lon_stats.usb_rx.capacity = RING_BUFFER_MAX_CAPACITY;
		state->lon_stats.usb_rx.max_occupancy = RingBufferSize(&state->lon_usb_uplink_ring_buffer);

		// Initialize LON USB link statistics
		memset(&state->lon_stats, 0, sizeof(state->lon_stats));
		state->lon_stats.size = sizeof(state->lon_stats);
    }
	return code;
}

/*
 * Assigns a free interface state.
 * Parameters:
 *   None
 * Returns:
 *   Interface state index if one is available or -1 if an interface
 *   state index is not available
 */
static int AssignIfaceState(void)
{
    for (int i = 0; i < MAX_IFACE_STATES; i++) {
        if (!iface_state[i].assigned) {
			ResetUplinkState(i);
            iface_state[i].assigned = TRUE;
            return i;  // Return index of assigned interface state
        }
    }
    return -1; // No free slot found
}

/*
 * Gets a pointer to an interface state by index.
 * Parameters:
 *   index: Interface state index
 * Returns:
 *   Pointer to LonUsbLinkState if valid and assigned; NULL otherwise
 * Notes:
 *   Used to access interface state data.
 */
static LonUsbLinkState* GetIfaceState(int index)
{
    if (index < 0 || index >= MAX_IFACE_STATES) {
        return NULL; // Invalid index
    }
    if (!iface_state[index].assigned) {
		OsalPrintDebug(LonStatusNoError, "GetIfaceState: index %d not assigned", index);
        return NULL; // Not currently assigned
    }
    return &iface_state[index];
}

/*
 * Counts the number of free (unassigned) slots.
 * Parameters:
 *   None
 * Returns:
 *   Number of free (unassigned) slots
 */
static int CountFreeIfaceStates(void) {
    int count = 0;
    for (int i = 0; i < MAX_IFACE_STATES; i++) {
        if (!iface_state[i].assigned) {
            count++;
        }
    }
    return count;
}

/*****************************************************************
 * Section: Downlink Queue Management Function Definitions
 *****************************************************************/
/*
 * Downlink queue helpers (singly linked with head pointer)
 * ----------------------------------------------------------
 * Head:   LonUsbLinkState.downlink_queue points to first node, or NULL if empty.
 * Nodes:  Each node embeds a LonUsbQueueBuffer in entry->data.
 * Ops:
 *  - QueueDownlinkBuffer: copy src into a new node; if head NULL set head, else append.
 *  - ReadDownlinkBuffer: pop head (advance pointer), copy into dst, free node.
 *  - GetDownlinkBufferCount: O(n) walk via next pointers.
 */

/*
 * Removes and returns the front buffer from a downlink queue.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   dst: pointer to LonUsbQueueBuffer to receive the data
 * Returns:
 *   LonUsbQueueBuffer in dst if available; NULL if no buffer is available
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode ReadDownlinkBuffer(int iface_index, LonUsbQueueBuffer *dst) {
    if (!dst) {
        return LonStatusInvalidParameter;
    }
    LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode status = LonStatusNoError;
	OsalLockMutex(&state->queue_lock);
	// Get source queue: priority if not empty, else normal
	Queue *source_queue = &state->lon_usb_downlink_priority_queue;
	if (QueueEmpty(source_queue)) {
		source_queue = &state->lon_usb_downlink_normal_queue;
	}
	// Get head of source queue, if any
	LonUsbQueueBuffer* buffer = QueuePeek(source_queue);
    if (!buffer) {
		status = LonStatusNoBufferAvailable;
    } else {
    	// Copy the queue entry to dst
    	memcpy(dst, buffer, sizeof(LonUsbQueueBuffer));
    	// Advance source queue head past the first entry
		QueueDropHead(source_queue);
	}
	OsalUnlockMutex(&state->queue_lock);
    return status;
}

/*
 * Flushes all downlink buffers from the specified queues.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   priority: MessagePriorityLevel specifying which queues to flush
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode FlushDownlinkQueue(int iface_index, MessagePriorityLevel priority)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	OsalLockMutex(&state->queue_lock);
	if ((priority == HIGH_PRIORITY) || (priority == ALL_PRIORITIES)) {
		QueueFlush(&state->lon_usb_downlink_priority_queue);
	}
	if ((priority == NORMAL_PRIORITY) || (priority == ALL_PRIORITIES)) {
		QueueFlush(&state->lon_usb_downlink_normal_queue);
	}
	OsalUnlockMutex(&state->queue_lock);
	return LonStatusNoError;
}

/*
 * Appends a copy of src to the end of the downlink queue.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   src: pointer to LonUsbQueueBuffer containing the data to append
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode QueueDownlinkBuffer(int iface_index, LonUsbQueueBuffer *src) {
    LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	if (!src) {
		OsalPrintError(LonStatusInvalidParameter, "QueueDownlinkBuffer: NULL src for interface %d", iface_index);
		return LonStatusInvalidParameter;
	}
	Queue *queue;
	LonUsbQueueBuffer *tail;
	LonStatusCode status = LonStatusNoError;
	OsalLockMutex(&state->queue_lock);
	if (src->priority == HIGH_PRIORITY) {
		queue = &state->lon_usb_downlink_priority_queue;
	} else {
		queue = &state->lon_usb_downlink_normal_queue;
	}
	tail = QueueTail(queue);
	if (!tail) {
		// Invalid queue
		status = LonStatusInvalidParameter;
		OsalPrintError(status, "QueueDownlinkBuffer: Invalid downlink queue for interface %d", iface_index);
	} else {
		// Append the new entry to the queue
		memcpy(tail, src, sizeof(LonUsbQueueBuffer));
		QueueWrite(queue);
	}
	OsalUnlockMutex(&state->queue_lock);
	return status;
}

/*
 * Gets the number of downlink buffers currently queued.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   Number of buffers in the downlink queues; 0 if interface is invalid
 */
static size_t GetDownlinkBufferCount(int iface_index) {
    LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
        return 0;
	}
	OsalLockMutex(&state->queue_lock);
	size_t cnt = QueueSize(&state->lon_usb_downlink_priority_queue) + 
				 QueueSize(&state->lon_usb_downlink_normal_queue);
	OsalUnlockMutex(&state->queue_lock);
	return cnt;
}

/*
 * Returns TRUE if the downlink queues are empty for the given interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   TRUE if the downlink queues are empty or the interface is invalid
 */
static bool IsDownlinkQueueEmpty(int iface_index) {
    LonUsbLinkState *state = GetIfaceState(iface_index);
    if (state == NULL) {
        return true;
    }
	OsalLockMutex(&state->queue_lock);
	bool empty = QueueEmpty(&state->lon_usb_downlink_priority_queue)
			&& QueueEmpty(&state->lon_usb_downlink_normal_queue);
	OsalUnlockMutex(&state->queue_lock);
	return empty;
}


/*****************************************************************
 * Section: Uplink Queue Management Function Definitions
 *****************************************************************/
/*
 * Uplink queue helpers (singly linked with head pointer)
 * -----------------------------------------------------
 * Head:   LonUsbLinkState.uplink_queue points to first node, or NULL if empty.
 * Nodes:  Each node embeds a LonUsbQueueBuffer in entry->data.
 * Ops:
 *  - QueueUplinkBuffer: copy src into a new node; if head NULL set head, else append.
 *  - ReadUplinkBuffer: pop head (advance pointer), copy into dst, free node.
 *  - GetUplinkBufferCount: O(n) walk via next pointers.
 */

/*
 * Removes and returns the front buffer from an uplink queue.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   dst: pointer to LonUsbQueueBuffer to receive the data
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode ReadUplinkBuffer(int iface_index, LonUsbQueueBuffer *dst) 
{
    if (!dst) {
        return LonStatusInvalidParameter;
    }
    LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	LonStatusCode status = LonStatusNoError;
	OsalLockMutex(&state->queue_lock);
	// Get source queue: priority if not empty, else normal
	Queue *source_queue = &state->lon_usb_uplink_priority_queue;
	if (QueueEmpty(source_queue)) {
		source_queue = &state->lon_usb_uplink_normal_queue;
	}
	// Get head of source queue, if any
	LonUsbQueueBuffer* buffer = QueuePeek(source_queue);
    if (!buffer) {
		status = LonStatusNoBufferAvailable;
    } else {
		// Copy the queue entry to dst
		memcpy(dst, buffer, sizeof(LonUsbQueueBuffer));
		// Advance source queue head past the first entry
		QueueDropHead(source_queue);
	}
	OsalUnlockMutex(&state->queue_lock);
    return status;
}

/*
 * Flushes all uplink buffers from the specified queue.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   priority: MessagePriorityLevel specifying which queues to flush
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode FlushUplinkQueue(int iface_index, MessagePriorityLevel priority)
{
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	OsalLockMutex(&state->queue_lock);
	if ((priority == HIGH_PRIORITY) || (priority == ALL_PRIORITIES)) {
		QueueFlush(&state->lon_usb_uplink_priority_queue);
	}
	if ((priority == NORMAL_PRIORITY) || (priority == ALL_PRIORITIES)) {
		QueueFlush(&state->lon_usb_uplink_normal_queue);
	}
	OsalUnlockMutex(&state->queue_lock);
	return LonStatusNoError;
}

/*
 * Appends a copy of src to the end of the uplink queue.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 *   src: pointer to LonUsbQueueBuffer containing the data to append
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 */
static LonStatusCode QueueUplinkBuffer(int iface_index, LonUsbQueueBuffer *src) {
    LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return LonStatusInvalidInterfaceId;
	}
	if (!src) {
		OsalPrintError(LonStatusInvalidParameter, "QueueDownlinkBuffer: NULL src for interface %d", iface_index);
		return LonStatusInvalidParameter;
	}
	Queue *queue;
	LonUsbQueueBuffer *tail;
	LonStatusCode status = LonStatusNoError;
	OsalLockMutex(&state->queue_lock);
	if (src->priority == HIGH_PRIORITY) {
		queue = &state->lon_usb_uplink_priority_queue;
	} else {
		queue = &state->lon_usb_uplink_normal_queue;
	}
	tail = QueueTail(queue);
	if (!tail) {
		// Invalid queue
		status = LonStatusInvalidParameter;
		OsalPrintError(status, "QueueDownlinkBuffer: Invalid downlink queue for interface %d", iface_index);
	} else {
		// Append the new entry to the queue
		memcpy(tail, src, sizeof(LonUsbQueueBuffer));
		QueueWrite(queue);
	}
	OsalUnlockMutex(&state->queue_lock);
    return status;
}

/*
 * Gets the number of uplink buffers currently queued.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   Number of buffers in the uplink queues; 0 if interface is invalid
 */
static size_t GetUplinkBufferCount(int iface_index) {
    LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
        return 0;
	}
	OsalLockMutex(&state->queue_lock);
	size_t cnt = QueueSize(&state->lon_usb_uplink_priority_queue) + 
				 QueueSize(&state->lon_usb_uplink_normal_queue);
	OsalUnlockMutex(&state->queue_lock);
	return cnt;
}

/*
 * Returns TRUE if the uplink queues are empty for the given interface.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   TRUE if the uplink queues are empty or the interface is invalid
 */
static bool IsUplinkQueueEmpty(int iface_index) {
    LonUsbLinkState *state = GetIfaceState(iface_index);
    if (state == NULL) {
        return true;
    }
	OsalLockMutex(&state->queue_lock);
	bool empty = QueueEmpty(&state->lon_usb_uplink_priority_queue)
			&& QueueEmpty(&state->lon_usb_uplink_normal_queue);
	OsalUnlockMutex(&state->queue_lock);
	return empty;
}

/*****************************************************************
 * Section: Debugging Support Function Definitions
 *****************************************************************/
/*
 * Prints a LON message buffer in a formatted hex dump
 * Parameters:
 *   msg: pointer to message buffer
 *   length: length of message in bytes
 * Returns:
 *   None
 * Notes:
 *   Only prints if log level is LOG_TRACE or higher.
 */
static void PrintMessage(const uint8_t *msg, size_t length)
{
	if (OsalGetLogLevel() < LOG_TRACE) {
		return;    // Fast path: tracing disabled
	}

	const char *prefix = "LON interface message: ";
	const size_t prefix_len = strlen(prefix); // Used for indentation
	const int per_line = 20;                  // Hex bytes per output line

	size_t index = 0;
	while (index < length) {
		char line[256];
		int pos = 0;

		// First line starts with the prefix; continuation lines are indented with spaces
		if (index == 0) {
			strncpy(line, prefix, sizeof(line) - 1);
			line[sizeof(line) - 1] = '\0';
			pos = (int)strlen(line);
		} else {
			// Fill indentation spaces
			size_t indent = prefix_len < (sizeof(line) - 1) ? prefix_len : (sizeof(line) - 1);
			memset(line, ' ', indent);
			pos = (int)indent;
			line[pos] = '\0';
		}

		// Append up to per_line hex bytes
		int count = 0;
		while (index < length && count < per_line) {
			if (pos >= (int)(sizeof(line) - 4)) { // Reserve space for "..\n\0"
				break; // Prevent overflow
			}
			int written = snprintf(line + pos, sizeof(line) - pos, "%02X ", msg[index]);
			if (written <= 0 || written >= (int)(sizeof(line) - pos)) {
				break; // Truncation or error
			}
			pos += written;
			index++;
			count++;
		}

		// Trim trailing space if present
		if (pos > 0 && line[pos - 1] == ' ') {
			line[--pos] = '\0';
		}
		// Add newline
		if (pos < (int)sizeof(line) - 2) {
			line[pos++] = '\n';
			line[pos] = '\0';
		} else {
			line[sizeof(line) - 2] = '\n';
			line[sizeof(line) - 1] = '\0';
		}

		// Emit this line
		OsalPrintTrace(LonStatusNoError, line);
	}
}

/*
 * Prints the contents of an uplink code packet for debugging.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   None
 * Notes:
 *   Only prints if log level is LOG_TRACE or higher.
 */
static void PrintUplinkCodePacket(int iface_index)
{
	if (OsalGetLogLevel() < LOG_TRACE) {
		return;    // Fast path: tracing disabled
	}
	LonUsbLinkState *state = GetIfaceState(iface_index);
	if (state == NULL) {
		return;
	}
	LonUsbFrameCommand frame_cmd = state->uplink_buffer.frame_header.frame_code.frame_cmd;
	unsigned int sequence_num = state->uplink_buffer.frame_header.frame_code.sequence_num;
	bool ack_flag = state->uplink_buffer.frame_header.frame_code.ack;
	uint8_t parameter = state->uplink_buffer.frame_header.parameter;
	uint8_t checksum = state->uplink_buffer.frame_header.checksum;
	OsalPrintTrace(LonStatusNoError, "Uplink code packet frame command: 0x%02X", frame_cmd);
	OsalPrintTrace(LonStatusNoError, "Uplink code packet sequence number: %u", sequence_num);
	OsalPrintTrace(LonStatusNoError, "Uplink code packet ACK flag: %s", (ack_flag) ? "SET" : "CLEAR");
	OsalPrintTrace(LonStatusNoError, "Uplink code packet parameter: 0x%02X", parameter);
	OsalPrintTrace(LonStatusNoError, "Uplink code packet checksum: 0x%02X", checksum);
}

/*****************************************************************
 * Section: Implementation-specific Function Definitions
 *****************************************************************/
/*
 * Responds to a LON network interface Identify (wink) command.
 * Parameters:
 *   iface_index: interface index returned by OpenLonUsbLink()
 * Returns:
 *   None
 * Notes:
 *   Called by CheckUplinkCompleted() when a wink command is received.
 */
static void IdentifyLonNi(int iface_index)
{
	// TBD: add an implementation-specific callback function to
	// physically identify the LON network interface
	OsalPrintDebug(LonStatusNoError, "IdentifyLonNi: LON network interface Identify (wink) command received");
	// For now, just log the event
}
