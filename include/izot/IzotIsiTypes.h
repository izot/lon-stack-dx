/*
 * IzotIsiTypes.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Interoperable Self-Installation (ISI) API Types
 * Purpose: Defines LON ISI types for a LON stack.
 */

#ifndef _ISI_TYPES_H
#define  _ISI_TYPES_H

#include "izot/IzotPlatform.h"

#define ID_STR_LEN 8            // LON program ID length

// ISI messages codes (see isi_msg.h)
typedef IZOT_ENUM_BEGIN(IsiMessageCode) {
    isiDrum    = 0x00,    	    //  Domain resource usage information
	isiDrumEx  = 0x01,		    // 	Extended domain resource usage information (must be isiDrum+1)
    isiCsmo    = 0x02,     	    //  Connections: open enrollment
	isiCsmoEx  = 0x03,		    // 	Extended connection open enrollment (must be isiCsmo+1)
    isiCsma    = 0x04,     	    //  Connections: automatic open enrollment
	isiCsmaEx  = 0x05,		    //  Extended automatic open enrollment (must be isiCsma+1)
    isiCsmr    = 0x06,     	    //  Connections: automatic enrollment reminder
	isiCsmrEx  = 0x07,		    //  Extended automatic open enrollment reminder (must be isiCsmr+1)

	isiLastEx  = isiCsmrEx,	    //  Last extended command

    isiDidrq   = 0x08,    	    //  Domain ID Request
    isiDidrm   = 0x09,    	    //  Domain ID Response
    isiDidcf   = 0x0A,    	    //  Domain ID Confirmation
    isiTimg    = 0x0B,     	    //  Timing guidance message
    isiCsmx    = 0x0C,     	    //  Connections: cancel enrollment
    isiCsmc    = 0x0D,     	    //  Connections: close and confirm enrollment
    isiCsme    = 0x0E,     	    //  Connections: enrollment acceptance
    isiCsmd    = 0x0F,     	    //  Connections: connection deletion
    isiCsmi    = 0x10,     	    //  Connections: status and resource info
    isiCtrq    = 0x11,          //  Controlled enrollment control request
    isiCtrp    = 0x12,          //  Controlled enrollment control response
    isiRdct    = 0x13,          //  Controlled enrollment read connection table request
    isiRdcs    = 0x14,          //  Controlled enrollment read connection table success
    isiRdcf    = 0x15,          //  Controlled enrollment read connection table failure    
	//	Following are helpers, not codes:
	isiLastCode = isiRdcf,
	isiCodeMask = 0x1F
} IZOT_ENUM_END(IsiMessageCode);

// ISI message header
typedef IZOT_STRUCT_BEGIN(IsiMessageHeader) {
    IZOT_ENUM(IsiMessageCode)  Code;
} IZOT_STRUCT_END(IsiMessageHeader);

// ISI domain ID request message
typedef IZOT_STRUCT_BEGIN(IsiDidrq) {
    IzotByte  NeuronId[IZOT_UNIQUE_ID_LENGTH];  // requestor's neuron id
    IzotByte    Nuid;                           // requestor's non-unique id
} IZOT_STRUCT_END(IsiDidrq);

// ISI domain ID response and confirmations messages
// Use the ISI_DID_LENGTH_* macros to access the length field in
// IsiDidrm.Attributes1, IsiDidcf.Attributes1
#define ISI_DID_LENGTH_MASK		0xE0
#define ISI_DID_LENGTH_SHIFT	5
#define ISI_DID_LENGTH_FIELD	Attributes1

typedef IZOT_STRUCT_BEGIN(IsiDidrm) {
    IzotByte     Attributes1;    // contains domain ID length: 1, 3, or 6.  See ISI_DID_LENGTH_* macros 
    IzotDomainId DomainId;       // primary domain ID to use
    IzotUniqueId NeuronId;       // DAS's neuron id
    IzotByte     DeviceCountEstimate;
    IzotByte     ChannelType;
} IZOT_STRUCT_END(IsiDidrm);

typedef IsiDidrm IsiDidcf;

// ISI domain resource usage message (DRUM/DRUMEX)
// Use the ISI_DRUM_DIDLENGTH_* macros to access the DidLength field in IsiDrum.Attributes1
#define ISI_DRUM_DIDLENGTH_MASK		0xE0
#define ISI_DRUM_DIDLENGTH_SHIFT	5
#define ISI_DRUM_DIDLENGTH_FIELD	Attributes1

// Use the ISI_DRUM_USER_* macros to access the UserDefined field in IsiDrum.Attributes1.
#define ISI_DRUM_USER_MASK		0x03
#define ISI_DRUM_USER_SHIFT	    0
#define ISI_DRUM_USER_FIELD	    Attributes1

typedef IZOT_STRUCT_BEGIN(IsiDrum) {
    IzotByte        Attributes1;    // Domain ID length: 1, 3, or 6, and used-defined code;
                                    // see ISI_DRUM_* macros 
	IzotDomainId    DomainId;       // Sender's primary domain ID
	IzotUniqueId    NeuronId;       // Sender's unique id
	IzotSubnetId    SubnetId;
	IzotByte        NodeId;
	IzotByte        Nuid;
	IzotByte        ChannelType;
    IZOT_STRUCT_NESTED_BEGIN(Extended) {
        IzotWord DeviceClass;
	    IzotByte Usage;
    } IZOT_STRUCT_NESTED_END(Extended);
} IZOT_STRUCT_END(IsiDrum);

// LON ISI timing guidance message (TIMG)
// Use the ISI_DRUM_DIDLENGTH_* macros to access the DidLength field in IsiDrum.Attributes1.
#define ISI_TIMG_ORIG_MASK		0xF0
#define ISI_TIMG_ORIG_SHIFT     4
#define ISI_TIMG_ORIG_FIELD	    Attributes1

typedef IZOT_STRUCT_BEGIN(IsiTimg) {
    IzotByte    Attributes1;         // Contains 8 for DAS; see ISI_TIMG_ORIG_* macros 
    IzotByte    DeviceCountEstimate;
    IzotByte    ChannelType;
} IZOT_STRUCT_END(IsiTimg);

// Host's unique ID based on the LON unique ID with the last byte removed
typedef IzotByte  HostUniqueId[IZOT_UNIQUE_ID_LENGTH-1];

/*
 *  Structure representing the unique connection ID.
 *  This structure contains the unique connection ID for a connection.
 */
typedef IZOT_STRUCT_BEGIN(IsiCid) {
    HostUniqueId UniqueId;    // host's unique ID (derived from Neuron ID)
    IzotWord      SerialNumber;
} IZOT_STRUCT_END(IsiCid);

/*
 *  Structure representing the connection header.
 *  Following the ISI Message Header, all connection-related messages start with
 *  this structure.
 */
typedef IZOT_STRUCT_BEGIN(IsiConnectionHeader) {
    IsiCid      Cid;
    IzotWord     Selector;
} IZOT_STRUCT_END(IsiConnectionHeader);

/* 
 *  Enumeration for scope of a connection message.
 *	This enumeration represents the different values that can be used in
 *  the scope field of a CSMO message. This value represents the scope of the
 *  resource file containing the functional profile and datapoint type
 *  definitions specified by the Profile and DpType fields.
 */
typedef IZOT_ENUM_BEGIN(IsiScope) {
    isiScopeStandard = 0,
    isiScopeManufacturer = 3
} IZOT_ENUM_END(IsiScope);

/* 
 *	This enumeration represents the direction of the datapoint
 *  on offer in a CSMO.
 */
typedef IZOT_ENUM_BEGIN(IsiDirection) {
    isiDirectionOutput = 0,
    isiDirectionInput,
    isiDirectionAny,
    isiDirectionVarious
} IZOT_ENUM_END(IsiDirection);

typedef IzotByte  ApplicationId[IZOT_PROGRAM_ID_LENGTH-1];

/*
 *  This structure contains the fields of a CSMO message to be sent by the
 *  ISI engine.
 */

/*
 * Use the ISI_CSMO_DIR_* macros to access the Direction field in IsiCsmoData.Attributes1
 */
#define ISI_CSMO_DIR_MASK		0xC0
#define ISI_CSMO_DIR_SHIFT		6
#define ISI_CSMO_DIR_FIELD		Attributes1

/*
 * Use the ISI_CSMO_WIDTH_* macros to access the Width field in IsiCsmoData.Attributes1
 */
#define ISI_CSMO_WIDTH_MASK		0x3F
#define ISI_CSMO_WIDTH_SHIFT	0
#define ISI_CSMO_WIDTH_FIELD	Attributes1

/*
 * Use the ISI_CSMO_ACK_* macros to access the Acknowledged field in IsiCsmoData.Attributes2
 */
#define ISI_CSMO_ACK_MASK		0x80
#define ISI_CSMO_ACK_SHIFT		7
#define ISI_CSMO_ACK_FIELD		Attributes2

/*
 * Use the ISI_CSMO_POLL_* macros to access the Poll field in IsiCsmoData.Attributes2
 */
#define ISI_CSMO_POLL_MASK		0x40
#define ISI_CSMO_POLL_SHIFT		6
#define ISI_CSMO_POLL_FIELD		Attributes2

/*
 * Use the ISI_CSMO_SCOPE_* macros to access the Scope field in IsiCsmoData.Attributes2
 */
#define ISI_CSMO_SCOPE_MASK		0x30
#define ISI_CSMO_SCOPE_SHIFT	4
#define ISI_CSMO_SCOPE_FIELD	Attributes2

typedef IZOT_STRUCT_BEGIN(IsiCsmoData) {
    /* The group (or: device category) that this connection applies to. */
    IzotByte		Group;
	IzotByte		Attributes1;	/* contains Direction, Width. See ISI_CSMO_DIR_* and _WIDTH_* macros */
    /* Functional profile number of the functional profile that defines the 
       functional block containing this input or output, or zero if none. */
	IzotWord		Profile;
    /* Dp type index of the Dp type for the datapoint, or zero if none
       specified. The Dp type index is an index into resource file that defines the
       datapoint type for the datapoint on offer. */
    IzotByte	    DpType;
    /* Variant number for the offered datapoint. Variants can be defined for
       any functional profile/member number pair. */
    IzotByte	    Variant;
	IZOT_STRUCT_NESTED_BEGIN(Extended) 
	{
        IzotByte		Attributes2;	/* contains Ack, Poll, Scope. See ISI_CSMO_ACK_*, _POLL_* and _SCOPE_* macros */
        /* The first 6 bytes of the connection host�s standard program ID. 
           The last two standard program ID bytes (channel type and model
           number) are not included. */
		IzotByte		Application[IZOT_PROGRAM_ID_LENGTH - 2];
        /* Dp member number within the functional block, or zero if none. */
		IzotByte	    Member;
    } IZOT_STRUCT_NESTED_END(Extended);    
} IZOT_STRUCT_END(IsiCsmoData);

/*
 *  Structure representing the manual open enrollment message (CSMO).
 */
typedef IZOT_STRUCT_BEGIN(IsiCsmo) {
    IsiConnectionHeader Header;
    IsiCsmoData         Data;
} IZOT_STRUCT_END(IsiCsmo);

// Literals for accessing Desc field of IsiCsmi without using bitfields (for efficiency)
#define CsmiOffset_SHIFT	2
#define CsmiOffset_MASK     0xFC
#define CsmiOffset_FIELD    Attributes1
#define CsmiCount_SHIFT	    0
#define CsmiCount_MASK  	0x03
#define CsmiCount_FIELD  	Attributes1


typedef IZOT_STRUCT_BEGIN(CsmiDesc) {
    IzotByte Attributes1;  // contains Offset,  Count
} IZOT_STRUCT_END(CsmiDesc);

/*
 *  Structure representing the enrollment information message (CSMI) sent by the ISI engine.
 */
typedef IZOT_STRUCT_BEGIN(IsiCsmi) {
    IsiConnectionHeader Header;
	IZOT_UNION_BEGIN(Desc) 
    {        
		CsmiDesc Bf;
		IzotByte OffsetCount;
    } IZOT_UNION_END(Desc);
} IZOT_STRUCT_END(IsiCsmi);

typedef IsiConnectionHeader IsiCsmx;
typedef IsiConnectionHeader IsiCsmc;
typedef IsiConnectionHeader IsiCsme;
typedef IsiConnectionHeader IsiCsmd;
typedef IsiCsmo    IsiCsma;
typedef IsiCsmo    IsiCsmr;

/* 
 *  Specifies the requested operation for a controlled enrollment request contained in 
 *  a control request (CTRQ) message.
 */
typedef IZOT_ENUM_BEGIN(IsiControl) {
    isiNoop     =   0,
    isiOpen     =   1,
    isiCreate   =   2,
    isiExtend   =   3,
    isiCancel   =   4,
    isiLeave    =   5,
    isiDelete   =   6,
    isiFactory  =   7
} IZOT_ENUM_END(IsiControl);

typedef IZOT_STRUCT_BEGIN(IsiCtrq) {
    IZOT_ENUM(IsiControl) Control;
    IzotByte    Parameter;
} IZOT_STRUCT_END(IsiCtrq);

typedef IZOT_STRUCT_BEGIN(IsiCtrp) {
    IzotByte         Success;
    IzotUniqueId     NeuronID;
} IZOT_STRUCT_END(IsiCtrp);

//  IsiConnectionTable
//  The connection table state values are an ordered enumeration, with
//  isiConnectionStateUnused < isiConnectionStatePending < isiConnectionStateInUse
typedef IZOT_ENUM_BEGIN(IsiConnectionState) {
    isiConnectionStateUnsed = 0,
    isiConnectionStatePending,
    isiConnectionStateInUse,
    isiConnectionStateTcsmr
} IZOT_ENUM_END(IsiConnectionState);

// Literals for accessing Desc field of IsiConnection without using bitfields (for efficiency)
#define ConnectionOffset_SHIFT	2
#define ConnectionOffset_MASK	0xFC
#define ConnectionOffset_FIELD	Attributes1
#define ConnectionAuto_SHIFT		1
#define ConnectionAuto_MASK		0x02
#define ConnectionAuto_FIELD    Attributes1

typedef IZOT_STRUCT_BEGIN(ConnDesc) {
    IzotByte        Attributes1;  // contains Offset, Auto
} IZOT_STRUCT_END(ConnDesc);

/*
 *  Structure representing a row in the connection table.
 *  This structure is used to represent a row in the connection table that
 *  is returned by <IsiGetConnection> and is used in <IsiSetConnection> to set a
 *  row in the table.
 */

/*
 * Use the ISI_CONN_STATE_* macros to access the State field in IsiConnection.Attributes1
 */
#define ISI_CONN_STATE_MASK		0xC0
#define ISI_CONN_STATE_SHIFT	6
#define ISI_CONN_STATE_FIELD	Attributes1

 /*
 * Use the ISI_CONN_EXTEND_* macros to access the Extend field in IsiConnection.Attributes1
 */
#define ISI_CONN_EXTEND_MASK	0x20
#define ISI_CONN_EXTEND_SHIFT	5
#define ISI_CONN_EXTEND_FIELD	Attributes1

/*
 * Use the ISI_CONN_CSME_* macros to access the State field in IsiConnection.Attributes1
 */
#define ISI_CONN_CSME_MASK		0x10
#define ISI_CONN_CSME_SHIFT		4
#define ISI_CONN_CSME_FIELD	Attributes1

/*
 * Use the ISI_CONN_WIDTH_* macros to access the State field in IsiConnection.Attributes1
 */
#define ISI_CONN_WIDTH_MASK		0x0F
#define ISI_CONN_WIDTH_SHIFT	0
#define ISI_CONN_WIDTH_FIELD	Attributes1

typedef IZOT_STRUCT_BEGIN(IsiConnection) {
    IsiConnectionHeader Header;
    IzotByte    Host;           //  local assembly that is hosted here, or ISI_NO_ASSEMBLY if this is not the host for this connection
    IzotByte    Member;         //  local assembly that is enrolled in this connection, or ISI_NO_ASSEMBLY if none.
    IzotByte		Attributes1;	/* contains state, extend, csme, width. See ISI_CONN_STATE_*, _EXTEND_*, _CSME_* and _WIDTH_* macros */
    IZOT_UNION_BEGIN(Desc) 
	{
		ConnDesc Bf;
		IzotByte OffsetAuto;
    } IZOT_UNION_END(Desc);
} IZOT_STRUCT_END(IsiConnection);

typedef IZOT_STRUCT_BEGIN(IsiRdct) {
    IzotByte    Index;
    IzotByte    Host;
    IzotByte    Member;
} IZOT_STRUCT_END(IsiRdct);

typedef IZOT_STRUCT_BEGIN(IsiRdcs) {
    IzotByte    Index;
    IsiConnection Data;
} IZOT_STRUCT_END(IsiRdcs);

//
//  ISI Message Structure
//
typedef IZOT_STRUCT_BEGIN(IsiMessage) {
    IsiMessageHeader Header;
    IZOT_UNION_BEGIN(Msg) 
    {
        IsiDidrq   Didrq;
        IsiDidrm   Didrm;
        IsiDidcf   Didcf;
        IsiTimg    Timg;
        IsiDrum    Drum;
        IsiCsmo    Csmo;
        IsiCsmx    Csmx;
        IsiCsmc    Csmc;
        IsiCsmd    Csmd;
        IsiCsme    Csme;
        IsiCsmi    Csmi;
        IsiCsma    Csma;
        IsiCsmr    Csmr;
        IsiCtrq    Ctrq;
        IsiCtrp    Ctrp;
        IsiRdct    Rdct;
        IsiRdcs    Rdcs; 
    } IZOT_UNION_END(Msg);
} IZOT_STRUCT_END(IsiMessage);

// The following constant defines the 709.1 (Izot) application message code used
// with all ISI management messages. The structure of these messages is IsiMessage,
// as defined above, which contains an ISI specific IsiMessageCode to further 
// distringuish the individual ISI message types. 
// DO NOT CONFUSE THE 709.1 APPLICATION MESSAGE CODE WITH THE IsiMessageCode enumeration!
extern const IzotByte isiApplicationMessageCode;

//  Basic ISI API macros
#define ISI_DEFAULT_GROUP   128u
#define ISI_NO_ASSEMBLY     255u
#define ISI_NO_INDEX        255u
#define ISI_TICKS_PER_SECOND   4u
#define ISI_DEFAULT_CONTAB_SIZE     32u		// was 8
#define ISI_DEFAULT_REPEATS			3
#define ISI_DEFAULT_DOMAIN_ID       { 'I', 'S', 'I' }
#define ISI_DEFAULT_DOMAIN_ID_LEN   3u


//  ***********************************************************************
//  Basic ISI entry points
//  ***********************************************************************

//  Starting / Stopping / Running the ISI engine. Use a combination of the
//  IsiFlags with the Start functions.
typedef IZOT_ENUM_BEGIN(IsiFlags) {
    isiFlagNone               =   0x00,   // does nothing
    isiFlagExtended 		  =   0x01,   // enables use of extended DRUM and enrollment messages
    isiFlagHeartbeat          =   0x02,   // enables ISI Dp heartbeats
    isiFlagApplicationPeriodic =  0x04,	  // enables IsiApplicationPeriodic()
	isiFlagSupplyDiagnostics  =   0x08,   // enables UpdateDiagnostics callback
    isiControlledEnrollment   =   0x10,   // enables controlled enrollment  
    isiFlagDisableAddrMgmt    =   0x20    // always assign a randomly allocated primary address
} IZOT_ENUM_END(IsiFlags);

typedef IZOT_ENUM_BEGIN(IsiType) {
    isiTypeS,                       //  use for ISI-S and ISI-S/C
    isiTypeDa,                      //  use for ISI-DA and ISI-DA/C
    isiTypeDas                      //  use for ISI-DAS and ISI-DAS/C
} IZOT_ENUM_END(IsiType);

typedef IZOT_ENUM_BEGIN(IsiEvent){
    isiNormal    =   0,		// Some code ASSUMES this value.
    isiRun       =   1,
    //  following are events related to connection enrollment:
    isiPending,
    isiApproved,
    isiImplemented,
    isiCancelled,
    isiDeleted,
    isiWarm,
    isiPendingHost,
    isiApprovedHost,
    //  following are events related to domain and device acquisition:
    isiAborted, // see param for IsiAbortReason detail
    isiRetry,   // see param for remaining number of retries
    isiWink,    // device should perform Wink operation
    isiRegistered   // successful start (param 0) or completion (param 0xFF) of device or domain acquisition
} IZOT_ENUM_END(IsiEvent);

typedef IZOT_ENUM_BEGIN(IsiAbortReason) {
    // Abort reasons from the process that controls the domain acquisition
    // process on a ISI-DA device:
    isiAbortUnsuccessful = 1,       // Abort domain acq. after 20 retries
    isiAbortMismatchingDidrm,       // Abort domain acq. due to arrival of mismatching DIDRM
    isiAbortMismatchingDidcf,       // Abort domain acq. due to arrival of mismatching DIDCF
    isiAbortMismatchService         // Abort domain acq. due to mismatching confirmation service msg
} IZOT_ENUM_END(IsiAbortReason);

extern void IsiUpdateUserInterface(IsiEvent Event, IzotByte Parameter);

typedef IZOT_ENUM_BEGIN(IsiDiagnostic){
    isiSubnetNodeAllocation = 1,
    isiSubnetNodeDuplicate,
    isiReceiveDrum = 4,
    isiReceiveTimg,
    isiSendPeriodic,
    isiSelectorDuplicate,
    isiSelectorUpdate,
    isiReallocateSlot
} IZOT_ENUM_END(IsiDiagnostic);

// Get and set bit values
#define GET_BITS_VALUE(field, MASK, SHIFT)          ((field & MASK) >> SHIFT)
#define SET_BITS_VALUE(field, MASK, SHIFT, value)   field = (field & ~MASK) | (value << SHIFT)

// Type definitions for callback and event handers
typedef void (*IsiCreateCsmoFunction)(unsigned Assembly, IsiCsmoData* const pCsmoData);
typedef IzotBool (*IsiCreatePeriodicMsgFunction) (void);
typedef unsigned (*IsiGetAssemblyFunction)(const IsiCsmoData* pCsmoData, IzotBool Auto, unsigned Assembly);
typedef unsigned (*IsiGetDpIndexFunction)(unsigned Assembly, unsigned Offset, unsigned PreviousIndex);
typedef unsigned (*IsiGetPrimaryGroupFunction)(unsigned Assembly);
typedef unsigned (*IsiGetWidthFunction)(unsigned Assembly);
typedef IzotBool (*IsiQueryHeartbeatFunction) (unsigned DpIndex);
typedef IzotBool (*IsiUpdateDiagnosticsFunction) (IsiDiagnostic Event, IzotByte Parameter);
typedef void (*IsiUpdateUserInterfaceFunction) (IsiEvent Event, IzotByte Parameter);
typedef void (*IzotIsiLightConnectedFunction)(void);
typedef struct {
    IsiCreateCsmoFunction               createCsmo; 
    IsiCreatePeriodicMsgFunction        createPeriodicMsg;
    IsiGetAssemblyFunction              getAssembly;
    IsiGetDpIndexFunction               getDpIndex;
    IsiGetPrimaryGroupFunction          getPrimaryGroup;
    IsiGetWidthFunction                 getWidth;
    IsiQueryHeartbeatFunction           queryHeartbeat;
    IsiUpdateDiagnosticsFunction        updateDiagnostics;
    IsiUpdateUserInterfaceFunction      updateUserInterface;

}   IsiCallbackVectors;

#endif  //  !defined _ISI_TYPES_H
