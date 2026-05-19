/*
 * SPDX-FileCopyrightText: Copyright 2026 DZG Metering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsa_tag_ksz8863, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag.h>
#include <zephyr/net/ethernet.h>

#include "dsa_ksz8863.h"

static struct net_if *dsa_tag_ksz8863_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_if *iface_dst;
	size_t len;
	uint8_t port_idx;

	len = net_pkt_get_len(pkt);
	if (len < KSZ8863_TAIL_TAG_LEN) {
		LOG_ERR("RX packet too short for KSZ8863 tag");
		return iface;
	}

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, len - KSZ8863_TAIL_TAG_LEN);
	net_pkt_read_u8(pkt, &port_idx);
	net_pkt_update_length(pkt, len - KSZ8863_TAIL_TAG_LEN);

	iface_dst = dsa_user_get_iface(iface, port_idx & BIT_MASK(2));
	if (iface_dst == NULL) {
		LOG_ERR("No DSA user iface for port %u", (unsigned int)(port_idx & BIT_MASK(2)));
		return iface;
	}

	return iface_dst;
}

static struct net_pkt *dsa_tag_ksz8863_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dsa_port_config *cfg = dev->config;
	struct net_buf *tail_buf;
	uint8_t *tail;
	size_t len;
	size_t pad = 0;

	len = net_pkt_get_len(pkt);
	if (len < KSZ8863_MIN_FRAME_NO_FCS) {
		pad = KSZ8863_MIN_FRAME_NO_FCS - len;
	}

	tail_buf = net_buf_alloc_len(net_buf_pool_get(pkt->buffer->pool_id),
				     pad + KSZ8863_TAIL_TAG_LEN, K_NO_WAIT);
	if (tail_buf == NULL) {
		LOG_ERR("Cannot allocate KSZ8863 tail tag buffer");
		return NULL;
	}

	tail = net_buf_simple_tail(&tail_buf->b);
	memset(tail, 0, pad + KSZ8863_TAIL_TAG_LEN);
	tail[pad] = BIT(cfg->port_idx);
	net_buf_add(tail_buf, pad + KSZ8863_TAIL_TAG_LEN);
	net_buf_frag_add(pkt->buffer, tail_buf);

	return pkt;
}

DSA_TAG_REGISTER(DSA_TAG_PROTO_KSZ8863, dsa_tag_ksz8863_recv, dsa_tag_ksz8863_xmit);
