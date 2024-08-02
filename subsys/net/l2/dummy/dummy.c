/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_dummy, LOG_LEVEL_NONE);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/net/dummy.h>

static inline enum net_verdict dummy_recv(struct net_if *iface,
					  struct net_pkt *pkt)
{
	const struct dummy_api *api = net_if_get_device(iface)->api;

	if (api == NULL) {
		return NET_DROP;
	}

	if (api->recv == NULL) {
		return NET_CONTINUE;
	}

	return api->recv(iface, pkt);
}

static inline int dummy_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct dummy_api *api = net_if_get_device(iface)->api;
	int ret;

	if (!api) {
		return -ENOENT;
	}

	ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
	if (!ret) {
		ret = net_pkt_get_len(pkt);
		net_pkt_unref(pkt);
	}

	return ret;
}

static inline int dummy_enable(struct net_if *iface, bool state)
{
	int ret = 0;
	const struct dummy_api *api = net_if_get_device(iface)->api;

	if (!api) {
		return -ENOENT;
	}

	if (!state) {
		if (api->stop) {
			ret = api->stop(net_if_get_device(iface));
		}
	} else {
		if (api->start) {
			ret = api->start(net_if_get_device(iface));
		}
	}

	return ret;
}

static enum net_l2_flags dummy_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST;
}

NET_L2_INIT(DUMMY_L2, dummy_recv, dummy_send, dummy_enable, dummy_flags);
