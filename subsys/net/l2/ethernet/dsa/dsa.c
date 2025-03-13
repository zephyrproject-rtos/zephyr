/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_core, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa.h>

#include "tag.h"

#if !defined(CONFIG_NET_DSA_LEGACY)
int dsa_register_recv_callback(struct net_if *iface, dsa_net_recv_cb_t cb)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (ctx->dsa_port != DSA_SLAVE_PORT) {
		return -ENOENT;
	}

	if (cb) {
		ctx = net_if_l2_data(iface);
		ctx->dsa_recv_cb = cb;
	}

	return 0;
}
#endif

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
	 * The callback shall be only present (and used) for slave port, but
	 * not for the master port, which shall support all other
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
	struct net_if *iface_master = dsa_ctx->iface_master;
	const struct device *dev_master = net_if_get_device(iface_master);
	const struct ethernet_api *api = dev_master->api;

	/* Transmit from master port, needing tag protocol to handle pkt first */
	return api->send(dev_master, dsa_tag_xmit(iface, pkt));
}
