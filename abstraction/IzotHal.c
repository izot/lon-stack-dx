/*
 * IzotHal.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Hardware Abstraction Layer
 * Purpose: Hardware dependent APIs.
 * Notes:   This file includes optional interfaces for the 88MC200
 *          GPIO and external flash.
 */

#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "abstraction/IzotConfig.h" // Project-specific configuration
 
#if OS_IS(LINUX)
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#endif // OS_IS(LINUX)

#include "abstraction/IzotHal.h"

#if PROCESSOR_IS(MC200)
#include <wm_os.h>
#include <mdev.h>
#include <mc200_gpio.h>
#include <mc200_pinmux.h>
#endif // PROCESSOR_IS(MC200)

/*****************************************************************
 * Section: Globals
 *****************************************************************/

#if OS_IS(LINUX)
int flashFd = -1; // File descriptor for the flash file

// LON application configuration file path
const char *configFilePath = "/var/lib/lon-device-stack/lon-app-config";
#endif // OS_IS(LINUX)

#if PROCESSOR_IS(MC200)
mdev_t *flashFd = NULL; // File descriptor for the flash device
mdev_t *dev = NULL;
#endif // PROCESSOR_IS(MC200)

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/

/* Create the LON Stack configuration directory if it does not exist.
 * Parameters:
 *   path: the directory path to create
 *   mode: permissions to use for any directories created
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code
 *   on failure.
 */
IzotApiError HalCreateConfigDirectory(const char *path, mode_t mode) {
#if OS_IS(LINUX)
    char tmp[512];
    struct stat st;
    size_t len;
    char *p;

    if (!path || !*path)
        return IzotApiPersistentDirError;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    // Remove trailing slash (but preserve root "/")
    if (len > 1 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    // Iterate through the path and identify directories to create
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (stat(tmp, &st) != 0) {
                if (errno == ENOENT) {
                    if (mkdir(tmp, mode) != 0) {
                        // mkdir failed
                        return IzotApiPersistentDirError;
                    }
                } else {
                    // stat failed
                    return IzotApiPersistentDirError;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                // Path element exists but is not a directory
                return IzotApiPersistentDirError;
            }
            *p = '/';
        }
    }

    // Final directory
    if (stat(tmp, &st) != 0) {
        if (errno == ENOENT) {
            if (mkdir(tmp, mode) != 0) {
                // mkdir failed
                return IzotApiPersistentDirError;
            }
        } else {
            // stat failed
            return IzotApiPersistentDirError;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        // Path element exists but is not a directory
        return IzotApiPersistentDirError;
    }

    return IzotApiNoError;
#else
    // Not implemented for this platform
    return IzotApiPersistentDirError;
#endif // OS_IS(LINUX)
}

/*
 * Returns information about the flash region used for persistent data.
 * Parameters:
 *   offset: pointer to offset of region in memory
 *   region_size: pointer to size of region in bytes
 *   number_of_blocks: pointer to number of blocks in region
 *   block_size: pointer to size of each block in bytes
 *   number_of_regions: pointer to number of regions
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code
 *   on failure.
 * Notes:
 *   The flash region may be a directly mapped flash memory region,
 *   or it may be a file on a file system.  An offset to the flash
 *   memory region is returned.  The offset is zero for a file on
 *   a file system, but is typically non-zero for a directly mapped
 *   memory flash memory region.  The flash region is used
 *   for persistent data storage.
 */
IzotApiError HalGetFlashInfo(unsigned long *offset, 
        unsigned long *region_size, int *number_of_blocks, 
        unsigned long *block_size, int *number_of_regions)
{
    IzotApiError result = IzotApiNoError;

#if OS_IS(LINUX)
    *offset             = LINUX_FLASH_OFFSET;
#elseif PROCESSOR_IS(MC200)
    *offset             = FREERTOS_FLASH_OFFSET;
#endif 

#if OS_IS(LINUX) || PROCESSOR_IS(MC200)
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
    result              = IzotApiPersistentFailure
#endif

    return result;
}

/*
 * Function:   HalFlashDrvOpen
 * Use this API to open the driver for the flash for your hardware.
 *
 */
IzotApiError HalFlashDrvOpen(void)
{
    IzotApiError result = IzotApiNoError

#if OS_IS(LINUX)
    // Open file (read/write, create if missing, no truncation)
    flashFd = open(configFilePath, O_RDWR | O_CREAT, 0644);
    if (flashFd == -1) {
        // Configuration file open error
        return IzotApiPersistentFileError;
    }
#elif PROCESSOR_IS(MC200)
    result = ((flashFd = (mdev_t *)iflash_drv_open("iflash", 0)) != NULL) ? IzotApiNoError : IzotApiPersistentFileError);
#else
    result = IzotApiPersistentFileError;
#endif // OS_IS(FREERTOS)

    return result;
}

/*
 * Function:   HalFlashDrvClose
 * Use this API to close the driver for the flash for your hardware.
 *
 */
void HalFlashDrvClose(void)
{
// TBD: Linux close:  fclose(fd);
#if PROCESSOR_IS(MC200)
    iflash_drv_close(dev);
#endif
}

/*
 * Function:   HalFlashDrvInit
 * Use this API to initialize the driver for the flash for your hardware.
 *
 */
IzotApiError HalFlashDrvInit(void)
{
#if OS_IS(LINUX)
    // Get directory portion of configuration file path
    char filedir[512];
    snprintf(filedir, sizeof(filedir), "%s", configFilePath);
    dirname(filedir); // modifies in place

    // Ensure configuration file path exists
    return HalCreateConfigDirectory(filedir, 0755);
#elif PROCESSOR_IS(MC200)
    return (iflash_drv_init() ? IzotApiNoError : IzotApiPersistentFailure);
#else
    return IzotApiPersistentFailure; // No flash driver available
#endif
}

/*
 * Function:   HalFlashDrvErase
 * Use this API to erase the data from the given offset by number of bytes 
 * provided the parameter for your hardware.
 *
 */
int HalFlashDrvErase(unsigned long start, unsigned long size)
{
#if PROCESSOR_IS(MC200)
    return iflash_drv_erase(flashFd, start, size);
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
int HalFlashDrvWrite(IzotByte *buf, uint32_t len, uint32_t addr)
{
// TBD: Linux write:  write(fd, "test", 4);
#if PROCESSOR_IS(MC200)
    return iflash_drv_write(flashFd, buf, len, addr);
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
int HalFlashDrvRead(IzotByte *buf, uint32_t len, uint32_t addr)
{
#if PROCESSOR_IS(MC200)
    return iflash_drv_read(flashFd, buf, len, addr);
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

#ifdef  __cplusplus
}
#endif

