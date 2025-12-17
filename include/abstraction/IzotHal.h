/*
 * IzotHal.h
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

#if !defined(IZOTHAL_H)
#define IZOTHAL_H

#ifdef  __cplusplus
extern "C"
{
#endif  // __cplusplus

#include "izot/IzotPlatform.h"
#include "abstraction/IzotOsal.h"

#define LINUX_FLASH_OFFSET      0
#define FREERTOS_FLASH_OFFSET   0x6000
#define FLASH_OFFSET            0x6000
#define FLASH_REGION_SIZE       0x6000
#define NUM_OF_BLOCKS           6
#define BLOCK_SIZE              0x1000
#define NO_OF_REGIONS           1

/*****************************************************************
 * Section: Storage Function Declarations
 *****************************************************************/

/*
 * Initializes the hardware-specific driver for interfacing with
 * persistent memory.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
extern LonStatusCode HalFlashDrvInit(void);

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
extern LonStatusCode HalGetFlashInfo(size_t *offset, size_t *region_size,
        int *number_of_blocks, size_t *block_size, int *number_of_regions
);

/*
 * Opens the hardware-specific driver for interfacing with 
 * persistent memory.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
extern LonStatusCode HalFlashDrvOpen(void);

/*
 * Closes the hardware-specific driver for interfacing with
 * persistent memory.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
extern LonStatusCode HalFlashDrvClose(void);

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
extern LonStatusCode HalFlashDrvErase(size_t start, size_t size);

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
extern LonStatusCode HalFlashDrvWrite(IzotByte *buf, size_t start, size_t size);

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
extern LonStatusCode HalFlashDrvRead(IzotByte *buf, size_t start, size_t size);

/*****************************************************************
 * Section: LON USB Interface Abstraction Function Declarations
 *****************************************************************/
// Callback for async read: receives buffer, byte count, user data
typedef void (*usbtty_read_callback_t)(const char *buf, ssize_t len, void *user);

// USB TTY context structure
typedef struct usbtty_ctx {
    int fd;                     // File descriptor for the open tty device
    usbtty_read_callback_t cb;  // Pointer to the user-supplied callback function
    void *user;                 // Optional user data pointer passed to the callback
    OsalThreadId thread;        // Thread handle for the asynchronous read thread
    volatile int stop;          // Flag to signal the thread to stop reading and exit
} usbtty_ctx;

// Open USB tty device with custom line discipline; returns file descriptor or <0 on error
int HalOpenUsb(const char *usb_dev_name, int ldisc);

// Close device (just closes fd)
void HalCloseUsb(int fd);

/*
 * Writes buffer to LON USB network interface.
 * Reliable write helper for user-space file descriptors (TTY/USB).
 * Attempts to write the entire buffer unless a non-recoverable error occurs.
 * Retries on EINTR and EAGAIN. For partial progress followed by error, the
 * already-written byte count is returned via bytes_written.
 */
LonStatusCode HalWriteUsb(int fd, const void *buf, size_t len, size_t *bytes_written);

// Polling USB read abstraction. Returns LonStatusNoError with *bytes_read set
// to the number of bytes read, LonStatusNoMessageAvailable if no data is available,
// or another LonStatusCode on error.
LonStatusCode HalReadUsb(int fd, void *buf, size_t len, ssize_t *bytes_read);

/*****************************************************************
 * Section: MAC Address Function Definition
 *****************************************************************/
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
extern LonStatusCode HalGetMacAddress(unsigned char *mac);

/*****************************************************************
 * Section: Reboot Function Declaration
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
extern LonStatusCode HalReboot(void);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  /* defined(IZOTHAL_H) */
