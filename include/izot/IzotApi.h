//
// IzotApi.h
//
// Copyright (C) 2023-2025 EnOcean
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
 * Title: LON Stack API Interfaces
 *
 * Abstract:
 * This file declares prototypes for LON Stack API functions.
 */

#ifndef _IZOT_API_H
#define _IZOT_API_H

#include <stdlib.h>
#ifdef  __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include "IzotConfig.h"
#include "IzotPlatform.h"
#include "IzotTypes.h"
#include "IzotCal.h"
#include "IzotHal.h"
#include "IzotOsal.h"
#include "IzotPersistentFlashDirect.h"
#include "IPv4ToLsUdp.h"
#include "isi_int.h"
#include "lcs_api.h"
#include "lcs_timer.h"
#include "lcs_node.h"
#include "lcs.h"
#include "Persistent.h"

#define IZOT_EXTERNAL_FN extern


/*
 * *****************************************************************************
 * SECTION: MACROS
 * *****************************************************************************
 */

#define IZOT_EXTERNAL_FN extern
#define IZOT_MOD_NAME "izot"
                                                    
/*
 * *****************************************************************************
 * SECTION: API FUNCTIONS
 * *****************************************************************************
 *
 * This section details the LON Stack DX API functions.
 */

/*
 * Function: IzotEventPump
 * Process asynchronous IzoT events.
 *
 * Remarks:
 * The application calls this API frequently and periodically, after the
 * <IzotStartStack>() call returned with success. This function processes
 * any events that have been posted by the IzoT protocol stack.
 *
 * This function must be called at least once every 10 ms.  Use the following
 * formula to determine the minimum call rate:
 * rate = MaxPacketRate / (InputBufferCount - 1)
 * where MaxPacketRate is the maximum number of packets per second arriving
 * for the device and InputBufferCount is the number of input buffers defined
 * for the application
 */
IZOT_EXTERNAL_FN void IzotEventPump(void);

/*
 * Function: IzotGetUniqueId
 * Gets the registered unique ID (Neuron ID).
 *
 * Parameters:
 * pId   - pointer to the the unique ID
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * The *Unique ID* is also known as the *Neuron ID*, however, *Neuron ID* is
 * a deprecated term.
 */
IZOT_EXTERNAL_FN IzotApiError IzotGetUniqueId(IzotUniqueId* const pId);

/*
 * Function: IzotGetVersion
 * Returns the IzoT Device Stack version number.
 *
 * Parameters:
 * pMajorVersion - pointer to receive the IzoT Device Stack major version
 *      number.
 * pMinorVersion - pointer to receive the IzoT Device Stack minor version
 *      number.
 * pBuildNumber - pointer to receive the IzoT Device Stack build number.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function provides the version of the IzoT Device Stack.  Note that
 * this function can be called at any time.
 */
IZOT_EXTERNAL_FN IzotApiError IzotGetVersion(unsigned* const pMajorVersion,
    unsigned* const pMinorVersion, unsigned* const pBuildNumber);

/*
 * Function: IzotGetDatapointIndex
 * Get the index of datapoint by name
 *
 * Parameters:
 * name  - the name of the datapoint
 *
 */
#define IzotGetDatapointIndex(name) name.global_index

/*
 * Function: IzotPoll
 * Polls a bound, polling, input datapoint. See <IzotPollByIndex> for more.
 *
 * Parameters:
 * name  - the name of the datapoint
 *
 * Returns:
 * <IzotApiError>
 */
#define IzotPoll(name) IzotPollByIndex(name.global_index)

/*
 * Function: IzotSleep
 * Sleep for a specified number of ticks.
 *
 * Parameters:
 * ticks - The number of ticks to sleep 
 *
 * Returns:
 * <void>.
 *
 * Remarks:
 * Suspend the task for the specified number of clock ticks.
 */
IZOT_EXTERNAL_FN void IzotSleep(unsigned int ticks);    
    
/*
 * Function: IzotPollByIndex
 * Polls a bound, polling, input datapoint. See <IzotPoll> for an alternative.
 *
 * Parameters:
 * index - index of the input datapoint
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to poll an input datapoint. Polling an input datapoint
 * causes the device to solicit the current value of all output datapoints
 * that are bound to this one.
 *
 * The function returns <IzotApiError> if the API has successfully queued the
 * request. Note that the successful completion of this function does not
 * indicate the successful arrival of the requested values. The values received
 * in response to this poll are reported by one or more of calls to the
 * <IzotDatapointUpdateOccurred> event handler.
 *
 * IzotPoll operates only on input datapoints that have been declared with the
 * *polled* attribute. Only output datapoints that are bound to the input 
 * datapoint will be received.
 *
 * Note that it is *not* an error to poll an unbound polling input datapoint
 * If this is done, the application will not receive any
 * <IzotDatapointUpdateOccurred> events, but will receive a
 * <IzotDatapointUpdateCompleted> event with the success parameter set to TRUE.
 */

IZOT_EXTERNAL_FN IzotApiError IzotPollByIndex(signed index);

/*
 * Function: IzotPropagate
 * Propagates the value of a bound output datapoint to the network.
 * See <IzotPropagateByIndex> for more.
 *
 * Parameters:
 * name - name of the datapoint to propagate.
 *
 * Returns:
 * <IzotApiError>
 */
#define IzotPropagate(name) IzotPropagateByIndex(name.global_index)

/*
 * Function: IzotPropagateByIndex
 * Propagates the value of a bound output datapoint to the network.
 * See <IzotPropagate> for an alternative.
 *
 * Parameters:
 * index - the index of the datapoint
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Note that it is not an error to propagate an unbound non-polled output.
 * If this is done, the IzoT  protocol stack will not send any updates
 * to the network, but will generate a <IzotDatapointUpdateCompleted> event 
 * with the success parameter set to TRUE.
 *
 * If IzotPropagate() returns <IzotApiError>, 
 * the <IzotDatapointUpdateCompleted> event will be triggered when the 
 * datapoint update has successfully completed or failed.
 *
 * The IzoT Device Stack DX implements the <IzotPropagate> function such
 * that the datapoint and its current value are scheduled for propagation
 * at the time of the <IzotPropagate> call.
 *
 * The IzoT Device Stack DX does not support the sync attribute, as it
 * treats all datapoints as synchronous datapoints. To propagate only the
 * mostly assigned value, make you can make multiple assignments to the value
 * of an output datapoint while executing your algorithm, calling the
 * <IzotPropagate> function (or <IzotPropagateByIndex>) just once at the
 * end of your computation.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPropagateByIndex(signed index);


/*
 * Function: IzotSendServicePin
 * Propagates a service pin message.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Use this function to propagate a service pin message to the network.
 * The function will fail if the device is not yet fully initialized.
 */
IZOT_EXTERNAL_FN IzotApiError IzotSendServicePin(void);

/*
 * Function: IzotSendMsg
 * Send an application message (not a datapoint message).
 *
 * Parameters:
 * tag - message tag for this message
 * priority - priority attribute of the message
 * serviceType - service type for use with this message
 * authenticated - TRUE to use authenticated service
 * pDestAddr - pointer to destination address
 * code - message code
 * pData - message data, may be NULL if length is zero
 * length - number of valid bytes available through pData
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function is called to send an application message. For application
 * messages, the message code should be in the range of 0x00..0x2f.  Codes in
 * the 0x30..0x3f range are reserved for protocols such as file transfer.
 *
 * If the tag field specifies one of the bindable messages tags
 * (tag < # bindable message tags), the pDestAddr is ignored (and can be
 * NULL) because the message is sent using implicit addressing. Otherwise,
 * explicit addressing is used, and pDestAddr must be provided.
 *
 * A successful return from this function indicates only that the message has
 * been queued to be sent.  If this function returns success, the IzoT
 * API will call <IzotMsgCompleted> with an indication of the
 * transmission success.
 *
 * If the message is a request, <IzotResponseArrived> event handlers are
 * called when corresponding responses arrive.
 */
IZOT_EXTERNAL_FN IzotApiError IzotSendMsg(
    const unsigned tag, const IzotBool priority,
    const IzotServiceType serviceType,
    const IzotBool authenticated,
    const IzotSendAddress* const pDestAddr,
    const IzotByte code,
    const IzotByte* const pData, const unsigned length);

/*
 * Function: IzotSendResponse
 * Sends a response.
 *
 * Parameters:
 * correlator - message correlator, received from <IzotMsgArrived>
 * code - response message code
 * pData - pointer to response data, may be NULL if length is zero
 * length - number of valid response data bytes in pData
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function is called to send an application response.  The correlator
 * is passed in to <IzotMsgArrived> and must be copied and saved if the 
 * response is to be sent after returning from that routine.  A response code 
 * should be in the 0x00..0x2f range.
 */
IZOT_EXTERNAL_FN IzotApiError IzotSendResponse(
    const IzotCorrelator correlator,
    const IzotByte code,
    const IzotByte* const pData,
    const unsigned length);

/*
 * Function: IzotReleaseCorrelator
 * Release a request correlator without sending a response.
 *
 * Parameters:
 * correlator - The correlator, obtained from <IzotMsgArrived>
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function is called to release a correlator obtained from
 * <IzotMsgArrived> without sending a response.  The application must either
 * send a response to every message with a service type of request, or release
 * the correlator, but not both.
 */
IZOT_EXTERNAL_FN IzotApiError IzotReleaseCorrelator(const IzotCorrelator correlator);

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
 * Function: IzotQueryStatus
 * Request local status and statistics.
 *
 * Parameters:
 * pStatus - pointer to a <IzotStatus> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to obtain the local status and statistics of the IzoT
 * device. The status will be stored in the <IzotStatus> structure provided.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryStatus(IzotStatus* const pStatus);

/*
 * Function: IzotClearStatus
 * Clears the status statistics on the IzoT device.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function can be used to clear the IzoT device status and statistics
 * records.
 */
IZOT_EXTERNAL_FN IzotApiError IzotClearStatus(void);

/*
 * Function: IzotQueryConfigData
 * Request a copy of local configuration data.
 *
 * Parameters:
 * pConfig - pointer to a <IzotConfigData> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to request a copy of device's configuration data.
 * The configuration is stored in the <IzotConfigData> structure
 * provided.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryConfigData(IzotConfigData* const pConfig);

/*
 * Function: IzotUpdateConfigData
 * Updates the configuration data on the IzoT device.
 *
 * Parameters:
 * pConfig - pointer to <IzotConfigData> configuration data
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to update the device's configuration data based on the
 * configuration stored in the <IzotConfigData> structure.
 */
IZOT_EXTERNAL_FN IzotApiError IzotUpdateConfigData(const IzotConfigData* const pConfig);

/*
 * Function: IzotSetNodeMode
 * Sets the device's mode and/or state.
 *
 * Parameters:
 * mode - mode of the IzoT device, see <IzotNodeMode>
 * state - state of the IzoT device, see <IzotNodeState>
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Use this function to set the IzoT device's mode and state.
 * If the mode parameter is *IzotChangeState*, the state parameter may be
 * set to one of *IzotApplicationUnconfig*, *IzotNoApplicationUnconfig*,
 * *IzotConfigOffLine* or *IzotConfigOnLine*.  Otherwise the state parameter
 * should be *IzotStateInvalid* (0).  Note that while the <IzotNodeState>
 * enumeration is used to report both the state and mode (see <IzotStatus>),
 * it is *not* possible to change both the state and mode (online/offline) at
 * the same time.
 *
 * You can also use the shorthand functions <IzotGoOnline>, <IzotGoOffline>,
 * <IzotGoConfigured>, and <IzotGoUnconfigured>.
*/
IZOT_EXTERNAL_FN IzotApiError IzotSetNodeMode(const IzotNodeMode mode,
    const IzotNodeState state);

/*
* Function: IzotGoOnline
* Sets the IzoT device online.
*
* Returns:
* <IzotApiError>
*
* Remarks:
* Call this function to put the IzoT device into online mode.
*/
#define IzotGoOnline() IzotSetNodeMode(IzotApplicationOnLine, IzotStateInvalid)

/*
* Function: IzotGoOffline
* Sets the IzoT device offline.
*
* Returns:
* <IzotApiError>
*
* Remarks:
* Call this function to put the IzoT device into offline mode.
*/
#define IzotGoOffline() IzotSetNodeMode(IzotApplicationOffLine, IzotStateInvalid)

/*
* Function: IzotGoConfigured
* Sets the IzoT device state to configured.
*
* Returns:
* <IzotApiError>
*
* Remarks:
* Call this function to set the IzoT device state to *IzotConfigOnLine*.
*/
#define IzotGoConfigured() IzotSetNodeMode(IzotChangeState, IzotConfigOnLine)

/*
* Function: IzotGoUnconfigured
* Sets the IzoT device state to unconfigured.
*
* Returns:
* <IzotApiError>
*
* Remarks:
* Call this function to set the IzoT device state to *IzotApplicationUnconfig*.
*/
#define IzotGoUnconfigured() IzotSetNodeMode(IzotChangeState, IzotApplicationUnconfig)

/*
 * Function: IzotQueryDomainConfig
 * Request a copy of a local domain table record.
 *
 * Parameters:
 * index - index of requested domain table record (0, 1)
 * pDomain - pointer to a <IzotDomain> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to request a copy of a local domain table record.
 * The information is returned through the <IzotDomain> structure provided.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryDomainConfig(
    unsigned index,
    IzotDomain* const pDomain);

/*
 * Function: IzotUpdateDomainConfig
 * Updates a domain table record on the IzoT device.
 *
 * Parameters:
 * index - the index of the domain table to update
 * pDomain - pointer to the domain table record
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function can be used to update one record of the domain table.
 */
IZOT_EXTERNAL_FN IzotApiError IzotUpdateDomainConfig(
    unsigned index,
    const IzotDomain* const pDomain);

/*
 * Function: IzotUpdateDomain
 * Updates a domain table record and changes the LON stack to online and configured.

 * Parameters:
 * index - domain index
 * length - domain ID length (0, 1, 3, or 6)
 * domainId - domain ID (number of bytes specified by length)
 * subnet - subnet ID
 * node - node ID
 *
 * Returns:
 * <IzotApiError>
 */

IZOT_EXTERNAL_FN IzotApiError IzotUpdateDomain(
        unsigned index, 
        unsigned length, 
        const IzotByte* domainId, 
        unsigned subnet, 
        unsigned node);

/*
 * Function: IzotQueryAddressConfig
 * Request a copy of address table configuration data.
 *
 * Parameters:
 * index - index of requested address table entry
 * pAddress - pointer to a <IzotAddress> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to request a copy of the address table configuration 
 * data. The configuration is stored in the <IzotAddress> structure
 * provided.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryAddressConfig(unsigned index,
        IzotAddress* const pAddress);

/*
 * Function: IzotUpdateAddressConfig
 * Updates an address table record on the IzoT device.
 *
 * Parameters:
 * index - index of the address table to update
 * pAddress - pointer to address table record
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Use this function to write a record to the local address table.
 */
IZOT_EXTERNAL_FN IzotApiError IzotUpdateAddressConfig(unsigned index,
    const IzotAddress* const pAddress);

/*
 * Function: IzotQueryDpConfig
 * Request a copy of datapoint configuration data.
 *
 * Parameters:
 * index - index of requested datapoint configuration table entry
 * pDatapointConfig - pointer to a <IzotDatapointEcsConfig> or
 *                    <IzotDatapointConfig> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to request a copy of the local datapoint
 * configuration data. The configuration is stored in the structure provided.
 * This API uses a signed index for compatibility with enumerations
 * of datapoint index values sometimes used with the application
 * framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryDpConfig(signed index, IzotDatapointConfig* const pDatapointConfig);

/*
 * Function: IzotUpdateDpConfig
 * Updates a datapoint configuration table record on the IzoT device.
 *
 * Parameter:
 * index - index of datapoint
 * pDatapointConfig - datapoint configuration
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function can be used to update one record of the datapoint
 * configuration table.
 * This API uses a signed index for compatibility with enumerations
 * of datapoint index values sometimes used with the application
 * framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN IzotApiError IzotUpdateDpConfig(signed index, const IzotDatapointConfig* const pDatapointConfig);


/*
 * Function: IzotDatapointSetup
 * Sets the static configuration for a datapoint (NV).
 *
 * Parameters:
 * index - datapoint index
 * pDatapointConfig - datapoint configuration
 * value - datapoint value
 * size - datapoint size in bytes
 * snvtId - standard type index; set to 0 for a non-standard type
 * arrayCount - number of elements in a datapoint (NV) array; set to 0 for a single value
 * name - short datapoint (NV) name
 * sdString - long datapoint (NV) name; may include LonMark self-documentation
 * maxRate - estimated maximum update rate; set to IZOT_DATAPOINT_RATE_UNKNOWN if unknown
 * meanRate - estimated average update rate; set to IZOT_DATAPOINT_RATE_UNKNOWN if unknown
 * ibol - memory address for file-based configuration properties; set to NULL if none
 * 
 * Returns:
 * <IzotApiError>.
 *
 * Remarks:
 * This function does not update the datapoint configuration flags.  Use IzotDatapointFlags() for setting flags.
 */

IZOT_EXTERNAL_FN IzotApiError IzotDatapointSetup(IzotDatapointDefinition* const pDatapointDef, 
        volatile void const *value, IzotDatapointSize size, uint16_t snvtId, uint16_t arrayCount, 
        const char *name, const char *sdString, uint8_t maxRate, uint8_t meanRate, const uint8_t *ibol);

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
 *  <IzotApiError>
 *
 * Remarks:
 *  This function only updates the datapoint definition flags.
 *  Use IzotDatapointSetup() for setting datapoint definition fields not included
 *  in the flags.
 */

IZOT_EXTERNAL_FN IzotApiError IzotDatapointFlags(IzotDatapointDefinition* const pDatapointDef,
        IzotBool priority, IzotDatapointDirection direction, IzotBool isProperty, 
        IzotBool persistent, IzotBool changeable, IzotBool authenticated);

/*
 * Function: IzotDatapointBind
 * Binds a datapoint (NV).  Binding is the process of creating a connection to or from a datapoint
 * from or to one or more datapoints.
 *
 * Parameters:
 * index - datapoint index
 * priority - set to TRUE for a priority datapoint (NV)
 * 
 * Returns:
 * <IzotApiError>.
 *
 * Remarks:
 * This function only updates the datapoint connection information.  Use IzotDatapointSetup() and
 * IzotDatapointFlags) setting other datapoint configuration.
 */

IZOT_EXTERNAL_FN IzotApiError IzotDatapointBind(int nvIndex, IzotByte address, IzotUbits16 selector, 
                IzotBool turnAround, IzotServiceType service);

/*
 * Function: IzotQueryAliasConfig
 * Request a copy of alias configuration data.
 *
 * Parameters:
 * index - index of requested alias configuration table entry
 * pAlias - pointer to a <IzotAliasEcsConfig> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to request a copy of the alias configuration data.
 * The configuration is stored in the <IzotAliasEcsConfig> structure
 * provided.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryAliasConfig(unsigned index, IzotAliasConfig* const pAlias);

/*
 * Function: IzotUpdateAliasConfig
 * Updates an alias table record on the IzoT device.
 *
 * Parameters:
 * index - index of alias table record to update
 * pAlias - pointer to the alias table record
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function writes a record in the local alias table.
 */
IZOT_EXTERNAL_FN IzotApiError IzotUpdateAliasConfig(unsigned index, const IzotAliasConfig* const pAlias);

/*
 * Function: IzotDatapointIsBound
 * Determines whether a datapoint is bound.
 * See <IzotDatapointIsBoundByIndex> for an alternative API.
 *
 * Parameters:
 * name - name of the datapoint to examine
 * pIsBound - pointer to receive the "is-bound" attribute
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to determine whether a datapoint is bound.
 * A datapoint is bound if it, or any of its aliases, has a bound
 * selector or an address table entry. The unbound selector for a given
 * datapoint is equal to (0x3fff - datapoint index).  A datapoint
 * or alias has an address if the address index is not equal to 0xffff.
 * This API uses a signed index for compatibility with enumerations
 * of datapoint index values sometimes used with the application
 * framework, because C language enumerations are signed integers.
 */
#define IzotDatapointIsBound(name, pIsBound) IzotDatapointIsBoundByIndex(name.global_index, pIsBound)

/*
 * Function: IzotDatapointIsBoundByIndex
 * Determines whether a datapoint is bound given its index.
 * See <IzotDatapointIsBound> for an alternative API.
 *
 * Parameters:
 * index - index of the datapoint
 * pIsBound - pointer to receive the "is-bound" attribute
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to determine whether a datapoint is bound.
 * A datapoint is bound if it, or any of its aliases, has a bound
 * selector or an address table entry. The unbound selector for a given
 * datapoint is equal to (0x3fff - datapoint index).  A datapoint
 * or alias has an address if the address index is not equal to 0xffff.
 * This API uses a signed index for compatibility with enumerations
 * of datapoint index values sometimes used with the application
 * framework, because C language enumerations are signed integers.
 */
IZOT_EXTERNAL_FN IzotApiError IzotDatapointIsBoundByIndex(signed index, IzotBool* const pIsBound);

/*
 * Function: IzotMtIsBound
 * Determines whether a message tag is bound.
 *
 * Parameters:
 * tag - the message tag
 * pIsBound - pointer to receive the "is-bound" attribute
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Call this function to determine whether a message tag is bound.
 * A message tag is bound if the associated address type is anything other
 * than *IzotAddressUnassigned*.
 */
IZOT_EXTERNAL_FN IzotApiError IzotMtIsBound(
    const unsigned tag,
    IzotBool* const pIsBound);


/*
 * *****************************************************************************
 * SECTION: persistent data API FUNCTIONS
 * *****************************************************************************
 *
 * This section details the API functions that support persistent data (non-
 * volatile data).
 *
 * Remarks:
 * Persistent data is stored in data segments, identified by
 * <IzotPersistentDataSegmentType>, and are used to store IzoT persistent
 * configuration data.
 */

/*
 * Function: IzotPersistentAppSegmentHasBeenUpdated
 * Informs the IzoT protocol stack that the application data segment has been
 * updated.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Use this function to inform the IzoT protocol stack that some application
 * data been updated that should be written out to the
 * <IzotPersistentSegApplicationData> persistent data segment.  The IzoT
 * protocol stack will schedule a write to the
 * <IzotPersistentSegApplicationData> segment after the flush timeout
 * has expired.
 *
 * It is generally not necessary to call this function when application data
 * has been updated by a network management write command or a datapoint 
 * update, because the IzoT  protocol stack automatically calls this function 
 * whenever the <IzotMemoryWrite> event handler returns <IzotApiError>, and 
 * whenever a datapoint update is received for a datapoint with the
 * *IZOT_DATAPOINT_CONFIG_CLASS* or *IZOT_DATAPOINT_PERSISTENT* attribute.
 * However, the application must call this function whenever it updates
 * application-specific persistent data directly.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentAppSegmentHasBeenUpdated(void);

/*
 * Function: IzotPersistentFlushData
 * Flush all persistent data out to persistent storage.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function can be called by the application task to block until all
 * persistent data writes have been completed.  The application might do
 * this, for example, in response to a <IzotPersistentStarvation> event.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentFlushData(void);

/*
 * Function: IzotPersistentGetMaxSize
 * Gets the number of bytes required to store persistent data.
 *
 * Parameters:
 * segmentType - The segment type, see <IzotPersistentSegmentType>
 *
 * Returns:
 * The number of bytes required to store persistent data for the specified
 * segment.
 *
 * Remarks:
 * This function will not typically be called directly by the application,
 * but may be used by persistent data event handlers (implemented by the
 * application) to reserve space for persistent data segments.
 */
IZOT_EXTERNAL_FN int IzotPersistentGetMaxSize(IzotPersistentSegmentType segmentType);

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
 * the static attributes of the program
 * pControlData - Pointer to a <IzotControlData> structure, which contains data
 * used to control the runtime aspects of the stack.  These aspects can be
 * set by the application at runtime.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Initializes and configures the IzoT driver and the IzoT  protocol
 * stack. This must be the first call into the IzoT Device Stack API, and 
 * cannot be called again until <IzotDestroyStack> has been called.
 * After this function has been called, the following functions can be called:
 * <IzotRegisterStaticDatapoint>, <IzotRegisterMemoryWindow>,
 * <IzotStartStack>, and <IzotDestroyStack>.
 *
 * Note that the stack expects reasonable values for all of the initialization
 * parameters, since typically these will be created and validated by the
 * tool which generates the application framework. Therefore, the stack does
 * not provide detailed error information when a parameter is out of range.
 *
 * If <IzotCreateStack> returns any error, the stack will simply not function.
 * It will not send or receive  messages over the network.  The service LED
 * will be left on (applicationless).
 *
 * A typical application framework calls this function within a general
 * initialization function (often called IzotInit()).
 */
IZOT_EXTERNAL_FN IzotApiError IzotCreateStack(const IzotStackInterfaceData* const pInterface,
        const IzotControlData * const pControlData);


/*
 * Function: IzotRegisterStaticDatapoint
 * Registers a static datapoint with the IzoT Device Stack.
 *
 * Parameters:
 * pDatapointDef - pointer to a <IzotDatapointDefinition> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function registers a static datapoint with the IzoT Device Stack API,
 * and is called once for each static datapoint.  This function can be
 * called only after <IzotCreateStack>, but before <IzotStartStack>.
 *
 * A typical application framework calls this function within a general
 * initialization function (often called IzotInit()).
 */
IZOT_EXTERNAL_FN IzotApiError IzotRegisterStaticDatapoint(IzotDatapointDefinition* const pDatapointDef);

/*
 * Function: IzotRegisterMemoryWindow
 * Register a virtual memory address range and enable DMF.
 *
 * Parameters:
 * windowAddress - the starting virtual address of the window
 * windowSize - the size of the window, in bytes
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * This function is used to open up a window in the device's memory
 * space. IzoT protocol messages that access memory using absolute addressing
 * within the 64kB legacy address range, provide the memory addressed falls
 * within the registered window, can access memory located within the IzoT
 * application through <IzotMemoryRead> and <IzotMemoryWrite> synchronous
 * events.
 *
 * This function can only be called after <IzotCreateStack>, but before
 * <IzotStartStack>.  The address space for these memory windows is between
 * 0x0001 and 0xffff, but some protocol stacks may further limit the supported
 * address range.
 */
IZOT_EXTERNAL_FN IzotApiError IzotRegisterMemoryWindow(const unsigned windowAddress, const unsigned windowSize);

/*
 * Function: IzotStartStack
 * Completes the initialization of the LON protocol stack.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Starts running the LON stack, following successful calls to 
 * <IzotCreatStack>, <IzotRegisterStaticDatapoint>, 
 * <IzotRegisterMemoryWindow> or other initialization-time functions.
 *
 * When the IzotStartStack() function returns with success, the device stack
 * is fully operational and all persistent data (if any) has been applied.
 * A typical application framework calls this function within a general
 * initialization function (often called IzotInit()).
 */
IZOT_EXTERNAL_FN IzotApiError IzotStartStack(void);

/*
 * Function: IzotDestroyStack
 * Stops the LON protocol stack and frees all memory that it has
 * allocated.  Support for this function is optional.
 *
 * Returns:
 * <IzotApiError>
 *
 * Remarks:
 * Waits for persistent writes to complete, stops the stack, and frees all
 * temporary memory created during execution of the stack.  The service LED is
 * lit to indicate that the device is applicationless.
 */
IZOT_EXTERNAL_FN void IzotDestroyStack(void);

/*
 * Function: IzotQueryReadOnlyData
 * Request copy of local read-only data.
 *
 * Parameters:
 * pReadOnlyData - pointer to a <IzotReadOnlyData> structure
 *
 * Returns:
 * <IzotApiError>
 *
 * Call this function to request a copy of device's read-only data
 * structure.  The read-only data will be stored in the 
 * provided <IzotReadOnlyData> structure.
 */
IZOT_EXTERNAL_FN IzotApiError IzotQueryReadOnlyData(IzotReadOnlyData* const pReadOnlyData);

/*
 * Function: IzotGetAppSignature
 * Gets the application's signature.
 *
 * Returns:
 * The application signature which was sepcified by the application
 * when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetAppSignature();

/*
 * Function: IzotGetAliasCount
 * Gets the number of aliases supported by the alias table.
 *
 * Returns:
 * The size of the alias table which is specified by the application
 * when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetAliasCount();

/*
 * Function: IzotGetAddressTableCount
 * Gets the number of addresses supported by the address table.
 *
 * Returns:
 * The size of the address table which is specified by the application
 * when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetAddressTableCount();

/*
 * Function: IzotGetStaticDatapointCount
 * Gets the number of static datapoints supported by the device.
 *
 * Returns:
 * The number of static datapoint specified by the application
 * when the stack is created (<IzotCreateStack>).
 */
IZOT_EXTERNAL_FN unsigned IzotGetStaticDatapointCount();

/*
 * Function: IzotIsFirstRun
 * Determine whether or not an application is running for the first time.
 *
 * Returns:
 * True if this is the first time the application is running with the same
 * setup and configuration.
 * You can depend on this information if you need to preset certain
 * values only when the first time the applicaiton is running.
 */
IZOT_EXTERNAL_FN IzotBool IzotIsFirstRun(void);

/*
 * Function: IzotGetTickCount
 * Get the current system tick count.
 *
 * Returns:
 * The system tick count.
 */
IZOT_EXTERNAL_FN OsalTickCount IzotGetTickCount(void);

/*
 * LON Stack DX implements event handlers as optional global
 * callback functions.
 * For each of the supported events, an event type is defined in IzotTypes.h,
 * and a registrar function is provided. The registrar can register an
 * application-defined callback function (the "event handler") for a given
 * event, and it can de-register an event handler when being called with a
 * NULL pointer.
 *
 * For example, the IzotWink event is implemented with a function
 * of type IzotWinkFunction, and registered using the IzotWinkRegistrar
 * API as shown in this hypothetical example:
 *
 * typedef void (*IzotWinkFunction)(void);
 * extern IzotApiError IzotWinkRegistrar(IzotWinkFunction handler);
 *
 * void myWinkHandler(void) {
 *    flash_leds();
 * }
 *
 * int main(void) {
 *    ...
 *    // register wink handler:
 *    IzotWinkRegistrar(myWinkHandler);
 *    ...
 *    // un-register wink handler:
 *    IzotWinkRegistrar(NULL);
 * }
 *
 * The IzotDeregisterAllCallbacks() API can be used to deregister all event
 * handlers. It is not an error to deregister a callback twice, but only an
 * unclaimed callback can be registered.
 */

/*
 * Event: IzotGetCurrentDatapointSizeRegistrar
 * This registrar registers <IzotGetCurrentDatapointSize>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotGetCurrentDatapointSizeRegistrar(IzotGetCurrentDatapointSizeFunction handler);

/*
 * Event: IzotResetRegistrar
 * This registrar registers <IzotReset>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotResetRegistrar(IzotResetFunction handler);

/*
 * Event: IzotWinkRegistrar
 * This registrar registers <IzotWink>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotWinkRegistrar(IzotWinkFunction handler);

/*
 * Event: IzotOfflineRegistrar
 * This registrar registers <IzotOffline>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotOfflineRegistrar(IzotOfflineFunction handler);

/*
 * Event: IzotOnlineRegistrar
 * This registrar registers <IzotOnline>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotOnlineRegistrar(IzotOnlineFunction handler);

/*
 * Event: IzotDatapointUpdateOccurredRegistrar
 * This registrar registers <IzotDatapointUpdateOccurred>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotDatapointUpdateOccurredRegistrar(IzotDatapointUpdateOccurredFunction handler);

/*
 * Event: IzotDatapointUpdateCompletedRegistrar
 * This registrar registers <IzotDatapointUpdateCompleted>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotDatapointUpdateCompletedRegistrar(IzotDatapointUpdateCompletedFunction handler);

/*
 * Event: IzotMsgArrivedRegistrar
 * This registrar registers <IzotMsgArrived>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotMsgArrivedRegistrar(IzotMsgArrivedFunction handler);

/*
 * Event: IzotResponseArrivedRegistrar
 * This registrar registers <IzotResponseArrived>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotResponseArrivedRegistrar(IzotResponseArrivedFunction handler);

/*
 * Event: IzotMsgCompletedRegistrar
 * This registrar registers <IzotMsgCompleted>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotMsgCompletedRegistrar(IzotMsgCompletedFunction handler);

/*
 * Event: IzotMemoryReadRegistrar
 * This registrar registers <IzotMemoryRead>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotMemoryReadRegistrar(IzotMemoryReadFunction handler);

/*
 * Event: IzotMemoryWriteRegistrar
 * This registrar registers <IzotMemoryWrite>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotMemoryWriteRegistrar(IzotMemoryWriteFunction handler);

/*
 * Event: IzotServiceLedStatusRegistrar
 * This registrar registers <IzotServiceLedStatus>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotServiceLedStatusRegistrar(IzotServiceLedStatusFunction handler);

/*
 * Event: IzotPersistentOpenForReadRegistrar
 * This registrar registers <IzotPersistentOpenForRead>.
 */

IZOT_EXTERNAL_FN IzotApiError IzotPersistentOpenForReadRegistrar(IzotPersistentOpenForReadFunction handler);

/*
 * Event: IzotPersistentOpenForWriteRegistrar
 * This registrar registers <IzotPersistentOpenForWrite>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentOpenForWriteRegistrar(IzotPersistentOpenForWriteFunction handler);

/*
 * Event: IzotPersistentCloseRegistrar
 * This registrar registers <IzotPersistentClose>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentCloseRegistrar(IzotPersistentCloseFunction handler);

/*
 * Event: IzotPersistentDeleteRegistrar
 * This registrar registers <IzotPersistentDelete>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentDeleteRegistrar(IzotPersistentDeleteFunction handler);

/*
 * Event: IzotPersistentReadRegistrar
 * This registrar registers <IzotPersistentRead>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentReadRegistrar(IzotPersistentReadFunction handler);

/*
 * Event: IzotPersistentWriteRegistrar
 * This registrar registers <IzotPersistentWrite>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentWriteRegistrar(IzotPersistentWriteFunction handler);

/*
 * Event: IzotPersistentIsInTransactionRegistrar
 * This registrar registers <IzotPersistentIsInTransaction>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentIsInTransactionRegistrar(IzotPersistentIsInTransactionFunction handler);

/*
 * Event: IzotPersistentEnterTransactionRegistrar
 * This registrar registers <IzotPersistentEnterTransaction>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentEnterTransactionRegistrar(IzotPersistentEnterTransactionFunction handler);

/*
 * Event: IzotPersistentExitTransactionRegistrar
 * This registrar registers <IzotPersistentExitTransaction>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentExitTransactionRegistrar(IzotPersistentExitTransactionFunction handler);

/*
 * Event: IzotPersistentGetApplicationSegmentSizeRegistrar
 * This registrar registers <IzotPersistentGetApplicationSegmentSize>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentGetApplicationSegmentSizeRegistrar(
        IzotPersistentGetApplicationSegmentSizeFunction handler);

/*
 * Event: IzotPersistentDeserializeSegmentRegistrar
 * This registrar registers <IzotPersistentDeserializeSegment>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentDeserializeSegmentRegistrar(
        IzotPersistentDeserializeSegmentFunction handler);

/*
 * Event: IzotPersistentSerializeSegmentRegistrar
 * This registrar registers <IzotPersistentSerializeSegment>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotPersistentSerializeSegmentRegistrar(
        IzotPersistentSerializeSegmentFunction handler);

#if defined(IZOT_STACK_EX)
/*
 * Event: IzotPersistentStarvationRegistrar
 * This registrar registers <IzotPersistentStarvation>.
 */
IZOT_EXTERNAL_FN const IzotApiError IzotPersistentStarvationRegistrar(
IzotPersistentStarvationFunction handler);
#endif

/*
 * Message filters
 */

/*
 * Event: IzotFilterMsgArrivedRegistrar
 * This registrar registers <IzotFilterMsgArrived>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotFilterMsgArrivedRegistrar(IzotFilterMsgArrivedFunction handler);

/*
 * Event: IzotFilterResponseArrivedRegistrar
 * This registrar registers <IzotFilterResponseArrived>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotFilterResponseArrivedRegistrar(IzotFilterResponseArrivedFunction handler);

/*
 * Event: IzotFilterMsgCompletedRegistrar
 * This registrar registers <IzotFilterMsgCompleted>.
 */
IZOT_EXTERNAL_FN IzotApiError IzotFilterMsgCompletedRegistrar(IzotFilterMsgCompletedFunction handler);

/*
 * Event: IzotDeregisterAllCallbacks
 * Deregister all callbacks.
 */
IZOT_EXTERNAL_FN void IzotDeregisterAllCallbacks(void);


IZOT_EXTERNAL_FN void IzotDatapointUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);

IZOT_EXTERNAL_FN void IzotReset(const IzotResetNotification* const pResetNotification);

IZOT_EXTERNAL_FN void IzotServiceLedStatus(IzotServiceLedState state, IzotServiceLedPhysicalState physicalState);

IZOT_EXTERNAL_FN IzotBool IzotFilterMsgCompleted(const unsigned tag, const IzotBool success);

IZOT_EXTERNAL_FN void IzotMsgCompleted(const unsigned tag, const IzotBool success);

#ifdef  __cplusplus
}
#endif

#endif /* _IZOT_API_H */
