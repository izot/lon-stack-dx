//
// IzotTypes.h
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

/*
 * Title: IzoT Device Stack DX API Reference
 *
 * Abstract:
 * This file declares the enumerations and data types for the LON DX Stack API.
 */


#ifndef _IZOT_TYPES_H
#define _IZOT_TYPES_H

#ifndef _IZOT_PLATFORM_H
#   error You must include IzotPlatform.h first
#endif  /* _IZOT_PLATFORM_H */

#include "IapTypes.h"       // IAP type definitions


/*
 * *****************************************************************************
 * SECTION: ERROR CODES
 * *****************************************************************************
 *
 * This section details the enumerations for error codes that this API can
 * issue.  This section also lists the errors reported by the device in
 * its error log (see <IzotStatus>).
 */

/*
 * Enumeration: IzotSystemError
 * System and Izot protocol stack error codes logged in the IzoT
 * protocol stack's error log.
 *
 * The codes can be accessed using the Query Status standard network management
 * command.
 *
 * The standard system errors range above value 128. Values between 1 and 128
 * are application-specific (but serious) errors. Values used by the IzoT
 * protocol stack are also included in the <IzotSystemError> enumeration.
 *
 * The standard system error codes are discussed in the Smart Transceiver
 * literature and in the CEA/EIA 709.1-B protocol specification.
 *
 */
typedef IZOT_ENUM_BEGIN(IzotSystemError)
{
    /*   0 */ IzotNoError                 = 0,
    /*
     * IzoT specific system error codes
     */

     /* none */

    /*
     * Standard system error codes
     */
    /* 129 */ IzotBadEvent            = 129u,
    /* 130 */ IzotDatapointLengthMismatch    = 130u,
    /* 131 */ IzotDatapointMsgTooShort       = 131u,
    /* 132 */ IzotEepromWriteFail     = 132u,
    /* 133 */ IzotBadAddressType      = 133u,
    /* 134 */ IzotPreemptionModeTimeout = 134u,
    /* 135 */ IzotAlreadyPreempted    = 135u,
    /* 136 */ IzotSyncDatapointUpdateLost    = 136u,
    /* 137 */ IzotInvalidRespAlloc    = 137u,
    /* 138 */ IzotInvalidDomain       = 138u,
    /* 139 */ IzotReadPastEndOfMsg    = 139u,
    /* 140 */ IzotWritePastEndOfMsg   = 140u,
    /* 141 */ IzotInvalidAddrTableIndex = 141u,
    /* 142 */ IzotIncompleteMsg       = 142u,
    /* 143 */ IzotDatapointUpdateOnOutput  = 143u,
    /* 144 */ IzotNoMsgAvail          = 144u,
    /* 145 */ IzotIllegalSend         = 145u,
    /* 146 */ IzotUnknownPdu          = 146u,
    /* 147 */ IzotInvalidDatapointIndex      = 147u,
    /* 148 */ IzotDivideByZero        = 148u,
    /* 149 */ IzotInvalidApplError    = 149u,
    /* 150 */ IzotMemoryAllocFailure  = 150u,
    /* 151 */ IzotWritePastEndOfNetBuffer = 151u,
    /* 152 */ IzotApplCheckSumError   = 152u,
    /* 153 */ IzotCnfgCheckSumError   = 153u,
    /* 154 */ IzotInvalidXcvrRegAddr  = 154u,
    /* 155 */ IzotXcvrRegTimeout      = 155u,
    /* 156 */ IzotWritePastEndOfApplBuffer = 156u,
    /* 157 */ IzotIoReady             = 157u,
    /* 158 */ IzotSelfTestFailed      = 158u,
    /* 159 */ IzotSubnetRouter        = 159u,
    /* 160 */ IzotAuthenticationMismatch  = 160u,
    /* 161 */ IzotSeltInstSemaphoreSet    = 161u,
    /* 162 */ IzotReadWriteSemaphoreSet   = 162u,
    /* 163 */ IzotApplSignatureBad    = 163u,
    /* 164 */ IzotRouterFirmwareVersionMismatch = 164
} IZOT_ENUM_END(IzotSystemError);

/*
 * Enumeration: IzotApiError
 * IzoT 1.0 API error codes.
 *
 * This enumeration contains all IzoT 1.0 API error codes, including the
 * code for success _IzotApiNoError_. Use the <IZOT_SUCCESS> macro to
 * determine successful completion of an API function.
 *
 * Some implementations may support only a subset of the error codes listed
 * here.
 */
typedef IZOT_ENUM_BEGIN(IzotApiError)
{
    /*    0    */ IzotApiNoError = 0,         /* no error. Use the IZOT_SUCCESS macro to test for this condition */
    /*
     * API errors related to datapoints
     */
    /*    1    */ IzotApiDatapointIndexInvalid =1,   /* invalid Datapoint index */
    /*    2    */ IzotApiDatapointLengthMismatch,    /* assumed length is not equal to actual length */
    /*    3    */ IzotApiDatapointLengthTooLong,     /* Datapoint data is too long */
    /*    4    */ IzotApiDatapointPollNotPolledDatapoint,   /* polling input Datapoint requires declaration of polled attribute in the model file */
    /*    5    */ IzotApiDatapointPollOutputDatapoint,      /* cannot poll output datapoint */
    /*    6    */ IzotApiDatapointPropagateInputDatapoint,  /* cannot propagate input datapoint */
    /*    7    */ IzotApiDatapointPropagatePolledDatapoint, /* cannot propagate a polled datapoint */

    /*
     * API errors related to application messages
     */
    /*    11   */ IzotApiMsgExplicitAddrMissing = 11, /* explicit destination required but missing */
    /*    12   */ IzotApiMsgInvalidMsgTag,    /* invalid message tag provided */
    /*    13   */ IzotApiMsgLengthTooLong,    /* message data exceeds limits */
    /*    14   */ IzotApiMsgNotRequest,       /* message should be sent as a request */
    /*    15   */ IzotApiMsgInvalidCode,      /* invalid message code */
    /*    16   */ IzotApiMsgInvalidCorrelator, /* Invalid <IzotCorrelator> */
    /*    17   */ IzotApiMsgInvalidAddress,   /* Invalid address */
    /*
     * API errors related to the ShortStack serial or IzoT parallel driver
     */
    /*    31   */ IzotApiTxBufIsFull              = 31,   /* no transmit (downlink) buffer available */
    /*    32   */ IzotApiRxMsgNotAvailable,               /* no message has been received from the Micro Server */
    /*    33   */ IzotApiMicroServerUnresponsive,         /* the Micro Server is not responding to RTS */
    /*
     * General API errors
     */
    /*    41   */ IzotApiVersionNotAvailable    = 41, /* Link-layer protocol version information unavailable */
    /*    42   */ IzotApiNeuronIdNotAvailable,    /* Unique ID (Neuron ID) unavailable */
    /*    43   */ IzotApiInitializationFailure,   /* Initialization failed */
    /*    44   */ IzotApiIndexInvalid,            /* invalid index (for Datapoint indices, see IzotApiDatapointIndexInvalid) */
    /*    45   */ IzotApiMessageNotAvailable,     /* invalid index (for Datapoint indices, see IzotApiDatapointIndexInvalid) */
    /*    46   */ IzotApiNotInitialized,          /* API is not currently initialized.  Call <IzotInit>. */
    /*    47   */ IzotApiVersionNotSupported,     /* Structure version not supported. */
    /*    48   */ IzotApiNotAllowed,              /* Operation not allowed */
    /*    49   */ IzotApiInvalidParameter,        /* Invalid parameter specified */
    /*    50   */ IzotApiOffline,                 /* Operation not allowed while device is offline. */
    /*    51   */ IzotApiCallbackNotRegistered,   /* Callback function has not been registered. */
    /*    52   */ IzotApiCallbackExceptionError,  /* An exception when executing the callback function.  */
    /*
     * Errors related to management of persistent data. Returned by persistent data callback functions.
     */
    /*    71   */ IzotApiInvalidSegmentType   = 71, /* Not a supported persistent segment type. */
    /*    72   */ IzotApiPersistentFailure,       /* Generic persistent data failure. */
    /*    73   */ IzotApiPersistentSizeNotSupported, /* Persistent data size is not supported. */
    /*    74   */ IzotApiPersistentFileError,     /* Persistent data access error. */
    /*
     * Direct Memory File (DMF) access errors.
     */
    /*    81   */ IzotApiDmfOutOfRange         = 81, /* DMF address + count is out of range for operation. */
    /*    82   */ IzotApiDmfReadOnly           = 82, /* Write to read-only DMF area. */
    /*    83   */ IzotApiDmfNoDriver           = 83, /* No DMF driver defined. */

    /*    90   */ IzotApiNoNetworkInterface     = 90, /* No Network Interface defined. See IzotGetMyNetworkInterface in IzoTHandlers.c */
    /*    91   */ IzotApiNoIpAddress            = 91,  /* No IP address defined.  See IzotGetMyIpAddress in IzoTHandlers.c. */
    /*    92   */ IzotUnknownLTSDeviceType      = 92,  /* Unknown LTS device type */
    /*    93   */ IzotInvalidDeviceURI          = 93  /* Unknown LTS device URI */

} IZOT_ENUM_END(IzotApiError);

/*
 * Macro: IZOT_SUCCESS
 * Use the IZOT_SUCCESS macro to convert a <IzotApiError> code into a boolean
 * success or failure indicator.
 */
#define IZOT_SUCCESS(n)   ((n) == IzotApiNoError)

/*
 * ******************************************************************************
 * SECTION: GENERAL ENUMERATIONS AND TYPES
 * ******************************************************************************
 *
 * This section contains the enumerations and data types used with the IzoT
 * 1.0 API
 */

/*
 *  Macros: IZOT_GET_UNSIGNED_WORD, IZOT_SET_UNSIGNED_WORD, and
 *  IZOT_SET_UNSIGNED_WORD_FROM_BYTES.  The first two convert an
 *  IzotWord into a 16-bit unsigned scalar and vice versa.  The third
 *  converts to IzotBytes to a 16-bit unsigned scalar.
 */
#define IZOT_GET_UNSIGNED_WORD(n)          (((uint16_t)((n).msb) << 8)+(uint16_t)((n).lsb))
#define IZOT_SET_UNSIGNED_WORD(n, v)       (n).msb = (IzotByte)((v)>>8); (n).lsb = (IzotByte)(v)
#define IZOT_SET_UNSIGNED_WORD_FROM_BYTES(n,b1,b2) (n).msb = (IzotByte)(b1); \
                                                    (n).lsb = (IzotByte)(b2)

/*
 *  Macros: IZOT_GET_SIGNED_WORD, IZOT_SET_SIGNED_WORD
 *  Converts a IzotWord into a signed 16 bit scalar and vice versa.
 */
#define IZOT_GET_SIGNED_WORD(n)            ((int16_t)IZOT_GET_UNSIGNED_WORD(n))
#define IZOT_SET_SIGNED_WORD(n, v)         IZOT_SET_UNSIGNED_WORD(n, v)

/*
 *  Macros: IZOT_GET_UNSIGNED_DOUBLEWORD, IZOT_SET_UNSIGNED_DOUBLEWORD
 *  Converts a IzotDoubleWord into an unsigned 32 bit integer and vice versa.
 */
#define IZOT_GET_UNSIGNED_DOUBLEWORD(n)    ((((uint32_t)IZOT_GET_UNSIGNED_WORD((n).msw)) << 16) \
                                          +(uint32_t)IZOT_GET_UNSIGNED_WORD((n).lsw))
#define IZOT_SET_UNSIGNED_DOUBLEWORD(n, v) IZOT_SET_UNSIGNED_WORD((n).msw, (uint16_t) ((v) >> 16)); \
                                          IZOT_SET_UNSIGNED_WORD((n).lsw, (uint16_t) (v))

/*
 *  Macros: IZOT_GET_SIGNED_DOUBLEWORD, IZOT_SET_SIGNED_DOUBLEWORD
 *  Converts a IzotDoubleWord into a signed 32 bit integer and vice versa.
 */
#define IZOT_GET_SIGNED_DOUBLEWORD(n)    ((int32_t)IZOT_GET_UNSIGNED_DOUBLEWORD(n))
#define IZOT_SET_SIGNED_DOUBLEWORD(n, v) IZOT_SET_UNSIGNED_DOUBLEWORD(n, v)

/*
 *  Macros: IZOT_GET_ATTRIBUTE, IZOT_SET_ATTRIBUTE
 *  Gets(sets) attributes from(to) a field by appropriately masking and shifting.
 */
#define IZOT_GET_ATTRIBUTE(var, n)           ((((var).n##_FIELD) & n##_MASK) >> n##_SHIFT)
#define IZOT_GET_ATTRIBUTE_P(var, n)         ((((var)->n##_FIELD) & n##_MASK) >> n##_SHIFT)
#define IZOT_SET_ATTRIBUTE(var, n, value)    ((var).n##_FIELD = (((var).n##_FIELD) & ~n##_MASK) | ((((value) << n##_SHIFT)) & n##_MASK))
#define IZOT_SET_ATTRIBUTE_P(var, n, value)  ((var)->n##_FIELD = (((var)->n##_FIELD) & ~n##_MASK) | ((((value) << n##_SHIFT)) & n##_MASK))

/*
 *  Macro: IZOT_DOMAIN_ID_MAX_LENGTH
 *  Maximum length of the domain identifier, in bytes.
 *
 *  The domain identifier can be zero, one, three, or
 *  IZOT_DOMAIN_ID_MAX_LENGTH (6) bytes long.  Space for the largest possible
 *  identifier is allocated in various structures and message types. See
 *  <IzotDomain> for the domain table structure.
 */
#define IZOT_DOMAIN_ID_MAX_LENGTH    6

/*
 *  Macro: IZOT_AUTHENTICATION_KEY_LENGTH
 *  Length of the authentication key, stored in the domain table (<IzotDomain>).
 */
#define IZOT_AUTHENTICATION_KEY_LENGTH   6

/*
 *  Macro: IZOT_PROGRAM_ID_LENGTH
 *  Length of the application's program identifier, in bytes.
 */
#define IZOT_PROGRAM_ID_LENGTH   8

/*
 *  Macro: IZOT_LOCATION_LENGTH
 *  Length of the location identifier, in bytes.
 */
#define IZOT_LOCATION_LENGTH     6

/*
 *  Macro: IZOT_UNIQUE_ID_LENGTH
 *  Length of the node's unique identifier, in bytes.
 *
 *  The Unique ID is also known as the Neuron ID, referring to the
 *  Neuron Chip's or Smart Transceiver's unique ID.
 */
#define IZOT_UNIQUE_ID_LENGTH    6

/*
 *  Macro: IZOT_COMMUNICATIONS_PARAMETER_LENGTH
 *  Number of communication control bytes.
 */
#define IZOT_COMMUNICATIONS_PARAMETER_LENGTH     7

/*
 *  Typedef: IzotTransceiverParameters
 *  Parameters for single-ended and special-purpose mode transceivers.
 *
 *  See <IzotDirectModeTransceiver> for direct-mode transceiver parameters.
*/
typedef IzotByte IzotTransceiverParameters[IZOT_COMMUNICATIONS_PARAMETER_LENGTH];

/*
 *  Typedef: IzotUniqueId
 *  Holds the unique ID.
 */
typedef IzotByte IzotUniqueId[IZOT_UNIQUE_ID_LENGTH];

/*
 *  Typedef: IzotProgramId
 *  Holds the program ID.
 */
typedef IzotByte IzotProgramId[IZOT_PROGRAM_ID_LENGTH];

/*
 *  Typedef: IzotDomainId
 *  Holds a single domain identifier.
 */
typedef IzotByte IzotDomainId[IZOT_DOMAIN_ID_MAX_LENGTH];

/*
 *  Typedef: IzotAuthenticationKey
 *  Holds a single authentication key.
 */
typedef IzotByte IzotAuthenticationKey[IZOT_AUTHENTICATION_KEY_LENGTH];

/*
 *  Typedef: IzotLocationId
 *  Holds a single location identifier.
 *
 *  The location identifier is often referred to as the "location string".
 *  Note that this is misleading, because this data is not limited to ASCII
 *  characters or other similar requirements that could be inferred from the
 *  word "string".
 */
typedef IzotByte IzotLocationId[IZOT_LOCATION_LENGTH];

/*
 *  Typedef: IzotSubnetId
 *  Holds a subnet identifier.
 */
typedef IzotByte IzotSubnetId;

/*
 *  Typedef: IzotGroupId
 *  Holds a group identifier.
 */
typedef IzotByte IzotGroupId;

/*
 *  Typedef: IzotNodeId
 *  Holds a node identifier.
 */
typedef IzotByte IzotNodeId;

/*
 *  Enumeration: IzotNeuronModel
 *  Neuron Chip and Smart Transceiver model codes.
 *
 *  This enumeration lists all model codes for Neuron Chips or Smart
 *  Transceivers.
 */
typedef IZOT_ENUM_BEGIN(IzotNeuronModel)
{
    /*  0 */ IzotNeuron3150Code    =  0,  /* 3150, FT 3150 */
    /*  1 */ IzotNeuronPl3150Code  =  1,  /* PL 3150 */
    /*  2 */ IzotNeuron3150LCode   =  2,  /* CY7C53150L */
    /*  8 */ IzotNeuron3120Code    =  8,  /* Legacy 3120 */
    /*  9 */ IzotNeuron3120E1Code  =  9,  /* 3120E1, TMPN3120FE1M */
    /* 10 */ IzotNeuron3120E2Code  = 10,  /* 3120E2 */
    /* 11 */ IzotNeuron3120E3Code  = 11,  /* 3120E3, TMPN3120FE3M */
    /* 12 */ IzotNeuron3120A20Code = 12,  /* 3120A20 */
    /* 13 */ IzotNeuron3120E5Code  = 13,  /* 3120E5, TMPN3120FE5M */
    /* 14 */ IzotNeuron3120E4Code  = 14,  /* CY7C53120E4, FT 3120-E4 */
    /* 15 */ IzotNeuronPl3120E4Code= 15,  /* PL 3120-E4 */
    /* 16 */ IzotNeuron3120L8Code  = 16,  /* CY7C53120L8 */
    /* 17 */ IzotNeuronPl3170Code  = 17,  /* PL 3170 */
    /* 32 */ IzotNeuronFT5000Code  = 32,  /* FT 5000 */
    /* 33 */ IzotNeuron5000Code    = 33,  /* Neuron 5000 */
    /* 36 */ IzotNeuronFT6050Code  = 36,  /* FT 6050 */
    /* 37 */ IzotNeuron6050Code    = 37,  /* Neuron 6050 */
    /* 38 */ IzotNeuronFT6010Code  = 38,  /* FT 6010 */
    /* 39 */ IzotNeuron6010Code    = 39,  /* Neuron 6010 */
    /*112 */ IzotNeuronSLBCode     = 112, /* Streetlight Bridge */
    /*114 */ IzotNeuronIzotCode    = 114, /* Izot Device Stack based device */
    /*128 */ IzotNeuronGenericCode = 128  /* Generic model code */
} IZOT_ENUM_END(IzotNeuronModel);

/*
 *  Enumeration: IzotNeuronState
 *  Neuron Chip and Smart Transceiver state values.
 *
 *  This enumeration contains the values of the Neuron Chip's or Smart
 *  Transceiver's state information.  To convert the state value returned by
 *  the Query Status command to one of these values, use the <IZOT_NODE_STATE>
 *  macro. To test for the offline flag, use the <IZOT_NODE_STATE_OFFLINE> macro.
 */

#define IZOT_OFFLINE_BIT 0x08
#define IZOT_NODE_STATE_MASK 0x07

/*
 *  Macro: IZOT_NEURON_STATE
 *  Obtain persistent Neuron state information.
 *
 *  Use this macro to obtain the 3-bit Neuron state that is stored in EEPROM.
 */
#define IZOT_NEURON_STATE(state) ((state) & IZOT_NODE_STATE_MASK)

/*
 *  Macro: IZOT_NODE_STATE
 *  Extracts state information from <IzotNeuronState>.
 *
 *  Use this macro to decipher the status information encoded within the
 *  <IzotNeuronState> enumeration.
 */
#define IZOT_NODE_STATE(state)   ((IZOT_NEURON_STATE(state) == IzotConfigOnLine) ? (state) : IZOT_NEURON_STATE(state))

/*
 *  Macro: IZOT_NODE_STATE_OFFLINE
 *  Extracts offline details from <IzotNeuronState>.
 *
 *  Use this macro to query the node's offline modes.
 */
#define IZOT_NODE_STATE_OFFLINE(state)   (IZOT_NEURON_STATE(state) == IzotConfigOffLine || IZOT_NEURON_STATE(state) == IzotSoftOffLine)

/*
 *  Typedef: IzotNodeState
 *  Decodes the node's state.
 */
typedef IZOT_ENUM_BEGIN(IzotNodeState)
{
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

/*
 * Enumeration: IzotNodeMode
 * Control node mode with <IzotNmSetNodeModeRequest>.
*/
typedef IZOT_ENUM_BEGIN(IzotNodeMode)
{
    IzotApplicationOffLine  = 0,
    IzotApplicationOnLine   = 1,
    IzotApplicationReset    = 2,
    IzotChangeState         = 3,
    IzotPhysicalReset       = 6
} IZOT_ENUM_END(IzotNodeMode);

/*
 *  Enumeration: IzotResetCause
 *  Decodes the last reset cause.
*/
typedef IZOT_ENUM_BEGIN(IzotResetCause)
{
    /* 0x00 */ IzotResetCleared = 0x00,
    /* 0x01 */ IzotPowerUpReset = 0x01,
    /* 0x02 */ IzotExternalReset = 0x02,
    /* 0x0C */ IzotWatchdogReset = 0x0C,
    /* 0x14 */ IzotSoftwareReset = 0x14
} IZOT_ENUM_END(IzotResetCause);

/*
 *  Enumeration: IzotAddressType
 *  Denotes destination address type.
 *
 *  This enumeration holds the literals for the 'type' field of destination
 *  addresses for outgoing messages.
 */
typedef IZOT_ENUM_BEGIN(IzotAddressType)
{
    /*  0 */ IzotAddressUnassigned = 0,
    /*  1 */ IzotAddressSubnetNode,
    /*  2 */ IzotAddressUniqueId,
    /*  3 */ IzotAddressBroadcast,
    /*127 */ IzotAddressLocal = 127
} IZOT_ENUM_END(IzotAddressType);

/*
 *  Enumeration: IzotRepeatTimer
 *  Encoded repeat timer values.
 *
 *  This enumeration defines the encoded repeat timer values.
 */
typedef IZOT_ENUM_BEGIN(IzotRepeatTimer)
{
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

/*
 *  Enumeration: IzotReceiveTimer
 *  Encoded receive timer values.
 *
 *  This enumeration defines the encoded receive timer values used with groups.
 *  For the non-group receive timer, see <IzotNonGroupReceiveTimer>.
 */
typedef IZOT_ENUM_BEGIN(IzotReceiveTimer)
{
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

/*
 *  Enumeration: IzotNonGroupReceiveTimer
 *  Encoded non-group receive timer values.
 *
 *  This enumeration defines the encoded values for the receive timer used
 *  with any addressing modes other than groups. For the receive timer used
 *  with groups, use <IzotReceiveTimer>.
 */
typedef IzotReceiveTimer IzotNonGroupReceiveTimer;

/*
 *  Enumeration: IzotTransmitTimer
 *  Encoded transmit timer values.
 *
 *  This enumeration defines the encoded transmit timer values.
 */
typedef IZOT_ENUM_BEGIN(IzotTransmitTimer)
{
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
/*
 * Enumeration: IzotServiceLedState
 * The service LED state
*/
typedef IZOT_ENUM_BEGIN(IzotServiceLedState)
{
    SERVICE_OFF = 0,
    SERVICE_ON = 1,
    SERVICE_BLINKING = 2,

    // Software controlled only
    SERVICE_FLICKER = -1
} IZOT_ENUM_END(IzotServiceLedState);

typedef IZOT_ENUM_BEGIN(IzotServiceLedPhysicalState)
{
    SERVICE_LED_OFF = 0,
    SERVICE_LED_ON = 1,
} IZOT_ENUM_END(IzotServiceLedPhysicalState);
#endif

/*
 * ******************************************************************************
 * SECTION: ADDRESSING TYPES
 * ******************************************************************************
 *
 *  This section contains definitions used with source addresses, destination
 *  addresses, and the address table.
 */

/*
 *  Typedef: IzotSendGroup
 *  Destination address type for group addressing.
 *
 *  Group address structure, used for multicast destination addresses with
 * <IzotSendAddress>.
 */

/*
 * Use the IZOT_SENDGROUP_TYPE_* macros to access the Type field in IzotSendGroup.TypeSize
 */
#define IZOT_SENDGROUP_TYPE_MASK     0x80    /* One for group, zero for all other address types */
#define IZOT_SENDGROUP_TYPE_SHIFT    7
#define IZOT_SENDGROUP_TYPE_FIELD    TypeSize

/*
 * Use the IZOT_SENDGROUP_SIZE_* macros to access the Size field in
 * IzotSendGroup.TypeSize
 */
#define IZOT_SENDGROUP_SIZE_MASK     0x7F    /* group size, or zero for unlimited group size */
#define IZOT_SENDGROUP_SIZE_SHIFT    0
#define IZOT_SENDGROUP_SIZE_FIELD    TypeSize

/*
 * Use the IZOT_SENDGROUP_DOMAIN_* macros to access the Domain field in
 * IzotSendGroup.DomainMember
 */
#define IZOT_SENDGROUP_DOMAIN_MASK   0x80    /* domain index for this group, zero or one */
#define IZOT_SENDGROUP_DOMAIN_SHIFT  7
#define IZOT_SENDGROUP_DOMAIN_FIELD  DomainMember

/*
 * Use the IZOT_SENDGROUP_MEMBER_* macros to access the Member field in
 * IzotSendGroup.DomainMember
 */
#define IZOT_SENDGROUP_MEMBER_MASK   0x7F    /* member ID within the group (0-63). Use zero for unlimited groups. */
#define IZOT_SENDGROUP_MEMBER_SHIFT  0
#define IZOT_SENDGROUP_MEMBER_FIELD  DomainMember

/*
 * Use the IZOT_SENDGROUP_REPEAT_TIMER_* macros to access the Repeat field in
 * IzotSendGroup.RepeatRetry
 */
#define IZOT_SENDGROUP_REPEAT_TIMER_MASK   0xF0  /* repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDGROUP_REPEAT_TIMER_SHIFT  4
#define IZOT_SENDGROUP_REPEAT_TIMER_FIELD  RepeatRetry

/*
 * Use the IZOT_SENDGROUP_RETRY_* macros to access the Retry field in
 * IzotSendGroup.RepeatRetry
 */
#define IZOT_SENDGROUP_RETRY_MASK    0x0F    /* number of retries, or number of transmissions minus one for unacknowledged service */
#define IZOT_SENDGROUP_RETRY_SHIFT   0
#define IZOT_SENDGROUP_RETRY_FIELD   RepeatRetry

/*
 * Use the IZOT_SENDGROUP_RECEIVE_TIMER_* macros to access the Receive Timer
 * field in IzotSendGroup.ReceiveTransmit
 */
#define IZOT_SENDGROUP_RECEIVE_TIMER_MASK  0xF0 /* receive timer. Use values from the <IzotReceiveTimer> enumeration. */
#define IZOT_SENDGROUP_RECEIVE_TIMER_SHIFT 4
#define IZOT_SENDGROUP_RECEIVE_TIMER_FIELD ReceiveTransmit

/*
 * Use the IZOT_SENDGROUP_TRANSMIT_TIMER_* macros to access the TransmitTimer
 * field in IzotSendGroup.ReceiveTransmit
 */
#define IZOT_SENDGROUP_TRANSMIT_TIMER_MASK  0x0F /* receive timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDGROUP_TRANSMIT_TIMER_SHIFT 0
#define IZOT_SENDGROUP_TRANSMIT_TIMER_FIELD ReceiveTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendGroup)
{
    IzotByte  TypeSize;       /* contains type, size. See IZOT_SENDGROUP_TYPE_* and _SIZE_* macros. */
    IzotByte  DomainMember;   /* contains domain, member. See IZOT_SENDGROUP_DOMAIN_* and _MEMBER_* macros. */
    IzotByte  RepeatRetry;    /* contains repeat, retry. See IZOT_SENDGROUP_REPEAT_* and _RETRY_* macros. */
    IzotByte  ReceiveTransmit;/* contains receive and transmit timers. See IZOT_SENDGROUP_RECEIVE_* and _TRANSMIT_* macros. */
    IzotGroupId GroupId;      /* the group ID, 0..255 */
} IZOT_STRUCT_END(IzotSendGroup);

/*
 *  Typedef: IzotSendSubnetNode
 *  Destination address type for subnet/node addressing.
 *
 *  Subnet/Node address structure, used for unicast destination addresses with
 *  <IzotSendAddress>.
 */

/*
 * Use the IZOT_SENDSN_DOMAIN_* macros to access the Domain field in
 * IzotSendSubnetNode.DomainNode
 */
#define IZOT_SENDSN_DOMAIN_MASK  0x80    /* domain index, zero or one */
#define IZOT_SENDSN_DOMAIN_SHIFT 7
#define IZOT_SENDSN_DOMAIN_FIELD DomainNode

/*
 * Use the IZOT_SENDSN_NODE_* and IZOT_NODEID_* macros to access the Node field
 * in IzotSendSubnetNode.DomainNode
 */
#define IZOT_SENDSN_NODE_MASK    0x7F    /* node ID 0-127 in this subnet, this domain */
#define IZOT_SENDSN_NODE_SHIFT   0
#define IZOT_SENDSN_NODE_FIELD   DomainNode

/*
 * Use the IZOT_SENDSN_REPEAT_* macros to access the Repeat field in
 * IzotSendSubnetNode.RepeatRetry
 */
#define IZOT_SENDSN_REPEAT_TIMER_MASK    0xF0    /* repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDSN_REPEAT_TIMER_SHIFT   4
#define IZOT_SENDSN_REPEAT_TIMER_FIELD   RepeatRetry

/*
 * Use the IZOT_SENDSN_RETRY_* macros to access the Retry field in
 * IzotSendSubnetNode.RepeatRetry
 */
#define IZOT_SENDSN_RETRY_MASK   0x0F    /* Retry count */
#define IZOT_SENDSN_RETRY_SHIFT  0
#define IZOT_SENDSN_RETRY_FIELD  RepeatRetry

/*
 * Use the IZOT_SENDSN_RSVD0_* macros to access the rsvd0 field in
 * IzotSendSubnetNode.RsvdTransmit
 */
#define IZOT_SENDSN_RSVD0_MASK   0xF0
#define IZOT_SENDSN_RSVD0_SHIFT  4
#define IZOT_SENDSN_RSVD0_FIELD  RsvdTransmit

/*
 * Use the IZOT_SENDSN_TRANSMIT_TIMER_* macros to access the transmit timer
 * field in IzotSendSubnetNode.RsvdTransmit
 */
#define IZOT_SENDSN_TRANSMIT_TIMER_MASK   0x0F   /* Transmit timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDSN_TRANSMIT_TIMER_SHIFT  0
#define IZOT_SENDSN_TRANSMIT_TIMER_FIELD  RsvdTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendSubnetNode)
{
    IZOT_ENUM(IzotAddressType) Type;  /* should be IzotAddressSubnetNode for subnet/node addressing */
    IzotByte  DomainNode;             /* contains domain, node. See IZOT_SENDSN_DOMAIN_* and _NODE_* macros. */
    IzotByte  RepeatRetry;            /* contains repeat, retry. See IZOT_SENDSN_REPEAT_* and _RETRY_* macros. */
    IzotByte  RsvdTransmit;           /* contains rsvd0, transmit. See IZOT_SENDSN_RSVD0_* and _TRANSMIT_TIMER_* macros. */
    IzotSubnetId Subnet;              /* destination subnet number, 1..255    */
} IZOT_STRUCT_END(IzotSendSubnetNode);

/*
 *  Typedef: IzotSendUniqueId
 *  Destination address type for Unique ID (Neuron ID) addressing.
 *
 *  Unique ID (Neuron ID) address structure, used for unicast destination
 *  addresses with <IzotSendAddress>.
 */

/*
 * Use the IZOT_SENDNID_DOMAIN_* macros to access the Domain field in
 * IzotSendUniqueId.DomainNode
 */
#define IZOT_SENDNID_DOMAIN_MASK     0x80     /* domain index, zero or one */
#define IZOT_SENDNID_DOMAIN_SHIFT    7
#define IZOT_SENDNID_DOMAIN_FIELD    Domain

/*
 * Use the IZOT_SENDNID_REPEAT_* macros to access the Repeat field in
 * IzotSendUniqueId.RepeatRetry
 */
#define IZOT_SENDNID_REPEAT_TIMER_MASK   0xF0 /* repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDNID_REPEAT_TIMER_SHIFT  4
#define IZOT_SENDNID_REPEAT_TIMER_FIELD  RepeatRetry

/*
 * Use the IZOT_SENDNID_RETRY_* macros to access the Retry field in
 * IzotSendUniqueId.RepeatRetry
 */
#define IZOT_SENDNID_RETRY_MASK      0x0F    /* Retry count */
#define IZOT_SENDNID_RETRY_SHIFT     0
#define IZOT_SENDNID_RETRY_FIELD     RepeatRetry

/*
 * Use the IZOT_SENDNID_RSVD0_* macros to access the rsvd0 field in
 * IzotSendUniqueId.RsvdTransmit
 */
#define IZOT_SENDNID_RSVD0_MASK  0xF0
#define IZOT_SENDNID_RSVD0_SHIFT 4
#define IZOT_SENDNID_RSVD0_FIELD RsvdTransmit

/*
 * Use the IZOT_SENDNID_TRANSMIT_TIMER_* macros to access the transmit timer
 * field in IzotSendUniqueId.RsvdTransmit
 */
#define IZOT_SENDNID_TRANSMIT_TIMER_MASK   0x0F /* Transmit timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDNID_TRANSMIT_TIMER_SHIFT  0
#define IZOT_SENDNID_TRANSMIT_TIMER_FIELD  RsvdTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendUniqueId)
{
    IZOT_ENUM(IzotAddressType) Type;  /* Should be IzotAddressUniqueId */
    IzotByte  Domain;                 /* Contains the domain index. See IZOT_SENDNID_DOMAIN_* macro. The remaining 7 bits must be zero. */
    IzotByte  RepeatRetry;            /* Contains repeat, retry. See IZOT_SENDNID_REPEAT_* and _RETRY_* macros. */
    IzotByte  RsvdTransmit;
    IzotSubnetId Subnet;              /* Destination subnet number, 1..255, or zero to pass all routers */
    IzotUniqueId NeuronId;            /* 48-bit unique ID of Neuron Chip or Smart Transceiver */
} IZOT_STRUCT_END(IzotSendUniqueId);

/*
 *  Typedef: IzotSendBroadcast
 *  Destination address type for broadcast addressing.
 *
 *  Broadcast address structure, used for multicast destination addresses with
 *  <IzotSendAddress>.
 */

/*
 * Use the IZOT_SENDBCAST_DOMAIN_* macros to access the Domain filed in
 * IzotSendBroadcast.DomainRsvdBacklog
 */
#define IZOT_SENDBCAST_DOMAIN_MASK   0x80    /* Domain index, zero or one */
#define IZOT_SENDBCAST_DOMAIN_SHIFT  7
#define IZOT_SENDBCAST_DOMAIN_FIELD  DomainRsvdBacklog

/*
 * Use the IZOT_SENDBCAST_RSVD0_* macros to access the rsvd0 field in
 * IzotSendBroadcast.DomainRsvdBacklog
 */
#define IZOT_SENDBCAST_RSVD0_MASK    0x40
#define IZOT_SENDBCAST_RSVD0_SHIFT   6
#define IZOT_SENDBCAST_RSVD0_FIELD   DomainRsvdBacklog

/*
 * Use the IZOT_SENDBCAST_BACKLOG_* macros to access the Backlog field in
 * IzotSendBroadcast.DomainRsvdBacklog
 */
#define IZOT_SENDBCAST_BACKLOG_MASK  0x3F   /* Backlog (set to zero if unknown) */
#define IZOT_SENDBCAST_BACKLOG_SHIFT 0
#define IZOT_SENDBCAST_BACKLOG_FIELD DomainRsvdBacklog

/*
 * Use the IZOT_SENDBCAST_REPEAT_* macros to access the Repeat field in
 * IzotSendBroadcast.RepeatRetry
 */
#define IZOT_SENDBCAST_REPEAT_TIMER_MASK     0xF0  /* Repeat timer. Use values from the <IzotRepeatTimer> enumeration. */
#define IZOT_SENDBCAST_REPEAT_TIMER_SHIFT    4
#define IZOT_SENDBCAST_REPEAT_TIMER_FIELD    RepeatRetry

/*
 * Use the IZOT_SENDBCAST_RETRY_* macros to access the Retry field in
 * IzotSendBroadcast.RepeatRetry
 */
#define IZOT_SENDBCAST_RETRY_MASK    0x0F       /* Retry count */
#define IZOT_SENDBCAST_RETRY_SHIFT   0
#define IZOT_SENDBCAST_RETRY_FIELD   RepeatRetry

/*
 * Use the IZOT_SENDNID_RSVD1_* macros to access the rsvd1 field in
 * IzotSendBroadcast.RsvdTransmit
 */
#define IZOT_SENDBCAST_RSVD1_MASK    0xF0		/* maxResponses */
#define IZOT_SENDBCAST_RSVD1_SHIFT   4
#define IZOT_SENDBCAST_RSVD1_FIELD   RsvdTransmit

/*
 * Use the IZOT_SENDBCAST_TRANSMIT_TIMER_* macros to access the transmit timer
 * field in IzotSendBroadcast.RsvdTransmit
 */
#define IZOT_SENDBCAST_TRANSMIT_TIMER_MASK   0x0F /* Transmit timer. Use values from the <IzotTransmitTimer> enumeration. */
#define IZOT_SENDBCAST_TRANSMIT_TIMER_SHIFT  0
#define IZOT_SENDBCAST_TRANSMIT_TIMER_FIELD  RsvdTransmit

typedef IZOT_STRUCT_BEGIN(IzotSendBroadcast)
{
    IZOT_ENUM(IzotAddressType) Type;  /* Should be IzotAddressBroadcast */
    IzotByte  DomainRsvdBacklog;      /* Contains domain, rsvd0, backlog. See IZOT_SENDBCAST_DOMAIN_*, _RSVD0_* and _BACKLOG_* macros. */
    IzotByte  RepeatRetry;            /* Contains repeat, retry. See IZOT_SENDBCAST_REPEAT_* and _RETRY_* macros. */
    IzotByte  RsvdTransmit;
    IzotSubnetId Subnet;              /* Destination subnet number, 1..255 for subnet broadcast, zero for domain broadcast */
} IZOT_STRUCT_END(IzotSendBroadcast);

/*
 *  Typedef: IzotSendUnassigned
 *  Address format to clear an address table entry.
 *
 *  Sets the first 2 bytes of the address table entry to 0.
 */
typedef IZOT_STRUCT_BEGIN(IzotSendUnassigned)
{
    IZOT_ENUM(IzotAddressType) Type;   /* Should be IzotAddressUnassigned */
} IZOT_STRUCT_END(IzotSendUnassigned);

/*
 *  Typedef: IzotSendLocal
 *  Destination address type to address the node locally with <IzotSendAddress>.
 *
 */
typedef IZOT_STRUCT_BEGIN(IzotSendLocal)
{
    IZOT_ENUM(IzotAddressType) Type;   /* Should be IzotAddressLocal */
} IZOT_STRUCT_END(IzotSendLocal);

/*
 * Typedef: IzotSendAddress
 * Union of all possible destination address formats.
 */
typedef IZOT_UNION_BEGIN(IzotSendAddress)
{
    IzotSendUnassigned   Unassigned;
    IzotSendGroup        Group;
    IzotSendSubnetNode   SubnetNode;
    IzotSendBroadcast    Broadcast;
    IzotSendUniqueId     UniqueId;
    IzotSendLocal        Local;
} IZOT_UNION_END(IzotSendAddress);

/*
 *  Typedef: IzotReceiveSubnetNode
 *  Received subnet/node ID destination addresses, used with unicast messages.
 *
 *  Used with <IzotReceiveDestination> and <IzotReceiveAddress>.
 */

/*
 * Use the IZOT_RECEIVESN_NODE_* macros to access the IzotReceiveSubnetNode.Node
 * field
 */
#define IZOT_RECEIVESN_SELFIELD_MASK  0x80
#define IZOT_RECEIVESN_SELFIELD_SHIFT 7
#define IZOT_RECEIVESN_SELFIELD_FIELD Node 
 
#define IZOT_RECEIVESN_NODE_MASK  0x7F  /* node Id 0..127, MSB is reserved */
#define IZOT_RECEIVESN_NODE_SHIFT 0
#define IZOT_RECEIVESN_NODE_FIELD Node

typedef IZOT_STRUCT_BEGIN(IzotReceiveSubnetNode)
{
    IzotByte  Subnet;
    IzotByte  Node;       /* node identifier, use IZOT_RECEIVESN_NODE_* macros */
} IZOT_STRUCT_END(IzotReceiveSubnetNode);

/*
 *  Typedef: IzotReceiveUniqueId
 *  Received 48-bit unique ID (Neuron ID) destination address.
 *
 *  Used with <IzotReceiveDestination>.
 */
typedef IZOT_STRUCT_BEGIN(IzotReceiveUniqueId)
{
    IzotSubnetId Subnet;
    IzotUniqueId UniqueId;
} IZOT_STRUCT_END(IzotReceiveUniqueId);

/*
 *  Typedef: IzotReceiveGroup
 *  Received a group destination address.
 *
 *  Used with <IzotReceiveDestination>.
 */
typedef IZOT_STRUCT_BEGIN(IzotReceiveGroup)
{
    IzotGroupId GroupId;             /* 0..255 */
} IZOT_STRUCT_END(IzotReceiveGroup);

/*
 *  Typedef: IzotReceiveBroadcast
 *  Received a broadcast destination address.
 *
 *  Used with <IzotReceiveDestination>.
 */
typedef IZOT_STRUCT_BEGIN(IzotReceiveBroadcast)
{
    IzotSubnetId SubnetId;       /* 1..255 for subnet broadcast, zero for domain broadcast */
} IZOT_STRUCT_END(IzotReceiveBroadcast);

/*
 *  Typedef: IzotReceiveDestination
 *  Union of all possible address formats for receiving an incoming message.
 */
typedef IZOT_UNION_BEGIN(IzotReceiveDestination)
{
    IzotReceiveBroadcast     Broadcast;
    IzotReceiveGroup         Group;
    IzotReceiveSubnetNode    SubnetNode;
    IzotReceiveUniqueId      UniqueId;
} IZOT_UNION_END(IzotReceiveDestination);

/*
 *  Enumeration: IzotReceiveDestinationAddressFormat
 *  Encodes the format of the receive address.
 *
 *  This enumeration encodes the format of the receive address of an incoming
 *  message, allowing you to select the corresponding member in
 *  <IzotReceiveDestination>.
 */
typedef IZOT_ENUM_BEGIN(IzotReceiveDestinationAddressFormat)
{
    /*  0 */ IzotReceiveDestinationAddressBroadcast = 0,
    /*  1 */ IzotReceiveDestinationAddressGroup,
    /*  2 */ IzotReceiveDestinationAddressSubnetNode,
    /*  3 */ IzotReceiveDestinationAddressUniqueId,
    /*  4 */ IzotReceiveDestinationAddressTurnaround
} IZOT_ENUM_END(IzotReceiveDestinationAddressFormat);

/*
 *  Typedef: IzotReceiveAddress
 *  Receive destination and source address for incoming messages.
 *
 *  This structure holds the receive destination address of an incoming message
 *  (the address through which the message was received on this node) and the
 *  message's source address (where it came from).
 */
#define IZOT_RECEIVEADDRESS_DOMAIN_MASK   0x80    /* Domain table index through which the message was received */
#define IZOT_RECEIVEADDRESS_DOMAIN_SHIFT  7
#define IZOT_RECEIVEADDRESS_DOMAIN_FIELD  DomainFormat

#define IZOT_RECEIVEADDRESS_FLEX_MASK     0x40    /* One for flex domain, that is, received message on unconfigured node */
#define IZOT_RECEIVEADDRESS_FLEX_SHIFT    6
#define IZOT_RECEIVEADDRESS_FLEX_FIELD    DomainFormat

#define IZOT_RECEIVEADDRESS_FORMAT_MASK   0x3F    /* Use <IzotReceiveDestinationAddressFormat> enumeration */
#define IZOT_RECEIVEADDRESS_FORMAT_SHIFT  0
#define IZOT_RECEIVEADDRESS_FORMAT_FIELD  DomainFormat

typedef IZOT_STRUCT_BEGIN(IzotReceiveAddress)
{
    IzotByte  DomainFormat;   /* Contains domain, flex domain, format. Use IZOT_RECEIVEADDRESS_* macros to access data. */
    IzotReceiveSubnetNode    Source;
    IzotReceiveDestination   Destination;
} IZOT_STRUCT_END(IzotReceiveAddress);

/*
 *  Typedef: IzotResponseSource
 *  Source address of a response message.
 *
 *  IzotResponseSource holds the source address of a response message.
 */
#define IZOT_RESPONSESOURCE_IS_SUBNETNODE_MASK   0x80    /* 1: subnet/node response. 0: group response. */
#define IZOT_RESPONSESOURCE_IS_SUBNETNODE_SHIFT  7
#define IZOT_RESPONSESOURCE_IS_SUBNETNODE_FIELD  Node

#define IZOT_RESPONSESOURCE_NODE_MASK     0x7F    /* Node ID of response source */
#define IZOT_RESPONSESOURCE_NODE_SHIFT    0
#define IZOT_RESPONSESOURCE_NODE_FIELD    Node

typedef IZOT_STRUCT_BEGIN(IzotResponseSource)
{
    IzotByte  Subnet;     /* subnet ID */
    IzotByte  Node;       /* contains node, isSubnetNode. Use IZOT_RESPONSESOURCE_NODE_* and IZOT_RESPONSESOURCE_IS_SUBNETNODE_* macros. */
} IZOT_STRUCT_END(IzotResponseSource);

/*
 *  Typedef: IzotResponseSubnetNode
 *  Destination of response to unicast request.
 *
 *  Used with <IzotResponseDestination>.
 */

/*
 * Use the IZOT_RESPONSESN_NODE_* macros to access the
 * IzotReceiveSubnetNode.Node field
 */
#define IZOT_RESPONSESN_NODE_MASK  0x7F    /* node Id 0..127, MSB is reserved */
#define IZOT_RESPONSESN_NODE_SHIFT 0
#define IZOT_RESPONSESN_NODE_FIELD Node

typedef IZOT_STRUCT_BEGIN(IzotResponseSubnetNode)
{
    IzotSubnetId Subnet;         /* subnet ID */
    IzotByte     Node;           /* node ID, use IZOT_RESPONSESN_NODE_* macros */
} IZOT_STRUCT_END(IzotResponseSubnetNode);

/*
 *  Typedef: IzotResponseGroup
 *  Destination of response to multicast request.
 *
 *  Used with <IzotResponseDestination>.
 */

/*
 * Use the IZOT_RESPONSESN_NODE_* macros to access the
 * IzotReceiveSubnetNode.Node field
 */
#define IZOT_RESPGROUP_NODE_MASK  0x7F    /* node Id 0..127, MSB is reserved */
#define IZOT_RESPGROUP_NODE_SHIFT 0
#define IZOT_RESPGROUP_NODE_FIELD Node

#define IZOT_RESPGROUP_MEMBER_MASK   0x3F
#define IZOT_RESPGROUP_MEMBER_SHIFT  0
#define IZOT_RESPGROUP_MEMBER_FIELD  Member

typedef IZOT_STRUCT_BEGIN(IzotResponseGroup)
{
    IzotSubnetId Subnet;         /* subnet ID */
    IzotByte     Node;
    IzotByte     Group;
    IzotByte     Member;         /* use IZOT_RESPGROUP_MEMBER_* macros for access */
} IZOT_STRUCT_END(IzotResponseGroup);

/*
 *  Typedef: IzotResponseDestination
 *  Destination of a response.
 */
typedef IZOT_UNION_BEGIN(IzotResponseDestination)
{
    IzotResponseSubnetNode   SubnetNode;
    IzotResponseGroup        Group;
} IZOT_UNION_END(IzotResponseDestination);

/*
 *  Typedef: IzotResponseAddress
 *  Address of incoming response.
 */
#define IZOT_RESPONSEADDRESS_DOMAIN_MASK     0x80    /* domain index, zero or one */
#define IZOT_RESPONSEADDRESS_DOMAIN_SHIFT    7
#define IZOT_RESPONSEADDRESS_DOMAIN_FIELD    Domain

#define IZOT_RESPONSEADDRESS_FLEX_MASK       0x40    /* 1: response from foreign domain */
#define IZOT_RESPONSEADDRESS_FLEX_SHIFT      6
#define IZOT_RESPONSEADDRESS_FLEX_FIELD      Domain

typedef IZOT_STRUCT_BEGIN(IzotResponseAddress)
{
    IzotByte                 Domain;         /* contains domain, flex domain. Use IZOT_RESPONSEADDRESS_* macros. */
    IzotResponseSource       Source;
    IzotResponseDestination  Destination;
} IZOT_STRUCT_END(IzotResponseAddress);

/*
 * Typedef: IzotExplicitAddress
 * Holds explicit addressing details, if enabled.
 *
 * IzotExplicitAddress holds the address details.
 */
typedef IZOT_UNION_BEGIN(IzotExplicitAddress)
{
    IzotReceiveAddress   Receive;
    IzotSendAddress      Send;
    IzotResponseAddress  Response;
} IZOT_UNION_END(IzotExplicitAddress);

/*
 * Typedef: IzotAddressTableGroup
 * Holds group addressing information in the address table (<IzotAddress>).
 *
 * IzotAddressTableGroup holds group addressing data in the address table, used
 * for multicast addressing. This structure also defines which group the node
 * belongs to.
 */
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

typedef IZOT_STRUCT_BEGIN(IzotAddressTableGroup)
{
    IzotByte      TypeSize;               /* Contains type, size. Use the IZOT_ADDRESS_GROUP_* macros. */
    IzotByte      DomainMember;           /* Contains domain, member. Use the IZOT_ADDRESS_GROUP_* macros. */
    IzotByte      RepeatRetry;            /* Contains repeatTimer, retry. Use the IZOT_ADDRESS_GROUP_* macros. */
    IzotByte      ReceiveTransmit;        /* Contains receive and transmit timer. Use Izot_ADDRESS_GROUP_* macros. */
    IzotGroupId   Group;                  /* Group identifier */
} IZOT_STRUCT_END(IzotAddressTableGroup);

/*
 *  Typedef: IzotAddressTableSubnetNode
 *  Holds subnet/node addressing information in the address table (<IzotAddress>).
 *
 *  IzotAddressTableSubnetNode holds subnet/node address information in the
 *  address table (<IzotAddress>), used for unicast addressing.
 */
#define IZOT_ADDRESS_SN_DOMAIN_MASK      0x80        /* domain index */
#define IZOT_ADDRESS_SN_DOMAIN_SHIFT     7
#define IZOT_ADDRESS_SN_DOMAIN_FIELD     DomainNode

#define IZOT_ADDRESS_SN_NODE_MASK        0x7F        /* node ID */
#define IZOT_ADDRESS_SN_NODE_SHIFT       0
#define IZOT_ADDRESS_SN_NODE_FIELD       DomainNode

#define IZOT_ADDRESS_SN_REPEAT_TIMER_MASK    0xF0    /* repeat timer, use IzotRepeatTimer */
#define IZOT_ADDRESS_SN_REPEAT_TIMER_SHIFT   4
#define IZOT_ADDRESS_SN_REPEAT_TIMER_FIELD   RepeatRetry

#define IZOT_ADDRESS_SN_RETRY_MASK       0x0F        /* retry count */
#define IZOT_ADDRESS_SN_RETRY_SHIFT      0
#define IZOT_ADDRESS_SN_RETRY_FIELD      RepeatRetry

typedef IZOT_STRUCT_BEGIN(IzotAddressTableSubnetNode)
{
    IZOT_ENUM(IzotAddressType) Type;         /* Set to IzotAddressSubnetNode   */
    IzotByte                  DomainNode;    /* Domain, node. Use IZOT_ADDRESS_SN_* macros. */
    IzotByte                  RepeatRetry;   /* Repeat timer and retry. Use IZOT_ADDRESS_SN_* macros. */
    IZOT_ENUM(IzotTransmitTimer) TransmitTimer;
    IzotSubnetId             Subnet;
} IZOT_STRUCT_END(IzotAddressTableSubnetNode);

/*
 *  Typedef: IzotAddressTableBroadcast
 *  Holds broadcast addressing information in the address table (<IzotAddress>).
 *
 * IzotAddressTableBroadcast holds broadcast addressing information in the
 *  address table (<IzotAddress>), used for multicast addressing.
 */
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

typedef IZOT_STRUCT_BEGIN(IzotAddressTableBroadcast)
{
    IZOT_ENUM(IzotAddressType) Type;          /* Set to IzotAddressBroadcast */
    IzotByte                   DomainBacklog; /* Domain, backlog. Use IZOT_ADDRESS_BROADCAST_* macros. */
    IzotByte                   RepeatRetry;   /* Repeat timer and retry. Use IZOT_ADDRESS_BROADCAST_* macros instead. */
    IZOT_ENUM(IzotTransmitTimer) TransmitTimer;
    IzotSubnetId               Subnet;
} IZOT_STRUCT_END(IzotAddressTableBroadcast);

/*
 *  Typedef: IzotAddressTableTurnaround
 *  Holds turnaround address information in the address table (<IzotAddress>).
 */
#define IZOT_ADDRESS_TURNAROUND_REPEAT_TIMER_MASK    0xF0    /* Use IzotRepeatTimer */
#define IZOT_ADDRESS_TURNAROUND_REPEAT_TIMER_SHIFT   4
#define IZOT_ADDRESS_TURNAROUND_REPEAT_TIMER_FIELD   RepeatRetry

#define IZOT_ADDRESS_TURNAROUND_RETRY_MASK           0x0F    /* Retry count */
#define IZOT_ADDRESS_TURNAROUND_RETRY_SHIFT          0
#define IZOT_ADDRESS_TURNAROUND_RETRY_FIELD          RepeatRetry

typedef IZOT_STRUCT_BEGIN(IzotAddressTableTurnaround)
{
    IZOT_ENUM(IzotAddressType) Type;          /* Set to IzotAddressUnassigned */
    IzotByte                   Turnaround;    /* 1: turnaround record. 0: not in use. */
    IzotByte                   RepeatRetry;   /* Contains repeat timer and retry. Use IZOT_ADDRESS_TURNAROUND_* macros. */
    IZOT_ENUM(IzotTransmitTimer) TransmitTimer;
} IZOT_STRUCT_END(IzotAddressTableTurnaround);

/*
 * ******************************************************************************
 * SECTION: SYSTEM STRUCTURES
 * ******************************************************************************
 *
 *  This section contains definitions of system resources and structures, such
 *  as the domain table or address table formats.
 */

/*
 *  Typedef: IzotAddress
 *  Describes one record of the address table.
 */
typedef IZOT_UNION_BEGIN(IzotAddress)
{
    IzotAddressTableGroup      Group;
    IzotAddressTableSubnetNode SubnetNode;
    IzotAddressTableBroadcast  Broadcast;
    IzotAddressTableTurnaround Turnaround;
} IZOT_UNION_END(IzotAddress);

/*
 *  Enumeration: IzotDomainLength
 *  Encodes the length of the domain.
 *
 *  This enumeration encodes the length of the domain.
 */
typedef IZOT_ENUM_BEGIN(IzotDomainLength)
{
    /*  0 */ IzotDomainLength_0  = 0,
    /*  1 */ IzotDomainLength_1  = 1,
    /*  3 */ IzotDomainLength_3  = 3,
    /*  6 */ IzotDomainLength_6  = 6
} IZOT_ENUM_END(IzotDomainLength);

/*
 *  Typedef: IzotDomain
 *  Format for a single domain table record.
 */
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

typedef IZOT_STRUCT_BEGIN(IzotDomain)
{
    IzotDomainId      Id;
    IzotSubnetId      Subnet;
    IzotByte          NodeClone;  /* contains non-clone, node. Use IZOT_DOMAIN_* macros. */
    IzotByte          InvalidIdLength;   /* use IZOT_DOMAIN_INVALID_* and IZOT_DOMAIN_ID_LENGTH_* macros */
    IzotAuthenticationKey Key;
} IZOT_STRUCT_END(IzotDomain);

/*
 *  Enumeration: IzotDatapointDirection
 *  Encodes the direction of a datapoint.
 *
 *  This enumeration encodes the direction of a datapoint.
 */
typedef IZOT_ENUM_BEGIN(IzotDatapointDirection)
{
    /*  0 */ IzotDatapointDirectionIsInput  = 0,
    /*  1 */ IzotDatapointDirectionIsOutput = 1
} IZOT_ENUM_END(IzotDatapointDirection);

/*
 *  Typedef: IzotDatapointConfig
 *  The datapoint configuration structure used for registering
 *  datapoints with the IzoT Device Stack DX at initialization
 *  time.
 */
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

typedef IZOT_STRUCT_BEGIN(IzotDatapointConfig)
{
    IzotByte  SelhiDirPrio;   /* Contains selector-high, direction, priority. Use IZOT_DATAPOINT_* macros. */
    IzotByte  SelectorLow;
    IzotByte  Attribute1;     /* Contains turnaround, service, authentication, and address-low. Use IZOT_DATAPOINT_* macros. */
	IzotByte  Attribute2;     /* Contains address-high and aes. Use IZOT_DATAPOINT_* macros. */
} IZOT_STRUCT_END(IzotDatapointConfig);

/*
 *  Enumeration: IzotSelectionType
 *  Literals for the ECS selection type.
 *
 */

typedef IZOT_ENUM_BEGIN(IzotSelectionType)
{
    IzotSelectionSelectorOnly,      /* 0: normal:  select as long as selector matches */
    IzotSelectionSelectorAndSource, /* 1: select if both selector and source match */
    IzotSelectionNoSelection,       /* 2: do not do Dp selection; reserved for poll-only inputs */
} IZOT_ENUM_END(IzotSelectionType);

/*
 *  Typedef: IzotDatapointEcsConfig
 *  The datapoint configuration structure for use with ECS devices.
 *
 *  IzotDatapointEcsConfig stores datapoint configuration in
 *  Extended Command Set (ECS) devices.
 */

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

typedef IZOT_STRUCT_BEGIN(IzotDatapointEcsConfig)
{
    IzotByte  EcsSelhiDirPrio; /* selector-high, direction, priority. Use IZOT_DATAPOINT_ECS_* macros. */
    IzotByte  SelectorLow;
    IzotByte  Attributes1;     /* turnaround, authentication write-by-index, remote-nm-auth, response-selection. Use IZOT_DATAPOINT_* macros. */
    IzotByte  Attributes2;     /* read-by-index, service, request-selection, update-selection, source-selection-only. Use IZOT_DATAPOINT_ECS_* macros. */
    IzotWord  AddressIndex;    /* Address table index */
    IzotWord  DatapointIndex;  /* Index of remote datapoint */
} IZOT_STRUCT_END(IzotDatapointEcsConfig);

/*
 *  Typedef: IzotAliasConfig
 *  Defines a datapoint alias for legacy devices.
 *
 */
typedef IZOT_STRUCT_BEGIN(IzotAliasConfig)
{
    IzotDatapointConfig        Alias;
    IzotByte                   Primary;
} IZOT_STRUCT_END(IzotAliasConfig);

/*
 *  Typedef: IzotAliasEcsConfig
 *  Defines a datapoint alias in ECS devices.
 *
 *  Used for Extended Command Set (ECS) devices, such as an IzoT device.
 */
typedef IZOT_STRUCT_BEGIN(IzotAliasEcsConfig)
{
    IzotDatapointEcsConfig               Alias;
    IzotWord                      Primary;
} IZOT_STRUCT_END(IzotAliasEcsConfig);

/*
 *  Typedef: IzotDirectModeTransceiver
 *  Holds direct-mode transceiver parameters.
 */
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

typedef IZOT_STRUCT_BEGIN(IzotDirectModeTransceiver)
{
    IzotByte      Parameter_1;    /* Collision-detect, bit-sync-threshold, filter, and hysteresis. Use IZOT_DIRECT_XCVR_* macros. */
    IzotByte      Parameter_2;    /* cd-to-end-packet, cd-tail, cd-preamble. Use IZOT_DIRECT_XCVR_* macros. */
} IZOT_STRUCT_END(IzotDirectModeTransceiver);

/*
 *  Typedef: IzotDatapointDefinition
 *  datapoint Definition.
 *
 * The IzotDatapointDefinition structure is used to define the attributes of a 
 * datapoint.  This is used to register static datapoints with the Izot protocol stack,
 * and is also used to retrieve information about static or dynamic datapoints.
 */

/*
 * Use the IZOT_DATAPOINT_* macros to form a bitmask that defines the attributes of a
 * datapoint.  These macros are used by the <flags> field in <IzotDatapointDefinition>.
 */
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

/*
 *  Macro: IZOT_DATAPOINT_RATE_UNKNOWN
 *  The max or mean datapoint rate is unknown or unspecified.
 *
 *  Use this value rather than an encoded rate for the MaxRate or MeanRate
 *  field of the <IzotDatapointDefinition> structure to indicate that the rate
 *  is unknown.
 */
#define IZOT_DATAPOINT_RATE_UNKNOWN      0

/*
 *  Macro: IZOT_DATAPOINT_DEFINITION_CURRENT_VERSION
 *  The current version of the IzotDatapointDefinition structure.
 *
 */
#define IZOT_DATAPOINT_DEFINITION_CURRENT_VERSION 2

typedef uint8_t IzotDatapointSize;

typedef IZOT_STRUCT_BEGIN(IzotDatapointDefinition)
{
    uint8_t         Version;        /* If the Izot protocol stack does
                                       not recognize the version, it will be
                                       rejected.  The current version is
                                       IZOT_DATAPOINT_DEFINITION_CURRENT_VERSION. */
    volatile void const *PValue;    /* Pointer to the datapoint value. */
    IzotDatapointSize DeclaredSize; /* The declared size of the Datapoint (1 to 228).
                                       This is also the initial size and the
                                       maximum size. */
    uint16_t        SnvtId;         /* Specifies the type of Datapoint if it is a
                                       SNVT (1-250). A non-standard datapoint
                                       type will have SnvtId = 0. */
    uint16_t        ArrayCount;     /* Array Count (0 to 4096).  A 0
                                       indicates that the datapoint is
                                       not an array. */
    uint32_t        Flags;          /* Bit flags describing attributes of the
                                       datapoint.  From a combination
                                       of the IZOT_DATAPOINT_* flags. */
    const char      *Name;          /* Datapoint name.  Limited to 16
                                       bytes base name plus an array
                                       designator of [dddd] where [dddd] is a
                                       one to four digit decimal number from
                                       0 to 4095. */
    const char      *SdString;      /* Self Doc String.  Can be null.  String
                                       length is 0 to 1023 characters. */
    uint8_t         MaxRate;        /* Encoded maximum rate (0 to 127, or 255).
                                       Set to IZOT_DATAPOINT_RATE_UKNOWN if not
                                       specified. */
    uint8_t         MeanRate;       /* Encoded rate(0 to 127, or 255).  Set
                                       to IZOT_DATAPOINT_RATE_UKNOWN if not specified. */                    
    const uint8_t   *ibol;          /* Points to the IBOL sequence */
    uint16_t        NvIndex;        /* NV index -- added for version 2 */
} IZOT_STRUCT_END(IzotDatapointDefinition);

/*
 *  Typedef: IzotConfigData
 *  The configuration data structure.
 */
#define IZOT_CONFIG_COMM_CLOCK_MASK      0xF8    /* communications clock rate */
#define IZOT_CONFIG_COMM_CLOCK_SHIFT     3
#define IZOT_CONFIG_COMM_CLOCK_FIELD     Clock

#define IZOT_CONFIG_INPUT_CLOCK_MASK     0x07    /* input clock */
#define IZOT_CONFIG_INPUT_CLOCK_SHIFT    0
#define IZOT_CONFIG_INPUT_CLOCK_FIELD    Clock

#define IZOT_CONFIG_COMM_TYPE_MASK       0xE0    /* communications type */
#define IZOT_CONFIG_COMM_TYPE_SHIFT      5
#define IZOT_CONFIG_COMM_TYPE_FIELD      CommConfiguration

#define IZOT_CONFIG_COMM_PINDIR_MASK     0x1F    /* pin direction */
#define IZOT_CONFIG_COMM_PINDIR_SHIFT    0
#define IZOT_CONFIG_COMM_PINDIR_FIELD    CommConfiguration

#define IZOT_CONFIG_NONGRPRCV_MASK       0xF0    /* non-group receive timer, use <IzotNonGroupReceiveTimer> */
#define IZOT_CONFIG_NONGRPRCV_SHIFT      4
#define IZOT_CONFIG_NONGRPRCV_FIELD      Config_1

#define IZOT_CONFIG_NMAUTH_MASK          0x08    /* network management authentication */
#define IZOT_CONFIG_NMAUTH_SHIFT         3
#define IZOT_CONFIG_NMAUTH_FIELD         Config_1

#define IZOT_CONFIG_PREEMPT_MASK         0x07    /* pre-emption timeout */
#define IZOT_CONFIG_PREEMPT_SHIFT        0
#define IZOT_CONFIG_PREEMPT_FIELD        Config_1

typedef IZOT_STRUCT_BEGIN(IzotConfigData)
{
    IzotWord         ChannelId;
    IzotLocationId   Location;
    IzotByte         Clock;              /* contains input clock, communications clock. Use IZOT_CONFIG_* macros. */
    IzotByte         CommConfiguration;  /* contains communications type, communications pin direction. Use IZOT_CONFIG_* macros. */
    IzotByte         PreambleLength;
    IzotByte         PacketCycle;
    IzotByte         Beta2Control;
    IzotByte         TransmitInterpacket;
    IzotByte         ReceiveInterpacket;
    IzotByte         NodePriority;
    IzotByte         ChannelPriorities;
    IZOT_UNION_BEGIN(CommunicationParameters)
    {
        IzotTransceiverParameters    TransceiverParameters;
        IzotDirectModeTransceiver    DirectModeParameters;
    } IZOT_UNION_END(CommunicationParameters);
    IzotByte          Config_1;           /* contains pre-emption timeout, network management authentication, non-group receive timer. Use IZOT_CONFIG_* macros. */
} IZOT_STRUCT_END(IzotConfigData);


#define IZOT_READONLY_CHECKSUM_MASK         0xF0    /* contains checksum for Unique Node ID */
#define IZOT_READONLY_CHECKSUM_SHIFT        4
#define IZOT_READONLY_CHECKSUM_FIELD        CheckSumMinorNum

#define IZOT_READONLY_MINORNUM_MASK         0x0F    /* minorModelNum */
#define IZOT_READONLY_MINORNUM_SHIFT        0
#define IZOT_READONLY_MINORNUM_FIELD        CheckSumMinorNum

#define IZOT_READONLY_RW_PROTECT_MASK        0x80    /* read+write protect flag */
#define IZOT_READONLY_RW_PROTECT_SHIFT       7
#define IZOT_READONLY_RW_PROTECT_FIELD       ReadOnly_1

#define IZOT_READONLY_RUN_UNCONFIG_MASK      0x40    /* runWhenUnconf */
#define IZOT_READONLY_RUN_UNCONFIG_SHIFT     6
#define IZOT_READONLY_RUN_UNCONFIG_FIELD     ReadOnly_1

#define IZOT_READONLY_DATAPOINT_COUNT_MASK           0x3F    /* dpCount */
#define IZOT_READONLY_DATAPOINT_COUNT_SHIFT          0
#define IZOT_READONLY_DATAPOINT_COUNT_FIELD          ReadOnly_1

#define IZOT_READONLY_DATAPOINT_PROCESSINGOFF_MASK          0x80    /* nvProcessingOff */
#define IZOT_READONLY_DATAPOINT_PROCESSINGOFF_SHIFT         7
#define IZOT_READONLY_DATAPOINT_PROCESSINGOFF_FIELD         ReadOnly_2

#define IZOT_READONLY_TWO_DOMAINS_MASK               0x40    /* twoDomains */
#define IZOT_READONLY_TWO_DOMAINS_SHIFT              6
#define IZOT_READONLY_TWO_DOMAINS_FIELD              ReadOnly_2

#define IZOT_READONLY_RESERVED2_MASK                 0x20    /* r2 */
#define IZOT_READONLY_RESERVED2_SHIFT                5
#define IZOT_READONLY_RESERVED2_FIELD                ReadOnly_2

#define IZOT_READONLY_RESERVED3_MASK                 0x10    /* r3 */
#define IZOT_READONLY_RESERVED3_SHIFT                4
#define IZOT_READONLY_RESERVED3_FIELD                ReadOnly_2

#define IZOT_READONLY_MSG_PROCESS_MASK               0x08    /* msgProcess */
#define IZOT_READONLY_MSG_PROCESS_SHIFT              3
#define IZOT_READONLY_MSG_PROCESS_FIELD              ReadOnly_2

#define IZOT_READONLY_NODE_STATE_MASK                0x07    /* nodeState */
#define IZOT_READONLY_NODE_STATE_SHIFT               0
#define IZOT_READONLY_NODE_STATE_FIELD               ReadOnly_2

#define IZOT_READONLY_ADDRESS_CNT_MASK               0xF0    /* # of entries in address table  */
#define IZOT_READONLY_ADDRESS_CNT_SHIFT              4
#define IZOT_READONLY_ADDRESS_CNT_FIELD              ReadOnly_3

#define IZOT_READONLY_RESERVED5_MASK                 0x0F    /* r5 */
#define IZOT_READONLY_RESERVED5_SHIFT                0
#define IZOT_READONLY_RESERVED5_FIELD                ReadOnly_3

#define IZOT_READONLY_RESERVED6_MASK                 0xF0    /* r6  */
#define IZOT_READONLY_RESERVED6_SHIFT                4
#define IZOT_READONLY_RESERVED6_FIELD                ReadOnly_4

#define IZOT_READONLY_REC_TRANSCNT_MASK              0x0F    /* receiveTransCnt */
#define IZOT_READONLY_REC_TRANSCNT_SHIFT             0
#define IZOT_READONLY_REC_TRANSCNT_FIELD             ReadOnly_4

#define IZOT_READONLY_OUTBUF_SIZE_MASK               0xF0    /* appOutBufSize */
#define IZOT_READONLY_OUTBUF_SIZE_SHIFT              4
#define IZOT_READONLY_OUTBUF_SIZE_FIELD              AppBufSize

#define IZOT_READONLY_INBUF_SIZE_MASK                0x0F    /* appInBufSize */
#define IZOT_READONLY_INBUF_SIZE_SHIFT               0
#define IZOT_READONLY_INBUF_SIZE_FIELD               AppBufSize

#define IZOT_READONLY_NW_OUTBUF_SIZE_MASK            0xF0    /* nwOutBufSize  */
#define IZOT_READONLY_NW_OUTBUF_SIZE_SHIFT           4
#define IZOT_READONLY_NW_OUTBUF_SIZE_FIELD           NwBufSize

#define IZOT_READONLY_NW_INBUF_SIZE_MASK             0x0F    /* nwInBufSize */
#define IZOT_READONLY_NW_INBUF_SIZE_SHIFT            0
#define IZOT_READONLY_NW_INBUF_SIZE_FIELD            NwBufSize

#define IZOT_READONLY_NW_OUT_PRICNT_MASK             0xF0    /* nwOutBufPriCnt  */
#define IZOT_READONLY_NW_OUT_PRICNT_SHIFT            4
#define IZOT_READONLY_NW_OUT_PRICNT_FIELD            PriCnt

#define IZOT_READONLY_OUT_PRICNT_MASK                0x0F    /* appOutBufPriCnt */
#define IZOT_READONLY_OUT_PRICNT_SHIFT               0
#define IZOT_READONLY_OUT_PRICNT_FIELD               PriCnt

#define IZOT_READONLY_OUTBUF_CNT_MASK                0xF0    /* appOutBufCnt  */
#define IZOT_READONLY_OUTBUF_CNT_SHIFT               4
#define IZOT_READONLY_OUTBUF_CNT_FIELD               AppBufCnt

#define IZOT_READONLY_INBUF_CNT_MASK                 0x0F    /* appInBufCnt */
#define IZOT_READONLY_INBUF_CNT_SHIFT                0
#define IZOT_READONLY_INBUF_CNT_FIELD                AppBufCnt

#define IZOT_READONLY_NW_OUTBUF_CNT_MASK             0xF0    /* nwOutBufCnt  */
#define IZOT_READONLY_NW_OUTBUF_CNT_SHIFT            4
#define IZOT_READONLY_NW_OUTBUF_CNT_FIELD            NwBufCnt

#define IZOT_READONLY_NW_INBUF_CNT_MASK              0x0F    /* nwInBufCnt */
#define IZOT_READONLY_NW_INBUF_CNT_SHIFT             0
#define IZOT_READONLY_NW_INBUF_CNT_FIELD             NwBufCnt

#define IZOT_READONLY_RESERVED7_MASK                 0xFC    /* R7  */
#define IZOT_READONLY_RESERVED7_SHIFT                4
#define IZOT_READONLY_RESERVED7_FIELD                ReadOnly_5

#define IZOT_READONLY_TX_BY_ADDRESS_MASK             0x02    /* txByAddress */
#define IZOT_READONLY_TX_BY_ADDRESS_SHIFT            1
#define IZOT_READONLY_TX_BY_ADDRESS_FIELD            ReadOnly_5

#define IZOT_READONLY_RESERVED8_MASK                 0x01    /* r8 */
#define IZOT_READONLY_RESERVED8_SHIFT                0
#define IZOT_READONLY_RESERVED8_FIELD                ReadOnly_5

#define IZOT_READONLY_RESERVED9_MASK                 0xC0    /* r9 */
#define IZOT_READONLY_RESERVED9_SHIFT                6
#define IZOT_READONLY_RESERVED9_FIELD                ReadOnly_6

#define IZOT_READONLY_ALIAS_CNT_MASK                 0x3F    /* aliasCnt */
#define IZOT_READONLY_ALIAS_CNT_SHIFT                0
#define IZOT_READONLY_ALIAS_CNT_FIELD                ReadOnly_6

#define IZOT_READONLY_MSG_TAG_CNT_MASK              0xF0    /* msgTagCnt */
#define IZOT_READONLY_MSG_TAG_CNT_SHIFT             4
#define IZOT_READONLY_MSG_TAG_CNT_FIELD             ReadOnly_7

#define IZOT_READONLY_RESERVED10_MASK               0x0F    /* r10 */
#define IZOT_READONLY_RESERVED10_SHIFT              0
#define IZOT_READONLY_RESERVED10_FIELD              ReadOnly_7

#define IZOT_READONLY_DMF_MASK                       0x80    /* dmf */
#define IZOT_READONLY_DMF_SHIFT                      0
#define IZOT_READONLY_DMF_FIELD                      ReadOnly_8

#define IZOT_READONLY_SEC_II_MASK                    0x40    /* Security II */
#define IZOT_READONLY_SEC_II_SHIFT                   6
#define IZOT_READONLY_SEC_II_FIELD                   ReadOnly_8

#define IZOT_READONLY_RESERVED11_MASK                0x7F    /* RESERVED11 */
#define IZOT_READONLY_RESERVED11_SHIFT               0
#define IZOT_READONLY_RESERVED11_FIELD               ReadOnly_8

typedef IZOT_STRUCT_BEGIN(IzotReadOnlyData)
{
    IzotUniqueId  UniqueNodeId;      /* 48-bit unique ID of Neuron Chip or Smart Transceiver              */
    IzotByte  ModelNum;              /* Model Number for Ref. Impl.                                       */
    IzotByte  CheckSumMinorNum;      /* contains checksum for Unique Node ID and minorModelNum (0-128). Use IZOT_READONLY_* macros. */
    IzotByte  DatapointFixed[2];            /* Location of nv fixed table.                                       */
    IzotByte  ReadOnly_1;            /* Contains:  (Use IZOT_READONLY_* macros. )                            */
                                     /* readWriteProtect , 1,       read+write protect flag.              */
                                     /* runWhenUnconf    , 1,       1=> Application runs.                 */
                                     /* nvCount          , 6,       0 for reference implementation.       */
    IzotByte  SnvtStruct[2];         /* 0xFFFF for reference implementation.                              */
    IzotProgramId ProgramId;         /* Program ID string (array of 8 bytes)                              */
    IzotByte  ReadOnly_2;            /* Contains:  (Use IZOT_READONLY_* macros. )                          */
                                     /* dpProcessingOff  , 1,       Must be one for NodeUtil.             */
                                     /* twoDomains       , 1,       1 if node is in 2 domains.            */
                                     /* r2               , 1,       explicitAddr not used in Ref. Impl.   */
                                     /* r3               , 1,       Unused.                               */
                                     /* msgProcess       , 1,       1 means explicit messages processed.  */
                                     /* nodeState        , 3,       Node State. See eia709_1.h            */
    IzotByte ReadOnly_3;             /* Contains: (Use IZOT_READONLY_* macros. )                           */
                                     /* # of entries in address table (AddressCn).                        */
                                     /* r5               , 4       Unused.                                */
    IzotByte ReadOnly_4;             /* Contains:  (Use IZOT_READONLY_* macros. )                          */
                                     /* r6               , 4,       Unused.                               */
                                     /* receiveTransCnt  , 4        RR Cnt = this field + 1               */
    IzotByte AppBufSize;             /* appOutBufSize   , 4,        Special Size Encoding.                */
                                     /* appInBufSize     , 4        Special Size Encoding.                */
    IzotByte NwBufSize;              /* nwOutBufSize, 4,            Special Size Encoding.                */
                                     /* nwInBufSize      , 4        Special Size Encoding.                */
    IzotByte PriCnt;                 /* nwOutBufPriCnt   , 4,       Special Count Enconding.              */
                                     /* appOutBufPriCnt  , 4        Special Count Enconding.              */
    IzotByte AppBufCnt;              /* appOutBufCnt, 4,            Special Count Enconding.              */
                                     /* appInBufCnt      , 4        Special Count Enconding.              */
    IzotByte NwBufCnt;               /* nwOutBufCnt, 4,             Special Count Enconding.              */
                                     /* nwInBufCnt       , 4        Special Count Enconding.              */
    IzotByte Reserved0;              /* Unused                                                            */
    IzotByte Reserved1[2];           /* Unused.                                                           */
    IzotByte Reserved2[3];           /* Unused.                                                           */
    IzotByte ReadOnly_5;             /* Contains:  (Use IZOT_READONLY_* macros. )                          */
                                     /* R7              , 6,       Unused.                                */
                                     /* txByAddress      , 1,      0 in reference implementation.         */
                                     /* r8               , 1)      Unused.                                */
    IzotByte ReadOnly_6;             /* r9               , 2,      Unused.                                */
                                     /* aliasCnt          , 6)     0 in reference implementation.         */
    IzotByte ReadOnly_7;             /* msgTagCnt, 4,      0 in reference implementation.                 */
                                     /* r10              , 4)       Unused.                               */
    IzotByte Reserved3[3];           /* Unused.                                                           */
    IzotByte DatapointCount;
    IzotByte AliasCount;
    IzotByte Snvt2Hi;
    IzotByte Snvt2Lo;
    IzotByte ReadOnly_8;             /* dmf  , 1,                                                         */
                                     /* r11              , 7                                              */
	IzotByte Extended;				/* Indicates the number of additional Address Table Entries
									   Changed as per Extended Address table doc requirement              */
} IZOT_STRUCT_END(IzotReadOnlyData);

/*
 *  Typedef: IzotStatus
 *  Holds node status and statistics.
 */
typedef IZOT_STRUCT_BEGIN(IzotStatus)
{
    IzotWord                 TransmitErrors;
    IzotWord                 TransactionTimeouts;
    IzotWord                 ReceiveTransactionsFull;
    IzotWord                 LostMessages;
    IzotWord                 MissedMessages;
    IZOT_ENUM(IzotResetCause) ResetCause;
    IZOT_ENUM(IzotNodeState)  NodeState;
    IzotByte                 VersionNumber;
    IZOT_ENUM(IzotSystemError) ErrorLog;
    IZOT_ENUM(IzotNeuronModel) ModelNumber;
    /*
     * The following members are available through the local <IzotQueryStatus>
     * API only, but are not transmitted in response to a query status network
     * diagnostic request.
     */
    IzotWord                 LostEvents;
} IZOT_STRUCT_END(IzotStatus);

/*
 * ******************************************************************************
 * SECTION: MESSAGE CODES
 * ******************************************************************************
 *
 *  This section defines message codes and utilities for dealing with message
 *  codes.
 */

/*
 *  Enumeration: IzotNmMessageCode
 *  Message codes for network management and diagnostic classes of messages.
 */
typedef IZOT_ENUM_BEGIN(IzotNmMessageCode)
{
    /* codes for network diagnostic commands */
    IzotNdQueryStatus                   = 0x51,
    IzotNdProxy                         = 0x52,
    IzotNdClearStatus                   = 0x53,
    IzotNdQueryXcvr                     = 0x54,
    IzotNdQueryStatusFlexDomain         = 0x56,

    /* codes for network management commands */
    IzotNmExpanded                      = 0x60,
    IzotNmQueryId                       = 0x61,
    IzotNmRespondToQuery                = 0x62,
    IzotNmUpdateDomain                  = 0x63,
    IzotNmLeaveDomain                   = 0x64,
    IzotNmUpdateKey                     = 0x65,
    IzotNmUpdateAddr                    = 0x66,
    IzotNmQueryAddr                     = 0x67,
    IzotNmQueryDatapointConfig                 = 0x68,
    IzotNmUpdateGroupAddr               = 0x69,
    IzotNmQueryDomain                   = 0x6A,
    IzotNmUpdateDatapointConfig                = 0x6B,
    IzotNmSetNodeMode                   = 0x6C,
    IzotNmReadMemory                    = 0x6D,
    IzotNmWriteMemory                   = 0x6E,
    IzotNmChecksumRecalculation         = 0x6F,
    IzotNmWink                          = 0x70,
    IzotNmInstall                       = IzotNmWink, /* See <IzotInstallCommand> */
    IzotNmAppCommand                    = IzotNmWink,
    IzotNmMemoryRefresh                 = 0x71,
    IzotNmQuerySnvt                     = 0x72,
    IzotNmDatapointFetch                       = 0x73,
    IzotNmDeviceEscape                  = 0x7D,
    IzotNmRouterEscape                  = 0x7E,
    IzotNmServicePin                    = 0x7F
} IZOT_ENUM_END(IzotNmMessageCode);

/*
 *  Enumeration: IzotInstallCommand
 *  Extended installation commands for devices that use SI data version 2. SI
 *  data version 2 is required for any device that supports dynamic datapoints.
 *  Used by <IzotNmInstallRequest>.
 */
typedef IZOT_ENUM_BEGIN(IzotInstallCommand)
{
    IzotInstallWink              = 0,    /* Basic application wink                     */
    IzotInstallQueryDatapointInfo       = 4,    /* Query datapoint information         */
    IzotInstallQueryNodeInfo     = 5,    /* Query node self-documentation information  */
} IZOT_ENUM_END(IzotInstallCommand);

/*
 *  Enumeration: IzotDatapointInfoType
 *  Types of datapoint information that can be queried using
 *  *IzotInstallQueryDatapointInfo*. Used by <IzotNmInstallRequest> when *Command* is
 *  set to *IzotInstallQueryDatapointInfo*.
 */
typedef IZOT_ENUM_BEGIN(IzotDatapointInfoType)
{
    IzotDatapointInfoDescriptor         = 0,    /* Query datapoint description (IzotNmInstallResponse.DatapointDescriptor) */
    IzotDatapointInfoRateEstimate       = 1,    /* Query datapoint rate estimates (IzotNmInstallResponse.DatapointRate) */
    IzotDatapointInfoName               = 2,    /* Query datapoint Name (IzotNmInstallResponse.DatapointName) */
    IzotDatapointInfoSdText             = 3,    /* Query datapoint self-documentation string (IzotNmInstallResponse.DatapointSd) */
    IzotDatapointInfoSnvtIndex          = 4,    /* Query datapoint SNVT index (IzotNmInstallResponse.SnvtTypeIndex) */
} IZOT_ENUM_END(IzotDatapointInfoType);

/*
 *  Enumeration: IzotNodeInfoType
 *  Types of node information that can be queried using *IzotInstallQueryNodeInfo*.
 *  Used by <IzotNmInstallRequest> when *Command* is set to
 *  *IzotInstallQueryNodeInfo*.
 */
typedef IZOT_ENUM_BEGIN(IzotNodeInfoType)
{
    IzotNodeInfoSdText             = 3,    /* Query node self-documentation string (IzotNmInstallResponse.NodeSd) */
} IZOT_ENUM_END(IzotNodeInfoType);

/*
 *  Enumeration: IzotDatapointOrigin
 *  Defines the origins of a datapoint.  Use the IZOT_DATAPOINT_DESC_ORIGIN_*
 *  macros with the <IzotNmInstallResponse> union.
 */
typedef IZOT_ENUM_BEGIN(IzotDatapointOrigin)
{
    IzotDatapointOriginUndefined    = 0,    /* Not currently defined */
    IzotDatapointOriginStatic       = 1,    /* Statically defined     */
    IzotDatapointOriginDynamic      = 2,    /* Dynamically defined     */
} IZOT_ENUM_END(IzotDatapointOrigin);

/*
 *  Macro: IZOT_NM_FAILURE
 *  Failure response codes for network management and diagnostic classes of
 *  messages.
 */
#define IZOT_NM_FAILURE(c)            ((c) & 0x1F)

/*
 *  Macro: IZOT_NM_SUCCESS
 *  Success response codes for network management and diagnostic classes of
 *  messages.
 */
#define IZOT_NM_SUCCESS(c)            (IZOT_NM_FAILURE(c) | 0x20)

/*
 *  Macro: IZOT_DATAPOINT_NAME_LEN
 *  The maximum number of bytes in a datapoint name, not including the 0
 *  terminator.
 *
 */
#define IZOT_DATAPOINT_NAME_LEN 16

/*
 *  The following IZOT_NM_* and IZOT_ND_* macros define offsets and masks for
 *  constructing request and response codes.
 */
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

/*
 *  Typedef: IzotCorrelator
 *  Data structure used for correlating requests and responses.
 *
 * IzotCorrelator is used to correlate request messages and their responses.
 */
typedef const void *IzotCorrelator;

/*
 *  Typedef: IzotResetNotification
 *  Structure for uplink reset message.
 *
 *  This type is used in the Reset callback, to provide call compatibility with
 *  ShortStack.  However, IzoT does not provide extended reset information at
 *  this time.
 */
typedef void IzotResetNotification;

/*
 *  Typedef: IzotNmDatapointFetchRequest
 *  Message structure for a Dp fetch request.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmDatapointFetchRequest)
{
    IzotByte  Index;
    IzotWord  EscapeIndex;  /* exists iff index==0xFF    */
} IZOT_STRUCT_END(IzotNmDatapointFetchRequest);

/*
 *  Typedef: IzotNmInstallRequest
 *  Message structure used with *IzotNmInstall* requests.
 *
 *  Each member of this union contains a *Command* field (<IzotInstallCommand>)
 *  that specifies the command type.  Even though this structure is defined as
 *  a union, the message size should include only the fields required for the
 *  particular command type.
 */

typedef IZOT_UNION_BEGIN(IzotNmInstallRequest)
{
    IZOT_STRUCT_NESTED_BEGIN(Wink)
    {
        IZOT_ENUM(IzotInstallCommand) Command; /* *IzotInstallWink* */
    } IZOT_STRUCT_NESTED_END(Wink);

    IZOT_STRUCT_NESTED_BEGIN(QueryDatapointInfo)
    {
        IZOT_ENUM(IzotInstallCommand) Command; /* *IzotInstallQueryDatapointInfo* */
        IZOT_ENUM(IzotDatapointInfoType)     DatapointInfoType;  /* Requested datapoint information */
        IzotWord                     DatapointIndex; /* Datapoint index */

        /* The following parameters are used only if DatapointInfoType is
         * *IzotDatapointInfoSdText*, and should be omitted when other types of Datapoint
         * information are being queried.
         */
        IZOT_UNION_NESTED_BEGIN(AdditionalParameters)
        {
            IZOT_STRUCT_NESTED_BEGIN(SdText)
            {
                /* Used when DatapointInfoType is *IzotDatapointInfoSdText* */
                IzotWord Offset;         /* Byte offset from beginning of SD text */
                IzotByte Length;         /* Maximum number of SD bytes to return  */
            } IZOT_STRUCT_NESTED_END(SdText);
        } IZOT_UNION_NESTED_END(AdditionalParameters);
    } IZOT_STRUCT_NESTED_END(QueryDatapointInfo);

    IZOT_STRUCT_NESTED_BEGIN(QueryNodeInfo)
    {
        IZOT_ENUM(IzotInstallCommand) Command;      /* *IzotInstallQueryNodeInfo*  */
        IZOT_ENUM(IzotNodeInfoType)   NodeInfoType; /* Requested node information */
        IZOT_UNION_NESTED_BEGIN(AdditionalParameters)
        {
            IZOT_STRUCT_NESTED_BEGIN(SdText)
            {
                /* Used when NodeInfoType is *IzotNodeInfoSdText* */
                IzotWord Offset;        /* Byte offset from beginning of SD text */
                IzotByte Length;        /* Maximum number of SD bytes to return  */
            } IZOT_STRUCT_NESTED_END(SdText);
        } IZOT_UNION_NESTED_END(AdditionalParameters);
    } IZOT_STRUCT_NESTED_END(QueryNodeInfo);

} IZOT_UNION_END(IzotNmInstallRequest);

/*
 *  Typedef: IzotNmInstallResponse
 *  Message structure used with *IzotNmInstall* responses.
 *
 */

/*
 * Use the IZOT_DATAPOINT_DESC_LENGTH_* macros to access the length field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_LENGTH_MASK   0xf8    /* Datapoint length */
#define IZOT_DATAPOINT_DESC_LENGTH_SHIFT  3
#define IZOT_DATAPOINT_DESC_LENGTH_FIELD  LengthAndOrigin

/*
 * Use the IZOT_DATAPOINT_DESC_ORIGIN_* macros to access the origin field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ORIGIN_MASK   0x07    /* Origin.  Use IzotDatapointOrigin. */
#define IZOT_DATAPOINT_DESC_ORIGIN_SHIFT  0
#define IZOT_DATAPOINT_DESC_ORIGIN_FIELD  LengthAndOrigin

/*
 * Use the IZOT_DATAPOINT_DESC_IS_OUTPUT_* macros to access the direction field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_IS_OUTPUT_MASK   0x10    /* 1: output, 0: input */
#define IZOT_DATAPOINT_DESC_IS_OUTPUT_SHIFT  4
#define IZOT_DATAPOINT_DESC_IS_OUTPUT_FIELD  Defaults

/*
 * Use the IZOT_DATAPOINT_DESC_DFLT_AUTH_* macros to access the default authentication
 * field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_DFLT_AUTH_MASK   0x08    /* 1: authenticated, 0: unauthenticated */
#define IZOT_DATAPOINT_DESC_DFLT_AUTH_SHIFT  3
#define IZOT_DATAPOINT_DESC_DFLT_AUTH_FIELD  Defaults

/*
 * Use the IZOT_DATAPOINT_DESC_DFLT_PRIORITY_* macros to access the default priority
 * field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_DFLT_PRIORITY_MASK   0x04    /* 1: priority, 0: non-priority */
#define IZOT_DATAPOINT_DESC_DFLT_PRIORITY_SHIFT  2
#define IZOT_DATAPOINT_DESC_DFLT_PRIORITY_FIELD  Defaults

/*
 * Use the IZOT_DATAPOINT_DESC_DFLT_SERVICE_* macros to access the service field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_DFLT_SERVICE_MASK   0x03    /* default service type.  Use IzotServiceType. */
#define IZOT_DATAPOINT_DESC_DFLT_SERVICE_SHIFT  0
#define IZOT_DATAPOINT_DESC_DFLT_SERVICE_FIELD  Defaults

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_SYNC_* macros to access the dp_sync field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_SYNC_MASK   0x40    /* 1: sync Datapoint, 0: non-sync Datapoint */
#define IZOT_DATAPOINT_DESC_ATTR_SYNC_SHIFT  6
#define IZOT_DATAPOINT_DESC_ATTR_SYNC_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_POLLED_* macros to access the dp_polled field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_POLLED_MASK   0x20    /* 1: polled or polling input, 0: not polled */
#define IZOT_DATAPOINT_DESC_ATTR_POLLED_SHIFT  5
#define IZOT_DATAPOINT_DESC_ATTR_POLLED_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_OFFLINE_* macros to access the dp_offline field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_OFFLINE_MASK   0x10    /* 1: take offline prior to updating */
#define IZOT_DATAPOINT_DESC_ATTR_OFFLINE_SHIFT  4
#define IZOT_DATAPOINT_DESC_ATTR_OFFLINE_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_* macros to access the
 * dp_service_type_config field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_MASK   0x08    /* 1: service type is configurable */
#define IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_SHIFT  3
#define IZOT_DATAPOINT_DESC_ATTR_SRVC_TYPE_CONFIG_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_* macros to access the
 * dp_priority_config field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_MASK   0x04    /* 1: priority flag is configurable */
#define IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_SHIFT  2
#define IZOT_DATAPOINT_DESC_ATTR_PRIORITY_CONFIG_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_* macros to access the dp_auth_config
 * field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_MASK   0x02    /* 1: authentication flag is configurable */
#define IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_SHIFT  1
#define IZOT_DATAPOINT_DESC_ATTR_AUTH_CONFIG_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_* macros to access the
 * dp_config_class field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_MASK   0x01    /* 1: a config Datapoint, 0: non-config Datapoint */
#define IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_SHIFT  0
#define IZOT_DATAPOINT_DESC_ATTR_CONFIG_CLASS_FIELD  BasicAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_* macros to access the max rate
 * estimate (mre) field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_MASK   0x80    /* 1: Max rate estimate available using IzotInstallQueryDatapointInfo, IzotDatapointInfoRateEstimate */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_SHIFT  7
#define IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL_FIELD  ExtendedAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_* macros to access the rate estimate
 * (re) field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_MASK   0x40    /* 1: Average rate estimate available using IzotInstallQueryDatapointInfo, IzotDatapointInfoRateEstimate */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_SHIFT  6
#define IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL_FIELD  ExtendedAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_* macros to access the dp_name
 * field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_MASK   0x20    /* 1: Datapoint Name is available using IzotInstallQueryDatapointInfo, IzotDatapointInfoName. */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_SHIFT  5
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_AVAIL_FIELD  ExtendedAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_* macros to access the sd field in
 * IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_MASK   0x10    /* 1: Datapoint SD text is available using IzotInstallQueryDatapointInfo, IzotDatapointInfoSdText  */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_SHIFT  4
#define IZOT_DATAPOINT_DESC_EXT_ATTR_SD_AVAIL_FIELD  ExtendedAttributes

/*
 * Use the IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_* macros to access the
 * name_supplied field in IzotNmInstallResponse.DatapointDescriptor
 */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_MASK   0x08    /* 1: Name is supplied in IzotNmInstallResponse.DatapointDescriptor */
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_SHIFT  3
#define IZOT_DATAPOINT_DESC_EXT_ATTR_NAME_SUPPLIED_FIELD  ExtendedAttributes

typedef IZOT_UNION_BEGIN(IzotNmInstallResponse)
{
    /* Response for requested info IzotInstallQueryDatapointInfo, IzotDatapointInfoDescriptor */
    IZOT_STRUCT_NESTED_BEGIN(DatapointDescriptor)
    {
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

    /* Response for requested info IzotInstallQueryDatapointInfo, IzotDatapointInfoRateEstimate */
    IZOT_STRUCT_NESTED_BEGIN(DatapointRate)
    {
        IzotByte    RateEstimate;       /* Encoded rate estimate. Only valid if 'IZOT_DATAPOINT_DESC_EXT_ATTR_RE_AVAIL' is set in DatapointDescriptor. */
        IzotByte    MaxRateEstimate;    /* Encoded max rate estimate. Only valid if 'IZOT_DATAPOINT_DESC_EXT_ATTR_MRE_AVAIL' is set in DatapointDescriptor. */
    } IZOT_STRUCT_NESTED_END(DatapointRate);

    /* Response for requested info IzotInstallQueryDatapointInfo, IzotDatapointInfoName */
    char    DatapointName[IZOT_DATAPOINT_NAME_LEN];   /* Datapoint name. Only valid if 'nm' set in Datapoint DatapointDescriptor. */

    /* Response for requested info IzotInstallQueryDatapointInfo, IzotDatapointInfoSdText */
    IZOT_STRUCT_NESTED_BEGIN(DatapointSd)
    {
        IzotByte    Length;      /* Number of bytes of SD text returned */
        IzotByte    Text[1];     /* SD text - actual length is Length above.
                                 * Might not be NULL terminated. */
    } IZOT_STRUCT_NESTED_END(DatapointSd);

    /* Response for requested info IzotInstallQueryDatapointInfo, IzotDatapointInfoSnvtIndex */
    IzotByte    SnvtTypeIndex;

    /* Response for requested info IzotInstallQueryNodeInfo, IzotNodeInfoSdText */
    IZOT_STRUCT_NESTED_BEGIN(NodeSd)
    {
        IzotByte    Length;      /* Number of bytes of SD text returned */
        IzotByte    Text[1];     /* SD text - actual length is Length above.
                                 * Might not be NULL terminated. */
    } IZOT_STRUCT_NESTED_END(NodeSd);
} IZOT_UNION_END(IzotNmInstallResponse) ;

/*
 *  Typedef: IzotNmSetNodeModeRequest
 *  Message structure for standard network management command *IzotNmSetNodeMode*.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmSetNodeModeRequest)
{
    IZOT_ENUM(IzotNodeMode)     Mode;
    IZOT_ENUM(IzotNodeState)    State;  /* iff mode == IzotChangeState */
} IZOT_STRUCT_END(IzotNmSetNodeModeRequest);

/*
 *  Enumeration: IzotMemoryReadWriteMode
 *  Defines addressing mode for memory read and write request.
 */
typedef IZOT_ENUM_BEGIN(IzotMemoryReadWriteMode)
{
    IzotAbsoluteMemory           = 0,         /* Address is absolute Smart Transceiver memory address */
    IzotReadOnlyRelative         = 1,         /* Address is offset into read-only memory structures */
    IzotConfigStructRelative     = 2,         /* Address is offset into configuration data structures */
    IzotStatisticStructRelative  = 3,         /* Address is offset into statistics data structures */
    IzotMemoryModeReserved_A     = 4          /* Reserved for Echelon internal use only */
} IZOT_ENUM_END(IzotMemoryReadWriteMode);

/*
 *  Enumeration: IzotMemoryWriteForm
 *  Defines actions that follow a memory write request.
 */
typedef IZOT_ENUM_BEGIN(IzotMemoryWriteForm)
{
    IzotNoAction                   = 0,
    IzotBothCsRecalculation        = 1,
    IzotDeltaCsRecalculation       = 3,
    IzotConfigCsRecalculation      = 4,
    IzotOnlyReset                  = 8,
    IzotBothCsRecalculationReset   = 9,
    IzotConfigCsRecalculationReset = 12
} IZOT_ENUM_END(IzotMemoryWriteForm);

/*
 *  Typedef: IzotNmReadMemoryRequest
 *  Message structure used with the *IzotNmReadMemory* request.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmReadMemoryRequest)
{
    IZOT_ENUM(IzotMemoryReadWriteMode)     Mode;
    IzotWord     Address;
    IzotByte     Count;
} IZOT_STRUCT_END(IzotNmReadMemoryRequest);

/*
 *  Typedef: IzotNmWriteMemoryRequest
 *  Message structure used with the *IzotNmWriteMemory* request.
 *
 *  Note that this structure shows only the message header. The message header
 *  is followed by the data to write, which must be *count* bytes following
 *  the field *Form*.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmWriteMemoryRequest)
{
    IZOT_ENUM(IzotMemoryReadWriteMode)    Mode;
    IzotWord     Address;
    IzotByte     Count;
    IZOT_ENUM(IzotMemoryWriteForm)        Form;
    /* <count> bytes of data following... */
} IZOT_STRUCT_END(IzotNmWriteMemoryRequest);

/*
 *  Enumeration: IzotApplicationMessageCode
 *  Message codes used with application messages.
 *
 *  Application message codes are in the [IzotApplicationMsg, IzotForeignMsg]
 *  range, but selected values are frequently used and should be avoided:
 * IzotApplicationIsi is used by the Interoperable Self-Installation (ISI)
 *  protocol, and IzotApplicationFtp is used for the interoperable file
 *  transfer protocol (FTP).
 */
typedef IZOT_ENUM_BEGIN(IzotApplicationMessageCode)
{
    IzotApplicationMsg        = 0x00,
    IzotApplicationIsi        = 0x3D,
    IzotApplicationFtp        = 0x3E,
    IzotApplicationIsOffLine  = 0x3F,
    IzotForeignMsg            = 0x40,
    IzotForeignIsOffLine      = 0x4F,
    IzotLastMessageCode       = 0x4F
} IZOT_ENUM_END(IzotApplicationMessageCode);

/*
 *  Typedef: IzotNmQueryDomainRequest
 *  Message structure used with *IzotNmQueryDomain*.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmQueryDomainRequest)
{
    IzotByte Index;             /* Domain index                 */
} IZOT_STRUCT_END(IzotNmQueryDomainRequest);

/*
 *  Typedef: IzotNmQueryDatapointAliasRequest
 *  Message structure used with *IzotNmQueryDatapointConfig*.
 *
 */
typedef IZOT_STRUCT_BEGIN(IzotNmQueryDatapointAliasRequest)
{
    IzotWord  Index;             /* Datapoint config table index                 */
} IZOT_STRUCT_END(IzotNmQueryDatapointAliasRequest);

/*
 *  Typedef: IzotNmQueryAddressRequest
 *  Message structure used with *IzotNmQueryAddr*.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmQueryAddressRequest)
{
    IzotByte Index;             /* address table index                 */
} IZOT_STRUCT_END(IzotNmQueryAddressRequest);

/*
 *  Typedef: IzotNdQueryStatusResponse
 *  Message structure used with responses to *IzotNdQueryStatus*.
 */
typedef IZOT_STRUCT_BEGIN(IzotNdQueryStatusResponse)
{
    IzotStatus    Status;
} IZOT_STRUCT_END(IzotNdQueryStatusResponse);

/*
 *  Typedef: IzotNdQueryXcvrResponse
 *  Message structure used with responses to *IzotNdQueryXcvr*.
 */
typedef IZOT_STRUCT_BEGIN(IzotNdQueryXcvrResponse)
{
    IzotTransceiverParameters    Status;
} IZOT_STRUCT_END(IzotNdQueryXcvrResponse);

/*
 *  Typedef: IzotNmUpdateAddressRequest
 *  Message structure used with *IzotNmUpdateAddr* requests.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateAddressRequest)
{
    IzotByte      Index;
    IzotAddress   Address;
} IZOT_STRUCT_END(IzotNmUpdateAddressRequest);

/*
 *  Typedef: IzotNmUpdateDatapointRequest
 *  Message structure used with *IzotNmUpdateDatapointConfig* requests.
 *
 *  This structure is used to update the datapoint configuration table.
 *  The structure exists in two forms. If the 'ShortIndex' field is in the
 *  range 0..254, use the ShortForm union member. If the short index equals
 *  255, use the LongForm union member and obtain the true index from the
 * LongIndex field. Note that the data structure shown is an abstraction; the
 *  actual message frame used is the smallest possible.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateDatapointRequest)
{
    IzotByte      ShortIndex;

    IZOT_UNION_NESTED_BEGIN(Request)
    {
        IZOT_STRUCT_NESTED_BEGIN(ShortForm)
        {
            IzotDatapointConfig     DatapointConfig;
        } IZOT_STRUCT_NESTED_END(ShortForm);
        IZOT_STRUCT_NESTED_BEGIN(LongForm)
        {
            IzotWord LongIndex;
            IzotDatapointConfig     DatapointConfig;
        } IZOT_STRUCT_NESTED_END(LongForm);
    } IZOT_UNION_NESTED_END(Request);
} IZOT_STRUCT_END(IzotNmUpdateDatapointRequest);

/*
 *  Typedef: IzotNmUpdateAliasRequest
 *  Message structure used with *IzotNmUpdateDatapointConfig* requests.
 *
 *  This structure is used to update the alias configuration table. The
 *  structure exists in two forms. If the 'ShortIndex' field is in the range
 *  0..254, use the ShortForm union member. If the short index equals 255, use
 *  the LongForm union member and obtain the true index from the LongIndex
 *  field. The index should be that of the alias plus the datapoint
 *  count; the alias table follows the datapoint config table and the
 *  index continues from that table.  Note that the data structure shown is an
 *  abstraction; the actual message frame used is the smallest possible.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateAliasRequest)
{
 IzotByte      ShortIndex;

    IZOT_UNION_NESTED_BEGIN(Request)
    {
        IZOT_STRUCT_NESTED_BEGIN(ShortForm)
        {
            IzotAliasConfig  AliasConfig;
        } IZOT_STRUCT_NESTED_END(ShortForm);
        IZOT_STRUCT_NESTED_BEGIN(LongForm)
        {
            IzotWord LongIndex;
            IzotAliasConfig  AliasConfig;
        } IZOT_STRUCT_NESTED_END(LongForm);
    } IZOT_UNION_NESTED_END(Request);
} IZOT_STRUCT_END(IzotNmUpdateAliasRequest);

/*
 *  Typedef: IzotNmUpdateDomainRequest
 *  Message structure used with the *IzotNmUpdateDomain* request.
 */
typedef IZOT_STRUCT_BEGIN(IzotNmUpdateDomainRequest)
{
    IzotByte     Index;
    IzotDomain   Domain;
} IZOT_STRUCT_END(IzotNmUpdateDomainRequest);

/*
*  Enumeration: IzotServiceType
*  Literals for the service type.
*/
typedef IZOT_ENUM_BEGIN(IzotServiceType)
{
    IzotServiceAcknowledged       = 0,    /* ACKD         */
    IzotServiceRepeated           = 1,    /* UNACKD_RPT   */
    IzotServiceUnacknowledged     = 2,    /* UNACKD       */
    IzotServiceRequest            = 3,    /* REQUEST      */
	IzotServiceResponse           = 4     /* RESPONSE     */ /* Session Layer */
} IZOT_ENUM_END(IzotServiceType);

/*
 * *****************************************************************************
 * SECTION: NON-VOLATILE DATA
 * *****************************************************************************
 *
 * This section contains the enumerations and data types used for non-volatile
 * data (NVD) support.
 */

/*
 * Enumeration: IzotPersistentSegmentType
 * Persistent data segment type. Configuration data is stored persistently
 * in persistent data segments, identified using this enumeration.  Used
 * by the non-volatile callback functions IzotPersistentxxx.
 *
 */
typedef IZOT_ENUM_BEGIN(IzotPersistentSegmentType)
{
	IzotPersistentSegNetworkImage,      /* Basic network configuration, such as the
                                           domain table, address tables, and 
                                           datapoint configuration.
                                         */
    IzotPersistentSegSecurityII,          /* Security state and Replay Table information */                                     
	IzotPersistentSegNodeDefinition,    /* Definitions that affect the current
                                       interface, including dynamic datapoint
                                       definitions.
                                     */
	IzotPersistentSegApplicationData,   /* Application data, such as CP values, that
                                       needs to be stored persistently.  Note that
                                       in some devices, this can include CP values
                                       implemented by datapoints, CP values
                                       defined in files, or both.
                                     */
    IzotPersistentSegUniqueId,          /* Unique ID defined in file for the IP852 device */
    IsiPersistentSegConnectionTable,   /* ISI connection table */
    IsiPersistentSegPersistent,              /* Other ISI persistence information */
    IzotPersistentSegNumSegmentTypes
} IZOT_ENUM_END(IzotPersistentSegmentType);

/*
 *  Typedef: IzotPersistentHandle
 *  A handle to an open persistent data segment.  Returned by the
 * IzotPersistentOpenxxx callbacks, and used by the <IzotPersistentRead>,
 * <IzotPersistentWrite>, and <IzotPersistentClose> callbacks.
 */
typedef void *IzotPersistentHandle;

/*
 *  Macro: IZOT_STACK_INTERFACE_CURRENT_VERSION
 *  The current version of the <IzotStackInterfaceData> structure.
 *
 */
#define IZOT_STACK_INTERFACE_CURRENT_VERSION 0

/*
 *  Macro: IZOT_CONTROL_DATA_CURRENT_VERSION
 *  The current version of the <IzotControlData> structure.
 *
 */
#define IZOT_CONTROL_DATA_CURRENT_VERSION 0

/*
 *  Macro: IzoT_NUM_COMM_BYTES
 *  The number of bytes of communication parameter data.
 *
 */
#define IzoT_NUM_COMM_BYTES 16

/*
 *  Enumeration: IzotTransceiverType
 *  IzoT Transceiver type.
 *
 *  This enumeration lists all transceivers supported by IzoT.
 */
typedef IZOT_ENUM_BEGIN(IzotTransceiverType)
{
    IzotTransceiverTypeDefault,
    IzotTransceiverType5MHz,
    IzotTransceiverType10MHz,
    IzotTransceiverType20MHz,
    IzotTransceiverType40MHz,
    IzotTransceiverTypeCustom,
} IZOT_ENUM_END(IzotTransceiverType);

/*
 *  Typedef: IzotStackInterfaceData
 *  IzoT programmatic interface.
 *
 *  The <IzotStackInterfaceData> defines the static attributes of the program.
 */

typedef IZOT_STRUCT_BEGIN(IzotStackInterfaceData)
{
    uint8_t Version;                /* Format version number for future
                                       extensions.  If the Izot
                                       protocol stack does not recognize the
                                       version, it will be rejected.  The
                                       current version is
                                       IZOT_STACK_INTERFACE_CURRENT_VERSION. */
    uint32_t Signature;             /* 32-bit unique numerical application
                                       identifier. */
    IzotProgramId ProgramId;        /* Program ID string (array of 8 bytes) */
    uint16_t StaticDatapoints;      /* Number of static datapoints */
    uint16_t MaxDatapoints;         /* Maximum number of datapoints
                                       (0..4096).  This is the maximum total
                                       number of static and dynamic datapoints. */
    uint8_t Domains;                /* Number of domain in the node (1 or 2) */
    uint16_t Addresses;             /* Maximum number of address table entries
                                       (0..4096) */
    uint16_t Aliases;               /* Maximum number of alias tables (0..8192) */
    uint16_t BindableMsgTags;       /* Number of bindable message tags (0.. 4096) */
    const char *NodeSdString;       /* Node self documentation string. */
    uint8_t AvgDynDatapointSdLength;  /* Average number of bytes to reserve for
                                       dynamic datapoint self-documentation data. */
    /* The following fields are added in version 1 of this structure: */
    uint8_t *SiData;    /* Pointer to self-identification data, NULL for EX */
    uint32_t SiDataLength;          /* Size of self-identification data, in bytes (0 for EX) */

} IZOT_STRUCT_END(IzotStackInterfaceData);

/*
 *  Typedef: IzotControlData
 *  IzoT control data.
 *
 *  The IzotControlData structure contains data used to control the runtime
 *  aspects of the Izot protocol stack.
 */

typedef IZOT_STRUCT_BEGIN(IzotControlData)
{
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

        /* Defines the communication parameters used by the Izot
           protocol stack. */
    IZOT_STRUCT_BEGIN(CommParmeters)
    {
            /* Transceiver type. If the value is IzoTTransceiverTypeDefault,
               the comm. parameters in the IzoT Transceiver are left unchanged.
               If the value is IzoTTransceiverTypeCustom, the comm. parameters
               are set based on the CommParms field.  Otherwise the comm.
               parameters are modified based on hard-coded comm. parameters
               for the specified type. */
        IzotTransceiverType TransceiverType;

            /* An array of 16 bytes representing the comm. parameters.  If
               TransceiverType is IzoTTransceiverTypeCustom, these values are
               written out to the IzoT Transceiver.  Otherwise they are
               ignored. */
        unsigned char CommParms[IzoT_NUM_COMM_BYTES];
    } IZOT_STRUCT_END(CommParmeters);

        /* The number of and size of various buffers used by the Izot
           protocol stack.
           The buffer counts and sizes are considered a minimum recommendation.
           Implementations of the IzoT Device Stack can chose to implement more
           or larger buffers provided that the minimum specifications provided
           here are met.
           Note that some implementations of the IzoT Device Stack only implement
           some of the buffer types defined here. */
    IZOT_STRUCT_BEGIN(Buffers)
    {
        /* There are three categories of buffers used by the Izot
           protocol stack */
        IZOT_STRUCT_BEGIN(ApplicationBuffers)
        {
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
           by the link-layer driver that communicates with the IzoT
           Transceiver.  At most, one such buffer is needed for output at any
           given time, but multiple input buffers might be required.  These
           buffers are shared for both input and output.  The size of the
           buffers is determined by the transceiver�s network buffer size.   */
        IZOT_STRUCT_BEGIN(LinkLayerBuffers)
        {
            uint8_t LinkLayerBufferCount;   /* Number of driver buffers (1 to
                                               100).  Recommended default = 2. */
        } IZOT_STRUCT_END(LinkLayerBuffers);

        /* These are the buffers defined on the IzoT Transceiver.  The
           application buffers on the transceiver are used only for local
           network management, and therefore the Izot protocol stack
           does not support modifying the application buffers.  The network
           buffers can be configured.  Because these are defined on the IzoT
           Transceiver, the counts and sizes must conform to those defined by
           tables B.1 and B.2 in the EIA/CEA-709.1-B standard, and the total
           memory requirement specified by the buffers must not exceed the
           capacity of the transceiver's firmware.  The IzoT Transceiver has
           hard-coded knowledge of the maximum buffer configuration, and
           prevents accidentally exceeding these constraints. */
        IZOT_STRUCT_BEGIN(TransceiverBuffers)
        {
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

/*
 * ******************************************************************************
 * SECTION: SYNCHRONOUS EVENTS
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API callback
 *  functions.
 *
 *  Remarks:
 *  Callback functions are called by the IzoT  protocol stack
 *  immediately, as needed, and may be called from any IzoT task or thread.  The
 *  application *must not* call into the IzoT Device Stack API from within a
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
 *  The IzoT  protocol stack will not propagate a datapoint with
 *  size 0, nor will it generate an update event if a datapoint update
 *  is received from the network when the current datapoint size is 0.
 *
 *  Note that even though this is a callback function, it *is* legal for the
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
 *  Read memory in the IzoT device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be read
 *  size - number of bytes to read
 *  pData - pointer to a buffer to store the requested data
 *
 *  Remarks:
 *  The IzoT protocol stack calls <IzotMemoryRead> whenever it receives
 *  a network management memory read request that fits into the registered
 *  file access window. This callback function is used to read data starting
 *  at the specified virtual memory address, but located within the IzoT
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
typedef IzotApiError (*IzotMemoryReadFunction)(const unsigned address,
        const unsigned size, void* const pData);

/*
 *  Callback: IzotMemoryWrite
 *  Update memory in the IzoT device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be written
 *  size - number of bytes to write
 *  pData - pointer to the data to write
 *
 *  Remarks:
 *  The IzoT  protocol stack calls <IzotMemoryWrite> whenever it
 *  receives a network management memory write request that fits into the
 *  registered file access window. This callback function is used to write
 *  data at the specified virtual memory address, but located within the IzoT
 *  application. This event is generally used to provide access to property
 *  value files through standard memory write protocol messages.
 *
 *  The IzoT protocol stack automatically calls the
 *  <IzotPersistentAppSegmentHasBeenUpdated> function to schedule an update
 *  whenever this callback returns <IzotApiError>.
 *
 *  Use <IzotRegisterMemoryWindow> to configure this service.
 *  Use <IzotMemoryWriteRegistrar> to register a handler for this event.
 *  Without an application-specific handler for this event, the stack returns
 *  a failure response to the memory write request message.
 *
 *  Applications must generally implement both the IzotMemoryRead and
 *  IzotMemoryWrite events, or none.
 */
typedef IzotApiError (*IzotMemoryWriteFunction)(const unsigned address,
        const unsigned size, const void* const pData);

/*
 *  Persistent data support events
 *
 *  Persistent data is variable data which must be kept safe across a device
 *  reset or a power cycle. The IzoT Device Stack EX provides transparent
 *  persistent data management functions, but the DX stack, which generally
 *  supports resource-limited platforms, uses the events defined in this
 *  section. Applications implement these events to supply the low-level
 *  functionality, for example by storing the persistent data segments in
 *  a file system or a flash memory device.
 */

/*
 *  Callback: IzotPersistentOpenForRead
 *  Open a persistent data segment for reading.
 *
 *  Parameters:
 *  type -  type of persistent data to be opened
 *
 *  Returns:
 *  <IzotPersistentHandle>.
 *
 *  Remarks:
 *  This function opens the data segment for reading.  When suitable storage
 *  exists and can be accessed, the function returns an application-specific
 *  non-zero handle. Otherwise it returns 0.
 *
 *  The handle returned by this function will be used as the first parameter
 *  when calling <IzotPersistentRead>.
 *
 *  The application must maintain the handle used for each segment. The
 *  application can invalidate a handle when <IzotPersistentClose> is called
 *  for that handle.
 *
 *  Use <IzotPersistentOpenForReadRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event always fails
 *  (the handle is always zero).
 */
typedef IzotPersistentHandle (*IzotPersistentOpenForReadFunction)(const IzotPersistentSegmentType type);

/*
 *  Callback: IzotPersistentOpenForWrite
 *  Open a persistent data segment for writing.
 *
 *  Parameters:
 *  type - type of persistent data to be opened
 *  size - size of the data to be stored
 *
 *  Returns:
 *  <IzotPersistentHandle>.
 *
 *  Remarks:
 *  This function is called by the IzoT protocol stack after changes
 *  have been made to data that is backed by persistent storage. Note that
 *  many updates might have been made, and this function is called only after
 *  the stack determines that all updates have been completed, based on a flush
 *  timeout.
 *
 *  This function opens the data segment for reading.  When suitable storage
 *  exists and can be accessed, the function returns an application-specific
 *  non-zero handle. Otherwise it returns 0.
 *
 *  The handle returned by this function will be used as the first parameter
 *  when calling <IzotPersistentWrite>.
 *
 *  The application must maintain the handle used for each segment. The
 *  application can invalidate a handle when <IzotPersistentClose> is called
 *  for that handle.
 *
 *  Use <IzotPersistentOpenForWriteRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event always fails
 *  (the handle is always zero).
 */
typedef IzotPersistentHandle (*IzotPersistentOpenForWriteFunction)(
        const IzotPersistentSegmentType type, const size_t size);

/*
 *  Callback: IzotPersistentClose
 *  Close a persistent data segment.
 *
 *  Parameters:
 *  handle - handle of the persistent segment returned by <IzotPersistentOpenForRead>
 *           or <IzotPersistentOpenForWrite>
 *
 *  Remarks:
 *  This function closes the persistent memory segment associated with this
 *  handle and invalidates the handle.
 *
 *  Use <IzotPersistentCloseRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotPersistentCloseFunction)(
    const IzotPersistentHandle handle);

/*
 *  Callback: IzotPersistentDelete
 *  Delete persistent data segment.
 *
 *  Parameters:
 *  type - type of persistent data to be deleted
 *
 *  Remarks:
 *  This function is used to delete the persistent memory segment referenced
 *  by the data type. The stack will only call this function on closed segments.
 *
 *  Note that this function can be called even if the segment does not exist
 *  in persistent storage.
 *  It is not necessary for this function to actually destroy the data or
 *  return space allocated for this data to other applications, but the event
 *  handler must invalidate the persistent data associated with the given
 *  <IzotPersistentSegmentType> such that a subsequent attempt to read the
 *  data fails.
 *
 *  Use <IzotPersistentDeleteRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotPersistentDeleteFunction)(
    const IzotPersistentSegmentType type);

/*
 *  Callback: IzotPersistentRead
 *  Read a section of a persistent data segment.
 *
 *  Parameters:
 *  handle - handle of the persistent segment (See <IzotPersistentOpenForRead>)
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack during initialization to
 *  read data from persistent storage. An error value is returned if the
 *  specified handle does not exist.
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 *
 *  Use <IzotPersistentReadRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event always fails (no
 *  data available to read).
 */
typedef IzotApiError (*IzotPersistentReadFunction)(const IzotPersistentHandle handle,
    const size_t offset, const size_t size, void * const pBuffer);


/*
 *  Callback: IzotPersistentWrite
 *  Write a section of a persistent data segment.
 *
 *  Parameters:
 *  handle - handle of the persistent segment (See <IzotPersistentOpenForWrite>)
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pData - pointer to the data to write into the segment
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack after changes have been
 *  made to data that is backed by persistent storage. Note that many updates
 *  might have been made, and this function is called only after the stack
 *  determines that all updates have been completed.
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 *
 *  Use <IzotPersistentWriteRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event always fails.
 */
typedef IzotApiError (*IzotPersistentWriteFunction) (const IzotPersistentHandle handle,
    const size_t offset, const size_t size, const void * const pData);

/*
 *  Callback: IzotPersistentIsInTransaction
 *  Returns TRUE if a persistent data transaction was in progress last time
 *  the device shut down.
 *
 *  Parameters:
 *  type - persistent segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack during initialization
 *  prior to reading the segment data. This callback must return TRUE if a
 *  persistent data transaction had been started and never committed.
 *  When this event returns TRUE, the IzoT Device Stack discards the segment,
 *  otherwise, the IzoT stack will attempt to read and apply the persistent
 *  data.
 *
 *  Use <IzotPersistentIsInTransactionRegistrar> to register a handler for
 *  this event. Without an application-specific handler, this event always
 *  returns TRUE.
 */
typedef IzotBool (*IzotPersistentIsInTransactionFunction)(const IzotPersistentSegmentType type);

/*
 *  Callback: IzotPersistentEnterTransaction
 *  Initiate a persistent transaction.
 *
 *  Parameters:
 *  type - persistent segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack when the first request
 *  arrives to update data stored in the specified segment. The event handler
 *  updates the persistent data control structures to indicate that a
 *  transaction is in progress. The stack schedules writes to update the
 *  persistent storage at a later time, then invokes the
 *  <IzotPersistentExitTransaction> event.
 *  When a fatal error occurs during a persistent data storage transaction,
 *  such as a power outage, the transaction control data is used to prevent
 *  the application of invalid persistent data at the time of the next start.
 *
 *  Use <IzotPersistentEnterTransactionRegistrar> to register a handler for
 *  this event. Without an application-specific handler, this event always
 *  fails.
 */
typedef IzotApiError (*IzotPersistentEnterTransactionFunction)(const IzotPersistentSegmentType type);

/*
 *  Callback: IzotPersistentExitTransaction
 *  Complete a persistent transaction.
 *
 *  Parameters:
 *  type - persistent segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stackafter <IzotPersistentWrite>
 *  has returned success and there are no further updates required.
 *  See <IzotPersistentEnterTransactionFunction> for the complementary event.
 *
 *  Use <IzotPersistentExitTransactionRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event always fails.
 */
typedef IzotApiError (*IzotPersistentExitTransactionFunction)(const IzotPersistentSegmentType type);

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
typedef unsigned (*IzotPersistentGetApplicationSegmentSizeFunction)(void);

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
typedef IzotApiError (*IzotPersistentDeserializeSegmentFunction) (
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
typedef IzotApiError (*IzotPersistentSerializeSegmentFunction)(void * const pData, const size_t size);

/*
 * ******************************************************************************
 * SECTION: ASYNCHRONOUS EVENTS
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API event handler
 *  functions.
 *
 *  Remarks:
 *  Like callback functions, event handlers are called from the IzoT Device Stack.
 *  However, unlike callback functions, event handlers are only called in the
 *  context of the application task, and only when the application calls the
 *  <IzotEventPump> function. Also, the application may make IzoT Device Stack
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
 *  Whenever the IzoT device has been reset, the mode of the device is changed
 *  to *online*, but no IzotOnline() event is generated.
 *
 *  Note that resetting the IzoT device only affects the IzoT  protocol
 *  stack and does not cause a processor or application software reset.
 *
 *  Use <IzotResetRegistrar> to register a handler for this event.
 *  Without an application-specific reset event handler, this event does nothing.
 */
typedef void (*IzotResetFunction)(void);

/*
 *  Event: IzotWink
 *  Occurs when the IzoT device receives a WINK command.
 *
 *  Remarks:
 *  This event is not triggered when wink sub-commands (extended install
 *  commands) are received.
 *  Applications are generally required to produce a non-harmful audible or
 *  visual response to a wink event. For example, a device might flash a
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
 *  While the device is offline, the IzoT  protocol stack will not
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
 *  Alternatively, if for any reason the application chooses not to respond to
 *  a request, it must explicitly release the correlator by calling
 *  <IzotReleaseCorrelator>.
 *
 *  Application messages are always delivered to the application, regardless
 *  of whether the message passed authentication or not. It is up to the
 *  application to decide whether authentication is required for any given
 *  message and compare that fact with the authenticated flag. The
 *  authenticated flag is clear (FALSE) for non-authenticated messages and for
 *  authenticated messages that do not pass authentication. The authenticated
 *  flag is set only for correctly authenticated messages.
 *
 *  Use <IzotMsgArrivedRegistrar> to register a handler for this event. Without
 *  an application-specific event handler, the stack automatically releases
 *  application request messages (see <IzotReleaseCorrelator>) and does nothing
 *  else.
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
 *  acknowledged messages, the IzoT  protocol stack calls
 *  <IzotMsgCompleted> with *success* set to TRUE after receiving
 *  acknowledgments from all of the destination devices, and calls
 *  <IzotMsgCompleted> with *success* set to FALSE if the transaction timeout
 *  period expires before receiving acknowledgements  from all destinations.
 *  Likewise, for request messages, the transaction is considered successful
 *  when the IzoT  protocol stack receives a response from each of the
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
 *  Applications are generally required to provide a "service LED," even
 *  though the implementation may not use a physical LED. Other visual
 *  indicators, such as a designated area on a LCD display, may also be
 *  used to implement the "service LED" functionality.
 *
 *  The service LED must be able to signal the configured state (LED off),
 *  unconfigured state (LED flashing at 0.5 Hz) or application-less state
 *  (LED solid on). Many applications override these states with additional
 *  information, for example when implementing the ISI connect LED
 *  functionality with the same physical hardware.
 *
 *  Use <IzotServiceLedStatusRegistrat> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing.
 */
typedef void (*IzotServiceLedStatusFunction)(
    IzotServiceLedState state, IzotServiceLedPhysicalState physicalState);

/*
 *  Event: IzotFilterMsgArrived
 *  Occurs when an application message arrives.
 *  This API is available with Izot Device Stack EX.
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
 *  This API is available with Izot Device Stack EX.
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
 *  This API is available with Izot Device Stack EX.
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

/*
 * *****************************************************************************
 * SECTION: LON PROTOCOL CONSTANT DEFINITIONS
 * *****************************************************************************
 */

#define UNIQUE_NODE_ID_LEN 6   /* Length of the unique node ID          */
#define ID_STR_LEN         8   /* Length of the program id string       */
#define AUTH_KEY_LEN       6   /* Length of the authentication key      */
#define OMA_KEY_LEN       12   /* Length of the OMA authentication key  */
#define DOMAIN_ID_LEN      6   /* Maximum length for a domain id        */
#define LOCATION_LEN       6   /* Maximum length for location string    */
#define NUM_COMM_PARAMS    7   /* Max # of parameters for a transceiver */
#define PROTOCOL_VERSION   0   /* Protocol version                      */
#define MAX_DOMAINS        2   /* Maximum # of domains allowed          */

/* Set the size of the array to log error messages from the LON Stack.
   The error messages wrap around, if there are too many errors.
   Errors seldom happen. So, there is no need for this to be too large. */
#define ERROR_MSG_SIZE  1000   /* 20 messages each with 50 chars        */

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
#define MAX_NV_LENGTH 50

/* Maximum number of network input variables that can
    be scheduled to be polled at any point in time */
#define MAX_NV_IN     50

/* Maximum number of bytes in data array for msg_in, msg_out, resp_in etc.
    This value is indepedent of application buffer sizes mentioned
    earlier. Clearly it does not make sense for this value to be
    larger than application buffer size (out or in). */
#define MAX_DATA_SIZE 255

/* Maximum size of message on the wire (approximate, might be over sized a byte or two for safety.) */
#define MAX_PDU_SIZE (MAX_DATA_SIZE+21)

/*********************************************************************
   NUM_ADDR_TBL_ENTRIES:
   The maximum supported value is 255.
**********************************************************************/
#define NUM_ADDR_TBL_ENTRIES   254     /* # of address table entries  */

#define RECEIVE_TRANS_COUNT    16      /* Can be > 16 for Ref. Impl */

#define NV_TABLE_SIZE          254     /* Check management tool for any restriction on maximum size */

#define NV_ALIAS_TABLE_SIZE    254     /* Check management tool for any restriction on maximum size */

/*******************************************************************************
    The LON Stack uses an array to allocate storage space dynamically.
    The size of the array used for this allocation is determined
    by this constant. If it is too low, it may be impossible
    to allocate necessary buffers or data structures.  If it is
    too high, some memory is unused.  To determine an appropriate value,
    set MALLOC_SIZE to some high value, run a test application, stop, and
    check gp->mallocUsedSize to determine the current usage.
    This array space is allocated during reset of all layers.
    Tracing through the reset code of all layers will indicate
    the approximate size of this array necessary.
    If AllocateStorage function in node.c is rewritten to use malloc, then
    this constant will not be used.
*******************************************************************************/
#define MALLOC_SIZE     10050

/*------------------------------------------------------------------------------
 LON/IP constants.
 ------------------------------------------------------------------------------*/
#define BROADCAST_PREFIX       0xEFC00000
#define IP_ADDRESS_LEN         4
#define MAX_NV_LEN_SUPPORTED   228
#define IBOL_FINISH            0xFF

/*-------------------------------------------------------------------
    Queue entry structure.
  -------------------------------------------------------------------*/
typedef struct {
    IzotUbits16 queueCnt;   /* Max number of items in queue. i.e capacity */
    IzotUbits16 queueSize;  /* Number of items currently in queue         */
    IzotUbits16 itemSize;   /* Number of bytes for each item in queue     */
    IzotByte *head;         /* Pointer to the head item of the queue      */
    IzotByte *tail;         /* Pointer to the tail item of the queue      */
    IzotByte *data;         /* Array of items -- Allocated during Init    */
} Queue;

// Structure: LonTimer
typedef struct __attribute__ ((packed))
{
	IzotUbits32	expiration;		// Time to expire
	IzotUbits32	repeatTimeout;  // Repeat timeout on expiration (0 means not repeating)
} LonTimer;

// Structure: LonWatch
typedef struct
{
	IzotUbits32	start;			// Time watch started
} LonWatch;


#endif /* _IZOT_TYPES_H */
