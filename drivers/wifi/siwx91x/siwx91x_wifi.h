/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_H
#define SIWX91X_WIFI_H
#include <stdint.h>
#include <zephyr/net/wifi_mgmt.h>

#include "siwx91x_nwp.h"
#include "device/silabs/si91x/wireless/inc/sl_si91x_protocol_types.h"

struct siwx91x_wifi_config {
	const struct device *nwp_dev;
	uint8_t scan_tx_power;
	uint8_t join_tx_power;
};

struct siwx91x_wifi_data {
	struct net_if *iface;
	struct siwx91x_nwp_wifi_cb nwp_ops;
	scan_result_cb_t zephyr_scan_cb;

	bool ps_enabled;
	unsigned int ps_timeout_ms;
	unsigned short ps_listen_interval;
	enum wifi_ps_exit_strategy ps_exit_strategy;
	enum wifi_ps_wakeup_mode ps_wakeup_mode;
};

#endif
