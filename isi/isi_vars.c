/*
 * isi/vars.c
 *
 * Copyright (c) 2005-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Interoperable Self-Installation (ISI) Internal Variables
 * Purpose: <brief purpose>
 * Notes:   The variables defined in this file are only used if ISI is enabled.
 *          Created February 2005, Bernd Gauweiler
 */

#ifdef  __cplusplus
extern "C" {
#endif

#include "izot/IzotPlatform.h"

#include "isi/isi_int.h"
#include <stddef.h>

// eeprom 
IsiPersist _isiPersist = {
#ifdef	ISI_SUPPORT_TIMG
	ISI_DEFAULT_DEVICECOUNT,	// Estimated device count
#endif	//	ISI_SUPPORT_TIMG
	0,							// Local unique ID - this could also be in RAM, but RAM might be more expensive on Shortstack.
	1,							// Serial number for CID creation
	isiReset,					// BootType
	ISI_NV_UPDATE_RETRIES		// Default repeat count for implicit addressing
};

IsiVolatile _isiVolatile;

IsiMessage isi_out;

IsiType gIsiType = isiTypeS;          // Default to ISI Type S (simple with no domain address server)
IsiFlags gIsiFlags;
unsigned char gIsiDerivableAddr = 0;  // Flag to indicate whether the IP address is derivabled or not

IzotReadOnlyData read_only_data;
IzotConfigData config_data;
IzotDomain domainTable[MAX_DOMAINS];  // Retrieve using IzotQueryDomainConfig(const unsigned index, IzotDomain* const pDomain);
IzotAddress addrTable;                // Retrieve using IzotQueryAddressConfig(const unsigned index, IzotAddress* const pAddress);
IzotDatapointConfig nv_config;
IzotAliasConfig alias_config;

unsigned char globalExtended = 0;

#ifdef  __cplusplus
}
#endif
