/*
 * IzotIsiApi.h
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Interoperable Self-Installation (ISI) API
 * Purpose: Defines LON ISI services for a LON stack.
 */

#ifndef __ISIAPI_H__
#define  __ISIAPI_H__

#include <stdlib.h>
#ifdef  __cplusplus
extern "C" {
#endif

#include "izot/IzotPlatform.h"
#include "izot/IzotTypes.h"
#include "izot/IzotIsiTypes.h"

#define ISI_EXTERNAL_FN extern

/*****************************************************************
 * Section: Globals
 *****************************************************************/

 ISI_EXTERNAL_FN IsiCallbackVectors theIsiCallbackVectors;

 /*****************************************************************
 * Section: Function Declarations
 *****************************************************************/

/*
 * Stops the ISI engine.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code on failure.
 * Notes:
 *   Use IsiStart() to restart the ISI engine.  This function has no forwarder. 
 *   Calling this function when the ISI engine is already stopped has no action.
 */
extern LonStatusCode IsiStop(void);

/*
 * Starts the ISI engine.
 * Parameters:
 *   apiVersion: IzoT ISI API version; must be 1
 *   type: Type of the ISI engine; values are isiTypeS, isiTypeDa, or isiTypeDas
 *   flags: Option flags for starting the engine; values 0x01, 0x04, and 0x08 
 *              have no effect; all unrecognized values will be ignored
 *   connections: Size of the connection table, in records
 *   didLength: Size of the default primary domain ID; must be 1, 3, or 6
 *   pDid; The primary domain ID default value; must be 6 bytes, but only 
 *              didLength bytes are valid
 *   repeatCount: The repeat count used with all datapoint connections, where all
 *              connections share the same repeat counter
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code on failure
 * Notes:
 *   After this function is called, the ISI engine sends and receives ISI
 *   messages, and manages the network configuration of the device; call
 *   this function in your reset task when self-installation is enabled
 *   stop the ISI engine when self-installation is disabled
 */
ISI_EXTERNAL_FN LonStatusCode IsiStart(
        unsigned apiVersion, 
        IsiType type, 
        IsiFlags flags, 
        unsigned connections, 
        unsigned didLength, 
        const IzotByte* pDid, 
        unsigned repeatCount);

/*
 * Performs periodic processing for the ISI engine.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code on failure.
 * Notes:
 *   After this function is called, the ISI engine sends and receives ISI
 *   messages, and manages the network configuration of the host device.
 *   Typically, you start the ISI engine in your reset task when self-installation
 *   is enabled, and you stop the ISI engine when self-installation is disabled.
 */
ISI_EXTERNAL_FN LonStatusCode IsiTick();

/*
 * Restores the device's self-installation data to factory defaults.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError (0) on success, or an <LonStatusCode> error code on failure.
 * Notes:
 *   Restore the host device's self-installation data to factory defaults,
 *   causing the immediate and unrecoverable loss of all connection information.
 *   This function has the same functionality regardless of whether the ISI engine 
 *   is running or not.
 *
 *   Executed on a Neuron Chip or Smart Transceiver, the engine is stopped and the
 *   device resets to complete the process.  Because of the reset, this function
 *   never returns to the caller.  Any changes related to returning to factory
 *   defaults, such as resetting of device-specific configuration properties to
 *   their initial values, must occur prior to calling this function.
 *
 *   On a LON stack,  IsiReturnToFactoryDefaults() returns to the caller.
 *   The node reset doesn't cause a physical reset on a typical LON device, but
 *   cleans up pending transactions and fires the LON stack's reset event. 
 *   Typical applications use this event to start the ISI engine, if desired.
 */
ISI_EXTERNAL_FN LonStatusCode IsiReturnToFactoryDefaults(void);

/*
 * Function: IsiFetchDomain
 * Starts or restarts the fetch domain process in a domain address server (DAS).
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * This function must not be called from a device that is not a domain address
 * server. The ISI engine must be running for this function to have any effect,
 * and this function only operates on a domain address server.
 */
ISI_EXTERNAL_FN LonStatusCode IsiFetchDomain(void);

/*
 * Function: IsiFetchDevice
 * Fetches a device by assigning a domain to the device from a domain address
 * server (DAS).
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * This function must not be called from a device that is not a domain address
 * server. This operation does not require application code on the remote 
 * device. The remote device remains unaware of the change to its primary 
 * domain. An alternate method to assign a domain to a device is for the device 
 * to use  the <IsiAcquireDomain> function. This function provides more 
 * immediate  recovery from network addressing conflicts and more immediate 
 * maintenance of  automatic connections. The disadvantage of using 
 * the <IsiAcquireDomain> function is that it typically requires more code 
 * on the device, and it requires that the device support ISI-DA. 
 * The <IsiFetchDevice> function can be used with any device. A DAS must 
 * support both methods. The ISI engine must be running for this function 
 * to have any effect, and this function only operates on a domain 
 * address server.
 */
ISI_EXTERNAL_FN LonStatusCode IsiFetchDevice(void);

/*
 * Function: IsiOpenEnrollment
 * Opens manual enrollment for the specified assembly.
 *
 * Parameters:
 * assembly - assembly for which the enrollment should be opened.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * This operation turns the device into a connection host for this connection,
 * and sends a CSMO manual connection invitation to all devices in the network.
 * No forwarder is provided for this function. The ISI engine must be running
 * and in the idle state.
 */
ISI_EXTERNAL_FN LonStatusCode IsiOpenEnrollment(unsigned Assembly);

/*
 * Function: IsiCreateEnrollment
 * Accepts a connection invitation.
 *
 * Parameters:
 * assembly - assembly for which the enrollment should be created.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * This function is called after the application has received and approved a
 * CSMO open enrollment message. The connection is created anew, replacing
 * any previously existing enrollment information associated with this assembly.
 * On a connection host that has received at least one CSME enrollment 
 * acceptance message, this command completes the enrollment and implements the 
 * connection as new, replacing any previously existing enrollment information 
 * associated with this assembly.
 * The ISI engine must be running and in the correct state for this function to
 * have any effect. For a connection host, the ISI engine must be in the
 * approved state. Other devices must be in the pending state.
 */
ISI_EXTERNAL_FN LonStatusCode IsiCreateEnrollment(unsigned Assembly);

/*
 * Function: IsiExtendEnrollment
 * Extends an enrollment invitation.
 *
 * Parameters:
 * assembly - assembly for which the enrollment should be extended.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * This function is called after the application has received and approved a
 * CSMO open enrollment message. The connection is added to any previously
 * existing connections. If no previous connection exists for assembly, a new
 * connection is created.
 * On a connection host that has received at least one CSME enrollment 
 * acceptance message, this command completes the enrollment and extends any 
 * existing connections. If no previous connection exists for assembly, a new 
 * connection is created.
 * The ISI engine must be running and in the correct state for this function to
 * have any effect. For a connection host, the ISI engine must be in the 
 * approved state. Other devices must be in the pending state.
 */
ISI_EXTERNAL_FN LonStatusCode IsiExtendEnrollment(unsigned assembly);

/*
 * Function: IsiCancelEnrollment
 * Cancels an open (pending or approved) enrollment.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * When used on a connection host, a CSMX connection cancellation message is
 * issued to cancel enrollment on the connection members. When used on a device
 * that has accepted, but not yet implemented, an open enrollment, this function
 * causes the device to opt out of the enrollment locally.
 * The function has no effect unless the ISI engine is running and in the 
 * pending or approved state.
 */
ISI_EXTERNAL_FN LonStatusCode IsiCancelEnrollment(void);

/*
 * Function: IsiLeaveEnrollment
 * Removes the specified assembly from all enrolled connections as a local
 * operation only.
 *
 * Parameters:
 * assembly - assembly for which the enrollment should be removed.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * When used on the connection host, the function is automatically
 * interpreted as <IsiDeleteEnrollment>.
 * No forwarder is provided for this function. This function has no effect if
 * the ISI engine is stopped.
 */
ISI_EXTERNAL_FN LonStatusCode IsiLeaveEnrollment(unsigned Assembly);

/*
 * Function: IsiDeleteEnrollment
 * Removes the specified assembly from all enrolled connections.
 *
 * Parameters:
 * assembly - assembly for which the enrollment should be deleted.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * This function removes the specified assembly from all enrolled connections,
 * and sends a CSMD connection deletion message to all other devices in the
 * connection to remove them from the connection as well.
 * This function has no effect if the ISI engine is stopped.
 */
ISI_EXTERNAL_FN LonStatusCode IsiDeleteEnrollment(unsigned Assembly);

/*
 * Function: IsiIsBecomingHost
 * Queries whether the specified assembly is a host for an enrollment.
 *
 * Parameters:
 * assembly - assembly for which to be queried.
 *
 * Returns:
 * True - If <IsiOpenEnrollment> has been called for the specified assembly and 
 *        the enrollment has not yet timed out, been cancelled, or confirmed.
 * False - Otherwise
 *
 * Remarks:
 * You can also override the <IsiUpdateUserInterface> callback function and
 * track the state of the assembly. The callack returns TRUE if that state is
 * *IsiPendingHost* or *IsiApprovedHost*.
 * The function operates whether the ISI engine is running or not.
 */
ISI_EXTERNAL_FN IzotBool IsiIsBecomingHost(unsigned Assembly);

/*
 * Function: IsiIsRunning
 * Queries the state of the ISI engine.
 *
 * Returns:
 * True - the ISI engine is running.
 * False - the ISI engine is stopped
 *
 * Remarks:
 * The Non-zero return values are considered TRUE.
 * No forwarder is provided for this function. The function operates in all 
 * states of the ISI engine.
 */
ISI_EXTERNAL_FN IzotBool IsiIsRunning(void);

/*
 * Function: IsiSendDrum
 * Sends the DRUM message.
 *
 * Returns:
 *
 * Remarks:
 * All devices in an ISI network periodically broadcast their status by sending 
 * out DRUM messages. To discover devices, you can monitor these status 
 * messages.
 */
ISI_EXTERNAL_FN void IsiSendDrum(void);


/*
 * Callback:   IsiUpdateUserInterface
 * Provides status feedback from the ISI engine.
 *
 * Parameters:
 * Event - ISI event value as defined in <IsiEvent> enumeration.
 * Parameter - event argument, as detailed in the <IsiEvent> definition.
 *
 * Remarks:
 * These events are useful for synchronizing the device's user interface with
 * the ISI engine. To receive notification of ISI status events, override the
 * <IsiUpdateUserInterface> callback function. The default implementation of
 * <IsiUpdateUserInterface> does nothing.
 * The ISI engine calls this function with the event parameter set to one of the
 * values defined for the <IsiEvent> enumeration. Some of these events carry a
 * meaningful value in the parameter argument, as detailed in the <IsiEvent>
 * definition. You can use the event parameter passed to the
 * <IsiUpdateUserInterface> callback function to track the state of the ISI
 * engine. This is a simple way to determine which ISI function calls make sense
 * at any time, and which ones don't. The function operates only when the ISI
 * engine is running.
 */
ISI_EXTERNAL_FN void IsiUpdateUserInterface(IsiEvent Event, IzotByte Parameter);

/*
 * Callback:   IsiUpdateDiagnostics
 * Provides optional detailed ISI diagnostic events.
 *
 * Parameters:
 * Event - ISI event value as defined in <IsiEvent> enumeration.
 * Parameter - event argument, as detailed in the <IsiEvent> definition.
 *
 * Remarks:
 * These events are useful for debugging ISI applications and are not typically
 * used for production products. To receive notification of diagnostic events,
 * enable diagnostics in the <IsiStart> function and override the 
 * <IsiUpdateDiagnostics> callback function. This callback is normally disabled 
 * and the default implementation of <IsiUpdateDiagnostics> does nothing. 
 * The ISI engine calls this function with the Event parameter set to one of the
 *  values defined for the IsiDiagnostic enumeration in Appendix A. Some of 
 * these events carry a meaningful value in the Parameter argument, as detailed 
 * in the IsiDiagnostic definition. Most diagnostics events supplement UI 
 * events;use a combination of both events for a complete trace record.
 */
ISI_EXTERNAL_FN void IsiUpdateDiagnostics(
IsiDiagnostic Event, 
IzotByte Parameter
);

/*
 * Callback:   IsiGetDpIndex
 * Returns the datapoint index 0 - 254 of the datapoint at the
 * specified offset within the specified assembly, or ISI_NO_INDEX if no such
 * datapoint exists.
 *
 * Parameters:
 * Assembly - assembly number
 * Offset - zero-based and relates to the selector number Offset that is used 
 *          with the enrollment of this assembly
 * PreviousIndex - sets to ISI_NO_INDEX to get the index of the first Dp.  
 *                 Otherwise, sets to previous datapoint index to get the 
 *                 next one.
 *
 * Returns:
 *  The valid datapoint index.
 *
 * Remarks:
 * This function must return at least one valid datapoint index for each
 * assembly number returned by <IsiGetAssembly>. The *offset* parameter is 
 * zero-based
 * and relates to the selector number offset that is used with the enrollment 
 * of this assembly. The function operates only when the ISI engine is running.
 */
ISI_EXTERNAL_FN unsigned IsiGetDpIndex(
unsigned Assembly, 
unsigned Offset, 
unsigned PreviousIndex
);

/*
 * Callback:   IsiGetWidth
 * Returns the width in the specified assembly.
 *
 * Parameters:
 * Assembly - assembly number
 *
 * Returns:
 *  The width in the specified assembly.
 *
 * Remarks:
 * The width is equal to the number of datapoint selectors associated
 * with the assembly.
 * Applications must override the <IsiGetWidth> function to support compound
 * assemblies. This function operates only when the ISI engine is running.
 */
ISI_EXTERNAL_FN unsigned IsiGetWidth(unsigned Assembly);

/*
 * Callback:   IsiCreateCsmo
 * Constructs the IsiCsmoData portion of a CSMO Message.
 *
 * Parameters:
 * Assembly - assembly number
 * pCsmoData - pointer to a <IsiCsmoData> structure
 *
 * Remarks:
 * The width is equal to the number of datapoint selectors associated
 * with the assembly.
 * Applications must override the <IsiGetWidth> function to support compound
 * assemblies. This function operates only when the ISI engine is running.
 */
ISI_EXTERNAL_FN void IsiCreateCsmo(
        unsigned Assembly, 
        IsiCsmoData* const pCsmoData);

/*
 * Callback:   IsiGetPrimaryGroup
 * Returns the group ID for the specified assembly.
 *
 * Parameters:
 * Assembly - assembly number
 *
 * Returns:
 *  The group ID.
 *
 * Remarks:
 * The default implementation returns ISI_DEFAULT_GROUP (128). 
 * This function is only required if the
 * default implementation, or the forwardee, of <IsiCreateCsmo> is in use.
 */
ISI_EXTERNAL_FN unsigned IsiGetPrimaryGroup(unsigned Assembly);

/*
 * Callback:   IsiGetAssembly
 * Returns the number of the first and the next applicable assembly that can 
 * join the enrollment characterized by pCsmo.
 *
 * Parameters:
 * pCsmoData - pointer to a <IsiCsmoData> structure.
 * Auto - True for an automatically initiated enrollment.
 *        False for manually initiated enrollment.
 * Assembly - sets to ISI_NO_ASSEMBLY to get the first assembly.
 *            Specifies the previous assembly number to return the next 
 *            applicable assembly following the previous one.
 *
 * Returns:
 *  The assembly number.
 *
 * Remarks:
 * The function returns ISI_NO_ASSEMBLY (0xFF) if no such assembly exists, or
 * an application-defined assembly number (0 to 254). The automatic parameter
 * specifies a manually initiated enrollment (automatic = FALSE) or an 
 * automatically initiated enrollment (automatic = TRUE). The automatic flag 
 * is TRUE both for initial automatic enrollment messages (CSMA) or reminders 
 * that relate to a possibly new connection (CSMR). Automatic enrollment 
 * reminder messages that relate to existing connections on the local device 
 * are not passed to the application. The pointer provided with the *pCsmo* 
 * parameter is only valid for the duration of this function call.
 * The function operates only when the ISI engine is running.
 */
ISI_EXTERNAL_FN unsigned IsiGetAssembly(
        const IsiCsmoData* pCsmoData, 
        IzotBool Auto, 
        unsigned Assembly);

/*
 * Function: IsiInitiateAutoEnrollment
 * Starts automatic enrollment.
 *
 * Parameters:
 * pCsma - data for automatic enrollment.
 * assembly - assembly for which the enrollment should be initiated.
 *
 * Returns:
 * <LonStatusCode>
 *
 * Remarks:
 * Use this function to start automatic enrollment. The local device becomes the
 * host for the automatic connection. Automatic enrollment can replace previous
 * connections, if any. No forwarder exists for this function.
 * This function cannot be called before the *isiWarm* event has been signaled
 * in the <IsiUpdateUserInterface> callback.
 * This function does nothing when the ISI engine is stopped.
 */
ISI_EXTERNAL_FN LonStatusCode IsiInitiateAutoEnrollment(
        const IsiCsmoData* pCsmaData, 
        unsigned Assembly);

/*
 * Function: IsiIsConnected
 * Returns the status of the specified assembly whether is currently enrolled 
 * in a connection or not.
 *
 * Parameters:
 * assembly - assembly for which the connection status needs to be queried.
 *
 * Returns:
 * True - if the specified assembly is enrolled in a connection.
 * False - if the ISI engine is stopped.
 *
 * Remarks:
 * The function operates whether the ISI engine is running or not.
 */
ISI_EXTERNAL_FN IzotBool IsiIsConnected(unsigned Assembly);

/*
 * Function: IsiIsAutomaticallyEnrolled
 * Returns the status of the specified assembly whether is currently 
 * connected and if that connection was made with automatic enrollment.
 *
 * Parameters:
 * assembly - assembly for which the connection status needs to be queried.
 *
 * Returns:
 * True - if the specified assembly is enrolled in a connection and the 
 *        connection was made with automatic enrollment.
 * False - if the ISI engine is stopped or if the connection was not made with 
 *         automatic enrollment.
 *
 * Remarks:
 * The function operates whether the ISI engine is running or not.
 */
ISI_EXTERNAL_FN IzotBool IsiIsAutomaticallyEnrolled(unsigned Assembly);

/*
 * Function: IsiImplementationVersion
 * Returns the version number of this ISI implementation.
 *
 * Returns:
 * The version number of the ISI library.
 *
 * Remarks:
 * The function operates whether the ISI engine is running or not.
 */
ISI_EXTERNAL_FN unsigned IsiImplementationVersion(void);

/*
 * Function: IsiProtocolVersion
 * Returns the version of the ISI protocol supported by the ISI engine.
 *
 * Returns:
 * The version number of the ISI protocol.
 *
 * Remarks:
 * The function operates whether the ISI engine is running or not.
 */
ISI_EXTERNAL_FN unsigned IsiProtocolVersion(void);

/*
 * Callback:   IsiQueryHeartbeat
 * Returns TRUE if a heartbeat for the datapoint with the global index
 * DpIndex has been sent, and returns FALSE otherwise.
 *
 * Parameters:
 * DpIndex - datapoint index.
 *
 * Returns:
 *  True - a heartbeat for the datapoint with the global index DpIndex has 
 *         been sent.
 *  False - otherwise
 *
 * Remarks:
 */
ISI_EXTERNAL_FN IzotBool IsiQueryHeartbeat(unsigned DpIndex);

/*
 * Function: IsiIssueHeartbeat
 * Sends an update for the specified bound output datapoint and its
 * aliases, using group addressing.
 *
 * Parameters:
 * DpIndex - datapoint index for which the heart beat needs to be issued.
 *
 * Returns:
 * True - if at least one update has been propagated
 * False - no update has been propagated.
 *
 * Remarks:
 * This function sends a datapoint update for the specified datapoint
 * index and its aliases, as long as the datapoint is a bound
 * output datapoint using group addressing.
 * This function uses the address table for addressing information, but always
 * uses unacknowledged service with one repeat when issuing the heartbeat. It
 * only sends a datapoint if it uses group addressing (the default ISI
 * addressing mode) - it skips updates that use a different addressing mode.
 * This function is typically called in an <IsiQueryHeartbeat> callback. If the
 * function is called outside of this callback to create a custom heartbeat
 * scheme, the application must only call this function for bound output 
 * datapoints.
 * This function requires that the ISI engine has been started with the
 * *IsiFlagHeartbeat* flag.
 */
ISI_EXTERNAL_FN IzotBool IsiIssueHeartbeat(unsigned DpIndex);

/*
 * Callback:   IsiCreatePeriodicMsg
 * Specifies whether the application has any messages for the ISI engine to send
 * using the periodic broadcast scheduler.
 *
 * Returns:
 *  True - the application has a message to send.
 *  False - the application has no message to send
 *
 * Remarks:
 * Since the ISI engine sends periodic outgoing messages at regular intervals,
 * this function allows an application to send a message at one of the periodic 
 * message slots. The default implementation of <IsiCreatePeriodicMsg> does 
 * nothing but return FALSE.
 */
ISI_EXTERNAL_FN IzotBool IsiCreatePeriodicMsg(void);

/*
 *  Function: IsiSetTracefile
 *  Sets the filename to use for the trace logging.  When the pFilename is NULL,
 *  trace logging stops and a previously opened tracefile is closed.
 *
 *  Parameters:
 *  pFilename - the filename for the trace logging.  Sets to NULL to stop the 
 *              trace logging.
 *  append    - if it is set to true, it means to open file for writing at the 
 *              end of the file (appending) without removing the EOF marker 
 *              before writing new data to the file; creates the file first if 
 *              it doesn't exist. if it is set to false, it means to open an 
 *              empty trace file for writing. If the given file exists, its 
 *              contents are destroyed.
 *
 *  Returns:
 *  none
 *
 */
ISI_EXTERNAL_FN void IsiSetTracefile(const char* pFilename, IzotBool append);

/*
 *  Function: IsiControlCommand
 *  Initiates control enrollment request (CTRQ) message
 *
 *  Parameters:
 *  pUniqueId - the unique ID of the device to send a request for controlled 
 *              enrollment
 *  Assembly  - the assembly of the device to send a request for controlled 
 *              enrollment
 *  command   - the requested operation for a controlled enrollment request 
 *              contained in a control request (CTRQ) message.
 *
 *  Returns:
 *      none
 *
 *  Remarks:
 *  The host sends a control enrollment request message to a device to be 
 *  connected. If the device supports controlled enrollment, it responds with 
 *  control response (CTRP) message and the device itself will call appropriate 
 *  control enrollement function based on the command specified.
 *
 */
ISI_EXTERNAL_FN LonStatusCode IsiControlCommand(
        const IzotUniqueId* pId, 
        unsigned Assembly, 
        IsiControl command);

/*
 *  In IzoT, ISI callback functions are registered
 *  with a callback handler of a suitable type, and must be
 *  de-registered by calling the corresponding handler registrar
 *  with a NULL pointer for the callback.
 *
 *  For example, the IsiGetWidth event is implemented with a function
 *  of type IsiGetWidthFunction, and registered using the IsiGetWidthRegistrar
 *  API as shown in this hypothetical example:
 *
 *  typedef void (*IsiGetWidthFunction)(unsigned Assembly);
 *  extern LonStatusCode IsiGetWidthRegistrar(IsiGetWidthFunction handler);
 *
 *  unsigned myIsiGetWidthHandler(void)
 *  {
 *     return MY_WIDTH;
 *  }
 *
 *  int main(void) {
 *     ...
 *     // register width handler:
 *     IsiGetWidthRegistrar(myIsiGetWidthHandler);
 *     ...
 *     // un-register wink handler:
 *     IsiGetWidthRegistrar(NULL);
 *  }
 *
 *  The IsiDeregisterAllCallbacks() API can be used to deregister all callback
 *  functions.
 *  It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.
 */

/*
 *  Function: IsiCreateCsmoRegistrar
 *  Register the <IsiCreateCsmo>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiCreateCsmoRegistrar(IsiCreateCsmoFunction handler);

/*
 *  Function: IsiGetAssemblyRegistrar
 *  Register the <IsiGetAssembly>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiGetAssemblyRegistrar(IsiGetAssemblyFunction handler);

/*
 *  Function: IsiGetDpIndexRegistrar
 *  Register the <IsiGetDpIndex>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiGetDpIndexRegistrar(IsiGetDpIndexFunction handler);

/*
 *  Function: IsiGetPrimaryGroupRegistrar
 *  Register the <IsiGetPrimaryGroup>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiGetPrimaryGroupRegistrar(IsiGetPrimaryGroupFunction handler);

/*
 *  Function: IsiGetWidthRegistrar
 *  Register the <IsiGetWidth>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiGetWidthRegistrar(IsiGetWidthFunction handler);

/*
 *  Function: IsiQueryHeartbeatRegistrar
 *  Register the <IsiQueryHeartbeat>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiQueryHeartbeatRegistrar(IsiQueryHeartbeatFunction handler);

/*
 *  Function: IsiUpdateDiagnosticsRegistrar
 *  Register the <IsiUpdateDiagnostics>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiUpdateDiagnosticsRegistrar(IsiUpdateDiagnosticsFunction handler);

/*
 *  Function: IsiUpdateUserInterfaceRegistrar
 *  Register the <IsiUpdateUserInterface>.

 *  Returns:
 *  LonStatusCode
 * 
 */
ISI_EXTERNAL_FN LonStatusCode IsiUpdateUserInterfaceRegistrar(IsiUpdateUserInterfaceFunction handler);

/*
 *  Function: IsiDeregisterAllCallbacks
 *  Deregister all callbackGet functions.   
 *  It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.

 *  Returns:
 *  void.
 * 
 *  Remarks:
 *  Returns the IP address and port used for an IP852 interface.  
 */
ISI_EXTERNAL_FN void IsiDeregisterAllCallbacks(void);

#ifdef  __cplusplus
}
#endif

#endif  //  !defined __ISIAPI_H__
