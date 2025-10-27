//
// LonStackExample1.c
//
// Copyright (C) 2023 EnOcean
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

// Title: LON Stack Example 1
//
// Abstract:
// Simple example application for the LON Stack.

#include "LonStackExample1.h"   // Example declarations


//
// Section: Globals
//

// LON Stack Interface and Control Data
#define STACK_INTERFACEDATA_VERSION    0
#define STACK_CONTROLDATA_VERSION      0

static uint8_t siDataBuffer[300];

static const IzotStackInterfaceData LonStackInterface = {
   STACK_INTERFACEDATA_VERSION, // Format version number
   0x5a7abe7f,                  // 32-bit unique application identifier
   {0x9F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01},    // Program ID
   9,                           // Number of static NVs
   9,                           // Max number of static + dynamic NVs
   2,                           // Number of domains (1 or 2)
   15,                          // Number of address table entries (0 -- 4096)
   3,                           // Number of alias table entries (0 -- 8192)
   0,                           // Number of bindable message tags (0 -- 4096)
   "Custom",                    // Device self-documentation (SD) string
   10,                          // Average number of bytes to reserve for
                                // dynamic NV SD data
   siDataBuffer,                // Pointer to self-identification (SI) data
   sizeof(siDataBuffer)         // Size of SI data in bytes
};

static const IzotControlData LonStackControlData = {
   STACK_CONTROLDATA_VERSION,   // Format version number
   0,                           // See IZOT_CONTROL_FLAG
   5,                           // Number of seconds to wait after receiving
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

//
// Section: NV Definitions
//

// SNVT_elapsed_tm heartbeatIn -- Heartbeat Interval Input NV; default to 2 seconds
SNVT_elapsed_tm heartbeatIn = {{0, 0}, 0, 0, 2, {0, 0}};
#define HEARTBEAT_IN_ADDRESS 0
#define HEARTBEAT_IN_SELECTOR 0
IzotDatapointDefinition heartbeatInDef; // Used to create definition and get NV index.  Copied into NV config table.
// IzotDatapointConfig heartbeatInConfig;  // Used to create NV configuration.  Copied into NV config table.
const char* heartbeatInName = "heartbeatIn";
const char* heartbeatInSD = "Heartbeat Interval Input";

// SNVT_flow_p flow1In -- Flow 1 Input NV
SNVT_flow_p flow1In;
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


//
// Section: Constants
//

//
// Section: Timers
//

static LonTimer HeartbeatTimer;


//
// Section: Prototypes
//

LonStatusCode SetUpAddressTable(void);
LonStatusCode SetUpStaticNVs(void);
void Example1DatapointUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void SetHeartbeatTimer(void);
void HeartbeatInUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Flow1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Flow2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Temp1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);
void Temp2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress);


//
// Section: Functions
//


// Function: main()
// Main loop for this application.  Create, confiugure, and start the LON Stack and then
// start the main loop.  The main loop calls the LON Stack event pump and then handles
// any pending application events.

#ifdef INCLUDE_EXAMPLE_MAIN
void main() {
    LonStatusCode lastError = LonStatusNoError;
    IzotBool success = TRUE;

    // Set up Example1
    SetUpExample1();

    // Main loop
    while (success) {
        success = IZOT_SUCCESS(lastError = LoopExample1());
        // LON Stack event pump
        LoopExample1();
    }
}
#endif


// Function: SetUpExample1()
// Initial setup for this application, call from main() when 
// using C.
//
// Returns: <LonStatusCode>

LonStatusCode SetUpExample1(void)
{
    LonStatusCode lastError = LonStatusNoError;
    IzotBool success = TRUE;
    IzotBool domainId = EXAMPLE_DOMAIN_ID;  // Use a 1-byte domain

    // Create, configure, and start the LON Stack
    success =  IZOT_SUCCESS(lastError = IzotCreateStack(&LonStackInterface, &LonStackControlData)) 
            && IZOT_SUCCESS(lastError = SetUpStaticNVs()) 
            && IZOT_SUCCESS(lastError = IzotStartStack())
            && IZOT_SUCCESS(lastError = IzotUpdateDomain(0, EXAMPLE_DOMAIN_LENGTH, (IzotByte*) &domainId, EXAMPLE_SUBNET, EXAMPLE_NODE))
            && IZOT_SUCCESS(lastError = SetUpAddressTable())
            && IZOT_SUCCESS(lastError = IzotDatapointUpdateOccurredRegistrar(&Example1DatapointUpdateOccurred));

    if (success) {
        // Start the heartbeat timer using the heartbeatIn NV default value
        SetHeartbeatTimer();
    }

    return lastError;
}


// Function: LoopExample1()
// Event loop code for a single pass of the Example 1 event loop.
// Call from an event loop when using C.

LonStatusCode LoopExample1(void)
{
    LonStatusCode ret = LonStatusNoError;

    // LON Stack event pump
    ret = IzotEventPump();

    // ToDo -- add application-specific event-handlers here, or in separate tasks if available.
    // Keep these handlers under min(10, ((InputBufferCount - 1) * 1000) / MaxPacketRate) milliseconds.

    if (LonTimerExpired(&HeartbeatTimer)) {
        // Send heartbeats
        uint16_t value;

        value = IZOT_GET_UNSIGNED_WORD(*(SNVT_flow_p*) flow1OutDef.PValue) + 100;
        IZOT_SET_UNSIGNED_WORD(*(SNVT_flow_p*) flow1OutDef.PValue, value);
        IzotPropagateByIndex(flow1OutDef.NvIndex);

        // TBD -- increment flow2Out
        IzotPropagateByIndex(flow2OutDef.NvIndex);

        value = IZOT_GET_UNSIGNED_WORD(*(SNVT_temp_p*) temp1OutDef.PValue) + 100;
        IZOT_SET_UNSIGNED_WORD(*(SNVT_temp_p*) temp1OutDef.PValue, value);
        IzotPropagateByIndex(temp1OutDef.NvIndex);

        // TBD -- increment temp2Out
        IzotPropagateByIndex(temp2OutDef.NvIndex);
    }
    return ret;
}

// Function: SetUpAddressTable()
// Sets up a simple single address table for this application.
//
// Returns: <LonStatusCode>

LonStatusCode SetUpAddressTable(void)
{
    IzotAddress AddressTableEntry = {}; 
    LonStatusCode lastError = LonStatusNoError;

    AddressTableEntry.SubnetNode.Type = IzotAddressSubnetNode;
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_DOMAIN, 0);
    AddressTableEntry.SubnetNode.Subnet = EXAMPLE_TARGET_SUBNET;
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_NODE, EXAMPLE_TARGET_NODE);
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_REPEAT_TIMER, 0);
    IZOT_SET_ATTRIBUTE(AddressTableEntry.SubnetNode, IZOT_ADDRESS_SN_RETRY, 0);

    lastError = IzotUpdateAddressConfig(0, &AddressTableEntry);

    return (lastError);
}


// Function: SetUpStaticNVs()
// Creates and binds the static NVs for this application.
//
// Returns: <LonStatusCode>

LonStatusCode SetUpStaticNVs(void)
{
    LonStatusCode lastError = LonStatusNoError;
    IzotBool success = TRUE;

    // Specify static configuration with: IzotDatapointSetup(IzotDatapointDefinition* pDatapointDef, volatile void const *value, IzotDatapointSize size, 
    //      uint16_t snvtId, uint16_t arrayCount, const char *name, const char *sdString, uint8_t maxRate, uint8_t meanRate, const uint8_t *ibol)
    // Specify flag configuration with: IzotDatapointConfiguration(IzotDatapointConfig DatapointConfig, IzotBool priority, IzotDatapointDirectiond irection, 
    //      IzotBool authentication, IzotBool aes)
    // Specify connection configuration with: IzotDatapointBind(IzotDatapointDefinition* pDatapointDef, IzotByte address, IzotWord selector, 
    //      IzotBool turnAround, IzotServiceTypeservice)

    // SNVT_elapsed_tm heartbeatIn NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&heartbeatInDef, &heartbeatIn, sizeof(heartbeatIn), SNVT_flow_p_index, 0, heartbeatInName,  
               heartbeatInSD, IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&heartbeatInDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&heartbeatInDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(heartbeatInDef.NvIndex, HEARTBEAT_IN_ADDRESS, HEARTBEAT_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_flow_p flow1In NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&flow1InDef, &flow1In, sizeof(flow1In), SNVT_flow_p_index, 0, flow1InName,  
               flow1InSD, IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&flow1InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&flow1InDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(flow1InDef.NvIndex, FLOW1_IN_ADDRESS, FLOW1_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_flow_f flow2In NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&flow2InDef, &flow2In, sizeof(flow2In), SNVT_flow_f_index, 0, flow2InName,  
               flow2InSD, IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&flow2InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&flow2InDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(flow2InDef.NvIndex, FLOW2_IN_ADDRESS, FLOW2_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_temp_p temp1In NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&temp1InDef, &temp1In, sizeof(temp1In), SNVT_flow_p_index, 0, temp1InName, temp1InSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&temp1InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&temp1InDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(temp1InDef.NvIndex, TEMP1_IN_ADDRESS, TEMP1_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_flow_f temp2In NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&temp2InDef, &temp2In, sizeof(temp2In), SNVT_flow_f_index, 0, temp2InName, temp2InSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&temp2InDef, FALSE, IzotDatapointDirectionIsInput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&temp2InDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(temp2InDef.NvIndex, TEMP2_IN_ADDRESS, TEMP2_IN_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_flow_p flow1Out NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&flow1OutDef, &flow1Out, sizeof(flow1Out), SNVT_flow_p_index, 0, flow1OutName, flow1OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&flow1OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&flow1OutDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(flow1OutDef.NvIndex, FLOW1_OUT_ADDRESS, FLOW1_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_flow_f flow2Out NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&flow2OutDef, &flow2Out, sizeof(flow2Out), SNVT_flow_f_index, 0, flow2OutName, flow2OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&flow2OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&flow2OutDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(flow2OutDef.NvIndex, FLOW2_OUT_ADDRESS, FLOW2_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_temp_p temp1Out NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&temp1OutDef, &temp1Out, sizeof(temp1Out), SNVT_flow_p_index, 0, temp1OutName, temp1OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&temp1OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&temp1OutDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(temp1OutDef.NvIndex, TEMP1_OUT_ADDRESS, TEMP1_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    // SNVT_flow_f temp2Out NV
    success =  IZOT_SUCCESS(lastError = IzotDatapointSetup(&temp2OutDef, &temp2Out, sizeof(temp2Out), SNVT_flow_f_index, 0, temp2OutName, temp2OutSD, 
               IZOT_DATAPOINT_RATE_UNKNOWN, IZOT_DATAPOINT_RATE_UNKNOWN, NULL))
            && IZOT_SUCCESS(lastError = IzotDatapointFlags(&temp2OutDef, FALSE, IzotDatapointDirectionIsOutput, FALSE, FALSE, FALSE, FALSE))
            && IZOT_SUCCESS(lastError = IzotRegisterStaticDatapoint(&temp2OutDef))
            && IZOT_SUCCESS(lastError = IzotDatapointBind(temp2OutDef.NvIndex, TEMP2_OUT_ADDRESS, TEMP2_OUT_SELECTOR, FALSE, IzotServiceAcknowledged));
    if (!success) {
        return(lastError);
    }

    return(lastError);
}


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


void HeartbeatInUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Change heartbeat timer interval to the updated value.
    SetHeartbeatTimer();
}


void Flow1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated flow1In to flow1Out.
    memcpy(&flow1Out, &flow1In, sizeof(SNVT_flow_p));
}


void Flow2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated flow2In to flow2Out.
    memcpy(&flow2Out, &flow2In, sizeof(SNVT_flow_f));
}


void Temp1InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated temp1In to temp1Out.
    memcpy(&temp1Out, &temp1In, sizeof(SNVT_temp_p));
}


void Temp2InUpdateOccurred(const unsigned index, const IzotReceiveAddress* const pSourceAddress)
{
    // Copy updated temp2In to temp2Out.
    memcpy(&temp2Out, &temp2In, sizeof(SNVT_temp_p));
}


// Set the repeating heartbeat timer using only the seconds and milliseconds values from the heartbeatIn NV.
void SetHeartbeatTimer(void)
{
    IzotUbits32 heartbeatInterval = (heartbeatIn.second * 1000) + (IZOT_GET_UNSIGNED_WORD(heartbeatIn.millisecond));

    SetLonRepeatTimer(&HeartbeatTimer, heartbeatInterval, heartbeatInterval);
}


