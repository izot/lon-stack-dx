//
// iup.h
//
// Copyright (C) 2023 EnOcean
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
 * Title: Image Update Protocol header file
 *
 * Abstract:
 * This header file contains the Network Management function declarations for
 * the LON Image Update protocol
 */
#ifndef _IUP_H
#define _IUP_H

#include <stdio.h>
#include <string.h>
#include "IzotConfig.h"
#include "IzotApi.h"
#include "IzotHal.h"
#include "endian.h"
#include "iup.h"
#include "lcs_api.h"
#include "lcs_netmgmt.h"
#include "lcs_timer.h"
#include "lcs_node.h"

#if PROCESSOR_IS(MC200)
    #include <wmstdio.h>
    #include <rfget.h>
    #include <flash.h>
    #include <partition.h>
#endif

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
// Expanded NM command for IUP
#define NME_IUP_INIT				0x1C
#define NME_IUP_TRANSFER			0x1D
#define NME_IUP_CONFIRM				0x1E
#define NME_IUP_VALIDATE			0x1F
#define NME_IUP_SWITCHOVER			0x20
#define NME_IUP_STATUS				0x21
#define NME_IUP_COMMIT				0x22
#define NME_IUP_ACK_TRANSFER 		0x25

// Image Types
#define  NEURON_SYSTEM_IMAGE			0x00
#define	 NEURON_APPLICATION				0x01
#define	 NEURON_DSP_IMAGE				0x02
#define	 NEURON_DATA_IMAGE				0x03
#define	 HOST_PROCESSOR_SYSTEM_IMAGE	0x80
#define	 HOST_PROCESSOR_APP_IMAGE		0x81
#define	 HOST_PROCESSOR_DSP_IMAGE		0x82
#define	 HOST_PROCESSOR_DATA_IMAGE		0x83
#define	 HOST_PROCESSOR_COMBINED_IMAGE	0x84
// 0xC0-0xFF		- Host specific image types

// Result Code for Init Request
#define IUP_INIT_RESULT_SUCCESS					0x00
#define IUP_INIT_RESULT_STILL_PENDING			0x01
#define IUP_INIT_RESULT_LARGE_PACKET_SIZE		0x02
#define IUP_INIT_RESULT_INVALID_IMAGE_TYPE		0x03
#define IUP_INIT_RESULT_INVALID_IMAGE_SUBTYPE	0x04
#define IUP_INIT_RESULT_VERSION_INCOMPATIBLE    0x05
#define IUP_INIT_RESULT_MODEL_INCOMPATIBLE      0x06
#define IUP_INIT_RESULT_IMAGE_TOO_LARGE         0x07
#define IUP_INIT_RESULT_PACKET_COUNT_TOO_HIGH   0x08

// Time required to init the image update process
#define IUP_INIT_IMAGE_UPDATE_INIT_TIMER		10   //seconds

// Result code for Confirm Request
#define IUP_CONFIRM_RESULT_SUCESS				0x00
#define IUP_CONFIRM_RESULT_STILL_PENDING		0x01
#define IUP_CONFIRM_RESULT_PACKET_MISSED		0x02
#define IUP_CONFIRM_RESULT_IMAGE_NOT_VIABLE		0x03

//
// digestType
//
#define DIGEST_TYPE_NONE	0x00 
#define DIGEST_TYPE_MD5		0x01 
#define DIGEST_TYPE_SHA256	0x02
#define DIGEST_TYPE_NATIVE  0x03

#define DIGEST_TYPE_SUPPORTED DIGEST_TYPE_MD5

#define SALT_LENGTH       16
#define MD5_DIGEST_LENGTH 16

//
// resultCode for validate request
//
// Success
#define IUP_VALIDATE_RESULT_SUCCESS						0x00
#define IUP_VALIDATE_RESULT_STILL_PENDING				0x01
#define IUP_VALIDATE_RESULT_VERSION_INCOMPATIBLE		0x02
#define IUP_VALIDATE_RESULT_MODEL_INCOMPATIBLE			0x03
#define IUP_VALIDATE_RESULT_INVALID_DIGEST				0x04
#define IUP_VALIDATE_RESULT_INVALID_DIGITAL_SIGNATURE	0x05
#define IUP_VALIDATE_RESULT_DIGEST_TYPE_NOT_SUPPORTED	0x06
#define IUP_VALIDATE_RESULT_OTHER						0x07 

// Time required to init the image update process
#define IUP_VALIDATE_IMAGE_TIMER		10   //seconds

// Flag below is switchOverflags
#define IUP_isSecondaryFlag(flags)	(0x01 & flags) ? 1 : 0
#define IUP_isPreseveConfig(flags)	(0x02 & flags) ? 1 : 0
#define IUP_isPersistence(flags)	(0x04 & flags) ? 1 : 0

// switchOverflags
// Secondary Flag [Bit 0] - 1 => Image(s) to remain secondary after switchover.
// Preserve Config [Bit 1] - 1 => Preserve the configuration (network image).
// Preserve Persistence [Bit 2]- 1 => Perserve persistent data (CPs, etc).
// Other (Bits 3-7) - These bits must be zero.

//
// resultCode for switch over request
//
#define IUP_SWITCHOVER_RESULT_SUCCESS						0x00
#define IUP_SWITCHOVER_RESULT_VERSION_INCOMPATIBLE			0x01
#define IUP_SWITCHOVER_RESULT_MODEL_INCOMPATIBLE			0x02 
#define IUP_SWITCHOVER_RESULT_SEC_MODE_NOT_SUPPORTED		0x03
#define IUP_SWITCHOVER_RESULT_PRESERVATION_NOT_SUPPORTED	0x04
#define IUP_SWITCHOVER_RESULT_IMAGE_NOT_AVAILABLE			0x05
#define IUP_SWITCHOVER_RESULT_IMAGE_REJECTED				0x06
#define IUP_SWITCHOVER_DELAY_NOT_SUPPORTED                  0x07

//
// rejectionCode
//
#define IUP_STATUS_REJECTION_NONE						0x00
#define IUP_STATUS_REJECTION_VERSION_INCOMPATIBLE		0x01 
#define IUP_STATUS_REJECTION_MODEL_INCOMPATIBLE			0x02
#define IUP_STATUS_REJECTION_SEC_MODE_NOT_SUPPORTED		0x03
#define IUP_STATUS_REJECTION_PRESERVATION_NOT_SUPPORTED	0x04
#define IUP_STATUS_REJECTION_IMAGE_REJECTED				0x05

#define isImgAccepted(flags) (0x01 & flags) ? 1 : 0
#define isImgecondary(flags) (0x02 & flags) ? 1 : 0
#define isImgActive(flags) (0x04 & flags) ? 1 : 0

//Bit 0 - 1=> Image is accepted; 0=> Image was rejected (see rejectionCode)
//Bit 1 - 1=> Image is the secondary image; 0 =>image is the primary image
//Bit 2 - 1=> Image is currently active; 0 =>image is not currently active

// resultCode for commit request
#define IUP_COMMIT_RESULT_SUCCESS				0x00 
#define IUP_COMMIT_RESULT_STILL_PENDING			0x01 
#define IUP_COMMIT_RESULT_IMAGE_NOT_EXIST		0x02
#define IUP_COMMIT_RESULT_IMAGE_ALREADY_PRIMARY 0x03
#define IUP_COMMIT_RESULT_FAILED 				0x04

#define EIGHTY_PERCENT(x)  80*(x)/100

#define EEPROM_NOT_WRITTEN                  0xFF
#define EEPROM_WRITTEN                      0x01
#define EEPROM_BLOCK_SIZE                   0x1000

#define IUP_PERSIST_DATA_VALID              0x01
#define IUP_ERROR                           -1
#define IUP_ERROR_NONE                      0
#define IUP_PACKET_SPACING                  100
#define IUP_PACKET_SIZE_SUPPORTED           240
#define IUP_INIT_FIRMWARE_TIMER_VALUE       500
#define IUP_VALIDATE_FIRMWARE_TIMER_VALUE   100
#define IUP_SWITCHOVER_TIMER_VALUE          1000
#define IUP_COMMIT_FIRMWARE_TIMER_VALUE     2000
#define IUP_VALIDATE_RESPONSE_ACTION_TIME    3
#define IUP_SWITCHOVER_RESPONSE_ACTION_TIME  10
#define IUP_COMMIT_RESPONSE_ACTION_TIME      20

#define MAX_PACKET_COUNT_IN_CONFIRM_RESPONSE 20
#define IZOT_RESET_TIME_AFTER_SWITCHOVER      60

/*------------------------------------------------------------------------------
  Section: Type Definitions
  ------------------------------------------------------------------------------*/
typedef IZOT_STRUCT_BEGIN(IUP_ImageIdentifier)
{
	IzotByte imgType;			//	8-bit iamge type : see above
	IzotByte imgSubType;		//	8-bit subtype [image type dependent]
	IzotByte imgInstance[8];	//	8-byte number that specifies the image within this scope
} IZOT_STRUCT_END(IUP_ImageIdentifier);

typedef IZOT_STRUCT_BEGIN(IUP_InitRequest)
{
    IzotByte    subCode;
	uint32_t    sessionNumber;
	IUP_ImageIdentifier imgIdent;
	IzotUbits16 pcktSize;
	IzotUbits16 pcktCount;
	uint32_t    imageLen;
    IzotByte    imageHeader[16];
} IZOT_STRUCT_END(IUP_InitRequest);

typedef IZOT_STRUCT_BEGIN(IUP_TransferRequest)
{
    IzotByte    subCode;
	uint32_t    sessionNumber;
	IzotUbits16 packetNumber;
	IzotByte    data[1];
} IZOT_STRUCT_END(IUP_TransferRequest);

typedef IZOT_STRUCT_BEGIN(IUP_ConfirmRequest)
{
    IzotByte  subCode;
	uint32_t  sessionNumber;
} IZOT_STRUCT_END(IUP_ConfirmRequest);

typedef IZOT_STRUCT_BEGIN(IUP_ValidateRequest)
{
    IzotByte  subCode;
    uint32_t  sessionNumber;
    IzotByte  digestType;
    IzotByte  saltBytes[SALT_LENGTH];
    IzotByte  digestBytes[MD5_DIGEST_LENGTH];
} IZOT_STRUCT_END(IUP_ValidateRequest);

typedef IZOT_STRUCT_BEGIN(IUP_rejectionInfo)
{
	IzotByte dataLen;
	IzotByte type;
	IzotByte data[1];
} IZOT_STRUCT_END(IUP_rejectionInfo);

typedef IZOT_STRUCT_BEGIN(IUP_SwitchOverRequest)
{
    IzotByte subCode;
	IzotByte switchOverflags; 		// switchover flags : see above
	uint32_t switchOverTime;		// A 16-bit countdown in seconds until switchover
	IzotByte imgNumber; 			// 8-bit number (N) of image identifiers to switchover
	IUP_ImageIdentifier imgIdent;	// iamge identifier
} IZOT_STRUCT_END(IUP_SwitchOverRequest);

typedef IZOT_STRUCT_BEGIN(IUP_StatusRequest)
{
    IzotByte subCode;
    IUP_ImageIdentifier imgIdent;
} IZOT_STRUCT_END(IUP_StatusRequest);

typedef IZOT_STRUCT_BEGIN(IUP_CommitRequest)
{
    IzotByte subCode;
    IzotByte imageCount;
    IUP_ImageIdentifier imgIdent;
} IZOT_STRUCT_END(IUP_CommitRequest);

typedef IZOT_STRUCT_BEGIN(IupInitData)
{
    uint32_t	IupSessionNumber;
    uint32_t	IupImageLen;
    IzotUbits16	IupPacketLen;
    IzotUbits16	IupPacketCount;
    IUP_ImageIdentifier IupImageIdentifier;
} IZOT_STRUCT_END(IupInitData);

typedef IZOT_STRUCT_BEGIN(IupPersistent)
{
	IzotByte	iupMode;					// 1 byte
	IupInitData initData;					// 22 byte
	IzotByte 	iupConfirmResultSucceed;    // 1 byte
	IzotByte	iupCommitDone;		     	// 1 byte
    IzotByte    SecondaryFlag;              // 1 byte
} IZOT_STRUCT_END(IupPersistent);

typedef IZOT_STRUCT_BEGIN(IupTransferAckResponse)
{
  IzotByte subCode; 		// 
  IzotByte resultCode; 		// 
  IzotByte actionTime; 		// Time required to finish the action(in seconds)
} IZOT_STRUCT_END(IupTransferAckResponse);

typedef IZOT_ENUM_BEGIN(TransferAckResponse)
{
	TRANSFER_STOP = 0,
	TRANSFER_CONTINUE = 1
} IZOT_ENUM_END(TransferAckResponse);

/*------------------------------------------------------------------------------
  Section: Globals
  ------------------------------------------------------------------------------*/
extern LonTimer  iupInitFirmwareTimer;
extern IzotByte	iupImageValidated;
extern LonTimer  iupValidateFirmwareTimer;
extern LonTimer	iupSwitchOverTimer;
extern IzotByte	iupImageCommited;
extern LonTimer	iupCommitFirmwareTimer;
extern LonTimer  iupMd5EventTimer;

/*------------------------------------------------------------------------------
  Section: Function Prototypes
  ------------------------------------------------------------------------------*/
extern int  InitUpdateProcess(void);
extern int  VerifyImage(void);
extern void SwitchOverImage(void);

#endif	// _IUP_H