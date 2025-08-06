//
// lcs_netmgmt.h
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

/*******************************************************************************
     Reference:        Section 10, ISO/IEC 14908-1

       Purpose:        LON App Layer / Network Management

          Note:        For more information, see netmgmt.c.
*******************************************************************************/
#ifndef _LCS_NETMGMT_H
#define _LCS_NETMGMT_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "abstraction/endian.h"
#include "common/err.h"
#include "abstraction/IzotConfig.h"
#include "izot/IzotApi.h"
#include "abstraction/IzotCal.h"
#include "lcs/iup.h"
#include "lcs/lcs_api.h"
#include "lcs/lcs_app.h"
#include "lcs/lcs_eia709_1.h"
#include "lcs/lcs_timer.h"
#include "lcs/lcs_node.h"
#include "lcs/lcs_queue.h"
#include "lcs/lcs_tsa.h"
#include "lcs/lcs_network.h"

#if PROCESSOR_IS(MC200)
#include <wmstdio.h>
#endif

#ifdef SECURITY_II
#include "SecNmMsgs.h"
#endif

#pragma pack(push, 1)

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
typedef enum
{
    ND_MESSAGE,
    NM_MESSAGE
} NtwkMgmtMsgType;

/* Message codes */

#define NM_EXPANDED			  0x0
#define NM_QUERY_ID           0x1
#define NM_RESPOND_TO_QUERY   0x2
#define NM_UPDATE_DOMAIN      0x3
#define NM_LEAVE_DOMAIN       0x4
#define NM_UPDATE_KEY         0x5
#define NM_UPDATE_ADDR        0x6
#define NM_QUERY_ADDR         0x7
#define NM_QUERY_NV_CNFG      0x8
#define NM_UPDATE_GROUP_ADDR  0x9
#define NM_QUERY_DOMAIN       0xA
#define NM_UPDATE_NV_CNFG     0xB
#define NM_SET_NODE_MODE      0xC
#define NM_READ_MEMORY        0xD
#define NM_WRITE_MEMORY       0xE
#define NM_CHECKSUM_RECALC    0xF
#define NM_INSTALL            0x10
#define NM_WINK               0x10 /* Alias for NM_INSTALL */
#define NM_MEMORY_REFRESH     0x11
#define NM_QUERY_SNVT         0x12
#define NM_NV_FETCH           0x13
#define NM_MANUAL_SERVICE_REQUEST 0x1F

// The expanded command set
#define NME_QUERY_VERSION			  0x01
#define NME_UPDATE_NV_BY_INDEX        0x02
#define NME_UPDATE_DOMAIN_NO_KEY	  0x07
#define NME_REPORT_DOMAIN_NO_KEY	  0x08
#define NME_REPORT_KEY				  0x09
#define NME_UPDATE_KEY				  0x0A
#define NME_INIT_CONFIG				  0x0F

/* Expanded NM command for NV table and Alias Table */
#define NME_UPDATE_NV_CONFIG          0x11
#define	NME_QUERY_NV_CONFIG           0x12
#define NME_UPDATE_NV_ALIAS_CONFIG    0x13
#define	NME_QUERY_NV_ALIAS_CONFIG     0x14

/* Expanded NM command for mapping announcement */
#define NME_QUERY_LS_ADDR_MAPPING_ANNOUNCEMENT      0x18
#define NME_QUERY_IP_ADDRESS						0x19

/* Define offsets and masks for constructing request and response codes */

#define NM_opcode_base      0x60
#define NM_opcode_mask      0x1F
#define NM_resp_mask        0xE0
#define NM_resp_success     0x20
#define NM_resp_failure     0x00

#define ND_opcode_base      0x50
#define ND_opcode_mask      0x0F
#define ND_resp_mask        0xF0
#define ND_resp_success     0x30
#define ND_resp_failure     0x10

/* Network Diagnostic Message Codes */
#define ND_QUERY_STATUS     0x01
#define ND_PROXY_COMMAND    0x02
#define ND_CLEAR_STATUS     0x03
#define ND_QUERY_XCVR       0x04
#define ND_QUERY_EVN_SDI	0x05
#define ND_QUERY_STATUS_FLEX 0x06
#define ND_QUERY_XCVR_BIDIR 0x07
#define ND_COMPUTE_PHASE    0x08
#define ND_GET_FULL_VERSION 0x09

// Foreign Codes 
#define LT_APDU_ENHANCED_PROXY 0x4D

/*------------------------------------------------------------------------------
Section: Type Definitions
------------------------------------------------------------------------------*/
typedef enum {
    ABSOLUTE_MEM_ADDR  = 0,
    READ_ONLY_RELATIVE = 1,
    CONFIG_RELATIVE    = 2,
    STAT_RELATIVE      = 3,
	
	RFLOG_RELATIVE	   = 253,	// Used to access RF Event log data (see rflog.h)
	DBG_RELATIVE       = 254,   // Used to access DBG specific data (see stacktrace.h/.c)
	MFG_RELATIVE	   = 255,	// Used to access MFG specific extensions such as a flash info block.
} ModeType;

typedef enum
{
    UNCONFIGURED   = 0,
    SELECTED       = 1,
    SELECTED_UNCFG = 2,   /* selected *AND* unconfigured */
} Selector;

typedef enum
{
    NO_ACTION            = 0,
    BOTH_CS_RECALC       = 1,
    CNFG_CS_RECALC       = 4,
    ACTION_RESET         = 8,
} Form;

typedef struct
{
    IzotByte   stats[10]; // First 5 statistics
    IzotByte   resetCause;
    IzotByte   nodeState;
    IzotByte   versionNumber;
    IzotByte   errorLog;
    IzotByte   modelNumber;
} NDQueryStat;

typedef struct {
    NetEnum(Selector) selector;
    NetEnum(ModeType) mode;
    IzotUbits16       offset;
    IzotByte          count;
    IzotByte          data[MAX_DATA_SIZE];
} NMQueryIdRequest;

typedef struct {
    NetEnum(ModeType) mode;
    IzotUbits16       offset;
    IzotByte          count;
    Form              form;
    IzotByte          data[MAX_DATA_SIZE];
} NMWriteMemoryRequest;

// Capabilities reported by query version
#define NMV_OMA			0x01
#define NMV_PROXY		0x02
#define NMV_PHASE		0x04
#define NMV_SSI			0x08

#define NMV_INITCONFIG	       0x10
#define NMV_READ_MEMORY_MAP    0x20
#define NMV_UPDATE_NV_BY_INDEX 0x40
#define NVM_SECURITY_II        0x80

#define NMV_LS_MODE_COMPATIBILITY_ONLY           0x00 
#define NMV_LS_MODE_ENHANCED_ONLY                0x01
#define NMV_LS_MODE_COMPATIBILITY_OR_ENHANCED    0x02
#define NMV_LS_MODE_RSVD                         0x03

#define NMV_LSIP_ADDR_MAPPING_ANNOUNCEMENTS      0x04

typedef enum 
{
	NDQB_ATTENUATE = 	0x01,
} NDQueryBidirFlags;

typedef struct
{
    IzotByte   version;
    IzotByte   variant;
    IzotByte   rom_version;
    IzotByte   minor_version;
    IzotByte   build_number;
    IzotByte   code;
} ND_get_full_version_response;
#pragma pack(pop)

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/
void HandleNM(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr);
void HandleND(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr);
void HandleProxyResponse(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr);

#endif	// _LCS_NETMGMT_H

