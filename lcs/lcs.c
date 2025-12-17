/*
 * lcs.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Core Services
 * Purpose: Provides the main initialization and service functions for
 *          the LON Stack DX.
 * Notes:   All applications using the LON Stack DX must call the
 *          LCS_Init() function during initialization and the
 *          LCS_Service() function as often as practical (e.g., once
 * 			per millisecond).
 */

#include "lcs/lcs.h"
#include "izot/IzotApi.h"

// LON Stack DX initialization
extern LonStatusCode APPInit(void); // Init function for application layer

// Send functions for the LON Stack DX layers
extern void APPSend(void);
extern void TPSend(void);
extern void SNSend(void);
extern void AuthSend(void);
extern void NWSend(void);
#if LINK_IS(WIFI) || LINK_IS(ETHERNET)
extern void LsUDPSend(void);
#endif // LINK_IS(WIFI) || LINK_IS(ETHERNET)
#if LINK_IS(USB) || LINK_IS(MIP)
extern void LKSend(void);
#endif // LINK_IS(USB) || LINK_IS(MIP)

// Receive functions for the LON Stack DX layers
extern void APPReceive(void);
extern void TPReceive(void);
extern void SNReceive(void);
extern void AuthReceive(void);
extern void NWReceive(void);
#if LINK_IS(WIFI) || LINK_IS(ETHERNET)
extern void LsUDPReceive(void);
#endif // LINK_IS(WIFI) || LINK_IS(ETHERNET)
#if LINK_IS(USB) || LINK_IS(MIP)
extern void LKReceive(void);
#endif // LINK_IS(USB) || LINK_IS(MIP)

#define LED_TIMER_VALUE      2000  // How often to flash in ms
#define CHECKSUM_TIMER_VALUE 1000  // How often to check config checksum in ms

LonStatusCode LCS_Init(IzotResetCause cause)
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
        if (APPInit() != LonStatusNoError) {
			return LonStatusStackInitializationFailure;
		}
        // Compute the configCheckSum for the first time. NodeReset
        // will not verify checkSum firt time.
        eep->configCheckSum   = ComputeConfigCheckSum();
		nmp->resetCause		  = cause;

	    SetLonTimer(&gp->ledTimer, LED_TIMER_VALUE);
		SetLonTimer(&gp->checksumTimer, CHECKSUM_TIMER_VALUE); // Initial value
    }
	return LonStatusNoError;
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
		#if LINK_IS(MIP) || LINK_IS(USB)
			LKSend();
		#else // !LINK_IS(MIP)
			LsUDPSend();
		#endif // LINK_IS(MIP)
		
		// Call all the Receive functions.
		#if LINK_IS(MIP) || LINK_IS(USB)
			LKReceive();
		#else // !LINK_IS(MIP)
			LsUDPReceive();
		#endif // LINK_IS(MIP)
		NWReceive();
		AuthReceive();
		TPReceive();
		SNReceive();
		APPReceive();

		// Flash service LED if needed.
		if (LonTimerExpired(&gp->ledTimer)) {
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
			SetLonTimer(&gp->ledTimer, LED_TIMER_VALUE); // Reset timer
		}

		// Check for integrity of config structure
		if (LonTimerExpired(&gp->checksumTimer)) {
			if (!NodeUnConfigured() &&
					eep->configCheckSum != ComputeConfigCheckSum()) {
				// Go unconfigured and reset.
				IZOT_SET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE, IzotApplicationUnconfig);
				gp->appPgmMode  = ON_LINE;
				IzotOffline();  // Indicate to application program.
				gp->resetNode = TRUE;
				nmp->resetCause = IzotSoftwareReset;
				OsalPrintError(LonStatusCnfgChecksumError, 
						"LCS_Service: Configuration checksum error detected, resetting");
			}
			SetLonTimer(&gp->checksumTimer, CHECKSUM_TIMER_VALUE);
		}
	}
}
