/*
 * IzotCal.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   IP Connectivity Abstraction Layer
 * Purpose: Defines portable functions and types for communicating
 *          with IP sockets.
 * Notes:   IP sockets are required for LON/IP and are not required
 * 			for native LON.
 */

#include "abstraction/IzotCal.h"
#include "lcs/lcs_api.h"

#if PROCESSOR_IS(MC200)
#include <app_framework.h>
#include <appln_cb.h>
#include <wm_net.h>
#include <wm_os.h>
#include <board.h>
#include <mdev_gpio.h>
#include <mdev_pinmux.h>
#endif

#if PROTOCOL_IS(LON_IP)
#include "ls_udp/IPv4ToLsUdp.h"
#endif

#if LINK_IS(WIFI)
    #define MAX_SSID_LEN      15
    #define MAX_HOST_NAME_LEN 10
    #define MAC_ID_LEN        6
    #define UAP_PASSPHRASE    "TBD"
#endif  // LINK_IS(WIFI)

#if PROCESSOR_IS(MC200)
    #define FTFS_API_VERSION  100
    #define FTFS_PART_NAME    "ftfs"
#endif  // PLATFORM_IS(FRTOS_ARM_EABI)

/*****************************************************************
 * Section: Globals
 *****************************************************************/
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
IzotByte ownIpAddress[IPV4_ADDRESS_LEN]; 
                // Buffer to store the IP address
IzotBool is_connected;
                // Flag to report IP link connectivity

static LonTimer linkCheckTimer;
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)

#if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
static int        app_udp_socket = -1;
static int        provisioned;
static struct fs *fs;
static char       ssid_uap[MAX_SSID_LEN];
static char       dhcp_host_name[MAX_HOST_NAME_LEN];
static uint8_t    connecting = 0;
#endif

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/
 #if PROCESSOR_IS(MC200)
/*
 * Handles a critical error for the MC200.
 * Parameters:
 *   data: Pointer to data (not used) 
 * Returns:
 *   None
 * Notes:
 *   Stalls and does nothing when a critical error occurs.
 */
void appln_critical_error_handler(void *data)
{
    while (1)
        ;
    // do nothing -- stall
}
#endif  // PROCESSOR_IS(MC200)


#if LINK_IS(WIFI)
/* 
 * Handles a Wi-Fi initialization completion event.
 * Parameters:
 *   data: Pointer to data (provisioning status)
 * Returns:
 *  None
 * Notes:
 *   Handle the AF_EVT_WLAN_INIT_DONE event that occurs for Wi-Fi.
 *   When Wi-Fi is started, the application framework looks to
 *   see whether a home network information is configured
 *   and stored in PSM (persistent storage module).
 * 
 *   The data field returns whether a home network is provisioned
 *   or not, which is used to determine what network interfaces
 *   to start (station, micro-ap, or both).
 * 
 *   If provisioned, the station interface of the device is
 *   connected to the configured network.
 * 
 *   Else, the micro-AP network is configured.
 * 
 *   (If desired, the micro-AP network can also be started
 *   along with the station interface.)
 * 
 *   Starts all the services which don't need to be
 *   restarted between provisioned and non-provisioned mode
 *   or between connected and disconnected state.
 * 
 *   Accordingly:
 *     -- Starts mDNS and advertize services
 *     -- Starts HTTP Server
 *     -- Registers WSGI handlers for HTTP server
 */ 
static void EventWlanInitDone(void *data)
{
    int ret;
    unsigned char mac[MAC_ID_LEN];
    
    // Receive provisioning status from data
    provisioned = (int)data;

    CAL_Printf("AF_EVT_WLAN_INIT_DONE provisioned=%d\r\n", provisioned);

    // Limit the number of Wi-Fi Channels to 11
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

    // Get the MAC address of the Wi-Fi interface
    if (!IZOT_SUCCESS(HalGetMacAddress(mac))) {
        CAL_Printf("Failed to to get MAC address\r\n");
        return;
    }

    // Copy only last two bytes of MAC address to the SSID
    snprintf(ssid_uap, MAX_SSID_LEN, "CPM-4200-%02X%02X", mac[4], mac[5]);
    snprintf(dhcp_host_name, MAX_HOST_NAME_LEN, "IZOT-%02X%02X", mac[4], mac[5]);
    CAL_Printf("SSID: %s\r\n", ssid_uap);
    CAL_Printf("DHCP host Name: %s\r\n", dhcp_host_name);
    
    if (provisioned) {
        app_sta_start();
    } else {
        app_uap_start_with_dhcp(ssid_uap, UAP_PASSPHRASE);
    }

    // Start the HTTP server and enable webapp in the flash FTFS partition
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
 * Handles a micro-AP start-up event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 * Notes:
 *   If not provisioned, start provisioning on on the micro-AP network,
 *   enable WPS, and announce mDNS service on the micro-AP interface.
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
 * Handles a normal provisioned network connection start-up event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 */
static void EventNormalConnecting(void *data)
{
    connecting = 1;
    net_dhcp_hostname_set(dhcp_host_name);
    CAL_Printf("Connecting to provisioned Network\r\n");
}
#endif  // LINK_IS(WIFI)


/*
 * Handles a normal provisioned network connection completion event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 * Notes:
 *   Handle the AF_EVT_NORMAL_CONNECTED event that occurs for Wi-Fi
 *   when the station interface is connected to the home access point.
 *   Network dependent services can be started here. These services
 *   can be stopped on disconnection and reset-to-provisioning events.
 */ 
static void EventNormalConnected(void *data)
{
#if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
    // Stop Micro-AP mode if still ON
    if (is_uap_started()) {
        app_uap_stop();
    }
    app_network_ip_get(ip);
#endif  // LINK_IS(WIFI) && PROCESSOR_IS(MC200)

#if !IUP_IS(NO_IUP)
    readIupPersistData();
#endif  // !IUP_IS(NO_IUP)

#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    is_connected = 1;
    CAL_Printf("Connected\r\n");

    // Set current LON/IP address and send announcement
    SendAnnouncement();

    // Set LON address from LON/IP address        
    SetLsAddressFromIpAddr();
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)
}


/*
 * Handles a normal provisioned network disconnection event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 * Notes:
 *   Handle the AF_EVT_NORMAL_DISCONNECTED event that occurs for Wi-Fi
 *   when the station interface is diconnected from the home access point.
 *   Network dependent services not required while disconnected can be 
 *   stopped here.
 */ 
static void EventNormalUserDisconnect(void *data)
{
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    is_connected = 0;
    CAL_Printf("Disconnected\r\n");
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)
}


/*
 * Handles a network link lost event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 * Notes:
 *   Handle a connection lost event that occurs for Wi-Fi when the
 *   station interface link to the home access point is lost.
 */ 
static void EventNormalLinkLost(void *data)
{
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    is_connected = 0;
    CAL_Printf("Link Lost\r\n");
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)
}


/*
 * Handles a DHCP address assignment event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 * Notes:
 *   Handle a possible IP address change after the DHCP-assigned
 *   address is renewed.
 */ 
static void EventNormalDHCPRenew(void *data)
{
    CAL_Printf("DHCP renew\r\n");
}


#if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
/*
 * Handles a Wi-Fi link provisioning reset event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 */
static void EventNormalResetProv(void *data)
{
    provisioned = 0;
    if (is_uap_started() == false) {
        app_uap_start_with_dhcp(ssid_uap, UAP_PASSPHRASE);
    } else {
        app_provisioning_start(PROVISIONING_WLANNW);
    }
    is_connected = 0;
}


/*
 * Handles a Wi-Fi link provisioning reset completion event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 */
static void EventProvDone(void *data)
{
    app_provisioning_stop();
    CAL_Printf("Provisioning successful\r\n");
}


/*
 * Handles a Wi-Fi client link completion event.
 * Parameters:
 *   data: Pointer to data (not used)
 * Returns:
 *   None
 */
static void EventProvClientDone(void *data)
{
    app_uap_stop();
    dhcp_server_stop();
}

#endif  // LINK_IS(WIFI) && PROCESSOR_IS(MC200)


/*
 * Handles all network events.
 * Parameters:
 *   event: Network event
 *   data: Pointer to data
 * Returns:
 *   0
 * Notes:
 *   The application framework calls this function in response to
 *   various events in the system.
 */
int common_event_handler(int event, void *data)
{
#if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
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

#endif  // LINK_IS(WIFI) && PROCESSOR_IS(MC200)

    return 0;
}

/*
 * Initializes all required modules.
 * Parameters:
 *  None
 * Returns:
 *  None
 */
static void InitModules()
{
#if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
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
#endif  // LINK_IS(WIFI) && PROCESSOR_IS(MC200)
}

/*
 * Starts the IP link.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code
 *   on failure.
 */
LonStatusCode CalStart(void)
{
    LonStatusCode ret = LonStatusNoError;
    
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    InitModules();
    SetLonRepeatTimer(&linkCheckTimer, 1, LINK_CHECK_INTERVAL);
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)

#if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
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
#endif  // LINK_IS(WIFI) && PROCESSOR_IS(MC200)

#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    EventNormalConnected(NULL);
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)

    return ret;
}

/*
 * Sets the current IP address.
 * Parameters:
 *   None
 * Returns:
 *   TRUE if the IP address changed since the last call
 */
IzotBool SetCurrentIP(void)
{
#if PROTOCOL_IS(LON_IP)
    IzotUbits32 currentIpAddress = 0;
    static IzotUbits32 lastIpAddress = 0;
    IzotBool ipAddressChanged = FALSE;

#if PLATFORM_IS(FRTOS_ARM_EABI)
    char ip[16];
    
    app_network_ip_get(ip);
    CAL_Printf("Connected to provisioned network with IP address =%s\r\n", ip);
    inet_aton(ip, &currentIpAddress);
#else   // !PLATFORM_IS(FRTOS_ARM_EABI)
    #pragma message("Implement code to get the current IP address")
    // currentIpAddress = <Get current IP address>;
    // currentIpAddress = 0xC0A80101;  // 192.168.1.1 for testing
#endif  // !PLATFORM_IS(FRTOS_ARM_EABI)

    ipAddressChanged = currentIpAddress != lastIpAddress;

    ownIpAddress[0] = (IzotByte)(currentIpAddress);
    ownIpAddress[1] = (IzotByte)(currentIpAddress >> 8);
    ownIpAddress[2] = (IzotByte)(currentIpAddress >> 16);
    ownIpAddress[3] = (IzotByte)(currentIpAddress >> 24);

    if (ipAddressChanged) {
        CAL_Printf("Source IP set to %d.%d.%d.%d\r\n",ownIpAddress[0],
                             ownIpAddress[1],ownIpAddress[2],ownIpAddress[3]);
    }
    return ipAddressChanged;
#else   // !PROTOCOL_IS(LON_IP)
    return FALSE;
#endif  // !PROTOCOL_IS(LON_IP)
}

/*
 * Initializes IP sockets and adds a MAC filter for broadcast messages.
 * Parameters:
 *   port: UDP port to open
 * Returns:
 *   0 on success, or a negative error code on failure
 * Notes:
 *   Priority and non-priority sockets are initialized to the same port.
 *   Implementation of this function is required for LON/IP support.
 */
int InitSocket(int port)
{
#if PROTOCOL_IS(LON_IP)
#if PLATFORM_IS(FRTOS_ARM_EABI)
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
#else   // !PLATFORM_IS(FRTOS_ARM_EABI)
    #pragma message("Implement code to open priority and non priority sockets and add MAC filter for broadcast messages")
#endif  // !PLATFORM_IS(FRTOS_ARM_EABI)
#endif  // PROTOCOL_IS(LON_IP)

    return 0;
}

/*
 * Removes membership of the specified address from a multicast group.
 * Parameters:
 *   addr: Address to remove
 * Returns:
 *   None
 * Notes:
 *   Implementation of this function is required for LON/IP support.
 */
void RemoveIPMembership(uint32_t addr)
{
#if PROTOCOL_IS(LON_IP)
#if PLATFORM_IS(FRTOS_ARM_EABI)
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
#else   // !PLATFORM_IS(FRTOS_ARM_EABI)
    #pragma message("Implement code to remove address membership from a multicast group")
#endif  // !PLATFORM_IS(FRTOS_ARM_EABI)
#endif  // PROTOCOL_IS(LON_IP)
}

/*
 * Adds membership for the specified address to a multicast group.
 * Parameters:
 *   addr: Address to add
 * Returns:
 *   None
 * Notes:
 *   Implementation of this function is required for LON/IP support.
 */
void AddIpMembership(uint32_t addr)
{
#if PROTOCOL_IS(LON_IP)
#if PLATFORM_IS(FRTOS_ARM_EABI)
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
#else   // !PLATFORM_IS(FRTOS_ARM_EABI)
    #pragma message("Implement code to add address membership to a multicast group")
#endif  // !PLATFORM_IS(FRTOS_ARM_EABI)
#endif  // PROTOCOL_IS(LON_IP)
}

/*
 * Sends data on a UDP socket.
 * Parameters:
 *   port: UDP port to send to
 *   addr: IP address to send to
 *   pData: Pointer to data to send
 *   dataLength: Length of data to send
 * Returns:
 *   None
 */
void CalSend(uint32_t port, IzotByte* addr, IzotByte* pData, 
                uint16_t dataLength)
{
#if PROTOCOL_IS(LON_IP)
#if PLATFORM_IS(FRTOS_ARM_EABI)
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
#else   // !PLATFORM_IS(FRTOS_ARM_EABI)
    #pragma message("Implement code to send a UDP packet")
#endif  // !PLATFORM_IS(FRTOS_ARM_EABI)
#endif  // PROTOCOL_IS(LON_IP)
}

/*
 * Receives data from a UDP socket.  
 * Parameters:
 *   pData: Pointer to buffer to receive data
 *   pSourceAddr: Pointer to buffer to receive source IP address
 * Returns:
 *   Number of bytes received, or a negative error code on failure
 * Notes:
 *   Keep the DUP socket non-blockable.  Implementation of this function is 
 *   required for LON/IP support.
 */
int CalReceive(IzotByte* pData, IzotByte* pSourceAddr)
{
    int dataLength = 0;
#if PROTOCOL_IS(LON_IP)
#if PLATFORM_IS(FRTOS_ARM_EABI)
    struct sockaddr_in  from;
    int                 fromLen = sizeof(from);
    uint32_t            SrcIP;
    
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
#else   // !PLATFORM_IS(FRTOS_ARM_EABI)
    #pragma message("Implement code to receive data on a UDP socket")
#endif  // !PLATFORM_IS(FRTOS_ARM_EABI)
#endif  // PROTOCOL_IS(LON_IP)
    return dataLength;
}

/*
 * Checks for a change of status for the data link. 
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes: 
 *   Handle an unexpected loss or recovery of a data link.
 */
void CheckNetworkStatus(void)
{
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    if (LonTimerExpired(&linkCheckTimer)) {
        #if LINK_IS(WIFI) && PROCESSOR_IS(MC200)
            if (!is_sta_connected() && is_uap_started()) {
                int ret = app_load_configured_network();
                if (!ret) {
                    app_sta_start();
                }
            } else {
                if (is_uap_started()) {
                    app_uap_stop();
                }
            }
        #endif  // LINK_IS(WIFI) && PROCESSOR_IS(MC200)
        #if PROTOCOL_IS(LON_IP)
            #pragma message("Implement code to test for an IP link transition from not connected to connected")
            // if (!is_connected && <link is connected>) }
                // Link has changed from not connected to connected; handle the change
                // EventNormalConnected(NULL);
            // }
            (void) SetCurrentIP();
        #endif  // LINK_IS(LON_IP)
    }
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)
}