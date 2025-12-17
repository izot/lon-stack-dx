/*
 * lon_usb_link.c
 *
 * Temporary stub implementation for the LON USB link interface.
 *
 * This is used to satisfy the LDV-style link API (OpenLonLink/ReadLonLink/...) when
 * LINK_ID is configured for USB but no platform-specific USB driver integration is
 * present in this repository.
 */

#include "izot/IzotApi.h"

LonStatusCode OpenLonUsbLink(const char *name, short *handle)
{
    (void)name;
    (void)handle;
    return LonStatusNotImplemented;
}

LonStatusCode CloseLonUsbLink(short handle)
{
    (void)handle;
    return LonStatusNotImplemented;
}

LonStatusCode ReadLonUsbLink(short handle, void *msg, short len)
{
    (void)handle;
    (void)msg;
    (void)len;
    return LonStatusNotImplemented;
}

LonStatusCode WriteLonUsbLink(short handle, void *msg, short len)
{
    (void)handle;
    (void)msg;
    (void)len;
    return LonStatusNotImplemented;
}
