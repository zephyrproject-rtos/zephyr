/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>

struct net_if *dsa_tag_recv(struct net_if *iface, struct net_pkt *pkt)
{
	const struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	const struct dsa_switch_context *dsa_switch_ctx = eth_ctx->dsa_switch_ctx;

	/* For no tag type, use NULL recv to let host ethernet driver handle. */
	if (dsa_switch_ctx->dapi->recv == NULL) {
		return iface;
	}

	return dsa_switch_ctx->dapi->recv(iface, pkt);
}

struct net_pkt *dsa_tag_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dsa_switch_context *dsa_switch_ctx = dev->data;

	/* For no tag type, use NULL xmit to let host ethernet driver handle. */
	if (dsa_switch_ctx->dapi->xmit == NULL) {
		/* Here utilize iface field to store origin user port iface.
		 * Host ethernet driver needs to handle then.
		 */
		pkt->iface = iface;
		return pkt;
	}

	return dsa_switch_ctx->dapi->xmit(iface, pkt);
}
