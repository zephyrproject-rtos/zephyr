/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsa_tag_netc, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag.h>

#include "dsa_tag_netc.h"

struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	const struct dsa_switch_context *dsa_switch_ctx = eth_ctx->dsa_switch_ctx;
#ifdef CONFIG_NET_L2_PTP
	struct dsa_tag_netc_data *tagger_data =
		(struct dsa_tag_netc_data *)(dsa_switch_ctx->tagger_data);
#endif
	void *header = pkt->frags->data;
	uint16_t tag_len = sizeof(struct netc_switch_tag_host);
	struct netc_switch_tag_common *tag_common;
	struct net_if *iface_dst = iface;
	uint8_t *ptr;

	/* Tag is inserted after DMAC/SMAC fields. Check the smallest tag type header size. */
	if (pkt->frags->len < NET_ETH_ADDR_LEN * 2 + tag_len) {
		LOG_ERR("tag len error");
		return iface_dst;
	}

	/* Handle tag type */
	tag_common = (struct netc_switch_tag_common *)((uintptr_t)pkt->frags->data +
			NET_ETH_ADDR_LEN * 2);
	if (tag_common->type == NETC_SWITCH_TAG_TYPE_FORWARD) {

		/* Update tag length per tag type */
		tag_len = sizeof(struct netc_switch_tag_forward);

	} else if (tag_common->type == NETC_SWITCH_TAG_TYPE_TO_HOST) {
#ifdef CONFIG_NET_L2_PTP
		struct netc_switch_tag_host_rx_ts *tag_rx_ts;
		struct netc_switch_tag_host_tx_ts *tag_tx_ts;
		uint64_t ts;
#endif
		/* Handle tag sub-type */
		switch (tag_common->subtype) {
		case NETC_SWITCH_TAG_SUBTYPE_TO_HOST_NO_TS:
			/* Normal case */
			break;
		case NETC_SWITCH_TAG_SUBTYPE_TO_HOST_RX_TS:
#ifdef CONFIG_NET_L2_PTP
			tag_rx_ts = (struct netc_switch_tag_host_rx_ts *)tag_common;
			ts = net_ntohll(tag_rx_ts->timestamp);

			/* Fill timestamp */
			pkt->timestamp.nanosecond = ts % NSEC_PER_SEC;
			pkt->timestamp.second = ts / NSEC_PER_SEC;
#endif
			/* Update tag length per tag type */
			tag_len = sizeof(struct netc_switch_tag_host_rx_ts);
			break;
		case NETC_SWITCH_TAG_SUBTYPE_TO_HOST_TX_TS:
#ifdef CONFIG_NET_L2_PTP
			tag_tx_ts = (struct netc_switch_tag_host_tx_ts *)tag_common;
			ts = net_ntohll(tag_tx_ts->timestamp);

			if (tagger_data->twostep_timestamp_handler != NULL) {
				tagger_data->twostep_timestamp_handler(dsa_switch_ctx,
					tag_tx_ts->ts_req_id, ts);
			}
#endif
			/* Update tag length per tag type */
			tag_len = sizeof(struct netc_switch_tag_host_tx_ts);
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
	struct netc_switch_tag_common *tag_common;
	void *tag;

	/* Tag is inserted after DMAC/SMAC fields. Decide header size per tag type. */
	if (net_ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP) {
		header_len += sizeof(struct netc_switch_tag_port_two_step_ts);
	} else {
		header_len += sizeof(struct netc_switch_tag_port_no_ts);
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
	if (net_ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP) {

		/* Utilize control block for timestamp request ID */
		((struct netc_switch_tag_port_two_step_ts *)tag)->ts_req_id = pkt->cb.cb[0] & 0xf;

		tag_common = &((struct netc_switch_tag_port_two_step_ts *)tag)->common;
		tag_common->subtype = NETC_SWITCH_TAG_SUBTYPE_TO_PORT_TWOSTEP_TS;
	} else {
		tag_common = &((struct netc_switch_tag_port_no_ts *)tag)->common;
		tag_common->subtype = NETC_SWITCH_TAG_SUBTYPE_TO_PORT_NO_TS;
	}
#else
	tag_common = &((struct netc_switch_tag_port_no_ts *)tag)->common;
	tag_common->subtype = NETC_SWITCH_TAG_SUBTYPE_TO_PORT_NO_TS;
#endif
	tag_common->tpid = NETC_SWITCH_ETHER_TYPE;
	tag_common->type = NETC_SWITCH_TAG_TYPE_TO_PORT;
	tag_common->swtid = 1;
	tag_common->port = cfg->port_idx;

	/* Drop DMAC/SMAC on original frag */
	net_buf_pull(pkt->frags, NET_ETH_ADDR_LEN * 2);

	/* Insert header */
	header_buf->frags = pkt->frags;
	pkt->frags = header_buf;

	net_pkt_cursor_init(pkt);
	return pkt;
}

DSA_TAG_REGISTER(DSA_TAG_PROTO_NETC, dsa_tag_netc_recv, dsa_tag_netc_xmit);
