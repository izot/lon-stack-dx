//
// lcs.c
//
// Copyright (C) 2022 Dialog Semiconductor
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
// LonTalk C Stack main entry points
//

// 
// Any APP wishing to use LCS must do the following:
// 1. Call LCS_Init() during initialization
// 2. Call LCS_Service() as often as practical (e.g., once per millisecond)
//

#include "lcs_eia709_1.h"
#include "lcs_custom.h"
#include "lcs_node.h"

// Application init functions
extern Status AppInit(void); /* Init function for application program */
extern void APPInit(void); /* Init function for application layer */

/* Send functions for the layers */
extern void APPSend(void);
extern void TPSend(void);
extern void SNSend(void);
extern void AuthSend(void);
extern void NWSend(void);
extern void LKSend(void);
extern void PHYSend(void);

/* Receive functions for the layers */
extern void APPReceive(void);
extern void TPReceive(void);
extern void SNReceive(void);
extern void AuthReceive(void);
extern void NWReceive(void);
extern void LKReceive(void);
extern void PHYReceive(void);

#define LED_TIMER_VALUE      1000  /* How often to flash in ms */
#define CHECKSUM_TIMER_VALUE 1000  /* How often to check config checksum? */

Status LCS_Init()
{
    uint8   stackNum;

    /* First init EEPROM based on custom.h, custom.c and default
       values for several variables */
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++)
    {
        eep = &eeprom[stackNum];
        cp  = &customDataGbl[stackNum];
        nmp = &nm[stackNum];
        InitEEPROM();
    }

    /* Reset the node at the start */
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++)
    {
        gp = &protocolStackDataGbl[stackNum];
        eep = &eeprom[stackNum];
        nmp = &nm[stackNum];
        gp->resetOk = TRUE;
        nmp->resetCause = POWER_UP_RESET;
        NodeReset(TRUE);
        if (!gp->resetOk)
        {
            return(FAILURE); /* Leave main and loop. */
        }
        /* Call APPInit and AppInit once before the loop. */
        APPInit();
        if (AppInit() == FAILURE)
        {
            return(FAILURE);
        }
        /* Compute the configCheckSum for the first time. NodeReset
           will not verify checkSum firt time. */
        eep->configCheckSum   = ComputeConfigCheckSum();

	    MsTimerSet(&gp->ledTimer, LED_TIMER_VALUE);
		MsTimerSet(&gp->checksumTimer, CHECKSUM_TIMER_VALUE); /* Initial value */
    }
	return SUCCESS;
}

void LCS_Service()
{
	int stackNum;
    for (stackNum = 0; stackNum < NUM_STACKS; stackNum++)
    {
		gp  = &protocolStackDataGbl[stackNum];
		eep = &eeprom[stackNum];
		nmp = &nm[stackNum];

		/* Check if the node needs to be reset. */
		if (gp->resetNode)
		{
			gp->resetOk = TRUE;
			NodeReset(FALSE);
			if (!gp->resetOk)
			{
				return; 
			}
			continue; /* Easy way to do scheduler reset, */
		}

		/* Call the application program, if needed. */
		if (AppPgmRuns())
		{
			DoApp(); /* Call the application. Let it do whatever it wants. */
		}

		/* Call all the Send functions */
		APPSend();
		SNSend();
		TPSend();
		AuthSend();
		NWSend();
		LKSend();

		/* Call all the Receive functions. */
		LKReceive();
		NWReceive();
		AuthReceive();
		TPReceive();
		SNReceive();
		APPReceive();

		/* Flash service LED if needed. */
		if (MsTimerExpired(&gp->ledTimer))
		{
			if (eep->readOnlyData.nodeState == APPL_UNCNFG)
			{
				TOGGLE_SERVICE_LED;
			}
			MsTimerSet(&gp->ledTimer, LED_TIMER_VALUE); /* Reset timer */
		}

		/* Check for integrity of config structure */
		if (MsTimerExpired(&gp->checksumTimer))
		{
			if (NodeConfigured() &&
					eep->configCheckSum != ComputeConfigCheckSum() )
			{
				/* Go unconfigured and reset. */
				eep->readOnlyData.nodeState = APPL_UNCNFG;
				gp->appPgmMode  = ON_LINE;
				OfflineEvent();  /* Indicate to application program. */
				gp->resetNode = TRUE;
				nmp->resetCause = SOFTWARE_RESET;
				LCS_RecordError(CNFG_CS_ERROR);
			}
			MsTimerSet(&gp->checksumTimer, CHECKSUM_TIMER_VALUE);
		}
	}
}