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

typedef uint8_t sl_si91x_wlan_module_state_t;
#define WLAN_REASON_CODES 10

typedef enum {
	STATE_IDLE = 0x00,
	STATE_BEACON_LOSS = 0x10,      /* Failover Roam */
	STATE_DEAUTHENTICATION = 0x20, /* AP induced Roam / Disconnect from supplicant */
	STATE_CURRENT_AP_BEST = 0x50,
	STATE_BETTER_AP_FOUND = 0x60, /* While roaming */
	STATE_NO_AP_FOUND = 0x70,
	STATE_ASSOCIATED = 0x80,
	STATE_UNASSOCIATED = 0x90
} sl_si91x_wlan_module_state_enum_t;

struct siwx91x_bit_to_string {
	uint8_t bit;
	char *string;
};

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
