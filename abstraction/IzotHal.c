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

#include "izot/IzotPlatform.h" // Project-specific configuration

// Forward declare sync() for platforms that need it
#if defined(__APPLE__) || defined(__unix__) || defined(__linux__)
extern void sync(void);
#endif

// Include networking headers for all Unix-like systems
// MUST come after sys/types.h and in this specific order for macOS
#if OS_IS(LINUX) || defined(__unix__) || defined(__APPLE__) || defined(_POSIX_VERSION)
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#ifdef __APPLE__
#include <sys/sockio.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif
#ifdef __linux__
#include <linux/if_packet.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#ifndef SIOCGIFHWADDR
#define SIOCGIFHWADDR 0x8927
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#endif

// Additional Linux-specific headers (not macOS)
#if OS_IS(LINUX) && defined(__linux__)
#include <sys/reboot.h>
#endif

// Linux kernel headers
#if OS_IS(LINUX_KERNEL)
    #include <linux/if.h>
    #include <linux/netdevice.h>
#endif // OS_IS(LINUX) || OS_IS(LINUX_KERNEL)

#if OS_IS(LINUX) || OS_IS(FREERTOS)
#include <sys/types.h>
#endif // OS_IS(LINUX) || OS_IS(FREERTOS)

#if PROCESSOR_IS(MC200)
#include <wm_os.h>
#include <mdev.h>
#include <mc200_gpio.h>
#include <mc200_pinmux.h>
#endif // PROCESSOR_IS(MC200)

#ifdef  __cplusplus
extern "C" {
#endif

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
 * Section: Storage Function Definitions
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

    if (!LON_SUCCESS(persistentMemError)) {
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
    if (!LON_SUCCESS(persistentMemError)) {   
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
    if (!LON_SUCCESS(persistentMemError)) {   
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
    if (!LON_SUCCESS(persistentMemError)) {   
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
    if (!LON_SUCCESS(persistentMemError)) {   
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
    if (!LON_SUCCESS(persistentMemError)) {   
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
    if (!LON_SUCCESS(persistentMemError)) {   
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
    if (!LON_SUCCESS(persistentMemError)) {   
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

/*****************************************************************
 * Section: USB TTY Interface Function Definitions
 *****************************************************************/
int HalOpenUsb(const char *usb_dev_name, int ldisc)
{
    if (!usb_dev_name || ldisc < 0 || ldisc >= NR_LDISCS) {
        return -1;
    }
#if OS_IS(LINUX)
    int fd = open(usb_dev_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return -1;

    // Set custom line discipline if requested
    if (ldisc >= 0) {
        if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
            close(fd);
            return -2;
        }
    }

    // Set raw mode
    struct termios tio;
    if (tcgetattr(fd, &tio) < 0) {
        close(fd);
        return -3;
    }
    cfmakeraw(&tio);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        close(fd);
        return -4;
    }
    return fd;
#elif OS_IS(FREERTOS)
    // For FreeRTOS, assume descriptor is a UART-like driver and not standard POSIX fd.
    // Placeholder: integrate with platform-specific open API when available.
    // int fd = open(usb_dev_name, ...);
    // if (fd < 0) return -1;
    // return fd;
    return -1; // Not implemented
#else
    // Placeholder: integrate with platform-specific open API when available.
    return -1; // Not implemented
#endif
}

void HalCloseUsb(int fd)
{
    if (fd >= 0)
        close(fd);
}

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
LonStatusCode HalWriteUsb(int fd, const void *buf, size_t len, size_t *bytes_written)
{
#if OS_IS(LINUX)
    const uint8_t *p = (const uint8_t*)buf;
    size_t total = 0;
    if (bytes_written) *bytes_written = 0;
    const int MAX_POLL_MS = 5000;        // overall soft budget
    const int SLICE_MS = 100;            // poll slice
    int elapsed = 0;
    struct pollfd pfd; pfd.fd = fd; pfd.events = POLLOUT;
    while (total < len) {
        ssize_t n = write(fd, p + total, len - total);
        if (n > 0) { total += (size_t)n; continue; }
        if (n == -1) {
            if (errno == EINTR) continue; // transient
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                int to = (elapsed + SLICE_MS > MAX_POLL_MS) ? (MAX_POLL_MS - elapsed) : SLICE_MS;
                if (to <= 0) { // budget exhausted
                    if (bytes_written) *bytes_written = total;
                    return LonStatusTimeout;
                }
                int pr = poll(&pfd, 1, to);
                if (pr < 0) {
                    if (errno == EINTR) continue; // reattempt
                    if (bytes_written) *bytes_written = total;
                    OsalPrintError(LonStatusWriteFailed, "HalWriteUsb poll error (errno=%d)", errno);
                    return LonStatusWriteFailed;
                }
                elapsed += to;
                continue; // try write again
            }
            LonStatusCode ec = LonStatusWriteFailed;
            if (errno == ENODEV || errno == EIO) ec = LonStatusInterfaceError;
            else if (errno == ETIMEDOUT) ec = LonStatusTimeout;
            if (bytes_written) *bytes_written = total;
            OsalPrintError(ec, "HalWriteUsb failed after %zu/%zu bytes (errno=%d)", total, len, errno);
            return ec;
        }
    }
    if (bytes_written) *bytes_written = total;
#elif OS_IS(FREERTOS)
    // For FreeRTOS, assume descriptor is a UART-like driver and not standard POSIX fd.
    // Placeholder: integrate with platform-specific write API when available.
    // size_t total = 0;
    // const uint8_t *p = (const uint8_t*)buf;
    // while (total < len) {
    // int rc = write(fd, p + total, len - total); // depends on provided BSP
    //     if (rc > 0) { total += (size_t)rc; continue; }
    //     if (rc < 0) { if (bytes_written) *bytes_written = total; return LonStatusWriteFailed; }
    // }
    if (bytes_written) *bytes_written = 0;
    OsalPrintError(LonStatusWriteFailed, "HalWriteUsb() not implemented");
    return LonStatusWriteFailed;
#else
    // Placeholder: integrate with platform-specific write API when available.
    // size_t total = 0; const uint8_t *p = (const uint8_t*)buf;
    // while (total < len) {
    //     ssize_t n = write(fd, p + total, len - total);
    //     if (n <= 0) { if (bytes_written) *bytes_written = total; return LonStatusWriteFailed; }
    //     total += (size_t)n;
    // }
    // if (bytes_written) *bytes_written = total;
    // return LonStatusNoError;
    if (bytes_written) *bytes_written = 0;
    OsalPrintError(LonStatusWriteFailed, "HalWriteUsb() not implemented");
    return LonStatusWriteFailed;
#endif
    return LonStatusNoError;
}

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
 *   Drivers can call this function periodically to retrieve incoming data,
 *   or implement code to asynchronously call LonUsbFeedRx() to feed data
 *   received from the LON USB network interface into the RX ring buffer.
 */
LonStatusCode HalReadUsb(int fd, void *buf, size_t len, ssize_t *bytes_read)
{
#if OS_IS(LINUX)
    if (fd < 0 || !buf || len == 0 || !bytes_read) {
        OsalPrintError(LonStatusInvalidParameter, "HalReadUsb invalid parameter");
        return LonStatusInvalidParameter;
    }
    *bytes_read = read(fd, buf, len);
    if (*bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return LonStatusNoMessageAvailable;
        }
        OsalPrintError(LonStatusReadFailed, "Read error %d", errno);
        return LonStatusReadFailed;
    }
    if (*bytes_read == 0) {
        OsalPrintError(LonStatusInterfaceError, "Read returned 0 bytes; device may be disconnected");
        return LonStatusInterfaceError;
    }
    OsalPrintTrace(LonStatusNoError, "Read %zd bytes", *bytes_read);
    return LonStatusNoError;
#elif OS_IS(FREERTOS)
    // HalReadUsb for FreeRTOS depends on the specific UART or USB stack in use;
    // implement a non-blocking read from the LON USB network interface here,
    // or implement code to asynchronously call LonUsbFeedRx() to feed data 
    // received from the LON USB network interface into the RX ring buffer
    #pragma message("Optional: implement OS-dependent definition of HalReadUsb()")
    return LonStatusNoMessageAvailable;
#else
    // Implement a non-blocking read from the LON USB network interface here,
    // or implement code to asynchronously call LonUsbFeedRx() to feed data 
    // received from the LON USB network interface into the RX ring buffer
    #pragma message("Optional: implement OS-dependent definition of HalReadUsb()")
    return LonStatusNoMessageAvailable;
#endif
}

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
        return LonStatusDeviceUniqeIdNotAvailable;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);

#if defined(__linux__) && !defined(__APPLE__)
    // Linux uses SIOCGIFHWADDR
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
        close(fd);
        return LonStatusDeviceUniqeIdNotAvailable;
    }
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
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
    return (wlan_get_mac_address(mac) ? LonStatusDeviceUniqeIdNotAvailable : LonStatusNoError);
#else
    return LonStatusDeviceUniqeIdNotAvailable;
#endif
}

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

#ifdef  __cplusplus
}
#endif

