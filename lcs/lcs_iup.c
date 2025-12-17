/*
 * lcs_iup.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Image Update Protocol (IUP) Implementation
 * Purpose: Provides functions to support the LON Image Update Protocol (IUP).
 */

#include <string.h>
#include "lcs/lcs_iup.h"

#define F1(x, y, z)       (z ^ (x & (y ^ z)))
#define F2(x, y, z)       F1(z, x, y)
#define F3(x, y, z)       (x ^ y ^ z)
#define F4(x, y, z)       (y ^ (x | ~z))

// This is the central step in the MD5 algorithm.
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )
typedef struct MD5Context {
        uint32_t buf[4];
        uint32_t bits[2];
        unsigned char in[64];
} MD5Context;

typedef union {               // overlay two bytes on a 16-bit word
    uint16_t int_16;
    struct {
        uint8_t byte_1;
        uint8_t byte_2;
    } b;
} union_16;

typedef union {               // overlay two bytes on a 16-bit word
    uint32_t int_32;
    struct {
        uint8_t byte_1;
        uint8_t byte_2;
        uint8_t byte_3;
        uint8_t byte_4;
    } b;
} union_32;

/*------------------------------------------------------------------------------
 Section: Globals
 ------------------------------------------------------------------------------*/

#if !IUP_IS(NO_IUP)
struct partition_entry *part = NULL;
mdev_t                 *device = NULL;
#endif

IzotByte  iupImageValidated;
IzotByte  iupCommitTimerStarted;
LonTimer  iupInitFirmwareTimer;
LonTimer  iupValidateFirmwareTimer;
LonTimer  iupMd5EventTimer;
LonTimer  iupSwitchOverTimer;
LonTimer  iupCommitFirmwareTimer;

/*------------------------------------------------------------------------------
 Section: Static
 ------------------------------------------------------------------------------*/
#if !IUP_IS(NO_IUP)
static IupPersistent  iupPersistData;
static uint32_t       iupPersistDataLen = sizeof(iupPersistData);
static IzotUbits16    iupRcvdPckCount;
static IzotByte       validationOnceStarted;
static IzotByte       digestBytes[MD5_DIGEST_LENGTH];
#endif

static IzotByte       saltBytes[SALT_LENGTH];
static IzotByte       digestmatch;
static MD5Context     md5c;
static uint32_t       fileSizeTemp;

uint16_t swapword(uint16_t int_16) {             // swap bytes in 16-bit num
    union_16 u1, u2;
    u1.int_16 = int_16;
    u2.b.byte_2 = u1.b.byte_1;
    u2.b.byte_1 = u1.b.byte_2;
    return u2.int_16;
}

uint32_t swaplong(uint32_t int_32) {             // swap bytes in 32-bit num
    union_32 u1, u2;
    u1.int_32 = int_32;
    u2.b.byte_4 = u1.b.byte_1;
    u2.b.byte_3 = u1.b.byte_2;
    u2.b.byte_2 = u1.b.byte_3;
    u2.b.byte_1 = u1.b.byte_4;
    return u2.int_32;
}

/*******************************************************************************
Function:  isPacketMissed
Purpose:   Return TRUE if given packet is missed
*******************************************************************************/
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
static IzotByte isPacketMissed(IzotUbits16 packetNumber) 
{
    IzotByte isPktWritten = 0;
    
    // Read only one byte for particular packet number at respective location
    iflash_drv_read(NULL, &isPktWritten, 1, 
    IUP_FLASH_OFFSET + iupPersistDataLen + packetNumber - 1);
    
    if (isPktWritten == EEPROM_NOT_WRITTEN) {// packet is not received before
       return TRUE;
    } else {
        return FALSE;
    }
}
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)

/*******************************************************************************
Function:  writeIupPersistData
Purpose:   Write the IUP data into EEPROM
*******************************************************************************/
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
static void writeIupPersistData(IzotByte *data, uint32_t len, uint32_t addr)
{
    iflash_drv_write(NULL, data, len, addr);
}
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)

/*******************************************************************************
Function:  EraseIupPersistData
Purpose:   Erase the IUP data from EEPROM and take the appropriate actio on that
*******************************************************************************/
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
void EraseIupPersistData(void) 
{
    iflash_drv_erase(NULL, IUP_FLASH_OFFSET, EEPROM_BLOCK_SIZE);
}
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)

/*******************************************************************************
Function:  readIupPersistData
Purpose:   Read the IUP data from EEPROM and take the appropriate action on that
*******************************************************************************/
void readIupPersistData(void) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    IzotUbits16 pktNumber;
    IzotByte    isPktWritten = 0;
    IzotByte data[iupPersistDataLen];
    
    part = rfget_get_passive_firmware();

    iflash_drv_read(NULL, data, 1, IUP_FLASH_OFFSET);
    
    if (data[0] == IUP_PERSIST_DATA_VALID) {
        OsalPrintTrace(LonStatusNoError, "readIupPersistData: Found IUP persistent data");
        
        iflash_drv_init();
        device = flash_drv_open(part->device);
        
        iflash_drv_read(NULL, data, iupPersistDataLen, IUP_FLASH_OFFSET);
        memcpy(&iupPersistData, (IupPersistent *)&data, iupPersistDataLen);
        
        for (pktNumber = 0; pktNumber < iupPersistData.initData.IupPacketCount; pktNumber++) {
            iflash_drv_read(NULL, &isPktWritten, 1, 
            IUP_FLASH_OFFSET + iupPersistDataLen + pktNumber);
            if (isPktWritten != EEPROM_NOT_WRITTEN) {
                iupRcvdPckCount++;
            }
        }
        OsalPrintDebug(LonStatusNoError, "readIupPersistData: Received %d packets before power failure", 
                iupRcvdPckCount);
    } else {
        OsalPrintDebug(LonStatusNoError, "readIupPersistData: No IUP persistent data found");
    }
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
Function:  InitUpdateProcess
Purpose:   Initialize the firmware update process. Get the passive firmware
           Partition.
*******************************************************************************/
int InitUpdateProcess(void) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    // Check whether transfer in process. 
    // If yes then stop the previous transfer in process
    iupPersistData.iupMode = 1;
    iupRcvdPckCount = 0;    // Set the rcvd packet count to zero
    iupPersistData.iupConfirmResultSucceed = FALSE;
    iupCommitTimerStarted = FALSE;
    iupImageValidated = FALSE;
    validationOnceStarted = FALSE;
    iupPersistData.iupCommitDone = FALSE;
    iupPersistData.SecondaryFlag = FALSE;
    
    OsalPrintDebug(LonStatusNoError, "InitUpdateProcess: Erasing IUP persistent data in Init stage");
    EraseIupPersistData();
    
    iflash_drv_init();
    device = flash_drv_open(part->device);
    if (device == NULL) {
        OsalPrintError(LonStatusInvalidOperation, "InitUpdateProcess: Flash driver initialization is required before open");
        return IUP_ERROR;
    }

    if (iflash_drv_erase(device, part->start, part->size) < 0) {
        OsalPrintError(LonStatusInitializationFailed, "InitUpdateProcess: Failed to erase partition");
        return IUP_ERROR;
    }
    
    writeIupPersistData((IzotByte *)&iupPersistData.iupMode, sizeof(iupPersistData.iupMode) + 
    sizeof(iupPersistData.initData), IUP_FLASH_OFFSET);
    
    OsalPrintDebug(LonStatusNoError, "InitUpdateProcess: Image Update Process (IUP) initialization completed");
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    return IUP_ERROR_NONE;
}

/*******************************************************************************
Function:  VerifyImage
Purpose:   verify the image data stored into passive firmware
*******************************************************************************/
int VerifyImage(void)
{
    int error = IUP_ERROR_NONE;

 #if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)   
    OsalPrintDebug(LonStatusNoError, "VerifyImage: Validating firmware start from %X... IupImageLen = %d",
            part->start, iupPersistData.initData.IupImageLen);
    
    // Then validate firmware data in flash
    error = verify_load_firmware(part->start, iupPersistData.initData.IupImageLen);

    if (error) {
        OsalPrintError(LonStatusInvalidFirmwareImage, "VerifyImage: Validation failed with error %d", error);
    } else {
        OsalPrintDebug(LonStatusNoError, "VerifyImage: Validation done successfully");
    }
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    return error;
}

/*******************************************************************************
Function:  ByteReverse
Purpose:   this code is harmless on little-endian machines.
*******************************************************************************/
void ByteReverse(IzotByte *buf, IzotUbits16 longs) 
{
    uint32_t t;
    do {
	t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 | ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32_t *) buf = t;
	buf += 4;
    } while (--longs);
}

/*******************************************************************************
Function:  MD5Transform
Purpose:   The core of the MD5 algorithm, this alters an existing MD5 hash to
           reflect the addition of 16 longwords of new data.  MD5Update blocks
           the data and converts bytes into longwords for this routine.
*******************************************************************************/
void MD5Transform(uint32_t *buf, uint32_t *in) 
{
    register uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/*******************************************************************************
Function:  MD5Init
Purpose:   Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
           initialization constants.
*******************************************************************************/
void MD5Init(MD5Context *ctx) 
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}

/*******************************************************************************
Function:  MD5Update
Purpose:   Update context to reflect the concatenation of another buffer full
           of bytes.
*******************************************************************************/
void MD5Update(MD5Context *ctx, IzotByte *buf, IzotByte len) 
{
    uint32_t t;

    // Update bitcount
    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t) {
        ctx->bits[1]++; 	    // Carry from low to high
    }
    ctx->bits[1] += len >> 29;

    t = (t >> 3) & 0x3f;	// Bytes already in shsInfo->data

    // Handle any leading odd-sized chunks
    if (t) {
        unsigned char *p = (unsigned char *) ctx->in + t;
        t = 64 - t;
        if (len < t) {
            memcpy(p, buf, len);
            return;
        }
        memcpy(p, buf, t);
        ByteReverse(ctx->in, 16);
        MD5Transform(ctx->buf, (uint32_t *) ctx->in);
        buf += t;
        len -= t;
    }
    
    // Process data in 64-byte chunks
    while (len >= 64) {
        memcpy(ctx->in, buf, 64);
        ByteReverse(ctx->in, 16);
        MD5Transform(ctx->buf, (uint32_t *) ctx->in);
        buf += 64;
        len -= 64;
    }

    // Handle any remaining bytes of data.
    memcpy(ctx->in, buf, len);
}

/*******************************************************************************
Function:  MD5Final
Purpose:   Final wrapup - pad to 64-byte boundary with the bit pattern
           1 0* (64-bit count of bits processed, MSB-first)
*******************************************************************************/
void MD5Final(uint8_t *digest, MD5Context *ctx) 
{
    unsigned count;
    unsigned char *p;

    // Compute number of bytes mod 64
    count = (ctx->bits[0] >> 3) & 0x3F;

    // Set the first char of padding to 0x80.  This is safe since there is
    // always at least one byte free
    p = ctx->in + count;
    *p++ = 0x80;

    // Bytes of padding needed to make 64 bytes
    count = 64 - 1 - count;

    // Pad out to 56 mod 64
    if (count < 8) {
        // Two lots of padding:  Pad the first block to 64 bytes
        memset(p, 0, count);
        ByteReverse(ctx->in, 16);
        MD5Transform(ctx->buf, (uint32_t *) ctx->in);
    
        // Now fill the next block with 56 bytes
        memset(ctx->in, 0, 56);
    } else {
        // Pad block to 56 bytes
        memset(p, 0, count - 8);
    }
    ByteReverse(ctx->in, 14);

    // Append length in bits and transform
    ((uint32_t *) ctx->in)[14] = ctx->bits[0];
    ((uint32_t *) ctx->in)[15] = ctx->bits[1];

    MD5Transform(ctx->buf, (uint32_t *) ctx->in);
    ByteReverse((unsigned char *) ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);
    memset(ctx, 0, sizeof(MD5Context));        // In case it's sensitive
}

/*******************************************************************************
Function:  ComputeMD5Digest
Purpose:   Start MD5 Event Timer and consider Salt requested.
*******************************************************************************/
void ComputeMD5Digest(void) 
{
    int salt_index = 0;
    MD5Init(&md5c);

    if(VerifyImage() != 0) {
        iupImageValidated = TRUE;
        digestmatch = FALSE;
    }
    for(salt_index = 0; salt_index < 16; ++salt_index) {
        MD5Update(&md5c, &saltBytes[salt_index], 1);
    }
        
    fileSizeTemp = 0;
    SetLonTimer(&iupMd5EventTimer, 2);
}

/*******************************************************************************
Function:  CalculateMD5
Purpose:   Calulate MD5 for every 128 bytes from image and
           Start MD5 Event Timer again
*******************************************************************************/
void CalculateMD5(void) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    IzotByte digestResp[MD5_DIGEST_LENGTH];
    IzotByte md5buffer[128];
    IzotByte surPlus = iupPersistData.initData.IupImageLen % sizeof(md5buffer);
    int i;
    
    if(fileSizeTemp < iupPersistData.initData.IupImageLen) {
        if ((iupPersistData.initData.IupImageLen - fileSizeTemp) == surPlus) {
            iflash_drv_read(NULL, &md5buffer[0], surPlus, 
            fileSizeTemp + part->start);
            for (i = 0; i < surPlus; i++) {
                MD5Update(&md5c, &md5buffer[i], 1);
            }
            fileSizeTemp += surPlus;
        } else {
            iflash_drv_read(NULL, &md5buffer[0], sizeof(md5buffer), 
            fileSizeTemp + part->start);
            for (i = 0; i < sizeof(md5buffer); i++) {
                MD5Update(&md5c, &md5buffer[i], 1);
            }
            fileSizeTemp += sizeof(md5buffer);
        }
        SetLonTimer(&iupMd5EventTimer, 2);
    } else {
        MD5Final(digestResp, &md5c);
        OsalPrintDebug(LonStatusNoError, 
            "CalculateMD5: MD5 requested for digest %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X",
            digestBytes[0], digestBytes[1], digestBytes[2], digestBytes[3],
            digestBytes[4], digestBytes[5], digestBytes[6], digestBytes[7],
            digestBytes[8], digestBytes[9], digestBytes[10], digestBytes[11],
            digestBytes[12], digestBytes[13], digestBytes[14],  digestBytes[15]);
        OsalPrintDebug(LonStatusNoError, 
            "CalculateMD5: MD5 computed with digest %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X : %02X %02X",
            digestResp[0], digestResp[1], digestResp[2], digestResp[3],
            digestResp[4], digestResp[5], digestResp[6], digestResp[7],
            digestResp[8], digestResp[9], digestResp[10], digestResp[11],
            digestResp[12], digestResp[13], digestResp[14],  digestResp[15]);
        if (memcmp(digestResp, digestBytes, MD5_DIGEST_LENGTH)) {
			digestmatch = FALSE;	// not match
            OsalPrintError(LonStatusInvalidFirmwareImage, "CalculateMD5: MD5 digest not matched");
        } else {
			digestmatch = TRUE;	    // match
        }
        iupImageValidated = TRUE;
        fileSizeTemp = 0;
    }
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
Function:  SwitchOverImage
Purpose:   Set the passive partition as a active parttition after
           successfully verification of a firmware image.
*******************************************************************************/
void SwitchOverImage(void) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    int error;
    
    iflash_drv_close(device);
	
	if (!iupPersistData.SecondaryFlag){
		error = part_set_active_partition(part);
	}
    if (!iupPersistData.SecondaryFlag){
		arch_reboot();
	}
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}
 
void CommitImage(void) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
	if (!iupPersistData.iupCommitDone) {
		part_set_active_partition(part);
		iflash_drv_close(device);
		iupPersistData.iupCommitDone = TRUE;
		writeIupPersistData(&iupPersistData.iupCommitDone, 1, IUP_FLASH_OFFSET + sizeof(iupPersistData.iupMode) +
		sizeof(iupPersistData.initData) + sizeof(iupPersistData.iupConfirmResultSucceed));
		arch_reboot();
	}
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

IzotByte isEmpty(IzotByte *in, IzotUbits16 len) 
{
    int index = 0;
    for (index = 0; index < len; index++) {
        if (in[index] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

/*******************************************************************************
Function:  HandleNmeIupInit
Purpose:   Handle incoming NME Image Init request
*******************************************************************************/
void HandleNmeIupInit(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    // Get the available partition size
    uint32_t sizeAvailable = part->size;
    struct __attribute__ ((packed))
    {
        IzotByte subCode;        // IUP_Init : 0x1C
        IzotByte resultCode;     // 8-bit result code
        IzotUbits16 actionTime;     // time required to finish the action(in seconds)
        IzotUbits16 pcktSize;    // 16-bit packet size [used if resultCode == 0x02]
        IzotUbits16 pcktSpacing;
        IzotByte md5:1;
        IzotByte sha:1;
        IzotByte unused:6;
        IzotUbits16 packetCount;
    } init_response;

    // Fail if the Init Request does not have correct size
    if (appReceiveParamPtr->pduSize != 1 + sizeof(IUP_InitRequest)) {
        NMNDRespond(NM_MESSAGE, LonStatusInvalidParameter, appReceiveParamPtr, apduPtr);
        return;
    }
    
    // Get the request data
    IUP_InitRequest *init_request = (IUP_InitRequest *)(&apduPtr->data[0]);
    
    init_response.subCode = init_request->subCode;
    init_response.actionTime = 0;
    init_response.pcktSize = 0;
    init_response.pcktSpacing = swapword(IUP_PACKET_SPACING);
    init_response.md5 = 1;
    init_response.sha = 0;
    init_response.packetCount = 0;
    
    // Check for the correct image type and subtype 
    // and space available to store new image
    if (init_request->imgIdent.imgType != HOST_PROCESSOR_COMBINED_IMAGE) {
        init_response.resultCode = IUP_INIT_RESULT_INVALID_IMAGE_TYPE;
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(init_response), 
        (IzotByte*)&init_response);
        return;
    } else if (init_request->imgIdent.imgSubType != HOST_PROCESSOR_COMBINED_IMAGE) {
        init_response.resultCode = IUP_INIT_RESULT_INVALID_IMAGE_SUBTYPE;
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(init_response), 
        (IzotByte*)&init_response);
        return;
    } else if (swaplong(init_request->imageLen) > sizeAvailable) {
        init_response.resultCode = IUP_INIT_RESULT_IMAGE_TOO_LARGE;   
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(init_response), 
        (IzotByte*)&init_response);
        return;
    }
    
    iupPersistData.initData.IupImageLen = swaplong(init_request->imageLen);
    // Check for packet size that will be used by initiator
    
    if (swapword(init_request->pcktSize) > IUP_PACKET_SIZE_SUPPORTED) {
        init_response.resultCode = IUP_INIT_RESULT_LARGE_PACKET_SIZE;
        iupPersistData.initData.IupPacketCount = iupPersistData.initData.IupImageLen / IUP_PACKET_SIZE_SUPPORTED;
        if (iupPersistData.initData.IupImageLen % IUP_PACKET_SIZE_SUPPORTED) {
			iupPersistData.initData.IupPacketCount++;
        }
        init_response.actionTime = swapword(IUP_INIT_IMAGE_UPDATE_INIT_TIMER);
        init_response.pcktSize = swapword(IUP_PACKET_SIZE_SUPPORTED);
        iupPersistData.initData.IupPacketLen = IUP_PACKET_SIZE_SUPPORTED;
    } else {
        init_response.resultCode = IUP_INIT_RESULT_SUCCESS;
		init_response.actionTime = swapword(IUP_INIT_IMAGE_UPDATE_INIT_TIMER);
		init_response.pcktSize = 0;
		iupPersistData.initData.IupPacketLen = swapword(init_request->pcktSize);
		iupPersistData.initData.IupPacketCount = swapword(init_request->pcktCount);
    }

    iupPersistData.initData.IupSessionNumber = swaplong(init_request->sessionNumber);
    memcpy(&iupPersistData.initData.IupImageIdentifier, &init_request->imgIdent, 
    sizeof(iupPersistData.initData.IupImageIdentifier));
    
    if (iupPersistData.initData.IupPacketCount > (4096 - iupPersistDataLen)) {
        init_response.resultCode = IUP_INIT_RESULT_PACKET_COUNT_TOO_HIGH;
        init_response.packetCount = swapword(1);
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(init_response),
        (IzotByte *)&init_response);
        return;
    }
    
    if (!isEmpty(init_request->imageHeader, sizeof(init_request->imageHeader))) {
        IzotByte tagId = 0xC5;
        IzotByte mfgId[4] = {0x00, 0x00, 0x00, 0x01};
        IzotByte hwId[2] = {0x00, 0x02};
        IzotByte hwVer = 0x00;
        
        if (init_request->imageHeader[0] != tagId || memcmp(&init_request->imageHeader[2], mfgId, 4) ||
        memcmp(&init_request->imageHeader[6], hwId, 2)) {
            OsalPrintError(LonStatusIupInvalidImage, "HandleNmeIupInit: Firmware image model incompatible %02X %02X %02X %02X %02X %02X %02X", init_request->imageHeader[0], 
            init_request->imageHeader[2], init_request->imageHeader[3], init_request->imageHeader[4], 
            init_request->imageHeader[5], init_request->imageHeader[6], init_request->imageHeader[7]);
            init_response.resultCode = IUP_INIT_RESULT_MODEL_INCOMPATIBLE;
        }
        if (init_request->imageHeader[1] != 0 || init_request->imageHeader[10] != hwVer) {
            OsalPrintError(LonStatusIupInvalidImage, "HandleNmeIupInit: Firmware image version incompatible %02X %02X", 
            init_request->imageHeader[1], init_request->imageHeader[10]);
            init_response.resultCode = IUP_INIT_RESULT_VERSION_INCOMPATIBLE;
        }
    }
    
    OsalPrintDebug(LonStatusNoError, "HandleNmeIupInit: IupPacketLen: %d", iupPersistData.initData.IupPacketLen);
    OsalPrintDebug(LonStatusNoError, "HandleNmeIupInit: IupPacketCount: %d", iupPersistData.initData.IupPacketCount);
    OsalPrintDebug(LonStatusNoError, "HandleNmeIupInit: IupSessionNumber: %X", iupPersistData.initData.IupSessionNumber);
    OsalPrintDebug(LonStatusNoError, "HandleNmeIupInit: IupImageLen: %d", iupPersistData.initData.IupImageLen);
    OsalPrintDebug(LonStatusNoError, "HandleNmeIupInit: Initializing image update process");
    
    SetLonTimer(&iupInitFirmwareTimer, IUP_INIT_FIRMWARE_TIMER_VALUE);
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(init_response), 
    (IzotByte *)&init_response);
    return;
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
 Function:  HandleNmeIupTransfer
 Purpose:   Handle incoming NME Image Transfer message
 *******************************************************************************/
void HandleNmeIupTransfer(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    IzotUbits16 newPckNum = 0;

    // Drop the packet with no data
    if (appReceiveParamPtr->pduSize <= 2) {
        OsalPrintError(LonStatusInvalidParameter, "HandleNmeIupTransfer: IUP transfer packet length too small");
        return;
    }

    // Get the request data
    IUP_TransferRequest *transfer_request = (IUP_TransferRequest *)(&apduPtr->data[0]);
                                    
    if (appReceiveParamPtr->pduSize > 8) {
        // Check the session number 
        if (swaplong(transfer_request->sessionNumber) != iupPersistData.initData.IupSessionNumber) {
            OsalPrintError(LonStatusIupInvalidParameter,"HandleNmeIupTransfer: IUP transfer session number does not match");
            return;
        }

        // If all packet are received then drop new incoming broadcast packets
        if (iupPersistData.iupConfirmResultSucceed == TRUE) {
            OsalPrintDebug(LonStatusNoError, "HandleNmeIupTransfer: Confirm result successfully sent; all packets rreceived; dropping this packet");
            return;
        }

        newPckNum = swapword(transfer_request->packetNumber);

        // Drop already received packet
        if (!isPacketMissed(newPckNum)) {
            OsalPrintDebug(LonStatusNoError, "HandleNmeIupTransfer: Packet Number %d already received; dropping it", newPckNum);
            return;
        }
        
        if (iflash_drv_write(device, &transfer_request->data[0], iupPersistData.initData.IupPacketLen, part->start + 
        ((newPckNum - 1) * iupPersistData.initData.IupPacketLen)) == 0) {
            // Increment the received packet count
            iupRcvdPckCount++;
            // Store in flash that this packet number is received
            IzotByte received = EEPROM_WRITTEN;
            writeIupPersistData(&received, 1, IUP_FLASH_OFFSET + iupPersistDataLen + newPckNum - 1);

            OsalPrintDebug(LonStatusNoError, "HandleNmeIupTransfer: Packet number %d at address %X", newPckNum, part->start + 
                    ((newPckNum - 1) * iupPersistData.initData.IupPacketLen));
        }
    }
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
 Function:  HandleNmeIupConfirm
 Purpose:   Handle incoming NME Image  Confirm request
 *******************************************************************************/
void HandleNmeIupConfirm(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    IzotUbits16 pktNumber;
    IzotByte    isPktWritten;
    struct __attribute__ ((packed))
    {
        IzotByte        subCode;     // IUP_Confirm : 0x1E
        IzotByte        resultCode;  // 8-bit result code
        IzotByte        packetCount; // Number of packets to be resent
        IzotUbits16     pcktNumberColl[MAX_PACKET_COUNT_IN_CONFIRM_RESPONSE];
    } confirm_response;
    
    // Fail if the request does not have correct size
    if (appReceiveParamPtr->pduSize != 1 + sizeof(IUP_ConfirmRequest)) {
        NMNDRespond(NM_MESSAGE, LonStatusInvalidParameter, appReceiveParamPtr, apduPtr);
        return;
    }
    
    IUP_ConfirmRequest *confirm_request = (IUP_ConfirmRequest *)(&apduPtr->data[0]);
    confirm_response.subCode = confirm_request->subCode;
    confirm_response.packetCount = 0;
    memset(confirm_response.pcktNumberColl, 0, sizeof(confirm_response.pcktNumberColl));
            
    if (swaplong(confirm_request->sessionNumber) != iupPersistData.initData.IupSessionNumber) {
        return;
    }
    
    if (iupRcvdPckCount < EIGHTY_PERCENT(iupPersistData.initData.IupPacketCount)) {
        OsalPrintError(LonStatusIupTransferFailure,"HandleNmeIupConfirm: 20 percent or more packets are lost; ignoring this update");
        confirm_response.resultCode = IUP_CONFIRM_RESULT_IMAGE_NOT_VIABLE;
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(confirm_response) - 
        sizeof(confirm_response.pcktNumberColl) + (confirm_response.packetCount * 2), (IzotByte *)&confirm_response);
        return;
    }
    
    if (iupPersistData.initData.IupPacketCount == iupRcvdPckCount) {
        OsalPrintDebug(LonStatusNoError, "HandleNmeIupConfirm: No packet error detected");
        
        confirm_response.resultCode = IUP_CONFIRM_RESULT_SUCESS;
        confirm_response.packetCount = 0;
        iupPersistData.iupConfirmResultSucceed = TRUE;
        writeIupPersistData(&iupPersistData.iupConfirmResultSucceed, 1, IUP_FLASH_OFFSET + 
        sizeof(iupPersistData.iupMode) + sizeof(iupPersistData.initData));
        
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(confirm_response) - 
        sizeof(confirm_response.pcktNumberColl) + (confirm_response.packetCount * 2), (IzotByte *)&confirm_response);
        return ;
    }
    
    // Packet error detected
    confirm_response.resultCode = IUP_CONFIRM_RESULT_PACKET_MISSED;
    confirm_response.packetCount = 0;
    for (pktNumber = 0; pktNumber < iupPersistData.initData.IupPacketCount; pktNumber++) {
        iflash_drv_read(NULL, &isPktWritten, 1, IUP_FLASH_OFFSET + iupPersistDataLen + pktNumber);
        
        if (isPktWritten == EEPROM_NOT_WRITTEN) {
            OsalPrintError(LonStatusIupImageWriteFailure,"HandleNmeIupConfirm: Packet number %d write failed", pktNumber + 1);
            confirm_response.pcktNumberColl[confirm_response.packetCount] = swapword(pktNumber + 1);
            confirm_response.packetCount++;
        }

        if (confirm_response.packetCount == MAX_PACKET_COUNT_IN_CONFIRM_RESPONSE) {
            break;
        }
    }
    OsalPrintError(LonStatusIupImageWriteFailure, "HandleNmeIupConfirm: Total %d packet(s) missed", confirm_response.packetCount);
    
    if (!confirm_response.packetCount) {
        confirm_response.resultCode = IUP_CONFIRM_RESULT_SUCESS;
    }

    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(confirm_response) - 
    sizeof(confirm_response.pcktNumberColl) + (confirm_response.packetCount * 2), (IzotByte *)&confirm_response);
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
Function:  HandleNmeIupValidate
Purpose:   Handle incoming NME Image Validate request
*******************************************************************************/
void HandleNmeIupValidate(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    struct __attribute__ ((packed))
    {
        IzotByte subCode;        // IUP_Validate : 0x1F
        IzotByte resultCode;     // 8-bit result code
        IzotUbits16 actionTime;     // time required to finish the action(in seconds)
    } validate_response;
    
    IUP_ValidateRequest *validate_request = (IUP_ValidateRequest *)(&apduPtr->data[0]);
        
    // Compare the session number
    if (swaplong(validate_request->sessionNumber) != iupPersistData.initData.IupSessionNumber) {
        OsalPrintError(LonStatusIupTransferFailure, "HandleNmeIupValidate: Session number does not match");
        return;
    }
    
    validate_response.subCode = validate_request->subCode;
    validate_response.actionTime = 0;
    
    // Go with Success if Image is not going to be validate with digest
    if (validate_request->digestType == DIGEST_TYPE_NONE) {
		if (appReceiveParamPtr->pduSize != 7) {
			NMNDRespond(NM_MESSAGE, LonStatusInvalidParameter, appReceiveParamPtr, apduPtr);
			return;
		}
		validate_response.resultCode = IUP_VALIDATE_RESULT_SUCCESS;
		SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(validate_response), 
        (IzotByte *)&validate_response);
        return;
	} 
    
    // Check whether digest type is supported or not
    if (validate_request->digestType != DIGEST_TYPE_SUPPORTED) {
        validate_response.resultCode = IUP_VALIDATE_RESULT_INVALID_DIGEST;
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(validate_response), 
        (IzotByte *)&validate_response);
        return;
    }
    
    if (iupImageValidated) {
        if (!digestmatch) {
            validate_response.resultCode = IUP_VALIDATE_RESULT_INVALID_DIGEST;
        } else {
            validate_response.resultCode = IUP_VALIDATE_RESULT_SUCCESS;
        }
    } else {
        validate_response.resultCode = IUP_VALIDATE_RESULT_STILL_PENDING;
        memcpy(saltBytes, validate_request->saltBytes, sizeof(saltBytes));
        memcpy(digestBytes, validate_request->digestBytes, sizeof(digestBytes));
        SetLonTimer(&iupValidateFirmwareTimer, 
        IUP_VALIDATE_FIRMWARE_TIMER_VALUE);
        validate_response.actionTime = swapword(60);
    }
    
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(validate_response), 
    (IzotByte *)&validate_response);
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
 Function:  HandleNmeIupSwitchOver
 Purpose:   Handle incoming NME Image  switch over request
 *******************************************************************************/
void HandleNmeIupSwitchOver(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    uint32_t countDownTimer;
    struct __attribute__ ((packed))
    {
        IzotByte            subCode;    // IUP_SwitchOver : 0x20
        IzotByte            resultCode;    
        IzotUbits16         actionTime; // 16-bits time required to finish
        IUP_ImageIdentifier imgIdent;   // image identifier
        IUP_rejectionInfo   rejectInfo; //reason to reject the request
    } switchover_response;
    
    // Fail if the request does not have correct size
    if (appReceiveParamPtr->pduSize != 1 + sizeof(IUP_SwitchOverRequest)) {
        NMNDRespond(NM_MESSAGE, LonStatusInvalidParameter, appReceiveParamPtr, apduPtr);
        return;
    }
    
    IUP_SwitchOverRequest *switchover_request = (IUP_SwitchOverRequest *)(&apduPtr->data[0]);
    
    switchover_response.subCode = switchover_request->subCode;
    switchover_response.actionTime = 0;
    switchover_response.rejectInfo.dataLen = 0;
    switchover_response.rejectInfo.type = 0; 
    memset(&switchover_response.rejectInfo.data[0], 0, switchover_response.rejectInfo.dataLen);
    memset(&switchover_response.imgIdent, 0, sizeof(switchover_response.imgIdent));
    
    //Image is not validated
    if (!iupImageValidated) {
        OsalPrintError(LonStatusIupInvalidImage, "HandleNmeIupSwitchOver: Image not validated");
        switchover_response.resultCode = IUP_SWITCHOVER_RESULT_IMAGE_REJECTED;
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(switchover_response), 
        (IzotByte *)&switchover_response);
        return;
    }
    
    if (IUP_isSecondaryFlag(switchover_request->switchOverflags)) {
        iupPersistData.SecondaryFlag = TRUE;
        writeIupPersistData(&iupPersistData.SecondaryFlag, 1, IUP_FLASH_OFFSET + sizeof(iupPersistData.iupMode) +
        sizeof(iupPersistData.initData) + sizeof(iupPersistData.iupConfirmResultSucceed) + 
        sizeof(iupPersistData.iupCommitDone));
    } 

    countDownTimer = swaplong(switchover_request->switchOverTime);
    if(!countDownTimer) {
        countDownTimer = 1;
    } else {
        switchover_response.resultCode = IUP_SWITCHOVER_DELAY_NOT_SUPPORTED;
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(switchover_response), 
        (IzotByte *)&switchover_response);
        return;
    }
    OsalPrintDebug(LonStatusNoError, "HandleNmeIupSwitchOver countdown timer: %d seconds", countDownTimer);
        
    switchover_response.actionTime = swapword(IZOT_RESET_TIME_AFTER_SWITCHOVER);
    if (!IUP_isPreseveConfig(switchover_request->switchOverflags)) {
        OsalPrintDebug(LonStatusNoError, "HandleNmeIupSwitchOver: Erasing configuration data");
        ErasePersistenceConfig();
    } else if (!IUP_isPersistence(switchover_request->switchOverflags)) {
        OsalPrintDebug(LonStatusNoError, "HandleNmeIupSwitchOver: Erasing persistent data");
        ErasePersistenceData();
    } else {
        OsalPrintDebug(LonStatusNoError, "HandleNmeIupSwitchOver: Config and persistent data needs to be preserved");
    }
        
    // Start the switchover timer
    SetLonTimer(&iupSwitchOverTimer, countDownTimer * IUP_SWITCHOVER_TIMER_VALUE);
    
    switchover_response.resultCode = IUP_SWITCHOVER_RESULT_SUCCESS;
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(switchover_response), 
    (IzotByte *)&switchover_response);
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
 Function:  HandleNmeIupStatus
 Purpose:   Handle incoming NME Image  Status request
 *******************************************************************************/
void HandleNmeIupStatus(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    struct __attribute__ ((packed))
    {
        IzotByte subCode;        // IUP_Status : 0x21
        IzotByte statusFlag;     // status flag : see above
        IzotByte rejectionCode;
        uint32_t actionTime;
        IUP_rejectionInfo rejectInfo; // reason to reject the request
    } status_response;
    
    // Fail if the request does not have correct size
    if (appReceiveParamPtr->pduSize != 1 + sizeof(IUP_StatusRequest)) {
        OsalPrintError(LonStatusIupTransferFailure, "HandleNmeIupStatus: invalid packet size");
        NMNDRespond(NM_MESSAGE, LonStatusInvalidParameter, appReceiveParamPtr, apduPtr);
        return;
     }
    
    IUP_StatusRequest *status_request = (IUP_StatusRequest *)(&apduPtr->data[0]);
    status_response.subCode = status_request->subCode;
    status_response.rejectInfo.dataLen = 0;
    status_response.rejectInfo.type = 0; 
    memset(&status_response.rejectInfo.data[0], 0, status_response.rejectInfo.dataLen);
    status_response.actionTime = 0;
    if (memcmp(&status_request->imgIdent, &iupPersistData.initData.IupImageIdentifier, sizeof(status_request->imgIdent))) {
        status_response.rejectionCode = IUP_STATUS_REJECTION_VERSION_INCOMPATIBLE;
        OsalPrintError(LonStatusIupInvalidImage, "HandleNmeIupStatus: Image identifier does not match in status request; erasing IUP data");
        EraseIupPersistData();
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(status_response), 
        (IzotByte *)&status_response);
        return;
    }
    
    if (iupPersistData.iupCommitDone) {
        status_response.statusFlag = 0x05;
        status_response.rejectionCode = IUP_STATUS_REJECTION_NONE;
        OsalPrintDebug(LonStatusNoError, "HandleNmeIupStatus: Erasing IUP persistent data in status request because there will be no commit request now");
        EraseIupPersistData();
    } else if (!iupPersistData.SecondaryFlag) {
        status_response.statusFlag = 0x00;
        status_response.rejectionCode = IUP_STATUS_REJECTION_IMAGE_REJECTED;
        OsalPrintError(LonStatusIupInvalidImage, "HandleNmeIupStatus: Image rejected, erasing IUP data");
        EraseIupPersistData();    
    } else {
        status_response.statusFlag = 0x07;
        status_response.rejectionCode = IUP_STATUS_REJECTION_NONE;     
    }
    
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(status_response), 
    (IzotByte *)&status_response);
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
 Function:  HandleNmeIupCommit
 Purpose:   Handle incoming NME Image  commit request
 *******************************************************************************/
void HandleNmeIupCommit(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
    struct __attribute__ ((packed))
    {
        IzotByte subCode;        // IUP_Commit : 0x22
        IzotByte resultCode;
        IzotUbits16 actionTime;  // 16-bits time required to finish the action(in seconds
        IUP_rejectionInfo rejectInfo; // reason to reject the request (see above)
    } commit_response;
    
    // Fail if the request does not have correct size
    if (appReceiveParamPtr->pduSize != 1 + sizeof(IUP_CommitRequest)) {
        OsalPrintError(LonStatusIupTransferFailure, "HandleNmeIupCommit: Invalid packet size, erasing IUP data");
        EraseIupPersistData();
        NMNDRespond(NM_MESSAGE, LonStatusIupTransferFailure, appReceiveParamPtr, apduPtr);
        return;
    }
    
    IUP_CommitRequest *commit_request = (IUP_CommitRequest *)(&apduPtr->data[0]);
    
    commit_response.subCode = commit_request->subCode;
    commit_response.actionTime = 0;
    commit_response.rejectInfo.dataLen = 0;
    commit_response.rejectInfo.type = 0; 
    memset(&commit_response.rejectInfo.data[0], 0, commit_response.rejectInfo.dataLen);
    
    if (memcmp(&commit_request->imgIdent, &iupPersistData.initData.IupImageIdentifier, sizeof(commit_request->imgIdent))) {
        OsalPrintError(LonStatusIupTransferFailure, "HandleNmeIupCommit: Image identifier does not match, erasing IUP data");
        commit_response.resultCode = IUP_COMMIT_RESULT_FAILED;
        EraseIupPersistData();
        SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(commit_response), 
        (IzotByte *)&commit_response);    
        return;
    }

	if (iupPersistData.SecondaryFlag) {
		if (!iupPersistData.iupCommitDone) {
			DBG_vPrintf("Commiting Image...\r\n");
			SetLonTimer(&iupCommitFirmwareTimer, IUP_COMMIT_FIRMWARE_TIMER_VALUE);
			commit_response.resultCode = IUP_COMMIT_RESULT_STILL_PENDING;
			commit_response.actionTime = swapword(IUP_COMMIT_RESPONSE_ACTION_TIME);
		} else {
			DBG_vPrintf("Image Commitment done...\r\n");
			EraseIupPersistData();
            commit_response.resultCode = IUP_COMMIT_RESULT_SUCCESS;
		}
    } else {
        OsalPrintError(LonStatusIupTransferFailure, "HandleNmeIupCommit: Image is already primary, erasing IUP data");
        commit_response.resultCode = IUP_COMMIT_RESULT_IMAGE_ALREADY_PRIMARY;
        EraseIupPersistData();
    }
    
    SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(commit_response), 
    (IzotByte *)&commit_response);
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}

/*******************************************************************************
 Function:  HandleNmeIupTransferAck
 Purpose:   Respond To Acknowledgment from SS at regular Interval.
 *******************************************************************************/
void HandleNmeIupTransferAck(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr) 
{
#if !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
	IupTransferAckResponse TransferAck_response;
	TransferAck_response.subCode = apduPtr->data[0];
	TransferAck_response.resultCode = TRANSFER_CONTINUE;
	TransferAck_response.actionTime = 0;
	SendResponse(appReceiveParamPtr->reqId, NM_resp_success | NM_EXPANDED, sizeof(TransferAck_response), 
    (IzotByte *)&TransferAck_response);
#endif  // !IUP_IS(NO_IUP) && PLATFORM_IS(FRTOS_ARM_EABI)
}
