//
// IzotHal.h
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

/*
 * Title: Hardware Abstaction Layer header file
 *
 * Abstract:
 * This file contains the hardware dependent APIs. This file contains
 * APIs for the 88MC200 for the hardware GPIO and external flash.
 */ 

#include "IzotPlatform.h"

#if !defined(DEFINED_IZOTHAL_H)
#define DEFINED_IZOTHAL_H


/*------------------------------------------------------------------------------
Section: Macro
------------------------------------------------------------------------------*/
#define IUP_FLASH_OFFSET    0x6000
#define FLASH_OFFSET        0x6000
#define FLASH_REGION_SIZE   0x6000
#define NUM_OF_BLOCKS       6
#define BLOCK_SIZE          0x1000
#define NO_OF_REGION        1

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/

/*
 * Function:   HalGetMacAddress
 * Use this API to get the MAC address of your hardware.
 *
 */ 
extern int HalGetMacAddress(unsigned char *mac);

/*
 * Function:   HalGetFlashInfo
 * Use this function get the inforamation of your flash region wich defines 
 * the area  for the Izot non volatile data
 *
 */
extern int HalGetFlashInfo(void *fd, unsigned long *offset, 
unsigned long *region_size, int *number_of_blocks, 
unsigned long *block_size, int *number_of_regions
);

/*
 * Function:   HalFlashDrvOpen
 * Use this API to open the driver for the flash for your hardware.
 *
 */
extern void *HalFlashDrvOpen(uint32_t flags);

/*
 * Function:   HalFlashDrvClose
 * Use this API to close the driver for the flash for your hardware.
 *
 */
extern void HalFlashDrvClose(void *fd);

/*
 * Function:   HalFlashDrvInit
 * Use this API to initialize the driver for the flash for your hardware.
 *
 */
extern int HalFlashDrvInit(void);

/*
 * Function:   HalFlashDrvErase
 * Use this API to erase the data from the given offset by number of bytes 
 * provided the parameter for your hardware.
 *
 */
extern int HalFlashDrvErase(
void *fd, unsigned long start, unsigned long size
);

/*
 * Function:   HalFlashDrvWrite
 * Use this API to write the data from the given offset by number of bytes
 * provided the parameter for your hardware.
 *
 */
extern int HalFlashDrvWrite(
void *fd, IzotByte *buf, uint32_t len, uint32_t addr
);

/*
 * Function:   HalFlashDrvRead
 * Use this API to read the data from the given offset by number of bytes
 * provided the parameter for your hardware.
 *
 */
extern int HalFlashDrvRead(
void *fd, IzotByte *buf, uint32_t len, uint32_t addr
);

#endif  /* defined(DEFINED_IZOTHAL_H) */
/* end of file. */
