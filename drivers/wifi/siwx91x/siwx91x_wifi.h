/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_H
#define SIWX91X_WIFI_H

#include <zephyr/net/net_context.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/kernel.h>

#include "sl_ieee802_types.h"
#include "sl_si91x_types.h"
#include "sl_si91x_protocol_types.h"

/* BG SCAN configurations related macros */
#define ADV_SCAN_THRESHOLD           -40
#define ADV_RSSI_TOLERANCE_THRESHOLD 5
#define ADV_ACTIVE_SCAN_DURATION     15
#define ADV_PASSIVE_SCAN_DURATION    20
#define ADV_MULTIPROBE               0
#define ADV_SCAN_PERIODICITY         10
#define ENABLE_INSTANT_SCAN          1

/* SCAN Dwell time related macros */
#define SL_WIFI_MIN_ACTIVE_SCAN_TIME_MS  5
#define SL_WIFI_MAX_ACTIVE_SCAN_TIME_MS  1000
#define SL_WIFI_MIN_PASSIVE_SCAN_TIME_MS 10
#define SL_WIFI_MAX_PASSIVE_SCAN_TIME_MS 1000

struct siwx91x_dev {
	struct net_if *iface;
	sl_mac_address_t macaddr;
	enum wifi_iface_state state;
	enum wifi_iface_state scan_prev_state;
	scan_result_cb_t scan_res_cb;
	uint16_t scan_max_bss_cnt;

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
	struct k_event fds_recv_event;
	sl_si91x_fd_set fds_watch;
	struct {
		net_context_recv_cb_t cb;
		void *user_data;
		struct net_context *context;
	} fds_cb[NUMBER_OF_SOCKETS];
#endif
};

#endif
