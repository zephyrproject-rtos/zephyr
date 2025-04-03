/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_core, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>

#include "dsa_tag.h"

struct net_if *dsa_recv(struct net_if *iface, struct net_pkt *pkt)
{
	if (iface == NULL || pkt == NULL) {
		return iface;
	}

	/* Tag protocol handles to untag and re-direct interface */
	return dsa_tag_recv(iface, pkt);
}

int dsa_xmit(const struct device *dev, struct net_pkt *pkt)
{
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct net_if *iface_conduit = dsa_switch_ctx->iface_conduit;
	const struct device *dev_conduit = net_if_get_device(iface_conduit);
	const struct ethernet_api *eth_api_conduit = dev_conduit->api;
	/* Tag protocol handles pkt first */
	struct net_pkt *dsa_pkt = dsa_tag_xmit(iface, pkt);

	/* Transmit from conduit port */
	return eth_api_conduit->send(dev_conduit, dsa_pkt);
}

int dsa_eth_init(struct net_if *iface)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	if (eth_ctx->dsa_port == DSA_CONDUIT_PORT) {
		net_if_flag_clear(iface, NET_IF_IPV4);
		net_if_flag_clear(iface, NET_IF_IPV6);
	}

	return 0;
}
