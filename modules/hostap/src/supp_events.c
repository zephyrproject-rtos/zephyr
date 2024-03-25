/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "supp_events.h"

#include "includes.h"
#include "common.h"
#include "common/ieee802_11_defs.h"
#include "wpa_supplicant_i.h"

#include <zephyr/net/wifi_mgmt.h>

/* Re-defines MAC2STR with address of the element */
#define MACADDR2STR(a) &(a)[0], &(a)[1], &(a)[2], &(a)[3], &(a)[4], &(a)[5]

static const struct wpa_supp_event_info {
	const char *event_str;
	enum supplicant_event_num event;
} wpa_supp_event_info[] = {
	{ "CTRL-EVENT-CONNECTED", SUPPLICANT_EVENT_CONNECTED },
	{ "CTRL-EVENT-DISCONNECTED", SUPPLICANT_EVENT_DISCONNECTED },
	{ "CTRL-EVENT-ASSOC-REJECT", SUPPLICANT_EVENT_ASSOC_REJECT },
	{ "CTRL-EVENT-AUTH-REJECT", SUPPLICANT_EVENT_AUTH_REJECT },
	{ "CTRL-EVENT-SSID-TEMP-DISABLED", SUPPLICANT_EVENT_SSID_TEMP_DISABLED },
	{ "CTRL-EVENT-SSID-REENABLED", SUPPLICANT_EVENT_SSID_REENABLED },
	{ "CTRL-EVENT-BSS-ADDED", SUPPLICANT_EVENT_BSS_ADDED },
	{ "CTRL-EVENT-BSS-REMOVED", SUPPLICANT_EVENT_BSS_REMOVED },
	{ "CTRL-EVENT-TERMINATING", SUPPLICANT_EVENT_TERMINATING },
	{ "CTRL-EVENT-SCAN-STARTED", SUPPLICANT_EVENT_SCAN_STARTED },
	{ "CTRL-EVENT-SCAN-FAILED", SUPPLICANT_EVENT_SCAN_FAILED },
	{ "CTRL-EVENT-NETWORK-NOT-FOUND", SUPPLICANT_EVENT_NETWORK_NOT_FOUND },
	{ "CTRL-EVENT-NETWORK-ADDED", SUPPLICANT_EVENT_NETWORK_ADDED },
	{ "CTRL-EVENT-NETWORK-REMOVED", SUPPLICANT_EVENT_NETWORK_REMOVED },
	{ "CTRL-EVENT-DSCP-POLICY", SUPPLICANT_EVENT_DSCP_POLICY },
};

static void copy_mac_addr(const unsigned int *src, uint8_t *dst)
{
	int i;

	for (i = 0; i < ETH_ALEN; i++) {
		dst[i] = src[i];
	}
}

static enum wifi_conn_status wpas_to_wifi_mgmt_conn_status(int status)
{
	switch (status) {
	case WLAN_STATUS_SUCCESS:
		return WIFI_STATUS_CONN_SUCCESS;
	case WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT:
		return WIFI_STATUS_CONN_WRONG_PASSWORD;
	/* Handle non-supplicant errors */
	case -ETIMEDOUT:
		return WIFI_STATUS_CONN_TIMEOUT;
	default:
		return WIFI_STATUS_CONN_FAIL;
	}
}

static enum wifi_disconn_reason wpas_to_wifi_mgmt_diconn_status(int status)
{
	switch (status) {
	case WLAN_REASON_DEAUTH_LEAVING:
		return WIFI_REASON_DISCONN_AP_LEAVING;
	case WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY:
		return WIFI_REASON_DISCONN_INACTIVITY;
	case WLAN_REASON_UNSPECIFIED:
	/* fall through */
	default:
		return WIFI_REASON_DISCONN_UNSPECIFIED;
	}
}

static int supplicant_process_status(struct supplicant_int_event_data *event_data,
				     char *supplicant_status)
{
	int ret = 1; /* For cases where parsing is not being done*/
	int i;
	union supplicant_event_data *data;
	unsigned int tmp_mac_addr[ETH_ALEN];
	unsigned int event_prefix_len;
	char *event_no_prefix;
	struct wpa_supp_event_info event_info;

	data = (union supplicant_event_data *)event_data->data;

	for (i = 0; i < ARRAY_SIZE(wpa_supp_event_info); i++) {
		if (strncmp(supplicant_status, wpa_supp_event_info[i].event_str,
		    strlen(wpa_supp_event_info[i].event_str)) == 0) {
			event_info = wpa_supp_event_info[i];
			break;
		}
	}

	if (i >= ARRAY_SIZE(wpa_supp_event_info)) {
		/* This is not a bug but rather implementation gap (intentional or not) */
		wpa_printf(MSG_DEBUG, "Event not supported: %s", supplicant_status);
		return -ENOTSUP;
	}

	/* Skip the event prefix and a space */
	event_prefix_len = strlen(event_info.event_str) + 1;
	event_no_prefix = supplicant_status + event_prefix_len;
	event_data->event = event_info.event;

	switch (event_data->event) {
	case SUPPLICANT_EVENT_CONNECTED:
		ret = sscanf(event_no_prefix, "- Connection to"
			MACSTR, MACADDR2STR(tmp_mac_addr));
		event_data->data_len = sizeof(data->connected);
		copy_mac_addr(tmp_mac_addr, data->connected.bssid);
		break;
	case SUPPLICANT_EVENT_DISCONNECTED:
		ret = sscanf(event_no_prefix,
			MACSTR" reason=%d", MACADDR2STR(tmp_mac_addr),
			&data->disconnected.reason_code);
		event_data->data_len = sizeof(data->disconnected);
		copy_mac_addr(tmp_mac_addr, data->disconnected.bssid);
		break;
	case SUPPLICANT_EVENT_ASSOC_REJECT:
		/* TODO */
		break;
	case SUPPLICANT_EVENT_AUTH_REJECT:
		ret = sscanf(event_no_prefix, MACSTR
			" auth_type=%u auth_transaction=%u status_code=%u",
			MACADDR2STR(tmp_mac_addr),
			&data->auth_reject.auth_type,
			&data->auth_reject.auth_transaction,
			&data->auth_reject.status_code);
		event_data->data_len = sizeof(data->auth_reject);
		copy_mac_addr(tmp_mac_addr, data->auth_reject.bssid);
		break;
	case SUPPLICANT_EVENT_SSID_TEMP_DISABLED:
		ret = sscanf(event_no_prefix,
			"id=%d ssid=%s auth_failures=%u duration=%d reason=%s",
			&data->temp_disabled.id, data->temp_disabled.ssid,
			&data->temp_disabled.auth_failures,
			&data->temp_disabled.duration,
			data->temp_disabled.reason_code);
		event_data->data_len = sizeof(data->temp_disabled);
		break;
	case SUPPLICANT_EVENT_SSID_REENABLED:
		ret = sscanf(event_no_prefix,
			"id=%d ssid=%s", &data->reenabled.id,
			data->reenabled.ssid);
		event_data->data_len = sizeof(data->reenabled);
		break;
	case SUPPLICANT_EVENT_BSS_ADDED:
		ret = sscanf(event_no_prefix, "%u "MACSTR,
			&data->bss_added.id,
			MACADDR2STR(tmp_mac_addr));
		copy_mac_addr(tmp_mac_addr, data->bss_added.bssid);
		event_data->data_len = sizeof(data->bss_added);
		break;
	case SUPPLICANT_EVENT_BSS_REMOVED:
		ret = sscanf(event_no_prefix,
			"%u "MACSTR,
			&data->bss_removed.id,
			MACADDR2STR(tmp_mac_addr));
		event_data->data_len = sizeof(data->bss_removed);
		copy_mac_addr(tmp_mac_addr, data->bss_removed.bssid);
		break;
	case SUPPLICANT_EVENT_TERMINATING:
	case SUPPLICANT_EVENT_SCAN_STARTED:
	case SUPPLICANT_EVENT_SCAN_FAILED:
	case SUPPLICANT_EVENT_NETWORK_NOT_FOUND:
	case SUPPLICANT_EVENT_NETWORK_ADDED:
	case SUPPLICANT_EVENT_NETWORK_REMOVED:
		strncpy(data->supplicant_event_str, event_info.event_str,
			sizeof(data->supplicant_event_str));
		event_data->data_len = strlen(data->supplicant_event_str) + 1;
	case SUPPLICANT_EVENT_DSCP_POLICY:
		/* TODO */
		break;
	default:
		break;
	}

	if (ret <= 0) {
		wpa_printf(MSG_ERROR, "%s Parse failed: %s",
			   event_info.event_str, strerror(errno));
	}

	return ret;
}

int supplicant_send_wifi_mgmt_conn_event(void *ctx, int status_code)
{
	struct wpa_supplicant *wpa_s = ctx;
	int status = wpas_to_wifi_mgmt_conn_status(status_code);
	enum net_event_wifi_cmd event;

	if (!wpa_s || !wpa_s->current_ssid) {
		return -EINVAL;
	}

	if (wpa_s->current_ssid->mode == WPAS_MODE_AP) {
		event = NET_EVENT_WIFI_CMD_AP_ENABLE_RESULT;
	} else {
		event = NET_EVENT_WIFI_CMD_CONNECT_RESULT;
	}

	return supplicant_send_wifi_mgmt_event(wpa_s->ifname,
					       event,
					       (void *)&status,
					       sizeof(int));
}

int supplicant_send_wifi_mgmt_disc_event(void *ctx, int reason_code)
{
	struct wpa_supplicant *wpa_s = ctx;
	int status = wpas_to_wifi_mgmt_diconn_status(reason_code);
	enum net_event_wifi_cmd event;

	if (!wpa_s || !wpa_s->current_ssid) {
		return -EINVAL;
	}

	if (wpa_s->wpa_state >= WPA_COMPLETED) {
		if (wpa_s->current_ssid->mode == WPAS_MODE_AP) {
			event = NET_EVENT_WIFI_CMD_AP_DISABLE_RESULT;
		} else {
			event = NET_EVENT_WIFI_CMD_DISCONNECT_RESULT;
		}
	} else {
		if (wpa_s->current_ssid->mode == WPAS_MODE_AP) {
			event = NET_EVENT_WIFI_CMD_AP_ENABLE_RESULT;
		} else {
			event = NET_EVENT_WIFI_CMD_CONNECT_RESULT;
		}
	}

	return supplicant_send_wifi_mgmt_event(wpa_s->ifname,
					       event,
					       (void *)&status,
					       sizeof(int));
}

#ifdef CONFIG_AP
int supplicant_send_wifi_mgmt_ap_status(void *ctx,
					enum net_event_wifi_cmd event,
					enum wifi_ap_status ap_status)
{
	struct wpa_supplicant *wpa_s = ctx;
	int status = ap_status;

	return supplicant_send_wifi_mgmt_event(wpa_s->ifname,
					       event,
					       (void *)&status,
					       sizeof(int));
}
#endif /* CONFIG_AP */

int supplicant_send_wifi_mgmt_event(const char *ifname, enum net_event_wifi_cmd event,
				    void *supplicant_status, size_t len)
{
	struct net_if *iface = net_if_get_by_index(net_if_get_by_name(ifname));
	union supplicant_event_data data;
	struct supplicant_int_event_data event_data;

	if (!iface) {
		wpa_printf(MSG_ERROR, "Could not find iface for %s", ifname);
		return -ENODEV;
	}

	switch (event) {
	case NET_EVENT_WIFI_CMD_CONNECT_RESULT:
		wifi_mgmt_raise_connect_result_event(
			iface,
			*(int *)supplicant_status);
		break;
	case NET_EVENT_WIFI_CMD_DISCONNECT_RESULT:
		wifi_mgmt_raise_disconnect_result_event(
			iface,
			*(int *)supplicant_status);
		break;
#ifdef CONFIG_AP
	case NET_EVENT_WIFI_CMD_AP_ENABLE_RESULT:
		wifi_mgmt_raise_ap_enable_result_event(iface,
						       *(int *)supplicant_status);
		break;
	case NET_EVENT_WIFI_CMD_AP_DISABLE_RESULT:
		wifi_mgmt_raise_ap_disable_result_event(iface,
							*(int *)supplicant_status);
		break;
#endif /* CONFIG_AP */
	case NET_EVENT_SUPPLICANT_CMD_INT_EVENT:
		event_data.data = &data;
		if (supplicant_process_status(&event_data, (char *)supplicant_status) > 0) {
			net_mgmt_event_notify_with_info(NET_EVENT_SUPPLICANT_INT_EVENT,
						iface, &event_data, sizeof(event_data));
		}
		break;
	default:
		wpa_printf(MSG_ERROR, "Unsupported event %d", event);
		return -EINVAL;
	}

	return 0;
}

int supplicant_generate_state_event(const char *ifname,
				    enum net_event_supplicant_cmd event,
				    int status)
{
	struct net_if *iface;

	iface = net_if_get_by_index(net_if_get_by_name(ifname));
	if (!iface) {
		wpa_printf(MSG_ERROR, "Could not find iface for %s", ifname);
		return -ENODEV;
	}

	switch (event) {
	case NET_EVENT_SUPPLICANT_CMD_READY:
		net_mgmt_event_notify(NET_EVENT_SUPPLICANT_READY, iface);
		break;
	case NET_EVENT_SUPPLICANT_CMD_NOT_READY:
		net_mgmt_event_notify(NET_EVENT_SUPPLICANT_NOT_READY, iface);
		break;
	case NET_EVENT_SUPPLICANT_CMD_IFACE_ADDED:
		net_mgmt_event_notify(NET_EVENT_SUPPLICANT_IFACE_ADDED, iface);
		break;
	case NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVING:
		net_mgmt_event_notify(NET_EVENT_SUPPLICANT_IFACE_REMOVING, iface);
		break;
	case NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED:
		net_mgmt_event_notify_with_info(NET_EVENT_SUPPLICANT_IFACE_REMOVED,
						iface, &status, sizeof(status));
		break;
	default:
		wpa_printf(MSG_ERROR, "Unsupported event %d", event);
		return -EINVAL;
	}

	return 0;
}
