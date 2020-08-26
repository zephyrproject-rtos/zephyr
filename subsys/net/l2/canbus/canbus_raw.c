/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_l2_canbus, LOG_LEVEL_NONE);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/socket_can.h>

static inline enum net_verdict canbus_recv(struct net_if *iface,
					   struct net_pkt *pkt)
{
	net_pkt_lladdr_src(pkt)->addr = NULL;
	net_pkt_lladdr_src(pkt)->len = 0U;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_CANBUS_RAW;
	net_pkt_lladdr_dst(pkt)->addr = NULL;
	net_pkt_lladdr_dst(pkt)->len = 0U;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_CANBUS_RAW;

	net_pkt_set_family(pkt, AF_CAN);

	return NET_CONTINUE;
}

static inline int canbus_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct canbus_api *api = net_if_get_device(iface)->api;
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

NET_L2_INIT(CANBUS_RAW_L2, canbus_recv, canbus_send, NULL, NULL);
