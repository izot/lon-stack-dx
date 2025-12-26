/*
 * isi_util.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Interoperable Self-Installation (ISI) Utility Functions
 * Purpose: Defines LON ISI utility functions for a LON stack.
 */

#include <stdio.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "izot/IzotPlatform.h"
#include "izot/lon_types.h"
#include "isi/isi_int.h"
#include "lcs/lcs_api.h"
#include "lcs/lcs_node.h"

#if PROCESSOR_IS(MC200)
#include <wmtime.h>
#endif

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
extern unsigned IzotGetCurrentDatapointSize(const unsigned index);
extern volatile void* IzotGetDatapointValue(const unsigned index);

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/

#if !ISI_IS(ISI_ID_NO_ISI)

/*
 * Updates the domain configuration for the specified index.
 * Parameters:
 *   domainConfig: Pointer to the new domain configuration
 *   domainIndex: Index of the domain configuration to update; must be 0 or 1
 *   nonCloneValue: 0 for a clone domain; 1 otherwise
 *   bUpdateID: TRUE to update the domain ID, FALSE to not update the domain ID
 * Returns:
 *   LonStatusNoError on success, or an <LonStatusCode> error code on failure
 */
LonStatusCode LonIsiUpdateDomainConfig(const IzotDomain* domainConfig, int domainIndex,
                int nonCloneValue, IzotBool bUpdateID)
{
    int i;
    IzotDomain* temp = NULL;
    LonStatusCode sts = LonStatusNoError;

    _IsiAPIDebug("Start LonIsiUpdateDomainConfig = %d\n", domainIndex);
    if (domainIndex <= IZOT_GET_ATTRIBUTE(read_only_data, IZOT_READONLY_TWO_DOMAINS)) {
        temp = &domainTable[domainIndex];
        if (bUpdateID) {
            for(i=0; i<DOMAIN_ID_LEN; i++) {
		        temp->Id[i] = domainConfig->Id[i];
	        };
        }
	    temp->Subnet = domainConfig->Subnet;
	    IZOT_SET_ATTRIBUTE_P(temp,IZOT_DOMAIN_NONCLONE,nonCloneValue);  // 0 = if it's a clone domain, 1= otherwise
	    IZOT_SET_ATTRIBUTE_P(temp,IZOT_DOMAIN_NODE, IZOT_GET_ATTRIBUTE_P(domainConfig,IZOT_DOMAIN_NODE));
	    temp->InvalidIdLength = domainConfig->InvalidIdLength;
        IZOT_SET_ATTRIBUTE_P(temp,IZOT_DOMAIN_INVALID, 0);    // set the domain to be valid.  Otherwise, the LTS will reset the length to 7
        sts = IzotUpdateDomainConfig(domainIndex, temp);
        _IsiAPIDebug("DomainID  = %x %x %x %x %x %x, Subnet=%d, NonClone=%d Node=%d Invalid=%d Length=%d Key=%x\n", 
                temp->Id[0],temp->Id[1],temp->Id[2],temp->Id[3],temp->Id[4],temp->Id[5],temp->Subnet, 
                IZOT_GET_ATTRIBUTE_P(temp,IZOT_DOMAIN_NONCLONE),
                IZOT_GET_ATTRIBUTE_P(temp,IZOT_DOMAIN_NODE),
                IZOT_GET_ATTRIBUTE_P(temp,IZOT_DOMAIN_INVALID),
                IZOT_GET_ATTRIBUTE_P(temp,IZOT_DOMAIN_ID_LENGTH), temp->Key[0]);
    }
    _IsiAPIDebug("End LonIsiUpdateDomainConfig = %d\n", sts);
    return sts;
}

/* 
 * Returns the number of static NVs usable by ISI.
 * Parameters:
 *   None
 * Returns:
 *   The number of static NVs, limited by ISI_MAX_NV_COUNT
 */
unsigned LonIsiNvCount(void)
{
    unsigned int nvCount = IzotGetStaticDatapointCount();
    return (nvCount <= ISI_MAX_NV_COUNT) ? nvCount : ISI_MAX_NV_COUNT;
}

/* 
 * Returns the number of static NV aliases usable by ISI.
 * Parameters:
 *   None
 * Returns:
 *   The number of static NV aliases, limited by ISI_MAX_ALIAS_COUNT
 */
unsigned int LonIsiAliasCount(void)
{
    unsigned int aliasCount = IzotGetAliasCount();
    return (aliasCount <= ISI_MAX_ALIAS_COUNT) ? aliasCount : ISI_MAX_ALIAS_COUNT;
}

/* 
 * Returns the number of address table entries usable by ISI.
 * Parameters:
 *   None
 * Returns:
 *   The number of address table entries, limited by ISI_MAX_ADDRESS_TABLE_SIZE
 */
unsigned int LonIsiAddressTableCount(void)
{
    unsigned int addressTableCount = IzotGetAddressTableCount();
    return (addressTableCount <= ISI_MAX_ADDRESS_TABLE_SIZE) ? addressTableCount : ISI_MAX_ADDRESS_TABLE_SIZE;
}

// Gets the current ISI type
// Parameters:
//   None
// Returns:
//   The current ISI type
// Notes:
//   Used to determine if the active ISI type is isiTypeS,
//   isiTypeDa, or isiTypeDAS
IsiType LonIsiType()
{
    return gIsiType;
}

// Sets the current ISI type
// Parameters:
//   type: The ISI type to set
// Returns:
//   None
// Notes:
//   Set the ISI type to isiTypeS, isiTypeDa, or isiTypeDAS
void LonIsiSetType(IsiType type)
{
    gIsiType = type;
}

#endif  // !ISI_IS(ISI_ID_NO_ISI)

#ifdef  __cplusplus
}
#endif
