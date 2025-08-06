
//
// vars.c
//
// Copyright (C) 2023-2025 EnOcean
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
 * Created February 2005, Bernd Gauweiler
 *
 * Abstract:
 * Implement the ISI internal variables
 */

#ifdef  __cplusplus
extern "C" {
#endif

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
