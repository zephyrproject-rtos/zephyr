/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa.h>

struct net_if *dsa_tag_recv(struct net_if *iface, struct net_pkt **pkt)
{
	const struct ethernet_context *eth_ctx;
	const struct dsa_context *dsa_ctx;

	if (pkt == NULL || iface == NULL) {
		return NULL;
	}

	eth_ctx = net_if_l2_data(iface);
	dsa_ctx = eth_ctx->dsa_ctx;

	/* For no tag type, use NULL recv to let host ethernet driver handle. */
	if (!dsa_ctx || !dsa_ctx->dapi || !dsa_ctx->dapi->recv) {
		return iface;
	}

	return dsa_ctx->dapi->recv(iface, pkt);
}

struct net_pkt *dsa_tag_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	const struct device *dev;
	const struct dsa_context *dsa_ctx;

	if (pkt == NULL || iface == NULL) {
		return NULL;
	}

	dev = net_if_get_device(iface);
	dsa_ctx = dev->data;

	/* For no tag type, use NULL xmit to let host ethernet driver handle. */
	if (!dsa_ctx || !dsa_ctx->dapi || !dsa_ctx->dapi->xmit) {
		/* Here utilize iface field to store origin iface.
		 * Host ethernet driver needs to handle then.
		 */
		pkt->iface = iface;
		return pkt;
	}

	return dsa_ctx->dapi->xmit(iface, pkt);
}
