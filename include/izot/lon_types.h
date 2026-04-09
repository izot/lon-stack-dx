/*
 * lon_types.h
 *
 * Copyright (c) 2023-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack API Types
 * Purpose: Defines types for a LON stack.
 */

#ifndef _LON_TYPES_H
#define _LON_TYPES_H

#include "izot/IzotPlatform.h"
#include "izot/iap_types.h"       // IAP type definitions

// Return status codes
typedef enum {
    LonStatusNoError                        = 0,    // No error; used by the LON_SUCCESS macro

    // Original LDV codes
    LonStatusNotFound						= 1,
    LonStatusAlreadyOpen					= 2,
    LonStatusNotOpen						= 3,
    LonStatusInterfaceError					= 4,
    LonStatusInvalidInterfaceId				= 5,
    LonStatusNoMessageAvailable				= 6,
    LonStatusNoBufferAvailable				= 7,
    LonStatusNoResourcesAvailable			= 8,
    LonStatusInvalidBufferLength			= 9,
    LonStatusNotEnabled						= 10,

    // LDV codes added in OpenLDV 1.0
    LonStatusInitializationFailed			= 11,
    LonStatusOpenFailed 					= 12,
    LonStatusCloseFailed					= 13,
    LonStatusReadFailed						= 14,
    LonStatusWriteFailed					= 15,
    LonStatusRegisterFailed					= 16,
    LonStatusInvalidXDriver					= 17,
    LonStatusDebugFailed					= 18,
    LonStatusAccessDenied					= 19,

    // LDV codes added in OpenLDV 2.0
    LonStatusCapableDeviceNotFound			= 20,
    LonStatusNoMoreCapableDevices			= 21,
    LonStatusCapabilityNotSupported			= 22,
    LonStatusInvalidDriverInfo				= 23,
    LonStatusInvalidDeviceInfo				= 24,
    LonStatusDeviceInUse					= 25,
    LonStatusNotImplemented					= 26,   // Feature not implemented
    LonStatusInvalidParameter				= 27,   // Invalid parameter specified
    LonStatusInvalidDriverId				= 28,
    LonStatusInvalidDataFormat				= 29,
    LonStatusInternalError					= 30,
    LonStatusException						= 31,
    LonStatusDriverUpdateFailed				= 32,
    LonStatusDeviceUpdateFailed				= 33,
    LonStatusStdDriverTypeReadOnly			= 34,
	LonStatusFrameError					    = 35,
	LonStatusFrameTimeout				    = 36,

    // LDV codes added in OpenLDV 4.0
    LonStatusOutputBufferSizeMismatch		= 40,	// Priority and non-priority output buffer sizes must be the same
    LonStatusInvalidBufferParameter			= 41,	// Invalid buffer parameter (e.g. too large)
    LonStatusInvalidBufferCount				= 42,	// Invalid buffer count (e.g. need at least one buffer of each type)
    LonStatusPriorityBufferCountMismatch	= 43,	// If one of the priority output buffer counts is zero, then both must be zero
    LonStatusBufferSizeTooSmall				= 44,	// Buffer size is too small to support subsequent buffer configuration changes
    LonStatusBufferConfigurationTooLarge	= 45,	// Requested buffer configuration is too large to fit in available space
    LonStatusAppBufferSizeMismatchWarning	= 46,	// Application buffer input-output size mismatch may cause problems (warning only)

    // LON stack codes
	LonStatusOutOfRange						= 47,
	LonStatusTimeout						= 48,
	LonStatusNoMemoryAvailable				= 49,
	LonStatusUnderflow						= 50,
	LonStatusOverflow						= 51,
	LonStatusDataIntegrityError				= 52,
	LonStatusSecurityViolation				= 53,
	LonStatusCreateFailure					= 54,
	LonStatusRemoveFailure					= 55,
	LonStatusInvalidOperation				= 56,
	LonStatusOffline						= 57,   // Operation not supported while device is offline
	LonStatusChecksumError					= 58,	// Checksum error detected
    LonStatusTransactionInProgress			= 59,   // A transaction is already in progress
    LonStatusTransactionNotAvailable        = 60,   // No transaction available
    LonStatusTransactionFailure             = 61,   // Transaction failed
    LonStatusModeChangeFailure              = 62,   // Failed to change mode
    LonStatusNotConfiguredOnline            = 63,   // Device not configured and online

    // OSAL codes
	LonStatusCriticalSectionError			= 64,	// Generic error accessing a critical section
	LonStatusSemaphoreError					= 65,	// Generic error creating or accessing a binary semaphore
	LonStatusEventError						= 66,	// Generic error creating or accessing an event
    LonStatusTaskCreationError				= 67,	// Failed to create a task
 
	// Interoperable Self Installation (ISI) codes
	LonStatusNoISIConnectionSpace			= 68,	// No space for ISI connection
	LonStatusIsiEngineNotRunning			= 69,	// ISI engine not running

	// Datapoint-related codes
	LonStatusDpIndexInvalid 				= 70,	// Invalid datapoint index
	LonStatusDpLengthMismatch				= 71,	// Assumed and actual length not equal
	LonStatusDpLengthTooLong				= 72,   // Datapoint data is too long
	LonStatusDpPollNotPolledDatapoint		= 73,   // Polled attribute missing for polling input datapoint 
	LonStatusDpPollOutputDatapoint			= 74,   // Cannot poll output datapoint
	LonStatusInputDpPropagateFailure		= 75,   // Cannot propagate input datapoint
	LonStatusPolledDpPropagateFailure		= 76,   // Cannot propagate polled datapoint
    LonStatusOutputDpPropagateFailure		= 77,   // Cannot propagate output datapoint
    LonStatusNoSpaceInNvTable               = 78,   // No space in NV table for new datapoint
    LonStatusInvalidNvName                  = 79,   // Invalid network variable name

   	// Application message-related codes
	LonStatusDestinationAddressMissing 		= 80,   // Explicit destination required but missing
	LonStatusInvalidMessageTag				= 81,   // Invalid message tag provided
	LonStatusInvalidMessageLength			= 82,   // Invalid message size
	LonStatusInvalidMessageService			= 83,   // Invalid message service
	LonStatusInvalidMessageCode				= 84,   // Invalid message code
	LonStatusInvalidMessageCorrelator		= 85,   // Invalid message correlator
	LonStatusInvalidMessageAddress			= 86,   // Invalid message address
    LonStatusInvalidMessageCommand          = 87,   // Invalid message command
    LonStatusInvalidMessageMode             = 88,   // Invalid message mode

   	// Network interface-related codes
	LonStatusLniNotDefined					= 89,	// No Network Interface defined. See IzotGetMyNetworkInterface in IzoTHandlers.c
	LonStatusLniNotResponding				= 90,   // LON network interface not responding
	LonStatusLniUniqueIdNotAvailable		= 91,   // LON network interface unique ID not available
	LonStatusLniWriteFailure				= 92,   // Failed to write to LON network interface
	LonStatusTxBufferFull              		= 93,   // No transmit (downlink) buffer available
	LonStatusRxMessageNotAvailable			= 94,   // No message received from the LON network interface

    // LON stack-related codes
	LonStatusLlpVersionNotAvailable    		= 95,   // Link-layer protocol version information not available
	LonStatusDeviceUniqueIdNotAvailable		= 96,   // Device unique ID (Neuron or MAC ID) unavailable
	LonStatusdDeviceUriInvalid				= 97, 	// LON stack device URI is invalid
	LonStatusStackNotInitialized			= 98,   // LON stack not initialized
	LonStatusStackInitializationFailure		= 99,   // LON stack initialization failed
	LonStatusHostRebootFailure				= 100,	// LON stack host reboot failed
	LonStatusIpAddressNotDefined			= 101,	// No IP address defined
	LonStatusIndexInvalid					= 102,  // Invalid index (not a datapoint index)
	LonStatusMessageNotAvailable			= 103,	// Message not available
	LonStatusInvalidStructureVersion		= 104,  // Structure version not supported
	LonStatusCallbackNotRegistered			= 105,	// Callback function has not been registered
	LonStatusCallbackExceptionError			= 106,	// An exception when executing a callback function
	LonStatusUnknownStackDeviceType			= 107,  // Unknown LON stack device type
    LonStatusProxyFailure                   = 108,  // Proxy operation failed

	// Persistent data management codes
	LonStatusInvalidSegmentType   			= 110,  // Not a supported persistent segment type
	LonStatusPersistentDataFailure			= 111,	// Generic persistent data failure
	LonStatusIvalidPersistentDataSize		= 112,	// Persistent data size is not supported
	LonStatusPersistentDataDirError			= 113,	// Persistent data directory error
	LonStatusPersistentDataAccessError		= 114,	// Persistent data access error

	// Direct Memory File (DMF) access codes
	LonStatusDmfAddressOutOfRange			= 115,	// DMF address + count is out of range for operation
	LonStatusDmfReadOnly					= 116,  // Write to read-only DMF area
	LonStatusDmfNoDriver					= 117,	// No DMF driver defined

    // Interoperable Update Protocol (IUP) codes
    LonStatusIupNotInitialized              = 118,  // IUP not initialized
    LonStatusIupInvalidImage                = 119,  // Invalid IUP image
    LonStatusIupInvalidParameter            = 120,  // Invalid IUP parameter
    LonStatusIupTransferInProgress          = 121,  // IUP transfer already in progress
    LonStatusIupNoTransferInProgress        = 122,  // No IUP transfer in progress
    LonStatusIupInsufficientMemory          = 123,  // Insufficient memory for IUP operation
    LonStatusIupImageWriteFailure           = 124,  // Failed to write IUP image to persistent memory
    LonStatusIupInvalidState                = 125,  // Invalid IUP state for requested operation
    LonStatusIupTransferFailure             = 126,  // IUP transfer failed
    LonStatusIupConfirmationFailure         = 127,  // IUP confirmation failed

	// System and LON stack error codes logged in the system event log.
	// these can be accessed using the Query Status network management command.
	// The standard system errors range from 129 to 255. Values between 1 and 128
	// are application-specific (but serious) errors. Values used by the LON
	// stack are also included in the <LonStatusCode> enumeration.  The standard
	// system error codes are described in the ISO/IEC 14908-1 protocol standard.
	LonStatusBadEvent						= 129,
	LonStatusDatapointLengthMismatch		= 130,
    LonStatusDatapointMsgTooShort			= 131,
    LonStatusEepromWriteFail				= 132,
    LonStatusBadAddressType					= 133,
    LonStatusPreemptionModeTimeout			= 134,
    LonStatusAlreadyPreempted				= 135,
    LonStatusSyncDatapointUpdateLost		= 136,
    LonStatusInvalidRespAlloc				= 137,
    LonStatusInvalidDomain					= 138,
    LonStatusReadPastEndOfMsg				= 139,
    LonStatusWritePastEndOfMsg				= 140,
    LonStatusInvalidAddrTableIndex			= 141,
    LonStatusIncompleteMsg					= 142,
    LonStatusDatapointUpdateOnOutput		= 143,
    LonStatusNoMsgAvail						= 144,
    LonStatusIllegalSend					= 145,
    LonStatusUnknownPdu						= 146,
    LonStatusInvalidDatapointIndex			= 147,
    LonStatusDivideByZero					= 148,
    LonStatusInvalidApplError				= 149,
    LonStatusMemoryAllocFailure				= 150,
    LonStatusWritePastEndOfNetBuffer		= 151,
    LonStatusApplChecksumError				= 152,
    LonStatusCnfgChecksumError				= 153,
    LonStatusInvalidXcvrRegAddr				= 154,
    LonStatusXcvrRegTimeout					= 155,
    LonStatusWritePastEndOfApplBuffer		= 156,
    LonStatusIoReady						= 157,
    LonStatusSelfTestFailed					= 158,
    LonStatusSubnetRouter					= 159,
    LonStatusAuthenticationMismatch			= 160,
    LonStatusSeltInstSemaphoreSet			= 161,
    LonStatusReadWriteSemaphoreSet			= 162,
    LonStatusApplSignatureBad				= 163,
    LonStatusRouterFirmwareVersionMismatch	= 164,
    LonStatusInvalidBufferSize              = 166,
    LonStatusInvalidFirmwareImage           = 167,
    LonStatusInvalidGroupSize               = 168,
    LonStatusInvalidNvLength                = 169,
    LonStatusInvalidPacketLength            = 170,
    LonStatusInvalidSelector                = 171,
    LonStatusInvalidSelfDocumentation       = 172,
    LonStatusInvalidTimer                   = 173,
    LonStatusNoSelectorsAvailable           = 174,
    LonStatusNoSpaceInConfigTable           = 175,
    LonStatusResetFailed                    = 176,
    LonStatusTimerExpirationNotExpected     = 177,
    LonStatusReceiveRecordNotAvailable      = 178,

	// Add any platform and application-specific system error codes 
	// with values between 513 (0x201) and 639 (0x27F), which will be 
	// reported as values between 1 and 127 (0x7F) by the Query Status 
	// network management command
	LonStatusCustomSystemCodeStart			= 512,
	// Add custom system error codes here
	LonStatusCustomSystemCodeEnd			= 639,

	// Add any platform and application-specific error codes with
	// values between 641 and 1023
	LonStatusCustomPlatformCodeStart		= 640,
	// Add custom platform error codes here
	LonStatusCustomePlatformCodeEnd			= 1023
} LonStatusCode;

// Evaluate success or failure of an LonStatusCode value
#define LON_SUCCESS(n)   ((n) == LonStatusNoError)

// Type conversions
#define IZOT_GET_UNSIGNED_WORD(n)          (((uint16_t)((n).msb) << 8)+(uint16_t)((n).lsb))
#define IZOT_SET_UNSIGNED_WORD(n, v)       (n).msb = (IzotByte)((v)>>8); (n).lsb = (IzotByte)(v)
#define IZOT_SET_UNSIGNED_WORD_FROM_BYTES(n,b1,b2) (n).msb = (IzotByte)(b1); \
                                                   (n).lsb = (IzotByte)(b2)

#define IZOT_GET_SIGNED_WORD(n)            ((int16_t)IZOT_GET_UNSIGNED_WORD(n))
#define IZOT_SET_SIGNED_WORD(n, v)         IZOT_SET_UNSIGNED_WORD(n, v)
#define IZOT_GET_UNSIGNED_DOUBLEWORD(n)    ((((uint32_t)IZOT_GET_UNSIGNED_WORD((n).msw)) << 16) \
                                          +(uint32_t)IZOT_GET_UNSIGNED_WORD((n).lsw))
#define IZOT_SET_UNSIGNED_DOUBLEWORD(n, v) IZOT_SET_UNSIGNED_WORD((n).msw, (uint16_t) ((v) >> 16)); \
                                          IZOT_SET_UNSIGNED_WORD((n).lsw, (uint16_t) (v))
#define IZOT_GET_SIGNED_DOUBLEWORD(n)    ((int32_t)IZOT_GET_UNSIGNED_DOUBLEWORD(n))
#define IZOT_SET_SIGNED_DOUBLEWORD(n, v) IZOT_SET_UNSIGNED_DOUBLEWORD(n, v)

// Gets or sets attributes from or to a field by appropriately masking and shifting
#define IZOT_GET_ATTRIBUTE(var, n)           ((((var).n##_FIELD) & n##_MASK) >> n##_SHIFT)
#define IZOT_GET_ATTRIBUTE_P(var, n)         ((((var)->n##_FIELD) & n##_MASK) >> n##_SHIFT)
#define IZOT_SET_ATTRIBUTE(var, n, value)    ((var).n##_FIELD = (((var).n##_FIELD) & ~n##_MASK) | ((((value) << n##_SHIFT)) & n##_MASK))
#define IZOT_SET_ATTRIBUTE_P(var, n, value)  ((var)->n##_FIELD = (((var)->n##_FIELD) & ~n##_MASK) | ((((value) << n##_SHIFT)) & n##_MASK))

// Maximum length of the domain identifier, in bytes
// The domain identifier can be zero, one, three, or
// IZOT_DOMAIN_ID_MAX_LENGTH (6) bytes long.  Space for the largest possible
// identifier is allocated in various structures and message types. See
// <IzotDomain> for the domain table structure.
 #define IZOT_DOMAIN_ID_MAX_LENGTH    6

// Length of the authentication key, stored in the domain table (<IzotDomain>)
#define IZOT_AUTHENTICATION_KEY_LENGTH   6

// Length of the Open Media Authentication (OMA) key, in bytes
#define IZOT_OMA_AUTHENTICATION_KEY_LENGTH   12

// Length of the application's program identifier, in bytes
#define IZOT_PROGRAM_ID_LENGTH   8

// Length of the location identifier, in bytes
#define IZOT_LOCATION_LENGTH     6

// Length of the node's unique identifier, in bytes;
// the Unique ID is also known as the Neuron ID or MAC ID
#define IZOT_UNIQUE_ID_LENGTH    6

// Number of communication control bytes
#define IZOT_COMMUNICATIONS_PARAMETER_LENGTH 7

// Custom channel type ID; see stdxcvr.xml for all channel types
#define IZOT_CUSTOM_CHANNEL_TYPE_ID 30

// Parameters for single-ended and special-purpose mode transceivers;
// see <IzotDirectModeTransceiver> for direct-mode transceiver parameters
typedef IzotByte IzotTransceiverParameters[IZOT_COMMUNICATIONS_PARAMETER_LENGTH];

// Holds the unique ID
typedef IzotByte IzotUniqueId[IZOT_UNIQUE_ID_LENGTH];

// Holds the program ID
typedef IzotByte IzotProgramId[IZOT_PROGRAM_ID_LENGTH];

// Holds a single domain identifier
typedef IzotByte IzotDomainId[IZOT_DOMAIN_ID_MAX_LENGTH];

// Holds a single authentication key
typedef IzotByte IzotAuthenticationKey[IZOT_AUTHENTICATION_KEY_LENGTH];

// Holds a single location identifier
// The location identifier is often referred to as the "location string".
// Note that this is misleading, because this data is not limited to ASCII
// characters or other similar requirements that could be inferred from the
// word "string".
typedef IzotByte IzotLocationId[IZOT_LOCATION_LENGTH];

// Holds a subnet identifier
typedef IzotByte IzotSubnetId;

// Holds a group identifier
typedef IzotByte IzotGroupId;

// Holds a node identifier
typedef IzotByte IzotNodeId;

// LON architecture types
typedef IZOT_ENUM_BEGIN(LonArchitecture) {
    /*  0 */ LonNeuron3150Arch     =  0,  /* 3150, FT 3150 */
    /*  1 */ LonNeuronPl3150Arch   =  1,  /* PL 3150 */
    /*  2 */ LonNeuron3150LArch    =  2,  /* CY7C53150L */
    /*  8 */ LonNeuron3120Arch     =  8,  /* Legacy 3120 */
    /*  9 */ LonNeuron3120E1Arch   =  9,  /* 3120E1, TMPN3120FE1M */
    /* 10 */ LonNeuron3120E2Arch   = 10,  /* 3120E2 */
    /* 11 */ LonNeuron3120E3Arch   = 11,  /* 3120E3, TMPN3120FE3M */
    /* 12 */ LonNeuron3120A20Arch  = 12,  /* 3120A20 */
    /* 13 */ LonNeuron3120E5Arch   = 13,  /* 3120E5, TMPN3120FE5M */
    /* 14 */ LonNeuron3120E4Arch   = 14,  /* CY7C53120E4, FT 3120-E4 */
    /* 15 */ LonNeuronPl3120E4Arch = 15,  /* PL 3120-E4 */
    /* 16 */ LonNeuron3120L8Arch   = 16,  /* CY7C53120L8 */
    /* 17 */ LonNeuronPl3170Arch   = 17,  /* PL 3170 */
    /* 32 */ LonNeuronFT5000Arch   = 32,  /* FT 5000 */
    /* 33 */ LonNeuron5000Arch     = 33,  /* Neuron 5000 */
    /* 36 */ LonNeuronFT6050Arch   = 36,  /* FT 6050 */
    /* 37 */ LonNeuron6050Arch     = 37,  /* Neuron 6050 */
    /* 38 */ LonNeuronFT6010Arch   = 38,  /* FT 6010 */
    /* 39 */ LonNeuron6010Arch     = 39,  /* Neuron 6010 */
    /*114 */ LonStackDxArch        = 112, /* LON Stack DX based device */
    /*115 */ LonStackExArch        = 114, /* LON Stack EX based device */
    /*128 */ LonGenericArch        = 128  /* Generic model code */
} IZOT_ENUM_END(LonArchitecture);

// Neuron Chip and Smart Transceiver state values.
// This enumeration contains the values of the Neuron Chip's or Smart
// Transceiver's state information.  To convert the state value returned by
// the Query Status command to one of these values, use the <IZOT_NODE_STATE>
// macro. To test for the offline flag, use the <IZOT_NODE_STATE_OFFLINE> macro.
#define IZOT_OFFLINE_BIT 0x08
#define IZOT_NODE_STATE_MASK 0x07

// Obtains persistent Neuron state information.
// Use this macro to obtain the 3-bit Neuron state that is stored in EEPROM.
#define IZOT_NEURON_STATE(state) ((state) & IZOT_NODE_STATE_MASK)

// Extracts state information from <IzotNeuronState>.
// Use this macro to decipher the status information encoded within the
// <IzotNeuronState> enumeration.
#define IZOT_NODE_STATE(state)   ((IZOT_NEURON_STATE(state) == IzotConfigOnLine) ? (state) : IZOT_NEURON_STATE(state))

// Extracts offline details from <IzotNeuronState>.
// Use this macro to query the node's offline modes.
#define IZOT_NODE_STATE_OFFLINE(state)   (IZOT_NEURON_STATE(state) == IzotConfigOffLine || IZOT_NEURON_STATE(state) == IzotSoftOffLine)

// Decodes the node's state
typedef IZOT_ENUM_BEGIN(IzotNodeState) {
    /*  0 */ IzotStateInvalid          = 0,    /* invalid or Echelon use only          */
    /*  1 */ IzotStateInvalid_1        = 1,    /* equivalent to StateInvalid          */
    /*  2 */ IzotApplicationUnconfig   = 2,    /* has application, unconfigured        */
    /*  3 */ IzotNoApplicationUnconfig = 3,    /* applicationless, unconfigured        */
    /*  4 */ IzotConfigOnLine          = 4,    /* configured, online                   */
    /*  5 */ IzotStateInvalid_5        = 5,    /* equivalent to StateInvalid          */
    /*  6 */ IzotConfigOffLine         = 6,    /* hard offline                         */
    /*  7 */ IzotStateInvalid_7        = 7,    /* equivalent to StateInvalid          */
    /* 12 */ IzotSoftOffLine           = (IzotConfigOnLine|IZOT_OFFLINE_BIT), /* (12) configured, soft-offline        */
    /*0x8C*/ IzotConfigByPass          = 0x8C  /* configured, in bypass mode    */
} IZOT_ENUM_END(IzotNodeState);

// Controls node mode with <IzotNmSetNodeModeRequest>
typedef IZOT_ENUM_BEGIN(IzotNodeMode) {
    IzotApplicationOffLine  = 0,
    IzotApplicationOnLine   = 1,
    IzotApplicationReset    = 2,
    IzotChangeState         = 3,
    IzotPhysicalReset       = 6
} IZOT_ENUM_END(IzotNodeMode);

// Decodes the last reset cause
typedef IZOT_ENUM_BEGIN(IzotResetCause) {
    /* 0x00 */ IzotResetCleared = 0x00,
    /* 0x01 */ IzotPowerUpReset = 0x01,
    /* 0x02 */ IzotExternalReset = 0x02,
    /* 0x0C */ IzotWatchdogReset = 0x0C,
    /* 0x14 */ IzotSoftwareReset = 0x14
} IZOT_ENUM_END(IzotResetCause);

// Denotes destination address type
// This enumeration holds the literals for the 'type' field of destination
// addresses for outgoing messages.
typedef IZOT_ENUM_BEGIN(IzotAddressType) {
    /*  0 */ IzotAddressUnassigned = 0,
    /*  1 */ IzotAddressSubnetNode,
    /*  2 */ IzotAddressUniqueId,
    /*  3 */ IzotAddressBroadcast,
    /*127 */ IzotAddressLocal = 127
} IZOT_ENUM_END(IzotAddressType);

// Encoded repeat timer values
// This enumeration defines the encoded repeat timer values.
typedef IZOT_ENUM_BEGIN(IzotRepeatTimer) {
    /*  0 */ IzotRpt16,      /* 16 ms */
    /*  1 */ IzotRpt24,      /* 24 ms */
    /*  2 */ IzotRpt32,      /* 32 ms */
    /*  3 */ IzotRpt48,      /* 48 ms */
    /*  4 */ IzotRpt64,      /* 64 ms */
    /*  5 */ IzotRpt96,      /* 96 ms */
    /*  6 */ IzotRpt128,     /* 128 ms */
    /*  7 */ IzotRpt192,     /* 192 ms */
    /*  8 */ IzotRpt256,     /* 256 ms */
    /*  9 */ IzotRpt384,     /* 384 ms */
    /* 10 */ IzotRpt512,     /* 512 ms */
    /* 11 */ IzotRpt768,     /* 768 ms */
    /* 12 */ IzotRpt1024,    /* 1024 ms */
    /* 13 */ IzotRpt1536,    /* 1536 ms */
    /* 14 */ IzotRpt2048,    /* 2048 ms */
    /* 15 */ IzotRpt3072     /* 3072 ms */
} IZOT_ENUM_END(IzotRepeatTimer);

// Encoded receive timer values.
// This enumeration defines the encoded receive timer values used with groups.
// For the non-group receive timer, see <IzotNonGroupReceiveTimer>.
typedef IZOT_ENUM_BEGIN(IzotReceiveTimer) {
    /*  0 */ IzotRcv128,     /* 128 ms */
    /*  1 */ IzotRcv192,     /* 192 ms */
    /*  2 */ IzotRcv256,     /* 256 ms */
    /*  3 */ IzotRcv384,     /* 384 ms */
    /*  4 */ IzotRcv512,     /* 512 ms */
    /*  5 */ IzotRcv768,     /* 768 ms */
    /*  6 */ IzotRcv1024,    /* 1024 ms */
    /*  7 */ IzotRcv1536,    /* 1536 ms */
    /*  8 */ IzotRcv2048,    /* 2048 ms */
    /*  9 */ IzotRcv3072,    /* 3072 ms */
    /* 10 */ IzotRcv4096,    /* 4096 ms */
    /* 11 */ IzotRcv6144,    /* 6144 ms */
    /* 12 */ IzotRcv8192,    /* 8192 ms */
    /* 13 */ IzotRcv12288,   /* 12288 ms */
    /* 14 */ IzotRcv16384,   /* 16384 ms */
    /* 15 */ IzotRcv24576    /* 24576 ms */
} IZOT_ENUM_END(IzotReceiveTimer);

// Encoded non-group receive timer values.
// This enumeration defines the encoded values for the receive timer used
// with any addressing modes other than groups. For the receive timer used
// with groups, use <IzotReceiveTimer>.
typedef IzotReceiveTimer IzotNonGroupReceiveTimer;

// Encoded transmit timer values.
// This enumeration defines the encoded transmit timer values.
typedef IZOT_ENUM_BEGIN(IzotTransmitTimer) {
    /*  0 */ IzotTx16,      /* 16 ms */
    /*  1 */ IzotTx24,      /* 24 ms */
    /*  2 */ IzotTx32,      /* 32 ms */
    /*  3 */ IzotTx48,      /* 48 ms */
    /*  4 */ IzotTx64,      /* 64 ms */
    /*  5 */ IzotTx96,      /* 96 ms */
    /*  6 */ IzotTx128,     /* 128 ms */
    /*  7 */ IzotTx192,     /* 192 ms */
    /*  8 */ IzotTx256,     /* 256 ms */
    /*  9 */ IzotTx384,     /* 384 ms */
    /* 10 */ IzotTx512,     /* 512 ms */
    /* 11 */ IzotTx768,     /* 768 ms */
    /* 12 */ IzotTx1024,    /* 1024 ms */
    /* 13 */ IzotTx1536,    /* 1536 ms */
    /* 14 */ IzotTx2048,    /* 2048 ms */
    /* 15 */ IzotTx3072     /* 3072 ms */
} IZOT_ENUM_END(IzotTransmitTimer);

#ifndef _LTDRIVER_H
// Service LED states
typedef IZOT_ENUM_BEGIN(IzotServiceLedState) {
    SERVICE_OFF = 0,
    SERVICE_ON = 1,
    SERVICE_BLINKING = 2,

    // Software controlled only
    SERVICE_FLICKER = -1
} IZOT_ENUM_END(IzotServiceLedState);

typedef IZOT_ENUM_BEGIN(IzotServiceLedPhysicalState) {
    SERVICE_LED_OFF = 0,
    SERVICE_LED_ON = 1,
} IZOT_ENUM_END(IzotServiceLedPhysicalState);
#endif

/*****************************************************************
 * Section: LON Stack Constants
 *****************************************************************/

#define PROTOCOL_VERSION   0   /* Protocol version                      */
#define MAX_DOMAINS        2   /* Maximum # of domains allowed          */

/* Flex domain indicates that the message was received in flex domain when
   domain index is 2 */
#define FLEX_DOMAIN        2   
      
/* When the application layer communicates with the transport or session layer,
   the domain index for the outgoing message can be either set by the application 
   layer or computed by the transport or session layer based on the destination 
   address.  This value is used only in the TSASenParam structure.      */
#define COMPUTE_DOMAIN_INDEX 3 

#define MAX_GROUP_NUMBER 63    /* Maximum number of a node in a group   */

/* Maximum number of array network variables allowed in
    the application program. This constant is used to allocate
    space that keeps track of all arrays and their dimension */
#define MAX_NV_ARRAYS 10

/* Maximum number of network output variables that can
    be scheduled to be sent out at any point in time */
#define MAX_NV_OUT     5

/* To implement synchronous variables, the values of the
    variables are to be stored along with index in the queue.
    Define the maximum size (in bytes) of a network variable
    in the application program. This is used for storage allocation. */
#define MAX_NV_LENGTH 228

/* Maximum number of network input variables that can
    be scheduled to be polled at any point in time */
#define MAX_NV_IN     50

/* Maximum number of bytes in data array for msg_in, msg_out, resp_in etc.
    This value is indepedent of application buffer sizes mentioned
    earlier. Clearly it does not make sense for this value to be
    larger than application buffer size (out or in). */
#define MAX_DATA_SIZE 255

// Maximum size of message on the wire (might be over sized a byte or two for safety)
#define MAX_PDU_SIZE (MAX_DATA_SIZE+21)

// Maximum LON MAC layer message size (bytes) for non-expanded non-extended
// and extended messages,and expanded non-extended and extended messages with
// framesync byte-stuffing; the LON MAC layer can carry ISO/IEC 14908-1
// payloads up to 228 bytes, or UDP payloads up to 1280 bytes
#define MAX_LON_MSG_NON_EX_LEN		240
#define MAX_EXP_LON_MSG_NON_EX_LEN	(2*MAX_LON_MSG_NON_EX_LEN+4)
#define MAX_LON_MSG_EX_LEN			1280
#define MAX_EXP_LON_MSG_EX_LEN		(2*MAX_LON_MSG_EX_LEN+4)

#define NUM_ADDR_TBL_ENTRIES   254     /* # of address table entries; maximum supported value is 255 */

#define RECEIVE_TRANS_COUNT    16      /* Can be > 16 for Ref. Impl */

#define NV_TABLE_SIZE          254     /* Check management tool for any restriction on maximum size */

#define NV_ALIAS_TABLE_SIZE    254     /* Check management tool for any restriction on maximum size */

/*******************************************************************************
    For some targets, OsalAllocateMemory() uses an array to allocate storage
    space dynamically.  The size of the array used for this allocation is 
    determined by this constant. If it is too low, it may be impossible
    to allocate necessary buffers or data structures.  If it is
    too high, some memory is unused.  To determine an appropriate value,
    set MALLOC_SIZE to some high value, run a test application, stop, and
    check gp->mallocUsedSize to determine the current usage.
    This array space is allocated during reset of all layers.
    Tracing through the reset code of all layers will indicate
    the approximate size of this array necessary.  The MALLOC_SIZE value
    is not used if the target's OsalAllocateMemory() implementation
    does not use such an array.  For example, if the OsalAllocateMemory()
    implementation uses malloc(), this constant is not used.
*******************************************************************************/
#if 0
// TBD: make this conditional based on malloc support in OSAL
#ifndef MALLOC_SIZE
#define MALLOC_SIZE     10050
#endif
#endif

// LON/IP constants
#define BROADCAST_PREFIX       0xEFC00000
#define IP_ADDRESS_LEN         4
#define IBOL_FINISH            0xFF

/*****************************************************************
 * Section: Addressing Type Globals
 *****************************************************************/
/*
 *  This section contains definitions used with source addresses, destination
 *  addresses, and the address table.
 */

/*
 *  Group address structure, used for multicast destination addresses with
 *  <IzotSendAddress>.
 */

// Macros to access the Type field in IzotSendGroup.TypeSize
#define IZOT_SENDGROUP_TYPE_MASK     0x80    /* One for group, zero for all other address types */
#define IZOT_SENDGROUP_TYPE_SHIFT    7
#define IZOT_SENDGROUP_TYPE_FIELD    TypeSize

// Macros to access the Size field in IzotSendGroup.TypeSize
#define IZOT_SENDGROUP_SIZE_MASK     0x7F    /* group size, or zero for unlimited group size */
#define IZOT_SENDGROUP_SIZE_SHIFT    0
#define IZOT_SENDGROUP_SIZE_FIELD    TypeSize

// Macros to access the Domain field in IzotSendGroup.DomainMember
#define IZOT_SENDGROUP_DOMAIN_MASK   0x80    /* domain index for this group, zero or one */
#define IZOT_SENDGROUP_DOMAIN_SHIFT  7
#define IZOT_SENDGROUP_DOMAIN_FIELD  DomainMember

// Macros to access the Member field in IzotSendGroup.DomainMember
#define IZOT_SENDGROUP_MEMBER_MASK   0x7F    /* member ID within the group (0-63). Use zero for unlimited groups. */
#define IZOT_SENDGROUP_MEMBER_SHIFT  0
#define IZOT_SENDGROUP_MEMBER_FIELD  DomainMember

// Macros to access the Repeat field in IzotSendGroup.RepeatRetry
#define IZOT_SENDGROUP_REPEAT_TIMER_MASK   0xF0  /* repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDGROUP_REPEAT_TIMER_SHIFT  4
#define IZOT_SENDGROUP_REPEAT_TIMER_FIELD  RepeatRetry

// Macros to access the Retry field in IzotSendGroup.RepeatRetry
#define IZOT_SENDGROUP_RETRY_MASK    0x0F    /* number of retries, or number of transmissions minus one for unacknowledged service */
#define IZOT_SENDGROUP_RETRY_SHIFT   0
#define IZOT_SENDGROUP_RETRY_FIELD   RepeatRetry

// Macros to access the Receive Timer field in IzotSendGroup.ReceiveTransmit
#define IZOT_SENDGROUP_RECEIVE_TIMER_MASK  0xF0 /* receive timer. Use values from the <IzotReceiveTimer> enumeration. */
#define IZOT_SENDGROUP_RECEIVE_TIMER_SHIFT 4
#define IZOT_SENDGROUP_RECEIVE_TIMER_FIELD ReceiveTransmit

// Macros to access the Transmit Timer field in IzotSendGroup.ReceiveTransmit
#define IZOT_SENDGROUP_TRANSMIT_TIMER_MASK  0x0F /* receive timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDGROUP_TRANSMIT_TIMER_SHIFT 0
#define IZOT_SENDGROUP_TRANSMIT_TIMER_FIELD ReceiveTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendGroup) {
    IzotByte  TypeSize;       /* contains type, size. See IZOT_SENDGROUP_TYPE_* and _SIZE_* macros. */
    IzotByte  DomainMember;   /* contains domain, member. See IZOT_SENDGROUP_DOMAIN_* and _MEMBER_* macros. */
    IzotByte  RepeatRetry;    /* contains repeat, retry. See IZOT_SENDGROUP_REPEAT_* and _RETRY_* macros. */
    IzotByte  ReceiveTransmit;/* contains receive and transmit timers. See IZOT_SENDGROUP_RECEIVE_* and _TRANSMIT_* macros. */
    IzotGroupId GroupId;      /* the group ID, 0..255 */
} IZOT_STRUCT_END(IzotSendGroup);

/*
 *  Subnet/Node address structure, used for unicast destination addresses with
 *  <IzotSendAddress>.
 */

// Macros to access the Domain field in IzotSendSubnetNode.DomainNode
#define IZOT_SENDSN_DOMAIN_MASK  0x80    /* domain index, zero or one */
#define IZOT_SENDSN_DOMAIN_SHIFT 7
#define IZOT_SENDSN_DOMAIN_FIELD DomainNode

// Macros to access the Node field in IzotSendSubnetNode.DomainNode
#define IZOT_SENDSN_NODE_MASK    0x7F    /* node ID 0-127 in this subnet, this domain */
#define IZOT_SENDSN_NODE_SHIFT   0
#define IZOT_SENDSN_NODE_FIELD   DomainNode

// Macros to access the Repeat field in IzotSendSubnetNode.RepeatRetry
#define IZOT_SENDSN_REPEAT_TIMER_MASK    0xF0    /* repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDSN_REPEAT_TIMER_SHIFT   4
#define IZOT_SENDSN_REPEAT_TIMER_FIELD   RepeatRetry

// Macros to access the Retry field in IzotSendSubnetNode.RepeatRetry
#define IZOT_SENDSN_RETRY_MASK   0x0F    /* Retry count */
#define IZOT_SENDSN_RETRY_SHIFT  0
#define IZOT_SENDSN_RETRY_FIELD  RepeatRetry

// Macros to access the rsvd0 field in IzotSendSubnetNode.RsvdTransmit
#define IZOT_SENDSN_RSVD0_MASK   0xF0
#define IZOT_SENDSN_RSVD0_SHIFT  4
#define IZOT_SENDSN_RSVD0_FIELD  RsvdTransmit

// Macros to access the transmit timer field in IzotSendSubnetNode.RsvdTransmit
#define IZOT_SENDSN_TRANSMIT_TIMER_MASK   0x0F   /* Transmit timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDSN_TRANSMIT_TIMER_SHIFT  0
#define IZOT_SENDSN_TRANSMIT_TIMER_FIELD  RsvdTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendSubnetNode) {
    IZOT_ENUM(IzotAddressType) Type;  /* Set to IzotAddressSubnetNode for subnet/node addressing */
    IzotByte  DomainNode;             /* Contains domain, node. See IZOT_SENDSN_DOMAIN_* and _NODE_* macros. */
    IzotByte  RepeatRetry;            /* Contains repeat, retry. See IZOT_SENDSN_REPEAT_* and _RETRY_* macros. */
    IzotByte  RsvdTransmit;           /* Contains rsvd0, transmit. See IZOT_SENDSN_RSVD0_* and _TRANSMIT_TIMER_* macros. */
    IzotSubnetId Subnet;              /* Destination subnet number, 1..255    */
} IZOT_STRUCT_END(IzotSendSubnetNode);

/*
 *  Unique ID (Neuron ID or MAC ID) address structure, used for unicast destination
 *  addresses with <IzotSendAddress>.
 */

// Macros to access the Domain field in IzotSendUniqueId.DomainNode
#define IZOT_SENDNID_DOMAIN_MASK     0x80     /* domain index, zero or one */
#define IZOT_SENDNID_DOMAIN_SHIFT    7
#define IZOT_SENDNID_DOMAIN_FIELD    Domain

// Macros to access the Repeat field in IzotSendUniqueId.RepeatRetry
#define IZOT_SENDNID_REPEAT_TIMER_MASK   0xF0 /* repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDNID_REPEAT_TIMER_SHIFT  4
#define IZOT_SENDNID_REPEAT_TIMER_FIELD  RepeatRetry

// Macros to access the Retry field in IzotSendUniqueId.RepeatRetry
#define IZOT_SENDNID_RETRY_MASK      0x0F    /* Retry count */
#define IZOT_SENDNID_RETRY_SHIFT     0
#define IZOT_SENDNID_RETRY_FIELD     RepeatRetry

// Macros to access the rsvd0 field in IzotSendUniqueId.RsvdTransmit
#define IZOT_SENDNID_RSVD0_MASK  0xF0
#define IZOT_SENDNID_RSVD0_SHIFT 4
#define IZOT_SENDNID_RSVD0_FIELD RsvdTransmit

// Macros to access the transmit timer field in IzotSendUniqueId.RsvdTransmit
#define IZOT_SENDNID_TRANSMIT_TIMER_MASK   0x0F /* Transmit timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDNID_TRANSMIT_TIMER_SHIFT  0
#define IZOT_SENDNID_TRANSMIT_TIMER_FIELD  RsvdTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendUniqueId) {
    IZOT_ENUM(IzotAddressType) Type;  /* Should be IzotAddressUniqueId */
    IzotByte  Domain;                 /* Contains the domain index. See IZOT_SENDNID_DOMAIN_* macro. The remaining 7 bits must be zero. */
    IzotByte  RepeatRetry;            /* Contains repeat, retry. See IZOT_SENDNID_REPEAT_* and _RETRY_* macros. */
    IzotByte  RsvdTransmit;           /* Contains rsvd0, transmit. See IZOT_SENDNID_RSVD0_* and _TRANSMIT_TIMER_* macros. */
    IzotSubnetId Subnet;              /* Destination subnet number, 1..255, or zero to pass all routers */
    IzotUniqueId NeuronId;            /* 48-bit unique ID of Neuron Chip or Smart Transceiver */
} IZOT_STRUCT_END(IzotSendUniqueId);

/*
 *  Broadcast address structure, used for multicast destination addresses with
 *  <IzotSendAddress>.
 */

// Macros to access the Domain field in IzotSendBroadcast.DomainRsvdBacklog
#define IZOT_SENDBCAST_DOMAIN_MASK   0x80    /* Domain index, zero or one */
#define IZOT_SENDBCAST_DOMAIN_SHIFT  7
#define IZOT_SENDBCAST_DOMAIN_FIELD  DomainRsvdBacklog

// Macros to access the rsvd0 field in IzotSendBroadcast.DomainRsvdBacklog
#define IZOT_SENDBCAST_RSVD0_MASK    0x40
#define IZOT_SENDBCAST_RSVD0_SHIFT   6
#define IZOT_SENDBCAST_RSVD0_FIELD   DomainRsvdBacklog

// Macros to access the Backlog field in IzotSendBroadcast.DomainRsvdBacklog
#define IZOT_SENDBCAST_BACKLOG_MASK  0x3F   /* Backlog (set to zero if unknown) */
#define IZOT_SENDBCAST_BACKLOG_SHIFT 0
#define IZOT_SENDBCAST_BACKLOG_FIELD DomainRsvdBacklog

// Macros to access the Repeat field in IzotSendBroadcast.RepeatRetry
#define IZOT_SENDBCAST_REPEAT_TIMER_MASK     0xF0  /* Repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDBCAST_REPEAT_TIMER_SHIFT    4
#define IZOT_SENDBCAST_REPEAT_TIMER_FIELD    RepeatRetry

// Macros to access the Retry field in IzotSendBroadcast.RepeatRetry
#define IZOT_SENDBCAST_RETRY_MASK    0x0F       /* Retry count */
#define IZOT_SENDBCAST_RETRY_SHIFT   0
#define IZOT_SENDBCAST_RETRY_FIELD   RepeatRetry

// Macros to access the rsvd1 field in IzotSendBroadcast.RsvdTransmit
#define IZOT_SENDBCAST_RSVD1_MASK    0xF0		/* maxResponses */
#define IZOT_SENDBCAST_RSVD1_SHIFT   4
#define IZOT_SENDBCAST_RSVD1_FIELD   RsvdTransmit

// Macros to access the transmit timer field in IzotSendBroadcast.RsvdTransmit
#define IZOT_SENDBCAST_TRANSMIT_TIMER_MASK   0x0F /* Transmit timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDBCAST_TRANSMIT_TIMER_SHIFT  0
#define IZOT_SENDBCAST_TRANSMIT_TIMER_FIELD  RsvdTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendBroadcast) {
    IZOT_ENUM(IzotAddressType) Type;  /* Should be IzotAddressBroadcast */
    IzotByte  DomainRsvdBacklog;      /* Contains domain, rsvd0, backlog. See IZOT_SENDBCAST_DOMAIN_*, _RSVD0_* and _BACKLOG_* macros. */
    IzotByte  RepeatRetry;            /* Contains repeat, retry. See IZOT_SENDBCAST_REPEAT_* and _RETRY_* macros. */
    IzotByte  RsvdTransmit;
    IzotSubnetId Subnet;              /* Destination subnet number, 1..255 for subnet broadcast, zero for domain broadcast */
} IZOT_STRUCT_END(IzotSendBroadcast);

/*
 *  Address format to clear an address table entry.
 *  Sets the first 2 bytes of the address table entry to 0.
 */
typedef IZOT_STRUCT_BEGIN(IzotSendUnassigned) {
    IZOT_ENUM(IzotAddressType) Type;   /* Should be IzotAddressUnassigned */
} IZOT_STRUCT_END(IzotSendUnassigned);

/*
 *  Destination address type to address the node locally with <IzotSendAddress>.
 */
typedef IZOT_STRUCT_BEGIN(IzotSendLocal) {
    IZOT_ENUM(IzotAddressType) Type;   /* Should be IzotAddressLocal */
} IZOT_STRUCT_END(IzotSendLocal);

/*
 * Union of all possible destination address formats
 */
typedef IZOT_UNION_BEGIN(IzotSendAddress) {
    IzotSendUnassigned   Unassigned;
    IzotSendGroup        Group;
    IzotSendSubnetNode   SubnetNode;
    IzotSendBroadcast    Broadcast;
    IzotSendUniqueId     UniqueId;
    IzotSendLocal        Local;
} IZOT_UNION_END(IzotSendAddress);

/*
 *  Received subnet/node ID destination addresses, used with unicast messages.
 *  Used with <IzotReceiveDestination> and <IzotReceiveAddress>.
 */

// Macros to access the IzotReceiveSubnetNode.Node field
#define IZOT_RECEIVESN_SELFIELD_MASK  0x80
#define IZOT_RECEIVESN_SELFIELD_SHIFT 7
#define IZOT_RECEIVESN_SELFIELD_FIELD Node 
 
#define IZOT_RECEIVESN_NODE_MASK  0x7F  /* node Id 0..127, MSB is reserved */
#define IZOT_RECEIVESN_NODE_SHIFT 0
#define IZOT_RECEIVESN_NODE_FIELD Node

typedef IZOT_STRUCT_BEGIN(IzotReceiveSubnetNode) {
    IzotByte  Subnet;
    IzotByte  Node;       /* node identifier, use IZOT_RECEIVESN_NODE_* macros */
} IZOT_STRUCT_END(IzotReceiveSubnetNode);

// Received 48-bit unique ID (Neuron ID) destination address.
// Used with IzotReceiveDestination.
typedef IZOT_STRUCT_BEGIN(IzotReceiveUniqueId) {
    IzotSubnetId Subnet;
    IzotUniqueId UniqueId;
} IZOT_STRUCT_END(IzotReceiveUniqueId);

// Received a group destination address.  Used with IzotReceiveDestination.
typedef IZOT_STRUCT_BEGIN(IzotReceiveGroup) {
    IzotGroupId GroupId;             /* 0..255 */
} IZOT_STRUCT_END(IzotReceiveGroup);

// Received a broadcast destination address.  Used with IzotReceiveDestination.
typedef IZOT_STRUCT_BEGIN(IzotReceiveBroadcast) {
    IzotSubnetId SubnetId;       /* 1..255 for subnet broadcast, zero for domain broadcast */
} IZOT_STRUCT_END(IzotReceiveBroadcast);

// Union of all possible address formats for receiving an incoming message.
typedef IZOT_UNION_BEGIN(IzotReceiveDestination) {
    IzotReceiveBroadcast     Broadcast;
    IzotReceiveGroup         Group;
    IzotReceiveSubnetNode    SubnetNode;
    IzotReceiveUniqueId      UniqueId;
} IZOT_UNION_END(IzotReceiveDestination);

// Encodes the format of the receive address of an incoming
// message, allowing you to select the corresponding member in
// IzotReceiveDestination.
typedef IZOT_ENUM_BEGIN(IzotReceiveDestinationAddressFormat) {
    /*  0 */ IzotReceiveDestinationAddressBroadcast = 0,
    /*  1 */ IzotReceiveDestinationAddressGroup,
    /*  2 */ IzotReceiveDestinationAddressSubnetNode,
    /*  3 */ IzotReceiveDestinationAddressUniqueId,
    /*  4 */ IzotReceiveDestinationAddressTurnaround
} IZOT_ENUM_END(IzotReceiveDestinationAddressFormat);

// Receive destination and source address for incoming messages.
//
// This structure holds the receive destination address of an incoming message
// (the address through which the message was received on this node) and the
// message's source address (where it came from).
#define IZOT_RECEIVEADDRESS_DOMAIN_MASK   0x80    /* Domain table index through which the message was received */
#define IZOT_RECEIVEADDRESS_DOMAIN_SHIFT  7
#define IZOT_RECEIVEADDRESS_DOMAIN_FIELD  DomainFormat

#define IZOT_RECEIVEADDRESS_FLEX_MASK     0x40    /* One for flex domain, that is, received message on unconfigured node */
#define IZOT_RECEIVEADDRESS_FLEX_SHIFT    6
#define IZOT_RECEIVEADDRESS_FLEX_FIELD    DomainFormat

#define IZOT_RECEIVEADDRESS_FORMAT_MASK   0x3F    /* Use <IzotReceiveDestinationAddressFormat> enumeration */
#define IZOT_RECEIVEADDRESS_FORMAT_SHIFT  0
#define IZOT_RECEIVEADDRESS_FORMAT_FIELD  DomainFormat

typedef IZOT_STRUCT_BEGIN(IzotReceiveAddress) {
    IzotByte                DomainFormat;   /* Contains domain, flex domain, format. Use IZOT_RECEIVEADDRESS_* macros to access data. */
    IzotReceiveSubnetNode   Source;
    IzotReceiveDestination  Destination;
} IZOT_STRUCT_END(IzotReceiveAddress);

// Source address of a response message
#define IZOT_RESPONSESOURCE_IS_SUBNETNODE_MASK   0x80    /* 1: subnet/node response. 0: group response. */
#define IZOT_RESPONSESOURCE_IS_SUBNETNODE_SHIFT  7
#define IZOT_RESPONSESOURCE_IS_SUBNETNODE_FIELD  Node

#define IZOT_RESPONSESOURCE_NODE_MASK     0x7F    /* Node ID of response source */
#define IZOT_RESPONSESOURCE_NODE_SHIFT    0
#define IZOT_RESPONSESOURCE_NODE_FIELD    Node

typedef IZOT_STRUCT_BEGIN(IzotResponseSource) {
    IzotByte  Subnet;     /* subnet ID */
    IzotByte  Node;       /* contains node, isSubnetNode. Use IZOT_RESPONSESOURCE_NODE_* and IZOT_RESPONSESOURCE_IS_SUBNETNODE_* macros. */
} IZOT_STRUCT_END(IzotResponseSource);

/*
 *  Destination of response to unicast request used with <IzotResponseDestination>.
 */

// Macros to access the IzotReceiveSubnetNode.Node field
#define IZOT_RESPONSESN_NODE_MASK  0x7F    /* node Id 0..127, MSB is reserved */
#define IZOT_RESPONSESN_NODE_SHIFT 0
#define IZOT_RESPONSESN_NODE_FIELD Node

typedef IZOT_STRUCT_BEGIN(IzotResponseSubnetNode) {
    IzotSubnetId Subnet;         /* subnet ID */
    IzotByte     Node;           /* node ID, use IZOT_RESPONSESN_NODE_* macros */
} IZOT_STRUCT_END(IzotResponseSubnetNode);

/*
 *  Destination of response to multicast request used with <IzotResponseDestination>.
 */

// Macros to access the IzotResponseGroup.Node field
#define IZOT_RESPGROUP_NODE_MASK  0x7F    /* node Id 0..127, MSB is reserved */
#define IZOT_RESPGROUP_NODE_SHIFT 0
#define IZOT_RESPGROUP_NODE_FIELD Node

#define IZOT_RESPGROUP_MEMBER_MASK   0x3F
#define IZOT_RESPGROUP_MEMBER_SHIFT  0
#define IZOT_RESPGROUP_MEMBER_FIELD  Member

typedef IZOT_STRUCT_BEGIN(IzotResponseGroup) {
    IzotSubnetId Subnet;         /* subnet ID */
    IzotByte     Node;
    IzotByte     Group;
    IzotByte     Member;         /* use IZOT_RESPGROUP_MEMBER_* macros for access */
} IZOT_STRUCT_END(IzotResponseGroup);

// Destination of a response
typedef IZOT_UNION_BEGIN(IzotResponseDestination) {
    IzotResponseSubnetNode   SubnetNode;
    IzotResponseGroup        Group;
} IZOT_UNION_END(IzotResponseDestination);

// Address of incoming response message.
#define IZOT_RESPONSEADDRESS_DOMAIN_MASK     0x80    /* domain index, zero or one */
#define IZOT_RESPONSEADDRESS_DOMAIN_SHIFT    7
#define IZOT_RESPONSEADDRESS_DOMAIN_FIELD    Domain

#define IZOT_RESPONSEADDRESS_FLEX_MASK       0x40    /* 1: response from foreign domain */
#define IZOT_RESPONSEADDRESS_FLEX_SHIFT      6
#define IZOT_RESPONSEADDRESS_FLEX_FIELD      Domain

typedef IZOT_STRUCT_BEGIN(IzotResponseAddress) {
    IzotByte                 Domain;         /* contains domain, flex domain. Use IZOT_RESPONSEADDRESS_* macros. */
    IzotResponseSource       Source;
    IzotResponseDestination  Destination;
} IZOT_STRUCT_END(IzotResponseAddress);

// Holds explicit addressing details, if enabled.
typedef IZOT_UNION_BEGIN(IzotExplicitAddress) {
    IzotReceiveAddress   Receive;
    IzotSendAddress      Send;
    IzotResponseAddress  Response;
} IZOT_UNION_END(IzotExplicitAddress);

// Holds group addressing information in the address table (<IzotAddress>).
// Used for multicast addressing. This structure also defines which group
// the node belongs to.
#define IZOT_ADDRESS_GROUP_TYPE_MASK     0x80    /* 1 -> group   */
#define IZOT_ADDRESS_GROUP_TYPE_SHIFT    7
#define IZOT_ADDRESS_GROUP_TYPE_FIELD    TypeSize

#define IZOT_ADDRESS_GROUP_SIZE_MASK     0x7F    /* Group size 1..63, or zero for open group */
#define IZOT_ADDRESS_GROUP_SIZE_SHIFT    0
#define IZOT_ADDRESS_GROUP_SIZE_FIELD    TypeSize

#define IZOT_ADDRESS_GROUP_DOMAIN_MASK   0x80    /* Domain index */
#define IZOT_ADDRESS_GROUP_DOMAIN_SHIFT  7
#define IZOT_ADDRESS_GROUP_DOMAIN_FIELD  DomainMember

#define IZOT_ADDRESS_GROUP_MEMBER_MASK   0x7F
#define IZOT_ADDRESS_GROUP_MEMBER_SHIFT  0
#define IZOT_ADDRESS_GROUP_MEMBER_FIELD  DomainMember

#define IZOT_ADDRESS_GROUP_REPEAT_TIMER_MASK  0xF0   /* Repeat timer, use IzotRepeatTimer */
#define IZOT_ADDRESS_GROUP_REPEAT_TIMER_SHIFT 4
#define IZOT_ADDRESS_GROUP_REPEAT_TIMER_FIELD RepeatRetry

#define IZOT_ADDRESS_GROUP_RETRY_MASK    0x0F        /* Retry count */
#define IZOT_ADDRESS_GROUP_RETRY_SHIFT   0
#define IZOT_ADDRESS_GROUP_RETRY_FIELD   RepeatRetry

#define IZOT_ADDRESS_GROUP_RECEIVE_TIMER_MASK 0xF0   /* Receive timer, use IzotReceiveTimer */
#define IZOT_ADDRESS_GROUP_RECEIVE_TIMER_SHIFT   4
#define IZOT_ADDRESS_GROUP_RECEIVE_TIMER_FIELD   ReceiveTransmit

#define IZOT_ADDRESS_GROUP_TRANSMIT_TIMER_MASK 0x0F  /* Transmit timer, use IzotTransmitTimer */
#define IZOT_ADDRESS_GROUP_TRANSMIT_TIMER_SHIFT  0
#define IZOT_ADDRESS_GROUP_TRANSMIT_TIMER_FIELD  ReceiveTransmit

typedef IZOT_STRUCT_BEGIN(IzotAddressTableGroup) {
    IzotByte      TypeSize;               /* Contains type, size. Use the IZOT_ADDRESS_GROUP_* macros. */
    IzotByte      DomainMember;           /* Contains domain, member. Use the IZOT_ADDRESS_GROUP_* macros. */
    IzotByte      RepeatRetry;            /* Contains repeatTimer, retry. Use the IZOT_ADDRESS_GROUP_* macros. */
    IzotByte      ReceiveTransmit;        /* Contains receive and transmit timer. Use Izot_ADDRESS_GROUP_* macros. */
    IzotGroupId   Group;                  /* Group identifier */
} IZOT_STRUCT_END(IzotAddressTableGroup);

// Holds subnet/node addressing information in the address table (<IzotAddress>).
// Used for unicast addressing.
#define IZOT_ADDRESS_SN_DOMAIN_MASK      0x80        /* Domain index */
#define IZOT_ADDRESS_SN_DOMAIN_SHIFT     7
#define IZOT_ADDRESS_SN_DOMAIN_FIELD     DomainNode

#define IZOT_ADDRESS_SN_NODE_MASK        0x7F        /* Node ID */
#define IZOT_ADDRESS_SN_NODE_SHIFT       0
#define IZOT_ADDRESS_SN_NODE_FIELD       DomainNode

#define IZOT_ADDRESS_SN_REPEAT_TIMER_MASK    0xF0    /* Repeat timer, use IzotRepeatTimer */
#define IZOT_ADDRESS_SN_REPEAT_TIMER_SHIFT   4
#define IZOT_ADDRESS_SN_REPEAT_TIMER_FIELD   RepeatRetry

#define IZOT_ADDRESS_SN_RETRY_MASK       0x0F        /* Retry count */
#define IZOT_ADDRESS_SN_RETRY_SHIFT      0
#define IZOT_ADDRESS_SN_RETRY_FIELD      RepeatRetry

typedef IZOT_STRUCT_BEGIN(IzotAddressTableSubnetNode) {
    IZOT_ENUM(IzotAddressType) Type;         /* Set to IzotAddressSubnetNode   */
    IzotByte                  DomainNode;    /* Domain, node. Use IZOT_ADDRESS_SN_* macros. */
    IzotByte                  RepeatRetry;   /* Repeat timer and retry. Use IZOT_ADDRESS_SN_* macros. */
    IZOT_ENUM(IzotTransmitTimer) TransmitTimer;
    IzotSubnetId             Subnet;
} IZOT_STRUCT_END(IzotAddressTableSubnetNode);

// Holds broadcast addressing information in the address table (<IzotAddress>).
// Used for multicast addressing.
#define IZOT_ADDRESS_BROADCAST_DOMAIN_MASK   0x80        /* Domain index */
#define IZOT_ADDRESS_BROADCAST_DOMAIN_SHIFT  7
#define IZOT_ADDRESS_BROADCAST_DOMAIN_FIELD  DomainBacklog

#define IZOT_ADDRESS_BROADCAST_BACKLOG_MASK  0x3F        /* Backlog. Use zero if unknown. */
#define IZOT_ADDRESS_BROADCAST_BACKLOG_SHIFT 0
#define IZOT_ADDRESS_BROADCAST_BACKLOG_FIELD DomainBacklog

#define IZOT_ADDRESS_BROADCAST_REPEAT_TIMER_MASK 0xF0    /* Repeat timer, use IzotRepeatTimer */
#define IZOT_ADDRESS_BROADCAST_REPEAT_SHIFT  4
#define IZOT_ADDRESS_BROADCAST_REPEAT_FIELD  RepeatRetry

#define IZOT_ADDRESS_BROADCAST_RETRY_MASK    0x0F        /* Retry counts */
#define IZOT_ADDRESS_BROADCAST_RETRY_SHIFT   0
#define IZOT_ADDRESS_BROADCAST_RETRY_FIELD   RepeatRetry

typedef IZOT_STRUCT_BEGIN(IzotAddressTableBroadcast) {
    IZOT_ENUM(IzotAddressType) Type;          /* Set to IzotAddressBroadcast */
    IzotByte                   DomainBacklog; /* Domain, backlog. Use IZOT_ADDRESS_BROADCAST_* macros. */
    IzotByte                   RepeatRetry;   /* Repeat timer and retry. Use IZOT_ADDRESS_BROADCAST_* macros instead. */
    IZOT_ENUM(IzotTransmitTimer) TransmitTimer;
    IzotSubnetId               Subnet;
} IZOT_STRUCT_END(IzotAddressTableBroadcast);

// Holds turnaround address information in the address table (<IzotAddress>).
#define IZOT_ADDRESS_TURNAROUND_REPEAT_TIMER_MASK    0xF0    /* Use IzotRepeatTimer */
#define IZOT_ADDRESS_TURNAROUND_REPEAT_TIMER_SHIFT   4
#define IZOT_ADDRESS_TURNAROUND_REPEAT_TIMER_FIELD   RepeatRetry

#define IZOT_ADDRESS_TURNAROUND_RETRY_MASK           0x0F    /* Retry count */
#define IZOT_ADDRESS_TURNAROUND_RETRY_SHIFT          0
#define IZOT_ADDRESS_TURNAROUND_RETRY_FIELD          RepeatRetry

typedef IZOT_STRUCT_BEGIN(IzotAddressTableTurnaround) {
    IZOT_ENUM(IzotAddressType) Type;          /* Set to IzotAddressUnassigned */
    IzotByte                   Turnaround;    /* 1: turnaround record. 0: not in use. */
    IzotByte                   RepeatRetry;   /* Contains repeat timer and retry. Use IZOT_ADDRESS_TURNAROUND_* macros. */
    IZOT_ENUM(IzotTransmitTimer) TransmitTimer;
} IZOT_STRUCT_END(IzotAddressTableTurnaround);

/*****************************************************************
 * Section: LON Host System Globals
 *****************************************************************/
/*
 *  This section contains definitions of system resources and structures, such
 *  as the domain table or address table formats.
 */

// Describes one record of the address table.
typedef IZOT_UNION_BEGIN(IzotAddress) {
    IzotAddressTableGroup      Group;
    IzotAddressTableSubnetNode SubnetNode;
    IzotAddressTableBroadcast  Broadcast;
    IzotAddressTableTurnaround Turnaround;
} IZOT_UNION_END(IzotAddress);

// Encodes the length of the domain.
typedef IZOT_ENUM_BEGIN(IzotDomainLength) {
    /*  0 */ IzotDomainLength_0  = 0,
    /*  1 */ IzotDomainLength_1  = 1,
    /*  3 */ IzotDomainLength_3  = 3,
    /*  6 */ IzotDomainLength_6  = 6
} IZOT_ENUM_END(IzotDomainLength);

// Format for a single domain table record
#define IZOT_DOMAIN_NONCLONE_MASK    0x80
#define IZOT_DOMAIN_NONCLONE_SHIFT   7
#define IZOT_DOMAIN_NONCLONE_FIELD   NodeClone

#define IZOT_DOMAIN_NODE_MASK        0x7F
#define IZOT_DOMAIN_NODE_SHIFT       0
#define IZOT_DOMAIN_NODE_FIELD       NodeClone

#define IZOT_DOMAIN_INVALID_MASK     0x80
#define IZOT_DOMAIN_INVALID_SHIFT    7
#define IZOT_DOMAIN_INVALID_FIELD    InvalidIdLength

#define IZOT_LS_MODE_MASK     		0x40
#define IZOT_LS_MODE_SHIFT    		6
#define IZOT_LS_MODE_FIELD    		InvalidIdLength

#define IZOT_DHCP_FLAG_MASK     	0x20
#define IZOT_DHCP_FLAG_SHIFT    	5
#define IZOT_DHCP_FLAG_FIELD    	InvalidIdLength

#define IZOT_AUTH_TYPE_MASK     	0x18
#define IZOT_AUTH_TYPE_SHIFT    	3
#define IZOT_AUTH_TYPE_FIELD    	InvalidIdLength

#define IZOT_DOMAIN_ID_LENGTH_MASK   0x07
#define IZOT_DOMAIN_ID_LENGTH_SHIFT  0
#define IZOT_DOMAIN_ID_LENGTH_FIELD  InvalidIdLength

typedef IZOT_STRUCT_BEGIN(IzotDomain) {
    IzotDomainId      Id;
    IzotSubnetId      Subnet;
    IzotByte          NodeClone;  /* contains non-clone, node. Use IZOT_DOMAIN_* macros. */
    IzotByte          InvalidIdLength;   /* use IZOT_DOMAIN_INVALID_* and IZOT_DOMAIN_ID_LENGTH_* macros */
    IzotAuthenticationKey Key;
} IZOT_STRUCT_END(IzotDomain);

// Encodes the direction of a datapoint
typedef IZOT_ENUM_BEGIN(IzotDatapointDirection) {
    /*  0 */ IzotDatapointDirectionIsInput  = 0,
    /*  1 */ IzotDatapointDirectionIsOutput = 1
} IZOT_ENUM_END(IzotDatapointDirection);

// Defines the datapoint configuration structure used for registering
// datapoints with LON Stack at initialization time
#define IZOT_DATAPOINT_PRIORITY_MASK    0x80        /* use IzotBool */
#define IZOT_DATAPOINT_PRIORITY_SHIFT   7
#define IZOT_DATAPOINT_PRIORITY_FIELD   SelhiDirPrio

#define IZOT_DATAPOINT_DIRECTION_MASK   0x40        /* use IzotDatapointDirection */
#define IZOT_DATAPOINT_DIRECTION_SHIFT  6
#define IZOT_DATAPOINT_DIRECTION_FIELD  SelhiDirPrio

#define IZOT_DATAPOINT_SELHIGH_MASK     0x3F
#define IZOT_DATAPOINT_SELHIGH_SHIFT    0
#define IZOT_DATAPOINT_SELHIGH_FIELD    SelhiDirPrio

#define IZOT_DATAPOINT_TURNAROUND_MASK  0x80        /* Use IzotBool */
#define IZOT_DATAPOINT_TURNAROUND_SHIFT 7
#define IZOT_DATAPOINT_TURNAROUND_FIELD Attribute1

#define IZOT_DATAPOINT_SERVICE_MASK     0x60        /* Use IzotServiceType */
#define IZOT_DATAPOINT_SERVICE_SHIFT    5
#define IZOT_DATAPOINT_SERVICE_FIELD    Attribute1

#define IZOT_DATAPOINT_AUTHENTICATION_MASK  0x10    /* Use IzotBool */
#define IZOT_DATAPOINT_AUTHENTICATION_SHIFT 4
#define IZOT_DATAPOINT_AUTHENTICATION_FIELD Attribute1

#define IZOT_DATAPOINT_ADDRESS_LOW_MASK     0x0F
#define IZOT_DATAPOINT_ADDRESS_LOW_SHIFT    0
#define IZOT_DATAPOINT_ADDRESS_LOW_FIELD    Attribute1

#define IZOT_DATAPOINT_ADDRESS_HIGH_MASK     0xF0
#define IZOT_DATAPOINT_ADDRESS_HIGH_SHIFT    4
#define IZOT_DATAPOINT_ADDRESS_HIGH_FIELD    Attribute2

#define IZOT_DATAPOINT_AES_MASK  	        0x08    /* Use IzotBool */
#define IZOT_DATAPOINT_AES_SHIFT 	        3
#define IZOT_DATAPOINT_AES_FIELD 	        Attribute2

typedef IZOT_STRUCT_BEGIN(IzotDatapointConfig) {
    IzotByte  SelhiDirPrio;   /* Contains selector-high, direction, priority. Use IZOT_DATAPOINT_* macros. */
    IzotByte  SelectorLow;    /* Contains selector-low. Use IZOT_DATAPOINT_* macros. */
    IzotByte  Attribute1;     /* Contains turnaround, service, authentication, and address-low. Use IZOT_DATAPOINT_* macros. */
	IzotByte  Attribute2;     /* Contains address-high and aes. Use IZOT_DATAPOINT_* macros. */
} IZOT_STRUCT_END(IzotDatapointConfig);

// Literals for the ECS selection type
typedef IZOT_ENUM_BEGIN(IzotSelectionType) {
    IzotSelectionSelectorOnly,      /* 0: normal:  select as long as selector matches */
    IzotSelectionSelectorAndSource, /* 1: select if both selector and source match */
    IzotSelectionNoSelection        /* 2: do not do Dp selection; reserved for poll-only inputs */
} IZOT_ENUM_END(IzotSelectionType);

// Defines the datapoint configuration structure for use with Extended
// Command Set (ECS) devices
#define IZOT_DATAPOINT_ECS_PRIORITY_MASK  0x80     /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_PRIORITY_SHIFT 7
#define IZOT_DATAPOINT_ECS_PRIORITY_FIELD EcsSelhiDirPrio

#define IZOT_DATAPOINT_ECS_DIRECTION_MASK  0x40    /* Use IzotDatapointDirection */
#define IZOT_DATAPOINT_ECS_DIRECTION_SHIFT 6
#define IZOT_DATAPOINT_ECS_DIRECTION_FIELD EcsSelhiDirPrio

#define IZOT_DATAPOINT_ECS_SELHIGH_MASK  0x3F
#define IZOT_DATAPOINT_ECS_SELHIGH_SHIFT 0
#define IZOT_DATAPOINT_ECS_SELHIGH_FIELD EcsSelhiDirPrio

#define IZOT_DATAPOINT_ECS_TURNAROUND_MASK  0x80      /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_TURNAROUND_SHIFT 7
#define IZOT_DATAPOINT_ECS_TURNAROUND_FIELD Attributes1

#define IZOT_DATAPOINT_ECS_AUTHENTICATION_MASK  0x40  /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_AUTHENTICATION_SHIFT 6
#define IZOT_DATAPOINT_ECS_AUTHENTICATION_FIELD Attributes1

#define IZOT_DATAPOINT_ECS_WRITE_BY_INDEX_MASK  0x20  /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_WRITE_BY_INDEX_SHIFT 5
#define IZOT_DATAPOINT_ECS_WRITE_BY_INDEX_FIELD Attributes1

#define IZOT_DATAPOINT_ECS_REMOTE_NM_AUTH_MASK 0x10   /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_REMOTE_NM_AUTH_SHIFT 4
#define IZOT_DATAPOINT_ECS_REMOTE_NM_AUTH_FIELD Attributes1

#define IZOT_DATAPOINT_ECS_RESP_SELECTION_MASK 0x0C   /* Use IzotSelectionType */
#define IZOT_DATAPOINT_ECS_RESP_SELECTION_SHIFT 2
#define IZOT_DATAPOINT_ECS_RESP_SELECTION_FIELD Attributes1

#define IZOT_DATAPOINT_ECS_UNUSED_MBZ_MASK 0x03
#define IZOT_DATAPOINT_ECS_UNUSED_MBZ_SHIFT 0
#define IZOT_DATAPOINT_ECS_UNUSED_MBZ_FIELD Attributes1

#define IZOT_DATAPOINT_ECS_READ_BY_INDEX_MASK 0x80    /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_READ_BY_INDEX_SHIFT 7
#define IZOT_DATAPOINT_ECS_READ_BY_INDEX_FIELD Attributes2

#define IZOT_DATAPOINT_ECS_SERVICE_MASK 0x60          /* Use IzotServiceType */
#define IZOT_DATAPOINT_ECS_SERVICE_SHIFT 5
#define IZOT_DATAPOINT_ECS_SERVICE_FIELD Attributes2

#define IZOT_DATAPOINT_ECS_REQUEST_SELECTION_MASK 0x18   /* Use IzotSelectionType */
#define IZOT_DATAPOINT_ECS_REQUEST_SELECTION_SHIFT 3
#define IZOT_DATAPOINT_ECS_REQUEST_SELECTION_FIELD Attributes2

#define IZOT_DATAPOINT_ECS_UPDATE_SELECTION_MASK 0x06    /* Use IzotSelectionType */
#define IZOT_DATAPOINT_ECS_UPDATE_SELECTION_SHIFT 1
#define IZOT_DATAPOINT_ECS_UPDATE_SELECTION_FIELD Attributes2

#define IZOT_DATAPOINT_ECS_SOURCE_SELECTION_MASK 0x01    /* Use IzotBool */
#define IZOT_DATAPOINT_ECS_SOURCE_SELECTION_SHIFT 0
#define IZOT_DATAPOINT_ECS_SOURCE_SELECTION_FIELD Attributes2

typedef IZOT_STRUCT_BEGIN(IzotDatapointEcsConfig) {
    IzotByte  EcsSelhiDirPrio; /* selector-high, direction, priority. Use IZOT_DATAPOINT_ECS_* macros. */
    IzotByte  SelectorLow;
    IzotByte  Attributes1;     /* turnaround, authentication write-by-index, remote-nm-auth, response-selection. Use IZOT_DATAPOINT_* macros. */
    IzotByte  Attributes2;     /* read-by-index, service, request-selection, update-selection, source-selection-only. Use IZOT_DATAPOINT_ECS_* macros. */
    IzotWord  AddressIndex;    /* Address table index */
    IzotWord  DatapointIndex;  /* Index of remote datapoint */
} IZOT_STRUCT_END(IzotDatapointEcsConfig);

// Defines a datapoint alias for legacy devices
typedef IZOT_STRUCT_BEGIN(IzotAliasConfig) {
    IzotDatapointConfig        Alias;
    IzotByte                   Primary;
} IZOT_STRUCT_END(IzotAliasConfig);

// Defines a datapoint alias in Extended Command Set (ECS) devices, including devices
// based on LON Stack
typedef IZOT_STRUCT_BEGIN(IzotAliasEcsConfig) {
    IzotDatapointEcsConfig        Alias;
    IzotWord                      Primary;
} IZOT_STRUCT_END(IzotAliasEcsConfig);

// Defines direct-mode transceiver parameters
#define IZOT_DIRECT_XCVR_CD_MASK             0x80    /* Collision-detect */
#define IZOT_DIRECT_XCVR_CD_SHIFT            7
#define IZOT_DIRECT_XCVR_CD_FIELD            Parameter_1

#define IZOT_DIRECT_XCVR_BST_MASK            0x60    /* Bit-sync-threshold */
#define IZOT_DIRECT_XCVR_BST_SHIFT           5
#define IZOT_DIRECT_XCVR_BST_FIELD           Parameter_1

#define IZOT_DIRECT_XCVR_FILTER_MASK         0x18    /* Filter */
#define IZOT_DIRECT_XCVR_FILTER_SHIFT        3
#define IZOT_DIRECT_XCVR_FILTER_FIELD        Parameter_1

#define IZOT_DIRECT_XCVR_HYSTERESIS_MASK     0x07    /* Hysteresis */
#define IZOT_DIRECT_XCVR_HYSTERESIS_SHIFT    0
#define IZOT_DIRECT_XCVR_HYSTERESIS_FIELD    Parameter_1

#define IZOT_DIRECT_XCVR_CDTEP_MASK          0xFC    /* CD to end packet */
#define IZOT_DIRECT_XCVR_CDTEP_SHIFT         2
#define IZOT_DIRECT_XCVR_CDTEP_FIELD         Parameter_2

#define IZOT_DIRECT_XCVR_CDTAIL_MASK         0x02    /* CD tail */
#define IZOT_DIRECT_XCVR_CDTAIL_SHIFT        1
#define IZOT_DIRECT_XCVR_CDTAIL_FIELD        Parameter_2

#define IZOT_DIRECT_XCVR_CDPREAMBLE_MASK     0x01    /* CD preamble */
#define IZOT_DIRECT_XCVR_CDPREAMBLE_SHIFT    0
#define IZOT_DIRECT_XCVR_CDPREAMBLE_FIELD    Parameter_2

typedef IZOT_STRUCT_BEGIN(IzotDirectModeTransceiver) {
    IzotByte      Parameter_1;    /* Collision-detect, bit-sync-threshold, filter, and hysteresis. Use IZOT_DIRECT_XCVR_* macros. */
    IzotByte      Parameter_2;    /* cd-to-end-packet, cd-tail, cd-preamble. Use IZOT_DIRECT_XCVR_* macros. */
} IZOT_STRUCT_END(IzotDirectModeTransceiver);

// Bitmask values that define the attributes of an NV for the <flags> field
// in <IzotDatapointDefinition>
#define IZOT_DATAPOINT_NONE             0x00000000 /* No flags specified. */
#define IZOT_DATAPOINT_CONFIG_CLASS     0x00000001 /* Implements a property. */
#define IZOT_DATAPOINT_AUTH_CONFIG      0x00000002 /* Authentication is configurable. */
#define IZOT_DATAPOINT_PRIORITY_CONFIG  0x00000004 /* Priority is configurable. */
#define IZOT_DATAPOINT_SERVICE_CONFIG   0x00000008 /* TService type is configurable. */
#define IZOT_DATAPOINT_OFFLINE          0x00000010 /* Network tools should only change this value when the device is offline. */
#define IZOT_DATAPOINT_POLLED           0x00000020 /* This is either a polling input or a polled output. */
#define IZOT_DATAPOINT_SYNC             0x00000040 /* A synchronous datapoint. */
#define IZOT_DATAPOINT_CHANGEABLE       0x00000080 /* The datapoint type is changeable. */
#define IZOT_DATAPOINT_PRIORITY         0x00000100 /* Default to priority. */
#define IZOT_DATAPOINT_AUTHENTICATED    0x00000200 /* Default to authenticated. */
#define IZOT_DATAPOINT_ACKD             0x00000400 /* Default to acknowledged service. */
#define IZOT_DATAPOINT_UNACKD_RPT       0x00000800 /* Default to unacknowledged repeat service. */
#define IZOT_DATAPOINT_UNACKD           0x00001000 /* Default to unacknowledged service. */
#define IZOT_DATAPOINT_PERSISTENT       0x00004000 /* Default to persistent datapoints */
#define IZOT_DATAPOINT_IS_OUTPUT        0x00008000 /* The datapoint is an output datapoint. */

// Unspecified max or mean datapoint rate
#define IZOT_DATAPOINT_RATE_UNKNOWN      0

// <IzotDatapointDefinition> structure version
#define IZOT_DATAPOINT_DEFINITION_CURRENT_VERSION 2

// Size type for datapoints
typedef uint8_t IzotDatapointSize;

// Defines the attributes of an NV (datapoint); this is used to register
// static NVs with LON Stack, and is also used to retrieve information about
// static or dynamic NVs
typedef IZOT_STRUCT_BEGIN(IzotDatapointDefinition) {
    uint8_t         Version;        /* If LON Stack does not recognize this version,
                                       the structure will be rejected.  The current
                                       version is IZOT_DATAPOINT_DEFINITION_CURRENT_VERSION */
    volatile void const *PValue;    /* Pointer to the datapoint value */
    IzotDatapointSize DeclaredSize; /* The declared size of the NV (1 to 228);
                                       this is also the initial size and the
                                       maximum size */
    uint16_t        SnvtId;         /* Specifies the NV type; set to 1 - 250
                                       for standard types (SNVTs); set to 0
                                       for non-standard (user) types */
    uint16_t        ArrayCount;     /* Specifies the NV array size; set to
                                       1 - 4096 for an NV array; set to 0 for
                                       a scalar */
    uint32_t        Flags;          /* Bit flags describing NV attributes based
                                       on <IZOT_DATAPOINT_*> values */
    const char      *Name;          /* NV name; limited to 16 bytes for the
                                       base name plus an array designator
                                       of [dddd] where [dddd] is a one to four
                                       digit decimal number from 0 to 4095 */
    const char      *SdString;      /* Self-documentation string with up to
                                       1023 characters; set to null if none */
    uint8_t         MaxRate;        /* Encoded maximum rate (0 to 127, or 255);
                                       set to IZOT_DATAPOINT_RATE_UKNOWN if not
                                       specified */
    uint8_t         MeanRate;       /* Encoded rate(0 to 127, or 255); set to
                                       IZOT_DATAPOINT_RATE_UNKNOWN if not specified */                    
    const uint8_t   *ibol;          /* Points to the IBOL sequence */
    uint16_t        NvIndex;        /* NV index -- added for version 2 */
} IZOT_STRUCT_END(IzotDatapointDefinition);

/*
 *  LON device configuration data structure
 */
#define IZOT_CONFIG_COMM_CLOCK_MASK      0xF8    /* Communications clock rate */
#define IZOT_CONFIG_COMM_CLOCK_SHIFT     3
#define IZOT_CONFIG_COMM_CLOCK_FIELD     Clock

#define IZOT_CONFIG_INPUT_CLOCK_MASK     0x07    /* Input clock rate */
#define IZOT_CONFIG_INPUT_CLOCK_SHIFT    0
#define IZOT_CONFIG_INPUT_CLOCK_FIELD    Clock

#define IZOT_CONFIG_COMM_TYPE_MASK       0xE0    /* Communications type */
#define IZOT_CONFIG_COMM_TYPE_SHIFT      5
#define IZOT_CONFIG_COMM_TYPE_FIELD      CommConfiguration

#define IZOT_CONFIG_COMM_PINDIR_MASK     0x1F    /* Pin direction */
#define IZOT_CONFIG_COMM_PINDIR_SHIFT    0
#define IZOT_CONFIG_COMM_PINDIR_FIELD    CommConfiguration

#define IZOT_CONFIG_NONGRPRCV_MASK       0xF0    /* Non-group receive timer, use <IzotNonGroupReceiveTimer> */
#define IZOT_CONFIG_NONGRPRCV_SHIFT      4
#define IZOT_CONFIG_NONGRPRCV_FIELD      Config_1

#define IZOT_CONFIG_NMAUTH_MASK          0x08    /* Network management authentication */
#define IZOT_CONFIG_NMAUTH_SHIFT         3
#define IZOT_CONFIG_NMAUTH_FIELD         Config_1

#define IZOT_CONFIG_PREEMPT_MASK         0x07    /* Pre-emption timeout */
#define IZOT_CONFIG_PREEMPT_SHIFT        0
#define IZOT_CONFIG_PREEMPT_FIELD        Config_1

typedef IZOT_STRUCT_BEGIN(IzotConfigData) {
    IzotWord         ChannelId;
    IzotLocationId   Location;
    IzotByte         Clock;              /* Input  and communications clocks; use IZOT_CONFIG_* macros */
    IzotByte         CommConfiguration;  /* Communications type and communications pin direction; use IZOT_CONFIG_* macros */
    IzotByte         PreambleLength;
    IzotByte         PacketCycle;
    IzotByte         Beta2Control;
    IzotByte         TransmitInterpacket;
    IzotByte         ReceiveInterpacket;
    IzotByte         NodePriority;
    IzotByte         ChannelPriorities;
    IZOT_UNION_BEGIN(CommunicationParameters) {
        IzotTransceiverParameters    TransceiverParameters;
        IzotDirectModeTransceiver    DirectModeParameters;
    } IZOT_UNION_END(CommunicationParameters);
    IzotByte          Config_1;           /* Pre-emption timeout, network management authentication, non-group receive timer; use IZOT_CONFIG_* macros */
} IZOT_STRUCT_END(IzotConfigData);


#define IZOT_READONLY_CHECKSUM_MASK                  0xF0    /* Checksum for Unique Node ID */
#define IZOT_READONLY_CHECKSUM_SHIFT                 4
#define IZOT_READONLY_CHECKSUM_FIELD                 CheckSumMinorNum

#define IZOT_READONLY_MINORNUM_MASK                  0x0F    /* Minor architecture number */
#define IZOT_READONLY_MINORNUM_SHIFT                 0
#define IZOT_READONLY_MINORNUM_FIELD                 CheckSumMinorNum

#define IZOT_READONLY_RW_PROTECT_MASK                0x80    /* Read+write protect flag */
#define IZOT_READONLY_RW_PROTECT_SHIFT               7
#define IZOT_READONLY_RW_PROTECT_FIELD               ReadOnly_1

#define IZOT_READONLY_RUN_UNCONFIG_MASK              0x40    /* Run when unconfigured */
#define IZOT_READONLY_RUN_UNCONFIG_SHIFT             6
#define IZOT_READONLY_RUN_UNCONFIG_FIELD             ReadOnly_1

#define IZOT_READONLY_DATAPOINT_COUNT_MASK           0x3F    /* NV count */
#define IZOT_READONLY_DATAPOINT_COUNT_SHIFT          0
#define IZOT_READONLY_DATAPOINT_COUNT_FIELD          ReadOnly_1

#define IZOT_READONLY_DATAPOINT_PROCESSINGOFF_MASK   0x80    /* NV processing off */
#define IZOT_READONLY_DATAPOINT_PROCESSINGOFF_SHIFT  7
#define IZOT_READONLY_DATAPOINT_PROCESSINGOFF_FIELD  ReadOnly_2

#define IZOT_READONLY_TWO_DOMAINS_MASK               0x40    /* Two domains */
#define IZOT_READONLY_TWO_DOMAINS_SHIFT              6
#define IZOT_READONLY_TWO_DOMAINS_FIELD              ReadOnly_2

#define IZOT_READONLY_RESERVED2_MASK                 0x20    /* Reserved 2 */
#define IZOT_READONLY_RESERVED2_SHIFT                5
#define IZOT_READONLY_RESERVED2_FIELD                ReadOnly_2

#define IZOT_READONLY_RESERVED3_MASK                 0x10    /* Reserved 3 */
#define IZOT_READONLY_RESERVED3_SHIFT                4
#define IZOT_READONLY_RESERVED3_FIELD                ReadOnly_2

#define IZOT_READONLY_MSG_PROCESS_MASK               0x08    /* Message process */
#define IZOT_READONLY_MSG_PROCESS_SHIFT              3
#define IZOT_READONLY_MSG_PROCESS_FIELD              ReadOnly_2

#define IZOT_READONLY_NODE_STATE_MASK                0x07    /* Node state */
#define IZOT_READONLY_NODE_STATE_SHIFT               0
#define IZOT_READONLY_NODE_STATE_FIELD               ReadOnly_2

#define IZOT_READONLY_ADDRESS_CNT_MASK               0xF0    /* Address table size  */
#define IZOT_READONLY_ADDRESS_CNT_SHIFT              4
#define IZOT_READONLY_ADDRESS_CNT_FIELD              ReadOnly_3

#define IZOT_READONLY_RESERVED5_MASK                 0x0F    /* Reserved 5 */
#define IZOT_READONLY_RESERVED5_SHIFT                0
#define IZOT_READONLY_RESERVED5_FIELD                ReadOnly_3

#define IZOT_READONLY_RESERVED6_MASK                 0xF0    /* Reserved 6 */
#define IZOT_READONLY_RESERVED6_SHIFT                4
#define IZOT_READONLY_RESERVED6_FIELD                ReadOnly_4

#define IZOT_READONLY_REC_TRANSCNT_MASK              0x0F    /* Receive transaction count */
#define IZOT_READONLY_REC_TRANSCNT_SHIFT             0
#define IZOT_READONLY_REC_TRANSCNT_FIELD             ReadOnly_4

#define IZOT_READONLY_OUTBUF_SIZE_MASK               0xF0    /* Application output buffer size */
#define IZOT_READONLY_OUTBUF_SIZE_SHIFT              4
#define IZOT_READONLY_OUTBUF_SIZE_FIELD              AppBufSize

#define IZOT_READONLY_INBUF_SIZE_MASK                0x0F    /* Application input buffer size */
#define IZOT_READONLY_INBUF_SIZE_SHIFT               0
#define IZOT_READONLY_INBUF_SIZE_FIELD               AppBufSize

#define IZOT_READONLY_NW_OUTBUF_SIZE_MASK            0xF0    /* Network output buffer size */
#define IZOT_READONLY_NW_OUTBUF_SIZE_SHIFT           4
#define IZOT_READONLY_NW_OUTBUF_SIZE_FIELD           NwBufSize

#define IZOT_READONLY_NW_INBUF_SIZE_MASK             0x0F    /* Network input buffer size */
#define IZOT_READONLY_NW_INBUF_SIZE_SHIFT            0
#define IZOT_READONLY_NW_INBUF_SIZE_FIELD            NwBufSize

#define IZOT_READONLY_NW_OUT_PRICNT_MASK             0xF0    /* Network output buffer priority count */
#define IZOT_READONLY_NW_OUT_PRICNT_SHIFT            4
#define IZOT_READONLY_NW_OUT_PRICNT_FIELD            PriCnt

#define IZOT_READONLY_OUT_PRICNT_MASK                0x0F    /* Application output buffer priority count */
#define IZOT_READONLY_OUT_PRICNT_SHIFT               0
#define IZOT_READONLY_OUT_PRICNT_FIELD               PriCnt

#define IZOT_READONLY_OUTBUF_CNT_MASK                0xF0    /* Application output buffer count */
#define IZOT_READONLY_OUTBUF_CNT_SHIFT               4
#define IZOT_READONLY_OUTBUF_CNT_FIELD               AppBufCnt

#define IZOT_READONLY_INBUF_CNT_MASK                 0x0F    /* Application input buffer count */
#define IZOT_READONLY_INBUF_CNT_SHIFT                0
#define IZOT_READONLY_INBUF_CNT_FIELD                AppBufCnt

#define IZOT_READONLY_NW_OUTBUF_CNT_MASK             0xF0    /* Network output buffer count */
#define IZOT_READONLY_NW_OUTBUF_CNT_SHIFT            4
#define IZOT_READONLY_NW_OUTBUF_CNT_FIELD            NwBufCnt

#define IZOT_READONLY_NW_INBUF_CNT_MASK              0x0F    /* Network input buffer count */
#define IZOT_READONLY_NW_INBUF_CNT_SHIFT             0
#define IZOT_READONLY_NW_INBUF_CNT_FIELD             NwBufCnt

#define IZOT_READONLY_RESERVED7_MASK                 0xFC    /* Reserved 7 */
#define IZOT_READONLY_RESERVED7_SHIFT                4
#define IZOT_READONLY_RESERVED7_FIELD                ReadOnly_5

#define IZOT_READONLY_TX_BY_ADDRESS_MASK             0x02    /* Transaction by address */
#define IZOT_READONLY_TX_BY_ADDRESS_SHIFT            1
#define IZOT_READONLY_TX_BY_ADDRESS_FIELD            ReadOnly_5

#define IZOT_READONLY_RESERVED8_MASK                 0x01    /* Reserved 8 */
#define IZOT_READONLY_RESERVED8_SHIFT                0
#define IZOT_READONLY_RESERVED8_FIELD                ReadOnly_5

#define IZOT_READONLY_RESERVED9_MASK                 0xC0    /* Reserved 9 */
#define IZOT_READONLY_RESERVED9_SHIFT                6
#define IZOT_READONLY_RESERVED9_FIELD                ReadOnly_6

#define IZOT_READONLY_ALIAS_CNT_MASK                 0x3F    /* Alias count */
#define IZOT_READONLY_ALIAS_CNT_SHIFT                0
#define IZOT_READONLY_ALIAS_CNT_FIELD                ReadOnly_6

#define IZOT_READONLY_MSG_TAG_CNT_MASK              0xF0     /* Message tag count */
#define IZOT_READONLY_MSG_TAG_CNT_SHIFT             4
#define IZOT_READONLY_MSG_TAG_CNT_FIELD             ReadOnly_7

#define IZOT_READONLY_RESERVED10_MASK               0x0F     /* Reserved 10 */
#define IZOT_READONLY_RESERVED10_SHIFT              0
#define IZOT_READONLY_RESERVED10_FIELD              ReadOnly_7

#define IZOT_READONLY_DMF_MASK                       0x80    /* Direct-memory file */
#define IZOT_READONLY_DMF_SHIFT                      0
#define IZOT_READONLY_DMF_FIELD                      ReadOnly_8

#define IZOT_READONLY_SEC_II_MASK                    0x40    /* Security II */
#define IZOT_READONLY_SEC_II_SHIFT                   6
#define IZOT_READONLY_SEC_II_FIELD                   ReadOnly_8

#define IZOT_READONLY_RESERVED11_MASK                0x7F    /* Reserved 11 */
#define IZOT_READONLY_RESERVED11_SHIFT               0
#define IZOT_READONLY_RESERVED11_FIELD               ReadOnly_8

typedef IZOT_STRUCT_BEGIN(IzotReadOnlyData) {
    IzotUniqueId  UniqueNodeId;      /* 48-bit unique ID for the device                                   */
    IzotByte  ArchNum;               /* LON architecture; set to <LonArchitecture> value                  */
    IzotByte  CheckSumMinorNum;      /* Checksum for Unique Node ID and minorArchNum (0-128); use IZOT_READONLY_* values */
    IzotByte  DatapointFixed[2];     /* Location of NV fixed table                                        */
    IzotByte  ReadOnly_1;            /* Contains:  (Use IZOT_READONLY_* values)                           */
                                     /* readWriteProtect , 1,       Read+write protect flag               */
                                     /* runWhenUnconf    , 1,       1 => Application runs when unconfigured */
                                     /* nvCount          , 6,                                             */
    IzotByte  SnvtStruct[2];         /* 0xFFFF for LON Stack                                              */
    IzotProgramId ProgramId;         /* Program ID string (array of 8 bytes)                              */
    IzotByte  ReadOnly_2;            /* Contains:  (Use IZOT_READONLY_* values)                           */
                                     /* dpProcessingOff  , 1,       Must be one for NodeUtil              */
                                     /* twoDomains       , 1,       1 if node is in 2 domains             */
                                     /* Reserved 2       , 1,       Explicit addressing not used          */
                                     /* Reserved 3       , 1,       Unused                                */
                                     /* msgProcess       , 1,       1 means explicit messages processed   */
                                     /* nodeState        , 3,       Node State. See eia709_1.h            */
    IzotByte ReadOnly_3;             /* Contains: (Use IZOT_READONLY_* values)                            */
                                     /* # of entries in address table (AddressCn)                         */
                                     /* Reserved 5       , 4        Unused                                */
    IzotByte ReadOnly_4;             /* Contains:  (Use IZOT_READONLY_* values)                           */
                                     /* Reserved 6       , 4,       Unused                                */
                                     /* receiveTransCnt  , 4        RR Cnt = this field + 1               */
    IzotByte AppBufSize;             /* appOutBufSize    , 4,       Special size encoding                 */
                                     /* appInBufSize     , 4        Special size encoding                 */
    IzotByte NwBufSize;              /* nwOutBufSize     , 4,       Special size encoding                 */
                                     /* nwInBufSize      , 4        Special size encoding                 */
    IzotByte PriCnt;                 /* nwOutBufPriCnt   , 4,       Special count encoding                */
                                     /* appOutBufPriCnt  , 4        Special count encoding                */
    IzotByte AppBufCnt;              /* appOutBufCnt     , 4,       Special count encoding                */
                                     /* appInBufCnt      , 4        Special count encoding                */
    IzotByte NwBufCnt;               /* nwOutBufCnt      , 4,       Special count encoding                */
                                     /* nwInBufCnt       , 4        Special count encoding                */
    IzotByte Reserved0;              /* Unused                                                            */
    IzotByte Reserved1[2];           /* Unused                                                            */
    IzotByte Reserved2[3];           /* Unused                                                            */
    IzotByte ReadOnly_5;             /* Contains:  (Use IZOT_READONLY_* values)                           */
                                     /* Reserved 7       , 6,       Unused                                */
                                     /* txByAddress      , 1,       0 in LON Stack                        */
                                     /* Reserved 8       , 1)       Unused                                */
    IzotByte ReadOnly_6;             /* Reserved 9       , 2,       Unused                                */
                                     /* aliasCnt         , 6)       0 in LON Stack                        */
    IzotByte ReadOnly_7;             /* msgTagCnt        , 4,       0 in LON Stack                        */
                                     /* Reserved 10      , 4)       Unused                                */
    IzotByte Reserved3[3];           /* Unused                                                            */
    IzotByte DatapointCount;
    IzotByte AliasCount;
    IzotByte Snvt2Hi;
    IzotByte Snvt2Lo;
    IzotByte ReadOnly_8;             /* Direct memory file , 1,                                           */
                                     /* r11              , 7                                              */
	IzotByte Extended;				/* Number of additional address table entries
									   for an Extended Address table  */
} IZOT_STRUCT_END(IzotReadOnlyData);

// Node status and statistics structure
typedef IZOT_STRUCT_BEGIN(IzotStatus) {
    IzotWord                  TransmitErrors;
    IzotWord                  TransactionTimeouts;
    IzotWord                  ReceiveTransactionsFull;
    IzotWord                  LostMessages;
    IzotWord                  MissedMessages;
    IZOT_ENUM(IzotResetCause) ResetCause;
    IZOT_ENUM(IzotNodeState)  NodeState;
    IzotByte                  VersionNumber;
    IZOT_ENUM(LonStatusCode)  ErrorLog;
    IZOT_ENUM(LonArchitecture) ArchitectureNumber;
    /*
     * The following members are available through the local <IzotQueryStatus>
     * API only, but are not transmitted in response to a query status network
     * diagnostic request.
     */
    IzotWord                  LostEvents;
} IZOT_STRUCT_END(IzotStatus);

/*****************************************************************
 * Section: LON Network Interface Messages
 *****************************************************************/
/*
 * Messages exchanged between LON Stack Layer 2 and the LON/IP or native
 * LON network interface have up to four parts, as follows:
 *
 *   Network Interface Command (NiHdr)                         (2 Bytes)
 *   Message Header (MsgHdr)                                   (3 Bytes)
 *   Network Address (ExplicitAddr)                            (11 Bytes)
 *   Data (MsgData)                                            (varies)
 *
 * Network Interface Command (NiHdr):
 *
 *   The network interface command is always present.  It contains the
 *   network interface command and queue specifier.  This is the only
 *   field required for local network interface commands such as 
 *   LonNiResetDeviceCmd.
 *
 * Message Header (MsgHdr: union of NetVarHdr and ExpMsgHdr):
 *
 *   This field is present if the buffer is a data transfer or a completion
 *   event.  The message header describes the type of LON message contained
 *   in the data field.
 *
 *   NetVarHdr is used if the message is a network variable message and
 *   network interface selection is enabled.
 *
 *   ExpMsgHdr is used if the message is an explicit message, or a network
 *   variable message and host selection is enabled.
 *
 * Network Address (ExplicitAddr:  SendAddrDtl, RcvAddrDtl, or RespAddrDtl)
 *
 *   This field is present if the message is a data transfer or completion
 *   event, and explicit addressing is enabled.  The network address
 *   specifies the destination address for downlink application buffers,
 *   or the source address for uplink application buffers.
 *
 *   SendAddrDtl is used for outgoing messages or NV updates.
 *
 *   RcvAddrDtl is used  for incoming messages or unsolicited NV updates.
 * 
 *   RespAddrDtl is used for incoming responses or NV updates solicited
 *   by a poll.
 *
 * Data (MsgData: union of UnprocessedNV, ProcessedNV, and ExplicitMsg)
 *
 *   This field is present if the message is a data transfer or completion
 *   event.
 *
 *   If the message is a completion event, then the first two bytes of the
 *   data are included.  This provides the NV index, the NV selector, or the
 *   message code as appropriate.
 *
 *   UnprocessedNV is used if the message is a network variable update, and
 *   host selection is enabled. It consists of a two-byte header followed by
 *   the NV data.
 * 
 *   ProcessedNV is used if the message is a network variable update, and
 *   network interface selection is enabled. It consists of a two-byte header
 *   followed by the NV data.
 *
 *   ExplicitMsg is used if the message is an explicit message.  It consists
 *   of a one-byte code field followed by the message data.
 */

// Network interface command codes; many of these commands are optional and
// are not supported by all network interfaces
typedef IZOT_ENUM_BEGIN(LonNiCommand) {
	LonNiClearCmd 			= 0x00,		// Clear (no operation)--used by driver
	LonNiNormalTxnQueue		= 0x02,		// Normal transaction queue--used by driver
	LonNiPriorityTxnQueue	= 0x03,		// Priority transaction queue--used by driver
	LonNiNormalNonTxnQueue	= 0x04,		// Normal non-transaction queue
	LonNiPriNonTxnQueue	    = 0x05,		// Priority non-transaction queue
    LonNiResponseQueue      = 0x06,     // Response message & completion event queue
    LonNiIncomingQueue      = 0x08,     // Received message queue
	LonNiCommandMask		= 0x10,     // LON network interface command bit--used by driver
    LonNiApplicationData    = 0x10,     // Application data
    LonNiResponseStatus     = 0x11,     // Response/status from the NI to host
    LonNiNetworkMgmtCmd     = 0x12,     // Network management command
    LonNiDiagnosticData		= 0x13,     // Diagnostic data used for debugging, testing, and statistics
	LonNiResponseCmd		= 0x16,     // Used by stack L2
	LonNiIncomingCmd		= 0x18,     // Incoming command--used by driver for Wink
	LonNiIncomingL2Cmd		= 0x1A,     // Used by stack L2
	LonNiIncomingL2Mode1Cmd	= 0x1B,		// Layer 2 Mode 1 frame--used by stack L2
	LonNiIncomingL2Mode2Cmd	= 0x1C,		// Layer 2 Mode 2 frame--used by stack L2
    LonNiNetMgmtCmd		    = 0x20,
	LonNiLocalNetMgmtCmd	= 0x22,     // Local network management command--used by stack L2 and driver
	LonNiError			    = 0x30,     // General network interface error-used by stack L2 and driver
	LonNiCrcError		    = 0x31,     // LON network interface CRC error--used by driver
	LonNiPhaseModeCmd	    = 0x40,		// PL phase setting, or with 1 or 2--used by stack L2
	LonNiResetDeviceCmd	    = 0x50,     // Reset device--used by stack L2 and driver
	LonNiInitiateCmd		= 0x51,     // Used by driver
	LonNiChallengeResponse	= 0x52,     // Used by driver
	LonNiReplyCmd			= 0x53,
	LonNiFlushCompleteCmd	= 0x60,     // Uplink command--used by driver
    LonNiFlushCancelCmd     = 0x60,     // Downlink command
    LonNiOnlineCmd          = 0x70,
    LonNiOfflineCmd         = 0x80,
	LonNiFlushCmd			= 0x90,
    LonNiFlushIgnoreCmd     = 0xA0,
    LonNiSleepCmd           = 0xB0,
	LonNiSetPhaseCmd		= 0xC0,		// IO_0 - IO_3 output low--PL only
	LonNiAckCmd				= 0xC0,     // Used by driver
	LonNiNackCmd			= 0xC1,     // Used by driver
	LonNiDriverCmdMask		= 0xD0,     // Used by driver
	LonNiSetL5ModeCmd		= 0xD0, 	// MIP/U50 only--used by driver
	LonNiSetL2ModeCmd		= 0xD1, 	// MIP/U50 only--used by driver
	LonNiIdentifyCmd		= 0xD4,     // Used by driver
	LonNiFlowControlCmd	    = 0xD5,
	LonNiResetDriverCmd	    = 0xD6,
	LonNiCycleDriverCmd	    = 0xD7,
	LonNiStatusCmd			= 0xE0,     // Used by driver
    LonNiPupXoffCmd         = 0xE1,
    LonNiPupXonCmd          = 0xE2,
    LonNiPtrHRotlCmd        = 0xE4,
	LonNiLayerModeCmd	    = 0xE5,		// Layer mode in payload--used by driver
    LonNiRqEnableCmd        = 0xE5,
    LonNiTxIdCmd            = 0xE8,
    LonNiSltaPlsCmd         = 0xEA,     // SLTA only
	LonNiNonLonCmd		    = 0xEF,
    LonNiDriverCmd          = 0xF0
} IZOT_ENUM_END(LonNiCommand);

/*****************************************************************
 * Section: LON Message Globals
 *****************************************************************/
/*
 *  This section defines LON message formats, codes, and utilities for
 *  dealing with LON messages.  It includes definitions for network
 *  management and diagnostic messages.
 */

// Message codes for network management and diagnostic classes of messages
typedef IZOT_ENUM_BEGIN(IzotNmMessageCode) {
    // Network diagnostic commands
    IzotNdQueryStatus                   = 0x51,
    IzotNdProxy                         = 0x52,
    IzotNdClearStatus                   = 0x53,
    IzotNdQueryXcvr                     = 0x54,
    IzotNdQueryStatusFlexDomain         = 0x56,

    // Network management commands
    IzotNmExpanded                      = 0x60,
    IzotNmQueryId                       = 0x61,
    IzotNmRespondToQuery                = 0x62,
    IzotNmUpdateDomain                  = 0x63,
    IzotNmLeaveDomain                   = 0x64,
    IzotNmUpdateKey                     = 0x65,
    IzotNmUpdateAddr                    = 0x66,
    IzotNmQueryAddr                     = 0x67,
    IzotNmQueryDatapointConfig          = 0x68,
    IzotNmUpdateGroupAddr               = 0x69,
    IzotNmQueryDomain                   = 0x6A,
    IzotNmUpdateDatapointConfig         = 0x6B,
    IzotNmSetNodeMode                   = 0x6C,
    IzotNmReadMemory                    = 0x6D,
    IzotNmWriteMemory                   = 0x6E,
    IzotNmChecksumRecalculation         = 0x6F,
    IzotNmWink                          = 0x70,
    IzotNmInstall                       = IzotNmWink, /* See <IzotInstallCommand> */
    IzotNmAppCommand                    = IzotNmWink,
    IzotNmMemoryRefresh                 = 0x71,
    IzotNmQuerySnvt                     = 0x72,
    IzotNmDatapointFetch                = 0x73,
    IzotNmDeviceEscape                  = 0x7D,
    IzotNmRouterEscape                  = 0x7E,
    IzotNmServicePin                    = 0x7F
} IZOT_ENUM_END(IzotNmMessageCode);

#define NI_CMD_MASK			0xF0

// Extended installation commands for devices that use SI data version 2; SI
// data version 2 is required for any device that supports dynamic datapoints;
// used by IzotNmInstallRequest()
typedef IZOT_ENUM_BEGIN(IzotInstallCommand) {
    IzotInstallWink               = 0,    /* Basic application wink                     */
    IzotInstallQueryDatapointInfo = 4,    /* Query datapoint information         */
    IzotInstallQueryNodeInfo      = 5     /* Query node self-documentation information  */
} IZOT_ENUM_END(IzotInstallCommand);

// Types of datapoint information that can be queried using
// IzotInstallQueryDatapointInfo; used by IzotNmInstallRequest when Command is
// set to IzotInstallQueryDatapointInfo
typedef IZOT_ENUM_BEGIN(IzotDatapointInfoType) {
    IzotDatapointInfoDescriptor         = 0,    /* Query datapoint description (IzotNmInstallResponse.DatapointDescriptor) */
    IzotDatapointInfoRateEstimate       = 1,    /* Query datapoint rate estimates (IzotNmInstallResponse.DatapointRate) */
    IzotDatapointInfoName               = 2,    /* Query datapoint Name (IzotNmInstallResponse.DatapointName) */
    IzotDatapointInfoSdText             = 3,    /* Query datapoint self-documentation string (IzotNmInstallResponse.DatapointSd) */
    IzotDatapointInfoSnvtIndex          = 4     /* Query datapoint SNVT index (IzotNmInstallResponse.SnvtTypeIndex) */
} IZOT_ENUM_END(IzotDatapointInfoType);

// Types of node information that can be queried using IzotInstallQueryNodeInfo
// Used by IzotNmInstallRequest when Command is set to IzotInstallQueryNodeInfo
typedef IZOT_ENUM_BEGIN(IzotNodeInfoType) {
    IzotNodeInfoSdText             = 3,    /* Query node self-documentation string (IzotNmInstallResponse.NodeSd) */
} IZOT_ENUM_END(IzotNodeInfoType);

// Defines the origins of an NV; use the IZOT_DATAPOINT_DESC_ORIGIN_* values
// with the <IzotNmInstallResponse> union
typedef IZOT_ENUM_BEGIN(IzotDatapointOrigin) {
    IzotDatapointOriginUndefined    = 0,    /* Not defined          */
    IzotDatapointOriginStatic       = 1,    /* Statically defined   */
    IzotDatapointOriginDynamic      = 2,    /* Dynamically defined  */
} IZOT_ENUM_END(IzotDatapointOrigin);

// Failure response codes for network management and diagnostic classes of
// messages
#define IZOT_NM_FAILURE(c)            ((c) & 0x1F)

// Success response codes for network management and diagnostic classes of
// messages
#define IZOT_NM_SUCCESS(c)            (IZOT_NM_FAILURE(c) | 0x20)

// Maximum number of bytes in a datapoint name, not including the 0 terminator
#define IZOT_DATAPOINT_NAME_LEN 16

// Offsets and masks for constructing request and response codes
#define IZOT_NM_OPCODE_BASE      0x60
#define IZOT_NM_OPCODE_MASK      0x1F
#define IZOT_NM_RESPONSE_MASK    0xE0
#define IZOT_NM_RESPONSE_SUCCESS 0x20
#define IZOT_NM_RESPONSE_FAILED  0x00

#define IZOT_ND_OPCODE_BASE      0x50
#define IZOT_ND_OPCODE_MASK      0x0F
#define IZOT_ND_RESPONSE_MASK    0xF0
#define IZOT_ND_RESPONSE_SUCCESS 0x30
#define IZOT_ND_RESPONSE_FAILED  0x10

// Data structure used for correlating request messages and their responses
typedef const void *IzotCorrelator;

// Structure for uplink reset message; used in the Reset callback
typedef void IzotResetNotification;

// Message structure for an NV fetch request
typedef IZOT_STRUCT_BEGIN(IzotNmDatapointFetchRequest) {
    IzotByte  Index;
    IzotWord  EscapeIndex;  /* Exists iff index==0xFF    */
} IZOT_STRUCT_END(IzotNmDatapointFetchRequest);

// Message structure used with *IzotNmInstall* requests.  Each member
// of this union contains a *Command* field (<IzotInstallCommand>)
// that specifies the command type.  Even though this structure is defined as
// a union, the message size should include only the fields required for the
// particular command type.
typedef IZOT_UNION_BEGIN(IzotNmInstallRequest) {
    IZOT_STRUCT_NESTED_BEGIN(Wink) {
        IZOT_ENUM(IzotInstallCommand) Command; /* *IzotInstallWink* */
    } IZOT_STRUCT_NESTED_END(Wink);

    IZOT_STRUCT_NESTED_BEGIN(QueryDatapointInfo) {
        IZOT_ENUM(IzotInstallCommand) Command; /* *IzotInstallQueryDatapointInfo* */
        IZOT_ENUM(IzotDatapointInfoType)     DatapointInfoType;  /* Requested datapoint information */
        IzotWord                     DatapointIndex; /* Datapoint index */

        /* The following parameters are used only if DatapointInfoType is
         * *IzotDatapointInfoSdText*, and should be omitted when other types of Datapoint
         * information are being queried.
         */
        IZOT_UNION_NESTED_BEGIN(AdditionalParameters) {
            IZOT_STRUCT_NESTED_BEGIN(SdText) {
                /* Used when DatapointInfoType is *IzotDatapointInfoSdText* */
                IzotWord Offset;         /* Byte offset from beginning of SD text */
                IzotByte Length;         /* Maximum number of SD bytes to return  */
            } IZOT_STRUCT_NESTED_END(SdText);
        } IZOT_UNION_NESTED_END(AdditionalParameters);
    } IZOT_STRUCT_NESTED_END(QueryDatapointInfo);

    IZOT_STRUCT_NESTED_BEGIN(QueryNodeInfo) {
        IZOT_ENUM(IzotInstallCommand) Command;      /* *IzotInstallQueryNodeInfo*  */
        IZOT_ENUM(IzotNodeInfoType)   NodeInfoType; /* Requested node information */
        IZOT_UNION_NESTED_BEGIN(AdditionalParameters) {
            IZOT_STRUCT_NESTED_BEGIN(SdText) {
                /* Used when NodeInfoType is *IzotNodeInfoSdText* */
                IzotWord Offset;        /* Byte offset from beginning of SD text */
                IzotByte Length;        /* Maximum number of SD bytes to return  */
            } IZOT_STRUCT_NESTED_END(SdText);
        } IZOT_UNION_NESTED_END(AdditionalParameters);
    } IZOT_STRUCT_NESTED_END(QueryNodeInfo);
} IZOT_UNION_END(IzotNmInstallRequest);

/*
 * Message structure used with *IzotNmInstall* responses.  Each member
 * of this union contains a *Command* field (<IzotInstallCommand>) that
 * specifies the command type.  Even though this structure is defined as
 * a union, the message size should include only the fields required for the
 * particular command type.
 */

// Macros to access the length field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_LENGTH_MASK   0xf8          /* Datapoint length */
#define IZOT_DATAPOINT_DESC_LENGTH_SHIFT  3
#define IZOT_DATAPOINT_DESC_LENGTH_FIELD  LengthAndOrigin

// Macros to access the origin field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ORIGIN_MASK   0x07          /* Origin; use IzotDatapointOrigin */
#define IZOT_DATAPOINT_DESC_ORIGIN_SHIFT  0
#define IZOT_DATAPOINT_DESC_ORIGIN_FIELD  LengthAndOrigin

// Macros to access the direction field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_IS_OUTPUT_MASK   0x10       /* 1: output, 0: input */
#define IZOT_DATAPOINT_DESC_IS_OUTPUT_SHIFT  4
#define IZOT_DATAPOINT_DESC_IS_OUTPUT_FIELD  Defaults

// Macros to access the default authentication field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_DFLT_AUTH_MASK   0x08       /* 1: authenticated, 0: unauthenticated */
#define IZOT_DATAPOINT_DESC_DFLT_AUTH_SHIFT  3
#define IZOT_DATAPOINT_DESC_DFLT_AUTH_FIELD  Defaults

// Macros to access the default priority field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_DFLT_PRIORITY_MASK   0x04   /* 1: priority, 0: non-priority */
#define IZOT_DATAPOINT_DESC_DFLT_PRIORITY_SHIFT  2
#define IZOT_DATAPOINT_DESC_DFLT_PRIORITY_FIELD  Defaults

// Macros to access the default service field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_DFLT_SERVICE_MASK   0x03    /* Default service type; use IzotServiceType. */
#define IZOT_DATAPOINT_DESC_DFLT_SERVICE_SHIFT  0
#define IZOT_DATAPOINT_DESC_DFLT_SERVICE_FIELD  Defaults

// Macros to access the dp_sync field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_SYNC_MASK   0x40       /* 1: sync Datapoint, 0: non-sync Datapoint */
#define IZOT_DATAPOINT_DESC_ATTR_SYNC_SHIFT  6
#define IZOT_DATAPOINT_DESC_ATTR_SYNC_FIELD  BasicAttributes

// Macros to access the dp_polled field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_POLLED_MASK   0x20     /* 1: polled or polling input, 0: not polled */
#define IZOT_DATAPOINT_DESC_ATTR_POLLED_SHIFT  5
#define IZOT_DATAPOINT_DESC_ATTR_POLLED_FIELD  BasicAttributes

// Macros to access the dp_offline field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_OFFLINE_MASK   0x10    /* 1: take offline prior to updating */
#define IZOT_DATAPOINT_DESC_ATTR_OFFLINE_SHIFT  4
#define IZOT_DATAPOINT_DESC_ATTR_OFFLINE_FIELD  BasicAttributes

// Macros to access the dp_service_type_config field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_MASK   0x08    /* 1: service type is configurable */
#define IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_SHIFT  3
#define IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_FIELD  BasicAttributes

// Macros to access the dp_priority_config field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_MASK   0x04    /* 1: priority flag is configurable */
#define IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_SHIFT  2
#define IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_FIELD  BasicAttributes

// Macros to access the dp_auth_config field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_MASK   0x02    /* 1: authentication flag is configurable */
#define IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_SHIFT  1
#define IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_FIELD  BasicAttributes

// Macros to access the dp_config_class field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_MASK   0x01    /* 1: a config Datapoint, 0: non-config Datapoint */
#define IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_SHIFT  0
#define IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_FIELD  BasicAttributes

// Macros to access the max rate estimate (mre) field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_MASK   0x80    /* 1: Max rate estimate available using IzotInstallQueryDatapointInfo, IzotDatapointInfoRateEstimate */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_SHIFT  7
#define IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_FIELD  ExtendedAttributes

// Macros to access the rate estimate (re) field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_MASK   0x40    /* 1: Average rate estimate available using IzotInstallQueryDatapointInfo, IzotDatapointInfoRateEstimate */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_SHIFT  6
#define IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_FIELD  ExtendedAttributes

// Macros to access the dp_name field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_MASK   0x20    /* 1: Datapoint Name is available using IzotInstallQueryDatapointInfo, IzotDatapointInfoName. */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_SHIFT  5
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_FIELD  ExtendedAttributes

// Macros to access the sd field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_MASK   0x10    /* 1: Datapoint SD text is available using IzotInstallQueryDatapointInfo, IzotDatapointInfoSdText  */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_SHIFT  4
#define IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_FIELD  ExtendedAttributes

// Macros to access the name_supplied field in IzotNmInstallResponse.DatapointDescriptor
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_MASK   0x08    /* 1: Name is supplied in IzotNmInstallResponse.DatapointDescriptor */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_SHIFT  3
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_FIELD  ExtendedAttributes

typedef IZOT_UNION_BEGIN(IzotNmInstallResponse) {
    /* Response for IzotInstallQueryDatapointInfo, IzotDatapointInfoDescriptor */
    IZOT_STRUCT_NESTED_BEGIN(DatapointDescriptor) {
        IzotByte LengthAndOrigin;    /* Use IZOT_DATAPOINT_DESC_LENGTH_* and IZOT_DATAPOINT_DESC_ORIGIN_* macros */
        IzotByte Defaults;           /* Use IZOT_DATAPOINT_DESC_IS_OUTPUT_* and IZOT_DATAPOINT_DESC_DFLT_* macros */
        IzotByte BasicAttributes;    /* Use IZOT_DATAPOINT_DESC_ATTR_* macros */
        IzotByte SnvtIndex;
        IzotByte ExtendedAttributes; /* Use IZOT_DATAPOINT_DESC_EXT_ATTR_* macros */
        IzotWord ArraySize;
        IzotWord ArrayElement;
        char    DatapointName[IZOT_DATAPOINT_NAME_LEN]; /* Optional field - Included only if
                                        IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED flag is set. */
    } IZOT_STRUCT_NESTED_END(DatapointDescriptor);

    /* Response for IzotInstallQueryDatapointInfo, IzotDatapointInfoRateEstimate */
    IZOT_STRUCT_NESTED_BEGIN(DatapointRate) {
        IzotByte    RateEstimate;       /* Encoded rate estimate. Only valid if 'IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL' is set in DatapointDescriptor. */
        IzotByte    MaxRateEstimate;    /* Encoded max rate estimate; only valid if 'IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL' is set in DatapointDescriptor. */
    } IZOT_STRUCT_NESTED_END(DatapointRate);

    /* Response for IzotInstallQueryDatapointInfo, IzotDatapointInfoName */
    char DatapointName[IZOT_DATAPOINT_NAME_LEN];   /* Datapoint name; only valid if 'nm' set in Datapoint DatapointDescriptor. */

    /* Response for IzotInstallQueryDatapointInfo, IzotDatapointInfoSdText */
    IZOT_STRUCT_NESTED_BEGIN(DatapointSd) {
        IzotByte    Length;      /* Number of bytes of SD text returned */
        IzotByte    Text[1];     /* SD text - actual length is Length above.
                                 * Might not be NULL terminated. */
    } IZOT_STRUCT_NESTED_END(DatapointSd);

    /* Response for IzotInstallQueryDatapointInfo, IzotDatapointInfoSnvtIndex */
    IzotByte    SnvtTypeIndex;

    /* Response for IzotInstallQueryNodeInfo, IzotNodeInfoSdText */
    IZOT_STRUCT_NESTED_BEGIN(NodeSd) {
        IzotByte    Length;      /* Number of bytes of SD text returned */
        IzotByte    Text[1];     /* SD text - actual length is Length above.
                                  * Might not be NULL terminated. */
    } IZOT_STRUCT_NESTED_END(NodeSd);
} IZOT_UNION_END(IzotNmInstallResponse) ;

// Message structure for standard network management command *IzotNmSetNodeMode*
typedef IZOT_STRUCT_BEGIN(IzotNmSetNodeModeRequest) {
    IZOT_ENUM(IzotNodeMode)     Mode;
    IZOT_ENUM(IzotNodeState)    State;  /* iff mode == IzotChangeState */
} IZOT_STRUCT_END(IzotNmSetNodeModeRequest);

// Defines addressing mode for memory read and write request
typedef IZOT_ENUM_BEGIN(IzotMemoryReadWriteMode) {
    IzotAbsoluteMemory           = 0,         /* Address is absolute memory address */
    IzotReadOnlyRelative         = 1,         /* Address is offset into read-only memory structures */
    IzotConfigStructRelative     = 2,         /* Address is offset into configuration data structures */
    IzotStatisticStructRelative  = 3,         /* Address is offset into statistics data structures */
    IzotMemoryModeReserved_A     = 4          /* Reserved */
} IZOT_ENUM_END(IzotMemoryReadWriteMode);

// Defines actions that follow a memory write request
typedef IZOT_ENUM_BEGIN(IzotMemoryWriteForm) {
    IzotNoAction                   = 0,
    IzotBothCsRecalculation        = 1,
    IzotDeltaCsRecalculation       = 3,
    IzotConfigCsRecalculation      = 4,
    IzotOnlyReset                  = 8,
    IzotBothCsRecalculationReset   = 9,
    IzotConfigCsRecalculationReset = 12
} IZOT_ENUM_END(IzotMemoryWriteForm);

// Message structure used with the *IzotNmReadMemory* request
typedef IZOT_STRUCT_BEGIN(IzotNmReadMemoryRequest) {
    IZOT_ENUM(IzotMemoryReadWriteMode)     Mode;
    IzotWord     Address;
    IzotByte     Count;
} IZOT_STRUCT_END(IzotNmReadMemoryRequest);

// Message structure used with the *IzotNmWriteMemory* request. This
// structure shows only the message header. The message header is
// followed by the data to write, which must be *count* bytes following
// the *Form* field.
typedef IZOT_STRUCT_BEGIN(IzotNmWriteMemoryRequest) {
    IZOT_ENUM(IzotMemoryReadWriteMode)    Mode;
    IzotWord     Address;
    IzotByte     Count;
    IZOT_ENUM(IzotMemoryWriteForm)        Form;
    /* <count> bytes of data following... */
} IZOT_STRUCT_END(IzotNmWriteMemoryRequest);

// Message codes used with application messages. Application message
// codes are in the [IzotApplicationMsg, IzotForeignMsg] range, but
// these values are frequently used and should be avoided:
// IzotApplicationIsi is used by the Interoperable Self-Installation (ISI)
// protocol, and IzotApplicationFtp is used for the interoperable file
// transfer protocol (FTP).
typedef IZOT_ENUM_BEGIN(IzotApplicationMessageCode) {
    IzotApplicationMsg        = 0x00,
    IzotApplicationIsi        = 0x3D,
    IzotApplicationFtp        = 0x3E,
    IzotApplicationIsOffLine  = 0x3F,
    IzotForeignMsg            = 0x40,
    IzotForeignIsOffLine      = 0x4F,
    IzotLastMessageCode       = 0x4F
} IZOT_ENUM_END(IzotApplicationMessageCode);

// Message structure used with *IzotNmQueryDomain*
typedef IZOT_STRUCT_BEGIN(IzotNmQueryDomainRequest) {
    IzotByte Index;             /* Domain index                 */
} IZOT_STRUCT_END(IzotNmQueryDomainRequest);

// Message structure used with *IzotNmQueryDatapointConfig*
typedef IZOT_STRUCT_BEGIN(IzotNmQueryDatapointAliasRequest) {
    IzotWord  Index;             /* Datapoint config table index                 */
} IZOT_STRUCT_END(IzotNmQueryDatapointAliasRequest);

// Message structure used with *IzotNmQueryAddr*
typedef IZOT_STRUCT_BEGIN(IzotNmQueryAddressRequest) {
    IzotByte Index;             /* Address table index                 */
} IZOT_STRUCT_END(IzotNmQueryAddressRequest);

// Message structure used with responses to *IzotNdQueryStatus*
typedef IZOT_STRUCT_BEGIN(IzotNdQueryStatusResponse) {
    IzotStatus    Status;
} IZOT_STRUCT_END(IzotNdQueryStatusResponse);

// Message structure used with responses to *IzotNdQueryXcvr*
typedef IZOT_STRUCT_BEGIN(IzotNdQueryXcvrResponse) {
    IzotTransceiverParameters    Status;
} IZOT_STRUCT_END(IzotNdQueryXcvrResponse);

// Message structure used with *IzotNmUpdateAddr* requests
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateAddressRequest) {
    IzotByte      Index;
    IzotAddress   Address;
} IZOT_STRUCT_END(IzotNmUpdateAddressRequest);

// Message structure used with *IzotNmUpdateDatapointConfig* requests.
// This structure is used to update the datapoint configuration table.
// The structure exists in two forms. If the ShortIndex field is in the
// range 0..254, use the ShortForm union member. If the short index equals
// 255, use the LongForm union member and obtain the true index from the
// LongIndex field. Note that the data structure shown is an abstraction;
// the actual message frame used is the smallest possible.
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateDatapointRequest) {
    IzotByte      ShortIndex;

    IZOT_UNION_NESTED_BEGIN(Request) {
        IZOT_STRUCT_NESTED_BEGIN(ShortForm) {
            IzotDatapointConfig     DatapointConfig;
        } IZOT_STRUCT_NESTED_END(ShortForm);
        IZOT_STRUCT_NESTED_BEGIN(LongForm) {
            IzotWord LongIndex;
            IzotDatapointConfig     DatapointConfig;
        } IZOT_STRUCT_NESTED_END(LongForm);
    } IZOT_UNION_NESTED_END(Request);
} IZOT_STRUCT_END(IzotNmUpdateDatapointRequest);

// Message structure used with *IzotNmUpdateDatapointConfig* requests.
// This structure is used to update the alias configuration table. The
// structure exists in two forms. If the 'ShortIndex' field is in the range
// 0..254, use the ShortForm union member. If the short index equals 255, use
// the LongForm union member and obtain the true index from the LongIndex
// field. The index should be that of the alias plus the datapoint count;
// the alias table follows the datapoint config table and the index
// continues from that table.  Note that the data structure shown is an
// abstraction; the actual message frame used is the smallest possible.
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateAliasRequest) {
    IzotByte ShortIndex;

    IZOT_UNION_NESTED_BEGIN(Request) {
        IZOT_STRUCT_NESTED_BEGIN(ShortForm) {
            IzotAliasConfig  AliasConfig;
        } IZOT_STRUCT_NESTED_END(ShortForm);
        IZOT_STRUCT_NESTED_BEGIN(LongForm) {
            IzotWord LongIndex;
            IzotAliasConfig  AliasConfig;
        } IZOT_STRUCT_NESTED_END(LongForm);
    } IZOT_UNION_NESTED_END(Request);
} IZOT_STRUCT_END(IzotNmUpdateAliasRequest);

// Message structure used with the *IzotNmUpdateDomain* request
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateDomainRequest) {
    IzotByte     Index;
    IzotDomain   Domain;
} IZOT_STRUCT_END(IzotNmUpdateDomainRequest);

// Message structure used for an *IzotNmUpdateDomain* response
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateDomainResponse) {
    IzotDomain   Domain;
} IZOT_STRUCT_END(IzotNmUpdateDomainResponse);

// LON Service message structure
typedef IZOT_STRUCT_BEGIN(IzotServiceMsg) {
    IzotByte    unique_id[IZOT_UNIQUE_ID_LENGTH];
    IzotByte    id_string[IZOT_PROGRAM_ID_LENGTH];
} IZOT_STRUCT_END(IzotServiceMsg);

// LON network interface status command (LonNiStatusCmd) response message structure
typedef IZOT_STRUCT_BEGIN(IzotLonInterfaceStatusResponse) {
    IzotByte    model_id;       // LON interface model ID; 30 (0x1E) for U60 FT and
                                // U10 Rev C (MIP/U50)
    IzotByte    fw_version;     // LON interface firmware version; encoded as (Value / 10). 
                                // Example: 0xA0 = 160, interpreted as version 16.0
    IzotByte    layer_mode;     // LON interface layer mode; use LonUsbInterfaceMode values
    IzotByte    status;         // LON interface status; 0 is ready
} IZOT_STRUCT_END(IzotLonInterfaceStatusResponse);

// LON message service types
typedef IZOT_ENUM_BEGIN(IzotServiceType)
{
    IzotServiceAcknowledged     = 0,    /* ACKD         */
    IzotServiceRepeated         = 1,    /* UNACKD_RPT   */
    IzotServiceUnacknowledged   = 2,    /* UNACKD       */
    IzotServiceRequest          = 3,    /* REQUEST      */
	IzotServiceResponse         = 4     /* RESPONSE     */ /* Session Layer */
} IZOT_ENUM_END(IzotServiceType);

/*****************************************************************
 * Section: Non-Volatile Data Globals
 *****************************************************************/
/*
 * This section contains the enumerations and data types used for non-volatile
 * data (NVD) support.
 */

// Persistent data segment type. Configuration data is stored persistently
// in persistent data segments, identified using this enumeration.  Used
// by the non-volatile callback functions IzotPersistentxxx.
typedef IZOT_ENUM_BEGIN(IzotPersistentSegType) {
	IzotPersistentSegNetworkImage,      /* Basic network configuration, such as the
                                           domain table, address tables, and 
                                           datapoint configuration.
                                         */
    IzotPersistentSegSecurityII,        /* Security state and Replay Table information */                                     
	IzotPersistentSegNodeDefinition,    /* Definitions that affect the current
                                           interface, including dynamic datapoint
                                           definitions.
                                        */
	IzotPersistentSegApplicationData,   /* Application data, such as CP values, that
                                           needs to be stored persistently.  In some
                                           devices this can include CP values
                                           implemented by datapoints, CP values
                                           defined in files, or both.
                                        */
    IzotPersistentSegUniqueId,          /* Unique ID */
    IzotPersistentSegConnectionTable,   /* ISI connection table */
    IzotPersistentSegIsi,               /* Other ISI persistent data */
    IzotPersistentSegNumSegmentTypes,   /* Number of segment types */
    IzotPersistentSegUnassigned = 0xFF, /* Unassigned segment type */
} IZOT_ENUM_END(IzotPersistentSegType);

// <IzotStackInterfaceData> structure version
#define IZOT_STACK_INTERFACE_CURRENT_VERSION 0

// <IzotControlData> structure version
#define IZOT_CONTROL_DATA_CURRENT_VERSION 0

// Number of bytes of communication parameter data
#define IzoT_NUM_COMM_BYTES 16

// LON transceiver types
typedef IZOT_ENUM_BEGIN(IzotTransceiverType) {
    IzotTransceiverTypeDefault,     /* Leave transceiver comm parameters unchanged */
    IzotTransceiverType5MHz,        /* Use hard-coded 5MHz comm parameters */
    IzotTransceiverType10MHz,       /* Use hard-coded 10MHz comm parameters */
    IzotTransceiverType20MHz,       /* Use hard-coded 20MHz comm parameters */
    IzotTransceiverType40MHz,       /* Use hard-coded 40MHz comm parameters */
    IzotTransceiverTypeCustom,      /* Use custom comm parameters in CommParms field */
} IZOT_ENUM_END(IzotTransceiverType);

// Static LON application attributes
typedef IZOT_STRUCT_BEGIN(IzotStackInterfaceData) {
    uint8_t Version;                /* Format version number for future
                                       extensions.  If LON Stack does not
                                       recognize the version, the structure
                                       will be rejected.  The current version is
                                       IZOT_STACK_INTERFACE_CURRENT_VERSION. */
    uint32_t AppSignature;          /* 32-bit unique numerical application
                                       identifier. */
    IzotProgramId ProgramId;        /* Program ID string (array of 8 bytes) */
    uint16_t StaticDatapoints;      /* Number of static NVs */
    uint16_t MaxDatapoints;         /* Maximum number of NVs (0..4096).  This 
                                       is the maximum total number of static 
                                       and dynamic NVs */
    uint8_t Domains;                /* Number of domain in the node (1 or 2) */
    uint16_t Addresses;             /* Maximum number of address table entries
                                       (0..4096) */
    uint16_t Aliases;               /* Maximum number of alias tables (0..8192) */
    uint16_t BindableMsgTags;       /* Number of bindable message tags (0.. 4096) */
    const char *NodeSdString;       /* Node self documentation string */
    uint8_t AvgDynDatapointSdLength;  /* Average number of bytes to reserve for
                                       dynamic datapoint self-documentation data */
    /* The following fields are added in version 1 of this structure: */
    uint8_t *SiData;                /* Pointer to self-identification data, NULL for EX */
    uint32_t SiDataLength;          /* Size of self-identification data, in bytes (0 for EX) */
} IZOT_STRUCT_END(IzotStackInterfaceData);

// Data structure used to control the runtime aspects of LON Stack
typedef IZOT_STRUCT_BEGIN(IzotControlData) {
    uint8_t Version;                /* Format version number for future
                                       extensions.  Unknown versions will be
                                       rejected.  The current version is
                                       <IZOT_CONTROL_DATA_CURRENT_VERSION>. */
    uint32_t Flags;                 /* See IZOT_CONTROL_FLAG_* */
    uint8_t PersistentFlushGuardTimeout;  /* The number of seconds to wait after
                                       receiving an update that affects non-
                                       volatile configuration before starting
                                       to write the data out.  The internal
                                       timer gets reset to this guard-band as
                                       each new update comes in.  Valid range
                                       is 1 to 60 seconds. */

        /* Defines the communication parameters used by LON Stack */
    IZOT_STRUCT_BEGIN(CommParmeters) {
            /* Transceiver type. If the value is IzoTTransceiverTypeDefault,
               the comm. parameters in the LON transceiver are left unchanged.
               If the value is IzoTTransceiverTypeCustom, the comm. parameters
               are set based on the CommParms field.  Otherwise the comm.
               parameters are modified based on hard-coded comm. parameters
               for the specified type. */
        IzotTransceiverType TransceiverType;

            /* An array of 16 bytes representing the comm. parameters.  If
               TransceiverType is IzoTTransceiverTypeCustom, these values are
               written out to the LON transceiver.  Otherwise they are
               ignored. */
        unsigned char CommParms[IzoT_NUM_COMM_BYTES];
    } IZOT_STRUCT_END(CommParmeters);

        /* The number of and size of various buffers used by LON Stack.
           The buffer counts and sizes are considered a minimum recommendation.
           LON Stack implementations can chose to implement more
           or larger buffers provided that the minimum specifications provided
           here are met.
           Some implementations of LON Stack only implement
           some of the buffer types defined here. */
    IZOT_STRUCT_BEGIN(Buffers) {
        /* There are three categories of buffers used by LON Stack */
        IZOT_STRUCT_BEGIN(ApplicationBuffers) {
            uint8_t PriorityMsgOutCount;        /* Number of priority output
                                                   message buffers (1 to 100).
                                                   These are allocated on the
                                                   host processor.
                                                   Recommended default = 5. */
            uint8_t NonPriorityMsgOutCount;     /* Number of non-priority
                                                   output message buffers
                                                   (0 to 100).  These are
                                                   allocated on the host
                                                   processor.  Recommended
                                                   default = 5. */
            uint8_t MsgInCount;                 /* Number of input message
                                                   buffers (1 to 100).
                                                   Recommended default = 10. */
        } IZOT_STRUCT_END(ApplicationBuffers);

        /* These buffers are also allocated on the host and are used directly
           by the link-layer driver that communicates with the LON network
           interface.  At most, one such buffer is needed for output at any
           given time, but multiple input buffers might be required.  These
           buffers are shared for both input and output.  The size of the
           buffers is determined by the transceiver network buffer size.   */
        IZOT_STRUCT_BEGIN(LinkLayerBuffers) {
            uint8_t LinkLayerBufferCount;   /* Number of driver buffers (1 to
                                               100).  Recommended default = 2. */
        } IZOT_STRUCT_END(LinkLayerBuffers);

        /* These are the buffers defined on the LON network interface.  The
           application buffers on the network interface  are used only for
           local network management, and therefore the LON Stack does not 
           support modifying the application buffers.  The network buffers
           can be configured.  Because these are defined on the LON network
           interface, the counts and sizes must conform to those defined by
           the ISO/IEC 14908-1 standard, and the total memory requirement
           specified by the buffers must not exceed the capacity of the
           transceiver's firmware. The LON network interface has
           knowledge of the maximum buffer configuration, and prevents
           accidentally exceeding these constraints. */
        IZOT_STRUCT_BEGIN(TransceiverBuffers) {
            uint16_t NetworkBufferInputSize;      /* Must be at least 66 bytes.
                                                     Recommended default = 114. */
            uint16_t NetworkBufferOutputSize;     /* Must be at least 66 bytes.
                                                     Recommended default = 114. */
            uint8_t PriorityNetworkOutCount;      /* Number of priority
                                                     network output buffers.
                                                     Must be non-zero if
                                                     PriorityMsgOutCount is
                                                     non-zero.  Recommended
                                                     default = 2 */
            uint8_t NonPriorityNetworkOutCount;   /* Number of non-priority
                                                     network output buffers.
                                                     Must be non-zero if
                                                     NonPriorityMsgOutCount
                                                     is non-zero.  Default = 2. */
            uint8_t NetworkInCount;               /* Number of network input
                                                     buffers.  Must be non-
                                                     zero.  Recommended
                                                     default = 5. */
        } IZOT_STRUCT_END(TransceiverBuffers);
    } IZOT_STRUCT_END(Buffers);

    /* Maximum number of receive transaction records (1..200) */
    /* A receive transaction record is required for any incoming message
       which uses either unacknowledged repeat, acknowledged, or request
       service. No receive transaction entries are required for
       unacknowledged service. A receive transaction record is required for
       each unique source address/destination address/priority attribute.
       Each receive transaction entry contains a current transaction number.
       A message is considered to be a duplicate if its source address,
       destination address, and priority attribute vector into an existing
       receive transaction, and the message's transaction number matches the
       record�s transaction number.

       A receive transaction record is freed when a new message is received
       that vectors into an existing receive transaction record, if the
       message's transaction number *does not* match the transaction number
       in the receive transaction record.

       Receive transaction records are also freed after the receive timer
       expires. The receive timer duration is determined by the destination
       device and varies as a function of the message addressing mode. For
       group addressed messages, the receive timer is determined by the
       address table. For Unique ID addressed messages, the receive timer
       is fixed at eight seconds. For other addressing modes, the non-group
       receive timer in the configuration data structure is used.
     */
    uint16_t ReceiveTransCount;

    /* Maximum number of transmit transactions records (1..8192) */
    /* Transmit transaction records are allocated each time a message is
       sent using the acknowledged, unacknowledged repeated, or request
       service.  The record is keyed by the source address, destination
       address, and priority.  A transmit transaction record is used both
       to maintain information about current transactions and about the
       most recently completed transactions.  Among other things, the
       transaction record includes the 4-bit protocol transaction number.
       The receiving device maintains a corresponding receive
       transaction until either a new transaction with the same source
       address, destination address, and priority has been received, or
       the receive transaction times out.  Therefore, the transmit
       transaction should also be maintained until either a new
       transaction is initiated or the receive transaction times out.
       However, the sending application does not know how long the
       receive transaction time is.  Instead, the Izot protocol
       stack's transaction records are released when the
       TransmitTransIdLifetime timer expires (see below).

       If the device uses only implicit transactions, the maximum number
       of transaction records it could possibly need is 2 times the
       number of address tables that it has, because the transaction
       record is keyed by source/destination address AND priority.  If
       there are sufficient transmit transaction records, the
       TransmitTransIdLifetime can be very large (though it need never
       exceed the largest receive transaction timeout, which is 24576
       milliseconds).

       If explicit addressing is used, then the maximum number of
       transmit transactions is unknowable.  However, there is never a
       need for more transmit transaction records than there are possible
       transactions within the TransmitTransIdLifetime.  However, if we
       assume that the IzoT device can send 70 transactions per second,
       and the TransmitTransIdLifetime is set to the maximum, the
       TransmitTransCount would need to be on the order of 1750.  On the
       other hand, it is extremely unlikely that an all FT network would
       ever have such long receive transactions, and therefore the
       TransmitTransIdLifetime could be set much lower.  If there are
       other slower channels involved, it is unlikely that the IzoT
       device would be able to send 70 transactions per second.  But
       unfortunately, there could be configurations where some of the
       receiving devices have very long receive transactions and the IzoT
       device can send a lot of transactions per second.
     */
    uint16_t TransmitTransCount;

    /* Transmit transaction ID lifetime, in milliseconds.  Recommended
       default = 24576 (the maximum receive timer).

       This value indicates how long a transaction record should remain
       in use.  You should set it to the value of the largest receive
       transaction timer for all devices that the IzoT device will send
       acknowledged, unacknowledged repeat, or request messages to.
       Because the application is unlikely to know this value, the maximum
       possible receive timer is used as the recommended default.  This
       value works well if there are enough transaction table entries for
       all addressed messages, but if there are not, an application
       message send could fail because of a lack of transaction records.
     */
    uint32_t TransmitTransIdLifetime;
} IZOT_STRUCT_END(IzotControlData);

/*****************************************************************
 * Section: Synchronous Event Globals
 *****************************************************************/
/*
 *  This section defines the prototypes for the IzoT Device Stack API callback
 *  functions.
 *
 *  Remarks:
 *  Callback functions are called by the LON Stack
 *  immediately, as needed, and may be called from any LON task or thread.  The
 *  application *must not* call into the LON Stack API from within a
 *  synchronous event handler.
 */

/*
 *  Callback: IzotGetCurrentDatapointSize
 *  Gets the current size of a datapoint.
 *
 *  Parameters:
 *  index - the local index of the datapoint
 *
 *  Returns:
 *  Current size of the datapoint. Zero if the index is invalid.
 *
 *  Remarks:
 *  If the datapoint size is fixed, this function should return
 *  <IzotGetDeclaredDatapointSize> (when this API is available), or
 *  (unsigned)-1. If the datapoint size is changeable, the
 *  current size should be returned.
 *
 *  LON Stack will not propagate a datapoint with
 *  size 0, nor will it generate an update event if a datapoint update
 *  is received from the network when the current datapoint size is 0.
 *
 *  Even though this is a callback function, it *is* legal for the
 *  application to call <IzotGetDeclaredDatapointSize> from this callback.
 *
 *  Use <IzotGetCurrentDatapointSizeRegistrar> to register a handler for
 *  this event. Without an application-specific handler, the stack assumes
 *  that each datapoint's declared size always equals its current size.
 */
typedef unsigned (*IzotGetCurrentDatapointSizeFunction)(const signed index);

/*
 *  Direct memory file support events
 */

/*
 *  Callback: IzotMemoryRead
 *  Read memory in the LON device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be read
 *  size - number of bytes to read
 *  pData - pointer to a buffer to store the requested data
 *
 *  Remarks:
 *  The LON Stack calls <IzotMemoryRead> whenever it receives
 *  a network management memory read request that fits into the registered
 *  file access window. This callback function is used to read data starting
 *  at the specified virtual memory address, but located within the LON
 *  application. This event is generally used to provide access to property
 *  template and value files through standard memory read protocol messages.
 *
 *  Use <IzotRegisterMemoryWindow> to configure this service.
 *  Use <IzotMemoryReadRegistrar> to register a handler for this event.
 *  Without an application-specific handler for this event, the stack returns
 *  a failure response to the memory read request message.
 *
 *  Applications must generally implement both the IzotMemoryRead and
 *  IzotMemoryWrite events, or none.
 */
typedef LonStatusCode (*IzotMemoryReadFunction)(const unsigned address,
        const unsigned size, void* const pData);

/*
 *  Callback: IzotMemoryWrite
 *  Update memory in the LON device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be written
 *  size - number of bytes to write
 *  pData - pointer to the data to write
 *
 *  Remarks:
 *  LON Stack calls <IzotMemoryWrite> whenever it
 *  receives a network management memory write request that fits into the
 *  registered file access window. This callback function is used to write
 *  data at the specified virtual memory address, but located within the LON
 *  application. This event is generally used to provide access to property
 *  value files through standard memory write protocol messages.
 *
 *   LON Stack automatically calls the
 *  <IzotPersistentDataHasBeenUpdated> function to schedule an update
 *  whenever this callback returns <LonStatusCode>.
 *
 *  Use <IzotRegisterMemoryWindow> to configure this service.
 *  Use <IzotMemoryWriteRegistrar> to register a handler for this event.
 *  Without an application-specific handler for this event, the stack returns
 *  a failure response to the memory write request message.
 *
 *  Applications must generally implement both the IzotMemoryRead and
 *  IzotMemoryWrite events, or none.
 */
typedef LonStatusCode (*IzotMemoryWriteFunction)(const unsigned address,
        const unsigned size, const void* const pData);

/*
 *  Persistent data support events
 *
 *  Persistent data is variable data which must be kept safe across a device
 *  reset or a power cycle. LON Stack EX provides transparent
 *  persistent data management functions, but LON Stack DX, which generally
 *  supports resource-limited platforms, uses the events defined in this
 *  section. Applications implement these events to supply the low-level
 *  functionality, for example by storing the persistent data segments in
 *  a file system or a flash memory device.
 */

/*
 *  Callback: IzotStorageOpenSegForRead
 *  Open a persistent data segment for reading.
 *
 *  Parameters:
 *  persistent_seg_type - type of persistent data to be opened
 *
 *  Returns:
 *  <IzotPersistentSegType>.
 *
 *  Remarks:
 *  This function opens the data segment for reading.  When suitable storage
 *  exists and can be accessed, the function returns an application-specific
 *  non-zero handle. Otherwise it returns 0.
 *
 *  The handle returned by this function will be used as the first parameter
 *  when calling <IzotStorageReadSeg>.
 *
 *  The application must maintain the handle used for each segment. The
 *  application can invalidate a handle when <IzotStorageCloseSeg> is called
 *  for that handle.
 *
 *  Use <IzotFlashSegOpenForReadRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event always fails
 *  (the handle is always zero).
 */
typedef IzotPersistentSegType (*IzotPersistentSegOpenForReadFunction)(const IzotPersistentSegType persistent_seg_type);

/*
 *  Callback: IzotStorageOpenSegForWrite
 *  Open a persistent data segment for writing.
 *
 *  Parameters:
 *  persistent_seg_type - type of persistent data to be opened
 *  size - size of the data to be stored
 *
 *  Returns:
 *  <IzotPersistentSegType>.
 *
 *  Remarks:
 *  This function is called by LON Stack after changes have been made
 *  to data that is backed by persistent storage. Multiple updates may
 *  have been made.  This function is called only after LON Stack
 *  determines that all updates have been completed, based on a flush
 *  timeout.
 *
 *  This function opens the data segment for reading.  When suitable storage
 *  exists and can be accessed, the function returns an application-specific
 *  non-zero persistent segment type. Otherwise it returns 
 *  IzotPersistentSegUnassigned.
 *
 *  The persistent segment type returned by this function will be used as the
 *  first parameter when calling <IzotStorageWriteSeg>.
 *
 *  The application must maintain the persistent segment type used for each
 *  segment. The application can invalidate a segment type when 
 *  <IzotStorageCloseSeg> is called for that handle.
 *
 *  Use <IzotFlashSegOpenForWriteRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event always fails
 *  (the segment type is always IzotPersistentSegUnassigned).
 */
typedef IzotPersistentSegType (*IzotPersistentSegOpenForWriteFunction)(
        const IzotPersistentSegType persistent_seg_type, const size_t size);

/*
 *  Callback: IzotStorageCloseSeg
 *  Close a persistent data segment.
 *
 *  Parameters:
 *  persistent_seg_type - persistent segment type returned by <IzotStorageOpenSegForRead>
 *           or <IzotStorageOpenSegForWrite>
 *
 *  Remarks:
 *  This function closes the persistent memory segment associated with this
 *  persistent segment type and invalidates the segment type.
 *
 *  Use <IzotFlashSegCloseRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotPersistentSegCloseFunction)(
    const IzotPersistentSegType persistent_seg_type);

/*
 *  Callback: IzotStorageDeleteSeg
 *  Delete a persistent data segment.
 *
 *  Parameters:
 *  persistent_seg_type - type of persistent segment to be deleted
 *
 *  Remarks:
 *  This function is used to delete the persistent memory segment referenced
 *  by the segment type. The stack will only call this function on closed segments.
 *
 *  This function can be called even if the segment does not exist
 *  in persistent storage.
 *  It is not necessary for this function to actually destroy the data or
 *  return space allocated for this data to other applications, but the event
 *  handler must invalidate the persistent data associated with the given
 *  <IzotPersistentSegType> such that a subsequent attempt to read the
 *  data fails.
 *
 *  Use <IzotFlashSegDeleteRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotPersistentSegDeleteFunction)(
    const IzotPersistentSegType persistent_seg_type);

/*
 *  Callback: IzotStorageReadSeg
 *  Read a section of a persistent data segment.
 *
 *  Parameters:
 *  persistent_seg_type - persistent segment type (See <IzotStorageOpenSegForRead>)
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by LON Stack during initialization to
 *  read data from persistent storage. An error value is returned if the
 *  specified persistent segment type does not exist.
 *
 *  The offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 *
 *  Use <IzotFlashSegReadRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event always fails (no
 *  data available to read).
 */
typedef LonStatusCode (*IzotPersistentSegReadFunction)(const IzotPersistentSegType persistent_seg_type,
    const size_t offset, const size_t size, IzotByte* const pBuffer);


/*
 *  Callback: IzotStorageWriteSeg
 *  Write a section of a persistent data segment.
 *
 *  Parameters:
 *  persistent_seg_type - persistent segment type (See <IzotStorageOpenSegForWrite>)
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pData - pointer to the data to write into the segment
 *
 *  Remarks:
 *  This function is called by LON Stack after changes have been
 *  made to data that is backed by persistent storage. Multiple updates
 *  may have been made.  This function is called only after LON Stack
 *  determines that all updates have been completed.
 *
 *  The offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 *
 *  Use <IzotFlashSegWriteRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event always fails.
 */
typedef LonStatusCode (*IzotPersistentSegWriteFunction) (const IzotPersistentSegType persistent_seg_type,
    const size_t offset, const size_t size, const IzotByte* const pData);

/*
 *  Callback: IzotStorageSegIsInvalid
 *  Returns TRUE if a persistent data transaction was in progress last time
 *  the device shut down.
 *
 *  Parameters:
 *  persistent_seg_type - persistent segment type
 *
 *  Remarks:
 *  This function is called by the LON stack during initialization
 *  prior to reading the segment data. This callback must return TRUE if a
 *  persistent data transaction had been started and never committed.
 *  When this event returns TRUE, the LON stack discards the segment,
 *  otherwise, the LON stack will attempt to read and apply the persistent
 *  data.
 *
 *  Use <IzotFlashSegIsInvalidRegistrar> to register a handler for
 *  this event. Without an application-specific handler, this event always
 *  returns TRUE.
 */
typedef IzotBool (*IzotPersistentSegIsInvalidFunction)(const IzotPersistentSegType persistent_seg_type);

/*
 *  Callback: IzotStorageStartSegUpdate
 *  Initiate a persistent transaction.
 *
 *  Parameters:
 *  persistent_seg_type - persistent segment type
 *
 *  Remarks:
 *  This function is called by LON Stack when the first request
 *  arrives to update data stored in the specified segment. The event handler
 *  updates the persistent data control structures to indicate that a
 *  transaction is in progress. The stack schedules writes to update the
 *  persistent storage at a later time, then invokes the
 *  <IzotStorageFinishSegUpdate> event.
 * 
 *  When a fatal error occurs during a persistent data storage transaction,
 *  such as a power outage, the transaction control data is used to prevent
 *  the application of invalid persistent data at the time of the next start.
 *
 *  Use <IzotFlashSegEnterTransactionRegistrar> to register a handler for
 *  this event. Without an application-specific handler, this event always
 *  fails.
 */
typedef LonStatusCode (*IzotPersistentSegEnterTransactionFunction)(const IzotPersistentSegType persistent_seg_type);

/*
 *  Callback: IzotStorageFinishSegUpdate
 *  Complete a persistent transaction.
 *
 *  Parameters:
 *  persistent_seg_type - persistent segment type
 *
 *  Remarks:
 *  This function is called by LON Stack after <IzotStorageWriteSeg>
 *  has returned success and there are no further updates required.
 *  See <IzotPersistentSegEnterTransactionFunction> for the complementary event.
 *
 *  Use <IzotFlashSegExitTransactionRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event always fails.
 */
typedef LonStatusCode (*IzotPersistentSegExitTransactionFunction)(const IzotPersistentSegType persistent_seg_type);

/*
 *  Callback: IzotPersistentGetApplicationSegmentSize
 *  Returns the number of bytes required to store the application persistent
 *  data segment.
 *
 *  Remarks:
 *  This segment includes all persistent datapoints and properties. Applications
 *  may also store additional, application-specific, data in this segment.
 *
 *  Use <IzotPersistentGetApplicationSegmentSizeRegistrar> to register a handler
 *  for this event. Without an application-specific handler, the application's
 *  persistent data segment is assumed to be empty (zero bytes long).
 */
typedef unsigned (*IzotPersistentSegGetAppSizeFunction)(void);

/*
 *  Callback: IzotPersistentDeserializeSegment
 *  Update the affected application control structures from a serialized image.
 *
 *  Parameters:
 *  pData - pointer to the serialized image
 *  size - size of the data, in bytes
 *
 *  Remarks:
 *  This event is used to transfer the serialized image of the application's
 *  persistent data segment to the application's corresponding variables, such
 *  as persistent datapoint values of property files.
 *
 *  Use <IzotPersistentDeserializeSegmentRegistrar> to register a handler for
 *  this event. Without an application-specific handler, this event always
 *  fails.
 */
typedef LonStatusCode (*IzotPersistentSegDeserializeFunction) (
    const void * const pData, const size_t size);

/*
 *  Callback: IzotPersistentSerializeSegment
 *  Return a serialized image of the specified segment.
 *
 *  Parameters:
 *  pData - pointer to store the serialized image
 *  size - size of the buffer, in bytes
 *
 *  Remarks:
 *  This event is used to gather the application's persistent data from
 *  persistent datapoints, property files and other variables into one
 *  persistent data segment.
 *
 *  Use <IzotPersistentSerializeSegmentRegistrar> to register a handler for
 *  this event. Without an application-specific handler, this event always
 *  fails.
 */
typedef LonStatusCode (*IzotPersistentSegSerializeFunction)(void * const pData, const size_t size);

/*****************************************************************
 * Section: Asynchronous Event Globals
 *****************************************************************/
/*
 *  This section defines the prototypes for the LON Stack API event handler
 *  functions.
 *
 *  Remarks:
 *  Like callback functions, event handlers are called from LON Stack.
 *  However, unlike callback functions, event handlers are only called in the
 *  context of the application task, and only when the application calls the
 *  <IzotEventPump> function. Also, the application may make LON Stack
 *  function calls from within handlers to synchronous events.
 */

/*
 *  Event: IzotReset
 *  Occurs when the IzoT protocol stack has been reset.
 *
 *  Parameters:
 *  None
 *
 *  Remarks:
 *  Whenever the LON device has been reset, the mode of the device
 *  is changed to *online*, but no IzotOnline() event is generated.
 *
 *  Resetting the LON device only affects LON Stack and does not
 *  cause a processor or application software reset.
 *
 *  Use <IzotResetRegistrar> to register a handler for this event.
 *  Without an application-specific reset event handler, this event
 *  does nothing.
 */
typedef void (*IzotResetFunction)(void);

/*
 *  Event: IzotWink
 *  Occurs when the LON device receives a Wink command.
 *
 *  Remarks:
 *  This event is not triggered when Wink sub-commands (extended install
 *  commands) are received.
 *  LON device applications can produce a non-harmful audible or
 *  visual response to a Wink event. For example, a device might flash a
 *  dedicated LED for 20s. Wink events are used to identify individual devices
 *  in a larger installation.
 *
 *  Use <IzotWinkRegistrar> to register a handler for this event. Without an
 *  application-specific wink event handler, this event does nothing.
 */
typedef void (*IzotWinkFunction)(void);

/*
 *  Event: IzotOffline
 *  Occurs when the IzoT device has entered the offline state.
 *
 *  Remarks:
 *  While the device is offline, LON Stack will not
 *  generate datapoint updates, and will return an error when
 *  <IzotPropagate> is called.
 *
 *  Use <IzotOfflineRegistrar> to register a handler for this event. Without
 *  an application-specific offline event handler, this event does nothing.
 */
typedef void (*IzotOfflineFunction)(void);

/*
 *  Event: IzotOnline
 *  Occurs when the IzoT device has entered the online state.
 *
 *  Remarks:
 *  The online event is fired only when an explict mode change to online
 *  occurs. Following a reset event, a configured device is also considered
 *  online.
 *
 *  Use <IzotOnlineRegistrat> to register a handler for this event.
 *  Without an application-specific online event handler, this event does
 *  nothing.
 */
typedef void (*IzotOnlineFunction)(void);

/*
 *  Event: IzotDatapointUpdateOccurred
 *  Occurs when new input datapoint data has arrived.
 *
 *  Parameters:
 *  index - global index (local to the device) of the datapoint in question
 *  pSourceAddress - pointer to source address description
 *
 *  Remarks:
 *  The datapoint with local index given in this event handler has been
 *  updated with a new value. The new value is already stored in the datapoint.
 *  Access the value through the variable representing the datapoint, or obtain
 *  the pointer to the datapoint's value from the <IzotGetDatapointValue>
 *  function. The pSourceAddress pointer is only valid for the duration of this
 *  event handler.
 *
 *  For an element of a datapoint array, the index is the global datapoint
 *  index plus the array-element index. For example, if nviVolt[0] has
 *  global datapoint index 4, then nviVolt[1] has global datapoint
 *  index 5.
 *
 *  Use <IzotDatapointUpdateOccurredRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotDatapointUpdateOccurredFunction)(
    const unsigned index,
    const IzotReceiveAddress* const pSourceAddress);

/*
 *  Event: IzotDatapointUpdateCompleted
 *  Signals completion of a datapoint update.
 *
 *  Parameters:
 *  index - global index (local to the device) of the datapoint that was updated
 *  success - indicates whether the update was successful or unsuccessful
 *
 *  Remarks:
 *  This event handler signals the completion of a datapoint update or
 *  poll transaction (see <IzotPropagate> and <IzotPoll>, respectively). For
 *  unacknowledged or repeated messages, the transaction is complete when the
 *  message has been sent with the configured number of retries. For
 *  acknowledged messages, it is successfully complete when the IzoT
 *  protocol stack receives an acknowledgement from each of the destination
 *  devices, and is unsuccessfully complete if not all acknowledgements are
 *  received.  Poll requests always use the request service type, and
 *  generates a successful completion if responses are received from all
 *  expected devices, and generates a failed completion otherwise.
 *
 *  Use <IzotDatapointUpdateCompletedRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotDatapointUpdateCompletedFunction)(
    const unsigned index,
    const IzotBool success);

/*
 *  Event: IzotMsgArrived
 *  Occurs when an application message arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <IzotReceiveAddress>)
 *  correlator - correlator to be used with <IzotSendResponse>
 *  authenticated - TRUE if the message was successfully authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Remarks:
 *  This event handler reports the arrival of a message that is neither a
 *  datapoint message or a non-Datapoint message that is otherwise processed
 *  by the IzoT device stack (such as a network management command).
 *  Typically, this is used with application message codes in the value range
 *  indicated by the <IzotApplicationMessageCode> enumeration.
 *  All pointers are only valid for the duration of this event handler.
 *
 *  If the message is a request message, then the function must deliver a
 *  response using <IzotSendResponse> passing the provided *correlator*.
 *
 *  Application messages are always delivered to the application, regardless
 *  of whether the message passed authentication or not. It is up to the
 *  application to decide whether authentication is required for any given
 *  message and compare that fact with the authenticated flag. The
 *  authenticated flag is clear (FALSE) for non-authenticated messages and for
 *  authenticated messages that do not pass authentication. The authenticated
 *  flag is set only for correctly authenticated messages.
 *
 *  Use <IzotMsgArrivedRegistrar> to register a handler for this event
 */
typedef void (*IzotMsgArrivedFunction)(
    const IzotReceiveAddress* const pAddress,
    const IzotCorrelator correlator,
    const IzotBool priority,
    const IzotServiceType serviceType,
    const IzotBool authenticated,
    const IzotByte code,
    const IzotByte* const pData,
    const unsigned dataLength);

/*
 *  Event: IzotResponseArrived
 *  Occurs when a response arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address used for response (see
 *             <IzotResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *          dataLength - number of bytes available through pData.
 *
 *  Remarks:
 *  This event handler is called when a response arrives.  Responses may be
 *  sent by other devices when the IzoT device sends a message using
 *  <IzotSendMsg> with a <IzotServiceType> of *IzotServiceRequest*.
 *
 *  Use <IzotResponseArrivedRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotResponseArrivedFunction)(
    const IzotResponseAddress* const pAddress,
    const unsigned tag,
    const IzotByte code,
    const IzotByte* const pData,
    const unsigned dataLength);

/*
 *  Event: IzotMsgCompleted
 *  Occurs when a message transaction has completed.  See <IzotSendMsg>.
 *
 *  Parameters:
 *  tag - used to correlate the event with the message sent
 *        Same as the *tag* specified in the call to <IzotSendMsg>
 *  success - TRUE for successful completion, otherwise FALSE
 *
 *  Remarks:
 *  For unacknowledged or repeated messages, the transaction is complete when
 *  the message has been sent with the configured number of retries. For
 *  acknowledged messages, LON Stack calls
 *  <IzotMsgCompleted> with *success* set to TRUE after receiving
 *  acknowledgments from all of the destination devices, and calls
 *  <IzotMsgCompleted> with *success* set to FALSE if the transaction timeout
 *  period expires before receiving acknowledgements  from all destinations.
 *  Likewise, for request messages, the transaction is considered successful
 *  when LON Stack receives a response from each of the
 *  destination devices, and unsuccessful if the transaction timeout expires
 *  before responses have been received from all destination devices.
 *
 *  Use <IzotMsgCompletedRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotMsgCompletedFunction)(
    const unsigned tag,
    const IzotBool success);

/*
 *  Event: IzotServiceLedStatus
 *  Occurs when the service LED changes state.
 *
 *  Parameters:
 *  state - indicates the state of the service LED with one of the values
 *          defined in the IzotServiceLedState enumeration.
 *
 *  Remarks:
 *  Applications can provide a Service LED or other method of providing
 *  visual feedback. For example, a designated area on an LCD display may
 *  be used to implement the "service LED" functionality.
 *
 *  The Service LED can signal the configured state (LED off),
 *  unconfigured state (LED flashing at 0.5 Hz) or application-less state
 *  (LED solid on). Applications may override these states with additional
 *  information, for example when implementing the ISI Connect LED
 *  functionality with the same physical hardware.
 *
 *  Use <IzotServiceLedStatusRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotServiceLedStatusFunction)(
    IzotServiceLedState state, IzotServiceLedPhysicalState physicalState);

/*
 *  Event: IzotFilterMsgArrived
 *  Occurs when an application message arrives.
 *  This API is available with LON Stack EX.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <IzotReceiveAddress>)
 *  correlator - correlator to be used with <IzotSendResponse>
 *  authenticated - TRUE if the message was (successfully) authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Returns:
 *  TRUE if the message has been processed by the filter event handler.
 *
 *  Remarks:
 *  This event handler filters the arrival of a message.  Typically this
 *  is used by the ISI engine to filter ISI messages.  If the message
 *  does not get processed by the filter handler, the message will be
 *  passed to the <IzotMsgArrived> handler.
 *
 *  Use <IzotFilterMsgArrivedRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing (no
 *  incoming messages are filtered, all are permitted to the application).
 */
typedef IzotBool (*IzotFilterMsgArrivedFunction)(
        const IzotReceiveAddress* const pAddress,
        const IzotCorrelator correlator,
        const IzotBool priority,
        const IzotServiceType serviceType,
        const IzotBool authenticated,
        const IzotByte code,
        const IzotByte* const pData,
        const unsigned dataLength);

/*
 *  Event: IzotFilterResponseArrived
 *  Occurs when a response arrives.
 *  This API is available with LON Stack EX.
 *
 *  pAddress - source and destination address used for response (see
 *             <IzotResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *  dataLength - number of bytes available through pData.
 *
 *  Returns:
 *  TRUE if the response has been processed by the filter event handler.
 *
 *  Remarks:
 *  This event handler filters incoming response messages. Responses may be
 *  sent by other devices when the IzoT device sends a message using
 *  <IzotSendMsg> with a <IzotServiceType> of *IzotServiceRequest*.
 *
 *  Use <IzotFilterResponseArrivedRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing
 *  (no incoming response is filtered, all are permitted to the application).
 */
typedef IzotBool(*IzotFilterResponseArrivedFunction)(
        const IzotResponseAddress* const pAddress,
        const unsigned tag,
        const IzotByte code,
        const IzotByte* const pData,
        const unsigned dataLength);

/*
 *  Event: IzotFilterMsgCompleted
 *  Occurs when a message transaction has completed.  See <IzotSendMsg>.
 *  This API is available with LON Stack EX.
 *
 *  Parameters:
 *  tag - used to correlate the event with the message sent
 *        Same as the *tag* specified in the call to <IzotSendMsg>
 *  success - TRUE for successful completion, otherwise FALSE
 *
 *  Returns:
 *  TRUE if the completion event has been processed by the filter event
 *  handler.
 *
 *  Remarks:
 *  This event handler filters the completion event of a message.
 *  Typically this is used by the ISI engine to filter the completion
 *  notification of ISI messages.  If the completion event does not get
 *  processed by the filter handler, the message will be passed to the
 *  <IzotMsgCompleted> handler.
 *
 *  Use <IzotFilterMsgCompletedRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing
 *  (no completion event is filtered, all are permitted to the application).
 */
typedef IzotBool(*IzotFilterMsgCompletedFunction)(const unsigned tag, const IzotBool success);

typedef void (*IzotisiTickFunction)(void);

/*****************************************************************
 * Section: LON Stack Shared Data Structures
 *****************************************************************/

// Queue-entry structure
typedef struct {
    char *queueName;            // Name of the queue (for debugging)
    IzotUbits16 queueCapacity;  // Max number of entries in queue
    IzotUbits16 queueEntries;   // Number of entries currently in queue
    IzotUbits16 entrySize;      // Number of bytes for each entry in queue
    IzotBool emptyCountReports; // Number of times the empty condition has been reported
    IzotByte *head;             // Pointer to the head entry of the queue
    int headIndex;              // Index of the head entry in the data array for debugging
    IzotByte *tail;             // Pointer to the tail entry of the queue
    int tailIndex;              // Index of the tail entry in the data array for debugging
    IzotByte *data;             // Array of entries -- allocated during initialization
} Queue;

// LON timer structure
typedef struct __attribute__ ((packed)) {
	uint32_t expiration;		// Time to expire
	uint32_t repeatTimeout;     // Repeat timeout on expiration (0 means not repeating)
} LonTimer;

// Stopwatch structure
typedef struct {
	uint32_t start;			    // Time stopwatch started
} LonWatch;

#endif /* _LON_TYPES_H */
