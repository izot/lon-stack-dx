/*
 * lcs_proxy.c
 *
 * Copyright (c) 2022-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Enhanced Proxy (LTEP) Repeating Implementation
 * Purpose: Implements the LON Enhanced Proxy (LTEP) repeating functions.
 * Notes:   LTEP repeating allows a LON Stack DX device to function
 *          as a proxy repeater for LON Enhanced Proxy messages.
 *          LTEP repeating is only used in systems using LON power line
 *          communication that require the extended range provided
 *          by LTEP repeating.
 * 
 *          Following are the key roles in LTEP repeating:
 * 
 *          - PS: proxy source - initiates transaction chain
 *          - PR: proxy repeater - forwards proxy message
 *          - PA: proxy agent - sends normal message to target
 *          - PT: proxy target - terminates proxy chain
 */

#ifdef ENABLE_PROXY_REPEATING
#include "lcs/lcs_proxy.h"

LonStatusCode ProcessLTEP(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr);

static const unsigned char addrSizeTable[PX_ADDRESS_TYPES] = {
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyGroupAddress),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxySubnetNodeAddress),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyNeuronIdAddress),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyBroadcastAddress),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyGroupAddressCompact),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxySubnetNodeAddressCompact),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyNeuronIdAddressCompact),
    sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxySubnetNodeAddressCompact),
};

void processProxyRepeaterAsAgent(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr, int txcpos)
{
    // Enable a repeater to also serve as an agent.  This is only for
    // unackd/multicast/broadcast.  Allocate an additional buffer.  
    // If not available don't send the message.
    int len = (int)(txcpos+sizeof(ProxyTxCtrl));

    // Make sure packet has a certain minimum size.
    if (len < appReceiveParamPtr->pduSize && appReceiveParamPtr->srcAddr.dmn.domainIndex != FLEX_DOMAIN) {
		APPReceiveParam arp;
		APDU			apdu;

		memcpy(&arp, appReceiveParamPtr, sizeof(arp));
		arp.service				= IzotServiceUnacknowledged;
		arp.proxy				= TRUE;
		arp.pduSize				= arp.pduSize - len + 2;
		apdu.code.allBits       = LT_APDU_ENHANCED_PROXY;
		apdu.data[0]			= 0x00;
		memcpy(&apdu.data[1], &apduPtr->data[len], arp.pduSize-2);
        ProcessLTEP(&arp, &apdu);
    }
}

// Processes an LTEP completion event
LonStatusCode ProcessLtepCompletion(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr, LonStatusCode status)
{
	LonStatusCode sts = LonStatusNoError;
	if (!appReceiveParamPtr->proxyDone) {
		int len = 0;
		IzotByte code = LT_ENHANCED_PROXY_SUCCESS;
		IzotByte data[1];

		if (status != LonStatusNoError) {
			code = LT_ENHANCED_PROXY_FAILURE;
			len = 1;
			data[0] = appReceiveParamPtr->proxyCount;
		}
		sts = SendResponse(appReceiveParamPtr->tag, code, len, data);
	}
	return sts;
}

// Processes LonTalk Enhanced Proxy
LonStatusCode ProcessLTEP(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
    int             offset;
    int             count;
	IzotByte    	proxyCount;
    int             uniform;
    int				alt;
    DestinType      code;
    int             subnet;
	int				node = 0;
    int             src_subnet;
    int             dst_subnet;
    int             txcpos;
    int             addressType = AM_SUBNET_NODE;
    ProxyHeader     ph;
    ProxyTxCtrl     txc;
    int             dataLen = appReceiveParamPtr->pduSize-1;
    IzotByte*       pData = apduPtr->data;
    int             txTimer = 0;
    int             domIdx = appReceiveParamPtr->srcAddr.dmn.domainIndex;
    Queue		   *outQPtr;
	AltKey			altKey;
	IzotByte		longTimer = FALSE;
	IzotServiceType	service;
    int 			adjust = 0;
	IzotSendAddress	addr;

	memset(&addr, 0, sizeof(addr));
	altKey.altKey = false;

    ph = *(ProxyHeader*)pData;
    uniform = ph.uniform_by_src || ph.uniform_by_dest;
    src_subnet = appReceiveParamPtr->srcAddr.subnetAddr.Subnet;
    dst_subnet = eep->domainTable[domIdx].Subnet;
    if (ph.uniform_by_src) {
        subnet = src_subnet;
    } else {
        subnet = dst_subnet;
    }
    proxyCount = count = ph.count;
    txcpos = (int)(sizeof(ProxyHeader) + (uniform ?
                                          sizeof(ProxySubnetNodeAddressCompact)*count :
                                          sizeof(ProxySubnetNodeAddress)*count));

    // Get txctrl values
    txc = *(ProxyTxCtrl*)(&pData[txcpos]);

    if (ph.all_agents && count) {
        processProxyRepeaterAsAgent(appReceiveParamPtr, apduPtr, txcpos);
    }

    // If we got something on the flex domain, we just use domain index 0.
    if (domIdx == FLEX_DOMAIN) {
        domIdx = 0;
	}

    if (count != 0) {
        ProxySubnetNodeAddress *pa;

        pa = (ProxySubnetNodeAddress*) (&pData[sizeof(ProxyHeader)] - uniform);
        alt = pa->path;
        txTimer = txc.timer;
		longTimer = ph.long_timer;
		subnet = uniform ? subnet : pa->subnet;
		node = pa->node;
	    service  = appReceiveParamPtr->service;

        count--;

        // Skip header and address
        offset = (int)(sizeof(ProxyHeader) + sizeof(ProxySubnetNodeAddress) - uniform);

        if (count == 0) {
            // Last address.  Skip txctrl too.
            offset += (int)sizeof(ProxyTxCtrl);
        }

        // To handle the case where repeated messages time out, allow for the fact that each
        // repeater in the chain needs a little bit more time on the last timeout that the
        // previous repeater.  So, we allow 512 msec for each round trip to propagate the failure
        // message.  This means adding 512*count msec at each hop.
        // This is timed for A-band power line.  This may not be necessary of higher-speed channels.
        // The adjustment is deployed as a function of the tx_timer.
        adjust = 256;                               // Add constant 256 msec at every hop
        if (ph.long_timer || txc.timer >= 10) adjust = 512*count;     // Add 512 msec per hop
        else if (txc.timer >= 8) adjust = 256*count;// Add 256 msec per hop

        // Send message on to next PR or PA
        code.allBits = LT_APDU_ENHANCED_PROXY;
        ph.count--;
        // Position new header into data space in preparation for copy below.
        pData[--offset] = *(unsigned char*)&ph;
    } else {
        // Proxy Agent mode - no more addresses.
        ProxySicb* pProxySicb;

        pProxySicb = (ProxySicb*)&pData[sizeof(ProxyHeader)];
        txc = pProxySicb->txctrl;

        service = (IzotServiceType)pProxySicb->service;

        // Set some explicit address fields.  These are assumed
        // to be the same for all address types.

        addressType = pProxySicb->type;
        offset = addrSizeTable[addressType];
        txTimer = txc.timer;

        // Remove "compact" bit
        addressType &= 0x3;

        {
            ProxyTargetAddress* pAddress;

            pAddress = (ProxyTargetAddress*)(pProxySicb + 1);

            if (pProxySicb->mode == PROXY_AGENT_MODE_ALTKEY) {
                int i;
				int len;
				int domainIndex = domIdx;
                ProxyAuthKey* pKey;
                IzotByte            *pKeyDelta;

                pKey = (ProxyAuthKey*)pAddress;
				altKey.altKey = TRUE;
				// The altKeyOma field only tells us the length of the key.  Whether or not OMA is used is dependent on the 
				// type of challenge received which is based on the configuration of the target domain index 0.
                len = (unsigned char)(pKey->type == AUTH_OMA ? sizeof(((ProxyOmaKey*)pKey)->key):sizeof(pKey->key));

                pKeyDelta = pKey->key;
				i = 0;
				while (domainIndex < 2) {
					int j;
                    for (j=0; j<DOMAIN_ID_LEN; j++) {
                        altKey.altKeyValue[i][j] = eep->domainTable[domainIndex].Key[j] + *pKeyDelta;
                        pKeyDelta++;
                    }
					domainIndex++;
					i++;
                }
                offset += (int)(sizeof(ProxyAuthKey)-sizeof(pKey->key)+len);
                pAddress = (ProxyTargetAddress*)((unsigned char*)pAddress + (int)(sizeof(ProxyAuthKey)-sizeof(pKey->key)+len));
            }

            switch (pProxySicb->type) {
            case PX_NEURON_ID:
				memcpy(addr.uniqueNodeId.NeuronId, pAddress->nid.nid, sizeof(addr.uniqueNodeId.NeuronId));
				subnet = pAddress->nid.subnet;
                break;
            case PX_NEURON_ID_COMPACT:
				memcpy(addr.uniqueNodeId.NeuronId, pAddress->nidc.nid, sizeof(addr.uniqueNodeId.NeuronId));
				subnet = 0;
                break;
            case PX_SUBNET_NODE_COMPACT_SRC:
				subnet = src_subnet;
				node = pAddress->snc.node;
                break;
            case PX_SUBNET_NODE_COMPACT_DEST:
				subnet = dst_subnet;
				node = pAddress->snc.node;
				addressType = AM_SUBNET_NODE;
                break;
            case PX_GROUP:
                addressType = 0x80 | pAddress->gp.size;
				subnet = pAddress->gp.group;
                break;
            case PX_GROUP_COMPACT:
                addressType = 0x80;
				subnet = pAddress->gp.group;
                break;
            case PX_BROADCAST:
				subnet = pAddress->bc.subnet;
				node = pAddress->bc.backlog;
                break;
            case PX_SUBNET_NODE:
				subnet = pAddress->sn.subnet;
				node = pAddress->sn.node;
                break;
            default:
                // Force failed completion from lower layers.
                addressType = 0x7f;
                break;
            }
        }

        code.allBits = pData[offset++];
        alt = pProxySicb->path;
    }

	addr.snode.longTimer    = longTimer;
	addr.snode.addrMode		= addressType;
	addr.snode.txTimer		= txTimer;
	addr.snode.rptTimer     = txTimer;
	addr.snode.retryCount	= txc.retry;
	addr.snode.node			= node;
	addr.snode.subnetID		= subnet;
	addr.snode.domainIndex  = domIdx;  			// Set the domain index to be the one in which it was received. 
	if (addressType & 0x80) {
		addr.noAddress = addressType;
	}

	// Get an output buffer for the proxy relay
    if (appReceiveParamPtr->priority) {
        outQPtr = (service==IzotServiceUnacknowledged) ? &gp->nwOutPriQ : &gp->tsaOutPriQ;
    } else {
        outQPtr = (service==IzotServiceUnacknowledged) ? &gp->nwOutQ : &gp->tsaOutQ;
    }

    /* Check if the target queue has space for forwarding this request. */
    if (QueueFull(outQPtr)) {
        // Failure indicates we didn't process the message so don't free it!  
	    // Note that the following deadlock condition can occur:
	    // App Input queue full but stymied by proxy relay
		// TSA queue full but stymied because there are no App Input buffers to send completion event to.
		// On the Neuron, this can't occur because completion events are sent in the output buffer, not a new input buffer.
		// LCS should probably be implemented more like the Neuron.  In the meantime, let's just have a timeout where
		// we fail the proxy if we can't get an output buffer.
		if (LonTimerExpired(&gp->proxyBufferWait)) {
			SendResponse(appReceiveParamPtr->reqId, LT_ENHANCED_PROXY_FAILURE, sizeof(proxyCount), &proxyCount);
			return LonStatusNoError;
		} else if (!LonTimerRunning(&gp->proxyBufferWait)) {
			SetLonTimer(&gp->proxyBufferWait, 1000);
		}
        return LonStatusNoBufferAvailable;
    } else {
		SetLonTimer(&gp->proxyBufferWait, 0);
	}
    // Include sanity checks on incoming packet length
    if (dataLen >= (int)(sizeof(ProxyHeader) + sizeof(ProxySicb) + 1) && offset <= dataLen) {
		LonStatusCode sts = LonStatusNoError;
		dataLen -= offset;
		if (service == IzotServiceUnacknowledged) {
			PktCtrl ctrl = alt ? PKT_ALTPATH|PKT_PROXY : PKT_PROXY;
			sts = AllocSendUnackd(ctrl, appReceiveParamPtr->reqId, &addr, code, pData[offset], dataLen, &pData[offset+1]);
		} else if (dataLen+1 > gp->tsaOutBufSize) {
			sts = LonStatusInvalidBufferLength;
		} else {						
			TSASendParam   *tsaSendParamPtr = QueueTail(outQPtr);
			APDU		   *apduSendPtr 	= (APDU *)(tsaSendParamPtr + 1);
	
			tsaSendParamPtr->dmn.domainIndex= COMPUTE_DOMAIN_INDEX;
			tsaSendParamPtr->service 		= service;
			tsaSendParamPtr->txTimerDeltaLast = adjust;
			tsaSendParamPtr->auth			= appReceiveParamPtr->auth;
			tsaSendParamPtr->altKey		    = altKey;
			// See explanation in HandleNDProxyCommand().
			tsaSendParamPtr->tag      		= appReceiveParamPtr->reqId;
			tsaSendParamPtr->altPathOverride = true;
			tsaSendParamPtr->altPath 		= alt;
			tsaSendParamPtr->priority 		= appReceiveParamPtr->priority;
			tsaSendParamPtr->proxy	  		= TRUE;
			tsaSendParamPtr->proxyCount 	= proxyCount;
			tsaSendParamPtr->proxyDone 		= FALSE;
			// txInherit relies on the tag/reqId relationship above as well (see SendNewMsg())
			tsaSendParamPtr->txInherit 		= TRUE;		
			memcpy(apduSendPtr->data, &pData[offset], dataLen);
			tsaSendParamPtr->apduSize 		= dataLen+1;
			apduSendPtr->code		  		= code;
			tsaSendParamPtr->destAddr 		= addr;
			QueueWrite(outQPtr);
		}
		if (sts != LonStatusNoError) {
			SendResponse(appReceiveParamPtr->reqId, LT_ENHANCED_PROXY_FAILURE, sizeof(proxyCount), &proxyCount);
		}
	}
	return LonStatusNoError;
}
#endif // ENABLE_PROXY_REPEATING
