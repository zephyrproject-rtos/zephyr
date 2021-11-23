/*
 * Copyright (c) 2021 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>

#include "ptp_slave.h"

void main(void)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	int ret = ptp_slave_init(iface);

	if (ret < 0) {
		LOG_ERR("ptp_slave_init() failed: %d", ret);
		return;
	}

	LOG_INF("PTP slave is running.");
}
