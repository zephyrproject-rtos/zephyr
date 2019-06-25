/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_LOOPBACK_CAN_H_
#define ZEPHYR_DRIVERS_CAN_LOOPBACK_CAN_H_

#include <drivers/can.h>

#define DEV_DATA(dev) ((struct can_loopback_data *const)(dev)->driver_data)
#define DEV_CFG(dev) \
	((const struct can_loopback_config *const)(dev)->config->config_info)

struct can_loopback_filter {
	can_rx_callback_t rx_cb;
	void *cb_arg;
	struct zcan_filter filter;
};

struct can_loopback_data {
	struct can_loopback_filter filters[CONFIG_CAN_MAX_FILTER];
	struct k_mutex mtx;
	bool loopback;
};

struct can_loopback_config {
};

#endif /*ZEPHYR_DRIVERS_CAN_LOOPBACK_CAN_H_*/
