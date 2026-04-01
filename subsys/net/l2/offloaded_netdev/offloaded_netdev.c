/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/offloaded_netdev.h>

static inline int offloaded_netdev_if_enable(struct net_if *iface, bool state)
{
	const struct offloaded_if_api *off_if = net_if_get_device(iface)->api;

	if (!off_if || !(off_if->enable)) {
		return 0;
	}

	return off_if->enable(iface, state);
}

static int offloaded_netdev_alloc(struct net_if *iface, struct net_pkt *pkt,
				  size_t size, enum net_ip_protocol proto,
				  k_timeout_t timeout)
{
	const struct offloaded_if_api *off_if = net_if_get_device(iface)->api;

	if (!off_if || !off_if->alloc) {
		return -ENOTSUP;
	}

	return off_if->alloc(iface, pkt, size, proto, timeout);
}

NET_L2_INIT(OFFLOADED_NETDEV, NULL, NULL, offloaded_netdev_if_enable, NULL,
	    offloaded_netdev_alloc);
