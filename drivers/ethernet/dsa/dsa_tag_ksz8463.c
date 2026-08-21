/*
 * Copyright (c) 2026 Vilhelm Engström
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/ethernet/dsa_tag_proto.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_log.h>
#include <zephyr/toolchain.h>

#include "dsa_ksz8463.h"

LOG_MODULE_REGISTER(dsa_tag_ksz8463, CONFIG_ETHERNET_LOG_LEVEL);

/*
 * The KSZ8463 supports inserting a single-byte tag following the
 * payload in each Ethernet frame. Thus, a tagged VLAN frame would
 * look something akin to
 *                                             Inserted
 *                                                 v
 * +------+------+------+-----+-------+---------+-----+-----+
 * | DMAC | SMAC | VPID | TCI | PTYPE | Payload | Tag | FCS |
 * +------+------+------+-----+-------+---------+-----+-----+
 *
 * The figure is obviously not to scale.
 *
 * The tag byte is used slightly differently for ingress and egress packets.
 * In ingress packets - i.e. packets sent from the DSA conduit port on the host
 * to the CPU port on the switch - the two least significant bits of the
 * tag encode the destination port. The values are interpreted as follows
 *
 *   00b - Destination looked up via DMAC
 *   01b - Packet to be sent on user port 1.
 *   10b - Packet to be sent on user port 2.
 *   11b - Packet to be sent on both user port 1 and user port 2.
 *
 * Additionally, the ingress tag includes 802.1p priorities 0-3 encoded in
 * zero-indexed bits 3:2. These are all left set to 0, indicating priority 0.
 *
 * Egress packets - that is, packets from the KSZ8463 to the host - are simpler.
 * These encode the zero-based port index in bit 0. If said bit is cleared, the
 * packet was received on user port 1. If the bit is set, the packet was
 * received on user port 2.
 *
 * Refer to Section 3.3.4 of the data sheet for more information.
 */

static struct net_if *dsa_tag_ksz8463_recv(struct net_if *iface, struct net_pkt *pkt)
{
	size_t len;
	uint8_t tag;
	enum ethernet_hw_caps caps;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct dsa_switch_context *dsa_switch_ctx = eth_ctx->dsa_switch_ctx;

	caps = net_eth_get_hw_capabilities(iface);
	if (!(caps & ETHERNET_DSA_CONDUIT_PORT)) {
		NET_WARN("Packet received on non-conduit port");
		return iface;
	}

	/* Extract the tag following the payload. Note that the FCS is
	 * not part of the frame here, meaning the tag is the very last
	 * byte in the net_pkt
	 */
	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);
	len = net_pkt_get_len(pkt);
	net_pkt_skip(pkt, len - sizeof(tag));
	net_pkt_read_u8(pkt, &tag);
	net_pkt_update_length(pkt, len - sizeof(tag));

	/* Port index encoded in least significant bit */
	tag &= BIT_MASK(1);

	if (unlikely(tag >= dsa_switch_ctx->num_ports)) {
		NET_ERR("Ingress packet tagged for port %d/%d dropped", (int)tag,
			dsa_switch_ctx->num_ports);
		return iface;
	}

	NET_DBG("Redirecting packet to port %d", (int)tag);
	return dsa_switch_ctx->iface_user[tag];
}

static struct net_pkt *dsa_tag_ksz8463_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	uint8_t *tag;
	struct net_buf *buf;
	struct net_buf_pool *pool;
	const struct dsa_port_config *dsa_cfg;
	const struct device *ptdev = net_if_get_device(iface);

	dsa_cfg = ptdev->config;

	if (unlikely(dsa_cfg->port_idx >= KSZ8463_NUM_USER_PORTS)) {
		NET_ERR("Invalid user port index %d", dsa_cfg->port_idx);
		/* DSA core does not allow NULL to be returned here. Return
		 * the unaltered packet and allow the switch to discard it
		 */
		return pkt;
	}

	pool = net_buf_pool_get(pkt->buffer->pool_id);
	buf = net_buf_alloc_len(pool, sizeof(*tag), K_NO_WAIT);
	if (!buf) {
		NET_WARN("Could not allocate root for tag byte");
		return pkt;
	}

	tag = net_buf_simple_tail(&buf->b);
	*tag = dsa_cfg->port_idx + 1;

	net_buf_add(buf, sizeof(*tag));
	net_buf_frag_add(pkt->buffer, buf);

	NET_DBG("Packet tagged for port %d", dsa_cfg->port_idx);
	return pkt;
}

DSA_TAG_REGISTER(DSA_TAG_PROTO_KSZ8463, dsa_tag_ksz8463_recv, dsa_tag_ksz8463_xmit);
