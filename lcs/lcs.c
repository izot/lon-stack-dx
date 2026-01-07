/*
 * lcs.c
 *
 * Copyright (c) 2022-2026 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Stack Core Services
 * Purpose: Provides the main initialization and service functions for
 *          LON Stack DX.
 * Notes:   All applications using LON Stack DX must call LCS_Init()
 *          during initialization and LCS_Service() as often as practical
 * 			(e.g., once per millisecond).
 */

#include "lcs/lcs.h"
#include "izot/IzotApi.h"

// LON Stack DX initialization
extern LonStatusCode APPInit(void); // Init function for application layer

// Send functions for the LON Stack layers
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

/*
 * Provides the main initialization functions for LON Stack.
 * Parameters:
 *   cause: The reset cause
 * Returns:
 *   LonStatusNoError if successful, LonStatusCode error code otherwise.
 * Notes:
 *   This function must be called once during system initialization.
 */
LonStatusCode LCS_Init(IzotResetCause cause)
{
    IzotByte   stackNum;

#if LINK_IS(WIFI)
    UnlockWiFiDevice();
#endif  // LINK_IS(WIFI)

    // First init EEPROM based on custom.h, custom.c and default
    // values for several variables
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++) {
        eep = &eeprom[stackNum];
        nmp = &nm[stackNum];
        gp = &protocolStackDataGbl[stackNum];
        snvt_capability_info = &capability_info;
        si_header_ext = &header_ext;
	    if (InitEEPROM(IzotGetAppSignature()) != LonStatusNoError) {
			OsalPrintError(LonStatusStackInitializationFailure, "LCS_Init: Non-volatile data initialization failed for stack %d", stackNum);
            return LonStatusStackInitializationFailure;
        }
    }

    // Reset the LON device at the start
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++) {
        gp = &protocolStackDataGbl[stackNum];
        eep = &eeprom[stackNum];
        nmp = &nm[stackNum];
        snvt_capability_info = &capability_info;
        si_header_ext = &header_ext;
        if (APPInit() != LonStatusNoError) {
			OsalPrintError(LonStatusStackInitializationFailure, "LCS_Init: Application initialization failed for stack %d", stackNum);
			return LonStatusStackInitializationFailure;
		}
        // Compute the configCheckSum for the first time; NodeReset
        // will not verify checkSum first time
        eep->configCheckSum   = ComputeConfigCheckSum();
		nmp->resetCause		  = cause;

	    SetLonTimer(&gp->ledTimer, LED_TIMER_VALUE);
		SetLonTimer(&gp->checksumTimer, CHECKSUM_TIMER_VALUE); // Initial value
    }
	OsalPrintDebug(LonStatusNoError, "LCS_Init: LON Stack initialization completed successfully");
	return LonStatusNoError;
}

/*
 * Provides the main service function for LON Stack.
 * Returns:
 *   LonStatusNoError if successful; LonStatusCode error code otherwise.
 * Notes:
 *   This function must be called as often as practical (e.g., once
 *   per millisecond) to allow LON Stack to perform its processing.
 */
LonStatusCode LCS_Service()
{
	LonStatusCode status = LonStatusNoError;
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
			status = NodeReset(FALSE);
			if (!LON_SUCCESS(status) || !gp->resetOk) {
				OsalPrintError(status, "LCS_Service: LON application reset failed for stack %d", stackNum);
				return status;
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
				// Report but don't return a checksum error to enable reset to proceed
				OsalPrintError(LonStatusCnfgChecksumError, 
						"LCS_Service: Configuration checksum error detected, resetting");
			}
			SetLonTimer(&gp->checksumTimer, CHECKSUM_TIMER_VALUE);
		}
	}
	return status;
}