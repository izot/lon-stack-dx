//
// IzotCal.c
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
 * Title: Connectivity Abstaction Layer file
 *
 * Abstract:
 * This file contains the platform dependent APIs. This file contains
 * has APIs for FreeRTOS to send data on a udp socket.
 */

#if PLATFORM_IS(FRTOS)
#include <app_framework.h>
#include <appln_cb.h>
#include <wm_net.h>
#include <wm_os.h>
#endif

#include "IzotTypes.h"
#include "IzotHal.h"
#include "IzotCal.h"
#include "IzotOsal.h"

#if PROCESSOR_IS(MC200)
#include <board.h>
#include <mdev_gpio.h>
#include <mdev_pinmux.h>
#endif

#include "IPv4ToLsUdp.h"
#include "lcs_api.h"

/*------------------------------------------------------------------------------
Section: Macro
------------------------------------------------------------------------------*/
#define MAX_SSID_LEN      15
#define MAX_HOST_NAME_LEN 10
#define MAC_ID_LEN        6
#define UAP_PASSPHRASE    "echelonizot"
#define FTFS_API_VERSION  100
#define FTFS_PART_NAME    "ftfs"

/*------------------------------------------------------------------------------
Section: Global
------------------------------------------------------------------------------*/
IzotByte    ownIpAddress[IPV4_ADDRESS_LEN]; // Buffer to store the IP addresse
IzotBool    is_connected;            // Flag to define network connected or not

/*------------------------------------------------------------------------------
Section: Static
------------------------------------------------------------------------------*/
static int        app_udp_socket = -1;
static int        provisioned;
static struct fs *fs;
static char       ssid_uap[MAX_SSID_LEN];
static char       dhcp_host_name[MAX_HOST_NAME_LEN];
static uint8_t    connecting = 0;
static uint32_t   previous, current = 0;

/*
 * *****************************************************************************
 * SECTION: FUNCTIONS
 * *****************************************************************************
 *
 */
 
/*
 * Function: appln_critical_error_handler
 * 
 * This function is defined for handling critical error.
 * For this application, we just stall and do nothing when
 * a critical error occurs.
 */
void appln_critical_error_handler(void *data)
{
    while (1)
        ;
    // do nothing -- stall
}

/* 
 * Function: EventWlanInitDone
 * 
 * Handler invoked on WLAN_INIT_DONE event.
 * 
 * When WLAN is started, the application framework looks to
 * see whether a home network information is configured
 * and stored in PSM (persistent storage module).
 * 
 * The data field returns whether a home network is provisioned
 * or not, which is used to determine what network interfaces
 * to start (station, micro-ap, or both).
 * 
 * If provisioned, the station interface of the device is
 * connected to the configured network.
 * 
 * Else, Micro-AP network is configured.
 * 
 * (If desired, the Micro-AP network can also be started
 * along with the station interface.)
 * 
 * We also start all the services which don't need to be
 * restarted between provisioned and non-provisioned mode
 * or between connected and disconnected state.
 * 
 * Accordingly:
 * -- Start mDNS and advertize services
 * -- Start HTTP Server
 * -- Register WSGI handlers for HTTP server
 */ 
static void EventWlanInitDone(void *data)
{
    int ret;
    unsigned char mac[MAC_ID_LEN];
    
    // We receive provisioning status in data
    provisioned = (int)data;

    CAL_Printf("AF_EVT_WLAN_INIT_DONE provisioned=%d\r\n", provisioned);

    //Limiting the number of WiFi Channels to 11
    wifi_sub_band_set_t sb = {
    .first_chan = 1,
    .no_of_chan= 11,
    .max_tx_pwr = 30,
    };

    wifi_domain_param_t *dp = os_mem_alloc(sizeof(wifi_domain_param_t) +
    										sizeof(wifi_sub_band_set_t));

	memcpy(dp->country_code, "US\0", COUNTRY_CODE_LEN);
	dp->no_of_sub_band = 1;
	memcpy(dp->sub_band, &sb, sizeof(wifi_sub_band_set_t));

	wifi_uap_set_domain_params(dp);
	os_mem_free(dp);

    // Get the MAC address of WIFI driver
    HalGetMacAddress(mac);
    
    // Copy only Last two bytes of MAC address to the SSID
    snprintf(ssid_uap, MAX_SSID_LEN, "CPM-4200-%02X%02X", mac[4], mac[5]);
    snprintf(dhcp_host_name, MAX_HOST_NAME_LEN, "IZOT-%02X%02X", mac[4], mac[5]);
    CAL_Printf("SSID: %s\r\n", ssid_uap);
    CAL_Printf("DHCP host Name: %s\r\n", dhcp_host_name);
    
    if (provisioned) {
        app_sta_start();
    } else {
        app_uap_start_with_dhcp(ssid_uap, UAP_PASSPHRASE);
    }

    // 
    // Start http server and enable webapp in the
    // FTFS partition on flash
    // 
    ret = app_httpd_with_fs_start(FTFS_API_VERSION, FTFS_PART_NAME, &fs);
    if (ret != WM_SUCCESS) {
        CAL_Printf("Failed to start HTTPD\r\n");
    }
    
    ret = sysinfo_init();
    if (ret != WM_SUCCESS) {
        CAL_Printf("Error: psm_cli_init failed\r\n");
    }

    ret = psm_cli_init();
    if (ret != WM_SUCCESS) {
        CAL_Printf("Error: psm_cli_init failed\r\n");
    }

    ret = wlan_cli_init();
    if (ret != WM_SUCCESS)
        CAL_Printf("Error: wlan_cli_init failed\r\n");

}

/*
 * Function: EventUapStarted
 *
 * Event: Micro-AP Started
 * 
 * If we are not provisioned, then start provisioning on
 * the Micro-AP network.
 * 
 * Also, enable WPS.
 * 
 * Since Micro-AP interface is UP, announce mDNS service
 * on the Micro-AP interface.
 */ 
static void EventUapStarted(void *data)
{
    if (!provisioned) {
        CAL_Printf("Starting provisioning\r\n");
        app_provisioning_start(PROVISIONING_WLANNW);
    }
    
    is_connected = 0;
}

/*
 * Function: EventNormalConnecting
 *
 * Connecting to the provisioned Network
 */
static void EventNormalConnecting(void *data)
{
    connecting = 1;
    net_dhcp_hostname_set(dhcp_host_name);
    CAL_Printf("Connecting to provisioned Network\r\n");
}

/*
 * Function: EventNormalConnected
 * 
 * Event: AF_EVT_NORMAL_CONNECTED
 * 
 * Station interface connected to home AP.
 * 
 * Network dependent services can be started here. Note that these
 * services need to be stopped on disconnection and
 * reset-to-provisioning event.
 */ 
static void EventNormalConnected(void *data)
{
    char ip[16];
    u32_t tempIp;
    
    // Stop Micro-AP mode if still ON.
    if (is_uap_started()) {
        app_uap_stop();
    }
    
    app_network_ip_get(ip);
    CAL_Printf("Connected to provisioned network with ip address =%s\r\n", ip);
    inet_aton(ip, &tempIp);
    
    ownIpAddress[0] = (IzotByte)(tempIp);
    ownIpAddress[1] = (IzotByte)(tempIp >> 8);
    ownIpAddress[2] = (IzotByte)(tempIp >> 16);
    ownIpAddress[3] = (IzotByte)(tempIp >> 24);    
    CAL_Printf("Source IP set as %d.%d.%d.%d\r\n", ownIpAddress[0], ownIpAddress[1], ownIpAddress[2], ownIpAddress[3]);
    
    readIupPersistData();
    OsalSleep(10);
    is_connected = 1;
    
    // send announcement
    SendAnnouncement();
        
    //SetLsAddressFromIpAddr();
}

/*
 * Function: EventNormalUserDisconnect
 *
 * Event handler for AF_EVT_NORMAL_DISCONNECTED - Station interface
 * disconnected.
 * Stop the network services which need not be running in disconnected mode.
 */
static void EventNormalUserDisconnect(void *data)
{
    is_connected = 0;
    CAL_Printf("Disconnected\r\n");
}

/*
 * Function: EventNormalLinkLost
 *
 * Network link has been lost.
 */
static void EventNormalLinkLost(void *data)
{
    is_connected = 0;
    CAL_Printf("Link Lost\r\n");
}

/*
 * Function: EventNormalDHCPRenew
 *
 * DHCP has been renewed.
 */
static void EventNormalDHCPRenew(void *data)
{
    CAL_Printf("DHCP renew\r\n");
}

/*
 * Function: EventNormalResetProv
 *
 * Reset to Provisioning
 */
static void EventNormalResetProv(void *data)
{
    // Reset to provisioning
    provisioned = 0;

    if (is_uap_started() == false) {
        app_uap_start_with_dhcp(ssid_uap, UAP_PASSPHRASE);
    } else {
        app_provisioning_start(PROVISIONING_WLANNW);
    }
    
    is_connected = 0;
}

/*
 * Function: EventProvDone
 *
 * Provisioning successful
 */
static void EventProvDone(void *data)
{
    app_provisioning_stop();
    CAL_Printf("Provisioning successful\r\n");
}

/*
 * Function: EventProvClientDone
 *
 * AF_EVT_PROV_CLIENT_DONE Event
 */
static void EventProvClientDone(void *data)
{
    app_uap_stop();
    dhcp_server_stop();
}

void CheckNetworkStatus(void)
{
    int ret;
    current = IzotGetTickCount();
    
    if (current >= previous) {
        if (!is_sta_connected() && is_uap_started()) {
            ret = app_load_configured_network();
			if (!ret) {
				app_sta_start();
            }
        } else {
            if (is_uap_started()) {
                app_uap_stop();
            }
        }
        previous = current + 300000;
    }
}

/*
 * Function: common_event_handler
 *
 * This is the main event handler for this project. The application framework
 * calls this function in response to the various events in the system.
 */
int common_event_handler(int event, void *data)
{
    switch (event) {
    case AF_EVT_WLAN_INIT_DONE:
        EventWlanInitDone(data);
        break;
    case AF_EVT_NORMAL_CONNECTING:
        EventNormalConnecting(data);
        break;
    case AF_EVT_NORMAL_CONNECTED:
        EventNormalConnected(data);
        break;
    case AF_EVT_NORMAL_LINK_LOST:
        EventNormalLinkLost(data);
        break;
    case AF_EVT_NORMAL_USER_DISCONNECT:
        EventNormalUserDisconnect(data);
        break;
    case AF_EVT_NORMAL_DHCP_RENEW:
        EventNormalDHCPRenew(data);
        break;
    case AF_EVT_PROV_DONE:
        EventProvDone(data);
        break;
    case AF_EVT_NORMAL_RESET_PROV:
        EventNormalResetProv(data);
        break;
    case AF_EVT_UAP_STARTED:
        EventUapStarted(data);
        break;
    case AF_EVT_PROV_CLIENT_DONE:
        EventProvClientDone(data);
        break;
    default:
        break;
    }

    return 0;
}

/*
 * Function: InitModules
 *
 * Initialize all required modules
 */
static void InitModules()
{
    int ret;

    // Initialize CLI Command
    ret = cli_init();
    if (ret != WM_SUCCESS) {
        CAL_Printf("Error: cli_init failed\r\n");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    ret = gpio_drv_init();
    if (ret != WM_SUCCESS) {
        CAL_Printf("Error: gpio_drv_init failed\r\n");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    app_sys_register_upgrade_handler();
    app_sys_register_diag_handler();
    
    set_reconnect_iter(5);
}

/*
 * Function: CalStart
 * This function starts the wlan connection for the FreeRtos

 */
int CalStart(void)
{
    int ret = IzotApiNoError;
    
    InitModules();
    
    // Start the application framework
    if (app_framework_start(common_event_handler) != WM_SUCCESS) {
        CAL_Printf("Failed to start application framework\r\n");
        appln_critical_error_handler((void *) -WM_FAIL);
    }
    
    CAL_Printf("Waiting for Board to connect\r\n");
    
    while (!is_connected) {
        OsalSleep(100);
        if (is_uap_started() && connecting) {
            break;
        }
    }
    set_reconnect_iter(30);
    return ret;
}

/*
 * Function: SetCurrentIP
 *
 * This function will send the announcement whenever new ip is
 * assigned to the device
 */
void SetCurrentIP(void)
{
    char ip[16];
    u32_t tempIp;
    
    app_network_ip_get(ip);
    CAL_Printf("Connected to provisioned network with ip address =%s\r\n", ip);
    inet_aton(ip, &tempIp);
    
    ownIpAddress[0] = (IzotByte)(tempIp);
    ownIpAddress[1] = (IzotByte)(tempIp >> 8);
    ownIpAddress[2] = (IzotByte)(tempIp >> 16);
    ownIpAddress[3] = (IzotByte)(tempIp >> 24);
    CAL_Printf("Source IP set as %d.%d.%d.%d\r\n",ownIpAddress[0],
                         ownIpAddress[1],ownIpAddress[2],ownIpAddress[3]);
    CAL_Printf("\r\n");
}

/*
 * Function: InitSocket
 *
 * Open priority and non priority sockets and 
 * also add mac filter for broadcast message
 */
int InitSocket(int port)
{
    struct sockaddr_in     sinme;
    
    // Open UDP socket for queue at start-up
    app_udp_socket = net_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (app_udp_socket < 0) {
        CAL_Printf("Error to create socket due to %d \r\n", app_udp_socket);
        return net_get_sock_error(app_udp_socket);
    }
    
    memset((char *) &sinme, 0, sizeof(sinme));
    
    sinme.sin_family     = PF_INET;
    sinme.sin_port       = htons(port);
    inet_aton(INADDR_ANY, &sinme.sin_addr);
    
    // Set the timeout for the udp receive socket
    int flags = fcntl(app_udp_socket, F_GETFL, 0);
    if (flags < 0) {
        CAL_Printf("fcntl get failed\r\n");
    }
    
    flags = fcntl(app_udp_socket, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0) {
        CAL_Printf("fcntl set failed\r\n");
    }
        
    // bind the socket
    if (bind(app_udp_socket, (struct sockaddr *)&sinme, sizeof(sinme)) == -1) {
        CAL_Printf("ERROR: Failed to Bind\r\n");
        app_udp_socket = -1;
        net_close(app_udp_socket);
        return -1;
    }
    
    return 0;
}

/*
 * Function: RemoveIPMembership
 *
 * Remove membership of given address in multicast group.
 */
void RemoveIPMembership(uint32_t addr)
{
    IzotByte mcast_mac[MLAN_MAC_ADDR_LENGTH];
    
    // Multicast address group structure
    struct ip_mreq group;   
    
    // Remove the 'addr' address from the mac filter
    wifi_get_ipv4_multicast_mac(addr, mcast_mac);
    wifi_remove_mcast_filter(mcast_mac);
    
    // Remove membership of 'addr' address
    group.imr_multiaddr.s_addr = htonl(addr);
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    
    // Set the socket option to remove membership
    if (setsockopt(app_udp_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
    (char *)&group, sizeof(group)) < 0) {
        CAL_Printf("Failed to remove membership of %X\r\n", addr);
        return;
    }
    
    CAL_Printf("Removed Membership of %X \r\n",addr);wmstdio_flush();
}

/*
 * Function: AddIpMembership
 *
 * Add membership of given address in multicast group.
 */
void AddIpMembership(uint32_t addr)
{
    IzotByte mcast_mac[MLAN_MAC_ADDR_LENGTH];
    
    // Multicast address group structure
    struct ip_mreq group;
    
    // Add the 'addr' address from the mac filter
    wifi_get_ipv4_multicast_mac(addr, mcast_mac);
    wifi_add_mcast_filter(mcast_mac);
    
    // Add membership of 'addr' address
    memset((char *)&group, 0, sizeof(struct ip_mreq));
    group.imr_multiaddr.s_addr = htonl(addr);
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    
    // Set the socket option to add the membership
    if (setsockopt(app_udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
    (char *)&group, sizeof(group)) < 0) {
        CAL_Printf("Failed to add membership for %X\r\n", addr);
        return;
    }
    
    CAL_Printf("Added Membership of %X \r\n", addr);
}

/*
 * Function: CalSend
 *
 * This API is used to send data on udp socket
 */
void CalSend(uint32_t port, IzotByte* addr, IzotByte* pData, 
uint16_t dataLength)
{
    IzotByte            loopch = 0;
    int                 sock = -1;
    int                 reuse = 1;
    struct sockaddr_in  to;
    int                 toLen = sizeof(to);
    struct in_addr      localInterface;
    
    to.sin_family = AF_INET;
    to.sin_port = htons(port);
    to.sin_addr.s_addr = (u32_t)(addr[3]) << 24 | (u32_t)(addr[2]) << 16 
                                             | (u32_t)(addr[1]) << 8 | addr[0];
    
    // socket for sending data
    sock = net_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
    (char *)&reuse, sizeof(reuse)) < 0) {
        CAL_Printf("Failed to SO_REUSEADDR\r\n");
        net_close(sock);
        return;
    }
    
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, 
    &loopch, sizeof(loopch)) < 0) {
        CAL_Printf("ERROR while setting IP_MULTICAST_LOOP\r\n");
        net_close(sock);
        return;
    }
    
    memset((char *) &localInterface, 0, sizeof(localInterface));
    localInterface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,(char *)&localInterface, 
    sizeof(localInterface)) < 0) {
        CAL_Printf("Failed to IP_MULTICAST_IF\r\n");
        net_close(sock);
        return;
    }
#ifdef CAL_DEBUG
    int len;
    len = 
#endif
    sendto(sock, pData, dataLength, 0, (struct sockaddr *)&to, toLen);
#ifdef CAL_DEBUG
    if (len > 0) {
        CAL_Printf("Dst IP: %s\r\n\n", inet_ntoa(to.sin_addr.s_addr));

        IzotByte i;
        CAL_Printf("LSUDP: %d bytes sent: ", len);
        for(i = 0; i < len; i++) {
            CAL_Printf("%X ", pData[i]);
        }
        CAL_Printf("\r\n");
        CAL_Printf("Dst IP: %s\r\n\n", inet_ntoa(to.sin_addr.s_addr));
        wmstdio_flush();
    } else {
        CAL_Printf("%d bytes sent\r\n", len);
    }
#endif
    
    net_close(sock);
}

/*
 * Function: CalReceive
 *
 * This API is used to receive data on udp socket.
 * Keep the udp socket non blockable.
 */
int CalReceive(IzotByte* pData, IzotByte* pSourceAddr)
{
    struct sockaddr_in  from;
    int                 fromLen = sizeof(from);
    uint32_t            SrcIP;
    int                 dataLength;
    
    dataLength = recvfrom(app_udp_socket, pData, 
    DecodeBufferSize(CAL_RECEIVE_BUF_SIZE), 0, (struct sockaddr *)&from, 
    (socklen_t *)&fromLen);
    
    // Get the data if dataLength is bigger than zero
    if (dataLength > 0) {
#ifdef CAL_DEBUG
        CAL_Printf("Src IP: %s\r\n", inet_ntoa(from.sin_addr.s_addr));
        
        int i;
        CAL_Printf("LSUDP: %d bytes recv: ",dataLength);
        for(i = 0; i < dataLength; i++) {
            CAL_Printf("%02X ", pData[i]);
        }
        CAL_Printf("\r\n");
        wmstdio_flush();
#endif
        // Get the ip and port from which data is received
        SrcIP = ntohl(from.sin_addr.s_addr);

        pSourceAddr[0] = (IzotByte)((SrcIP & 0xFF000000) >> 24);
        pSourceAddr[1] = (IzotByte)((SrcIP & 0x00FF0000) >> 16);
        pSourceAddr[2] = (IzotByte)((SrcIP & 0x0000FF00) >> 8);
        pSourceAddr[3] = (IzotByte)(SrcIP & 0x000000FF);
    }
    
    return dataLength;
}
