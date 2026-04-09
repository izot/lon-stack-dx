/*
 * LonStackExample1.c
 *
 * Copyright (c) 2023-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Example 1
 * Purpose: Provides a simple example application for LON Stack.
 * Notes:   This example creates a LON Stack application with
 *          three network variables (NVs):
 *          - Heartbeat Interval Input NV (SNVT_elapsed_tm)
 *          - Flow 1 Input NV (SNVT_flow_p)
 *          - Flow 2 Input NV (SNVT_flow_f)
 *          The example configures the LON Stack interface and
 *          control data structures, and defines the NVs with
 *          appropriate definitions and configurations.
 *          The example does not include application logic for
 *          handling NV updates or other functionality.
 *          This file is intended to be used as a starting point
 *          for testing LON Stack ports to new platforms and for
 *          developing LON Stack applications.
 */

#include "LonStackExample1.h"   // Example declarations

// LON Stack Interface and Control Data Versions
#define STACK_INTERFACEDATA_VERSION    0
#define STACK_CONTROLDATA_VERSION      0

/*****************************************************************
 * Section: Globals
 *****************************************************************/
static uint8_t siDataBuffer[300];

static const IzotStackInterfaceData LonStackInterface = {
   STACK_INTERFACEDATA_VERSION, // Format version number
   0xABCD2552,                  // 32-bit unique application identifier
   {0x9F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01},    // Program ID
   9,                           // Number of static NVs
   9,                           // Max number of static + dynamic NVs
   2,                           // Number of domains (1 or 2)
   15,                          // Number of address table entries (0 -- 4096)
   3,                           // Number of alias table entries (0 -- 8192)
   0,                           // Number of bindable message tags (0 -- 4096)
   "LON Stack Example 1",       // Device self-documentation (SD) string
   10,                          // Average number of bytes to reserve for
                                // dynamic NV SD data
   siDataBuffer,                // Pointer to self-identification (SI) data
   sizeof(siDataBuffer)         // Size of SI data in bytes
};

static const IzotControlData LonStackControlData = {
   STACK_CONTROLDATA_VERSION,   // Format version number
   0,                           // See IZOT_CONTROL_FLAG
   10,                          // Number of seconds to wait after receiving
                                // an update to non-volatile config data before
                                // writing the data (1 -- 60)
   {  // Transceiver type and communication parameters
      IzotTransceiverTypeDefault,   // Transceiver type -- use default for LON/IP
      {                         // Comm parameters -- not used for LON/IP
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
      }
   },
   {
      {  // Application buffer configuration
         1,                     // Number of priority output message buffers (1 -- 100)
         5,                     // Number of non-priority output message buffers (0 -- 100)
         10                     // Number of input message buffers (0 -- 100)
      },
      {  // Link-layer buffer configuration (shared by input and output)
         5                      // Number of link-layer buffers (1 -- 100)
      },
      {  /* Transceiver buffer configuration */
         114,                   // Input network buffer size (66 minimum) 
         114,                   // Output network buffer size (66 minimum)
         2,                     // Number of priority output network buffers
         2,                     // Number of non-priority output network buffers
         5                      // Number of input network buffers
      }
   },
   30,                          // Number of receive transaction records (1 -- 200)
   10,                          // Number of transmit transaction records (1 -- 8192)
   24576                        // Transmit transaction ID lifetime
};

/*****************************************************************
 * Section: Network Variable Definitions
 *****************************************************************/
// SNVT_elapsed_tm heartbeatIn -- Heartbeat Interval Input NV; default to 2 seconds
SNVT_elapsed_tm heartbeatIn = {{0, 0}, 0, 0, 3, {0, 0}};
#define HEARTBEAT_IN_ADDRESS 0
#define HEARTBEAT_IN_SELECTOR 0
IzotDatapointDefinition heartbeatInDef; // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig heartbeatInConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* heartbeatInName = "heartbeatIn";
const char* heartbeatInSD = "Heartbeat Interval Input";

// SNVT_flow_p flow1In -- Flow 1 Input NV
SNVT_flow_p flow1In = {0, 0};
#define FLOW1_IN_ADDRESS 0
#define FLOW1_IN_SELECTOR 1
IzotDatapointDefinition flow1InDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig flow1InConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* flow1InName = "flow1In";
const char* flow1InSD = "Flow 1 Input";

// SNVT_flow_f flow2In -- Flow 2 Input NV
SNVT_flow_f flow2In;
#define FLOW2_IN_ADDRESS 0
#define FLOW2_IN_SELECTOR 2
IzotDatapointDefinition flow2InDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig flow2InConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* flow2InName = "flow2In";
const char* flow2InSD = "Flow 2 Input";

// SNVT_temp_p temp1In -- Temperature 1 Input NV
SNVT_temp_p temp1In;
#define TEMP1_IN_ADDRESS 0
#define TEMP1_IN_SELECTOR 3
IzotDatapointDefinition temp1InDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig temp1InConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* temp1InName = "temp1In";
const char* temp1InSD = "Temp 1 Input";

// SNVT_temp_f temp2In -- Temperature 2 Input NV
SNVT_temp_f temp2In;
#define TEMP2_IN_ADDRESS 0
#define TEMP2_IN_SELECTOR 4
IzotDatapointDefinition temp2InDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig temp2InConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* temp2InName = "temp2In";
const char* temp2InSD = "Temp 2 Input";

// SNVT_flow_p flow1Out -- Flow 1 Output NV
SNVT_flow_p flow1Out;
#define FLOW1_OUT_ADDRESS 0
#define FLOW1_OUT_SELECTOR 1
IzotDatapointDefinition flow1OutDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig flow1OutConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* flow1OutName = "flow1Out";
const char* flow1OutSD = "Flow 1 Output";

// SNVT_flow_f flow2Out -- Flow 2 Output NV
SNVT_flow_f flow2Out;
#define FLOW2_OUT_ADDRESS 0
#define FLOW2_OUT_SELECTOR 2
IzotDatapointDefinition flow2OutDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig flow2OutConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* flow2OutName = "flow2Out";
const char* flow2OutSD = "Flow 2 Output";

// SNVT_temp_p temp1Out -- Temperature 1 Output NV
SNVT_temp_p temp1Out;
#define TEMP1_OUT_ADDRESS 0
#define TEMP1_OUT_SELECTOR 3
IzotDatapointDefinition temp1OutDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig temp1OutConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* temp1OutName = "temp1Out";
const char* temp1OutSD = "Temp 1 Output";

// SNVT_temp_f temp2Out -- Temperature 2 Output NV
SNVT_temp_f temp2Out;
#define TEMP2_OUT_ADDRESS 0
#define TEMP2_OUT_SELECTOR 4
IzotDatapointDefinition temp2OutDef;    // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig temp2OutConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* temp2OutName = "temp2Out";
const char* temp2OutSD = "Temp 2 Output";

/*****************************************************************
 * Section: Timers
 *****************************************************************/
static LonTimer HeartbeatTimer;

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
LonStatusCode SetUpAddressTable(void);
LonStatusCode SetUpStaticNVs(void);
void Example1DatapointUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void SetHeartbeatTimer(void);
void HeartbeatInUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Flow1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Flow2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Temp1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Temp2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/
/*
 * Implements the main entry point for this application.
 * Parameters:  None
 * Returns:     None
 * Notes:       Define INCLUDE_EXAMPLE_MAIN to include this function.
 *              This function creates, configures, and starts the LON Stack
 *              and then starts the main loop for the example application.
 *              The main loop calls the LON Stack event pump and then handles
 *              any pending application events.
 */
#ifdef INCLUDE_EXAMPLE_MAIN
void main() {
    LonStatusCode status = LonStatusNoError;

    // Set up Example1
    status = SetUpExample1();

    // Main loop
    while (LON_SUCCESS(status)) {
        // Execute one pass of the LON Stack event pump
        status = LoopExample1();
    }

    OsalPrintLog(ERROR_LOG, status, "Example1 main: Application terminated due to error");
}
#endif

/*
 * Initializes the LON Stack and application for Example 1.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if successful, otherwise a <LonStatusCode> error code.
 */
LonStatusCode SetUpExample1(void)
{
    LonStatusCode status = LonStatusNoError;
    IzotBool success = TRUE;
    IzotBool domainId = EXAMPLE_DOMAIN_ID;  // Use a 1-byte domain

    // Create, configure, and start the LON Stack
    success =  LON_SUCCESS(status = IzotCreateStack(&LonStackInterface, &LonStackControlData))
            && LON_SUCCESS(status = SetUpStaticNVs()) 
            && LON_SUCCESS(status = IzotStartStack())
            && LON_SUCCESS(status = IzotUpdateDomain(0, EXAMPLE_DOMAIN_LENGTH, (IzotByte*) &domainId, EXAMPLE_SUBNET, EXAMPLE_NODE))
            && LON_SUCCESS(status = SetUpAddressTable())
            && LON_SUCCESS(status = IzotDatapointUpdateOccurredRegistrar(&Example1DatapointUpdateOccurred));

    if (success) {
        // Start the heartbeat timer using the heartbeatIn NV default value
        SetHeartbeatTimer();
    }

    return status;
}

/*
 * Implements the loop code for a single pass of the Example 1 event loop.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if successful, otherwise a LonStatusCode error code.
 * Notes:
 *   On startup, send INITIAL_SERVICE_COUNT Service messages to indicate that
 *   the LON application has started; this is not required, but is useful for
 *   testing purposes.  Set INITIAL_SERVICE_COUNT to 0 to skip sending these
 *   messages.
 */
#define INITIAL_SERVICE_COUNT 5
LonStatusCode LoopExample1(void)
{
    LonStatusCode status = LonStatusNoError;
    static unsigned service_count = INITIAL_SERVICE_COUNT;

    // LON Stack event pump
    if (!LON_SUCCESS(status = IzotEventPump())) {
        OsalPrintLog(ERROR_LOG, status, "LoopExample1: LON stack failure");
        return status;
    }

    // ToDo -- add application-specific event-handlers here, or in separate
    // tasks if available on the target platform; keep these handlers under
    // min(10, ((InputBufferCount - 1) * 1000) / MaxPacketRate) milliseconds
    if (LonTimerExpired(&HeartbeatTimer)) {
        if (service_count > 0) {
            OsalPrintLog(INFO_LOG, status, "LoopExample1: Send Service message, attempt number %u", INITIAL_SERVICE_COUNT - service_count + 1);
            if (!LON_SUCCESS(IzotSendServiceMessage())) {
                OsalPrintLog(ERROR_LOG, status, "LoopExample1: Failed to send initial service message");
                return status;
            }
            service_count--;
        } else if (ElapsedTimeToMs(&heartbeatIn) > 0) {
            // Send heartbeats
            uint16_t value;
            value = IZOT_GET_UNSIGNED_WORD(*(SNVT_flow_p*) flow1OutDef.PValue) + 100;
            IZOT_SET_UNSIGNED_WORD(*(SNVT_flow_p*) flow1OutDef.PValue, value);
            if (!LON_SUCCESS(IzotPropagateByIndex(flow1OutDef.NvIndex))) {
                OsalPrintLog(ERROR_LOG, status, "LoopExample1: Failed to propagate flow1Out");
            }
            // TBD -- increment flow2Out
            if (!LON_SUCCESS(IzotPropagateByIndex(flow2OutDef.NvIndex))) {
                OsalPrintLog(ERROR_LOG, status, "LoopExample1: Failed to propagate flow2Out");
            }
            value = IZOT_GET_UNSIGNED_WORD(*(SNVT_temp_p*) temp1OutDef.PValue) + 100;
            IZOT_SET_UNSIGNED_WORD(*(SNVT_temp_p*) temp1OutDef.PValue, value);
            if (!LON_SUCCESS(IzotPropagateByIndex(temp1OutDef.NvIndex))) {
                OsalPrintLog(ERROR_LOG, status, "LoopExample1: Failed to propagate temp1Out");
            }
            // TBD -- increment temp2Out
            if (!LON_SUCCESS(IzotPropagateByIndex(temp2OutDef.NvIndex))) {
                OsalPrintLog(ERROR_LOG, status, "LoopExample1: Failed to propagate temp2Out");
            }
        }
    }

    return status;
}

/*
 * Set up a simple single address table for Example 1.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if successful, otherwise a LonStatusCode error code.
 */
LonStatusCode SetUpAddressTable(void)
{
    IzotAddress AddressTableEntry = {}; 
    LonStatusCode status = LonStatusNoError;

    AddressTableEntry.SubnetNode.Type = IzotAddressSubnetNode;
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_DOMAIN, 0);
    AddressTableEntry.SubnetNode.Subnet = EXAMPLE_TARGET_SUBNET;
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_NODE, EXAMPLE_TARGET_NODE);
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_REPEAT_TIMER, 0);
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_RETRY, 0);

    status = IzotUpdateAddressConfig(0, &AddressTableEntry);

    return (status);
}

/*
 * Creates and binds the static NVs for Example 1.
 * Parameters:
 *   None
 * Returns:
 *   LonStatusNoError if successful, otherwise a LonStatusCode error code.
 */
LonStatusCode SetUpStaticNVs(void)
{
    LonStatusCode status = LonStatusNoError;
    IzotBool success = TRUE;

    // Specify static configuration with: IzotDatapointSetup(IzotDatapointDefinition* pDatapointDef, volatile void const *value, IzotDatapointSize size, 
    //      uint16_t snvtId, uint16_t arrayCount, const char *name, const char *sdString, uint8_t maxRate, uint8_t meanRate, const uint8_t *ibol)
    // Specify flag configuration with: IzotDatapointConfiguration(IzotDatapointConfig DatapointConfig, IzotBool priority, IzotDatapointDirection direction, 
    //      IzotBool authentication, IzotBool aes)
    // Specify connection configuration with: IzotDatapointBind(IzotDatapointDefinition* pDatapointDef, IzotByte address, IzotWord selector, 
    //      IzotBool turnAround, IzotServiceTypeservice)

    // SNVT_elapsed_tm heartbeatIn NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&heartbeatInDef, &heartbeatIn, sizeof(heartbeatIn), SNVT_flow_p_index, 0, heartbeatInName,  
               heartbeatInSD, IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&heartbeatInDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&heartbeatInDef))
            && LON_SUCCESS(status = IzotDatapointBind(heartbeatInDef.NvIndex, HEARTBEAT_IN_ADDRESS, HEARTBEAT_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_flow_p flow1In NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&flow1InDef, &flow1In, sizeof(flow1In), SNVT_flow_p_index, 0, flow1InName,  
               flow1InSD, IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&flow1InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&flow1InDef))
            && LON_SUCCESS(status = IzotDatapointBind(flow1InDef.NvIndex, FLOW1_IN_ADDRESS, FLOW1_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_flow_f flow2In NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&flow2InDef, &flow2In, sizeof(flow2In), SNVT_flow_f_index, 0, flow2InName,  
               flow2InSD, IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&flow2InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&flow2InDef))
            && LON_SUCCESS(status = IzotDatapointBind(flow2InDef.NvIndex, FLOW2_IN_ADDRESS, FLOW2_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_temp_p temp1In NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&temp1InDef, &temp1In, sizeof(temp1In), SNVT_flow_p_index, 0, temp1InName, temp1InSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&temp1InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&temp1InDef))
            && LON_SUCCESS(status = IzotDatapointBind(temp1InDef.NvIndex, TEMP1_IN_ADDRESS, TEMP1_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_flow_f temp2In NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&temp2InDef, &temp2In, sizeof(temp2In), SNVT_flow_f_index, 0, temp2InName, temp2InSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&temp2InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&temp2InDef))
            && LON_SUCCESS(status = IzotDatapointBind(temp2InDef.NvIndex, TEMP2_IN_ADDRESS, TEMP2_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_flow_p flow1Out NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&flow1OutDef, &flow1Out, sizeof(flow1Out), SNVT_flow_p_index, 0, flow1OutName, flow1OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&flow1OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&flow1OutDef))
            && LON_SUCCESS(status = IzotDatapointBind(flow1OutDef.NvIndex, FLOW1_OUT_ADDRESS, FLOW1_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_flow_f flow2Out NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&flow2OutDef, &flow2Out, sizeof(flow2Out), SNVT_flow_f_index, 0, flow2OutName, flow2OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&flow2OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&flow2OutDef))
            && LON_SUCCESS(status = IzotDatapointBind(flow2OutDef.NvIndex, FLOW2_OUT_ADDRESS, FLOW2_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_temp_p temp1Out NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&temp1OutDef, &temp1Out, sizeof(temp1Out), SNVT_flow_p_index, 0, temp1OutName, temp1OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&temp1OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&temp1OutDef))
            && LON_SUCCESS(status = IzotDatapointBind(temp1OutDef.NvIndex, TEMP1_OUT_ADDRESS, TEMP1_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    // SNVT_flow_f temp2Out NV
    success =  LON_SUCCESS(status = IzotDatapointSetup(&temp2OutDef, &temp2Out, sizeof(temp2Out), SNVT_flow_f_index, 0, temp2OutName, temp2OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && LON_SUCCESS(status = IzotDatapointFlags(&temp2OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && LON_SUCCESS(status = IzotRegisterStaticDatapoint(&temp2OutDef))
            && LON_SUCCESS(status = IzotDatapointBind(temp2OutDef.NvIndex, TEMP2_OUT_ADDRESS, TEMP2_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(status);
    }

    return(status);
}

/*
 * Handles network variable (NV) updates for Example 1.
 * Parameters:
 *   index: The index of the updated NV.
 *   pSourceAddress: Pointer to the source address of the update.
 * Returns:
 *   None
 */
void Example1DatapointUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Compare index to the NV definition NvIndex field and call the appropriate handler.
    if (index == heartbeatInDef.NvIndex) {
        HeartbeatInUpdateOccurred(index, pSourceAddress);
    } else if (index == flow1InDef.NvIndex) {
        Flow1InUpdateOccurred(index, pSourceAddress);
    } else if (index == flow2InDef.NvIndex) {
        Flow2InUpdateOccurred(index, pSourceAddress);
    } else if (index == temp1InDef.NvIndex) {
        Temp1InUpdateOccurred(index, pSourceAddress);
    } else if (index == temp2InDef.NvIndex) {
        Temp2InUpdateOccurred(index, pSourceAddress);
    }
}

/*
 * Handles updates to the heartbeatIn network variable (NV).
 * Parameters:
 *   index: The index of the updated NV.
 *   pSourceAddress: Pointer to the source address of the update.
 * Returns:
 *   None
 */
void HeartbeatInUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Change heartbeat timer interval to the updated value.
    SetHeartbeatTimer();
}

/*
 * Handles updates to the flow1In network variable (NV).
 * Parameters:
 *   index: The index of the updated NV.
 *   pSourceAddress: Pointer to the source address of the update.
 * Returns:
 *   None
 */
void Flow1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated flow1In to flow1Out.
    memcpy(&flow1Out, &flow1In, sizeof(SNVT_flow_p));
}

/*
 * Handles updates to the flow2In network variable (NV).
 * Parameters:
 *   index: The index of the updated NV.
 *   pSourceAddress: Pointer to the source address of the update.
 * Returns:
 *   None
 */
void Flow2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated flow2In to flow2Out.
    memcpy(&flow2Out, &flow2In, sizeof(SNVT_flow_f));
}

/*
 * Handles updates to the temp1In network variable (NV).
 * Parameters:
 *   index: The index of the updated NV.
 *   pSourceAddress: Pointer to the source address of the update.
 * Returns:
 *   None
 */
void Temp1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated temp1In to temp1Out.
    memcpy(&temp1Out, &temp1In, sizeof(SNVT_temp_p));
}

/*
 * Handles updates to the temp2In network variable (NV).
 * Parameters:
 *   index: The index of the updated NV.
 *   pSourceAddress: Pointer to the source address of the update.
 * Returns:
 *   None
 */
void Temp2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated temp2In to temp2Out.
    memcpy(&temp2Out, &temp2In, sizeof(SNVT_temp_p));
}

/*
 * Sets the heartbeat timer interval based on the heartbeatIn NV value.
 * Parameters: None
 * Returns: None
 * Notes
 *   Only the seconds and milliseconds fields of the heartbeatIn NV are used to
 *   set the timer interval.
 */
void SetHeartbeatTimer(void)
{
    uint32_t heartbeatInterval = ((uint32_t)heartbeatIn.second * 1000) + (IZOT_GET_UNSIGNED_WORD(heartbeatIn.millisecond));

    SetLonRepeatTimer(&HeartbeatTimer, heartbeatInterval, heartbeatInterval);
}
