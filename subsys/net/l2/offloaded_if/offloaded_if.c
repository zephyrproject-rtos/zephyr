/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(offloaded_if, LOG_LEVEL_DBG);

#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/offloaded_if.h>

static inline int offloaded_if_enable(struct net_if *iface, bool state)
{
	const struct device *dev = net_if_get_device(iface);
	const struct offloaded_if_api *off_if = dev->api;

	if (!off_if || !(off_if->enable)) {
		return 0;
	}

	/* Reviewers: Would it be better to pass the iface itself instead? */
	return off_if->enable(dev, state);
}

NET_L2_INIT(OFFLOADED_IF, NULL, NULL, offloaded_if_enable, NULL);
