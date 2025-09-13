/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_tag_netc, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag_netc.h>
#include "fsl_netc_tag.h"

struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	const struct dsa_switch_context *dsa_switch_ctx = eth_ctx->dsa_switch_ctx;
#ifdef CONFIG_NET_L2_PTP
	struct dsa_tag_netc_data *tagger_data =
		(struct dsa_tag_netc_data *)(dsa_switch_ctx->tagger_data);
#endif
	void *header = pkt->frags->data;
	uint16_t tag_len = sizeof(netc_swt_tag_host_t);
	netc_swt_tag_common_t *tag_common;
	struct net_if *iface_dst = iface;
	uint8_t *ptr;

	/* Tag is inserted after DMAC/SMAC fields. Check the smallest tag type header size. */
	if (pkt->frags->len < NET_ETH_ADDR_LEN * 2 + tag_len) {
		LOG_ERR("tag len error");
		return iface_dst;
	}

	/* Handle tag type */
	tag_common = (netc_swt_tag_common_t *)((uintptr_t)pkt->frags->data + NET_ETH_ADDR_LEN * 2);
	if (tag_common->type == kNETC_TagForward) {

		/* Update tag length per tag type */
		tag_len = sizeof(netc_swt_tag_forward_t);

	} else if (tag_common->type == kNETC_TagToHost) {
#ifdef CONFIG_NET_L2_PTP
		netc_swt_tag_host_rx_ts_t *tag_rx_ts;
		netc_swt_tag_host_tx_ts_t *tag_tx_ts;
		uint64_t ts;
#endif
		/* Handle tag sub-type */
		switch (tag_common->subType) {
		case kNETC_TagToHostNoTs:
			/* Normal case */
			break;
		case kNETC_TagToHostRxTs:
#ifdef CONFIG_NET_L2_PTP
			tag_rx_ts = (netc_swt_tag_host_rx_ts_t *)tag_common;
			ts = ntohll(tag_rx_ts->timestamp);

			/* Fill timestamp */
			pkt->timestamp.nanosecond = ts % NSEC_PER_SEC;
			pkt->timestamp.second = ts / NSEC_PER_SEC;
#endif
			/* Update tag length per tag type */
			tag_len = sizeof(netc_swt_tag_host_rx_ts_t);
			break;
		case kNETC_TagToHostTxTs:
#ifdef CONFIG_NET_L2_PTP
			tag_tx_ts = (netc_swt_tag_host_tx_ts_t *)tag_common;
			ts = ntohll(tag_tx_ts->timestamp);

			if (tagger_data->twostep_timestamp_handler != NULL) {
				tagger_data->twostep_timestamp_handler(dsa_switch_ctx,
					tag_tx_ts->tsReqId, ts);
			}
#endif
			/* Update tag length per tag type */
			tag_len = sizeof(netc_swt_tag_host_tx_ts_t);
			break;
		default:
			LOG_ERR("tag sub-type error");
			break;
		}
	} else {
		LOG_ERR("tag type error");
	}

	/* redirect to user port */
	iface_dst = dsa_switch_ctx->iface_user[tag_common->port];

	/* drop tag */
	ptr = net_buf_pull(pkt->frags, tag_len);
	for (int i = 0; i < (NET_ETH_ADDR_LEN * 2); i++) {
		ptr[i] = *(uint8_t *)((uintptr_t)header + i);
	}

	return iface_dst;
}

struct net_pkt *dsa_tag_netc_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_port_config *cfg = (struct dsa_port_config *)dev->config;
	struct net_buf *header_buf;
	size_t header_len = NET_ETH_ADDR_LEN * 2;
	netc_swt_tag_common_t *tag_common;
	void *tag;

	/* Tag is inserted after DMAC/SMAC fields. Decide header size per tag type. */
	if (ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP) {
		header_len += sizeof(netc_swt_tag_port_two_step_ts_t);
	} else {
		header_len += sizeof(netc_swt_tag_port_no_ts_t);
	}

	/* Allocate net_buf for header */
	header_buf = net_buf_alloc_len(net_buf_pool_get(pkt->buffer->pool_id),
				       header_len, K_NO_WAIT);
	if (!header_buf) {
		LOG_ERR("Cannot allocate header buffer");
		return NULL;
	}

	header_buf->len = header_len;

	/* Fill the header */
	memcpy(header_buf->data, pkt->frags->data, NET_ETH_ADDR_LEN * 2);
	tag = (void *)((uintptr_t)header_buf->data + NET_ETH_ADDR_LEN * 2);

#ifdef CONFIG_NET_L2_PTP
	/* Enable two-step timestamping for gPTP. */
	if (ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP) {

		/* Utilize control block for timestamp request ID */
		((netc_swt_tag_port_two_step_ts_t *)tag)->tsReqId = pkt->cb.cb[0] & 0xf;

		tag_common = &((netc_swt_tag_port_two_step_ts_t *)tag)->comTag;
		tag_common->subType = kNETC_TagToPortTwoStepTs;
	} else {
		tag_common = &((netc_swt_tag_port_no_ts_t *)tag)->comTag;
		tag_common->subType = kNETC_TagToPortNoTs;
	}
#else
	tag_common = &((netc_swt_tag_port_no_ts_t *)tag)->comTag;
	tag_common->subType = kNETC_TagToPortNoTs;
#endif
	tag_common->tpid = NETC_SWITCH_DEFAULT_ETHER_TYPE;
	tag_common->type = kNETC_TagToPort;
	tag_common->swtId = 1;
	tag_common->port = cfg->port_idx;

	/* Drop DMAC/SMAC on original frag */
	net_buf_pull(pkt->frags, NET_ETH_ADDR_LEN * 2);

	/* Insert header */
	header_buf->frags = pkt->frags;
	pkt->frags = header_buf;

	net_pkt_cursor_init(pkt);
	return pkt;
}
