/*
 * IzotApi.c
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack API
 * Purpose: Provides high-level API functions for the LON Stack.
 */

#include "izot/IzotApi.h"
#include "lon_udp/ipv4_to_lon_udp.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*****************************************************************
 * Section: Globals
 *****************************************************************/
IzotByte DataPointCount;
IzotByte AliasTableCount;
uint16_t BindableMTagCount;

IzotGetCurrentDatapointSizeFunction izot_get_dp_size_handler = NULL;
IzotResetFunction izot_reset_handler = NULL;
IzotWinkFunction izot_wink_handler = NULL;
IzotDatapointUpdateOccurredFunction izot_dp_update_occurred_handler = NULL;
IzotDatapointUpdateCompletedFunction izot_dp_update_completed_handler = NULL;
IzotOnlineFunction izot_online_handler = NULL;
IzotOfflineFunction izot_offline_handler = NULL;
IzotMsgCompletedFunction izot_msg_completed_handler = NULL;
IzotMsgArrivedFunction izot_msg_arrived_handler = NULL;
IzotResponseArrivedFunction izot_response_arrived_handler = NULL;
IzotMemoryReadFunction izot_memory_read_handler = NULL;
IzotMemoryWriteFunction izot_memory_write_handler = NULL;
IzotServiceLedStatusFunction izot_service_led_handler = NULL;

IzotPersistentSegOpenForReadFunction izot_open_for_read_handler = IzotFlashSegOpenForRead;
IzotPersistentSegOpenForWriteFunction izot_open_for_write_handler = IzotFlashSegOpenForWrite;
IzotPersistentSegCloseFunction izot_close_handler = IzotFlashSegClose;
IzotPersistentSegDeleteFunction izot_delete_handler = NULL;
IzotPersistentSegReadFunction izot_read_handler = IzotFlashSegRead;
IzotPersistentSegWriteFunction izot_write_handler = IzotFlashSegWrite;
IzotPersistentSegIsInTransactionFunction izot_is_in_tx_handler = IzotFlashSegIsInTransaction;
IzotPersistentSegEnterTransactionFunction izot_enter_tx_handler = IzotFlashSegEnterTransaction;
IzotPersistentSegExitTransactionFunction izot_exit_tx_handler = IzotFlashSegExitTransaction;

IzotPersistentSegGetAppSizeFunction izot_get_app_seg_size_handler = NULL;
IzotPersistentSegDeserializeFunction izot_deserialize_handler = NULL;
IzotPersistentSegSerializeFunction izot_serialize_handler = NULL;
IzotFilterMsgArrivedFunction izot_filter_msg_arrived = NULL;
IzotFilterResponseArrivedFunction izot_filter_response_arrived = NULL;
IzotFilterMsgCompletedFunction izot_filter_msg_completed = NULL;
IzotisiTickFunction izot_isi_tick_handler = NULL;
LonTimer isi_tick_timer;

static uint32_t SiDataLength;

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
extern void Encrypt(IzotByte randIn[], APDU *apduIn, IzotUbits16 apduSizeIn, IzotByte *pKey, 
        IzotByte encryptValueOut[], IzotByte isOma, OmaAddress* pOmaDest);

/*****************************************************************
 * Section: Core LON Stack API Function Definitions
 *****************************************************************/
 /*
  * Sets the Length of SI data from III generated Length
  * Parameters:
  *   len: length of the SI data
  * Returns:
  *   None
  */
static void SetSiDataLength(uint32_t len)
{
    SiDataLength = len;
}

 /*
  * Gets the Length of SI data from III generated Length
  * Parameters:
  *   None
  * Returns:
  *   Length of the SI data
 */
uint32_t GetSiDataLength(void)
{
    return SiDataLength;
}

/*
 * Sleeps for a specified number of ticks.
 * Parameters:
 *   ticks: The number of ticks to sleep 
 * Returns:
 *   None
 * Notes:
 *   Suspend the task for the specified number of clock ticks.
 */
IZOT_EXTERNAL_FN void IzotSleep(unsigned int ticks)
{
    OsalSleep(ticks);
}

/*
 * Handles the IzotServiceLedStatus event.
 * Parameters:
 *   state: The current service LED state
 *   physicalState: The current physical state of the service LED
 * Returns:
 *   None
 * Notes:
 *   The IzotServiceLedStatus event occurs when the service pin state
 *   changes.
 */
void IzotServiceLedStatus(IzotServiceLedState state, IzotServiceLedPhysicalState physicalState)
{
    if(izot_service_led_handler) {
        izot_service_led_handler(state, physicalState);
    }
}

/*
 * Processes asynchronous LON Stack events.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success, or an <LonStatusCode> error code on failure.
 * Notes:
 *   Call this function periodically after calling IzotStartStack(),
 *   IzotRegisterStaticDatapoint() once per static NV, and
 *   IzotRegisterMemoryWindow().  Call at least once every 10 ms or at the
 *   following interval in milliseconds, whichever is less:
 *     Interval = ((InputBufferCount - 1) * 1000) / MaxPacketRate
 *   where MaxPacketRate is the maximum number of packets per second arriving
 *   for the device and InputBufferCount is the number of input buffers
 *   defined for the application.  
 *
 *   This function processes any events that have been posted by the LON Stack.
 *   Typically this function is called in response to IzotEventReady(), but
 *   must *not* be called directly from that event handler. The
 *   IzotEventReady() event handler typically sets an operating system event to
 *   schedule the main application task to call the IzotEventPump() function.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotEventPump(void)
{
    LonStatusCode status = LonStatusNoError;

#if LINK_IS(WIFI) || LINK_IS(ETHERNET)
    CheckNetworkStatus();

	if(is_connected) {    
		LCS_Service();
		IzotPersistentMemCommitCheck();
	}
#else
	LCS_Service();
	IzotPersistentMemCommitCheck();
#endif

    IzotSleep(1);
    if (gp->serviceLedState != SERVICE_BLINKING && (((gp->serviceLedState != gp->prevServiceLedState)
            && ((gp->serviceLedPhysical != gp->preServiceLedPhysical))))) {
        IzotServiceLedStatus(gp->serviceLedState, gp->serviceLedPhysical); // expose the Connect button state to the callback function
        
        gp->prevServiceLedState = gp->serviceLedState;
        gp->preServiceLedPhysical = gp->serviceLedPhysical;
    }
	
	if (IsPhysicalResetRequested() && !IzotPersistentSegCommitScheduled()) {
        HalReboot();
    }
    
    if (LonTimerExpired(&isi_tick_timer)) {
        if (izot_isi_tick_handler) {
            izot_isi_tick_handler();
        }
    }

    return status;
}

/*
 * Gets the registered device unique ID (Neuron or MAC ID).
 * Parameters:
 *   uId: Pointer to the the unique ID
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   The unique ID is a unique 48-bit identifier for a LON device.  
 *   The unique ID may be a LON Neuron ID or an IEEE MAC ID.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotGetUniqueId(IzotUniqueId* const uId)
{
    LonStatusCode status = LonStatusNoError;
    unsigned char mac[6] = {0};

    if (!LON_SUCCESS(HalGetMacAddress(mac))) {
        status = LonStatusDeviceUniqeIdNotAvailable;
    } else {
        memcpy(uId, &mac[0], 6);
    }
    return status;
}

/*
 * Returns the LON Stack version number.
 * Parameters:
 *   pMajorVersion: Pointer to receive the LON Stack major version number
 *   pMinorVersion:  Pointer to receive the LON Stack minor version number
 *   pBuildNumber: Pointer to receive the LON Stack build number
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   This function provides the version of the LON Stack.  This function
 *   can be called at any time.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotGetVersion(unsigned* const pMajorVersion, unsigned* const pMinorVersion,
        unsigned* const pBuildNumber)
{
    LonStatusCode status = LonStatusNoError;
    *pMajorVersion = FIRMWARE_VERSION;
    *pMinorVersion = FIRMWARE_MINOR_VERSION;
    *pBuildNumber = FIRMWARE_BUILD;
    return status;
}

/*
 * Polls a bound, polling, input datapoint. See <IzotPoll> for an alternative.
 * Parameters:
 *   index: Index of the input datapoint
 * Returns:
 *    LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   Call this function to poll an input datapoint. Polling an input datapoint
 *   causes the device to solicit the current value of all output datapoints
 *   that are bound to this one.
 *
 *   The function returns LonStatusNoError if the API has successfully queued
 *   the request. The successful completion of this function does not indicate
 *   the successful arrival of the requested values. The values received in
 *   response to this poll are reported by one or more of calls to the
 *   <IzotDatapointUpdateOccurred> event handler.
 *
 *   This function operates only on input datapoints that have been declared
 *   with the *polled* attribute. Only output datapoints that are bound to the
 *   input datapoint will be received.
 *
 *   It is *not* an error to poll an unbound polling input datapoint.  If this
 *   is done, the application will not receive any
 *   <IzotDatapointUpdateOccurred> events, but will receive a
 *   <IzotDatapointUpdateCompleted> event with the success parameter set to TRUE.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPollByIndex(signed index)
{
    PollNV(index);
    return LonStatusNoError;
}

/*
 * Gets the value of a datapoint by index
 * Parameters:
 *   index: Index of the datapoint
 * Returns:
 *   Address of the datapoint value
 */
IZOT_EXTERNAL_FN volatile void* IzotGetDatapointValue(const unsigned index)
{
    return NV_ADDRESS(index);
}

/*
 * Propagates the value of a bound output datapoint to the network.
 * Parameters:
 *   index: Index of the datapoint
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   See <IzotPropagate> for an alternative.
 * 
 *   It is not an error to propagate an unbound non-polled output.  If this
 *   is done, the LON Stack will not send any updates to the network, but will
 *   generate a <IzotDatapointUpdateCompleted> event with the success
 *   parameter set to TRUE.
 *
 *   If IzotPropagateByIndex() returns LonStatusNoError, the
 *   <IzotDatapointUpdateCompleted> event will be triggered when the 
 *   datapoint update has successfully completed or failed.
 *
 *   If IzotPropagateByIndex() is called multiple times before the datapoint
 *   is sent, the behavior is dependent on whether the datapoint has the
 *   synchronous attribute:
 *
 *   - If the datapoint is declared with the *sync* attribute,
 *     the datapoint will be sent on the network each time
 *     IzotPropagateByIndex() is called (subject to application buffer limits).
 *     The value sent will be the value of the datapoint at the time
 *     of the call to IzotPropagateByIndex(). IzotDatapointUpdateCompleted() 
 *     will be called each time as well.
 *
 *   - If the datapoint is *not* declared with the *sync* attribute,
 *     only the latest value of the datapoint will be sent
 *     out onto the network, and IzotDatapointUpdateCompleted() will be
 *     called only once. If there are no application buffers available, the
 *     datapoint will be propagated at a later time, when one becomes
 *     available.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPropagateByIndex(signed index)
{
    PropagateNV(index);
    return LonStatusNoError;
}

/*
 * Propagates a Service message.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   Use this function to propagate a Service message to the network.
 *   The function will fail if the device is not yet fully initialized.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotSendServiceMessage(void)
{
    LonStatusCode status;
    status = ManualServiceRequestMessage();
    return status;
}

/*
 * Sends an application message (not a datapoint message).
 * Parameters:
 *   tag: Message tag for this message
 *   priority: Priority attribute of the message
 *   serviceType: Service type for use with this message
 *   authenticated: TRUE to use authenticated service
 *   pDestAddr: Pointer to destination address
 *   code: Message code
 *   pData: Message data, may be NULL if length is zero
 *   length: Number of valid bytes available through pData
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   This function is called to send an application message. For application
 *   messages, the message code must be in the range of 0x00..0x2f.  Codes in
 *   the 0x30..0x3f range are reserved for protocols such as file transfer.
 *
 *   If the tag field specifies one of the bindable messages tags
 *   (tag < # bindable message tags), the pDestAddr is ignored (and can be
 *   NULL) because the message is sent using implicit addressing. Otherwise,
 *   explicit addressing is used, and pDestAddr must be provided.
 *
 *   A successful return from this function indicates only that the message has
 *   been queued to be sent.  If this function returns success, the LON Stack
 *   will call IzotMsgCompleted() with an indication of the transmission success.
 *
 *   If the message is a request, LON Stack calls the IzotResponseArrived()
 *   event handlers when the corresponding responses arrive.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotSendMsg(
        const unsigned tag, const IzotBool priority,
        const IzotServiceType serviceType,
        const IzotBool authenticated,
        const IzotSendAddress* const pDestAddr,
        const IzotByte code,
        const IzotByte* const pData, const unsigned length)
{
    gp->msgOut.priorityOn     = priority;
    gp->msgOut.tag            = (MsgTag) tag;
    gp->msgOut.len            = length;
    gp->msgOut.code           = code;
    if(length > 255) {
        MsgCompletes(LonStatusInvalidMessageLength, gp->msgOut.tag);
        return LonStatusInvalidMessageLength;
    }
    memcpy(gp->msgOut.data, (void *) pData, length);
    gp->msgOut.authenticated  = authenticated;
    gp->msgOut.service        = (IzotServiceType) serviceType;
    memcpy(&gp->msgOut.addr, (IzotSendAddress *) pDestAddr, sizeof(*pDestAddr));
    MsgSend();
    return LonStatusNoError;
}

/*
 * Sends a response.
 * Parameters:
 *   correlator: Message correlator, received from IzotMsgArrived()
 *   code: Response message code
 *   pData: Pointer to response data, may be NULL if length is zero
 *   length: Number of valid response data bytes in pData
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   This function is called to send an application response.  The correlator
 *   is passed in to IzotMsgArrived() and must be copied and saved if the 
 *   response is to be sent after returning from that routine.  A response code 
 *   for an application message must be in the 0x00..0x2f range.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotSendResponse(
        const IzotCorrelator correlator, const IzotByte code,
        const IzotByte* const pData, const unsigned length)
{
    memcpy(&gp->respOut.reqId, correlator, 2); 
    gp->respOut.nullResponse = length ? 0 : 1;
    gp->respOut.code = code;
    gp->respOut.len = length;
    memcpy(gp->respOut.data, (void *) pData, length);
    RespSend();
    return LonStatusNoError;
}

/*
 * Releases a request correlator without sending a response.
 * Parameters:
 *   correlator: The correlator, obtained from IzotMsgArrived()
 * Returns:
 *   LonStatusNoError on success, or <LonStatusCode> error code on failure.
 * Notes:
 *   This function is called to release a correlator obtained from
 *   IzotMsgArrived() without sending a response.  The application must either
 *   send a response to every message with a service type of request, or release
 *   the correlator, but not both.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotReleaseCorrelator(const IzotCorrelator correlator)
{
    return LonStatusInvalidOperation;
}

/*****************************************************************
 * Section: Extended LON Stack API Function Definitions
 *****************************************************************/
/*
 * This section details extended LON Stack API functions consisting of
 * query functions and update functions. These functions are not required
 * for typical LON Stack applications.
 */

/*
 * Requests local status and statistics.
 * Parameters:
 *   pStatus: Pointer to an <IzotStatus> structure
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to obtain the local status and statistics of the LON Stack
 *   device. The status will be stored in the <IzotStatus> structure provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryStatus(IzotStatus* const pStatus)
{
    IZOT_SET_UNSIGNED_WORD_FROM_BYTES(pStatus->TransmitErrors, nmp->stats.stats[0], nmp->stats.stats[1]);
    IZOT_SET_UNSIGNED_WORD_FROM_BYTES(pStatus->TransactionTimeouts, nmp->stats.stats[2], nmp->stats.stats[3]);
    IZOT_SET_UNSIGNED_WORD_FROM_BYTES(pStatus->ReceiveTransactionsFull, nmp->stats.stats[4], nmp->stats.stats[5]);
    IZOT_SET_UNSIGNED_WORD_FROM_BYTES(pStatus->LostMessages, nmp->stats.stats[6], nmp->stats.stats[7]);
    IZOT_SET_UNSIGNED_WORD_FROM_BYTES(pStatus->MissedMessages, nmp->stats.stats[8], nmp->stats.stats[9]);
    pStatus->ResetCause = nmp->resetCause;
    if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotConfigOnLine && 
            gp->appPgmMode == OFF_LINE) {
        pStatus->NodeState = IzotSoftOffLine;
    } else {
        pStatus->NodeState = IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE);
    }
    
    pStatus->VersionNumber  = FIRMWARE_VERSION;
    pStatus->ErrorLog       = eep->errorLog;
    pStatus->ArchitectureNumber = ARCHITECTURE_NUMBER;
    IZOT_SET_UNSIGNED_WORD(pStatus->LostEvents, 12);

    return LonStatusNoError;
}

/*
 * Clears the status statistics on the IzoT device.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function can be used to clear the LON Stack device status and statistics
 *   records.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotClearStatus(void)
{
    memset(&nmp->stats, 0, sizeof(nmp->stats));
    nmp->resetCause = IzotResetCleared;
    eep->errorLog = LonStatusNoError;
    LCS_WriteNvm();

    return LonStatusNoError;
}

/*
 * Requests a copy of local configuration data.
 * Parameters:
 *   pConfig: Pointer to a <IzotConfigData> structure
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to request a copy of device's configuration data.
 *   The configuration is stored in the provided <IzotConfigData> structure.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryConfigData(IzotConfigData* const pConfig)
{
    memcpy(pConfig, &eep->configData, sizeof(IzotConfigData));
    return LonStatusNoError;
}

/*
 * Updates the configuration data on the IzoT device.
 * Parameters:
 *   pConfig: Pointer to <IzotConfigData> configuration data
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to update the LON Stack device's configuration data
 *   based on the configuration stored in the <IzotConfigData> structure.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateConfigData(const IzotConfigData* const pConfig)
{
    memcpy(&eep->configData, pConfig, sizeof(IzotConfigData));
    RecomputeChecksum();
    LCS_WriteNvm();
    return LonStatusNoError;
}

/*
 * Sets the device's mode and/or state.
 * Parameters:
 *   mode: Mode of the IzoT device, see <IzotNodeMode>
 *   state: State of the IzoT device, see <IzotNodeState>
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Use this function to set the LON Stack device's mode and state.
 *   If the mode parameter is *IzotChangeState*, the state parameter may be
 *   set to one of *IzotApplicationUnconfig*, *IzotNoApplicationUnconfig*,
 *   *IzotConfigOffLine* or *IzotConfigOnLine*.  Otherwise the state parameter
 *   should be *IzotStateInvalid* (0).  While the <IzotNodeState> enumeration
 *   is used to report both the state and mode (see <IzotStatus>),
 *   it is *not* possible to change both the state and mode (online/offline) at
 *   the same time.
 *
 *   You can also use the shorthand functions IzotGoOnline(), IzotGoOffline(),
 *   IzotGoConfigured(), and IzotGoUnconfigured().
*/
IZOT_EXTERNAL_FN LonStatusCode IzotSetNodeMode(const IzotNodeMode mode, const IzotNodeState state)
{
    switch (mode) {
    case IzotApplicationOffLine: 
        // Go to soft offline state
        if (AppPgmRuns()) {
            IzotOffline(); // Indicate to application program.
        }
        gp->appPgmMode = OFF_LINE;
        break;

    case IzotApplicationOnLine: 
        // Go on-line
        IzotOnline(); // Indicate to application program.
        gp->appPgmMode = ON_LINE;
        break;

    case IzotApplicationReset: 
        // Application reset
        gp->resetNode = TRUE;
        nmp->resetCause = IzotSoftwareReset; // Software reset.
        break;

    case IzotChangeState: // Change State
        IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE, state);
        // Preserve the state of appPgmMode except for IzotNoApplicationUnconfig
        if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotNoApplicationUnconfig || 
                IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotStateInvalid_7) {
            gp->appPgmMode = NOT_RUNNING;
        }
        RecomputeChecksum();
        LCS_WriteNvm();
        break;

	case IzotPhysicalReset: 
        // New Physical Reset Sub Command
		PhysicalResetRequested();
		break;

    default:
        // Reset the device for this case
        gp->resetNode = TRUE;
        nmp->resetCause = IzotSoftwareReset;
        break;
    }
    return LonStatusNoError;
}

/*
 * Requests a copy of a local domain table record.
 * Parameters:
 *   index: Index of requested domain table record (0, 1)
 *   pDomain: Pointer to a <IzotDomain> structure
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to request a copy of a local domain table record.
 *   The information is returned through the provided <IzotDomain> structure.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryDomainConfig(unsigned index, IzotDomain* const pDomain)
{
    IzotDomain *p = (IzotDomain *)AccessDomain(index);
    memcpy(pDomain, p, sizeof(IzotDomain));
    return LonStatusNoError;
}

/*
 * Updates a domain table record on the IzoT device.
 *
 * Parameters:
 *   index: The index of the domain table to update
 *   pDomain: Pointer to the domain table record
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function can be used to update one record of the domain table.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateDomainConfig(unsigned index, const IzotDomain* const pDomain)
{
    LonStatusCode status;
    IzotDomain *p = AccessDomain(index);

    status = UpdateDomain(pDomain, index, true);
    if (status != LonStatusNoError) {
        return LonStatusIndexInvalid;
    }
    
    if (index == 0) {
        if (pDomain->Subnet != 0 && p->Subnet != pDomain->Subnet) {
            uint32_t oldaddr = BROADCAST_PREFIX | p->Subnet;
            uint32_t newaddr = BROADCAST_PREFIX | pDomain->Subnet;
            RemoveIPMembership(oldaddr);
            AddIpMembership(newaddr);
        }
    }
    
    RecomputeChecksum();
    LCS_WriteNvm();

    return LonStatusNoError;
}

/*
 * Updates a domain table record and changes the LON stack to online and configured.
 * Parameters:
 *   index: Domain index
 *   length: Domain ID length (0, 1, 3, or 6)
 *   domainId: Domain ID (number of bytes specified by length)
 *   subnet: Subnet ID
 *   node: Node ID
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateDomain(unsigned index, unsigned length, const IzotByte* domainId, unsigned subnet, unsigned node)
{
	IzotDomain Domain;
    LonStatusCode status = LonStatusNoError;

	memcpy(&Domain, access_domain(index), sizeof(Domain));

    // Set the domain ID length, domain ID, subnet ID, and node ID, nonclone flag, and mark the domain valid
    IZOT_SET_ATTRIBUTE(Domain, IZOT_DOMAIN_ID_LENGTH, length);
    memcpy(Domain.Id, domainId, length);
    Domain.Subnet = subnet;
	IZOT_SET_ATTRIBUTE(Domain, IZOT_DOMAIN_NODE, node);
    IZOT_SET_ATTRIBUTE(Domain, IZOT_DOMAIN_NONCLONE, index == 0);  // 0 = if it's a clone domain, 1= otherwise
    IZOT_SET_ATTRIBUTE(Domain, IZOT_DOMAIN_INVALID, 0);

	if (index == 0) {
        // Set authentication type and DCHP flag for domain index 0
		IZOT_SET_ATTRIBUTE(Domain, IZOT_AUTH_TYPE, AUTH_OMA);
		IZOT_SET_ATTRIBUTE(Domain, IZOT_DHCP_FLAG, 1);
	}
	
	if (memcmp(&Domain, access_domain(index), sizeof(Domain))) {
        // Domain changed, update the domain table
        status = IzotUpdateDomainConfig(index, &Domain);

        if (index == 0) {
            // Go configured and online for domain index 0
            IzotGoConfigured();
            IzotGoOnline();
        }
	}

	return status;
};

/*
 * Requests a copy of address table configuration data.
 * Parameters:
 *   index: Index of requested address table entry
 *   pAddress: Pointer to a <IzotAddress> structure
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Noteks:
 *   Call this function to request a copy of the address table configuration 
 *   data. The configuration is stored in the provided <IzotAddress> structure.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryAddressConfig(unsigned index, IzotAddress* const pAddress)
{
    IzotAddress *p = (IzotAddress *)AccessAddress(index);
    memcpy(pAddress, p, sizeof(IzotAddress));
    return LonStatusNoError;
}

/*
 * Updates an address table record on the LON Stack device.
 * Parameters:
 *   index: Index of the address table to update
 *   pAddress: Pointer to address table record
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Use this function to write a record to the local address table.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateAddressConfig(unsigned index, const IzotAddress* const pAddress)
{
    uint32_t oldaddr, newaddr;
    oldaddr = BROADCAST_PREFIX | 0x100 | eep->addrTable[index].Group.Group;
    UpdateAddress((const IzotAddress*)pAddress, index);
        
    // If the new update request for this entry is a group entry 
    // then add the new group address
    newaddr = BROADCAST_PREFIX | 0x100 | eep->addrTable[index].Group.Group;
    if (IZOT_GET_ATTRIBUTE(eep->addrTable[index].Group, IZOT_ADDRESS_GROUP_TYPE) == 1 && oldaddr != newaddr) {
        RemoveIPMembership(oldaddr);
        AddIpMembership(newaddr);
    }
    RecomputeChecksum();
    LCS_WriteNvm();
    return LonStatusNoError;
}

/*
 * Requests a copy of datapoint configuration data.
 * Parameters:
 *  index: Index of requested datapoint configuration table entry
 *  pDatapointConfig: Pointer to a <IzotDatapointEcsConfig> or
 *             <IzotDatapointConfig> structure
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to request a copy of the local datapoint
 *   configuration data. The configuration is stored in the provided structure.
 *   This API uses a signed index for compatibility with enumerations
 *   of datapoint index values typically used with the application
 *   framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryDpConfig(signed index, IzotDatapointConfig* const pDatapointConfig)
{
    memcpy(pDatapointConfig, &eep->nvConfigTable[index], sizeof(IzotDatapointConfig));
    return LonStatusNoError;
}

/*
 * Updates a datapoint configuration table record on the IzoT device.
 * Parameters:
 *   index: Index of datapoint
 *   pDatapointConfig: Datapoint configuration
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function can be used to update one record of the datapoint configuration
 *   table.  This API uses a signed index for compatibility with enumerations
 *   of datapoint index values typically used with the application
 *   framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateDpConfig(signed index, const IzotDatapointConfig* const pDatapointConfig)
{
    memcpy(&eep->nvConfigTable[index], pDatapointConfig, sizeof(IzotDatapointConfig));
    RecomputeChecksum();
    LCS_WriteNvm();
    return LonStatusNoError;
}

/*
 * Sets the static configuration for a datapoint (NV).
 * Parameters:
 *   pDatapointDef: Datapoint definition
 *   value: Datapoint value
 *   size: Datapoint size in bytes
 *   snvtId: Standard type index; set to 0 for a non-standard type
 *   arrayCount: Number of elements in a datapoint (NV) array; set to 0 for a single value
 *   name: Short datapoint (NV) name
 *   sdString: Long datapoint (NV) name; may include LonMark self-documentation
 *   maxRate: Estimated maximum update rate; set to IZOT_DATAPOINT_RATE_UNKNOWN if unknown
 *   meanRate: Estimated average update rate; set to IZOT_DATAPOINT_RATE_UNKNOWN if unknown
 *   ibol: Memory address for file-based configuration properties; set to NULL if none
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function does not update the datapoint definition flags or the datapoint configuration.
 *   Use IzotDatapointConfiguration() for those.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointSetup(IzotDatapointDefinition* const pDatapointDef, 
        volatile void const *value, IzotDatapointSize size, uint16_t snvtId, uint16_t arrayCount, 
        const char *name, const char *sdString, uint8_t maxRate, uint8_t meanRate, const uint8_t *ibol) 
{
    LonStatusCode status = LonStatusNoError;
    pDatapointDef->Version = 2;
    pDatapointDef->PValue = value;
    pDatapointDef->DeclaredSize = size;
    pDatapointDef->SnvtId = snvtId;
    pDatapointDef->ArrayCount = arrayCount;
    pDatapointDef->Name = name;
    pDatapointDef->SdString = sdString;
    pDatapointDef->MaxRate = maxRate;
    pDatapointDef->MeanRate = meanRate;
    pDatapointDef->ibol = ibol;
    return(status);
}

/*
 * Sets the datapoint definition flags for a datapoint (NV).
 * Parameters:
 *   pDatapointDef: Pointer to datapoint definition (includes Flags field)
 *   pDatapointConfig: Pointer to datapoint configuration
 *   priority: Set to TRUE for a priority datapoint (NV)
 *   direction: Input or output
 *   isProperty: Set to TRUE for property datapoints (configuration properties)
 *   persistent: Set to TRUE for persistent datapoints
 *   changeable: Set to TRUE for changeable type datapoints
 *   authenticated: Set to TRUE for authenticated transactions
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function only updates the datapoint definition flags.
 *   Use IzotDatapointSetup() for setting datapoint definition fields not included
 *   in the flags.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointFlags(IzotDatapointDefinition* const pDatapointDef,
        IzotBool priority, IzotDatapointDirection direction, IzotBool isProperty, IzotBool persistent, IzotBool changeable, IzotBool authenticated) 
{
    LonStatusCode status = LonStatusNoError;
    uint16_t flags = pDatapointDef->Flags;
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_PRIORITY) | (priority ? IZOT_DATAPOINT_PRIORITY : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_IS_OUTPUT) 
            | ((direction == IzotDatapointDirectionIsOutput) ? IZOT_DATAPOINT_IS_OUTPUT : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_CONFIG_CLASS) | (isProperty ? IZOT_DATAPOINT_CONFIG_CLASS : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_PERSISTENT) | (persistent ? IZOT_DATAPOINT_PERSISTENT : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_CHANGEABLE) | (changeable ? IZOT_DATAPOINT_CHANGEABLE : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_AUTHENTICATED) | (authenticated ? IZOT_DATAPOINT_AUTHENTICATED : 0);
    return(status);
}

/*
 * Connects a datapoint (NV).  
 * Parameters:
 *   nvIndex: Datapoint (NV) index (index into the NV configuration table)
 *   address: Address table index (index into the address table)
 *   selector: NV selector
 *   turnaround: Turnaround flag; set to TRUE if source or destination is on same device
 *   service: Delivery service
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Connecting is the process of creating a connection to or from a datapoint
 *   from or to one or more datapoints.  Sometimes also called "binding."
 * 
 *   This function only updates the datapoint connection information.  Use IzotDatapointSetup() and
 *   IzotDatapointFlags() for setting other datapoint configuration.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointBind(int nvIndex, IzotByte address, IzotUbits16 selector, 
        IzotBool turnAround, IzotServiceType service)
{
    LonStatusCode status = LonStatusNoError;
    IzotDatapointConfig DatapointConfig;

    status = IzotQueryDpConfig(nvIndex, &DatapointConfig);

    if (status != LonStatusNoError) {
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_ADDRESS_HIGH, address >> 4);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_ADDRESS_LOW, address);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_SELHIGH, high_byte(selector));
        DatapointConfig.SelectorLow = low_byte(selector);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_TURNAROUND, turnAround);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_SERVICE, service);
        status = IzotUpdateDpConfig(nvIndex, &DatapointConfig);
    }

    return(status);
}

/*
 * Requests a copy of alias configuration data.
 * Parameters:
 *   index: Index of requested alias configuration table entry
 *   pAlias: Pointer to a <IzotAliasEcsConfig> structure
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to request a copy of the alias configuration data.
 *   The configuration is stored in the <IzotAliasEcsConfig> structure
 *   provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryAliasConfig(unsigned index, IzotAliasConfig* const pAlias)
{
    memcpy(pAlias, &eep->nvAliasTable[index], sizeof(IzotAliasConfig));
    return LonStatusNoError;
}

/*
 * Updates an alias table record on the LON Stack device.
 * Parameters:
 *   index: Index of alias table record to update
 *   pAlias: Pointer to the alias table record
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function writes a record in the local alias table.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateAliasConfig(unsigned index, const IzotAliasConfig* const pAlias)
{
    memcpy(&eep->nvAliasTable[index], pAlias, sizeof(IzotAliasConfig));
    RecomputeChecksum();
    LCS_WriteNvm();
    return LonStatusNoError;
}

/*
 * Determines whether a datapoint is bound given its index.
 * Parameters:
 *   index: Index of the datapoint
 *   pIsBound: Pointer to receive the "is-bound" attribute
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   See <IzotDatapointIsBound> for an alternative API.
 * 
 *   Call this function to determine whether a datapoint is bound.
 *   A datapoint is bound if it, or any of its aliases, has a bound
 *   selector or an address table entry. The unbound selector for a given
 *   datapoint is equal to (0x3fff - datapoint index).  A datapoint
 *   or alias has an address if the address index is not equal to 0xffff.
 *   This API uses a signed index for compatibility with enumerations
 *   of datapoint index values typically used with the application
 *   framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointIsBoundByIndex(signed index, IzotBool* const pIsBound)
{
	if(IsNVBound(index)) {
		*pIsBound = TRUE;
    } else {
        *pIsBound = FALSE;
    }
	return LonStatusNoError;
}

/*
 * Determines whether a message tag is bound.
 * Parameters:
 *   tag: The message tag
 *   pIsBound: Pointer to receive the "is-bound" attribute
 * Returns:
 *   LonStatusNoError if no error, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to determine whether a message tag is bound.
 *   A message tag is bound if the associated address type is anything other
 *   than *IzotAddressUnassigned*.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotMtIsBound(const unsigned tag, IzotBool* const pIsBound)
{
	if(IsTagBound(tag)) {
		*pIsBound = TRUE;
	} else {
		*pIsBound = FALSE;
    }
	return LonStatusNoError;
}

/*****************************************************************
 * Section: Persistent Data API Function Definitions
 *****************************************************************/
/*
 * This section details the API functions that support persistent data (non-
 * volatile data).
 *
 * Notes:
 *   Persistent data is stored in data segments, identified by
 *   <IzotPersistentDataSegmentType>, and are used to store IzoT persistent
 *   configuration data.
 */

/*
 * Informs the LON Stack that the application data segment has been updated.
 * Returns:
 *   LonStatusNoError if no error, else a <LonStatusCode> error code.
 * Notes:
 *   Use this function to inform the LON Stack that some application
 *   data has been updated that should be written out to the
 *   <IzotPersistentSegApplicationData> persistent data segment.  The LON
 *   Stack will schedule a write to the <IzotPersistentSegApplicationData>
 *   segment after the flush timeout has expired.
 *
 *   It is generally not necessary to call this function when application data
 *   has been updated by a network management write command or a datapoint 
 *   update, because the LON Stack automatically calls this function 
 *   whenever the <IzotMemoryWrite> event handler returns <LonStatusCode>, and 
 *   whenever a datapoint update is received for a datapoint with the
 *   *IZOT_DATAPOINT_CONFIG_CLASS* or *IZOT_DATAPOINT_PERSISTENT* attribute.
 *   However, the application must call this function whenever it updates
 *   application-specific persistent data directly.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentAppSegmentHasBeenUpdated(void)
{
    IzotPersistentMemStartCommitTimer();
    return LonStatusNoError;
}

/*
 * Flushes all persistent data out to persistent storage.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if no error, else a <LonStatusCode> error code.
 * Notes:
 *   This function can be called by the application task to block until all
 *   persistent data writes have been completed.  The application might do
 *   this, for example, in response to a <IzotPersistentStarvation> event.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentFlushData(void)
{
    IzotPersistentMemSetCommitFlag();
    return LonStatusNoError;
}

/*
 * Gets the application segment size.
 * Parameters:
 *   None
 * Returns:
 *   The size of the application segment in bytes.
 */
size_t IzotGetAppSegmentSize(void)
{
	if(izot_get_app_seg_size_handler) {
		return izot_get_app_seg_size_handler();
	} else {
		return 0;
    }
}

/*
 * Gets the number of bytes required to store persistent data.
 * Parameters:
 *   persistentSegType: The segment type, see <IzotPersistentSegType>
 * Returns:
 *   The number of bytes required to store persistent data for the specified
 *   segment.
 * Notes:
 *   This function will not typically be called directly by the application,
 *   but may be used by persistent data event handlers (implemented by the
 *   application) to reserve space for persistent data segments.
 */
IZOT_EXTERNAL_FN int IzotPersistentSegGetMaxSize(IzotPersistentSegType persistentSegType)
{
	int length = 0;
    switch(persistentSegType) {
    case IzotPersistentSegNetworkImage:
        length = (sizeof(*eep)-sizeof(eep->readOnlyData));
        break;
    case IzotPersistentSegApplicationData:
        length = IzotGetAppSegmentSize();
        break;
#ifdef SECURITY_II
    case IzotPersistentSegSecurityII:
        length = IzotGetSecIIPersistentDataSize();
        break;
#endif
    default:
        length = 0;
    }
    return length;
}

/*
 * Secures the LON Stack device against unauthorized use
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   Uses a digest to secure the device.
 */
#if LINK_IS(WIFI)
#define FLICKER_INTERVAL 200    // LED flicker interval in milliseconds

static void UnlockWiFiDevice(void)
{
	uint8_t temp_y[8] = {0};
	uint8_t digestKeyFlash[8] = {0};
	uint8_t macAddress[6] = {0};            // Array to set MAC address
    LonTimer flickr_timer;
    
    // Get MAC address and digest from SPI
	HalGetMacAddress(macAddress);
    
    // Read the digest from flash
    iflash_drv_read(NULL, digestKeyFlash, sizeof(digestKeyFlash), 0xFF8);
    
    // Calculate the digest based on MAC Address
    Encrypt((IzotByte *)C, (void *) macAddress, sizeof(macAddress), (IzotByte *) K, temp_y, 0, NULL);
    
    if (memcmp(digestKeyFlash, temp_y, sizeof(digestKeyFlash))) {
        gp->serviceLedState = SERVICE_FLICKER;
        SetLonRepeatTimer(&flickr_timer, FLICKER_INTERVAL, FLICKER_INTERVAL);
    }
    
	while (memcmp(digestKeyFlash, temp_y, sizeof(digestKeyFlash))) {
        if (LonTimerExpired(&flickr_timer)) {
            IzotServiceLedStatus(gp->serviceLedState, gp->serviceLedPhysical);
            gp->serviceLedPhysical = 1 - gp->serviceLedPhysical;
        }
        IzotSleep(100);
    }
}
#endif  // LINK_IS(WIFI)

/*****************************************************************
 * Section: LON Stack Lifetime Management Function Definitions
 *****************************************************************/ 
/*
 * Initializes the LON Stack.
 * Parameters:
 *   pInterface: Pointer to a <IzotStackInterfaceData> structure, which defines
 *              the static attributes of the program
 *   pControlData: Pointer to a <IzotControlData> structure, which contains data
 *              used to control the runtime aspects of the stack.  These aspects
 *              can be set by the application at runtime.
 * Returns:
 *   LonStatusNoError if no error occurred, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Initializes and configures the LON driver and the LON Stack.  This function
 *   must be the first call into the LON Stack API, and cannot be called again
 *   until <IzotDestroyStack> has been called.  After this function has been
 *   called, the following functions can be called: IzotRegisterStaticDatapoint(),
 *   IzotRegisterMemoryWindow(), IzotStartStack(), and IzotDestroyStack().
 *
 *   The stack expects reasonable values for all initialization parameters.
 *   Therefore, the stack does not provide detailed error information when
 *   a parameter is out of range.
 *
 *   If IzotCreateStack() returns any error, the stack will not function.
 *   It will not send or receive  messages over the network.  The Service LED,
 *   if present, will typically be left on (applicationless).
 */
IZOT_EXTERNAL_FN LonStatusCode IzotCreateStack(const IzotStackInterfaceData* const pInterface, 
        const IzotControlData * const pControlData)
{
    LonStatusCode status = LonStatusNoError;
#if LINK_IS(WIFI)
    char oldProgId[8];
#endif 

    // Only a few of these fields are used by LON Stack.  The
    // stack implements partial support for a multi-stack model, but 
    // the stack is limited to a single stack model.  Only stack[0]
    // is supported.    
    SetAppSignature(pInterface->Signature);
    SetPeristenceGuardBand(pControlData->PersistentFlushGuardTimeout*1000);
    nm[0].snvt.sb = (char *)pInterface->SiData;
    SetSiDataLength(pInterface->SiDataLength);    
    cp  = &customDataGbl[0];
    cp->twoDomains = pInterface->Domains-1;
    cp->addressCnt = pInterface->Addresses;
    cp->szSelfDoc = (char *)pInterface->NodeSdString;
    memcpy(cp->progId, pInterface->ProgramId, sizeof(cp->progId));
    memset(cp->location, 0, sizeof(cp->location));
    cp->len[0] = 0;
    memset(cp->domainId[0], 0, IZOT_DOMAIN_ID_MAX_LENGTH);
    cp->subnet[0] = rand()%255 + 1; // Avoid 0.
    cp->node[0] =   rand()%124 + 2; // Avoid 0 plus 1, 126, 127 (used by NIs)
    cp->clone[0] = 1;
    cp->len[1] = 1;
    IzotByte tempDmn[6] = {0x7A};
    memcpy(cp->domainId[1], &tempDmn[0], IZOT_DOMAIN_ID_MAX_LENGTH);
    cp->clone[1] = 0;
    memset(cp->key[0], 0, IZOT_AUTHENTICATION_KEY_LENGTH);
    memset(cp->key[1], 0, IZOT_AUTHENTICATION_KEY_LENGTH);
    DataPointCount = pInterface->StaticDatapoints;
    AliasTableCount = pInterface->Aliases;
    BindableMTagCount = pInterface->BindableMsgTags;

#if LINK_IS(WIFI)
    // Initialize Wi-Fi interface
    status = WiFiInit();
    if (status != LonStatusNoError) {
        OsalPrintError(status, "IzotCreateStack: Wi-Fi initialization failed");
        return status;
    }
#endif  // LINK_IS(WIFI)

    // Initialize LON Stack
    status = LCS_Init(IzotPowerUpReset);
    if (status != LonStatusNoError) {
        OsalPrintError(status, "IzotCreateStack: LON Stack initialization failed");
        return status;
    }

#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    // Start the UDP interface
    status = UdpStart();
    if (status != LonStatusNoError) {
        return status;
    }
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)

#if LINK_IS(WIFI)
    // Start the Wi-Fi interface
    status = WiFiStart();
    if (status != LonStatusNoError) {
        OsalPrintError(status, "IzotCreateStack: Wi-Fi start failed");
        return status;
    }
#endif  // LINK_IS(WIFI)

    return status;
}

/*
 * Registers a static datapoint with the IzoT Device Stack.
 * Parameters:
 *   pDatapointDef: Pointer to a <IzotDatapointDefinition> structure
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function registers a static datapoint with the IzoT Device Stack API,
 *   and is called once for each static datapoint.  This function can be
 *   called only after <IzotCreateStack>, but before <IzotStartStack>.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotRegisterStaticDatapoint(IzotDatapointDefinition* const pDatapointDef) {
    LonStatusCode status = LonStatusNoError;
    NVDefinition d;
    IzotBits16 returnValue;

    uint16_t flags = pDatapointDef->Flags;
    memset(&d, 0, sizeof(d));
    d.snvtType = pDatapointDef->SnvtId;
    d.nvName = pDatapointDef->Name;
    d.nvSdoc = pDatapointDef->SdString;
    d.varAddr = pDatapointDef->PValue;
    d.nvLength = pDatapointDef->DeclaredSize;
    d.arrayCnt = pDatapointDef->ArrayCount;
    d.direction = 
    (flags & IZOT_DATAPOINT_IS_OUTPUT) ? 
    IzotDatapointDirectionIsOutput : IzotDatapointDirectionIsInput;
    d.priority = (flags & IZOT_DATAPOINT_PRIORITY) ? true : false;
    d.auth = (flags & IZOT_DATAPOINT_AUTHENTICATED) ? true : false;
    d.bind = true;
    d.service = IzotServiceAcknowledged;
    d.persist = (flags & (IZOT_DATAPOINT_PERSISTENT+IZOT_DATAPOINT_CONFIG_CLASS)) ? true : false;
    if (pDatapointDef->ibol) {
        d.ibol = pDatapointDef->ibol;
    } else {
        d.ibol = NULL;
    }

    if (flags & IZOT_DATAPOINT_CHANGEABLE) {
        d.changeable = true;
    }
    
    if (flags & IZOT_DATAPOINT_UNACKD_RPT) {
        d.service = IzotServiceRepeated;
    } else if (flags & IZOT_DATAPOINT_UNACKD) {
        d.service = IzotServiceUnacknowledged;
    }
    d.maxrEst = pDatapointDef->MaxRate;
    d.rateEst = pDatapointDef->MeanRate;
    if (d.maxrEst != IZOT_DATAPOINT_RATE_UNKNOWN) {
        d.snvtExt |= 0X80;
    }
    if (d.rateEst != IZOT_DATAPOINT_RATE_UNKNOWN) {
        d.snvtExt |= 0x40;
    }
    if (d.nvName && strlen(d.nvName)) {
        d.snvtExt |= 0x20;
    }
    if (d.nvSdoc && strlen(d.nvSdoc)) {
        d.snvtExt |= 0x10;
    }
    if (d.arrayCnt > 1) {
        d.snvtExt |= 0x08;
    }
    d.snvtDesc = flags & 0x7F;
    if (d.snvtExt) {
        d.snvtDesc |= 0x80;
    }
    returnValue = AddNV(&d);
    if (returnValue == -1) {
        status = LonStatusInvalidParameter;
    } else {
        pDatapointDef->NvIndex = returnValue;
    }

    return status;
}

/*
 * Registers a virtual memory address range and enable DMF.
 * Parameters:
 *   windowAddress: The starting virtual address of the window
 *   windowSize: The size of the window, in bytes
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   This function is used to open up a window in the device's memory
 *   space. LON protocol messages that access memory using absolute addressing
 *   within the 64kB legacy address range, provide the memory addressed falls
 *   within the registered window, can access memory located within the LON
 *   application through <IzotMemoryRead> and <IzotMemoryWrite> synchronous
 *   events.
 *
 *   This function can only be called after IzotCreateStack(), but before
 *   IzotStartStack().  The address space for these memory windows is between
 *   0x0001 and 0xffff, but some LON stacks may further limit the supported
 *   address range.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotRegisterMemoryWindow(const unsigned windowAddress, const unsigned windowSize)
{
    LonStatusCode status = LonStatusNoError;
    setMem(windowAddress, windowSize);
    return status;
}

/*
 * Completes the initialization of LON Stack.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Starts running the stack, following successful calls to IzotCreateStack(),
 *   IzotRegisterStaticDatapoint(), IzotRegisterMemoryWindow(), and other
 *   initialization-time functions.
 *
 *   When the IzotStartStack() function returns with success, the device stack
 *   is fully operational and all persistent data (if any) has been applied.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotStartStack(void)
{
    // Load persistent NVs from NVM
    if (IzotGetAppSegmentSize() != 0) {
        if (LCS_ReadNvs() != LonStatusNoError) {
            OsalPrintDebug(LonStatusNoError, "IzotStartStack: No application data found--put the device into unconfigured mode");
            ErasePersistenceData();
            ErasePersistenceConfig();
            IzotPersistentSegSetCommitFlag(IzotPersistentSegApplicationData);
            IzotPersistentAppSegmentHasBeenUpdated();
            IzotSetNodeMode(IzotChangeState, IzotApplicationUnconfig);
        }
    }
    return LonStatusNoError;
}

/*
 * Stops LON Stack and frees all memory that it has allocated.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Waits for persistent writes to complete, stops the stack, and frees all
 *   temporary memory created during execution of the stack.  The Service LED is
 *   lit to indicate that the device is applicationless.
 */
IZOT_EXTERNAL_FN void IzotDestroyStack(void)
{
    // Do nothing for this implementation
}

/*
 * Requests copy of local read-only data.
 * Parameters:
 *   pReadOnlyData: Pointer to a <IzotReadOnlyData> structure
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Call this function to request a copy of device's read-only data.
 *   The read-only data will be stored in the provided <IzotReadOnlyData>
 *   structure.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryReadOnlyData(IzotReadOnlyData* const pReadOnlyData)
{
    memcpy(pReadOnlyData, (IzotReadOnlyData*)&eep->readOnlyData, 
    sizeof(IzotReadOnlyData));
    return LonStatusNoError;
}

/*
 * Gets the LON application's signature.
 * Parameters:
 *   None
 * Returns:
 *   The application signature which was specified by the application
 *   when the stack is created in IzotCreateStack().
 */
IZOT_EXTERNAL_FN unsigned IzotGetAppSignature(void)
{
    return GetAppSignature();
}

/*
 * Gets the number of aliases supported by the alias table.
 * Parameters:
 *   None
 * Returns:
 *   The size of the alias table which is specified by the application
 *   when the stack is created in IzotCreateStack().
 */
IZOT_EXTERNAL_FN unsigned IzotGetAliasCount(void)
{
    return eep->readOnlyData.AliasCount;
}

/*
 * Gets the number of addresses supported by the address table.
 * Parameters:
 *   None
 * Returns:
 *   The size of the address table which is specified by the application
 *   when the stack is created in IzotCreateStack().
 */
IZOT_EXTERNAL_FN unsigned IzotGetAddressTableCount(void)
{
    return IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_ADDRESS_CNT);
}

/*
 * Gets the number of static datapoints supported by the device.
 * Parameters:
 *   None
 * Returns:
 *   The number of static datapoints specified by the application
 *   when the stack is created in IzotCreateStack().
 */
IZOT_EXTERNAL_FN unsigned IzotGetStaticDatapointCount(void)
{
    return eep->readOnlyData.DatapointCount;
}

/*
 * Gets the Domain id from Local IP address.
 * Parameters:
 *   pDid: Pointer to the domain ID buffer
 *   pDidLen: Pointer to the domain ID length
 *   pSub: Pointer to the subnet ID
 *   pNode: Pointer to the node ID
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Get the domain ID from a local IP address for LON/IP only.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotGetDidFromLocalAddress(IzotByte* pDid, IzotByte* pDidLen, IzotByte* pSub, 
IzotByte* pNode)
{
#if LINK_IS(ETHERNET) || LINK_IS(WIFI)
    const IzotByte* pDomainId = (IzotByte *)ownIpAddress;
    if (ownIpAddress[0] == 192 && ownIpAddress[1] == 168) {
        *pDidLen = 0;
    } else if (ownIpAddress[0] == 10) {
        *pDidLen = 1;   // 1 byte domain
        pDomainId++;
    } else {
        *pDidLen = 2;   // 3 byte domain
    }
    
    memcpy(pDid, pDomainId, *pDidLen);
    *pSub = ownIpAddress[2];
    *pNode = ownIpAddress[3];
    return LonStatusNoError;
#else
    return LonStatusInvalidParameter;
#endif  // LINK_IS(ETHERNET) || LINK_IS(WIFI)
}

/*
 * Determines whether or not an application is running for the first time.
 * Parameters:
 *   None
 * Returns:
 *   True if this is the first time the application is running with the same
 *   setup and configuration.
 * Notes:
 *   You can use this information if you need to initialize certain
 *   values only the first time the application is running.
 */
IZOT_EXTERNAL_FN IzotBool IzotIsFirstRun(void)
{
    IzotBool result = FALSE;
 
#if PROCESSOR_IS(MC200)
    // Check if first run, if so, set variable
    char first_run[1] = "";
    
    psm_get_single(IZOT_MOD_NAME, "first_run", first_run, 2);
    
    if(strcmp(first_run, "y") == 0) {
        result = TRUE;
    }
#endif  // PROCESSOR_IS(MC200)

    return result;
}

/*****************************************************************
 * Section: Callback Function Prototype Definitions
 *****************************************************************/
/*
 * This section defines the prototypes for the LON Stack callback
 * functions.
 *
 * Callback functions are called by the LON Stack immediately, as needed,
 * and may be called from any LON Stack task.  The application *must not*
 * call into the LON Stack API from within a callback.
 */

/*
 * Implements the IzotGetCurrentDatapointSize() callback function.
 * Parameters:
 *   index: Local index of the datapoint
 * Returns:
 *   Current size of the datapoint. Zero if the index is invalid.
 * Notes:
 *   If the datapoint size is fixed, this function should return
 *   <IzotGetDeclaredDpSize>. If the datapoint size is changeable, the
 *   current size should be returned. The default implementation for changeable
 *   type datapoints returns 0, and must be updated by the application
 *   developer.
 *
 *   LON Stack will not propagate a datapoint with size 0, nor will it generate
 *   an update event if a datapoint update is received from the network when
 *   the current datapoint size is 0.
 *
 *   Even though this is a callback function, it *is* legal for the
 *   application to call IzotGetDeclaredDpSize() from this callback.
 */
unsigned IzotGetCurrentDatapointSize(const unsigned index)
{
    unsigned result = NV_LENGTH(index);
 
    if (izot_get_dp_size_handler && IZOT_GET_ATTRIBUTE(izot_dp_prop[index], IZOT_DATAPOINT_CHANGEABLE_TYPE)) {
        const unsigned application_size = izot_get_dp_size_handler(index);
        if (application_size && application_size != (unsigned)-1) {
            result = application_size;
        }
    }
    return result;
}

/*****************************************************************
 * Section: Event Handler Function Prototype Definitions
 *****************************************************************/
/*
 *  This section defines the prototypes for the LON Stack event 
 *  handler functions.
 *
 *  Like callback functions, event handlers are called from the LON Stack 
 *  API. However, unlike callback functions, event handlers are only 
 *  called in the context of the application task, and only when the 
 *  application calls the IzotEventPump() function. Also, the application 
 *  may make LON Stack API function calls from within an event handler.
 */

/*
 * Handles the IzotReset event.
 * Parameters:
 *   pResetNotification - Pointer to a <IzotResetNotification> structure with
 *              capabilities and identifying data or NULL
 * Returns:
 *   None
 * Notes:
 *   The IzotReset event occurs when the LON protocol stack has been reset.
 *   The pointer to <IzotResetNotification> is provided for call compatibility
 *   with the ShortStack LonTalk Compact API.  For LON Stack, the value of
 *   pResetNotification is always NULL.
 *
 *   Whenever the LON Stack DX device has been reset, the mode of the device is changed
 *   to *online*, but no IzotOnline() event is generated.
 *
 *   Resetting the LON Stack DX device only affects the LON stack and does not
 *   cause a processor or application software reset.
 */
void IzotReset(const IzotResetNotification* const pResetNotification)
{
    if (izot_reset_handler) {
		izot_reset_handler();
    }
}

/*
 * Handles the IzotWink event.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The IzotWink event occurs when the LON Stack device receives a Wink
 *   command.  This event is not triggered when LON Stack receives Wink
 *   sub-commands (extended install commands).
 */
void IzotWink(void)
{
	if (izot_wink_handler) {
		izot_wink_handler();
    }
}

/*
 * Handles the IzotOffline event.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The IzotOffline event occurs when the LON Stack device has entered
 *   the offline state.  While the device is offline, the LON Stack will
 *   not generate datapoint updates, and will return an error when
 *   IzotPropagateDp() is called.
 *
 *   Offline processing in LON Stack differs from that in ShortStack.
 *   When a ShortStack Micro Server receives a request to go offline, it sends
 *   the request to the ShortStack LonTalk Compact API, which calls the
 *   application callback IzotOffline().  The Micro Server does not actually go
 *   offline until the IzotOffline() callback function returns and the
 *   ShortStack LonTalk Compact API sends a confirmation message to the Micro
 *   Server.  In contrast, a LON Stack device goes offline as soon as it 
 *   receives the offline request. The IzotOffline() event is handled 
 *   asynchronously.
 */
void IzotOffline(void)
{
	if (izot_offline_handler) {
		izot_offline_handler();
    }
}

/*
 * Handles the IzotOnline event.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The IzotOnline event occurs when the LON Stack device has entered
 *   the online state.
 */
void IzotOnline(void)
{
    if (izot_online_handler) {
		izot_online_handler();
    }
}

/*
 * Handles the IzotServicePinPressed event.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The IzotServicePinPressed event occurs when the Service button has
 *   been activated.  The LON Stack sends a Service message automatically
 *   any time the Service button has been activated.
 */
void IzotServicePinPressed(void)
{
    ManualServiceRequestMessage();
}

/*
 * Handles the IzotServicePinHeld event.
 * Parameters:
 *   None
 * Returns:
 *   None
 * Notes:
 *   The IzotServicePinHeld event occurs when the Service button has been
 *   continuously activated for a configurable time.
 */
void IzotServicePinHeld(void)
{
    // TBD: Application specific processing
    OsalPrintDebug(LonStatusNoError, "IzotServicePinHeld: Service pin held event occurred");
}

/*
 * Handles the IzotDatapointUpdateOccurred event.
 * Parameters:
 *   index: NV index (local to the device) of the updated NV
 *   pSourceAddress: Pointer to source address description
 * Returns:
 *   None
 * Notes:
 *   The IzotDatapointUpdateOccurred event occurs when LON Stack receives
 *   a new value for an input NV from the LON network.  When this event
 *   occurs, the new value has already been stored in the NV's location.
 *   The application can access the new value through the global variable
 *   representing the NV, or obtain the pointer to the NV's value from
 *   IzotGetDatapointValue().  The pSourceAddress pointer is only valid
 *   for the duration of this event handler.
 *
 *   For an element of a datapoint array, the index is the NV index
 *   plus the array-element index.  For example, if nviVolt[0] has
 *   NV index 4, then nviVolt[1] has NV index 5.  Thus, if nviVolt[1]
 *   is updated, the index parameter will have the value index 5.
 */
void IzotDatapointUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
	if (izot_dp_update_occurred_handler) {
		izot_dp_update_occurred_handler(index, pSourceAddress);
    }
}

/*
 * Handles the IzotDatapointUpdateCompleted event.
 * Parameters:
 *   index: NV index (local to the device) of the updated NV
 *   success: Indicates whether the update was successful or unsuccessful
 * Returns:
 *   None
 * Notes:
 *   The IzotDatapointUpdateCompleted event signals completion of an NV
 *   update or poll transaction (see IzotPropagateDp() and IzotPollDp()). For
 *   unacknowledged or repeated messages, the transaction is complete when the
 *   message has been sent with the configured number of retries. For
 *   acknowledged messages, it is successfully complete when LON Stack
 *   receives an acknowledgement from each of the destination devices, and is
 *   unsuccessfully complete if not all acknowledgements are received.  Poll
 *   requests always use the request service type, and generate a successful
 *   completion if responses are received from all expected devices, and
 *   generate a failed completion otherwise.
 */
void IzotDatapointUpdateCompleted(const unsigned index, const IzotBool success)
{
	if (izot_dp_update_completed_handler) {
		izot_dp_update_completed_handler(index, success);
    }
}

/*
 * Handles the IzotDatapointAdded event.
 * Parameters:
 *   index: The index of the dynamic NV that has been added
 *   pDpDef: Pointer to the NV definition
 * Returns:
 *   None
 * Notes:
 *   The IzotDatapointAdded event signals that a dynamic datapoint has been
 *   added.  During device startup, LON Stack calls this function for each
 *   dynamic datapoint that had been previously defined.  The pDpDef pointer,
 *   along with all of its contents, is invalid when the function returns.  If
 *   the application needs to maintain this information, it must copy the data,
 *   taking care to avoid copying the pointers contained within the 
 *   <IzotDatapointDefinition> structure.
 *
 *   When a datapoint is first added, the name and the self-documentation
 *   string may be blank.  A network manager may update the name or the
 *   self-documentation string in a subsequent message, and LON Stack
 *   will call the <IzotDatapointTypeChanged> event handler.
 */
void IzotDatapointAdded(const unsigned index, const IzotDatapointDefinition* const pDpDef)
{
    // This event is not supported in LON Stack DX
}

/*
 * Handles the IzotDatapointTypeChanged event.
 * Parameters:
 *   index: The index of the dynamic NV that has been changed
 *   pDpDef: Pointer to the NV definition
 *
 * Notes:
 *   The IzotDatapointTypeChanged event signals that one or more attributes
 *   of a dynamic NV have been changed.  LON Stack calls this function when
 *   a dynamic NV definition has been changed.  The pDpDef pointer, along
 *   with all of its contents, is invalid when the function returns.  If 
 *   the application needs to maintain this information, it must copy the
 *   data, taking care to avoid copying the pointers contained within the
 *   <IzotDatapointDefinition> structure.
 */
void IzotDatapointTypeChanged(const unsigned index, const IzotDatapointDefinition* const pDpDef)
{
    // This event is not supported in LON Stack DX
}

/*
 * Handles the IzotDatapointDeleted event.
 * Parameters:
 *   index: The index of the dynamic NV that has been deleted
 * Returns:
 *   None
 * Notes:
 *   The IzotDatapointDeleted event signals that a dynamic datapoint has been deleted.
 *   LON Stack calls this function when a dynamic NV is deleted.
 */
void IzotDatapointDeleted(const unsigned index)
{
    //This event is not supported in LON Stack DX
}

/*
 * Handles the IzotMsgArrived event.
 * Parameters:
 *   pAddress: Source and destination address (see <IzotReceiveAddress>)
 *   correlator: Correlator to be used with <IzotSendResponse>
 *   authenticated: TRUE if the message was (successfully) authenticated
 *   code: Message code
 *   pData: Pointer to message data bytes, may be NULL if dataLength is zero
 *   dataLength: Length of bytes pointed to by pData
 *  Returns:
 *    None
 *  Notes:
 *    The IzotMsgArrived event occurs when an application message arrives.
 *    This event handler reports the arrival of a message that is neither an 
 *    NV update or a non-NV message that is otherwise processed by the
 *    LON Stack (such as a network management command). Typically, this is used
 *    with application message codes in the value range indicated by the
 *    <IzotApplicationMessageCode> enumeration. All pointers are only valid for
 *    the duration of this event handler.
 *
 *    If the message is a request message, then the function must deliver a
 *    response using <IzotSendResponse> passing the provided *correlator*.
 *    Alternatively, if for any reason the application chooses not to respond to
 *    a request, it must explicitly release the correlator by calling
 *    <IzotReleaseCorrelator>.
 *
 *    Application messages are always delivered to the application, regardless
 *    of whether the message passed authentication or not. It is up to the
 *    application to decide whether authentication is required for any given
 *    message and compare that fact with the authenticated flag. The
 *    authenticated flag is clear (FALSE) for non-authenticated messages and for
 *    authenticated messages that do not pass authentication. The authenticated
 *    flag is set only for correctly authenticated messages.
 */
void IzotMsgArrived(
        const IzotReceiveAddress* const pAddress,
        const IzotCorrelator correlator,
        const IzotBool priority,
        const IzotServiceType serviceType,
        const IzotBool authenticated,
        const IzotByte code,
        const IzotByte* const pData, const unsigned dataLength)
{
    if (izot_msg_arrived_handler) {
        izot_msg_arrived_handler(pAddress, correlator, priority, serviceType, authenticated, code, pData, dataLength);
    }
}

/*
 * Handles the IzotResponseArrived event.
 *  Occurs when a response arrives.
 * Parameters:
 *   pAddress: Source and destination address used for response (see <IzotResponseAddress>)
 *   tag: Tag to match the response to the request
 *   code: Response code
 *   pData: Pointer to response data, may by NULL if dataLength is zero
 *   dataLength: Number of bytes available through pData.
 * Returns:
 *   None
 * Notes:
 *   The IzotResponseArrived event occurs when a response arrives.
 *   Responses may be sent by other devices when the LON device sends a message
 *   using IzotSendMsg() with a <IzotServiceType> of *IzotServiceRequest*.
 */
void IzotResponseArrived(
        const IzotResponseAddress* const pAddress,
        const unsigned tag,
        const IzotByte code,
        const IzotByte* const pData, 
        const unsigned dataLength)
{
    if (izot_response_arrived_handler) {
        izot_response_arrived_handler(pAddress, tag, code, pData, dataLength);
    }
}

/*
 * Handles the IzotMsgCompleted event.
 * Parameters:
 *   tag: Used to correlate the event with the message sent
 *        Same as the *tag* specified in the call to IzotSendMsg()
 *   success: TRUE for successful completion, otherwise FALSE
 * Returns:
 *   None
 * Notes:
 *   The IzotMsgCompleted event occurs when a message transaction has
 *   completed.  See IzotSendMsg().  For unacknowledged or repeated messages,
 *   the transaction is complete when the message has been sent with the
 *   configured number of retries. For acknowledged messages, LON Stack calls
 *   IzotMsgCompleted() with *success* set to TRUE after receiving
 *   acknowledgments from all of the destination devices, and calls
 *   IzotMsgCompleted() with *success* set to FALSE if the transaction timeout
 *   period expires before receiving acknowledgements from all destinations.
 *   For request messages, the transaction is considered successful when
 *   LON Stack receives a response from each of the destination devices, and
 *   unsuccessful if the transaction timeout expires before responses have been
 *   received from all destination devices.
 */
void IzotMsgCompleted(const unsigned tag, const IzotBool success)
{
    if (izot_msg_completed_handler) {
        izot_msg_completed_handler(tag, success);
    }
}

/*
 * Handles the IzotFilterMsgArrived event.
 * Parameters:
 *   pAddress: Source and destination address (see IzotReceiveAddress())
 *   correlator: Correlator to be used with IzotSendResponse()
 *   authenticated: TRUE if the message was (successfully) authenticated
 *   code: Message code
 *   pData: Pointer to message data bytes, may be NULL if dataLength is zero
 *   dataLength: Length of bytes pointed to by pData
 * Returns:
 *   TRUE if the message has been processed by the filter event handler.
 * Notes:
 *   The IzotFilterMsgArrived event occurs when an application message arrives.
 *   Typically this the ISI engine to filter ISI messages.  If the message
 *   does not get processed by the filter handler, the message will be
 *   passed to the IzotMsgArrived() handler.
 *
 *   Use IzotFilterMsgArrivedRegistrar() to register a handler for this event.
 *   Without an application-specific handler, this event does nothing (no
 *   incoming messages are filtered, all are forwarded to the application).
 */
IzotBool IzotFilterMsgArrived(
        const IzotReceiveAddress* const pAddress,
        const IzotCorrelator correlator, 
        const IzotBool priority,
        const IzotServiceType serviceType, 
        const IzotBool authenticated,
        const IzotByte code, 
        const IzotByte* const pData, 
        const unsigned dataLength)
{
    if (izot_filter_msg_arrived) {
        return izot_filter_msg_arrived(pAddress, correlator, priority, serviceType, authenticated, code, pData, 
        dataLength);
    } else {
        return FALSE;
    }
}

/*
 * Handles the IzotFilterResponseArrived event.
 * Parameters:
 *   pAddress: Source and destination address used for response (see IzotResponseAddress())
 *   tag: Tag to match the response to the request
 *   code: Response code
 *   pData: Pointer to response data, may by NULL if dataLength is zero
 *   dataLength: Number of bytes available through pData
 * Returns:
 *   TRUE if the response has been processed by the filter event handler.
 * Notes:
 *   The IzotFilterResponseArrived event is signalled when a response arrives.
 *   The application can use this event handler to filter incoming response
 *   messages. Responses may be sent by other devices when LON application
 *   sends a message using IzotSendMsg() with a IzotServiceType of
 *   *IzotServiceRequest*.
 *
 *   Use IzotFilterResponseArrivedRegistrar() to register a handler for this
 *   event. Without an application-specific handler, this event does nothing
 *   (no incoming response is filtered, all are permitted to the application).
 */
IzotBool IzotFilterResponseArrived(
        const IzotResponseAddress* const pAddress,
        const unsigned tag, 
        const IzotByte code,
        const IzotByte* const pData, 
        const unsigned dataLength)
{
    if (izot_filter_response_arrived) {
        return izot_filter_response_arrived(pAddress, tag, code, pData, dataLength);
    } else {
        return FALSE;
    }
}

/*
 * Handles the IzotFilterMsgCompleted event.
 * Parameters:
 *   tag: Used to correlate the event with the message sent
 *          Same as the *tag* specified in the call to IzotSendMsg()
 *   success: TRUE for successful completion, otherwise FALSE
 * Returns:
 *   TRUE if the completion event has been processed by the filter event
 *   handler.
 * Notes:
 *   The IzotFilterMsgCompleted event is signalled when a message transaction
 *   has completed.  See IzotSendMsg().  The LON application can use this event
 *   handler to filter the completion event of a message.  Typically this is
 *   used by the ISI engine to filter the completion notification of ISI
 *   messages.  If the completion event does not get processed by the filter
 *   handler, the message will be passed to the IzotMsgCompleted() handler.
 *
 *   Use IzotFilterMsgCompletedRegistrar() to register a handler for this
 *   event. Without an application-specific handler, this event does nothing
 *   (no completion event is filtered, all are forwarded to the application).
 */
IzotBool IzotFilterMsgCompleted(const unsigned tag, const IzotBool success)
{
    if (izot_filter_msg_completed) {
        return izot_filter_msg_completed(tag, success);
    } else {
        return FALSE;
    }
}

/*****************************************************************
 * Section: Direct Memory File (DMF) Management Function Definitions
 *****************************************************************/
/*
 *  This section defines the prototypes for the LON Stack API callback 
 *  functions supporting direct memory files (DMF) read and write.  This
 *  file contains complete default implementations of these callback
 *  functions. These callback functions use the IzotTranslateWindowArea()
 *  helper function generated by the LON Interface Developer to translate
 *  from the virtual memory address within the LON Transceiver to the 
 *  host memory address. These functions typically do not need to be modified.
 *
 *  Callback functions are called by LON Stack immediately, as needed,
 *  and may be called from any LON task.  The application *must not* call
 *  into the LON Stack API from within a callback.
 */
 
/* 
 * Reads memory in the LON Stack device's memory space.
 * Parameters:
 *   address: Virtual address of the memory to be read
 *   size: Number of bytes to read
 *   pData: Pointer to a buffer to store the requested data
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   LON Stack calls IzotMemoryRead() whenever it receives a network management
 *   memory read request that fits into the registered file access window.
 *   This callback function is used to read data starting at the
 *   specified virtual memory address. This function applies to reading template
 *   files, CP value files, user-defined files, and possibly other data. The
 *   address space for this command is limited to a 64 KB address space.
 */
LonStatusCode IzotMemoryRead(const unsigned address, const unsigned size, void* const pData)
{
#if LON_DMF_ENABLED
    char* pHostAddress = NULL;
    IzotMemoryDriver driver = IzotMemoryDriverUnknown;
    LonStatusCode result = IzotTranslateWindowArea(FALSE, address, size, &pHostAddress, &driver);
 
    if (result == LonStatusNoError) {
        if (driver == IzotMemoryDriverStandard) {
            (void)memcpy(pData, pHostAddress, size);
        } else {
            (void)memcpy(pData, pHostAddress, size);
        }
    }
    return result;
#else
    return LonStatusInvalidOperation;
#endif   /* LON_DMF_ENABLED */
}

/* 
 * Updates memory in the LON Stack device's memory space.
 * Parameters:
 *   address: Virtual address of the memory to be update
 *   size: Number of bytes to write
 *   pData: Pointer to the data to write
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   LON Stack calls IzotMemoryWrite() whenever it receives a network
 *   management memory write request that fits into the registered file access
 *   window. LON Stack uses this callback function to write data at the specified
 *   virtual memory address. This function applies to CP value files, 
 *   user-defined files, and possibly other data. The address space for this
 *   command is limited to a 64 KB address space.  LON Stack automatically
 *   calls the IzotPersistentAppSegmentHasBeenUpdated> function to schedule
 *   an update whenever this callback returns *LonStatusNoError*.
 */
LonStatusCode IzotMemoryWrite(const unsigned address, const unsigned size, const void* const pData)
{
#if LON_DMF_ENABLED
    char* pHostAddress = NULL;
    IzotMemoryDriver driver = IzotMemoryDriverUnknown;
    LonStatusCode result = IzotTranslateWindowArea(TRUE, address, size, &pHostAddress, &driver);
 
    if (result == LonStatusNoError) {
        if (driver == IzotMemoryDriverStandard) {
            (void)memcpy(pHostAddress, pData, size);
        } else {
            (void)memcpy(pHostAddress, pData, size);
        }
    }
    return result;
#else
    return LonStatusInvalidOperation;
#endif  /* LON_DMF_ENABLED */
}

/*
 * Handles the IzotPersistentSegOpenForRead event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to open for read
 * Returns:
 *   The persistent segment type if successful, otherwise IzotPersistentSegUnassigned.
 * Notes:
 *   Calls the registered callback for IzotFlashSegOpenForRead().
 */
IzotPersistentSegType IzotPersistentSegOpenForRead(const IzotPersistentSegType persistentSegType)
{
    if (izot_open_for_read_handler) {
        return izot_open_for_read_handler(persistentSegType);
    } else {
        return IzotPersistentSegUnassigned;
    }
}

/*
 * Handles the IzotPersistentSegOpenForWrite event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to open for write
 *   size: The size of the persistent segment to open for write
 * Returns:
 *   The persistent segment type if successful, otherwise IzotPersistentSegUnassigned.
 * Notes:
 *   Calls the registered callback for IzotFlashSegOpenForWrite().
 */
IzotPersistentSegType IzotPersistentSegOpenForWrite(const IzotPersistentSegType persistentSegType, const size_t size)
{
    if (izot_open_for_write_handler) {
        return izot_open_for_write_handler(persistentSegType, size);
    } else {
        return IzotPersistentSegUnassigned;
    }
}

/*
 * Handles the IzotPersistentSegClose event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to close
 * Returns:
 *   None
 * Notes:
 *   Calls the registered callback for IzotFlashSegClose().
 */
void IzotPersistentSegClose(const IzotPersistentSegType persistentSegType)
{
    if (izot_close_handler) {
        izot_close_handler(persistentSegType);
    }
}

/*
 * Handles the IzotPersistentSegRead event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to read
 *   offset: The offset within the persistent segment to read from
 *   size: The number of bytes to read
 *   pBuffer: Pointer to the buffer to store the read data
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Calls the registered callback for IzotFlashSegRead().
 */
LonStatusCode IzotPersistentSegRead(const IzotPersistentSegType persistentSegType, const size_t offset, const size_t size, 
void * const pBuffer) 
{
    if (izot_read_handler) {
        return izot_read_handler(persistentSegType, offset, size, pBuffer);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*
 * Handles the IzotPersistentSegWrite event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to write
 *   offset: The offset within the persistent segment to write to
 *   size: The number of bytes to write
 *   pData: Pointer to the data to write
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Calls the registered callback for IzotFlashSegWrite().
 */
LonStatusCode IzotPersistentSegWrite(const IzotPersistentSegType persistentSegType, const size_t offset, const size_t size, 
        const void* const pData)
{
    if (izot_write_handler) {
        return izot_write_handler(persistentSegType, offset, size, pData);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*
 * Handles the IzotPersistentSegIsInTransaction event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to check if in transaction
 * Returns:
 *   IzotBool indicating if the persistent segment is in a transaction
 * Notes:
 *   Calls the registered callback for IzotFlashSegIsInTransaction().
 */
IzotBool IzotPersistentSegIsInTransaction(const IzotPersistentSegType persistentSegType)
{
    if (izot_is_in_tx_handler) {
        return izot_is_in_tx_handler(persistentSegType);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*
 * Handles the IzotPersistentSegEnterTransaction event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to enter transaction
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Calls the registered callback for IzotFlashSegEnterTransaction().
 */
LonStatusCode IzotPersistentSegEnterTransaction(const IzotPersistentSegType persistentSegType)
{
    if (izot_enter_tx_handler) {
        return izot_enter_tx_handler(persistentSegType);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*
 * Handles the IzotPersistentSegExitTransaction event.
 * Parameters:
 *   persistentSegType: The type of persistent segment to exit transaction
 * Returns:
 *   LonStatusNoError if no error occurs, otherwise a <LonStatusCode> error code.
 * Notes:
 *   Calls the registered callback for IzotFlashSegExitTransaction().
 */
LonStatusCode IzotPersistentSegExitTransaction(const IzotPersistentSegType persistentSegType)
{
    if (izot_exit_tx_handler) {
        return izot_exit_tx_handler(persistentSegType);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*****************************************************************
 * Section: Event Registrar Function Definitions
 *****************************************************************/
/*
 *  Event handlers for LON Stack are implemented as optional callback functions.
 *  For each of the supported events, an event type is defined in lon_types.h,
 *  and a registrar function is provided. The registrar can register an
 *  application-defined callback function (the "event handler") for a given
 *  event, and it can de-register an event handler when being called with a
 *  NULL pointer.
 *
 *  For example, the IzotWink event is implemented with a function
 *  of type IzotWinkFunction, and registered using the IzotWinkRegistrar
 *  API as shown in this example:
 *
 *  typedef void (*IzotWinkFunction)(void);
 *  extern LonStatusCode IzotWinkRegistrar(IzotWinkFunction handler);
 *
 *  void myWinkHandler(void) {
 *     flash_leds();
 *  }
 *
 *  int main(void) {
 *     ...
 *     // register wink handler:
 *     IzotWinkRegistrar(myWinkHandler);
 *     ...
 *     // un-register wink handler:
 *     IzotWinkRegistrar(NULL);
 *  }
 *
 *  You can use the IzotDeregisterAllCallbacks() API to deregister all event
 *  handlers. It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.
 */
 
/*
 * Registers an IzotGetCurrentDatapointSize() event handler.
 * Parameters:
 *   handler: Pointer to the IzotGetCurrentDatapointSizeFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotGetCurrentDatapointSizeRegistrar(IzotGetCurrentDatapointSizeFunction handler)
{
    if (handler) {
        izot_get_dp_size_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotReset() event handler.
 * Parameters:
 *   handler: Pointer to the IzotResetFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotResetRegistrar(IzotResetFunction handler)
{
    if (handler) {
        izot_reset_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotWink() event handler.
 * Parameters:
 *   handler: Pointer to the IzotWinkFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotWinkRegistrar(IzotWinkFunction handler)
{
    if (handler) {
        izot_wink_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotOffline() event handler.
 * Parameters:
 *   handler: Pointer to the IzotOfflineFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotOfflineRegistrar(IzotOfflineFunction handler)
{
    if (handler) {
        izot_offline_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotOnline() event handler.
 * Parameters:
 *   handler: Pointer to the IzotOnlineFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotOnlineRegistrar(IzotOnlineFunction handler)
{
    if (handler) {
        izot_online_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotDatapointUpdateOccurred() event handler.
 * Parameters:
 *   handler: Pointer to the IzotDatapointUpdateOccurredFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointUpdateOccurredRegistrar(IzotDatapointUpdateOccurredFunction handler)
{
    if (handler) {
        izot_dp_update_occurred_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotDatapointUpdateCompleted() event handler.
 * Parameters:
 *   handler: Pointer to the IzotDatapointUpdateCompletedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointUpdateCompletedRegistrar(IzotDatapointUpdateCompletedFunction handler)
{
    if (handler) {
        izot_dp_update_completed_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotMsgArrived() event handler.
 * Parameters:
 *   handler: Pointer to the IzotMsgArrivedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotMsgArrivedRegistrar(IzotMsgArrivedFunction handler)
{
    if (handler) {
        izot_msg_arrived_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotResponseArrived() event handler.
 * Parameters:
 *   handler: Pointer to the IzotResponseArrivedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotResponseArrivedRegistrar(IzotResponseArrivedFunction handler)
{
    if (handler) {
        izot_response_arrived_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotMsgCompleted() event handler.
 * Parameters:
 *   handler: Pointer to the IzotMsgCompletedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotMsgCompletedRegistrar(IzotMsgCompletedFunction handler)
{
    if (handler) {
        izot_msg_completed_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotMemoryRead() event handler.
 * Parameters:
 *   handler: Pointer to the IzotMemoryReadFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotMemoryReadRegistrar(IzotMemoryReadFunction handler)
{
    if (handler) {
        izot_memory_read_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotMemoryWrite() event handler.
 * Parameters:
 *   handler: Pointer to the IzotMemoryWriteFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotMemoryWriteRegistrar(IzotMemoryWriteFunction handler)
{
    if (handler) {
        izot_memory_write_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotServiceLedStatus() event handler.
 * Parameters:
 *   handler: Pointer to the IzotServiceLedStatusFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotServiceLedStatusRegistrar(IzotServiceLedStatusFunction handler)
{
    if (handler) {
        izot_service_led_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegOpenForRead() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegOpenForReadFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegOpenForReadRegistrar(IzotPersistentSegOpenForReadFunction handler)
{
    if (handler) {
        izot_open_for_read_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegOpenForWrite() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegOpenForWriteFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegOpenForWriteRegistrar(IzotPersistentSegOpenForWriteFunction handler)
{
    if (handler) {
        izot_open_for_write_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegClose() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegCloseFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegCloseRegistrar(IzotPersistentSegCloseFunction handler)
{
    if (handler) {
        izot_close_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegDelete() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegDeleteFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegDeleteRegistrar(IzotPersistentSegDeleteFunction handler)
{
    if (handler) {
        izot_delete_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegRead() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegReadFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegReadRegistrar(IzotPersistentSegReadFunction handler)
{
    if (handler) {
        izot_read_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegWrite() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegWriteFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegWriteRegistrar(IzotPersistentSegWriteFunction handler)
{
    if (handler) {
        izot_write_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegIsInTransaction() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegIsInTransactionFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegIsInTransactionRegistrar(IzotPersistentSegIsInTransactionFunction handler)
{
    if (handler) {
        izot_is_in_tx_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegEnterTransaction() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegEnterTransactionFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegEnterTransactionRegistrar(IzotPersistentSegEnterTransactionFunction handler)
{
    if (handler) {
        izot_enter_tx_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFlashSegExitTransaction() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegExitTransactionFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFlashSegExitTransactionRegistrar(IzotPersistentSegExitTransactionFunction handler)
{
    if (handler) {
        izot_exit_tx_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotPersistentSerializeSegment() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegSerializeFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentSerializeSegmentRegistrar(IzotPersistentSegSerializeFunction handler)
{
    if (handler) {
        izot_serialize_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotPersistentDeserializeSegment() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegDeserializeFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentDeserializeSegmentRegistrar(IzotPersistentSegDeserializeFunction handler)
{
    if (handler) {
        izot_deserialize_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotPersistentGetApplicationSegmentSize() event handler.
 * Parameters:
 *   handler: Pointer to the IzotPersistentSegGetAppSizeFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentGetApplicationSegmentSizeRegistrar(
        IzotPersistentSegGetAppSizeFunction handler)
{
    if (handler) {
        izot_get_app_seg_size_handler = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFilterMsgArrived() event handler.
 * Parameters:
 *   handler: Pointer to the IzotFilterMsgArrivedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFilterMsgArrivedRegistrar(IzotFilterMsgArrivedFunction handler)
{
    if (handler) {
        izot_filter_msg_arrived = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }
}

/*
 * Registers an IzotFilterResponseArrived() event handler.
 * Parameters:
 *   handler: Pointer to the IzotFilterResponseArrivedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFilterResponseArrivedRegistrar(IzotFilterResponseArrivedFunction handler)
{
    if (handler) {
        izot_filter_response_arrived = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }    
}

/*
 * Registers an IzotFilterMsgCompleted() event handler.
 * Parameters:
 *   handler: Pointer to the IzotFilterMsgCompletedFunction to register
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotFilterMsgCompletedRegistrar(IzotFilterMsgCompletedFunction handler)
{
    if (handler) {
        izot_filter_msg_completed = handler;
        return LonStatusNoError;
    } else {
        return LonStatusCallbackNotRegistered;
    }    
}

/*
 * Deregisters all callbacks.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
IZOT_EXTERNAL_FN void IzotDeregisterAllCallbacks(void)
{
    izot_get_dp_size_handler = NULL;
    izot_reset_handler = NULL;
    izot_wink_handler = NULL;
    izot_dp_update_occurred_handler = NULL;
    izot_dp_update_completed_handler = NULL;
    izot_online_handler = NULL;
    izot_offline_handler = NULL;
    izot_msg_completed_handler = NULL;
    izot_msg_arrived_handler = NULL;
    izot_response_arrived_handler = NULL;
    izot_memory_read_handler = NULL;
    izot_memory_write_handler = NULL;
    izot_service_led_handler = NULL;
    izot_open_for_read_handler = NULL;
    izot_open_for_write_handler = NULL;
    izot_close_handler = NULL;
    izot_delete_handler = NULL;
    izot_read_handler = NULL;
    izot_write_handler = NULL;
    izot_is_in_tx_handler = NULL;
    izot_enter_tx_handler = NULL;
    izot_exit_tx_handler = NULL;
    izot_get_app_seg_size_handler = NULL;
    izot_deserialize_handler = NULL;
    izot_serialize_handler = NULL;
}

#ifdef  __cplusplus
}
#endif

