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
          File:        lcs_custom.c

       Version:        1

     Reference:        None

       Purpose:        To allow custom values for global data
                       of protocol stack. Since the reference
                       implementation supports multiple stacks
                       on a single 38360, we need to consider
                       custom values for each stack.

          Note:        Some or all of these values will be overwritten by
                       network management tools when this node is loaded.

         To Do:        None

*******************************************************************************/
/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <lcs_eia709_1.h>
#include <lcs_custom.h>

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
CustomData  customDataGbl[NUM_STACKS] =
{
    /* Stack 1 Values. This is the only one used when the reference
       implementation is run in real (i.e. not in simulation) mode. */
    {
        /* Unique Node Id. */
        {0x00,0xfd,0xff,0xff,0xff,0x01},

        /* How many domains? 0 => 1 domain 1 => 2 domains. */
        1,

        /* Program ID string. */
        "cStack1",

        /* Location. LOCATION_LEN represents the maximum # of characters. */
        "loc1",

        /* Domain Ids. Need one each domain. */
        {{0x2c}},
        /* Domain lengths. One for each domain. */
        {1},
        /* Subnet numbers. One for each domain. */
        {1},
        /* Node numbers. One for each domain. */
        {7},
        /* Authentication key */
        {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},

        /* Address table entries. */
        {0x00},

        /* Alias table entries */
        {0x00}
    }
#if (NUM_STACKS > 1)
    ,

    /* Stack 2 Values */
    {
        /* Unique Node Id. */
        {0x00,0xfd,0xff,0xff,0xff,0x02},

        /* How many domains? 0 => 1 domain 1 => 2 domains. */
        0,

        /* Program ID string. */
        "cStack2",

        /* Location. LOCATION_LEN represents the maximum # of characters. */
        "loc2",

        /* Domain Ids. Need one each domain. */
        {{0x2c}},
        /* Domain lengths. One for each domain. */
        {1},
        /* Subnet numbers. One for each domain. */
        {1},
        /* Node numbers. One for each domain. */
        {8},
        /* Authentication key */
        {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},

        /* Address table entries. */
        {0x00},

        /* Alias table entries. */
        {0x00}
    }
#endif

#if (NUM_STACKS > 2)
    ,
    /* Stack 3 Values */
    {
        /* Unique Node Id. */
        {0x00,0xfd,0xff,0xff,0xff,0x03},

        /* How many domains? 0 => 1 domain 1 => 2 domains. */
        0,

        /* Program ID string. */
        "cStack3",

        /* Location. LOCATION_LEN represents the maximum # of characters. */
        "loc3",

        /* Domain Ids. Need one each domain. */
        {{0x2c}},
        /* Domain lengths. One for each domain. */
        {1},
        /* Subnet numbers. One for each domain. */
        {1},
        /* Node numbers. One for each domain. */
        {9},
        /* Authentication key */
        {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},

        /* Address table entries. */
        {0x00},

        /* Alias table entries. */
        {0x00}
    }
#endif


};

CustomData *cp;

/*------------------------------------------------------------------------------
Section: Local Function Prototypes
------------------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------------
Section: Function Definitions
------------------------------------------------------------------------------*/
/* None */

/*******************************End of custom.c *******************************/
