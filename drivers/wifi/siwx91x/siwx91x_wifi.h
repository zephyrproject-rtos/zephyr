/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_H
#define SIWX91X_WIFI_H

#include <zephyr/net/wifi_mgmt.h>

#include "sl_si91x_types.h"

struct siwx91x_config {
	uint8_t scan_tx_power;
	uint8_t join_tx_power;
};

struct siwx91x_dev {
	struct net_if *iface;
	sl_mac_address_t macaddr;
	enum wifi_iface_state state;
	enum wifi_iface_state scan_prev_state;
	scan_result_cb_t scan_res_cb;
	uint16_t scan_max_bss_cnt;
	struct wifi_ps_params ps_params;
	uint8_t max_num_sta;
	bool reboot_needed;
	bool hidden_ssid;

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
	struct k_event fds_recv_event;
	sl_si91x_fdset_t fds_watch;
	struct {
		net_context_recv_cb_t cb;
		void *user_data;
		struct net_context *context;
	} fds_cb[SLI_NUMBER_OF_SOCKETS];
#endif
};

int siwx91x_status(const struct device *dev, struct wifi_iface_status *status);
bool siwx91x_param_changed(struct wifi_iface_status *prev_params,
			   struct wifi_connect_req_params *new_params);

#endif
