/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_l2_dummy, LOG_LEVEL_NONE);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <net/dummy.h>

static inline enum net_verdict dummy_recv(struct net_if *iface,
					  struct net_pkt *pkt)
{
	net_pkt_lladdr_src(pkt)->addr = NULL;
	net_pkt_lladdr_src(pkt)->len = 0U;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_DUMMY;
	net_pkt_lladdr_dst(pkt)->addr = NULL;
	net_pkt_lladdr_dst(pkt)->len = 0U;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_DUMMY;

	return NET_CONTINUE;
}

static inline int dummy_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct dummy_api *api = net_if_get_device(iface)->api;
	int ret;

	if (!api) {
		return -ENOENT;
	}

	ret = api->send(net_if_get_device(iface), pkt);
	if (!ret) {
		ret = net_pkt_get_len(pkt);
		net_pkt_unref(pkt);
	}

	return ret;
}

static enum net_l2_flags dummy_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST;
}

NET_L2_INIT(DUMMY_L2, dummy_recv, dummy_send, NULL, dummy_flags);
