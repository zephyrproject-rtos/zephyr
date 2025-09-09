/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_tag_netc, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>
#include "dsa_tag_netc.h"
#include "fsl_netc_tag.h"

/* tag is inserted after DMAC/SMAC fields */
struct dsa_tag_netc_to_host_header {
	uint8_t dmac[NET_ETH_ADDR_LEN];
	uint8_t smac[NET_ETH_ADDR_LEN];
	netc_swt_tag_host_t tag;
};

struct dsa_tag_netc_to_port_header {
	uint8_t dmac[NET_ETH_ADDR_LEN];
	uint8_t smac[NET_ETH_ADDR_LEN];
	netc_swt_tag_port_no_ts_t tag;
};

struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	uint16_t header_len = sizeof(struct dsa_tag_netc_to_host_header);
	struct dsa_tag_netc_to_host_header *header;
	struct net_if *iface_dst = iface;
	uint8_t *ptr;

	if (pkt->frags->len < header_len) {
		LOG_ERR("tag len error");
		return iface_dst;
	}

	/* redirect to user port */
	header = (struct dsa_tag_netc_to_host_header *)pkt->frags->data;
	iface_dst = eth_ctx->dsa_switch_ctx->iface_user[header->tag.comTag.port];

	/* drop tag */
	ptr = net_buf_pull(pkt->frags, sizeof(netc_swt_tag_host_t));
	for (int i = 0; i < (NET_ETH_ADDR_LEN * 2); i++) {
		ptr[i] = *(uint8_t *)((uintptr_t)header + i);
	}

	return iface_dst;
}

struct net_pkt *dsa_tag_netc_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_port_config *cfg = (struct dsa_port_config *)dev->config;
	struct dsa_tag_netc_to_port_header *header;
	struct net_buf *header_buf;

	/* Allocate net_buf for header */
	header_buf = net_buf_alloc_len(net_buf_pool_get(pkt->buffer->pool_id),
				       sizeof(*header), K_NO_WAIT);
	if (!header_buf) {
		LOG_ERR("Cannot allocate header buffer");
		return NULL;
	}

	header_buf->len = sizeof(*header);

	/* Fill the header */
	header = (struct dsa_tag_netc_to_port_header *)header_buf->data;
	memcpy(header, pkt->frags->data, NET_ETH_ADDR_LEN * 2);
	header->tag.comTag.tpid = NETC_SWITCH_DEFAULT_ETHER_TYPE;
	header->tag.comTag.subType = kNETC_TagToPortNoTs;
	header->tag.comTag.type = kNETC_TagToPort;
	header->tag.comTag.swtId = 1;
	header->tag.comTag.port = cfg->port_idx;

	/* Drop DMAC/SMAC on original frag */
	net_buf_pull(pkt->frags, NET_ETH_ADDR_LEN * 2);

	/* Insert header */
	header_buf->frags = pkt->frags;
	pkt->frags = header_buf;

	net_pkt_cursor_init(pkt);
	return pkt;
}
