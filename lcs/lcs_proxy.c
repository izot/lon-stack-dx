//
// lcs_proxy.c
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
// LonTalk Enhanced Proxy library
//
//  PS - proxy source - initiates transaction chain.
//  PR - proxy repeater - forwards proxy message.
//  PA - proxy agent - sends normal message to target.
//  PT - proxy target - terminates proxy chain.
//

#include <stddef.h>
#include <string.h>

#include "lcs_eia709_1.h"
#include "lcs_node.h"
#include "lcs_app.h"
#include "lcs_api.h"
#include "lcs_netmgmt.h"
#include "lcs_proxy.h"

Status ProcessLTEP(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr);

static const unsigned char addrSizeTable[PX_ADDRESS_TYPES] =
{
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
    // Even though I am a repeater, I must also serve as an agent.  This
    // intended only for unackd/multicast/broadcast.  For this case, we
    // must allocate an additional buffer.  If not available, then we
    // just don't send the message.
    int len = (int)(txcpos+sizeof(ProxyTxCtrl));

    // Make sure packet has a certain minimum size.
    if (len < appReceiveParamPtr->pduSize && appReceiveParamPtr->srcAddr.dmn.domainIndex != FLEX_DOMAIN)
    {
		APPReceiveParam arp;
		APDU			apdu;

		memcpy(&arp, appReceiveParamPtr, sizeof(arp));
		arp.service				= UNACKD;
		arp.proxy				= TRUE;
		arp.pduSize				= arp.pduSize - len + 2;
		apdu.code.allBits       = LT_APDU_ENHANCED_PROXY;
		apdu.data[0]			= 0x00;
		memcpy(&apdu.data[1], &apduPtr->data[len], arp.pduSize-2);
        ProcessLTEP(&arp, &apdu);
    }
}

//
// processLtepResponse
//
// Process a LTEP completion event
//
Status ProcessLtepCompletion(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr, Status status)
{
	Status sts = SUCCESS;
	if (!appReceiveParamPtr->proxyDone)
	{
		int len = 0;
		Byte code = LT_ENHANCED_PROXY_SUCCESS;
		Byte data[1];

		if (status == FAILURE)
		{
			code = LT_ENHANCED_PROXY_FAILURE;
			len = 1;
			data[0] = appReceiveParamPtr->proxyCount;
		}
		sts = SendResponse(appReceiveParamPtr->tag, code, len, data);
	}
	return sts;
}

//
// processLtep
//
// Process LonTalk Enhanced Proxy
//
Status ProcessLTEP(APPReceiveParam *appReceiveParamPtr, APDU *apduPtr)
{
    int             offset;
    int             count;
    int             uniform;
    int				alt;
    int             code;
    int             subnet;
	int				node;
    int             src_subnet;
    int             dst_subnet;
    int             txcpos;
    int             addressType = SUBNET_NODE;
    ProxyHeader     ph;
    ProxyTxCtrl     txc;
    int             dataLen = appReceiveParamPtr->pduSize-1;
    Byte*           pData = apduPtr->data;
    int             txTimer = 0;
    int             domIdx = appReceiveParamPtr->srcAddr.dmn.domainIndex;
    Queue		   *tsaOutQPtr;
	TSASendParam   *tsaSendParamPtr;
	APDU		   *apduSendPtr;
	Boolean			altKey = FALSE;
	Boolean			longTimer = FALSE;
	static TmrTimer proxyBufferWait;

    ph = *(ProxyHeader*)pData;
    uniform = ph.uniform_by_src || ph.uniform_by_dest;
    src_subnet = appReceiveParamPtr->srcAddr.subnetAddr.subnet;
    dst_subnet = eep->domainTable[domIdx].subnet;
    if (ph.uniform_by_src)
    {
        subnet = src_subnet;
    }
    else
    {
        subnet = dst_subnet;
    }
    count = ph.count;
    txcpos = (int)(sizeof(ProxyHeader) + (uniform ?
                                          sizeof(ProxySubnetNodeAddressCompact)*count :
                                          sizeof(ProxySubnetNodeAddress)*count));

    // Get txctrl values
    txc = *(ProxyTxCtrl*)(&pData[txcpos]);

    if (ph.all_agents && count)
    {
        processProxyRepeaterAsAgent(appReceiveParamPtr, apduPtr, txcpos);
    }

	// Get an output buffer for the proxy relay
    if (appReceiveParamPtr->priority)
    {
        tsaOutQPtr = &gp->tsaOutPriQ;
    }
    else
    {
        tsaOutQPtr = &gp->tsaOutQ;
    }

    /* Check if the target queue has space for forwarding this request. */
    if (QueueFull(tsaOutQPtr))
    {
        // Failure indicates we didn't process the message so don't free it!  
	    // Note that the following deadlock condition can occur:
	    // App Input queue full but stymied by proxy relay
		// TSA queue full but stymied because there are no App Input buffers to send completion event to.
		// On the Neuron, this can't occur because completion events are sent in the output buffer, not a new input buffer.
		// LCS should probably be implemented more like the Neuron.  In the meantime, let's just have a timeout where
		// we fail the proxy if we can't get an output buffer.
		if (TMR_Expired(&proxyBufferWait))
		{
			ProcessLtepCompletion(appReceiveParamPtr, apduPtr, FAILURE);
			return SUCCESS;
		}
		else if (!TMR_Running(&proxyBufferWait))
		{
			TMR_Start(&proxyBufferWait, 1000);
		}
        return FAILURE;
    }
	else
	{
		TMR_Stop(&proxyBufferWait);
	}

    tsaSendParamPtr					= QueueTail(tsaOutQPtr);
    apduSendPtr						= (APDU *)(tsaSendParamPtr + 1);

    // If we got something on the flex domain, we just use domain index 0.
    if (domIdx == FLEX_DOMAIN)
	{
        domIdx = 0;
	}

    if (count != 0)
    {
        int adjust;
        ProxySubnetNodeAddress *pa;

        pa = (ProxySubnetNodeAddress*) (&pData[sizeof(ProxyHeader)] - uniform);
        alt = pa->path;
        txTimer = txc.timer;
		longTimer = ph.long_timer;
		subnet = uniform ? subnet : pa->subnet;
		node = pa->node;
	    tsaSendParamPtr->service  = appReceiveParamPtr->service;

        count--;

        // Skip header and address
        offset = (int)(sizeof(ProxyHeader) + sizeof(ProxySubnetNodeAddress) - uniform);

        if (count == 0)
        {
            // Last address.  Skip txctrl too.
            offset += (int)sizeof(ProxyTxCtrl);
        }

        // To handle the case where repeated messages time out, we need to allow for
        // the fact that each repeater in the chain needs a little bit more time on the last timeout
        // than the previous guy.  So, we allow 512 msec for each round trip to propagate the failure
        // message.  This means adding 512*count msec at each hop.
        // This kludge is timed for A band power line.  If this proxy mechanism were employed in just about
        // any other medium, this would not be necessary.  So, the adjustment is deployed as a function of
        // the tx_timer.
        adjust = 256;                               // Add constant 256 msec at every hop
        if (ph.long_timer || txc.timer >= 10) adjust = 2*count;     // Add 512 msec per hop
        else if (txc.timer >= 8) adjust = 256*count;// Add 256 msec per hop
		tsaSendParamPtr->txTimerDeltaLast = adjust;

        // Send message on to next PR or PA
        code = LT_APDU_ENHANCED_PROXY;
        ph.count--;
        // Position new header into data space in preparation for copy below.
        pData[--offset] = *(unsigned char*)&ph;
    }
    else // Proxy Agent
    {
        ProxySicb* pProxySicb;

        pProxySicb = (ProxySicb*)&pData[sizeof(ProxyHeader)];
        txc = pProxySicb->txctrl;

        tsaSendParamPtr->service = (ServiceType)pProxySicb->service;
		tsaSendParamPtr->txTimerDeltaLast = 0;

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

            if (pProxySicb->mode == PROXY_AGENT_MODE_ALTKEY)
            {
                int i;
				int len;
				int domainIndex = domIdx;
                ProxyAuthKey* pKey;
                Byte            *pKeyDelta;

                pKey = (ProxyAuthKey*)pAddress;
				altKey = TRUE;
				// The altKeyOma field only tells us the length of the key.  Whether or not OMA is used is dependent on the 
				// type of challenge received which is based on the configuration of the target domain index 0.
                len = (unsigned char)(pKey->type == AUTH_OMA ? sizeof(((ProxyOmaKey*)pKey)->key):sizeof(pKey->key));

                pKeyDelta = pKey->key;
				i = 0;
				while (domainIndex < 2)
                {
					int j;
                    for (j=0; j<DOMAIN_ID_LEN; j++)
                    {
                        tsaSendParamPtr->altKey.altKeyValue[i][j] = eep->domainTable[domainIndex].key[j] + *pKeyDelta;
                        pKeyDelta++;
                    }
					domainIndex++;
					i++;
                }
                offset += (int)(sizeof(ProxyAuthKey)-sizeof(pKey->key)+len);
                pAddress = (ProxyTargetAddress*)((unsigned char*)pAddress + (int)(sizeof(ProxyAuthKey)-sizeof(pKey->key)+len));
            }

            switch (pProxySicb->type)
            {
            case PX_NEURON_ID:
				memcpy(tsaSendParamPtr->destAddr.uniqueNodeId.uniqueId, pAddress->nid.nid, sizeof(tsaSendParamPtr->destAddr.uniqueNodeId.uniqueId));
				subnet = pAddress->nid.subnet;
                break;
            case PX_NEURON_ID_COMPACT:
				memcpy(tsaSendParamPtr->destAddr.uniqueNodeId.uniqueId, pAddress->nidc.nid, sizeof(tsaSendParamPtr->destAddr.uniqueNodeId.uniqueId));
				subnet = 0;
                break;
            case PX_SUBNET_NODE_COMPACT_SRC:
				subnet = src_subnet;
				node = pAddress->snc.node;
                break;
            case PX_SUBNET_NODE_COMPACT_DEST:
				subnet = dst_subnet;
				node = pAddress->snc.node;
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

        code = pData[offset++];
        alt = pProxySicb->path;
    }

    // Include sanity checks on incoming packet length
    if (dataLen >= (int)(sizeof(ProxyHeader) + sizeof(ProxySicb) + 1) && offset <= dataLen)
    {
		SNodeAddrMode *pSn = &tsaSendParamPtr->destAddr.snode;
		memset(pSn, 0, sizeof(*pSn));

		pSn->longTimer      = longTimer;
		pSn->addrMode		= addressType;
		pSn->txTimer		= txTimer;
		pSn->retryCount		= txc.retry;
		pSn->node			= node;
		pSn->subnetID		= subnet;

		/* Set the domain index to be the one in which it was received.
		   Note that this cannot be flex domain. Transport or session
		   will use this instead of the one in the destAddr (i.e msg_out_addr) */
		tsaSendParamPtr->dmn.domainIndex= domIdx;
		tsaSendParamPtr->auth			= appReceiveParamPtr->auth;
		tsaSendParamPtr->altKey.altKey  = altKey;
		// See explanation in HandleNDProxyCommand().
		tsaSendParamPtr->tag      = appReceiveParamPtr->reqId;
	    tsaSendParamPtr->altPathOverride = TRUE;
	    tsaSendParamPtr->altPath = alt;
		tsaSendParamPtr->priority = appReceiveParamPtr->priority;
		tsaSendParamPtr->proxy	  = TRUE;
		tsaSendParamPtr->proxyCount = count;
		tsaSendParamPtr->proxyDone = FALSE;
		// txInherit relies on the tag/reqId relationship above as well (see SendNewMsg())
		tsaSendParamPtr->txInherit = TRUE;		
        memcpy(apduSendPtr->data, &pData[offset], dataLen-offset);
		tsaSendParamPtr->apduSize = dataLen-offset+1;
        apduSendPtr->code.allBits = code;
	    EnQueue(tsaOutQPtr);
    }
	return SUCCESS;
}

