/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SUPP_EVENTS_H__
#define __SUPP_EVENTS_H__

#include <zephyr/net/wifi_mgmt.h>

/* Connectivity Events */
#define _NET_MGMT_SUPPLICANT_LAYER NET_MGMT_LAYER_L3
#define _NET_MGMT_SUPPLICANT_CODE 0x157
#define _NET_MGMT_SUPPLICANT_BASE (NET_MGMT_LAYER(_NET_MGMT_SUPPLICANT_LAYER) | \
				   NET_MGMT_LAYER_CODE(_NET_MGMT_SUPPLICANT_CODE) | \
				   NET_MGMT_IFACE_BIT)
#define _NET_MGMT_SUPPLICANT_EVENT (NET_MGMT_EVENT_BIT | _NET_MGMT_SUPPLICANT_BASE)

enum net_event_supplicant_cmd {
	NET_EVENT_SUPPLICANT_CMD_READY = 1,
	NET_EVENT_SUPPLICANT_CMD_NOT_READY,
	NET_EVENT_SUPPLICANT_CMD_IFACE_ADDED,
	NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVING,
	NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED,
	NET_EVENT_SUPPLICANT_CMD_INT_EVENT,
	NET_EVENT_WIFI_CMD_MAX
};

#define NET_EVENT_SUPPLICANT_READY					\
	(_NET_MGMT_SUPPLICANT_EVENT | NET_EVENT_SUPPLICANT_CMD_READY)

#define NET_EVENT_SUPPLICANT_NOT_READY					\
	(_NET_MGMT_SUPPLICANT_EVENT | NET_EVENT_SUPPLICANT_CMD_NOT_READY)

#define NET_EVENT_SUPPLICANT_IFACE_ADDED					\
	(_NET_MGMT_SUPPLICANT_EVENT | NET_EVENT_SUPPLICANT_CMD_IFACE_ADDED)

#define NET_EVENT_SUPPLICANT_IFACE_REMOVED					\
	(_NET_MGMT_SUPPLICANT_EVENT | NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED)

#define NET_EVENT_SUPPLICANT_IFACE_REMOVING					\
	(_NET_MGMT_SUPPLICANT_EVENT | NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVING)

#define NET_EVENT_SUPPLICANT_INT_EVENT					\
	(_NET_MGMT_SUPPLICANT_EVENT | NET_EVENT_SUPPLICANT_CMD_INT_EVENT)

int supplicant_send_wifi_mgmt_event(const char *ifname,
				    enum net_event_wifi_cmd event,
				    void *status,
				    size_t len);
int supplicant_generate_state_event(const char *ifname,
				    enum net_event_supplicant_cmd event,
				    int status);

#define REASON_CODE_LEN 18
#define NM_WIFI_EVENT_STR_LEN 64
#define ETH_ALEN 6

union supplicant_event_data {
	struct supplicant_event_auth_reject {
		int auth_type;
		int auth_transaction;
		int status_code;
		uint8_t bssid[ETH_ALEN];
	} auth_reject;

	struct supplicant_event_connected {
		uint8_t bssid[ETH_ALEN];
		char ssid[WIFI_SSID_MAX_LEN];
		int id;
	} connected;

	struct supplicant_event_disconnected {
		uint8_t bssid[ETH_ALEN];
		int reason_code;
		int locally_generated;
	} disconnected;

	struct supplicant_event_assoc_reject {
		int status_code;
		int reason_code;
	} assoc_reject;

	struct supplicant_event_temp_disabled {
		int id;
		char ssid[WIFI_SSID_MAX_LEN];
		unsigned int auth_failures;
		unsigned int duration;
		char reason_code[REASON_CODE_LEN];
	} temp_disabled;

	struct supplicant_event_reenabled {
		int id;
		char ssid[WIFI_SSID_MAX_LEN];
	} reenabled;

	struct supplicant_event_bss_added {
		unsigned int id;
		uint8_t bssid[ETH_ALEN];
	} bss_added;

	struct supplicant_event_bss_removed {
		unsigned int id;
		uint8_t bssid[ETH_ALEN];
	} bss_removed;

	struct supplicant_event_network_added {
		unsigned int id;
	} network_added;

	struct supplicant_event_network_removed {
		unsigned int id;
	} network_removed;

	char supplicant_event_str[NM_WIFI_EVENT_STR_LEN];
};

enum supplicant_event_num {
	SUPPLICANT_EVENT_CONNECTED,
	SUPPLICANT_EVENT_DISCONNECTED,
	SUPPLICANT_EVENT_ASSOC_REJECT,
	SUPPLICANT_EVENT_AUTH_REJECT,
	SUPPLICANT_EVENT_TERMINATING,
	SUPPLICANT_EVENT_SSID_TEMP_DISABLED,
	SUPPLICANT_EVENT_SSID_REENABLED,
	SUPPLICANT_EVENT_SCAN_STARTED,
	SUPPLICANT_EVENT_SCAN_RESULTS,
	SUPPLICANT_EVENT_SCAN_FAILED,
	SUPPLICANT_EVENT_BSS_ADDED,
	SUPPLICANT_EVENT_BSS_REMOVED,
	SUPPLICANT_EVENT_NETWORK_NOT_FOUND,
	SUPPLICANT_EVENT_NETWORK_ADDED,
	SUPPLICANT_EVENT_NETWORK_REMOVED,
	SUPPLICANT_EVENT_DSCP_POLICY,
};

struct supplicant_int_event_data {
	enum supplicant_event_num event;
	void *data;
	size_t data_len;
};

#endif /* __SUPP_EVENTS_H__ */
