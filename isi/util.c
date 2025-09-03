/*
 * util.c
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
#include "isi/isi_int.h"

#if PROCESSOR_IS(MC200)
#include <wmtime.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define thisRandomInit 1234          //for random_t()
unsigned m_z = thisRandomInit;  	 //for random_t() //changed datatype to ignore truncated value

/*****************************************************************
 * Section: Function Declarations
 *****************************************************************/
extern unsigned IzotGetCurrentDatapointSize(const unsigned index);
extern volatile void* IzotGetDatapointValue(const unsigned index);

/*****************************************************************
 * Section: Function Definitions
 *****************************************************************/

/*
 * Gets the domain configuration for the specified index.
 * Parameters:
 *   domainIndex: Index of the domain configuration to retrieve; must be 0 or 1
 * Returns:
 *   Pointer to the domain configuration, or NULL if the index is invalid
 */
IzotDomain* access_domain(int domainIndex)
{
    IzotDomain* pDomain = NULL;

    _IsiAPIDebug("Start access_domain = %d\n", domainIndex);
    if (domainIndex <= IZOT_GET_ATTRIBUTE(read_only_data, IZOT_READONLY_TWO_DOMAINS)) {
        pDomain = &domainTable[domainIndex];
        if (IzotQueryDomainConfig(domainIndex, pDomain) == IzotApiNoError) {
            _IsiAPIDebug("DomainID  = %x %x %x %x %x %x, Subnet=%d, NonClone=%d Node=%d Invalid=%d Length=%d Key=%x\n", 
                pDomain->Id[0],pDomain->Id[1],pDomain->Id[2],pDomain->Id[3],pDomain->Id[4],pDomain->Id[5],pDomain->Subnet, 
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_NONCLONE),
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_NODE),
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_INVALID),
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_ID_LENGTH), pDomain->Key[0]);
        }
    }
     _IsiAPIDebug("End access_domain = %d\n", domainIndex);
    return pDomain;
}

/*
 * Updates the domain configuration for the specified index.
 * Parameters:
 *   domainConfig: Pointer to the new domain configuration
 *   domainIndex: Index of the domain configuration to update; must be 0 or 1
 *   nonCloneValue: 0 for a clone domain; 1 otherwise
 *   bUpdateID: TRUE to update the domain ID, FALSE to not update the domain ID
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 */
IzotApiError update_domain_address(const IzotDomain* domainConfig, int domainIndex,
                int nonCloneValue, IzotBool bUpdateID)
{
    int i;
    IzotDomain* temp = NULL;
    IzotApiError sts = IzotApiNoError;

    _IsiAPIDebug("Start update_domain_address = %d\n", domainIndex);
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
    _IsiAPIDebug("End update_domain_address = %d\n", sts);
    return sts;
}

/*
 * Updates the domain configuration for the specified index.
 * Parameters:
 *   domainConfig: Pointer to the new domain configuration
 *   domainIndex: Index of the domain configuration to update; must be 0 or 1
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 */
IzotApiError IsiSetDomain(const IzotDomain* domainConfig, unsigned domainIndex)
{
    IzotApiError sts;

    _IsiAPIDebug("Start IsiSetDomain = %d\n", domainIndex);
    sts = update_domain_address(domainConfig, domainIndex, 1, TRUE); // it's not a clone domain

    _IsiAPIDebug("End IsiSetDomain = %d\n", sts);
    return sts;
}

/* 
 * Returns the number of static NVs 
 * Parameters:
 *   None
 * Returns:
 *   The number of static NVs, limited by ISI_MAX_NV_COUNT
 */
unsigned _nv_count(void)
{
    unsigned int nvCount = IzotGetStaticDatapointCount();

    // Check against the ISI limit
    if (nvCount > ISI_MAX_NV_COUNT)
        nvCount = ISI_MAX_NV_COUNT;
    return nvCount;
}

/*
 * Sets the NV configuration for the specified NV index.
 * Parameters:
 *   nvConfig: Pointer to the new NV configuration
 *   nvIndex: Index of the NV configuration to update; must be less than the number of static NVs
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 */
void IsiSetNv(IzotDatapointConfig* nvConfig, unsigned nvIndex)
{
	update_nv(nvConfig, nvIndex);
}

/*
 * Sets the NV configuration for the specified NV index.
 * Parameters:
 *   nvConfig: Pointer to the new NV configuration
 *   nvIndex: Index of the NV configuration to update; must be less than the number of static NVs
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 */
void update_nv(const IzotDatapointConfig* nvConfig, unsigned nvIndex)
{
    if (nvConfig && nvIndex < _nv_count()) {
		/*IzotApiError sts = */IzotUpdateDpConfig(nvIndex, nvConfig);

        _IsiAPIDebug("update_nv index %u sts %i ", nvIndex, sts);
        _IsiAPIDump("data = 0x", (void *)nvConfig, sizeof(IzotDatapointConfig), "\n");
    }
}

/*
 * Gets the NV configuration for the specified NV index.
 * Parameters:
 *   nvIndex: Index of the NV configuration to get; must be less than the number of static NVs
 * Returns:
 *   Pointer to the NV configuration
 */
const IzotDatapointConfig* IsiGetNv(unsigned nvIndex)
{
    memset(&nv_config, 0, sizeof(IzotDatapointConfig));
    IzotQueryDpConfig(nvIndex, &nv_config);
    return (IzotDatapointConfig* )&nv_config;
}

/*
 * Returns the high byte of a 16-bit value.
 * Parameters:
 *   a: 16-bit value
 * Returns:
 *   The high byte of the 16-bit value
 */
IzotByte high_byte(IzotUbits16 a)
{
	return ((unsigned char)(a>>8));
}

/*
 * Returns the low byte of a 16-bit value.
 * Parameters:
 *   a: 16-bit value
 * Returns:
 *   The low byte of the 16-bit value
 */
IzotByte low_byte (IzotUbits16 a)
{
	return ((unsigned char)(a&(0x00FF)));
}

/*
 * Returns a 16-bit value composed of the specified low and high bytes.
 * Parameters:
 *   low_byte: Low byte of the 16-bit value
 *   high_byte: High byte of the 16-bit value
 * Returns:
 *   The combined 16-bit value
 */
IzotWord make_long(IzotByte low_byte, IzotByte high_byte)
{
    IzotWord w;
    IZOT_SET_UNSIGNED_WORD(w, (IzotUbits16)((high_byte<<8)|low_byte));
	return w;
}

/*
 * Returns a 32-bit pseudo-random number.
 * Parameters:
 *   None
 * Returns:
 *   A 32-bit pseudo-random number
 */
unsigned int getRandom(void)
{
	unsigned m_w = IzotGetTickCount();

	m_z = 36969 * (m_z & 65535) + (m_z >> 4);
	m_w = 18000 * (m_w & 65535) + (m_w >> 4);

	return (m_z << 4) + m_w;  // 32-bit result
}

/*
 * Gets the address configuration for the specified index.
 * Parameters:
 *   index: Index of the address configuration to retrieve; must be less than the number of
 *          address table entries
 * Returns:
 *   Pointer to the address configuration, or NULL if the index is invalid
 */ 
IzotAddress* access_address(int index)
{
    IzotAddress* devAddress = (IzotAddress*)&addrTable;

    if (IzotQueryAddressConfig(index, devAddress) == IzotApiNoError)
        return devAddress;
    else
        return (IzotAddress*)NULL;            
}

/*
 * Sets the address configuration for the specified index.
 * Parameters:
 *   devAddress: Pointer to the new address configuration
 *   index: Index of the address configuration to update
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 * Notes:
 *   The index must be less than the number of address table entries.
 */ 
IzotApiError update_address(const IzotAddress* devAddress, int index)
{
    IzotApiError sts = IzotUpdateAddressConfig(index, devAddress);
	if (sts != IzotApiNoError) {
		_IsiAPIDebug("update_address failed (entry %d)\n", index);
	}
    return sts;
}

/*
 * Updates the watchdog timer, if any.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
void lon_watchdog_update(void)
{
    // Do nothing
}

/*
 * Gets the alias configuration for the specified alias table index.
 * Parameters:
 *   aliasIndex: Index of the alias configuration to retrieve
 * Returns:
 *   Pointer to the alias configuration, or NULL if the index is invalid
 * Notes:
 *   The index must be less than the number of alias entries.
 */
const IzotAliasConfig* IsiGetAlias(unsigned aliasIndex)
{
    _IsiAPIDebug("Start IsiGetAlias(%d)\n", aliasIndex); 
    memset(&alias_config, 0, sizeof(IzotAliasConfig));
    if (IzotQueryAliasConfig(aliasIndex, &alias_config) != IzotApiNoError)
    {
        _IsiAPIDebug("Error - IsiGetAlias(%d)\n", aliasIndex); 
    }
    _IsiAPIDebug("End IsiGetAlias(%d)\n", aliasIndex);
	return (IzotAliasConfig*)&alias_config;
}

/*
 * Sets the alias configuration for the specified alias table index.
 * Parameters:
 *   aliasConfig: Pointer to the new alias configuration
 *   aliasIndex: Index of the alias configuration to update
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 * Notes:
 *   The index must be less than the number of alias entries.
 */
IzotApiError IsiSetAlias(IzotAliasConfig* aliasConfig, unsigned aliasIndex)
{
    return IzotUpdateAliasConfig(aliasIndex, aliasConfig); 
}

IzotApiError update_config_data(const IzotConfigData *configData)
{
    // Update the global copy of config data
    memcpy(&config_data, configData, sizeof(IzotConfigData));
    return IzotUpdateConfigData(configData);  // call LTS IzotUpdateConfigData to update the config data 
}

IzotConfigData* get_config_data(void)
{
    // Get the fresh config data from the stack
    IzotQueryConfigData(&config_data);
    return (IzotConfigData*)&config_data;
}

unsigned int _alias_count(void)
{
    unsigned int aliasCount = IzotGetAliasCount();
    // Check against the ISI limit
    if (aliasCount > ISI_MAX_ALIAS_COUNT)
        aliasCount = ISI_MAX_ALIAS_COUNT;
    return aliasCount;
}

unsigned int _address_table_count(void)
{
    unsigned int addressTableCount = IzotGetAddressTableCount();
    // Check against the ISI limit
    if (addressTableCount > ISI_MAX_ADDRESS_TABLE_SIZE)
        addressTableCount = ISI_MAX_ADDRESS_TABLE_SIZE;
    return addressTableCount;
}

unsigned get_nv_si_count(void)
{
	return _nv_count(); // same with the nv count
}

/*
 * Returns the NV type for a specified NV index.
 * Parameters:
 *   nvIndex: Index of the NV
 * Returns:
 *  The SNVT ID (1-250) for a SNVT; 0 for an NV that is not a SVNT
 */
unsigned get_nv_type (unsigned nvIndex)
{
	return 1;
}

IzotWord _IsiAddSelector(IzotWord selector, unsigned increment)
{
    IzotWord result;
	IZOT_SET_UNSIGNED_WORD(result, IZOT_GET_UNSIGNED_WORD(selector) + increment);
    return result;
}

IzotWord _IsiIncrementSelector(IzotWord selector)
{
    return _IsiAddSelector(selector, 1);
}

IsiType _IsiGetCurrentType()
{
    return gIsiType;
}

void _IsiSetCurrentType(IsiType type)
{
    gIsiType = type;
}

unsigned int get_nv_length(const unsigned index)
{
    return IzotGetCurrentDatapointSize(index);
}

IzotByte* get_nv_value(const unsigned index)
{
	return (IzotByte *)IzotGetDatapointValue(index);
}

IzotApiError service_pin_msg_send()
{
    return IzotSendServicePin();
}

void node_reset()
{
    NodeReset(FALSE);
}    

IzotApiError retrieve_status(IzotStatus* status)
{
    return IzotQueryStatus(status);       
}

/*
 * Initializes ISI data structures from persistent storage.
 * Parameters:
 *   bootType: Type of boot (isiColdStart, isiWarmStart, isiReset, or isiReboot)
 * Returns:
 *   IzotApiNoError (0) on success, or an <IzotApiError> error code on failure
 * Notes:   
 *   If no persistent data is found, the connection table, NV table, alias
 *   table and address table are initialized to default values.
 */
IzotApiError initializeData(IsiBootType bootType)
{
    IzotApiError sts = IzotQueryConfigData(&config_data);
   
    if (sts == IzotApiNoError) {
        sts = IzotQueryReadOnlyData(&read_only_data);
    }

#if ISI_IS(SIMPLE) || ISI_IS(DA)
	if (restorePersistentData(IzotPersistentSegConnectionTable) != 
    LT_PERSISTENCE_OK) {
		_IsiAPIDebug("No Isi connection table found\r\n");
        _IsiInitConnectionTable();
		return sts;
	}
#endif

	if (restorePersistentData(IzotPersistentSegIsi) != LT_PERSISTENCE_OK)
    {
		_IsiAPIDebug("No Isi Persistent data found\r\n");
        // First time run.  Signal the engine to clear-out NV, alias, connection table and address tables.
        if (bootType != isiReboot) {
			_isiPersist.BootType = isiReset;
		}
    }
	
    return sts;
}

IzotApiError set_node_mode(unsigned mode, unsigned state)
{
	return IzotSetNodeMode(mode, state);
}

#ifdef  __cplusplus
}
#endif
