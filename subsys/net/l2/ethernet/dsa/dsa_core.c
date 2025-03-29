/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_core, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/dsa_core.h>
#include <zephyr/net/ethernet.h>

#include "dsa_tag.h"

int dsa_register_recv_callback(struct net_if *iface, dsa_net_recv_cb_t cb)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (ctx->dsa_port != DSA_USER_PORT) {
		return -ENOENT;
	}

	if (cb) {
		ctx = net_if_l2_data(iface);
		ctx->dsa_recv_cb = cb;
	}

	return 0;
}

struct net_if *dsa_recv(struct net_if *iface, struct net_pkt **pkt)
{
	const struct ethernet_context *eth_ctx;
	struct net_if *iface_dst;

	/* tag protocol handles first to untag and re-direct interface */
	iface_dst = dsa_tag_recv(iface, pkt);
	if (!iface_dst || iface_dst == iface) {
		return iface_dst;
	}

	/*
	 * Optional code to change the destination interface with some
	 * custom callback (to e.g. filter/switch packets based on MAC).
	 *
	 * The callback shall be only present (and used) for user port, but
	 * not for the conduit port, which shall support all other
	 * protocols - i.e. UDP. ICMP, TCP.
	 */
	eth_ctx = net_if_l2_data(iface_dst);
	if (eth_ctx->dsa_recv_cb) {
		if (eth_ctx->dsa_recv_cb(iface_dst, *pkt) == NET_OK) {
			return iface;
		}
	}

	return iface_dst;
}

int dsa_xmit(const struct device *dev, struct net_pkt *pkt)
{
	struct dsa_context *dsa_ctx = dev->data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct net_if *iface_conduit = dsa_ctx->iface_conduit;
	const struct device *dev_conduit = net_if_get_device(iface_conduit);
	const struct ethernet_api *api = dev_conduit->api;

	/* Transmit from conduit port, needing tag protocol to handle pkt first */
	return api->send(dev_conduit, dsa_tag_xmit(iface, pkt));
}
