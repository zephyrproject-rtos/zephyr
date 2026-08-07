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

NET_L2_INIT(OFFLOADED_NETDEV, NULL, NULL, offloaded_netdev_if_enable, NULL);
