/*
 * IzotHal.c
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 *
 * Title:   Hardware Abstraction Layer
 * Purpose: Hardware dependent APIs.
 * Notes:   This file includes APIs for persistent memory and MAC address
 *          access, and reboot support.  This implementation includes support
 *          for POSIX-compliant Linux platforms, STM32 with FreeRTOS platforms,
 *          and Marvell 88MC200 platforms.
 */

// Define feature test macros before any system headers
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#include <sys/types.h>

#include "abstraction/IzotHal.h"
#include "izot/IzotPlatform.h"  // Project-specific configuration

// Forward declare sync() for platforms that need it
#if defined(__APPLE__) || defined(__unix__) || defined(__linux__)
extern void sync(void);
#endif

// Include networking headers for all Unix-like systems
// MUST come after sys/types.h and in this specific order for macOS
#if OS_IS(LINUX) || defined(__unix__) || defined(__APPLE__) || defined(_POSIX_VERSION)
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <sys/sockio.h>
#endif  // __APPLE__

#ifdef __linux__
#include <linux/if_packet.h>
#endif  // __linux__
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#ifndef SIOCGIFHWADDR
#define SIOCGIFHWADDR 0x8927
#endif  // SIOCGIFHWADDR
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif  // IFNAMSIZ
#endif  // OS_IS(LINUX) || defined(__unix__) || defined(__APPLE__) || defined(_POSIX_VERSION)

// Additional Linux-specific headers (not macOS)
#if OS_IS(LINUX) && defined(__linux__)
#include <sys/reboot.h>
#endif  // OS_IS(LINUX) && defined(__linux__)

// Linux kernel headers
#if OS_IS(LINUX_KERNEL)
#include <linux/if.h>
#include <linux/netdevice.h>
#endif  // OS_IS(LINUX_KERNEL)

#if OS_IS(LINUX) || OS_IS(FREERTOS)
#include <sys/types.h>
#endif  // OS_IS(LINUX) || OS_IS(FREERTOS)

#if PROCESSOR_IS(STM32)
#include "stm32XXXX_hal.h"  // Replace XXXX with your specific MCU family (e.g., stm32f4xx_hal.h)
#include "usbh_cdc.h"
#include "usbh_core.h"
#endif  // PROCESSOR_IS(STM32)

#if PROCESSOR_IS(MC200)
#include <mc200_gpio.h>
#include <mc200_pinmux.h>
#include <mdev.h>
#include <wm_os.h>
#endif  // PROCESSOR_IS(MC200)

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************
 * Section: Globals
 *****************************************************************/

LonStatusCode persistentMemError = LonStatusNoError;  // Last persistent memory error
IzotBool persistentMemInitialized = FALSE;  // Flag to indicate if flash is initialized

#if OS_IS(LINUX)
static const char *configDirectoryDefault = "/var/lib/lon-device-stack";
// LON application configuration file path
// Overridable via LON_STACK_DX_CONFIG_FILE environment variable
static char configDirectory[512] = "";
// LON Stack configuration directory
static int storageFd[IzotPersistentSegNumSegmentTypes] = {-1};
// File descriptors for segment data storage devices
static const char *iface = "eth0";  // Hardware dependent IP interface name
#elif PROCESSOR_IS(STM32)
USBH_HandleTypeDef hUsbHostFS;
// USB host handle used by HalWriteUsb() and USB host library callbacks
static volatile bool is_usb_cdc_ready = false;
// Flag to track when the U60/U10 device is fully enumerated
CDC_LineCodingTypeDef cdc_line_coding;
// Structure to hold the current CDC line coding settings; used to track when
// the device is ready for LON communication
static uint8_t cdc_init_state = 0;
// State variable used to track progress through necessary CDC control transfers
static uint8_t raw_usb_rx_buffer[64];
// Buffer used for receiving data from the USB host
// library; must be at least as large as the largest
// expected USB packet (e.g., 64 bytes for full-speed USB)
#endif  // OS_IS(LINUX) || OS_IS(FREERTOS)

#if PROCESSOR_IS(MC200)
static mdev_t *flashFd = NULL;  // File descriptor for the flash device
#endif                          // PROCESSOR_IS(MC200)

/*****************************************************************
 * Section: Storage Function Definitions
 *****************************************************************/

/*
 * Returns the default LON Stack configuration file path.
 * Parameters:
 *   None
 * Returns:
 *   Pointer to the configuration file path string.
 * Notes:
 *   The default path can be overridden by setting the
 *   LON_STACK_DX_CONFIG_FILE environment variable.
 */
#if OS_IS(LINUX)
static const char *HalGetConfigDirectory(void)
{
    const char *overridePath = getenv("LON_STACK_DX_CONFIG_FILE");
    if (overridePath && *overridePath) {
        return overridePath;
    }
    return configDirectoryDefault;
}
#endif  // OS_IS(LINUX)

/*
 * Creates the LON Stack configuration directory if it does not exist.
 * Parameters:
 *   path: the directory path to create
 *   mode: permissions to use for any directories created
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
static LonStatusCode HalCreateConfigDirectory(const char *path, mode_t mode)
{
#if OS_IS(LINUX)
    char workingPath[512];
    struct stat st;
    size_t len;
    char *p;

    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
    if (!path || !*path || strlen(path) >= sizeof(workingPath)) {
        // Invalid path
        return persistentMemError = LonStatusPersistentDataDirError;
    }
    snprintf(workingPath, sizeof(workingPath), "%s", path);
    len = strlen(workingPath);
    // Remove trailing slash (but preserve root "/")
    if (len > 1 && workingPath[len - 1] == '/') {
        workingPath[len - 1] = '\0';
    }
    // Iterate through the path and identify directories to create
    for (p = workingPath + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (stat(workingPath, &st) != 0) {
                if (errno == ENOENT) {
                    if (mkdir(workingPath, mode) != 0) {
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
    if (stat(workingPath, &st) != 0) {
        if (errno == ENOENT) {
            if (mkdir(workingPath, mode) != 0) {
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
#endif  // OS_IS(LINUX)
}

/*
 * Initializes the hardware-specific driver for interfacing with
 * persistent storage.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode HalInitStorage(void)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
    if (persistentMemInitialized) {
        return LonStatusNoError;
    }
    persistentMemInitialized = TRUE;
#if OS_IS(LINUX)
    // Get directory portion of configuration file path
    snprintf(configDirectory, sizeof(configDirectory), "%s", HalGetConfigDirectory());
    dirname(configDirectory);  // modifies in place
    // Ensure configuration file path exists
    return persistentMemError = HalCreateConfigDirectory(configDirectory, 0755);
#elif PROCESSOR_IS(MC200)
    return persistentMemError = (iflash_drv_init() ? LonStatusNoError
                                                   : LonStatusPersistentDataFailure);
#else
    return persistentMemError =
                   LonStatusPersistentDataFailure;  // No flash driver available
#endif
}

/*
 * Returns information about the storage used for persistent data.
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
 *   The storage region may be a directly mapped storage memory region,
 *   or it may be a file on a file system.  An offset to the storage
 *   memory region is returned.  The offset is zero for a file on
 *   a file system, but is typically non-zero for a directly mapped
 *   storage memory region.  The storage region is used
 *   for persistent data storage.
 */
LonStatusCode HalStorageInfo(size_t *offset, size_t *region_size, int *number_of_blocks,
        size_t *block_size, int *number_of_regions, bool *erase_required,
        uint8_t *erase_value)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }

#if OS_IS(LINUX)
    *offset = LINUX_FLASH_OFFSET;
    *erase_required = false;
    *erase_value = 0;
#elif PROCESSOR_IS(MC200)
    *offset = FREERTOS_FLASH_OFFSET;
    *erase_required = true;
    *erase_value = 0xFF;
#endif  // OS_IS(LINUX) || PROCESSOR_IS(STM32)

#if OS_IS(LINUX) || PROCESSOR_IS(MC200)
    *region_size = FLASH_REGION_SIZE;
    *number_of_blocks = NUM_OF_BLOCKS;
    *block_size = BLOCK_SIZE;
    *number_of_regions = NO_OF_REGIONS;
#else
    *offset = 0;
    *region_size = 0;
    *number_of_blocks = 0;
    *block_size = 0;
    *number_of_regions = 0;
    *erase_required = false;
    *erase_value = 0;
    persistentMemError = LonStatusPersistentDataFailure;
    OsalPrintLog(ERROR_LOG, persistentMemError,
            "HalStorageInfo: No persistent storage driver available");
#endif  // OS_IS(LINUX) || PROCESSOR_IS(MC200)

    return persistentMemError;
}

/*
 * Opens the hardware-specific driver for interfacing with
 * storage data segment persistent storage.
 * Parameters:
 *   persistent_seg_type: Persistent data storage segment to be opened
 *   persistent_seg_name: Name of the persistent data storage segment to be
 * opened max_data_size: Maximum size of the persistent data segment in bytes
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode HalOpenStorageSegment(const IzotPersistentSegType persistent_seg_type,
        char *persistent_seg_name, size_t max_data_size)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
#if OS_IS(LINUX)
    // Open file (read/write, create if missing, no truncation)
    if (storageFd[persistent_seg_type] != -1) {
        // Already open
        return persistentMemError = LonStatusNoError;
    }
    char config_file_path[512];
    snprintf(config_file_path, sizeof(config_file_path), "%s/%s", configDirectory,
            persistent_seg_name);
    storageFd[persistent_seg_type] = open(config_file_path, O_RDWR | O_CREAT, 0644);
    if (storageFd[persistent_seg_type] == -1) {
        // Configuration file open error
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalOpenStorageSegment: Cannot open or create %s, %s system "
                "error (errno %d)",
                config_file_path, strerror(errno), errno);
        return persistentMemError = LonStatusPersistentDataAccessError;
    }

    // Ensure the configuration file is the expected fixed size.
    // This allows random-access reads/writes up to the specified size.
    struct stat st;
    if (fstat(storageFd[persistent_seg_type], &st) != 0) {
        close(storageFd[persistent_seg_type]);
        storageFd[persistent_seg_type] = -1;
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalOpenStorageSegment: Cannot read attributes for %s, %s "
                "system error (errno %d)",
                config_file_path, strerror(errno), errno);
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    if ((size_t)st.st_size != max_data_size) {
        if (ftruncate(storageFd[persistent_seg_type], (off_t)max_data_size) != 0) {
            close(storageFd[persistent_seg_type]);
            storageFd[persistent_seg_type] = -1;
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalOpenStorageSegment: Cannot set size of %d for %s, %s "
                    "system error (errno %d)",
                    max_data_size, config_file_path, strerror(errno), errno);
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
    }
    OsalPrintLog(INFO_LOG, persistentMemError,
            "HalOpenStorageSegment: Opened %d byte storage segment %s", max_data_size,
            persistent_seg_name);
#elif PROCESSOR_IS(MC200)
    // Open the flash device
    if (flashFd != NULL) {
        // Already open
        return persistentMemError = LonStatusNoError;
    }
    persistentMemError = ((flashFd = (mdev_t *)iflash_drv_open("iflash", 0)) != NULL)
                                 ? LonStatusNoError
                                 : LonStatusPersistentDataAccessError;
#else
    persistentMemError = LonStatusPersistentDataAccessError;
    OsalPrintLog(ERROR_LOG, persistentMemError,
            "HalOpenStorageSegment: No persistent storage driver available");
#endif  // OS_IS(FREERTOS)
    return persistentMemError;
}

/*
 * Closes the hardware-specific driver for interfacing with persistent data
 * storage. 
 * Parameters: 
 *   persistent_seg_type: Persistent data storage segment to be closed
 * Returns: 
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.
 */
LonStatusCode HalCloseStorageSegment(const IzotPersistentSegType persistent_seg_type)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
#if OS_IS(LINUX)
    if (storageFd[persistent_seg_type] != -1) {
        close(storageFd[persistent_seg_type]);
        storageFd[persistent_seg_type] = -1;
    }
#elif PROCESSOR_IS(MC200)
    if (flashFd != NULL) {
        iflash_drv_close(flashFd);
        flashFd = NULL;
    }
#endif  // OS_IS(LINUX) || PROCESSOR_IS(MC200)
    return persistentMemError = LonStatusNoError;
}

/*
 * Prepare storage for writing persistent data from the specified starting
 * offset by the specified size in bytes.
 * Parameters:
 *   persistent_seg_type: Persistent data storage segment to be prepared
 *   seg_start: virtual offset in bytes of the segment from the start of storage
 * region start: virtual offset in bytes from the start of the storage region
 *   size: number of bytes to prepare
 *   erase_value: value to use for erasing
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.
 */
LonStatusCode HalPrepareStorageSegment(const IzotPersistentSegType persistent_seg_type,
        size_t seg_start, size_t start, size_t size, uint8_t erase_value)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
#if OS_IS(LINUX)
    struct stat st;
    if ((storageFd[persistent_seg_type] == -1) ||
            (fstat(storageFd[persistent_seg_type], &st) != 0)) {
        // Persistent file not open or stat failed
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalPrepareStorageSegment: Persistent file not open or stat failed");
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    size_t file_start = seg_start - start;
    off_t file_size = st.st_size;
    if (file_size < file_start) {
        // Seek to (start-1) and write a single 0x00 to extend the file
        if (lseek(storageFd[persistent_seg_type], file_start - 1, SEEK_SET) ==
                (off_t)-1) {
            // Extend seek failed
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalPrepareStorageSegment: Extend seek failed");
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        unsigned char zero = 0;
        if (write(storageFd[persistent_seg_type], &zero, 1) != 1) {
            // Extend write failed
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalPrepareStorageSegment: Extend write failed");
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
    }
    // Seek to offset
    if (lseek(storageFd[persistent_seg_type], file_start, SEEK_SET) == -1) {
        // Seek to start failed
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalPrepareStorageSegment: Seek to start failed");
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // If an erase is required, fill size bytes with erase_value, otherwise fill 2
    // bytes (the transaction record)
    unsigned char buf[256];
    memset(buf, erase_value, sizeof(buf));
    size_t left = size;
    while (left > 0) {
        size_t chunk = left > sizeof(buf) ? sizeof(buf) : left;
        ssize_t w = write(storageFd[persistent_seg_type], buf, chunk);
        if (w < 0) {
            // Write to region failed
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalPrepareStorageSegment: Write to region failed");
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        left -= w;
    }
    persistentMemError = LonStatusNoError;
    OsalPrintLog(INFO_LOG, persistentMemError,
            "HalPrepareStorageSegment: Prepared storage segment %d from "
            "offset %zu for %zu bytes",
            persistent_seg_type, seg_start - start, size);
    return persistentMemError;
#elif PROCESSOR_IS(MC200)
    if (flashFd == NULL) {
        // Flash driver not initialized
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Erase the storage region by filling the specified area with erase_value
    return persistentMemError = (iflash_drv_erase(flashFd, start, size)
                                         ? LonStatusNoError
                                         : LonStatusPersistentDataAccessError);
#else
    OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
            "HalPrepareStorageSegment: No persistent storage driver available");
    return persistentMemError = LonStatusPersistentDataAccessError;
#endif
}

/*
 * Writes a buffer to a persistent data storage segment.
 * Parameters:
 *   persistent_seg_type: Persistent data storage segment to write
 *   seg_start: virtual offset in bytes of the segment from the start of storage
 * region start: virtual offset in bytes from the start of the storage region
 *   size: number of bytes to write
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.
 * Notes:
 *   The file is extended if the file size is less than the starting
 *   offset.
 */
LonStatusCode HalWriteStorageSegment(const IzotPersistentSegType persistent_seg_type,
        IzotByte *buf, size_t seg_start, size_t start, size_t size)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
#if OS_IS(LINUX)
    struct stat st;
    if ((storageFd[persistent_seg_type] == -1) ||
            (fstat(storageFd[persistent_seg_type], &st) != 0)) {
        // Persistent file not open or stat failed
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalWriteStorageSegment: Persistent file not open or stat failed");
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    size_t file_start = start - seg_start;
    off_t file_size = st.st_size;
    if (file_size < file_start) {
        // Extend file to the desired offset
        if (lseek(storageFd[persistent_seg_type], file_start - 1, SEEK_SET) ==
                (off_t)-1) {
            // Extend seek failed
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalWriteStorageSegment: Extend seek failed");
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        unsigned char zero = 0;
        if (write(storageFd[persistent_seg_type], &zero, 1) != 1) {
            // Extend write failed
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalWriteStorageSegment: Extend write failed");
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
    }
    // Seek to the start offset
    if (lseek(storageFd[persistent_seg_type], file_start, SEEK_SET) == (off_t)-1) {
        // Seek to start failed
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalWriteStorageSegment: Seek to start failed");
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Write the data
    size_t written = 0;
    while (written < size) {
        ssize_t w = write(storageFd[persistent_seg_type], (const char *)buf + written,
                size - written);
        if (w < 0) {
            // Persistent data write failure
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalWriteStorageSegment: Persistent data write failure");
            return persistentMemError = LonStatusPersistentDataAccessError;
        }
        written += w;
    }
    OsalPrintLog(INFO_LOG, persistentMemError,
            "HalWriteStorageSegment: Wrote %zu bytes to storage segment %d "
            "at offset %zu",
            size, persistent_seg_type, start - seg_start);
    OsalPrintMessage(DETAIL_TRACE_LOG, "HalWriteStorageSegment: ", buf, size);
    return persistentMemError = LonStatusNoError;
#elif PROCESSOR_IS(MC200)
    return persistentMemError = (iflash_drv_write(flashFd, buf, len, addr)
                                         ? LonStatusNoError
                                         : LonStatusPersistentDataAccessError);
#else
    OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
            "HalWriteStorageSegment: No persistent storage driver available");
    return persistentMemError = LonStatusPersistentDataAccessError;
#endif
}

/*
 * Reads a buffer from a persistent data storage segment.
 * Parameters:
 *   persistent_seg_type: Persistent data storage segment to read
 *   seg_start: virtual offset in bytes of the segment from the start of storage
 * region start: virtual offset in bytes from the start of the storage region
 *   size: number of bytes to read
 * Returns:
 *   LonStatusNoError on success, or a LonStatusCode error code on failure.
 * Notes:
 *    An error is returned if the file size is less than `start + size` bytes.
 */
LonStatusCode HalReadStorageSegment(const IzotPersistentSegType persistent_seg_type,
        IzotByte *buf, size_t seg_start, size_t start, size_t size)
{
    if (!LON_SUCCESS(persistentMemError)) {
        return persistentMemError;
    }
#if OS_IS(LINUX)
    struct stat st;
    if ((storageFd[persistent_seg_type] == -1) ||
            (fstat(storageFd[persistent_seg_type], &st) != 0)) {
        // Persistent file not open or stat failed
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalReadStorageSegment: Persistent file not open or stat failed");
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Check that the file is large enough
    size_t file_start = start - seg_start;
    off_t file_size = st.st_size;
    if (file_size < (off_t)(file_start + size)) {
        // Attempt to read beyond end of file
        errno = EINVAL;
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalReadStorageSegment: Attempt to read beyond end of file "
                "for segment %d",
                persistent_seg_type);
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Seek to the start offset
    if (lseek(storageFd[persistent_seg_type], file_start, SEEK_SET) == (off_t)-1) {
        // Seek to start failed
        OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                "HalReadStorageSegment: Seek to %zu failed for segment %d", file_start,
                persistent_seg_type);
        return persistentMemError = LonStatusPersistentDataAccessError;
    }
    // Read the data
    size_t read_bytes = 0;
    while (read_bytes < size) {
        ssize_t r = read(storageFd[persistent_seg_type], (char *)buf + read_bytes,
                size - read_bytes);
        if (r < 0) {
            // Persistent data read failure
            OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                    "HalReadStorageSegment: Persistent data read failure for segment %d",
                    persistent_seg_type);
            return persistentMemError = LonStatusPersistentDataAccessError;
        } else if (r == 0) {
            // End of file reached before reading enough bytes
            if (read_bytes < size) {
                errno = EINVAL;
                OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
                        "HalReadStorageSegment: EOF before reading enough bytes "
                        "for segment %d",
                        persistent_seg_type);
                return persistentMemError = LonStatusPersistentDataAccessError;
            }
            break;  // Successfully read all requested bytes
        }
        read_bytes += r;
    }
    OsalPrintLog(INFO_LOG, persistentMemError,
            "HalReadStorageSegment: Read %zu bytes from storage segment %d "
            "at offset %zu",
            size, persistent_seg_type, start - seg_start);
    OsalPrintMessage(DETAIL_TRACE_LOG, "HalReadStorageSegment: ", buf, size);
    return persistentMemError = LonStatusNoError;
#elif PROCESSOR_IS(MC200)
    return persistentMemError = (iflash_drv_read(flashFd, buf, size, start)
                                         ? LonStatusNoError
                                         : LonStatusPersistentDataAccessError);
#else
    OsalPrintLog(ERROR_LOG, LonStatusPersistentDataAccessError,
            "HalReadStorageSegment: No persistent storage driver available");
    return persistentMemError = LonStatusPersistentDataAccessError;
#endif
}

/*****************************************************************
 * Section: USB Interface Function Declarations
 *****************************************************************/

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
static void HalCycleUsbPower(void);
void HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd);
void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost);
#endif  // PROCESSOR_IS(STM32) && OS_IS(FREERTOS)

/*****************************************************************
 * Section: USB Interface Function Definitions
 *****************************************************************/

/*
 * Opens the hardware-specific driver for interfacing with the LON USB
 * interface. Parameters: ldisc: line discipline to set on the USB interface, or
 * -1 to leave unchanged usb_dev_name: (Linux only) name of the USB device to
 * open (e.g. "/dev/ttyUSB0") usb_fd_out: (Linux only) pointer to variable to
 * receive the opened file descriptor Returns: LonStatusNoError (0) on success,
 * or an <LonStatusCode> error code on failure. Notes: On Linux, the specified
 * line discipline is set on the opened USB device. If ldisc is -1, the line
 * discipline is left unchanged (the caller is expected to have configured it
 * separately, e.g. via a udev rule).  The USB device is put into raw mode with
 * 8N1, no flow control, and DTR/RTS asserted.  On FreeRTOS, the line discipline
 * parameter is ignored and the USB interface is initialized with the necessary
 * settings for LON communication.
 */
LonStatusCode HalOpenUsb(int ldisc
#if OS_IS(LINUX)
        ,
        const char *usb_dev_name, int *usb_fd_out
#endif
)
{
    LonStatusCode status = LonStatusNoError;
    if (ldisc >= NR_LDISCS) {
        status = LonStatusInvalidParameter;
        OsalPrintLog(ERROR_LOG, status,
                "HalOpenUsb: Invalid USB line discipline parameter to open "
                "the LON USB device");
        return status;
    }
#if OS_IS(LINUX)
#if defined(__APPLE__)
    // Simulating successful open on macOS (no physical hardware)
    *usb_fd_out = -1;
    OsalPrintLog(INFO_LOG, status,
            "HalOpenUsb: Simulating successful open on macOS (no physical hardware)");
    return LonStatusNoError;
#endif
    if (!usb_dev_name || !*usb_dev_name || strlen(usb_dev_name) >= 256) {
        *usb_fd_out = -1;
        status = LonStatusInvalidParameter;
        OsalPrintLog(ERROR_LOG, status,
                "HalOpenUsb: Invalid USB device name parameter to open the "
                "LON USB device");
        return status;
    }
    *usb_fd_out = open(usb_dev_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (*usb_fd_out < 0) {
        status = LonStatusInterfaceError;
        OsalPrintLog(ERROR_LOG, status,
                "HalOpenUsb: Cannot open LON USB device %s, %s system error (errno %d)",
                usb_dev_name, strerror(errno), errno);
        return status;
    }
    // Set custom line discipline if requested
    if (ldisc >= 0) {
        if (ioctl(*usb_fd_out, TIOCSETD, &ldisc) < 0) {
            close(*usb_fd_out);
            status = LonStatusInterfaceError;
            OsalPrintLog(ERROR_LOG, status,
                    "HalOpenUsb: Cannot set line discipline %d on LON USB "
                    "device %s, %s system error (errno %d)",
                    ldisc, usb_dev_name, strerror(errno), errno);
            return status;
        }
    }
    // Set raw mode
    struct termios tio;
    if (tcgetattr(*usb_fd_out, &tio) < 0) {
        close(*usb_fd_out);
        status = LonStatusInterfaceError;
        OsalPrintLog(ERROR_LOG, status,
                "HalOpenUsb: Cannot get attributes for LON USB device %s, %s "
                "system error (errno %d)",
                usb_dev_name, strerror(errno), errno);
        return status;
    }
    cfmakeraw(&tio);
    // Explicitly configure 8N1, disable HW flow, enable receiver
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~PARENB;   // no parity
    tio.c_cflag &= ~CSTOPB;   // 1 stop bit
    tio.c_cflag &= ~CRTSCTS;  // no HW flow control
    // Belt-and-suspenders: ensure all echo flags are off.
    // cfmakeraw() should clear these, but some implementations
    // or CDC-ACM driver re-enumeration can leave them set, which
    // causes the tty layer to feed received bytes back to the
    // device; the device may then echo them back, producing
    // duplicate reads of the same frame.
    tio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    // Set a commonly used speed; for CDC-ACM many devices still expect it
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(*usb_fd_out, TCSANOW, &tio) < 0) {
        close(*usb_fd_out);
        status = LonStatusInterfaceError;
        OsalPrintLog(ERROR_LOG, status,
                "HalOpenUsb: Cannot set raw mode on LON USB device %s, %s "
                "system error (errno %d)",
                usb_dev_name, strerror(errno), errno);
        return status;
    }
    // Flush any data that arrived between open() and tcsetattr()
    // to prevent stale bytes from appearing on the first read
    tcflush(*usb_fd_out, TCIOFLUSH);
    // Assert DTR/RTS in case the interface requires it to exit reset
    int mflags = 0;
    if (ioctl(*usb_fd_out, TIOCMGET, &mflags) == 0) {
        mflags |= (TIOCM_DTR | TIOCM_RTS);
        (void)ioctl(*usb_fd_out, TIOCMSET, &mflags);
    }
    OsalPrintLog(INFO_LOG, status,
            "HalOpenUsb: Successfully opened LON USB device %s as interface "
            "%d (errno %d)",
            usb_dev_name, *usb_fd_out, errno);
    return status;
#elif PROCESSOR_IS(STM32)
    // Power up or power cycle the USB Port
    HalCycleUsbPower();
    // Initialize the USB Host Library
    if (USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS) != USBH_OK) {
        OsalPrintLog(ERROR_LOG, LonStatusOpenFailed, "USBH_Init failed");
        return LonStatusInterfaceError;
    }
    // Register the CDC Class
    if (USBH_RegisterClass(&hUsbHostFS, USBH_CDC_CLASS) != USBH_OK) {
        OsalPrintLog(ERROR_LOG, LonStatusOpenFailed,
                "HalOpenUsb: USBH_RegisterClass failed");
        return LonStatusInterfaceError;
    }
    // Start the USB Host Process
    if (USBH_Start(&hUsbHostFS) != USBH_OK) {
        OsalPrintLog(ERROR_LOG, LonStatusOpenFailed, "HalOpenUsb: USBH_Start failed");
        return LonStatusInterfaceError;
    }
    // Wait for Enumeration (timeout after 5 seconds)
    // The U60 needs time to negotiate with the STM32 host
    uint32_t start_time = OsalGetTickCount();
    while (!is_usb_cdc_ready) {
        // Drive the USB background state machine
        USBH_Process(&hUsbHostFS);
        HalHandleUsbCdcInitialization();
        if (!is_usb_cdc_ready && ((OsalGetTickCount() - start_time) > 5000)) {
            OsalPrintLog(ERROR_LOG, LonStatusOpenFailed,
                    "HalOpenUsb: USB Enumeration Timeout");
            return LonStatusInterfaceError;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return status;
#else
    // Placeholder: integrate with platform-specific open API when available.
    status = LonStatusNotImplemented;
    OsalPrintLog(ERROR_LOG, status,
            "HalOpenUsb: Implementation missing for the LON USB interface "
            "on this platform");
    return status;
#endif
}

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
/*
 * USB Host user process callback for handling USB events.
 * Parameters:
 *   phost: pointer to the USB host handle provided by the STM32 SDK
 *   id: USB host event ID indicating the type of event
 * Returns:
 *   None
 * Notes:
 *   This function is called by the STM32 USB Host Library to notify about
 *   various USB events such as device connection, disconnection, and class
 * activation.
 */
void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
    switch (id) {
    case HOST_USER_SELECT_CONFIGURATION:
        // No action needed for configuration selection in this implementation
        break;
    case HOST_USER_DISCONNECTION:
        is_usb_cdc_ready = false;
        cdc_init_state = 0;  // Reset state machine
        OsalPrintLog(INFO_LOG, LonStatusNoError,
                "USBH_UserProcess: LON USB device is disconnected");
        break;
    case HOST_USER_CLASS_ACTIVE:
        // The device is enumerated, but we need to do the CDC handshake
        cdc_init_state = 1;
        break;
    case HOST_USER_CONNECTION:
        OsalPrintLog(INFO_LOG, LonStatusNoError,
                "USBH_UserProcess: LON USB device connected - waiting for enumeration");
        break;
    default:
        OsalPrintLog(INFO_LOG, LonStatusNoError,
                "USBH_UserProcess: Unhandled USB event ID %d", id);
        break;
    }
}
#endif  // PROCESSOR_IS(STM32) && OS_IS(FREERTOS)

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
/*
 * Power cycles the USB port by toggling the control pin.
 * Parameters: None
 * Returns:    None
 * Notes:
 *   This function is specific to the FreeRTOS platform.
 */
static void HalCycleUsbPower(void)
{
    // Power cycle the USB Port
    IO_SetPinLow(IO_USB_CTRL_PWR);
    vTaskDelay(pdMS_TO_TICKS(10));
    IO_SetPinHigh(IO_USB_CTRL_PWR);
    vTaskDelay(pdMS_TO_TICKS(100));  // Give the U60 time to boot
}
#endif  // PROCESSOR_IS(STM32) && OS_IS(FREERTOS)

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
/*
 * Performs the CDC class-specific initialization handshake with the connected
 * USB device. Parameters: None Returns:    None Notes: This function is called
 * periodically from the main loop after the USB device is connected and the
 * class is active. It implements a simple state machine to perform the
 * necessary CDC control requests to set up the device for communication. Once
 * the handshake is complete, it sets a flag to indicate that the USB CDC
 * interface is ready for use.
 */
void HalHandleUsbCdcInitialization(void)
{
    // Only run if the class is active but our custom handshake isn't finished
    if (cdc_init_state == 0 || is_usb_cdc_ready) {
        return;
    }
    switch (cdc_init_state) {
    case 1:
        // Set USB CDC control line state to 0 (DTR=0, RTS=0) to start the handshake
        if (USBH_CDC_SetControlLineState(&hUsbHostFS, 0x0000) == USBH_OK) {
            cdc_init_state = 2;
        }
        break;

    case 2:
        // Get USB CDC line coding to read the current settings (some devices
        // require this before accepting the SetLineCoding)
        if (USBH_CDC_GetLineCoding(&hUsbHostFS, &cdc_line_coding) == USBH_OK) {
            // Modify the struct for the LON USB requirements (e.g., 115200 8N1)
            cdc_line_coding.b.dwDTERate = 115200;
            cdc_line_coding.b.bCharFormat = 0;  // 1 stop bit
            cdc_line_coding.b.bParityType = 0;  // None
            cdc_line_coding.b.bDataBits = 8;    // 8 bits
            cdc_init_state = 3;
        }
        break;
    case 3:
        // Set USB CDC line coding with the modified settings. This is required
        // for some devices to accept the control line state change in the next step
        if (USBH_CDC_SetLineCoding(&hUsbHostFS, &cdc_line_coding) == USBH_OK) {
            cdc_init_state = 4;
        }
        break;

    case 4:
        // Set DTR (bit 0) and RTS (bit 1) and arm the USB receive endpoint
        if (USBH_CDC_SetControlLineState(&hUsbHostFS, 0x0003) == USBH_OK) {
            // Handshake complete -- mark the USB interface as fully ready
            is_usb_cdc_ready = true;
            cdc_init_state = 0;
            OsalPrintLog(INFO_LOG, LonStatusNoError,
                    "HalHandleUsbCdcInitialization: LON USB CDC handshake complete");
            // Arm the USB receive endpoint to listen for the first packet
            USBH_CDC_Receive(&hUsbHostFS, raw_usb_rx_buffer, sizeof(raw_usb_rx_buffer));
        }
        break;
    default:
        OsalPrintLog(ERROR_LOG, LonStatusInterfaceError,
                "HalHandleUsbCdcInitialization: Invalid CDC initialization state %d",
                cdc_init_state);
        break;
    }
}
#endif  // PROCESSOR_IS(STM32) && OS_IS(FREERTOS)

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
/*
 * Overrides the weak STM32 Host Controller Driver (HCD) MCU Specific Packet
 * initialization (MspInit) function to initialize the USB host controller
 * hardware. Parameters: hhcd: pointer to the HCD handle provided by the STM32
 * SDK Returns: None Notes: This function is called by the STM32 SDK during USB
 * host initialization.
 */
void HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd)
{
    VAR_UNUSED(hhcd);

    IO_PinInit(IO_USB_CTRL_PWR);  // Power ON by default
    vTaskDelay(100);              // let the IO for DP pin to be low for 20 mSec.
    IO_PinInit(IO_USB_FS_DM);
    MODIFY_REG(RCC->CCIPR4, RCC_CCIPR4_USBSEL, (uint32_t)(RCC_USBCLKSOURCE_PLL3Q));

    // Enable VDDUSB
    SET_BIT(PWR->USBSCR, PWR_USBSCR_USB33SV);
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USBEN);

    // USB_DRD_FS interrupt Init
    ISR_Init(USB_DRD_FS_IRQn, 3);
}
#endif  // PROCESSOR_IS(STM32) && OS_IS(FREERTOS)

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
/*
 * Overrides the weak STM32 USB uplink callback.
 * Parameters:
 *   phost: pointer to the USB host handle provided by the STM32 SDK
 * Returns:
 *   None
 * Notes:
 *   The USB hardware interrupt handler in the STM32 SDK calls into the
 *   USB library's ISR processing, which automatically calls this function
 *   whenever a new packet arrives.
 */
void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost)
{
    // Get the length of the newly arrived data
    uint32_t length = USBH_CDC_GetLastReceivedDataSize(phost);
    if (length == 0) {
        // No data received, this should not happen but guard against it
        OsalPrintLog(INFO_LOG, LonStatusNoError,
                "USBH_CDC_ReceiveCallback: Received callback with zero-length data");
        // Re-arm the USB receive endpoint to listen for the next packet
        USBH_CDC_Receive(phost, raw_usb_rx_buffer, sizeof(raw_usb_rx_buffer));
        return;
    }
    // Get the pointer to the library's internal RX buffer
    uint8_t *data = USBH_CDC_GetRXBuffer(phost);
    // Hand the data directly to the LON Stack's ISR processor
#if LINK_IS(MULTIPLE_USB_MIPS)
    LonUsbMsgIsr(0, data, length);  // Pass interface index 0 if using multiple
#else
    LonUsbMsgIsr(data, length);
#endif
    // Re-arm the USB receive endpoint to listen for the next packet
    USBH_CDC_Receive(phost, raw_usb_rx_buffer, sizeof(raw_usb_rx_buffer));
}
#endif  // OS_IS(FREERTOS)

#if PROCESSOR_IS(STM32) && OS_IS(FREERTOS)
/*
 * Overrides the STM32 weak function to get the current TX state of the USB CDC
 * interface. Parameters: pHost: Pointer to the USB host handle provided by the
 * STM32 SDK Returns: The current TX state as a CDC_DataStateTypeDef value
 * Notes:
 *   Returns CDC_IDLE if the CDC handle is not initialized.
 */
CDC_DataStateTypeDef USBH_CDC_GetTX_State(USBH_HandleTypeDef *pHost)
{
    CDC_HandleTypeDef *pCDC_Handle = (CDC_HandleTypeDef *)pHost->pActiveClass->pData;
    if (pCDC_Handle == NULL) {
        return CDC_IDLE;  // There is no error state in the CDC library, return IDLE
    }
    return pCDC_Handle->data_tx_state;
}
#endif  // PROCESSOR_IS(STM32) && OS_IS(FREERTOS)

#if LINK_IS(MULTIPLE_USB_MIPS)
/*
 * Closes the hardware-specific driver for interfacing with the LON USB
 * interface. Parameters: fd: (Linux only) File descriptor of the opened LON USB
 * network interface Returns: None
 */
void HalCloseUsb(
#ifdef OS_IS(LINUX)
        int fd)
#else
        void)
#endif
{
#if OS_IS(LINUX)
    if (fd >= 0) {
        close(fd);
    }
#else
    // Placeholder: integrate with platform-specific open API when available.
    status = LonStatusNotImplemented;
    OsalPrintLog(ERROR_LOG, status,
            "HalCloseUsb: Implementation missing for the LON USB interface "
            "on this platform");
    return status;
#endif
}
#endif  // LINK_IS(MULTIPLE_USB_MIPS)

/*
 * Writes data to the LON USB network interface.
 * Parameters:
 *   fd: File descriptor of the opened LON USB network interface
 *   buf: Pointer to the data buffer to write
 *   len: Number of bytes to write
 *   bytes_written: Pointer to size_t to receive the number of bytes written
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful.
 * Notes:
 *   The entire buffer is written unless a non-recoverable error occurs.
 *   For Linux, it retries on EINTR and EAGAIN. For partial progress followed
 *   by error, the already-written byte count is returned via bytes_written.
 */
LonStatusCode HalWriteUsb(int usb_fd, const void *buf, size_t len, size_t *bytes_written)
{
    LonStatusCode status = LonStatusNoError;
    if (bytes_written) {
        *bytes_written = 0;
    }
#if OS_IS(LINUX)
#if defined(__APPLE__)
    // Simulating successful write on macOS (no physical hardware)
    if (usb_fd == -1) {
        if (bytes_written) *bytes_written = len;
        OsalPrintLog(PACKET_TRACE_LOG, status,
                "HalWriteUsb: Simulating write of %zu bytes on macOS", len);
        return LonStatusNoError;
    }
#endif
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;
    const int MAX_POLL_MS = 5000;  // Overall soft budget
    const int SLICE_MS = 100;      // Poll slice
    int elapsed = 0;
    struct pollfd pfd;
    pfd.fd = usb_fd;
    pfd.events = POLLOUT;
    while (total < len) {
        ssize_t n = write(usb_fd, p + total, len - total);
        if (n > 0) {
            total += (size_t)n;
            continue;
        }
        if (n == -1) {
            if (errno == EINTR) {
                continue;  // Transient
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                int to = (elapsed + SLICE_MS > MAX_POLL_MS) ? (MAX_POLL_MS - elapsed)
                                                            : SLICE_MS;
                if (to <= 0) {  // Budget exhausted
                    if (bytes_written) {
                        *bytes_written = total;
                    }
                    status = LonStatusTimeout;
                    OsalPrintLog(ERROR_LOG, status,
                            "HalWriteUsb: Timeout after writing %zu/%zu bytes for "
                            "LON USB interface %d",
                            total, len, usb_fd);
                    return status;
                }
                int pr = poll(&pfd, 1, to);
                if (pr < 0) {
                    if (errno == EINTR) {
                        continue;  // Reattempt
                    }
                    if (bytes_written) {
                        *bytes_written = total;
                    }
                    status = LonStatusWriteFailed;
                    OsalPrintLog(ERROR_LOG, status,
                            "HalWriteUsb: Write status poll error with %s system "
                            "error (errno %d) for LON USB interface %d",
                            strerror(errno), errno, usb_fd);
                    return status;
                }
                elapsed += to;
                continue;  // Try write again
            }
            if (errno == ENODEV || errno == EIO) {
                status = LonStatusInterfaceError;
            } else if (errno == ETIMEDOUT) {
                status = LonStatusTimeout;
            }
            if (bytes_written) {
                *bytes_written = total;
            }
            OsalPrintLog(ERROR_LOG, status,
                    "HalWriteUsb: Write failed after %zu/%zu bytes for LON USB "
                    "interface %d, %s system error (errno=%d)",
                    total, len, usb_fd, strerror(errno), errno);
            return status;
        }
    }
    if (bytes_written) {
        *bytes_written = total;
    }
    OsalPrintLog(PACKET_TRACE_LOG, status,
            "HalWriteUsb: Wrote %zu bytes to LON USB interface %d", total, usb_fd);
    OsalPrintMessage(PACKET_TRACE_LOG, "HalWriteUsb: ", buf, (size_t)total);
#elif PROCESSOR_IS(STM32)
    // Check the flag we set in HalOpenUsb() to ensure the U60 is fully enumerated
    if (!is_usb_cdc_ready) {
        status = LonStatusNotOpen;
        OsalPrintLog(ERROR_LOG, status,
                "HalWriteUsb: Attempt to write before LON USB interface is ready");
        return status;  // Port is not ready
    }

    // Attempt to transmit using the STM32 USB Host Library
    USBH_StatusTypeDef usbh_status =
            USBH_CDC_Transmit(&hUsbHostFS, (uint8_t *)pData, (uint32_t)dataLen);
    if (usbh_status == USBH_OK) {
        // Success: Return the number of bytes written so the LON stack proceeds
        if (bytes_written) {
            *bytes_written = (size_t)dataLen;
        }
        return status;
    } else if (usbh_status == USBH_BUSY) {
        // The USB hardware is currently busy transmitting a previous packet.
        // Returning 0 tells the LON stack that no bytes were written yet,
        // allowing it to safely buffer/retry on the next tick without dropping
        // data.
        return status;
    } else {
        // Hardware error or disconnection
        status = LonStatusInterfaceError;
        OsalPrintLog(ERROR_LOG, status,
                "HalWriteUsb: USB transmit error %d for LON USB interface, %s",
                usbh_status,
                (usbh_status == USBH_FAIL) ? "hardware error or disconnection"
                                           : "unknown error");
        return status;
    }
#else
    // Placeholder: integrate with platform-specific open API when available.
    if (bytes_written) {
        *bytes_written = 0;
    }
    status = LonStatusNotImplemented;
    OsalPrintLog(ERROR_LOG, status,
            "HalWriteUsb: Implementation missing for the LON USB interface "
            "on this platform");
#endif
    return status;
}

#if USB_UPLINK_IS(POLLING)
/*
 * Polls and reads data from the LON USB network interface.
 * Parameters:
 *   fd: File descriptor of the opened LON USB network interface
 *   buf: Pointer to the data buffer to receive read data
 *   len: Number of bytes to read
 *   bytes_read: Pointer to ssize_t to receive the number of bytes read
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   This function performs a non-blocking read. If no data is available,
 *   it returns LonStatusNoMessageAvailable. If data is available, it reads
 *   up to 'len' bytes and returns the number of bytes read via 'bytes_read'.
 *   Drivers can call this function periodically to retrieve incoming data.
 */
LonStatusCode HalReadUsb(int usb_fd, void *buf, size_t len, ssize_t *bytes_read)
{
    LonStatusCode status = LonStatusNoError;
#if OS_IS(LINUX)
#if defined(__APPLE__)
    // Simulating no data available on macOS (no physical hardware)
    if (usb_fd == -1) {
        *bytes_read = 0;
        return LonStatusNoMessageAvailable;
    }
#endif
    if (usb_fd < 0 || !buf || len == 0 || !bytes_read) {
        status = LonStatusInvalidParameter;
        OsalPrintLog(ERROR_LOG, status,
                "HalReadUsb: Invalid parameter to read from the LON USB device");
        return status;
    }
    *bytes_read = 0;  // Ensure caller never sees stale counts on error
    *bytes_read = read(usb_fd, buf, len);
    if (*bytes_read <= 0) {
        // Clear buffer so callers don't interpret stale bytes when no data is
        // available
        if (buf && len > 0) {
            memset(buf, 0, len);
        }
        if (*bytes_read == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            return LonStatusNoMessageAvailable;
        }
        status = LonStatusReadFailed;
        OsalPrintLog(ERROR_LOG, status,
                "HalReadUsb: Read error %s (errno %d) for LON USB interface %d",
                strerror(errno), errno, usb_fd);
        return status;
    }
    char plural[4] = "s";
    if (*bytes_read == 1) {
        plural[0] = '\0';
    };
    OsalPrintLog(PACKET_TRACE_LOG, status,
            "HalReadUsb: Read %zd byte%s from LON USB interface %d", *bytes_read, plural,
            usb_fd);
    OsalPrintMessage(PACKET_TRACE_LOG, "HalReadUsb: ", buf, (size_t)(*bytes_read));
#else
    // Placeholder: integrate with platform-specific open API when available.
    status = LonStatusNotImplemented;
    OsalPrintLog(ERROR_LOG, status,
            "HalReadUsb: Implementation missing for the LON USB interface "
            "on this platform");
#endif
    return status;
}
#endif  // USB_UPLINK_IS(POLLING)

#if USB_SERVICE_IS(PUMP)
/*
 * Service function to pump the USB network interface.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success; LonStatusCode error code if unsuccessful
 * Notes:
 *   When enabled, this is called periodically from the LCS_Service() event
 *   pump.
 */
LonStatusCode HalUsbService(void)
{
    LonStatusCode status = LonStatusNoError;
#if PROCESSOR_IS(STM32)
    USBH_Process(&hUsbHostFS);
    HalHandleUsbCdcInitialization();
#else   // PROCESSOR_IS(STM32)
    // Placeholder: integrate with platform-specific service API when available
    status = LonStatusNotImplemented;
    OsalPrintLog(ERROR_LOG, status,
            "HalUsbService: Implementation missing for the LON USB "
            "interface on this platform");
#endif  // PROCESSOR_IS(STM32)
    return status;
}
#endif  // USB_SERVICE_IS(PUMP)

/*****************************************************************
 * Section: MAC Address Function Definition
 *****************************************************************/

#if LINK_IS(UDP)
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
 *   TODO: add implementation for USB and MIP interfaces
 */
LonStatusCode HalGetMacAddress(unsigned char *mac)
{
#if (OS_IS(LINUX) || defined(__APPLE__) || defined(__unix__)) && !PROCESSOR_IS(MC200)
    // Unix-like systems (Linux, macOS, BSD)
    const char *iface = "eth0";
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        // Socket error
        return LonStatusDeviceUniqueIdNotAvailable;
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
#if defined(__linux__) && !defined(__APPLE__)
    // Linux uses SIOCGIFHWADDR
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
        close(fd);
        return LonStatusDeviceUniqueIdNotAvailable;
    }
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) ||              \
        defined(__NetBSD__)
    // macOS/BSD use getifaddrs() - ioctl doesn't work the same way
    // For now, return not available on macOS
    close(fd);
    return LonStatusDeviceUniqeIdNotAvailable;
#else
    close(fd);
    return LonStatusDeviceUniqeIdNotAvailable;
#endif
    close(fd);
    return LonStatusNoError;
#elif PROCESSOR_IS(MC200)
    return (wlan_get_mac_address(mac) ? LonStatusDeviceUniqeIdNotAvailable
                                      : LonStatusNoError);
#else
    return LonStatusDeviceUniqeIdNotAvailable;
#endif
}
#endif  // LINK_IS(UDP)

/*****************************************************************
 * Section: Reboot Function Definition
 *****************************************************************/

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

#if defined(__linux__) && !defined(__APPLE__)
    // Linux-specific reboot (requires sys/reboot.h and RB_AUTOBOOT)
    // Sync filesystems before rebooting
    sync();

    // Attempt to reboot the system (Requires root privileges)
    if (reboot(RB_AUTOBOOT) != 0) {
        // Reboot failure
        return ret;
    }
#elif PROCESSOR_IS(MC200)
    arch_reboot();
#else
    // Platform doesn't support reboot, or not implemented
    ret = LonStatusNotImplemented;
#endif
    // Should not reach here
    return ret;
}

#if PROCESSOR_IS(STM32)
/*****************************************************************
 * Section: Hardware Interrupt Handlers
 *****************************************************************/

extern HCD_HandleTypeDef hhcd_USB_DRD_FS;

void USB_DRD_FS_IRQHandler(void) { HAL_HCD_IRQHandler(&hhcd_USB_DRD_FS); }
#endif  // PROCESSOR_IS(STM32)

#ifdef __cplusplus
}
#endif
