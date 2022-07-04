/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr.h>
#include <zephyr/net/ptp.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/ptp_clock.h>

void main(void)
{
	struct net_if *iface;
	int ret;
	struct net_ptp_time init_time;
	const struct device *clk;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (iface == NULL) {
		LOG_ERR("Network interface for PTP not found.");
		return;
	}

	clk = net_eth_get_ptp_clock(iface);
	if (!device_is_ready(clk)) {
		LOG_ERR("Network interface for PTP does not support PTP clock.");
		return;
	}

	init_time.second = 1000;
	init_time.nanosecond = 0;
	ret = ptp_clock_set(clk, &init_time);
	if (ret < 0) {
		LOG_ERR("ptp_clock_set() failed: %d", ret);
		return;
	}

	ret = ptp_master_start(iface);
	if (ret < 0) {
		LOG_ERR("ptp_master_start() failed: %d", ret);
		return;
	}

	LOG_INF("PTP master is running.");
}
