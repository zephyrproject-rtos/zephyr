/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "supp_events.h"

#include "includes.h"
#include "common.h"

#define MAC_STR_FORMAT "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx"

static char *wpa_supp_event_map[] = {
	"CTRL-EVENT-CONNECTED",
	"CTRL-EVENT-DISCONNECTED",
	"CTRL-EVENT-ASSOC-REJECT",
	"CTRL-EVENT-AUTH-REJECT",
	"CTRL-EVENT-TERMINATING",
	"CTRL-EVENT-SSID-TEMP-DISABLED",
	"CTRL-EVENT-SSID-REENABLED",
	"CTRL-EVENT-SCAN-STARTED",
	"CTRL-EVENT-SCAN-RESULTS",
	"CTRL-EVENT-SCAN-FAILED",
	"CTRL-EVENT-BSS-ADDED",
	"CTRL-EVENT-BSS-REMOVED",
	"CTRL-EVENT-NETWORK-NOT-FOUND",
	"CTRL-EVENT-NETWORK-ADDED",
	"CTRL-EVENT-NETWORK-REMOVED",
	"CTRL-EVENT-DSCP-POLICY",
};

static int wpa_supp_process_status(struct supp_int_event_data *event_data, char *wpa_supp_status)
{
	int ret = 1; /* For cases where parsing is not being done*/
	int event = -1;
	int i;
	unsigned char *mac;
	union supp_event_data *data;

	data = (union supp_event_data *)event_data->data;

	for (i = 0; i < ARRAY_SIZE(wpa_supp_event_map); i++) {
		if (strncmp(wpa_supp_status, wpa_supp_event_map[i],
		    strlen(wpa_supp_event_map[i])) == 0) {
			event = i;
			break;
		}
	}

	if (i >= ARRAY_SIZE(wpa_supp_event_map)) {
		wpa_printf(MSG_ERROR, "Event not supported: %s\n", wpa_supp_status);
		return -ENOTSUP;
	}

	event_data->event = event;

	switch (event_data->event) {
	case WPA_SUPP_EVENT_CONNECTED:
		mac = data->connected.bssid;
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-CONNECTED - Connection to"),
			MAC_STR_FORMAT, &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		event_data->data_len = sizeof(data->connected);
		break;
	case WPA_SUPP_EVENT_DISCONNECTED:
		mac = data->disconnected.bssid;
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-DISCONNECTED bssid="),
			MAC_STR_FORMAT" reason=%d", &mac[0], &mac[1], &mac[2],
			 &mac[3], &mac[4], &mac[5], &data->disconnected.reason_code);
		event_data->data_len = sizeof(data->disconnected);
		break;
	case WPA_SUPP_EVENT_ASSOC_REJECT:
		/* TODO */
		break;
	case WPA_SUPP_EVENT_AUTH_REJECT:
		mac = data->auth_reject.bssid;
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-AUTH-REJECT "), MAC_STR_FORMAT
			" auth_type=%u auth_transaction=%u status_code=%u",
			&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5],
			&data->auth_reject.auth_type,
			&data->auth_reject.auth_transaction,
			&data->auth_reject.status_code);
		event_data->data_len = sizeof(data->auth_reject);
		break;
	case WPA_SUPP_EVENT_SSID_TEMP_DISABLED:
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-SSID-TEMP-DISABLED "),
			"id=%d ssid=%s auth_failures=%u duration=%d reason=%s",
			&data->temp_disabled.id, data->temp_disabled.ssid,
			&data->temp_disabled.auth_failures,
			&data->temp_disabled.duration,
			data->temp_disabled.reason_code);
		event_data->data_len = sizeof(data->temp_disabled);
		break;
	case WPA_SUPP_EVENT_SSID_REENABLED:
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-SSID-REENABLED "),
			"id=%d ssid=%s", &data->reenabled.id,
			data->reenabled.ssid);
		event_data->data_len = sizeof(data->reenabled);
		break;
	case WPA_SUPP_EVENT_BSS_ADDED:
		mac = data->bss_added.bssid;
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-BSS-ADDED "), "%u "MAC_STR_FORMAT,
			&data->bss_added.id,
			&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		event_data->data_len = sizeof(data->bss_added);
		break;
	case WPA_SUPP_EVENT_BSS_REMOVED:
		mac = data->bss_removed.bssid;
		ret = sscanf(wpa_supp_status + strlen("CTRL-EVENT-BSS-REMOVED "),
			"%u "MAC_STR_FORMAT,
			&data->bss_removed.id,
			&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		event_data->data_len = sizeof(data->bss_removed);
		break;
	case WPA_SUPP_EVENT_TERMINATING:
	case WPA_SUPP_EVENT_SCAN_STARTED:
	case WPA_SUPP_EVENT_SCAN_FAILED:
	case WPA_SUPP_EVENT_NETWORK_NOT_FOUND:
	case WPA_SUPP_EVENT_NETWORK_ADDED:
	case WPA_SUPP_EVENT_NETWORK_REMOVED:
		strncpy(data->supp_event_str, wpa_supp_event_map[event],
				 sizeof(data->supp_event_str));
		event_data->data_len = strlen(data->supp_event_str) + 1;
	case WPA_SUPP_EVENT_DSCP_POLICY:
		/* TODO */
		break;
	default:
		break;
	}

	if (ret <= 0) {
		wpa_printf(MSG_ERROR, "%s Parse failed: %s",
			   wpa_supp_event_map[event_data->event], strerror(errno));
	}

	return ret;
}

int send_wifi_mgmt_event(const char *ifname, enum net_event_wifi_cmd event,
		 void *wpa_supp_status, size_t len)
{
	const struct device *dev = device_get_binding(ifname);
	struct net_if *iface = net_if_lookup_by_dev(dev);
	union supp_event_data data;
	struct supp_int_event_data event_data;

	if (!iface) {
		wpa_printf(MSG_ERROR, "Could not find iface for %s", ifname);
		return -ENODEV;
	}

	switch (event) {
	case NET_EVENT_WIFI_CMD_CONNECT_RESULT:
		wifi_mgmt_raise_connect_result_event(iface, *(int *)wpa_supp_status);
		break;
	case NET_EVENT_WIFI_CMD_DISCONNECT_RESULT:
		wifi_mgmt_raise_disconnect_result_event(iface, *(int *)wpa_supp_status);
		break;
	case NET_EVENT_WPA_SUPP_CMD_INT_EVENT:
		event_data.data = &data;
		if (wpa_supp_process_status(&event_data, (char *)wpa_supp_status) > 0) {
			net_mgmt_event_notify_with_info(NET_EVENT_WPA_SUPP_INT_EVENT,
						iface, &event_data, sizeof(event_data));
		}
		break;
	default:
		wpa_printf(MSG_ERROR, "Unsupported event %d", event);
		return -EINVAL;
	}

	return 0;
}

int generate_supp_state_event(const char *ifname, enum net_event_wpa_supp_cmd event, int status)
{
	/* TODO: Replace device_get_binding. */
	const struct device *dev = device_get_binding(ifname);

	if (!dev) {
		wpa_printf(MSG_ERROR, "Could not find device for %s", ifname);
		return -ENODEV;
	}

	struct net_if *iface = net_if_lookup_by_dev(dev);

	if (!iface) {
		wpa_printf(MSG_ERROR, "Could not find iface for %s", ifname);
		return -ENODEV;
	}

	switch (event) {
	case NET_EVENT_WPA_SUPP_CMD_READY:
		net_mgmt_event_notify(NET_EVENT_WPA_SUPP_READY, iface);
		break;
	case NET_EVENT_WPA_SUPP_CMD_NOT_READY:
		net_mgmt_event_notify(NET_EVENT_WPA_SUPP_NOT_READY, iface);
		break;
	case NET_EVENT_WPA_SUPP_CMD_IFACE_ADDED:
		net_mgmt_event_notify(NET_EVENT_WPA_SUPP_IFACE_ADDED, iface);
		break;
	case NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVING:
		net_mgmt_event_notify(NET_EVENT_WPA_SUPP_IFACE_REMOVING, iface);
		break;
	case NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVED:
		net_mgmt_event_notify_with_info(NET_EVENT_WPA_SUPP_IFACE_REMOVED,
						iface, &status, sizeof(status));
		break;
	default:
		wpa_printf(MSG_ERROR, "Unsupported event %d", event);
		return -EINVAL;
	}

	return 0;
}
