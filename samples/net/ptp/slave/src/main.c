/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>

#include "ptp_slave.h"

void main(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	
	ret = ptp_slave_init(iface, 0);
	if (ret < 0) {
		LOG_ERR("ptp_slave_init() failed: %d", ret);
		return;
	}

	ret = ptp_slave_start();
	if (ret < 0) {
		LOG_ERR("ptp_slave_start() failed: %d", ret);
		return;
	}

	LOG_INF("PTP slave is running.");
}
