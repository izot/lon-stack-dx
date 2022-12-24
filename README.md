# lon-stack-dx
The EnOcean LON Stack DX enables Industrial IoT developers to build networks of communicating devices using any processor supporting the C programming language. Devices with the LON Stack DX can exchange data with each other on an easy-to-use publish-subscribe data model over IP or native LON channels. LON devices can collect data from physical sensors built to monitor the built environment including temperature, humidity, light-level, power-consumption, or moisture, and make the data available to other LON devices in the same network. Using data received from other LON devices or local sensors, LON devices can also control physical actuators such as LED dimmers, motor controllers, damper controllers, and solenoids. The LON protocol is an open standard defined by the ISO/IEC 14908 series of standards.

A larger implementation of the LON stack is available in the LON Stack EX available at https://github.com/izot/lon-stack-ex.  The LON Stack EX implements optional features of the LON protocol that are suitable for edge servers and workstations implementing the LON protocol.  

Following is a high-level summary of the LON Stack DX file structure.

    Definitions
        IzotCal.c/h -- IP connectivity abstraction layer.
        IzotHal.c/h -- Hardware abstraction layer.
        IzotOsal.c/h -- Operating system abstraction layer.
        EchelonStandardDefinitions.h -- Product, platform, processor, and data link conditional test macros.
        module_platform.h -- Product, platform, processor, and data link specific definitions.
        IzotPlatform.h -- Platform dependant flags and basic data types.
        lcs_node.c -- DX stack data structure access functions.
    Implementation
        IzotAPI.c/h -- Top-level API.  This is the primary application interface to the stack.
        + IzotCreateStack() -- DX stack initialization.  Call first.
        + IzotRegisterStaticDatapoint() -- Static datapoint registration.  Call once per static datapoint after IzotCreateStack().
        + IzotRegisterMemoryWindow() -- Virtual memory window registration.  Call once after IzotCreateStack() if using the optional LON virtual memory window (DMF).
        + IzotStartStack() -- Starts the DX stack.  Call once after IzoTCreateStack(), IzotRegisterStaticDatapoin(), and IzotRegisterMemoryWindows().
        + IzotEventPump() -- DX stack event pump API for the main event loop.  Call periodically as described in the IzotEventPump() comments.
            + Lcs.c -- DX stack main entry points. 
                +  LCS_Service() -- DX stack event pump for the main event loop
                    + lcs_app.c -- Layer 7 - 6 (application and presentation layers)
                        + APPSend() -- Send application and presentation layer message
                        + APPReceive() -- Receive application and presentation layer message
                    + lcs_tsa.c -- Layers 5 - 4 (session and transport layers)
                        + SNSend() -- Session layer send processing
                        + SNReceive() -- Session layer receive processing
                        + TPSend() -- Transport layer send processing
                        + TPReceive() -- Transport layer receive processing
                        + AuthSend() -- Authenticated message send processing
                        + AuthReceive() -- Authenticated message receive processing
                    + lcs_tcs.c -- Transaction control sublayer used by Layer 4
                    + lcs_network.c -- Layer 3 (network layer)
                        + NWSend() -- Network layer send processing
                        + NWReceive() -- Network layer receive processing
                    + lcs_link.c -- Layer 2 (data link layer) for a Neuron MIP data link
                        + LKSend() -- Neuron/MIP data link layer send processing 
                        + LKReceive() -- Neuron/MIP data link layer receive processing
                    + IPv4ToLsUdp.c -- Layer 2 (data link layer) for a UDP/IP data link
                        + LsUDPSend() -- UDP/IP data link layer send processing 
                        + LsUDPReceive() -- UDP/IP data link layer receive processing
                    + lcs_physical.h -- Layer 1 (physical layer) data structures for a Neuron MIP data link
	