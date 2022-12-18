//
// lcs.c
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

//
// LON DX Stack main entry points
//

//
// Any APP wishing to use LCS must do the following:
// 1. Call LCS_Init() during initialization
// 2. Call LCS_Service() as often as practical (e.g., once per millisecond)
//
#include <stddef.h>
#include "IzotTypes.h"
#include "lcs_eia709_1.h"
#include "lcs_custom.h"
#include "lcs_node.h"
#include "lcs.h"


// Application init functions
extern Status APPInit(void); // Init function for application layer

// Send functions for the layers
extern void APPSend(void);
extern void TPSend(void);
extern void SNSend(void);
extern void AuthSend(void);
extern void NWSend(void);
extern void LsUDPSend(void);
extern void PHYSend(void);

// Receive functions for the layers
extern void APPReceive(void);
extern void TPReceive(void);
extern void SNReceive(void);
extern void AuthReceive(void);
extern void NWReceive(void);
extern void LsUDPReceive(void);
extern void PHYReceive(void);

extern void IzotOffline(void);

#define LED_TIMER_VALUE      2000  // How often to flash in ms
#define CHECKSUM_TIMER_VALUE 1000  // How often to check config checksum?

Status LCS_Init(IzotResetCause cause)
{
    IzotByte   stackNum;

    // First init EEPROM based on custom.h, custom.c and default
    // values for several variables
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++) {
        eep = &eeprom[stackNum];
        nmp = &nm[stackNum];
        gp = &protocolStackDataGbl[stackNum];
        snvt_capability_info = &capability_info;
        si_header_ext = &header_ext;
	    InitEEPROM(IzotGetAppSignature());
    }

    // Reset the node at the start
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++) {
        gp = &protocolStackDataGbl[stackNum];
        eep = &eeprom[stackNum];
        nmp = &nm[stackNum];
        snvt_capability_info = &capability_info;
        si_header_ext = &header_ext;
        if (APPInit() == FAILURE) {
			return FAILURE;
		}
        // Compute the configCheckSum for the first time. NodeReset
        // will not verify checkSum firt time.
        eep->configCheckSum   = ComputeConfigCheckSum();
		nmp->resetCause		  = cause;

	    MsTimerSet(&gp->ledTimer, LED_TIMER_VALUE);
		MsTimerSet(&gp->checksumTimer, CHECKSUM_TIMER_VALUE); // Initial value
    }
	return SUCCESS;
}

void LCS_Service()
{
	int stackNum;
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++) {
		gp  = &protocolStackDataGbl[stackNum];
		eep = &eeprom[stackNum];
		nmp = &nm[stackNum];
        snvt_capability_info = &capability_info;
        si_header_ext = &header_ext;
        
		// Check if the node needs to be reset.
		if (gp->resetNode) {
			gp->resetOk = TRUE;
			NodeReset(FALSE);
			if (!gp->resetOk) {
				return;
			}
			continue; // Easy way to do scheduler reset,
		}

		// Call the application program, 
        // and let it know whether it is online or offline.
		DoApp(AppPgmRuns()); // Call the application. 
                             // Let it do whatever it wants.

		// Call all the Send functions
		APPSend();
		SNSend();
		TPSend();
		AuthSend();
		NWSend();
		#if TRANSPORT_IS(MIP)
			LKSend();
		#else
			LsUDPSend();
		#endif
		
		// Call all the Receive functions.
		#if TRANSPORT_IS(MIP)
			LKReceive();
		#else
			LsUDPReceive();
		#endif
		NWReceive();
		AuthReceive();
		TPReceive();
		SNReceive();
		APPReceive();

		// Flash service LED if needed.
		if (MsTimerExpired(&gp->ledTimer)) {
			if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotApplicationUnconfig) {
				gp->serviceLedState = SERVICE_BLINKING;
				gp->serviceLedPhysical = 1 - gp->serviceLedPhysical;
			}
            if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotConfigOnLine) {
                gp->serviceLedState = SERVICE_OFF;
                gp->serviceLedPhysical = SERVICE_LED_OFF;
            }   
            if (IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE) == IzotNoApplicationUnconfig) {
                gp->serviceLedState = SERVICE_ON;
                gp->serviceLedPhysical = SERVICE_LED_ON;
            }
            
			if (gp->prevServiceLedState != gp->serviceLedState || gp->preServiceLedPhysical != gp->serviceLedPhysical) {
                IzotServiceLedStatus(gp->serviceLedState, gp->serviceLedPhysical);
				gp->prevServiceLedState = gp->serviceLedState;
				gp->preServiceLedPhysical = gp->serviceLedPhysical;
            }
			MsTimerSet(&gp->ledTimer, LED_TIMER_VALUE); // Reset timer
		}

		// Check for integrity of config structure
		if (MsTimerExpired(&gp->checksumTimer)) {
			if (!NodeUnConfigured() &&
					eep->configCheckSum != ComputeConfigCheckSum()) {
				DBG_vPrintf(TRUE, "\n: +++++++++++  LCS RESET +++++++++++++");
				// Go unconfigured and reset.
				IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE, 
                IzotApplicationUnconfig);
				gp->appPgmMode  = ON_LINE;
				IzotOffline();  // Indicate to application program.
				gp->resetNode = TRUE;
				nmp->resetCause = IzotSoftwareReset;
				LCS_RecordError(IzotCnfgCheckSumError);
			}
			MsTimerSet(&gp->checksumTimer, CHECKSUM_TIMER_VALUE);
		}
	}
}
