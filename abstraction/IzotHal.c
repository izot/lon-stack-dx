//
// IzotHal.c
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
 * Title: Hardware Abstraction Layer
 *
 * Abstract:
 * Hardware-dependent APIs. This file includes optional interfaces
 * for the 88MC200 GPIO and external flash.
 */

#include "IzotHal.h"

#if PROCESSOR_IS(MC200)
#include <mdev.h>
#include <mc200_gpio.h>
#include <mc200_pinmux.h>
#endif


/*------------------------------------------------------------------------------
Section: Macro
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
Section: Global
------------------------------------------------------------------------------*/

#if PROCESSOR_IS(MC200)
mdev_t *dev = NULL;
#endif

/*
 * *****************************************************************************
 * SECTION: FUNCTIONS
 * *****************************************************************************
 *
 */

/*
 * Function:   HalGetFlashInfo
 * Use this function get the inforamation of your flash region wich defines 
 * the area  for the Izot non volatile data
 *
 */
int HalGetFlashInfo(void *fd, unsigned long *offset, 
unsigned long *region_size, int *number_of_blocks, 
unsigned long *block_size, int *number_of_regions)
{
#if PROCESSOR_IS(MC200)
    *offset             = FLASH_OFFSET;
    *region_size        = FLASH_REGION_SIZE;
    *number_of_blocks   = NUM_OF_BLOCKS;
    *block_size         = BLOCK_SIZE;
    *number_of_regions  = NO_OF_REGION;
#else
    *offset             = 0;
    *region_size        = 0;
    *number_of_blocks   = 0;
    *block_size         = 0;
    *number_of_regions  = 0;
#endif

    return 0;
}

/*
 * Function:   HalFlashDrvOpen
 * Use this API to open the driver for the flash for your hardware.
 *
 */
void *HalFlashDrvOpen(uint32_t flags)
{
#if PROCESSOR_IS(MC200)
    dev = (mdev_t *)iflash_drv_open("iflash", 0);
    return dev;
#else
    return NULL;
#endif
}

/*
 * Function:   HalFlashDrvClose
 * Use this API to close the driver for the flash for your hardware.
 *
 */
void HalFlashDrvClose(void *fd)
{
#if PROCESSOR_IS(MC200)
    iflash_drv_close(dev);
#endif
}

/*
 * Function:   HalFlashDrvInit
 * Use this API to initialize the driver for the flash for your hardware.
 *
 */
int HalFlashDrvInit(void)
{
#if PROCESSOR_IS(MC200)
    return iflash_drv_init();
#else
    return 0;
#endif
}

/*
 * Function:   HalFlashDrvErase
 * Use this API to erase the data from the given offset by number of bytes 
 * provided the parameter for your hardware.
 *
 */
int HalFlashDrvErase(void *fd, unsigned long start, unsigned long size)
{
#if PROCESSOR_IS(MC200)
    return iflash_drv_erase(dev, start, size);
#else
    return 0;
#endif
}

/*
 * Function:   HalFlashDrvWrite
 * Use this API to write the data from the given offset by number of bytes
 * provided the parameter for your hardware.
 *
 */
int HalFlashDrvWrite(void *fd, IzotByte *buf, uint32_t len, uint32_t addr)
{
#if PROCESSOR_IS(MC200)
    return iflash_drv_write(dev, buf, len, addr);
#else
    return 0;
#endif
}

/*
 * Function:   HalFlashDrvRead
 * Use this API to read the data from the given offset by number of bytes
 * provided the parameter for your hardware.
 *
 */
int HalFlashDrvRead(void *fd, IzotByte *buf, uint32_t len, uint32_t addr)
{
#if PROCESSOR_IS(MC200)
    return iflash_drv_read(dev, buf, len, addr);
#else
    return 0;
#endif
}

/*
 * Function:   HalGetMacAddress
 * Use this API to get the MAC address of your hardware.
 *
 */ 
int HalGetMacAddress(unsigned char *mac)
{
#if PROCESSOR_IS(MC200)
    return wlan_get_mac_address(mac);
#else
    return 0;
#endif
}

/*
 * Function:   HalReboot
 * Use this API to do an actual reset of the device
 *
 */ 
void HalReboot(void)
{
#if PROCESSOR_IS(MC200)
    arch_reboot();
#endif
}
