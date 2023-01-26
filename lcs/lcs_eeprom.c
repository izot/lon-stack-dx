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

/*******************************************************************************
          File:        lcs_eeprom.c

       Purpose:        Access and store non-volatile data.
*******************************************************************************/

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "IzotConfig.h"
#include "IzotApi.h"
#include "IzotTypes.h"
#include "lcs_eia709_1.h"
#include "lcs_timer.h"
#include "lcs_node.h"
#include "Persistent.h"

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
#define NVM_START	(&eep->configData)
#define NVM_SIZE 	(sizeof(*eep)-sizeof(eep->readOnlyData))
#define MISC_START  (&gp->nvm)
#define MISC_SIZE	(sizeof(gp->nvm))

/*------------------------------------------------------------------------------
Section: Type Definitions
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
EEPROM eeprom[NUM_STACKS];

/*------------------------------------------------------------------------------
Section: Function Prototypes
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Function Definitions
------------------------------------------------------------------------------*/
/*******************************************************************************
Function: LCW_WriteNvm
Returns:  void
Purpose:  Record all data to NVM.  Note we currently use a very simple model
that everything fits in 256 bytes.
*******************************************************************************/
void LCS_WriteNvm(void)
{
	eep->nodeState = IZOT_GET_ATTRIBUTE(eep->readOnlyData, IZOT_READONLY_NODE_STATE);
	SetPersistentDataType(IzotPersistentSegNetworkImage);
	IzotPersistentAppSegmentHasBeenUpdated();
}

/*******************************************************************************
Function: LCW_ReadNvm
Returns:  void
Purpose:  Read all data from NVM.  Note we currently use a very simple model
that everything fits in 256 bytes.
*******************************************************************************/
EchErr LCS_ReadNvm(void)
{
	IzotBits16 ret;
	ret = restore(IzotPersistentSegNetworkImage);
	return ((ret == 0) ? ECHERR_OK : ECHERR_NOT_FOUND);
}

/*******************************************************************************
Function: LCW_WriteNvs
Returns:  void
Purpose:  Record all persistent NV data to NVM.  Note we currently use a very simple model
that everything fits in 256 bytes.
*******************************************************************************/
void LCS_WriteNvs(void)
{
	SetPersistentDataType(IzotPersistentSegApplicationData);
	IzotPersistentAppSegmentHasBeenUpdated();
}

/*******************************************************************************
Function: LCS_ReadNvs
Returns:  void
Purpose:  Read all NV data from NVM.  Note we currently use a very simple model
that everything fits in 256 bytes.
*******************************************************************************/
EchErr LCS_ReadNvs(void)
{
	IzotUbits16 ret;
	ret = restore(IzotPersistentSegApplicationData);
	return ((ret == 0) ? ECHERR_OK : ECHERR_NOT_FOUND);
}

/*******************************End of eeprom.c *******************************/
