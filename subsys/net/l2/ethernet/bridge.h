/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BRIDGE_H
#define __BRIDGE_H

enum net_verdict net_eth_bridge_input(struct ethernet_context *ctx,
				      struct net_pkt *pkt);

static inline bool net_eth_iface_is_bridged(struct ethernet_context *ctx)
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE)
	return ctx->bridge.instance != NULL;
#else
	return false;
#endif
}

#endif /* __BRIDGE_H */
