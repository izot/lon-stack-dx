//
// IzotApi.c
//
// Copyright (C) 2022-2025 EnOcean
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
 * Title: LON Stack API
 *
 * Abstract:
 * This file declares prototypes for LON Stack API functions.
 * This file is part of the LON Stack API.
 */

#include "izot/IzotApi.h"

#ifdef  __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------------------
  Section: Globals
  ------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------
  Section: Static
  ------------------------------------------------------------------------------*/
static uint32_t SiDataLength;
// static IzotByte C[8] = {0x85, 0x93, 0x12, 0xFE, 0xDD, 0xD0, 0xDF, 0x55};
// static IzotByte K[6] = {0x57, 0x91, 0xBC, 0x23, 0xF2, 0x01};

/*------------------------------------------------------------------------------
  Section: Function Prototypes
  ------------------------------------------------------------------------------*/
void IzotOnline(void);
void IzotOffline(void);
extern void Encrypt(IzotByte randIn[], APDU *apduIn, IzotUbits16 apduSizeIn, IzotByte *pKey, 
        IzotByte encryptValueOut[], IzotByte isOma, OmaAddress* pOmaDest);

/*
 * *****************************************************************************
 * SECTION: API FUNCTIONS
 * *****************************************************************************
 *
 * This section details the IzoT Device Stack API functions.
 */

 /*
 *  Function: SetSiDataLength
 *  Set the Length of SI data from III generated Length
 *
 *  Parameters:
 *  len   - length of the SI data
 *
 *  Returns:
 *  None.
 *
 */
static void SetSiDataLength(uint32_t len)
{
    SiDataLength = len;
}

 /*
 *  Function: GetSiDataLength
 *  Get the Length of SI data from III generated Length
 *
 *  Parameters:
 *  void
 *
 *  Returns:
 *  <uint32_t>.
 *
 */
uint32_t GetSiDataLength(void)
{
    return SiDataLength;
}

/*
 *  Function: IzotSleep
 *  Sleep for a specified number of ticks.
 *
 *  Parameters:
 *  ticks - The number of ticks to sleep 
 *
 *  Returns:
 *  None.
 *
 *  Remarks:
 *  Suspend the task for the specified number of clock ticks.
 */
IZOT_EXTERNAL_FN void IzotSleep(unsigned int ticks)
{
    OsalSleep(ticks);
}

/*
 *  Event: IzotServiceLedStatus
 *  Occurs when the service pin state changed.
 *
 *  Remarks:
 *
 */
void IzotServiceLedStatus(IzotServiceLedState state, IzotServiceLedPhysicalState physicalState)
{
    if(izot_service_led_handler) {
        izot_service_led_handler(state, physicalState);
    }
}

/*
 * Function: IzotEventPump
 * Process asynchronous IzoT events.
 *
 * Remarks:
 * Call this function periodically after calling IzotStartStack(),
 * IzotRegisterStaticDatapoint() once per static NV, and IzotRegisterMemoryWindow()
 * Call at least once every 10 ms or at the following interval in milliseconds,
 * whichever is less:
 *      Interval = ((InputBufferCount - 1) * 1000) / MaxPacketRate
 * where MaxPacketRate is the maximum number of packets per second arriving
 * for the device and InputBufferCount is the number of input buffers defined
 * for the application.  
 
 * This function processes any events that have been posted by the DX stack.
 * Typically this function is called in response to IzotEventReady(), but
 * must *not* be called directly from that event handler. The IzotEventReady()
 * event handler typically sets an operating system event to schedule the main
 * application task to call the IzotEventPump() function.
 *
*/
IZOT_EXTERNAL_FN LonStatusCode IzotEventPump(void)
{
    LonStatusCode ret = LonStatusNoError;

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

    return ret;
}

/*
 *  Function: IzotGetUniqueId
 *  Gets the registered unique ID (Neuron or MAC ID).
 *
 *  Parameters:
 *  pId   - pointer to the the unique ID
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  The *Unique ID* is a unique 48-bit identifier for a LON device.  
 *  The unique ID may be a LON Neuron ID or an IEEE MAC ID.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotGetUniqueId(IzotUniqueId* const uId)
{
    LonStatusCode ret = LonStatusNoError;
    unsigned char mac[6] = {0};

    if (!IZOT_SUCCESS(HalGetMacAddress(mac))) {
        ret = LonStatusDeviceUniqeIdNotAvailable;
    } else {
        memcpy(uId, &mac[0], 6);
    }
    return ret;
}

/*
 *  Function: IzotGetVersion
 *  Returns the IzoT Device Stack version number.
 *
 *  Parameters:
 *  pMajorVersion - pointer to receive the IzoT Device Stack major version
 *      number.
 *  pMinorVersion - pointer to receive the IzoT Device Stack minor version
 *      number.
 *  pBuildNumber - pointer to receive the IzoT Device Stack build number.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function provides the version of the IzoT Device Stack.  Note that
 *  this function can be called at any time.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotGetVersion(unsigned* const pMajorVersion, unsigned* const pMinorVersion,
unsigned* const pBuildNumber)
{
    LonStatusCode error = LonStatusNoError;
    *pMajorVersion = FIRMWARE_VERSION;
    *pMinorVersion = FIRMWARE_MINOR_VERSION;
    *pBuildNumber = FIRMWARE_BUILD;
    return error;
}

/*
 * Function: IzotPollByIndex
 * Polls a bound, polling, input datapoint. See <IzotPoll> for an alternative.
 *
 * Parameters:
 * index - index of the input datapoint
 *
 * Returns:
 * <LonStatusCode>.
 *
 * Remarks:
 * Call this function to poll an input datapoint. Polling an input datapoint
 * causes the device to solicit the current value of all output datapoints
 * that are bound to this one.
 *
 * The function returns <LonStatusCode> if the API has successfully queued the
 * request. Note that the successful completion of this function does not
 * indicate the successful arrival of the requested values. The values received
 * in response to this poll are reported by one or more of calls to the
 * <IzotDatapointUpdateOccurred> event handler.
 *
 * IzotPoll operates only on input datapoints that have been declared with the
 * *polled* attribute. Only output datapoints that are bound to the input 
 * datapoint will be received.
 *
 * Note that it is *not* an error to poll an unbound polling input datapoint.
 * If this is done, the application will not receive any
 * <IzotDatapointUpdateOccurred> events, but will receive a
 * <IzotDatapointUpdateCompleted> event with the success parameter set to TRUE.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPollByIndex(signed index)
{
    PollNV(index);
    return LonStatusNoError;
}

/*
 * Function: IzotGetDatapointValue
 * Get the value of datapoint by index
 *
 * Parameters:
 * index - index of the datapoint
 *
 * Returns:
 * <void *>.
 *
 * Remarks:
 */
IZOT_EXTERNAL_FN volatile void* IzotGetDatapointValue(const unsigned index)
{
    return NV_ADDRESS(index);
}

/*
 *  Function: IzotPropagateByIndex
 *  Propagates the value of a bound output datapoint to the network.
 *  See <IzotPropagate> for an alternative.
 *
 *  Parameters:
 *  index - the index of the datapoint
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Note that it is not an error to propagate an unbound non-polled output.
 *  If this is done, the IzoT  protocol stack will not send any updates
 *  to the network, but will generate a <IzotDatapointUpdateCompleted> event 
 *  with the success parameter set to TRUE.
 *
 *  If IzotPropagate() returns <LonStatusCode>, 
 *  the <IzotDatapointUpdateCompleted> event will be triggered when the 
 *  datapoint update has successfully completed or failed.
 *
 *  If IzotPropagate() is called multiple times before the datapoint is
 *  sent, the behavior is dependent on whether the datapoint has the
 *  synchronous attribute:
 *
 *    - If the datapoint is declared with the *sync* attribute,
 *      the datapoint will be sent on the network each time
 *      <IzotPropagate> is called (subject to application buffer limits).
 *      The value sent will be the value of the datapoint at the time
 *      of the call to <IzotPropagate>. <IzotDatapointUpdateCompleted> will be
 *      called each time as well.
 *
 *    - If the datapoint is *not* declared with the *sync* attribute,
 *      only the latest value of the datapoint will be sent
 *      out onto the network, and <IzotDatapointUpdateCompleted> will be
 *      called only once. If there are no application buffers available, the
 *      datapoint will be propagated at a later time, when one becomes
 *      available.
 *
 *  Not all Izot stacks support the sync attribute.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPropagateByIndex(signed index)
{
    PropagateNV(index);
    return LonStatusNoError;
}

/*
 *  Function: IzotSendServicePin
 *  Propagates a service pin message.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Use this function to propagate a service pin message to the network.
 *  The function will fail if the device is not yet fully initialized.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotSendServicePin(void)
{
    ManualServiceRequestMessage();
    return LonStatusNoError;
}

/*
 *  Function: IzotSendMsg
 *  Send an application message (not a datapoint message).
 *
 *  Parameters:
 *  tag - message tag for this message
 *  priority - priority attribute of the message
 *  serviceType - service type for use with this message
 *  authenticated - TRUE to use authenticated service
 *  pDestAddr - pointer to destination address
 *  code - message code
 *  pData - message data, may be NULL if length is zero
 *  length - number of valid bytes available through pData
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function is called to send an application message. For application
 *  messages, the message code should be in the range of 0x00..0x2f.  Codes in
 *  the 0x30..0x3f range are reserved for protocols such as file transfer.
 *
 *  If the tag field specifies one of the bindable messages tags
 *  (tag < # bindable message tags), the pDestAddr is ignored (and can be
 *  NULL) because the message is sent using implicit addressing. Otherwise,
 *  explicit addressing is used, and pDestAddr must be provided.
 *
 *  A successful return from this function indicates only that the message has
 *  been queued to be sent.  If this function returns success, the IzoT
 *  API will call <IzotMsgCompleted> with an indication of the
 *  transmission success.
 *
 *  If the message is a request, <IzotResponseArrived> event handlers are
 *  called when corresponding responses arrive.
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
        MsgCompletes(FAILURE, gp->msgOut.tag);
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
 *  Function: IzotSendResponse
 *  Sends a response.
 *
 *  Parameters:
 *  correlator - message correlator, received from <IzotMsgArrived>
 *  code - response message code
 *  pData - pointer to response data, may be NULL if length is zero
 *  length - number of valid response data bytes in pData
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function is called to send an application response.  The correlator
 *  is passed in to <IzotMsgArrived> and must be copied and saved if the 
 *  response is to be sent after returning from that routine.  A response code 
 *  should be in the 0x00..0x2f range.
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
 *  Function: IzotReleaseCorrelator
 *  Release a request correlator without sending a response.
 *
 *  Parameters:
 *  correlator - The correlator, obtained from <IzotMsgArrived>
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function is called to release a correlator obtained from
 *  <IzotMsgArrived> without sending a response.  The application must either
 *  send a response to every message with a service type of request, or release
 *  the correlator, but not both.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotReleaseCorrelator(const IzotCorrelator correlator)
{
    return LonStatusInvalidOperation;
}

/*
 * *****************************************************************************
 * SECTION: EXTENDED API FUNCTIONS
 * *****************************************************************************
 *
 * This section details extended API functions consisting of query functions
 * and update functions. These functions are not typically required, and are
 * intended for use by advanced application developers only.
 */

/*
 *  Function: IzotQueryStatus
 *  Request local status and statistics.
 *
 *  Parameters:
 *  pStatus - pointer to a <IzotStatus> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to obtain the local status and statistics of the IzoT
 *  device. The status will be stored in the <IzotStatus> structure provided.
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
    pStatus->ModelNumber    = MODEL_NUMBER;
    IZOT_SET_UNSIGNED_WORD(pStatus->LostEvents, 12);

    return LonStatusNoError;
}

/*
 *  Function: IzotClearStatus
 *  Clears the status statistics on the IzoT device.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function can be used to clear the IzoT device status and statistics
 *  records.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotClearStatus(void)
{
    memset(&nmp->stats, 0, sizeof(nmp->stats));
    nmp->resetCause = IzotResetCleared;
    eep->errorLog = IzotNoError;
    LCS_WriteNvm();

    return LonStatusNoError;
}

/*
 *  Function: IzotQueryConfigData
 *  Request a copy of local configuration data.
 *
 *  Parameters:
 *  pConfig - pointer to a <IzotConfigData> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to request a copy of device's configuration data.
 *  The configuration is stored in the <IzotConfigData> structure
 *  provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryConfigData(IzotConfigData* const pConfig)
{
    memcpy(pConfig, &eep->configData, sizeof(IzotConfigData));

    return LonStatusNoError;
}

/*
 *  Function: IzotUpdateConfigData
 *  Updates the configuration data on the IzoT device.
 *
 *  Parameters:
 *  pConfig - pointer to <IzotConfigData> configuration data
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to update the device's configuration data based on the
 *  configuration stored in the <IzotConfigData> structure.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateConfigData(const IzotConfigData* const pConfig)
{
    memcpy(&eep->configData, pConfig, sizeof(IzotConfigData));
    RecomputeChecksum();
    LCS_WriteNvm();

    return LonStatusNoError;
}

/*
 *  Function: IzotSetNodeMode
 *  Sets the device's mode and/or state.
 *
 *  Parameters:
 *  mode - mode of the IzoT device, see <IzotNodeMode>
 *  state - state of the IzoT device, see <IzotNodeState>
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Use this function to set the IzoT device's mode and state.
 *  If the mode parameter is *IzotChangeState*, the state parameter may be
 *  set to one of *IzotApplicationUnconfig*, *IzotNoApplicationUnconfig*,
 *  *IzotConfigOffLine* or *IzotConfigOnLine*.  Otherwise the state parameter
 *  should be *IzotStateInvalid* (0).  Note that while the <IzotNodeState>
 *  enumeration is used to report both the state and mode (see <IzotStatus>),
 *  it is *not* possible to change both the state and mode (online/offline) at
 *  the same time.
 *
 *  You can also use the shorthand functions <IzotGoOnline>, <IzotGoOffline>,
 *  <IzotGoConfigured>, and <IzotGoUnconfigured>.
*/
IZOT_EXTERNAL_FN LonStatusCode IzotSetNodeMode(const IzotNodeMode mode, const IzotNodeState state)
{
    switch (mode) {
    case IzotApplicationOffLine: // Go to soft offline state
        if (AppPgmRuns()) {
            IzotOffline(); // Indicate to application program.
        }
        gp->appPgmMode = OFF_LINE;
        break;

    case IzotApplicationOnLine: // Go on-line
        IzotOnline(); // Indicate to application program.
        gp->appPgmMode = ON_LINE;
        break;

    case IzotApplicationReset: // Application reset
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

	case IzotPhysicalReset: /* New Physical Reset Sub Command */
		PhysicalResetRequested();
		break;

    default:
        /* Let us reset the node for this case */
        gp->resetNode = TRUE;
        nmp->resetCause = IzotSoftwareReset;
        break;
    }
    return LonStatusNoError;
}

/*
 *  Function: IzotQueryDomainConfig
 *  Request a copy of a local domain table record.
 *
 *  Parameters:
 *  index - index of requested domain table record (0, 1)
 *  pDomain - pointer to a <IzotDomain> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to request a copy of a local domain table record.
 *  The information is returned through the <IzotDomain> structure provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryDomainConfig(unsigned index, IzotDomain* const pDomain)
{
    IzotDomain *p = (IzotDomain *)AccessDomain(index);
    memcpy(pDomain, p, sizeof(IzotDomain));
    return LonStatusNoError;
}

/*
 *  Function: IzotUpdateDomainConfig
 *  Updates a domain table record on the IzoT device.
 *
 *  Parameters:
 *  index - the index of the domain table to update
 *  pDomain - pointer to the domain table record
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function can be used to update one record of the domain table.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateDomainConfig(unsigned index, const IzotDomain* const pDomain)
{
    IzotByte ret;
    IzotDomain *p = AccessDomain(index);

    ret = UpdateDomain(pDomain, index, true);
    if (ret) {
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
 *  Function: IzotUpdateDomain
 *  Updates a domain table record and changes the LON stack to online and configured.

 *  Parameters:
 *  index - domain index
 *  length - domain ID length (0, 1, 3, or 6)
 *  domainId - domain ID (number of bytes specified by length)
 *  subnet - subnet ID
 *  node - node ID
 *
 *  Returns:
 *  <LonStatusCode>
 */

IZOT_EXTERNAL_FN LonStatusCode IzotUpdateDomain(unsigned index, unsigned length, const IzotByte* domainId, unsigned subnet, unsigned node)
{
	IzotDomain Domain;
    LonStatusCode apiError = LonStatusNoError;

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
        apiError = IzotUpdateDomainConfig(index, &Domain);

        if (index == 0) {
            // Go configured and online for domain index 0
            IzotGoConfigured();
            IzotGoOnline();
        }
	}

	return apiError;
};


/*
 *  Function: IzotQueryAddressConfig
 *  Request a copy of address table configuration data.
 *
 *  Parameters:
 *  index - index of requested address table entry
 *  pAddress - pointer to a <IzotAddress> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to request a copy of the address table configuration 
 *  data. The configuration is stored in the <IzotAddress> structure
 *  provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryAddressConfig(unsigned index, IzotAddress* const pAddress)
{
    IzotAddress *p = (IzotAddress *)AccessAddress(index);
    memcpy(pAddress, p, sizeof(IzotAddress));

    return LonStatusNoError;
}

/*
 *  Function: IzotUpdateAddressConfig
 *  Updates an address table record on the IzoT device.
 *
 *  Parameters:
 *  index - index of the address table to update
 *  pAddress - pointer to address table record
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Use this function to write a record to the local address table.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateAddressConfig(unsigned index, const IzotAddress* const pAddress)
{
    uint32_t oldaddr, newaddr;
    oldaddr = BROADCAST_PREFIX | 0x100 | eep->addrTable[index].Group.Group;
    UpdateAddress((const IzotAddress*)pAddress, index);
        
    // If new update request for this entry is group entry 
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
 *  Function: IzotQueryDpConfig
 *  Request a copy of datapoint configuration data.
 *
 *  Parameters:
 *  index - index of requested datapoint configuration table entry
 *  pDatapointConfig - pointer to a <IzotDatapointEcsConfig> or
 *                     <IzotDatapointConfig> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to request a copy of the local datapoint
 *  configuration data. The configuration is stored in the structure provided.
 *  This API uses a signed index for compatibility with enumerations
 *  of datapoint index values typically used with the application
 *  framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryDpConfig(signed index, IzotDatapointConfig* const pDatapointConfig)
{
    memcpy(pDatapointConfig, &eep->nvConfigTable[index], sizeof(IzotDatapointConfig));

    return LonStatusNoError;
}

/*
 *  Function: IzotUpdateDpConfig
 *  Updates a datapoint configuration table record on the IzoT device.
 *
 *  Parameters:
 *  index - index of datapoint
 *  pDatapointConfig - datapoint configuration
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function can be used to update one record of the datapoint
 *  configuration table.
 *  This API uses a signed index for compatibility with enumerations
 *  of datapoint index values typically used with the application
 *  framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateDpConfig(signed index, const IzotDatapointConfig* const pDatapointConfig)
{
    memcpy(&eep->nvConfigTable[index], pDatapointConfig, sizeof(IzotDatapointConfig));
    RecomputeChecksum();
    LCS_WriteNvm();

    return LonStatusNoError;
}

/*
 * Function: IzotDatapointSetup
 * Sets the static configuration for a datapoint (NV).
 *
 * Parameters:
 *  pDatapointDef - datapoint definition
 *  value - datapoint value
 *  size - datapoint size in bytes
 *  snvtId - standard type index; set to 0 for a non-standard type
 *  arrayCount - number of elements in a datapoint (NV) array; set to 0 for a single value
 *  name - short datapoint (NV) name
 *  sdString - long datapoint (NV) name; may include LonMark self-documentation
 *  maxRate - estimated maximum update rate; set to IZOT_DATAPOINT_RATE_UNKNOWN if unknown
 *  meanRate - estimated average update rate; set to IZOT_DATAPOINT_RATE_UNKNOWN if unknown
 *  ibol - memory address for file-based configuration properties; set to NULL if none
 *  
 * Returns:
 *  <LonStatusCode>
 *
 * Remarks:
 *  This function does not update the datapoint definition flags or the datapoint configuration.
 *  Use IzotDatapointConfiguration() for those.
 */

IZOT_EXTERNAL_FN LonStatusCode IzotDatapointSetup(IzotDatapointDefinition* const pDatapointDef, 
        volatile void const *value, IzotDatapointSize size, uint16_t snvtId, uint16_t arrayCount, 
        const char *name, const char *sdString, uint8_t maxRate, uint8_t meanRate, const uint8_t *ibol) 
{
    LonStatusCode lastError = LonStatusNoError;

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

    return(lastError);
}

/*
 * Function: IzotDatapointFlags
 * Sets the datapoint definition flags for a datapoint (NV).
 * 
 * Parameters:
 *  pDatapointDef - pointer to datapoint definition (includes Flags field)
 *  pDatapointConfig - pointer to datapoint configuration
 *  priority - set to TRUE for a priority datapoint (NV)
 *  direction - input or output
 *  isProperty - set to TRUE for property datapoints (configuration properties)
 *  persistent - set to TRUE for persistent datapoints
 *  changeable - set to TRUE for changeable type datapoints
 *  authenticated - set to TRUE for authenticated transactions
 * 
 * Returns:
 *  <LonStatusCode>
 *
 * Remarks:
 *  This function only updates the datapoint definition flags.
 *  Use IzotDatapointSetup() for setting datapoint definition fields not included
 *  in the flags.
 */


IZOT_EXTERNAL_FN LonStatusCode IzotDatapointFlags(IzotDatapointDefinition* const pDatapointDef,
        IzotBool priority, IzotDatapointDirection direction, IzotBool isProperty, IzotBool persistent, IzotBool changeable, IzotBool authenticated) 
{
    LonStatusCode lastError = LonStatusNoError;
    UInt16 flags = pDatapointDef->Flags;

    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_PRIORITY) | (priority ? IZOT_DATAPOINT_PRIORITY : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_IS_OUTPUT) 
            | ((direction == IzotDatapointDirectionIsOutput) ? IZOT_DATAPOINT_IS_OUTPUT : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_CONFIG_CLASS) | (isProperty ? IZOT_DATAPOINT_CONFIG_CLASS : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_PERSISTENT) | (persistent ? IZOT_DATAPOINT_PERSISTENT : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_CHANGEABLE) | (changeable ? IZOT_DATAPOINT_CHANGEABLE : 0);
    pDatapointDef->Flags = (flags & ~IZOT_DATAPOINT_AUTHENTICATED) | (authenticated ? IZOT_DATAPOINT_AUTHENTICATED : 0);

    return(lastError);
}


/*
 * Function: IzotDatapointBind
 *  Binds a datapoint (NV).  Binding is the process of creating a connection to or from a datapoint
 *  from or to one or more datapoints.
 *
 * Parameters:
 *  nvIndex - datapoint (NV) index (index into the NV configuration table)
 *  address - address table index (index into the address table)
 *  selector - NV selector
 *  turnaround - turnaroud flag; set to TRUE if source or destination is on same device
 *  service - delivery service
 *  
 * Returns:
 *  <LonStatusCode>.
 *
 * Remarks:
 *  This function only updates the datapoint connection information.  Use IzotDatapointSetup() and
 *  IzotDatapointFlags() for setting other datapoint configuration.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotDatapointBind(int nvIndex, IzotByte address, IzotUbits16 selector, 
        IzotBool turnAround, IzotServiceType service)
{
    LonStatusCode lastError = LonStatusNoError;
    IzotDatapointConfig DatapointConfig;

    lastError = IzotQueryDpConfig(nvIndex, &DatapointConfig);

    if (lastError != LonStatusNoError) {
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_ADDRESS_HIGH, address >> 4);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_ADDRESS_LOW, address);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_SELHIGH, high_byte(selector));
        DatapointConfig.SelectorLow = low_byte(selector);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_TURNAROUND, turnAround);
        IZOT_SET_ATTRIBUTE_P(&DatapointConfig, IZOT_DATAPOINT_SERVICE, service);
        lastError = IzotUpdateDpConfig(nvIndex, &DatapointConfig);
    }

    return(lastError);
}

/*
 *  Function: IzotQueryAliasConfig
 *  Request a copy of alias configuration data.
 *
 *  Parameters:
 *  index - index of requested alias configuration table entry
 *  pAlias - pointer to a <IzotAliasEcsConfig> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to request a copy of the alias configuration data.
 *  The configuration is stored in the <IzotAliasEcsConfig> structure
 *  provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryAliasConfig(unsigned index, IzotAliasConfig* const pAlias)
{
    memcpy(pAlias, &eep->nvAliasTable[index], sizeof(IzotAliasConfig));
    return LonStatusNoError;
}

/*
 *  Function: IzotUpdateAliasConfig
 *  Updates an alias table record on the IzoT device.
 *
 *  Parameters:
 *  index - index of alias table record to update
 *  pAlias - pointer to the alias table record
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function writes a record in the local alias table.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotUpdateAliasConfig(unsigned index, const IzotAliasConfig* const pAlias)
{
    memcpy(&eep->nvAliasTable[index], pAlias, sizeof(IzotAliasConfig));
    RecomputeChecksum();
    LCS_WriteNvm();
    return LonStatusNoError;
}

/*
 *  Function: IzotDatapointIsBoundByIndex
 *  Determines whether a datapoint is bound given its index.
 *  See <IzotDatapointIsBound> for an alternative API.
 *
 *  Parameters:
 *  index - index of the datapoint
 *  pIsBound - pointer to receive the "is-bound" attribute
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to determine whether a datapoint is bound.
 *  A datapoint is bound if it, or any of its aliases, has a bound
 *  selector or an address table entry. The unbound selector for a given
 *  datapoint is equal to (0x3fff - datapoint index).  A datapoint
 *  or alias has an address if the address index is not equal to 0xffff.
 *  This API uses a signed index for compatibility with enumerations
 *  of datapoint index values typically used with the application
 *  framework, because C language enumerations are signed integers.
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
 *  Function: IzotMtIsBound
 *  Determines whether a message tag is bound.
 *
 *  Parameters:
 *  tag - the message tag
 *  pIsBound - pointer to receive the "is-bound" attribute
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Call this function to determine whether a message tag is bound.
 *  A message tag is bound if the associated address type is anything other
 *  than *IzotAddressUnassigned*.
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

/*
 * *****************************************************************************
 * SECTION: persistent data API FUNCTIONS
 * *****************************************************************************
 *
 *  This section details the API functions that support persistent data (non-
 *  volatile data).
 *
 *  Remarks:
 *  Persistent data is stored in data segments, identified by
 *  <IzotPersistentDataSegmentType>, and are used to store IzoT persistent
 *  configuration data.
 */

/*
 *  Function: IzotPersistentAppSegmentHasBeenUpdated
 *  Informs the IzoT protocol stack that the application data segment has been
 *  updated.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Use this function to inform the IzoT protocol stack that some application
 *  data been updated that should be written out to the
 *  <IzotPersistentSegApplicationData> persistent data segment.  The IzoT
 *  protocol stack will schedule a write to the
 *  <IzotPersistentSegApplicationData> segment after the flush timeout
 *  has expired.
 *
 *  It is generally not necessary to call this function when application data
 *  has been updated by a network management write command or a datapoint 
 *  update, because the IzoT  protocol stack automatically calls this function 
 *  whenever the <IzotMemoryWrite> event handler returns <LonStatusCode>, and 
 *  whenever a datapoint update is received for a datapoint with the
 *  *IZOT_DATAPOINT_CONFIG_CLASS* or *IZOT_DATAPOINT_PERSISTENT* attribute.
 *  However, the application must call this function whenever it updates
 *  application-specific persistent data directly.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentAppSegmentHasBeenUpdated(void)
{
    IzotPersistentMemStartCommitTimer();
    return LonStatusNoError;
}

/*
 *  Function: IzotPersistentFlushData
 *  Flush all persistent data out to persistent storage.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function can be called by the application task to block until all
 *  persistent data writes have been completed.  The application might do
 *  this, for example, in response to a <IzotPersistentStarvation> event.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotPersistentFlushData(void)
{
    IzotPersistentMemSetCommitFlag();
    return LonStatusNoError;
}

/*
 *  Function: IzotGetAppSegmentSize
 *  Get the application segment size.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  
 */
unsigned IzotGetAppSegmentSize(void)
{
	if(izot_get_app_seg_size_handler) {
		return izot_get_app_seg_size_handler();
	} else {
		return 0;
    }
}

/*
 *  Function: IzotPersistentSegGetMaxSize
 *  Gets the number of bytes required to store persistent data.
 *
 *  Parameters:
 *  persistentSegType - The segment type, see <IzotPersistentSegType>
 *
 *  Returns:
 *  The number of bytes required to store persistent data for the specified
 *  segment.
 *
 *  Remarks:
 *  This function will not typically be called directly by the application,
 *  but may be used by persistent data event handlers (implemented by the
 *  application) to reserve space for persistent data segments.
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
 * Function: Unlock_Device
 *
 * Secures the RFOLC against unauthorized use
 * Uses digest to secure the device
 */
#if LINK_IS(WIFI)
#define FLICKER_INTERVAL 200    // LED flicker interval in milliseconds

static void UnlockDevice(void)
{
	uint8_t temp_y[8] = {0};
	uint8_t digestKeyFlash[8] = {0};
	uint8_t macAddress[6] = {0};            // Array to set MAC address
    LonTimer flickr_timer;
    
    // Get Mac and Digest from SPI
	HalGetMacAddress(macAddress);
    
    // Read the digest from flash
    iflash_drv_read(NULL, digestKeyFlash, sizeof(digestKeyFlash), 0xFF8);
    
    // Calculate the digest based on Mac Address
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

/*
 * *****************************************************************************
 * SECTION: STACK LIFETIME MANAGEMENT FUNCTIONS
 * *****************************************************************************
 */

/*
 * Function: IzotCreateStack
 * Initialize the IzoT Device Stack.
 *
 * Parameters:
 * pInterface - Pointer to a <IzotStackInterfaceData> structure, which defines
 *   the static attributes of the program
 * pControlData - Pointer to a <IzotControlData> structure, which contains data
 *   used to control the runtime aspects of the stack.  These aspects can be
 *   set by the application at runtime.
 *
 * Returns:
 * <LonStatusCode>.
 *
 *  Remarks:
 *  Initializes and configures the IzoT driver and the IzoT  protocol
 *  stack. This must be the first call into the IzoT Device Stack API, and 
 *  cannot be called again until <IzotDestroyStack> has been called.  After this
 *  function has been called, the following functions can be called:
 *  <IzotRegisterStaticDatapoint>, <IzotRegisterMemoryWindow>,
 *  <IzotStartStack>, and <IzotDestroyStack>.
 *
 *  Note that the stack expects reasonable values for all of the initialization
 *  parameters, since typically these will be created and validated by the
 *  tool which generates the application framework. Therefore, the stack does
 *  not provide detailed error information when a parameter is out of range.
 *
 *  If <IzotCreateStack> returns any error, the stack will simply not function.
 *  It will not send or receive  messages over the network.  The service LED
 *  will be left on (applicationless).
 *
 *  A typical application framework calls this function within a general
 *  initialization function (often called IzotInit()).
 */
IZOT_EXTERNAL_FN LonStatusCode IzotCreateStack(const IzotStackInterfaceData* const pInterface, 
const IzotControlData * const pControlData)
{
    LonStatusCode err = LonStatusNoError;
#if LINK_IS(WIFI)
    char oldProgId[8];
#endif 

    // Only a few of these fields are used by the LON stack.  The
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

    // Start the IP stack if enabled and initialize a UDP socket for communication
    err = UdpInit();
    if (err != LonStatusNoError) {
        return err;
    }

#if LINK_IS(WIFI)
    UnlockDevice();
    if (InitEEPROM(pInterface->Signature) != SUCCESS || APPInit() != SUCCESS) {
        err = LonStatusStackInitializationFailure;
    }

    psm_register_module(IZOT_MOD_NAME, "common_part", 1); // Set up psm module if it does not exist
    psm_get_single(IZOT_MOD_NAME, "progId", oldProgId, 10); // get old program ID
    psm_set_single(IZOT_MOD_NAME, "progId", cp->progId); // set new program ID
    
    if (strcmp(oldProgId, cp->progId) != 0) {
        // set first run to true if program IDs are not equal
        psm_set_single(IZOT_MOD_NAME, "first_run", "y");
    } else {
        psm_set_single(IZOT_MOD_NAME, "first_run", "n");
    }
#endif  // LINK_IS(WIFI)

    return err;
}

/*
 * Function: IzotRegisterStaticDatapoint
 * Registers a static datapoint with the IzoT Device Stack.
 *
 * Parameters:
 *  pDatapointDef - pointer to a <IzotDatapointDefinition> structure
 *
 * Returns:
 *  <LonStatusCode>.
 *
 * Remarks:
 *  This function registers a static datapoint with the IzoT Device Stack API,
 *  and is called once for each static datapoint.  This function can be
 *  called only after <IzotCreateStack>, but before <IzotStartStack>.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotRegisterStaticDatapoint(IzotDatapointDefinition* const pDatapointDef) {
    LonStatusCode err = LonStatusNoError;
    NVDefinition d;
    IzotBits16 returnValue;

    UInt16 flags = pDatapointDef->Flags;
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
        err = LonStatusInvalidParameter;
    } else {
        pDatapointDef->NvIndex = returnValue;
    }

    return err;
}

/*
 *  Function: IzotRegisterMemoryWindow
 *  Register a virtual memory address range and enable DMF.
 *
 *  Parameters:
 *  windowAddress - the starting virtual address of the window
 *  windowSize - the size of the window, in bytes
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  This function is used to open up a window in the device's memory
 *  space. IzoT protocol messages that access memory using absolute addressing
 *  within the 64kB legacy address range, provide the memory addressed falls
 *  within the registered window, can access memory located within the IzoT
 *  application through <IzotMemoryRead> and <IzotMemoryWrite> synchronous
 *  events.
 *
 *  This function can only be called after <IzotCreateStack>, but before
 *  <IzotStartStack>.  The address space for these memory windows is between
 *  0x0001 and 0xffff, but some protocol stacks may further limit the supported
 *  address range.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotRegisterMemoryWindow(const unsigned windowAddress, const unsigned windowSize)
{
    LonStatusCode err = LonStatusNoError;
    setMem(windowAddress, windowSize);
    return err;
}

/*
 *  Function: IzotStartStack
 *  Completes the initialization of the IzoT  protocol stack.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Starts running the stack, following successful calls to <IzotCreatStack>,
 *  <IzotRegisterStaticDatapoint>, <IzotRegisterMemoryWindow> or other
 *  initialization-time functions.
 *
 *  When the IzotStartStack() function returns with success, the device stack
 *  is fully operational and all persistent data (if any) has been applied.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotStartStack(void)
{
    // Load persistent NVs from NVM
    if (IzotGetAppSegmentSize() != LonStatusNoError) {
        if (LCS_ReadNvs() != LonStatusNoError) {
            ErasePersistenceData();
            ErasePersistenceConfig();
            DBG_vPrintf(TRUE, "No Application Data found.Put the Device into Unconfigured mode\r\n");
            IzotPersistentSegSetCommitFlag(IzotPersistentSegApplicationData);
            IzotPersistentAppSegmentHasBeenUpdated();
            IzotSetNodeMode(IzotChangeState, IzotApplicationUnconfig);
        }
    }
    return LonStatusNoError;
}

/*
 *  Function: IzotDestroyStack
 *  Stops the IzoT  protocol stack and frees all memory that it has
 *  allocated.
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Waits for persistent writes to complete, stops the stack, and frees all
 *  temporary memory created during execution of the stack.  The service LED is
 *  lit to indicate that the device is applicationless.
 *
 *  Not all targets support application shut-down.
 */
IZOT_EXTERNAL_FN void IzotDestroyStack(void)
{
    // Do nothing for the IZOT DX DS
}

/*
 *  Function: IzotQueryReadOnlyData
 *  Request copy of local read-only data.
 *
 *  Parameters:
 *  pReadOnlyData - pointer to a <IzotReadOnlyData> structure
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Call this function to request a copy of device's read-only data.
 *  The read-only data will be stored in the <IzotReadOnlyData> structure
 *  provided.
 */
IZOT_EXTERNAL_FN LonStatusCode IzotQueryReadOnlyData(IzotReadOnlyData* const pReadOnlyData)
{
    memcpy(pReadOnlyData, (IzotReadOnlyData*)&eep->readOnlyData, 
    sizeof(IzotReadOnlyData));
    return LonStatusNoError;
}

/*
 *  Function: IzotGetAppSignature
 *  Gets the application's signature.
 *  This API is available with Izot Device Stack EX.
 *
 *  Returns:
 *  The application signature which was specified by the application
 *  when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetAppSignature()
{
    return GetAppSignature();
}

/*
 *  Function: IzotGetAliasCount
 *  Gets the number of aliases supported by the alias table.
 *  This API is available with Izot Device Stack EX.
 *
 *  Returns:
 *  The size of the alias table which is specified by the application
 *  when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetAliasCount()
{
    return eep->readOnlyData.AliasCount;
}

/*
 *  Function: IzotGetAddressTableCount
 *  Gets the number of addresses supported by the address table.
 *  This API is available with Izot Device Stack EX.
 *
 *  Returns:
 *  The size of the address table which is specified by the application
 *  when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetAddressTableCount()
{
    return eep->readOnlyData.Extended;
}

/*
 *  Function: IzotGetStaticDatapointCount
 *  Gets the number of static datapoints supported by the device.
 *  This API is available with Izot Device Stack EX.
 *
 *  Returns:
 *  The number of static datapoint specified by the application
 *  when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetStaticDatapointCount()
{
    return eep->readOnlyData.DatapointCount;
}

/*
 *  Function: IzotGetDidFromLocalAddress
 *  Get the Domain id from Local IP address.
 *
 *  Parameters:
 *  pDid - Pointer to domain Id buffer
 *  pDidLen - Pointer to domain Id length
 *  pSub - Pointer to the subnet Id
 *  pNode - Pointer to the Node Id
 *
 *  Returns:
 *  <LonStatusCode>.
 *
 *  Remarks:
 *  Get the Domain id from Local IP address
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
 *  Function: IzotIsFirstRun
 *  Determine whether or not an application is running for the first time.
 *
 *  Returns:
 *  True if this is the first time the application is running with the same
 *  setup and configuration.
 *  You can depend on this information if you need to preset certain
 *  values only when the first time the applicaiton is running.
 */
IZOT_EXTERNAL_FN IzotBool IzotIsFirstRun(void)
{
    int result = FALSE;
 
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


/*
 * *****************************************************************************
 * SECTION: CALLBACK PROTOTYPES
 * *****************************************************************************
 *
 *  This section defines the prototypes for the Izot Device Stack API callback
 *  functions.
 *
 *  Remarks:
 *  Callback functions are called by the IZOT protocol stack
 *  immediately, as needed, and may be called from any IZOT task.  The
 *  application *must not* call into the Izot Device Stack API from within 
 *  a callback.
 */

/*
 *  Callback: IzotGetCurrentDatapointSize
 *  Gets the current size of a datapoint.
 *
 *  Parameters:
 *  index - the local index of the datapoint
 *
 *  Returns:
 *  Current size of the datapoint. Zero if the index is invalid.
 *
 *  Remarks:
 *  If the datapoint size is fixed, this function should return
 *  <IzotGetDeclaredDpSize>. If the datapoint size is changeable, the
 *  current size should be returned. The default implementation for changeable
 *  type datapoints returns 0, and must be updated by the application
 *  developer.
 *
 *  The IZOT protocol stack will not propagate a datapoint with
 *  size 0, nor will it generate an update event if a datapoint update
 *  is received from the network when the current datapoint size is 0.
 *
 *  Note that even though this is a callback function, it *is* legal for the
 *  application to call <IzotGetDeclaredDpSize> from this callback.
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

/*
 * *****************************************************************************
 * SECTION: EVENT HANDLER PROTOTYPES
 * *****************************************************************************
 *
 *  This section defines the prototypes for the Izot Device Stack API event 
 *  handler functions.
 *
 *  Remarks:
 *  Like callback functions, event handlers are called from the Izot Device 
 *  Stack API. However, unlike callback functions, event handlers are only 
 *  called in the context of the application task, and only when the 
 *  application calls the <IzotEventPump> function. Also, the application may 
 *  make Izot Device Stack API function calls from within an event handler.
 */

/*
 *  Event: IzotReset
 *  Occurs when the IZOT protocol stack has been reset.
 *
 *  Parameters:
 *  pResetNotification - <IzotResetNotification> structure with capabilities
 *      and identifying data
 *
 *  Remarks:
 *  The pointer to <IzotResetNotification> is provided for call compatibility 
 *  with the ShortStack LonTalk Compact API.  For IZOT, the value of
 *  pResetNotification is always NULL.
 *
 *  Whenever the IZOT device has been reset, the mode of the device is changed
 *  to *online*, but no IzotOnline() event is generated.
 *
 *  Note that resetting the IZOT device only affects the IZOT protocol
 *  stack and does not cause a processor or application software reset.
 */
void IzotReset(const IzotResetNotification* const pResetNotification)
{
    if(izot_reset_handler) {
		izot_reset_handler();
    }
}

/*
 *  Event: IzotWink
 *  Occurs when the IZOT device receives a WINK command.
 *
 *  Remarks:
 *  This event is not triggered when wink sub-commands (extended install
 *  commands) are received.
 */
void IzotWink(void)
{
	if(izot_wink_handler) {
		izot_wink_handler();
    }
}

/*
 *  Event: IzotOffline
 *  Occurs when the IZOT device has entered the offline state.
 *
 *  Remarks:
 *  While the device is offline, the IZOT protocol stack will not
 *  generate datapoint updates, and will return an error when <
 *  IzotPropagateDp> is called.
 *
 *  Note that offline processing in IZOT differs from that in ShortStack.
 *  When a ShortStack Micro Server receives a request to go offline, it sends
 *  the request to the ShortStack LonTalk Compact API, which calls the
 *  application callback <IzotOffline>.  The Micro Server does not actually go
 *  offline until the <IzotOffline> callback function returns and the
 *  ShortStack LonTalk Compact API sends a confirmation message to the Micro
 *  Server.  In contrast, the IZOT device goes offline as soon as it receives
 *  the offline request. The <IzotOffline> event is handled asynchronously.
 */
void IzotOffline(void)
{
	if(izot_offline_handler) {
		izot_offline_handler();
    }
}

/*
 *  Event: IzotOnline
 *  Occurs when the IZOT device has entered the online state.
 */
void IzotOnline(void)
{
    if(izot_online_handler) {
		izot_online_handler();
    }
}

/*
 *  Event: IzotServicePinPressed
 *  Occurs when the service pin has been activated.
 *
 *  Remarks:
 *  Note that the IZOT protocol stack sends a service pin message
 *  automatically any time the service pin has been pressed.
 */
void IzotServicePinPressed(void)
{
    ManualServiceRequestMessage();
}

/*
 *  Event: IzotServicePinHeld
 *  Occurs when the service pin has been continuously activated for a
 *  configurable time.
 *
 *  Remarks:
 *  Use the LonTalk Interface Developer to enable this feature and to specify
 *  the duration for which the service pin must be activated to trigger this
 *  event handler.
 */
void IzotServicePinHeld(void)
{
    //REMINDER: CPM4200??
}

/*
 *  Event: IzotDatapointUpdateOccurred
 *  Occurs when new input datapoint data has arrived.
 *
 *  Parameters:
 *  index - global index (local to the device) of the datapoint in question
 *  pSourceAddress - pointer to source address description
 *
 *  Remarks:
 *  The datapoint with local index given in this event handler has been
 *  updated with a new value. The new value is already stored in the
 *  datapoint's location; access the value through the global variable
 *  representing the datapoint, or obtain the pointer to the datapoint's value 
 *  from the <IzotGetDatapointValue> function. The pSourceAddress
 *  pointer is only valid for the duration of this event handler.
 *
 *  For an element of a datapoint array, the index is the global datapoint
 *  index plus the array-element index. For example, if nviVolt[0] has
 *  global datapoint index 4, then nviVolt[1] has global datapoint
 *  index 5.
 */
void IzotDatapointUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
	if(izot_dp_update_occurred_handler) {
		izot_dp_update_occurred_handler(index, pSourceAddress);
    }
}

/*
 *  Event:   IzotDatapointUpdateCompleted
 *  Signals completion of a datapoint update.
 *
 *  Parameters:
 *  index - global index (local to the device) of the datapoint that
 *          was updated
 *  success - indicates whether the update was successful or unsuccessful
 *
 *  Remarks:
 *  This event handler signals the completion of a datapoint update or
 *  poll transaction (see <IzotPropagateDp> and <IzotPollDp>). For
 *  unacknowledged or repeated messages, the transaction is complete when the
 *  message has been sent with the configured number of retries. For
 *  acknowledged messages, it is successfully complete when the IZOT
 *  protocol stack receives an acknowledgement from each of the destination
 *  devices, and is unsuccessfully complete if not all acknowledgements are
 *  received.  Poll requests always use the request service type, and
 *  generates a successful completion if responses are received from all
 *  expected devices, and generates a failed completion otherwise.
 */
void IzotDatapointUpdateCompleted(const unsigned index, const IzotBool success)
{
	if(izot_dp_update_completed_handler) {
		izot_dp_update_completed_handler(index, success);
    }
}

/*
 *  Event:   IzotDatapointdded
 *  A dynamic datapoint has been added.
 *
 *  Parameters:
 *  index - The index of the dynamic datapoint that has been added
 *  pDpDef - Pointer to the datapoint definition <IzotDatapointDefinition>
 *
 *  Remarks:
 *  The IZOT protocol stack calls this function when a dynamic datapoint
 *  has been added.  During device startup, this function is called
 *  for each dynamic datapoint that had been previously defined.  The
 *  pDpDef pointer, along with all of its contents, is invalid when the
 *  function returns.  If the application needs to maintain this information,
 *  it must copy the data, taking care to avoid copying the pointers contained
 *  within <IzotDatapointDefinition>.
 *
 *  Note that when a datapoint is first added, the name and the
 *  self-documentation string may be blank.  A network manager may update the
 *  name or the self-documentation string in a subsequent message, and the
 *  IZOT protocol stack will call the <IzotDpTypeChanged> event handler.
 */
void IzotDatapointdded(const unsigned index, const IzotDatapointDefinition* const pDpDef)
{
    //REMINDER: Not supported in DX
}

/*
 *  Event:   IzotDatapointTypeChanged
 *  One or more attributes of a dynamic datapoint has been changed.
 *
 *  Parameters:
 *  index - The index of the dynamic datapoint that has been changed
 *  pDpDef - Pointer to the datapoint definition <IzotDatapointDefinition>
 *
 *  Remarks:
 *  The IZOT protocol stack calls this function when a dynamic datapoint
 *  definition been changed.  The pDpDef pointer, along with all of
 *  its contents, is invalid when the function returns.  If the application
 *  needs to maintain this information, it must copy the data, taking care to
 *  avoid copying the pointers contained within <IzotDatapointDefinition>.
 */
void IzotDatapointTypeChanged(const unsigned index, const IzotDatapointDefinition* const pDpDef)
{
    //REMINDER: Not supported in DX
}

/*
 *  Event:   IzotDatapointDeleted
 *  A dynamic datapoint has been deleted.
 *
 *  Parameters:
 *  index - The index of the dynamic datapoint that has been deleted
 *
 *  Remarks:
 *  The IZOT protocol stack calls this function when a dynamic datapoint
 *  has been deleted.
 */
void IzotDatapointDeleted(const unsigned index)
{
    //REMINDER: Not supported in DX
}

/*
 *  Event: IzotMsgArrived
 *  Occurs when an application message arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <IzotReceiveAddress>)
 *  correlator - correlator to be used with <IzotSendResponse>
 *  authenticated - TRUE if the message was (successfully) authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Remarks:
 *  This event handler reports the arrival of a message that is neither a 
 *  datapoint message or a non-Dp message that is otherwise processed by the
 *  IZOT device (such as a network management command). Typically, this is used
 *  with application message codes in the value range indicated by the
 *  <IzotApplicationMessageCode> enumeration. All pointers are only valid for
 *  the duration of this event handler.
 *
 *  If the message is a request message, then the function must deliver a
 *  response using <IzotSendResponse> passing the provided *correlator*.
 *  Alternatively, if for any reason the application chooses not to respond to
 *  a request, it must explicitly release the correlator by calling
 *  <IzotReleaseCorrelator>.
 *
 *  Application messages are always delivered to the application, regardless
 *  of whether the message passed authentication or not. It is up to the
 *  application to decide whether authentication is required for any given
 *  message and compare that fact with the authenticated flag. The
 *  authenticated flag is clear (FALSE) for non-authenticated messages and for
 *  authenticated messages that do not pass authentication. The authenticated
 *  flag is set only for correctly authenticated messages.
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
 *  Event: IzotResponseArrived
 *  Occurs when a response arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address used for response (see
 *             <IzotResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *          dataLength - number of bytes available through pData.
 *
 *  Remarks:
 *  This event handler is called when a response arrives.  Responses may be
 *  sent by other devices when the IZOT device sends a message using
 *  <IzotSendMsg> with a <IzotServiceType> of *IzotServiceRequest*.
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
 *  Event: IzotMsgCompleted
 *  Occurs when a message transaction has completed.  See <IzotSendMsg>.
 *
 *  Parameters:
 *  tag - used to correlate the event with the message sent
 *        Same as the *tag* specified in the call to <IzotSendMsg>
 *  success - TRUE for successful completion, otherwise FALSE
 *
 *  Remarks:
 *  For unacknowledged or repeated messages, the transaction is complete when
 *  the message has been sent with the configured number of retries. For
 *  acknowledged messages, the IZOT protocol stack calls
 *  <IzotMsgCompleted> with *success* set to TRUE after receiving
 *  acknowledgments from all of the destination devices, and calls
 *  <IzotMsgCompleted> with *success* set to FALSE if the transaction timeout
 *  period expires before receiving acknowledgements  from all destinations.
 *  Likewise, for request messages, the transaction is considered successful
 *  when the IZOT protocol stack receives a response from each of the
 *  destination devices, and unsuccessful if the transaction timeout expires
 *  before responses have been received from all destination devices.
 */
void IzotMsgCompleted(const unsigned tag, const IzotBool success)
{
    if (izot_msg_completed_handler) {
        izot_msg_completed_handler(tag, success);
    }
}

/*
 *  Event: IzotFilterMsgArrived
 *  Occurs when an application message arrives.
 *  This API is available with Izot Device Stack EX.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <IzotReceiveAddress>)
 *  correlator - correlator to be used with <IzotSendResponse>
 *  authenticated - TRUE if the message was (successfully) authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Returns:
 *  TRUE if the message has been processed by the filter event handler.
 *
 *  Remarks:
 *  This event handler filters the arrival of a message.  Typically this
 *  is used by the ISI engine to filter ISI messages.  If the message
 *  does not get processed by the filter handler, the message will be
 *  passed to the <IzotMsgArrived> handler.
 *
 *  Use <IzotFilterMsgArrivedRegistrar> to register a handler for this event.
 *  Without an application-specific handler, this event does nothing (no
 *  incoming messages are filtered, all are permitted to the application).
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
 *  Event: IzotFilterResponseArrived
 *  Occurs when a response arrives.
 *  This API is available with Izot Device Stack EX.
 *
 *  pAddress - source and destination address used for response (see
 *             <IzotResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *  dataLength - number of bytes available through pData.
 *
 *  Returns:
 *  TRUE if the response has been processed by the filter event handler.
 *
 *  Remarks:
 *  This event handler filters incoming response messages. Responses may be
 *  sent by other devices when the IzoT device sends a message using
 *  <IzotSendMsg> with a <IzotServiceType> of *IzotServiceRequest*.
 *
 *  Use <IzotFilterResponseArrivedRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing
 *  (no incoming response is filtered, all are permitted to the application).
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
 *  Event: IzotFilterMsgCompleted
 *  Occurs when a message transaction has completed.  See <IzotSendMsg>.
 *  This API is available with Izot Device Stack EX.
 *
 *  Parameters:
 *  tag - used to correlate the event with the message sent
 *        Same as the *tag* specified in the call to <IzotSendMsg>
 *  success - TRUE for successful completion, otherwise FALSE
 *
 *  Returns:
 *  TRUE if the completion event has been processed by the filter event
 *  handler.
 *
 *  Remarks:
 *  This event handler filters the completion event of a message.
 *  Typically this is used by the ISI engine to filter the completion
 *  notification of ISI messages.  If the completion event does not get
 *  processed by the filter handler, the message will be passed to the
 *  <IzotMsgCompleted> handler.
 *
 *  Use <IzotFilterMsgCompletedRegistrar> to register a handler for this
 *  event. Without an application-specific handler, this event does nothing
 *  (no completion event is filtered, all are permitted to the application).
 */
IzotBool IzotFilterMsgCompleted(const unsigned tag, const IzotBool success)
{
    if (izot_filter_msg_completed) {
        return izot_filter_msg_completed(tag, success);
    } else {
        return FALSE;
    }
}

/*
 * *****************************************************************************
 * SECTION: DMF CALLBACK PROTOTYPES
 * *****************************************************************************
 *
 *  This section defines the prototypes for the Izot Device Stack API callback 
 *  functions supporting direct memory files (DMF) read and write.  This file
 *  contains complete default implementations of these callback functions. These 
 *  callback functions use a helper function, IzotTranslateWindowArea(), that is 
 *  generated by the LonTalk Interface Developer to translate from the virtual 
 *  memory address within the IZOT Transceiver to the host memory address.  
 *  These functions typically do not need to be modified.
 *
 *  Remarks:
 *  Callback functions are called by the IZOT protocol stack 
 *  immediately, as needed, and may be called from any IZOT task.  The 
 *  application *must not* call into the Izot Device Stack API from within 
 *  a callback.
 */
 
/* 
 *  Callback: IzotMemoryRead
 *  Read memory in the IZOT device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be read
 *  size - number of bytes to read
 *  pData - pointer to a buffer to store the requested data
 *
 *  Remarks:
 *  The IZOT protocol stack calls <IzotMemoryRead> whenever it receives 
 *  a network management memory read request that fits into the registered 
 *  file access window. This callback function is used to read data starting 
 *  at the specified virtual memory address within the IZOT Transceiver. This 
 *  function applies to reading template files, CP value files, user-defined 
 *  files, and possibly other data. The address space for this command is 
 *  limited to the IZOT Transceiver's 64 KB address space.
 *
 */
LonStatusCode IzotMemoryRead(const unsigned address, const unsigned size, void* const pData)
{
#if     LON_DMF_ENABLED
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
 *  Callback: IzotMemoryWrite
 *  Update memory in the IZOT device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be update
 *  size - number of bytes to write
 *  pData - pointer to the data to write
 *
 *  Remarks:
 *  The IZOT protocol stack calls <IzotMemoryWrite> whenever it 
 *  receives a network management memory write request that fits into the 
 *  registered file access window. This callback function is used to write 
 *  data at the specified virtual memory address for the IZOT Transceiver.  
 *  This function applies to CP value files, user-defined files, and possibly 
 *  other data. The address space for this command is limited to the IZOT 
 *  Transceiver�s 64 KB address space.  The IZOT protocol stack 
 *  automatically calls the <IzotPersistentAppSegmentHasBeenUpdated> function to 
 *  schedule an update whenever this callback returns *LonStatusNoError*.
 *
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
 *  Event: IzotPersistentSegOpenForRead
 *  Calls the registered callback of <IzotFlashSegOpenForRead>.
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
 *  Event: IzotPersistentSegOpenForWrite
 *  Calls the registered callback of <IzotFlashSegOpenForWrite>.
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
 *  Event: IzotPersistentSegClose
 *  Calls the registered callback of <IzotFlashSegClose>.
 */
void IzotPersistentSegClose(const IzotPersistentSegType persistentSegType)
{
    if (izot_close_handler) {
        izot_close_handler(persistentSegType);
    }
}

/*
 *  Event: IzotPersistentSegRead
 *  Calls the registered callback of <IzotFlashSegRead>.
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
 *  Event: IzotPersistentSegWrite
 *  Calls the registered callback of <IzotFlashSegWrite>.
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
 *  Event: IzotPersistentSegIsInTransaction
 *  Calls the registered callback of <IzotFlashSegIsInTransaction>.
 *
 *  Remarks:
 *
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
 *  Event: IzotPersistentSegEnterTransaction
 *  Calls the registered callback of <IzotFlashSegEnterTransaction>.
 *
 *  Remarks:
 *
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
 *  Event: IzotPersistentSegExitTransaction
 *  Calls the registered callback of <IzotFlashSegExitTransaction>.
 *
 *  Remarks:
 *
 */
LonStatusCode IzotPersistentSegExitTransaction(const IzotPersistentSegType persistentSegType)
{
    if (izot_exit_tx_handler) {
        return izot_exit_tx_handler(persistentSegType);
    } else {
        return LonStatusStackNotInitialized;
    }
}

/*
 *  In IzoT,  event handlers are implemented as optional callback functions.
 *  For each of the supported events, an event type is defined in IzotTypes.h,
 *  and a registrar function is provided. The registrar can register an
 *  application-defined callback function (the "event handler") for a given
 *  event, and it can de-register an event handler when being called with a
 *  NULL pointer.
 *
 *  For example, the IzotWink event is implemented with a function
 *  of type IzotWinkFunction, and registered using the IzotWinkRegistrar
 *  API as shown in this hypothetical example:
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
 *  The IzotDeregisterAllCallbacks() API can be used to deregister all event
 *  handlers. It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.
 */
 
/*
 *  Event: IzotGetCurrentDatapointSizeRegistrar
 *  This registrar register the <IzotGetCurrentDatapointSize>.
 *
 *  Remarks:
 *
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
 *  Event: IzotResetRegistrar
 *  This registrar register the <IzotReset>.
 *
 *  Remarks:
 *
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
 *  Event: IzotWinkRegistrar
 *  This registrar register the <IzotWink>.
 *
 *  Remarks:
 *
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
 *  Event: IzotOfflineRegistrar
 *  This registrar register the <IzotOffline>.
 *
 *  Remarks:
 *
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
 *  Event: IzotOnlineRegistrar
 *  This registrar register the <IzotOnline>.
 *
 *  Remarks:
 *
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
 *  Event: IzotDatapointUpdateOccurredRegistrar
 *  This registrar register the <IzotDatapointUpdateOccurred>.
 *
 *  Remarks:
 *
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
 *  Event: IzotDatapointUpdateCompletedRegistrar
 *  This registrar register the <IzotDatapointUpdateCompleted>.
 *
 *  Remarks:
 *
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
 *  Event: IzotMsgArrivedRegistrar
 *  This registrar register the <IzotMsgArrived>.
 *
 *  Remarks:
 *
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
 *  Event: IzotResponseArrivedRegistrar
 *  This registrar register the <IzotResponseArrived>.
 *
 *  Remarks:
 *
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
 *  Event: IzotMsgCompletedRegistrar
 *  This registrar register the <IzotMsgCompleted>.
 *
 *  Remarks:
 *
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
 *  Event: IzotMemoryReadRegistrar
 *  This registrar register the <IzotMemoryRead>.
 *
 *  Remarks:
 *
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
 *  Event: IzotMemoryWriteRegistrar
 *  This registrar register the <IzotMemoryWrite>.
 *
 *  Remarks:
 *
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
 *  Event: IzotServiceLedStatusRegistrar
 *  This registrar register the <IzotServiceLedStatus>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegOpenForReadRegistrar
 *  This registrar register the <IzotFlashSegOpenForRead>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegOpenForWriteRegistrar
 *  This registrar register the <IzotFlashSegOpenForWrite>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegCloseRegistrar
 *  This registrar register the <IzotFlashSegClose>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegDeleteRegistrar
 *  This registrar register the <IzotFlashSegDelete>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegReadRegistrar
 *  This registrar register the <IzotFlashSegRead>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegWriteRegistrar
 *  This registrar register the <IzotFlashSegWrite>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegIsInTransactionRegistrar
 *  This registrar register the <IzotFlashSegIsInTransaction>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegEnterTransactionRegistrar
 *  This registrar register the <IzotFlashSegEnterTransaction>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFlashSegExitTransactionRegistrar
 *  This registrar register the <IzotFlashSegExitTransaction>.
 *
 *  Remarks:
 *
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
 *  Event: IzotPersistentSerializeSegmentRegistrar
 *  This registrar register the <IzotPersistentSerializeSegment>.
 *
 *  Remarks:
 *
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
 *  Event: IzotPersistentDeserializeSegmentRegistrar
 *  This registrar register the <IzotPersistentDeserializeSegment>.
 *
 *  Remarks:
 *
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
 *  Event: IzotPersistentGetApplicationSegmentSizeRegistrar
 *  This registrar register the <IzotPersistentGetApplicationSegmentSize>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFilterMsgArrivedRegistrar
 *  This registrar register the <IzotFilterMsgArrived>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFilterResponseArrivedRegistrar
 *  This registrar register the <IzotFilterResponseArrived>.
 *
 *  Remarks:
 *
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
 *  Event: IzotFilterMsgCompletedRegistrar
 *  This registrar register the <IzotFilterMsgCompleted>.
 *
 *  Remarks:
 *
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
 *  Event: IzotDeregisterAllCallbacks
 *  Deregister All callbacks.
 *
 *  Remarks:
 *
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

