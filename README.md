# lon-stack-dx

The EnOcean LON Stack DX enables Industrial IoT developers to build networks of communicating devices using any processor supporting the C programming language. Devices with the LON Stack DX can exchange data with each other on an easy-to-use publish-subscribe data model over IP or native LON channels. LON devices can collect data from physical sensors built to monitor the built environment including temperature, humidity, light-level, power-consumption, or moisture, and make the data available to other LON devices in the same network. Using data received from other LON devices or local sensors, LON devices can also control physical actuators such as LED dimmers, motor controllers, damper controllers, and solenoids. The LON protocol is an open standard defined by the ISO/IEC 14908 series of standards.

A larger implementation of the LON stack is available in the LON Stack EX available at [Izot Lon Stack EX](https://github.com/izot/lon-stack-ex).  The LON Stack EX implements optional features of the LON protocol that are suitable for edge servers and workstations implementing the LON protocol.

## Key Architectural Components

- **Abstraction Layers**: `abstraction/IzotCal.c`, `abstraction/IzotHal.c`, `abstraction/IzotOsal.c` provide platform, hardware, and OS abstraction.
- **API Entry Points**: `IzotApi.c` and `include/izot/IzotApi.h` expose the main stack API. Key functions include `IzotCreateStack()`, `IzotRegisterStaticDatapoint()`, `IzotStartStack()`, and `IzotEventPump()` for the stack lifecycle.
- **Protocol Layers**: Located under `lcs/`. The layers handle Application/Presentation, Session/Transport, Network, and Data Link concerns.
- **LON/IP**: `lon_udp/ipv4_to_lon_udp.c` implements the LON/IP over Ethernet or Wi-Fi data link layer.
- **LON Native**: `lon_usb/lon_usb_link.c` implements the LON over USB data link layer using the U60 network interface with MIP/U50 or MIP/U61 firmware.
- **Persistence**: `persistence/lon_persistence.c` and `persistence/storage_persistence.c` handle flash and persistent storage.
- **Example**: `example/LonStackExample1.c` implements a simple example for the LON stack.
- **Test**: `test/LonStackTest.scr` implements a test script for NodeUtil.

## Developer Workflows

- **Build**: The project uses CMake (`CMakeLists.txt`, `CMakePresets.json`). Source and header files are explicitly listed.
- **Stack Initialization**: A typical application should first call `IzotCreateStack()`, then register datapoints, and finally call `IzotStartStack()`. The main loop must recurrently call `IzotEventPump()`.
- **Datapoint (Network Variable) Handling**:
  - Declare a C variable and an `IzotDatapointDefinition`.
  - Register it using `IzotRegisterStaticDatapoint()`.
  - To send an update, modify the C variable and call `IzotPropagate()` or `IzotPropagateByIndex()`.
  - To receive updates, register a handler using `IzotDatapointUpdateOccurredRegistrar()`.
- **Application Messaging**:
  - Send messages with `IzotSendMsg()`.
  - Receive messages by registering a handler with `IzotMsgArrivedRegistrar()`.

## Key Files & Directories

- `IzotApi.c`: Core LON stack API implementation.
- `lcs/`: Main LON protocol stack implementation files.
- `lon_udp/`: LON-over-UDP (LON/IP over Ethernet or Wi-Fi) implementation.
- `lon_usb/`: LON-over-USB (LON over U60 with MIP/U50 or MIP/U61) implementation.
- `abstraction/`: OS and hardware abstraction layers.
- `persistence/`: Data persistence logic.
- `include/`: Public header files for the stack.
- `example/`: Example applications.
- `test/`: Test script.
- `CMakeLists.txt`: Main build configuration file.
- `README.md`: General project information.

## File Structure

Following is a high-level summary of the LON Stack DX file structure.

### Definitions

- IzotCal.c/h -- IP connectivity abstraction layer.
- IzotHal.c/h -- Hardware abstraction layer.
- IzotOsal.c/h -- Operating system abstraction layer.
- IzotPlatform.h -- Platform dependant flags and basic data types.
- lcs_node.c -- LON Stack data structure access functions.

### Implementation

- IzotAPI.c/h -- Top-level API; this is the primary application interface to LON Stack
- IzotCreateStack() -- LON Stack initialization
- IzotRegisterStaticDatapoint() -- Static datapoint registration
- IzotRegisterMemoryWindow() -- Virtual memory window registration
- IzotStartStack() -- Starts LON Stack
- IzotEventPump() -- LON Stack event pump API for the main event loop
  - Lcs.c -- LON Stack main entry points
    - LCS_Service() -- LON Stack event pump for the main event loop
      - lcs_app.c -- Layer 7 - 6 (application and presentation layers)
        - APPSend() -- Send application and presentation layer message
        - APPReceive() -- Receive application and presentation layer message
      - lcs_tsa.c -- Layers 5 - 4 (session and transport layers)
        - SNSend() -- Session layer send processing
        - SNReceive() -- Session layer receive processing
        - TPSend() -- Transport layer send processing
        - TPReceive() -- Transport layer receive processing
        - AuthSend() -- Authenticated message send processing
        - AuthReceive() -- Authenticated message receive processing
      - lcs_tcs.c -- Transaction control sublayer used by Layers 4 and 5
      - lcs_network.c -- Layer 3 (network layer)
        - NWSend() -- Network layer send processing
        - NWReceive() -- Network layer receive processing
      - lcs_link.c -- Layer 2 (data link layer) for a LON USB or MIP data link
        - LKSend() -- LON USB or MIP data link layer send processing
        - LKReceive() -- LON USB or MIP data link layer receive processing
      - ip_v4_to_lon_udp.c -- Layer 2 (data link layer) for a LON UDP/IP data link
        - LsUDPSend() -- LON UDP/IP data link layer send processing
        - LsUDPReceive() -- LON UDP/IP data link layer receive processing
      - lcs_physical.h -- Layer 1 (physical layer) data structures for a Neuron MIP data link

## LON Stack Lifetime

To manage LON stack lifetime use the following functions:

- IzotCreateStack() -- LON Stack initialization.  Call first.
- IzotRegisterStaticDatapoint() -- Static datapoint registration.  Call once per static datapoint after IzotCreateStack().
- IzotRegisterMemoryWindow() -- Virtual memory window registration.  Call once after IzotCreateStack() if using the optional LON virtual memory window (DMF).
- IzotStartStack() -- Starts LON Stack.  Call once after IzoTCreateStack() and any optional calls to IzotRegisterStaticDatapoint() and IzotRegisterMemoryWindow(),
- IzotRegisterStaticDatapoint(), and IzotRegisterMemoryWindows().
- IzotEventPump() -- LON Stack event pump API for the main event loop.  Call periodically as described in the IzotEventPump() comments.

To configure the domain, subnet, and node ID for self-installation use the following function:

- IzotUpdateDomain()

To configure an address table entry for self-installation use the following function:

- IzotUpdateAddressConfig()

To change the mode for a device to configured and online after setting the domain and address table configuration use the following function:

- IzotGoConfigured()

To create a network variable:

1. Declare the network variable as a standard variable in C.
2. Create an IzotDatapointDefinition structure for the network variable.
3. Initialize the IzotDatapointDefinition structure with the IzotDatapointSetup() and IzotDatapointFlags() functions.
4. To create a self-installed connection, update the IzotDatapointDefinition structure with the IzotDatapointBind() function.
5. Create the network variable with IzotRegisterStaticDatapoint(), supplying a pointer to the variable you created in step 1
    and the IzotDatapointDefinition structure you created and initialized in steps 2 through 4.

To update a network variable (which the DX stack API also calls a “datapoint”):

1. Configure the address table index for the network variable.
2. To update the network variable:
    a. Update the network variable value with a write to the network variable created in step 1.
    b. Propagate the network variable by calling either IzotPropagate() or IzotPropagateByIndex().

To poll a network variable call IzotPoll() or IzotPollByIndex().

To receive a network variable update:

1. Configure a received network variable update handler with IzotDatapointUpdateOccurredRegistrar().
2. Your received network variable update handler will be called for every received network variable update.
3. For each call to your received network variable update handler, the updated network variable will be identified.  Call the appropriate network variable update handler for each received network variable.  The handler reads the updated value from the network variable created in step 1.

To send an application message use the following function:

- Call IzotSendMsg()

To respond to a request application message use the following function:

- Call IzotSendResponse()

To receive an application message:

1. Configure an application message handler with IzotMsgArrivedRegistrar().
2. Your application message handler will be called for every received application message.
