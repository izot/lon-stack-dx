//
// util.c
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

/*
 * Purpose: Interoperable self-installation (ISI) utility functions
 */

#include <stdio.h>
#include <stdarg.h>
#include "isi_int.h"

#if PROCESSOR_IS(MC200)
#include <wmtime.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

//============== LOCAL VARIABLES ===========================================
#define thisRandomInit 1234          //for random_t()
unsigned m_z = thisRandomInit;  	 //for random_t() //changed datatype to ignore truncated value

/*=============================================================================
 *                           SUPPORT FUNCTIONS                                *
 *===========================================================================*/

extern unsigned IzotGetCurrentDatapointSize(const unsigned index);
extern void IzotReset(const IzotResetNotification* const pResetNotification);
extern volatile void* IzotGetDatapointValue(const unsigned index);


/***************************************************************************
 *  Function: access_domain
 *
 *  Parameters: int index
 *
 *  Operation: Call IzotQueryDomainConfig() LTS API function to get domain
 *  configuration of index.
 *
 ***************************************************************************/
const IzotDomain* access_domain(int index)
{
    IzotDomain* pDomain = NULL;

    _IsiAPIDebug("Start access_domain = %d\n", index);
    if (index <= IZOT_GET_ATTRIBUTE(read_only_data, IZOT_READONLY_TWO_DOMAINS)) {
        pDomain = &domainTable[index];
        if ((IsiApiError)IzotQueryDomainConfig(index, pDomain) == IsiApiNoError) {
            _IsiAPIDebug("DomainID  = %x %x %x %x %x %x, Subnet=%d, NonClone=%d Node=%d Invalid=%d Length=%d Key=%x\n", 
                pDomain->Id[0],pDomain->Id[1],pDomain->Id[2],pDomain->Id[3],pDomain->Id[4],pDomain->Id[5],pDomain->Subnet, 
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_NONCLONE),
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_NODE),
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_INVALID),
                IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_ID_LENGTH), pDomain->Key[0]);
        }
    }
     _IsiAPIDebug("End access_domain = %d\n", index);
    return pDomain;
}

/***************************************************************************
 *  Function: update_domain_address
 *
 *  Parameters: const IzotDomain* pDomain, int index
 *
 *  Operation: ISI Update Domain
 ***************************************************************************/
IsiApiError update_domain_address(const IzotDomain* pDomain, int index, int nonCloneValue, IzotBool bUpdateID)
{
    int i;
    IzotDomain* temp = NULL;
    IsiApiError sts = IsiApiNoError;

    _IsiAPIDebug("Start update_domain_address = %d\n", index);
    if (index <= IZOT_GET_ATTRIBUTE(read_only_data, IZOT_READONLY_TWO_DOMAINS)) {
        temp = &domainTable[index];
        if (bUpdateID) {
            for(i=0; i<DOMAIN_ID_LEN; i++) {
		        temp->Id[i] = pDomain->Id[i];
	        };
        }
	    temp->Subnet = pDomain->Subnet;
	    IZOT_SET_ATTRIBUTE_P(temp,IZOT_DOMAIN_NONCLONE,nonCloneValue);  // 0 = if it's a clone domain, 1= otherwise
	    IZOT_SET_ATTRIBUTE_P(temp,IZOT_DOMAIN_NODE, IZOT_GET_ATTRIBUTE_P(pDomain,IZOT_DOMAIN_NODE));
	    temp->InvalidIdLength = pDomain->InvalidIdLength;
        IZOT_SET_ATTRIBUTE_P(temp,IZOT_DOMAIN_INVALID, 0);    // set the domain to be valid.  Otherwise, the LTS will reset the length to 7
        sts = IzotUpdateDomainConfig(index, temp);
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


/***************************************************************************
 *  Function: IsiSetDomain
 *
 *  Parameters: IzotDomain* pDomain, unsigned Index
 *
 *  Operation: Using LTS IzotUpdateDomainConfig API function to update domain of index
 ***************************************************************************/
IsiApiError IsiSetDomain(const IzotDomain* pDomain, unsigned index)
{
    IsiApiError sts;

    _IsiAPIDebug("Start IsiSetDomain = %d\n", index);
    sts = update_domain_address(pDomain, index, 1, TRUE); // it's not a clone domain

    _IsiAPIDebug("End IsiSetDomain = %d\n", sts);
    return sts;
}

/***************************************************************************
 *  Function: _nv_count
 *
 *  Parameters: void
 *
 *  Operation: Return Number of static NVs
 ***************************************************************************/
unsigned _nv_count(void)
{
    unsigned int nvCount = IzotGetStaticDatapointCount();

    // Check against the ISI limit
    if (nvCount > ISI_MAX_NV_COUNT)
        nvCount = ISI_MAX_NV_COUNT;
    return nvCount;
}

/***************************************************************************
 *  Function: IsiSetNv
 *
 *  Parameters: IzotDatapointConfig* pNv, unsigned Index
 *
 *  Operation: ISI Set NV
 ***************************************************************************/
void IsiSetNv(IzotDatapointConfig* pNv, unsigned Index)
{
	update_nv(pNv, Index);
}

/***************************************************************************
 *  Function: update_nv
 *
 *  Parameters: const IzotDatapointConfig * nv_entry, int index
 *
 *  Operation: Update NV
 ***************************************************************************/
void update_nv(const IzotDatapointConfig * nv_entry, unsigned index)
{
    if (nv_entry && index < _nv_count()) {
		/*IsiApiError sts = */IzotUpdateDpConfig(index, nv_entry);

        _IsiAPIDebug("update_nv index %u sts %i ", index, sts);
        _IsiAPIDump("data = 0x", (void *)nv_entry, sizeof(IzotDatapointConfig), "\n");
    }
}

/***************************************************************************
 *  Function: IsiGetNv
 *
 *  Parameters: unsigned Index
 *
 *  Operation: Using LTS IzotQueryDpConfig function to return nv ptr of Index
 ***************************************************************************/
const IzotDatapointConfig* IsiGetNv(unsigned Index)
{
    memset(&nv_config, 0, sizeof(IzotDatapointConfig));
    IzotQueryDpConfig(Index, &nv_config);
    return (IzotDatapointConfig* )&nv_config;
}

/***************************************************************************
 *  Function: high_byte
 *
 *  Parameters: IzotUbits16 a
 *
 *  Operation: Return high byte of a
 ***************************************************************************/
IzotByte high_byte (IzotUbits16 a)
{
	return ((unsigned char)(a>>8));
}

/***************************************************************************
 *  Function: low_byte
 *
 *  Parameters: IzotUbits16 a
 *
 *  Operation: Return low byte of a
 ***************************************************************************/
IzotByte low_byte (IzotUbits16 a)
{
	return ((unsigned char)(a&(0x00FF)));
}

/***************************************************************************
 *  Function: make_long
 *
 *  Parameters: IzotByte low_byte, IzotByte high_byte
 *
 *  Operation: Return combined of high_byte and low_byte
 ***************************************************************************/
IzotWord make_long(IzotByte low_byte, IzotByte high_byte)
{
    IzotWord w;
    IZOT_SET_UNSIGNED_WORD(w, (IzotUbits16)((high_byte<<8)|low_byte));
	return w;
}

/***************************************************************************
 *  Function: getRandom
 *
 *  Parameters: void
 *
 *  Operation: return a random number based on time and the 
 *  least significant 4 bytes of NeuronID.
 *
 ***************************************************************************/
unsigned int getRandom(void)
{
	unsigned m_w = IzotGetTickCount();

	m_z = 36969 * (m_z & 65535) + (m_z >> 4);
	m_w = 18000 * (m_w & 65535) + (m_w >> 4);

	return (m_z << 4) + m_w;  /* 32-bit result */
}

/***************************************************************************
 *  Function: access_address
 *
 *  Parameters: int index
 *
 *  Operation: Using LTS IzotQueryAddressConfig function to return ptr of index
 ***************************************************************************/
const IzotAddress*	access_address(int index)
{
    IzotAddress* pAddress = (IzotAddress*)&addrTable;

    if ((IsiApiError)IzotQueryAddressConfig(index, pAddress) == IsiApiNoError)
        return pAddress;
    else
        return (IzotAddress*)NULL;            
}

/***************************************************************************
 *  Function: update_address
 *
 *  Parameters: const IzotAddress * address, int index
 *
 *  Operation: Using LTS IzotUpdateAddressConfig function to update address of index
 ***************************************************************************/
IsiApiError update_address(const IzotAddress* address, int index)
{
    IsiApiError sts = IzotUpdateAddressConfig(index, address);
	if (sts != IsiApiNoError) {
		_IsiAPIDebug("update_address failed (entry %d)\n", index);
	}
    return sts;
}

/***************************************************************************
 *  Function: lon_watchdog_update
 *
 *  Parameters: void
 *
 *  Operation: 
 ***************************************************************************/
void lon_watchdog_update(void)
{
    // do nothing
}


/***************************************************************************
 *  Function: IsiGetAlias
 *
 *  Parameters: unsigned Index
 *
 *  Operation: using LTS IzotQueryAliasConfig to return ptr to Index
 ***************************************************************************/
const IzotAliasConfig* IsiGetAlias(unsigned Index)
{
    _IsiAPIDebug("Start IsiGetAlias(%d)\n", Index); 
    memset(&alias_config, 0, sizeof(IzotAliasConfig));
    if ((IsiApiError)IzotQueryAliasConfig(Index, &alias_config) != IsiApiNoError)
    {
        _IsiAPIDebug("Error - IsiGetAlias(%d)\n", Index); 
    }
    _IsiAPIDebug("End IsiGetAlias(%d)\n", Index);
	return (IzotAliasConfig*)&alias_config;
}

/***************************************************************************
 *  Function: IsiSetAlias
 *
 *  Parameters: IzotAliasConfig* pAlias, unsigned Index
 *
 *  Operation: using LTS IzotUpdateAliasConfig to update alias
 ***************************************************************************/
IsiApiError IsiSetAlias(IzotAliasConfig* pAlias, unsigned Index)
{
    return IzotUpdateAliasConfig(Index, pAlias); 
}

IsiApiError update_config_data(const IzotConfigData *config_data1)
{
    // Update the global copy of config data
    memcpy(&config_data, config_data1, sizeof(IzotConfigData));
    return IzotUpdateConfigData(config_data1);  // call LTS IzotUpdateConfigData to update the config data 
}

IzotConfigData* get_config_data()
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

/***************************************************************************
 *  Function: get_nv_type
 *
 *  Parameters: unsigned index
 *
 *  Operation: Returns the type of NV  if it is a SNVT (1-250). A non-standard network 
 *  variable type will have SnvtId = 0.  
 ***************************************************************************/
unsigned get_nv_type (unsigned index)
{
	return 1;
}

IzotWord _IsiAddSelector(IzotWord Selector, unsigned Increment)
{
    IzotWord result;
	IZOT_SET_UNSIGNED_WORD(result, IZOT_GET_UNSIGNED_WORD(Selector) + Increment);
    return result;
}

IzotWord _IsiIncrementSelector(IzotWord Selector)
{
    return _IsiAddSelector(Selector, 1);
}

IsiType _IsiGetCurrentType()
{
    return gIsiType;
}

void _IsiSetCurrentType(IsiType type)
{
    gIsiType = type;
}

const unsigned get_nv_length(const unsigned index)
{
    return IzotGetCurrentDatapointSize(index);
}

IzotByte* get_nv_value(const unsigned index)
{
	return (IzotByte *)IzotGetDatapointValue(index);
}

IsiApiError service_pin_msg_send()
{
    return IzotSendServicePin();
}

void node_reset()
{
    NodeReset(FALSE);
}    

IsiApiError retrieve_status(IzotStatus* status)
{
    return IzotQueryStatus(status);       
}

// Initialize the persitent data, connection table, config_data and read_only_data
IsiApiError initializeData(IsiBootType bootType)
{
    IsiApiError sts = IzotQueryConfigData(&config_data);
   
    if (sts == IsiApiNoError) {
        sts = IzotQueryReadOnlyData(&read_only_data);
    }

#if ISI_IS(SIMPLE) || ISI_IS(DA)
	if (restorePersistentData(IsiPersistentSegConnectionTable) != 
    LT_PERSISTENCE_OK) {
		_IsiAPIDebug("No Isi connection table found\r\n");
        _IsiInitConnectionTable();
		return sts;
	}
#endif

	if (restorePersistentData(IsiPersistentSegPersistent) != LT_PERSISTENCE_OK)
    {
		_IsiAPIDebug("No Isi Persistent data found\r\n");
        // First time run.  Signal the engine to clear-out NV, alias, connection table and address tables.
        if (bootType != isiReboot) {
			_isiPersist.BootType = isiReset;
		}
    }
	
    return sts;
}

IsiApiError set_node_mode(unsigned mode, unsigned state)
{
	return IzotSetNodeMode(mode, state);
}

#ifdef  __cplusplus
}
#endif
