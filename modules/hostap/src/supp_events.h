/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SUPP_EVENTS_H__
#define __SUPP_EVENTS_H__

#include <zephyr/net/wifi_mgmt.h>

/* Connectivity Events */
#define _NET_MGMT_WPA_SUPP_LAYER NET_MGMT_LAYER_L2
#define _NET_MGMT_WPA_SUPP_CODE 0x157
#define _NET_MGMT_WPA_SUPP_BASE (NET_MGMT_LAYER(_NET_MGMT_WPA_SUPP_LAYER) | \
				NET_MGMT_LAYER_CODE(_NET_MGMT_WPA_SUPP_CODE) | \
				NET_MGMT_IFACE_BIT)
#define _NET_MGMT_WPA_SUPP_EVENT (NET_MGMT_EVENT_BIT | _NET_MGMT_WPA_SUPP_BASE)

enum net_event_wpa_supp_cmd {
	NET_EVENT_WPA_SUPP_CMD_READY = 1,
	NET_EVENT_WPA_SUPP_CMD_NOT_READY,
	NET_EVENT_WPA_SUPP_CMD_IFACE_ADDED,
	NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVING,
	NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVED,
	NET_EVENT_WPA_SUPP_CMD_INT_EVENT,
	NET_EVENT_WPA_SUPP_CMD_MAX
};

#define NET_EVENT_WPA_SUPP_READY					\
	(_NET_MGMT_WPA_SUPP_EVENT | NET_EVENT_WPA_SUPP_CMD_READY)

#define NET_EVENT_WPA_SUPP_NOT_READY					\
	(_NET_MGMT_WPA_SUPP_EVENT | NET_EVENT_WPA_SUPP_CMD_NOT_READY)

#define NET_EVENT_WPA_SUPP_IFACE_ADDED					\
	(_NET_MGMT_WPA_SUPP_EVENT | NET_EVENT_WPA_SUPP_CMD_IFACE_ADDED)

#define NET_EVENT_WPA_SUPP_IFACE_REMOVED					\
	(_NET_MGMT_WPA_SUPP_EVENT | NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVED)

#define NET_EVENT_WPA_SUPP_IFACE_REMOVING					\
	(_NET_MGMT_WPA_SUPP_EVENT | NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVING)

#define NET_EVENT_WPA_SUPP_INT_EVENT					\
	(_NET_MGMT_WPA_SUPP_EVENT | NET_EVENT_WPA_SUPP_CMD_INT_EVENT)

int send_wifi_mgmt_event(const char *ifname, enum net_event_wifi_cmd event, void *status,
			size_t len);
int generate_supp_state_event(const char *ifname, enum net_event_wpa_supp_cmd event, int status);

#define REASON_CODE_LEN 18
#define NM_WIFI_EVENT_STR_LEN 64
#define ETH_ALEN 6

union supp_event_data {
	struct supp_event_auth_reject {
		int auth_type;
		int auth_transaction;
		int status_code;
		uint8_t bssid[ETH_ALEN];
	} auth_reject;

	struct supp_event_connected {
		uint8_t bssid[ETH_ALEN];
		char ssid[WIFI_SSID_MAX_LEN];
		int id;
	} connected;

	struct supp_event_disconnected {
		uint8_t bssid[ETH_ALEN];
		int reason_code;
		int locally_generated;
	} disconnected;

	struct supp_event_assoc_reject {
		int status_code;
		int reason_code;
	} assoc_reject;

	struct supp_event_temp_disabled {
		int id;
		char ssid[WIFI_SSID_MAX_LEN];
		unsigned int auth_failures;
		unsigned int duration;
		char reason_code[REASON_CODE_LEN];
	} temp_disabled;

	struct supp_event_reenabled {
		int id;
		char ssid[WIFI_SSID_MAX_LEN];
	} reenabled;

	struct supp_event_bss_added {
		unsigned int id;
		uint8_t bssid[ETH_ALEN];
	} bss_added;

	struct supp_event_bss_removed {
		unsigned int id;
		uint8_t bssid[ETH_ALEN];
	} bss_removed;

	struct supp_event_network_added {
		unsigned int id;
	} network_added;

	struct supp_event_network_removed {
		unsigned int id;
	} network_removed;

	char supp_event_str[NM_WIFI_EVENT_STR_LEN];
};

enum supp_event_num {
	WPA_SUPP_EVENT_CONNECTED,
	WPA_SUPP_EVENT_DISCONNECTED,
	WPA_SUPP_EVENT_ASSOC_REJECT,
	WPA_SUPP_EVENT_AUTH_REJECT,
	WPA_SUPP_EVENT_TERMINATING,
	WPA_SUPP_EVENT_SSID_TEMP_DISABLED,
	WPA_SUPP_EVENT_SSID_REENABLED,
	WPA_SUPP_EVENT_SCAN_STARTED,
	WPA_SUPP_EVENT_SCAN_RESULTS,
	WPA_SUPP_EVENT_SCAN_FAILED,
	WPA_SUPP_EVENT_BSS_ADDED,
	WPA_SUPP_EVENT_BSS_REMOVED,
	WPA_SUPP_EVENT_NETWORK_NOT_FOUND,
	WPA_SUPP_EVENT_NETWORK_ADDED,
	WPA_SUPP_EVENT_NETWORK_REMOVED,
	WPA_SUPP_EVENT_DSCP_POLICY,
};



struct supp_int_event_data {
	enum supp_event_num event;
	void *data;
	size_t data_len;
};

#endif /* __SUPP_EVENTS_H__ */
