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

/*******************************************************************************
          File:        lcs_eeprom.c

       Version:        1

     Reference:        None

       Purpose:        To place data structures that are placed
                       in eeprom.

          Note:        eeprom is actually stored in RAM in reference
                       implementation. Some rewriting is needed to
                       handle actual EEPROM usage.

         To Do:        None

*******************************************************************************/
/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <lcs_eia709_1.h>
#include <lcs_node.h>
#include "pal_platform.h"

/*------------------------------------------------------------------------------
Section: Constant Definitions
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Type Definitions
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Globals
------------------------------------------------------------------------------*/
EEPROM eeprom[NUM_STACKS];

/*------------------------------------------------------------------------------
Section: Local Function Prototypes
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Function Definitions
------------------------------------------------------------------------------*/
/*******************************************************************************
Function: Persist
Returns:  void
Purpose:  Record all data to NVM.  Note we currently use a very simple model
that everything fits in 256 bytes.
*******************************************************************************/
void LCS_WriteNvm(void)
{
	PAL_ExtWriteNvmBlockByType(eep, sizeof(*eep), PAL_BLOCK_TYPE_LCS_EEPROM);
}

EchErr LCS_ReadNvm(void)
{
	return PAL_ExtReadNvmBlockByType(eep, sizeof(*eep), PAL_BLOCK_TYPE_LCS_EEPROM);
}

/*******************************End of eeprom.c *******************************/
