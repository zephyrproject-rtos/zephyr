/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>

#include "ptp_master.h"

void main(void)
{
	struct net_if *iface;
	int ret;
	struct net_ptp_time init_time;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	ret = ptp_master_init(iface);
	if (ret < 0) {
		LOG_ERR("ptp_master_init() failed: %d", ret);
		return;
	}

	init_time.second = 1000;
	init_time.nanosecond = 0;
	ret = ptp_master_clk_set(&init_time);
	if (ret < 0) {
		LOG_ERR("ptp_master_clk_set() failed: %d", ret);
		return;
	}

	ret = ptp_master_start();
	if (ret < 0) {
		LOG_ERR("ptp_master_start() failed: %d", ret);
		return;
	}

	LOG_INF("PTP master is running.");
}
