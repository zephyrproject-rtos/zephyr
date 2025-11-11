/*
 * Copyright (c) 2021 BayLibre SAS
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BRIDGE_H
#define __BRIDGE_H

static inline bool net_eth_iface_is_bridged(struct ethernet_context *ctx)
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE)
	struct eth_bridge_iface_context *br_ctx;

	if (ctx->bridge == NULL) {
		return false;
	}

	br_ctx = net_if_get_device(ctx->bridge)->data;
	if (br_ctx->is_setup) {
		return true;
	}

	return false;
#else
	return false;
#endif
}

static inline struct net_if *net_eth_get_bridge(struct ethernet_context *ctx)
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE)
	return ctx->bridge;
#else
	return NULL;
#endif
}

#endif /* __BRIDGE_H */
