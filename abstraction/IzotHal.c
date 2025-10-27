/*
 * IzotHal.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Hardware Abstraction Layer
 * Purpose: Hardware dependent APIs.
 * Notes:   This file includes APIs for persistent memory and MAC
 *          address access, and reboot support.  This
 *          implementatipon includes support for POSIX-compliant
 *          Linux hosts as well as the Marvell 88MC200.
 */

#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "izot/IzotPlatform.h" // Project-specific configuration

#if OS_IS(LINUX)
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#ifndef SIOCGIFHWADDR
#define SIOCGIFHWADDR 0x8927
#endif
#endif // OS_IS(LINUX)

#if OS_IS(LINUX) || OS_IS(FREERTOS)
#include <sys/types.h>
#endif // OS_IS(LINUX) || OS_IS(FREERTOS)

#if PROCESSOR_IS(MC200)
#include <wm_os.h>
#include <mdev.h>
#include <mc200_gpio.h>
#include <mc200_pinmux.h>
#endif // PROCESSOR_IS(MC200)

#include "abstraction/IzotHal.h"

/*****************************************************************
 * Section: Globals
 *****************************************************************/

LonStatusCode persistentMemError = LonStatusNoError; // Last persistent memory error
IzotBool persistentMemInitialized = FALSE; // Flag to indicate if flash is initialized

#if OS_IS(LINUX)
const char *configFilePath = "/var/lib/lon-device-stack/lon-app-config";
                            // LON application configuration file path
int flashFd = -1;           // File descriptor for the flash file
const char *iface = "eth0"; // Hardware dependent IP interface name
#endif // OS_IS(LINUX)

#if PROCESSOR_IS(MC200)
mdev_t *flashFd = NULL; // File descriptor for the flash device
#endif // PROCESSOR_IS(MC200)

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/

/* 
 * Creates the LON Stack configuration directory if it does not exist.
 * Parameters:
 *   path: the directory path to create
 *   mode: permissions to use for any directories created
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode HalCreateConfigDirectory(const char *path, mode_t mode) {
#if OS_IS(LINUX)
    char tmp[512];
    struct stat st;
    size_t len;
    char *p;

    if (!IZOT_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
    if (!path || !*path || strlen(path) >= sizeof(tmp)) {
        // Invalid path
        return persistentMemError = LonStatusPersistentDataDirError;
    }
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
                        return persistentMemError = LonStatusPersistentDataDirError;
                    }
                } else {
                    // stat failed
                    return persistentMemError = LonStatusPersistentDataDirError;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                // Path element exists but is not a directory
                return persistentMemError = LonStatusPersistentDataDirError;
            }
            *p = '/';
        }
    }
    // Final directory
    if (stat(tmp, &st) != 0) {
        if (errno == ENOENT) {
            if (mkdir(tmp, mode) != 0) {
                // mkdir failed
                return persistentMemError = LonStatusPersistentDataDirError;
            }
        } else {
            // stat failed
            return persistentMemError = LonStatusPersistentDataDirError;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        // Path element exists but is not a directory
        return persistentMemError = LonStatusPersistentDataDirError;
    }
    return persistentMemError;
#else
    // Not implemented for this platform
    return persistentMemError = LonStatusPersistentDataDirError;
#endif // OS_IS(LINUX)
}

/*
 * Initializes the hardware-specific driver for interfacing with
 * persistent memory.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode HalFlashDrvInit(void)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }
    if (persistentMemInitialized) {
        return LonStatusNoError;
    }
    persistentMemInitialized = TRUE;
#if OS_IS(LINUX)
    // Get directory portion of configuration file path
    char filedir[512];
    snprintf(filedir, sizeof(filedir), "%s", configFilePath);
    dirname(filedir); // modifies in place

    // Ensure configuration file path exists
    return persistentMemError = HalCreateConfigDirectory(filedir, 0755);
#elif PROCESSOR_IS(MC200)
    return persistentMemError = (iflash_drv_init() 
            ? LonStatusNoError : LonStatusPersistentDataFailure);
#else
    return persistentMemError = LonStatusPersistentDataFailure; // No flash driver available
#endif
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
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 * Notes:
 *   The flash region may be a directly mapped flash memory region,
 *   or it may be a file on a file system.  An offset to the flash
 *   memory region is returned.  The offset is zero for a file on
 *   a file system, but is typically non-zero for a directly mapped
 *   memory flash memory region.  The flash region is used
 *   for persistent data storage.
 */
LonStatusCode HalGetFlashInfo(size_t *offset, size_t *region_size,
        int *number_of_blocks, size_t *block_size, int *number_of_regions)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }

#if OS_IS(LINUX)
    *offset             = LINUX_FLASH_OFFSET;
#elif PROCESSOR_IS(MC200)
    *offset             = FREERTOS_FLASH_OFFSET;
#endif 

#if OS_IS(LINUX) || PROCESSOR_IS(MC200)
    *region_size        = FLASH_REGION_SIZE;
    *number_of_blocks   = NUM_OF_BLOCKS;
    *block_size         = BLOCK_SIZE;
    *number_of_regions  = NO_OF_REGIONS;
#else
    *offset             = 0;
    *region_size        = 0;
    *number_of_blocks   = 0;
    *block_size         = 0;
    *number_of_regions  = 0;
    persistentMemError  = LonStatusPersistentDataFailure
#endif

    return persistentMemError;
}

/*
 * Opens the hardware-specific driver for interfacing with 
 * persistent memory.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode HalFlashDrvOpen(void)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }
#if OS_IS(LINUX)
    // Open file (read/write, create if missing, no truncation)
    if (flashFd != -1) {
        // Already open
        return persistentMemError = LonStatusNoError;
    }
    flashFd = open(configFilePath, O_RDWR | O_CREAT, 0644);
    if (flashFd == -1) {
        // Configuration file open error
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
#elif PROCESSOR_IS(MC200)
    // Open the flash device
    if (flashFd != NULL) {
        // Already open
        return persistentMemError = LonStatusNoError;
    }
    persistentMemError = ((flashFd = (mdev_t *)iflash_drv_open("iflash", 0)) != NULL) 
            ? LonStatusNoError : LonStatusPersistentDataAccessError);
#else
    persistentMemError = LonStatusPersistentDataAccessError;
#endif // OS_IS(FREERTOS)
    return persistentMemError;
}

/*
 * Closes the hardware-specific driver for interfacing with
 * persistent memory.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
LonStatusCode HalFlashDrvClose(void)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }
#if OS_IS(LINUX)
    if (flashFd != -1) {
        close(flashFd);
        flashFd = -1;
    }
#elif PROCESSOR_IS(MC200)
    if (flashFd != NULL) {
        iflash_drv_close(flashFd);
        flashFd = NULL;
    }
#endif
    return persistentMemError = LonStatusNoError;
}

/*
 * Erases the persistent data from the specified starting offset
 * by the specified size in bytes.
 * Parameters:
 *   start: offset in bytes from the start of the flash region
 *   size: number of bytes to erase
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode HalFlashDrvErase(size_t start, size_t size)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }
#if OS_IS(LINUX)
    struct stat st;
    if ((flashFd == -1) || (fstat(flashFd, &st) != 0)) {
        // Persistent file not open or stat failed
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    off_t file_size = st.st_size;
    if (file_size < start) {
        // Seek to (start-1) and write a single 0x00 to extend the file
        if (lseek(flashFd, start - 1, SEEK_SET) == (off_t)-1) {
            // Extend seek failed
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        unsigned char zero = 0;
        if (write(flashFd, &zero, 1) != 1) {
            // Extend write failed
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
    }

    // Seek to offset
    if (lseek(flashFd, start, SEEK_SET) == -1) {
        // Seek to start failed
        return persistentMemError = LonStatusPersistentDataAccessError;
    }

    // Write 0xFF bytes
    unsigned char buf[256];
    memset(buf, 0xFF, sizeof(buf));
    size_t left = size;
    while (left > 0) {
        size_t chunk = left > sizeof(buf) ? sizeof(buf) : left;
        ssize_t w = write(flashFd, buf, chunk);
        if (w < 0) {
            // Write to region failed
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        left -= w;
    }
    return persistentMemError = LonStatusNoError;
#elif PROCESSOR_IS(MC200)
    if (flashFd == NULL) {
        // Flash driver not initialized
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Erase the flash region by filling the specified area with 0xFF
    return persistentMemError = (iflash_drv_erase(flashFd, start, size) 
            ? LonStatusNoError : LonStatusPersistentDataAccessError);
#else
    return persistentMemError = LonStatusPersistentDataAccessError;
#endif
}

/*
 * Writes the contents of buffer `buf` to an open file descriptor
 * `flashFd`, starting at offset `start` for `size` bytes.
 * Parameters:
 *   start: offset in bytes from the start of the flash region
 *   size: number of bytes to write
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 * Notes:
 *   The file is extended if the file size is less than the starting
 *   offset.
 */
LonStatusCode HalFlashDrvWrite(IzotByte *buf, size_t start, size_t size)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }
#if OS_IS(LINUX)
    struct stat st;
    if ((flashFd == -1) || (fstat(flashFd, &st) != 0)) {
        // Persistent file not open or stat failed
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    if (st.st_size < start) {
        // Extend file to the desired offset
        if (lseek(flashFd, start - 1, SEEK_SET) == (off_t)-1) {
            // Extend seek failed
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        unsigned char zero = 0;
        if (write(flashFd, &zero, 1) != 1) {
            // Extend write failed
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
    }
    // Seek to the start offset
    if (lseek(flashFd, start, SEEK_SET) == (off_t)-1) {
        // Seek to start failed
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Write the data
    size_t written = 0;
    while (written < size) {
        ssize_t w = write(flashFd, (const char*)buf + written, size - written);
        if (w < 0) {
            // Persistent data write failure
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        written += w;
    }
    return persistentMemError = LonStatusNoError;
#elif PROCESSOR_IS(MC200)
    return persistentMemError = (iflash_drv_write(flashFd, buf, len, addr) 
            ? LonStatusNoError : LonStatusPersistentDataAccessError);
#else
    return persistentMemError = LonStatusPersistentDataAccessError;
#endif
}

/*
 * Reads `size` bytes from the file descriptor `flashFd` into buffer
 * `buf`, starting at offset `start`.
 * Parameters:
 *   start: offset in bytes from the start of the flash region
 *   size: number of bytes to read
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 * Notes:
 *    An error is returned if the file size is less than `start + size` bytes.
 */
LonStatusCode HalFlashDrvRead(IzotByte *buf, size_t start, size_t size)
{
    if (!IZOT_SUCCESS(persistentMemError)) {   
        return persistentMemError;
    }
#if OS_IS(LINUX)
    struct stat st;
    if ((flashFd == -1) || (fstat(flashFd, &st) != 0)) {
        // Persistent file not open or stat failed
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Check that the file is large enough
    if (st.st_size < (off_t)(start + size)) {
        // Attempt to read beyond end of file
        errno = EINVAL;
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Seek to the start offset
    if (lseek(flashFd, start, SEEK_SET) == (off_t)-1) {
        // Seek to start failed
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Read the data
    size_t read_bytes = 0;
    while (read_bytes < size) {
        ssize_t r = read(flashFd, (char*)buf + read_bytes, size - read_bytes);
        if (r < 0) {
            // Persistent data read failure
            return persistentMemError = LonStatusPersistentDataAccessError;
        } else if (r == 0) {
            // End of file reached before reading enough bytes
            if (read_bytes < size) {
                errno = EINVAL;
                return persistentMemError = LonStatusPersistentDataAccessError;
            }
            break; // Successfully read all requested bytes
        }
        read_bytes += r;
    }
    return persistentMemError = LonStatusNoError;
#elif PROCESSOR_IS(MC200)
    return persistentMemError = (iflash_drv_read(flashFd, buf, size, start) 
            ? LonStatusNoError : LonStatusPersistentDataAccessError);
#else
    return persistentMemError = LonStatusPersistentDataAccessError;
#endif
}

/*
 * Gets the MAC address of the host IP interface.
 * Parameters:
 *   mac: pointer to 6 byte array for the MAC ID
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 * Notes:
 *   For a Linux host, the IP interface name is defined in the 'iface'
 *   global.  The name is host-dependent and must match the name for
 *   the host.
 */ 
LonStatusCode HalGetMacAddress(unsigned char *mac)
{
#if OS_IS(LINUX)
    const char *iface = "eth0";
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        // Socket error
        return LonStatusDeviceUniqeIdNotAvailable;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
        // ioctl error
        close(fd);
        return LonStatusDeviceUniqeIdNotAvailable;
    }

    return LonStatusNoError; // Success
#elif PROCESSOR_IS(MC200)
    return (wlan_get_mac_address(mac) ? LonStatusDeviceUniqeIdNotAvailable : LonStatusNoError);
#else
    return LonStatusDeviceUniqeIdNotAvailable;
#endif
}

/*
 * Reboots the host device.
 * Parameters:
 *   None
 * Returns:
 *   If successful, this function does not return.
 *   If not successful, <LonStatusCode> LonStatusHostRebootFailure
 *   is returned.
 */ 
LonStatusCode HalReboot(void)
{
    LonStatusCode ret = LonStatusHostRebootFailure;

#if OS_IS(LINUX)
    // Sync filesystems before rebooting
    sync();

    // Attempt to reboot the system (Requires root privileges)
    if (reboot(RB_AUTOBOOT) != 0) {
        // Reboot failure
        return ret;
    }
#elif PROCESSOR_IS(MC200)
    arch_reboot();
#endif
    // Should not reach here
    return ret;
}

#ifdef  __cplusplus
}
#endif

