/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "supp_events.h"

#include "includes.h"
#include "common.h"

static const char * const supplicant_event_map[] = {
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

static int supplicant_process_status(struct supplicant_int_event_data *event_data,
				     char *supplicant_status)
{
	int ret = 1; /* For cases where parsing is not being done*/
	int event = -1;
	int i;
	union supplicant_event_data *data;

	data = (union supplicant_event_data *)event_data->data;

	for (i = 0; i < ARRAY_SIZE(supplicant_event_map); i++) {
		if (strncmp(supplicant_status, supplicant_event_map[i],
		    strlen(supplicant_event_map[i])) == 0) {
			event = i;
			break;
		}
	}

	if (i >= ARRAY_SIZE(supplicant_event_map)) {
		wpa_printf(MSG_ERROR, "Event not supported: %s\n", supplicant_status);
		return -ENOTSUP;
	}

	event_data->event = event;

	switch (event_data->event) {
	case SUPPLICANT_EVENT_CONNECTED:
		ret = sscanf(supplicant_status +
			     sizeof("CTRL-EVENT-CONNECTED - Connection to") - 1,
			     MACSTR, MAC2STR(data->connected.bssid));
		event_data->data_len = sizeof(data->connected);
		break;
	case SUPPLICANT_EVENT_DISCONNECTED:
		ret = sscanf(supplicant_status + sizeof("CTRL-EVENT-DISCONNECTED bssid=") - 1,
			     MACSTR" reason=%d", MAC2STR(data->disconnected.bssid),
			     &data->disconnected.reason_code);
		event_data->data_len = sizeof(data->disconnected);
		break;
	case SUPPLICANT_EVENT_ASSOC_REJECT:
		/* TODO */
		break;
	case SUPPLICANT_EVENT_AUTH_REJECT:
		ret = sscanf(supplicant_status + sizeof("CTRL-EVENT-AUTH-REJECT ") - 1,
			     MACSTR
			     " auth_type=%u auth_transaction=%u status_code=%u",
			     MAC2STR(data->auth_reject.bssid),
			     &data->auth_reject.auth_type,
			     &data->auth_reject.auth_transaction,
			     &data->auth_reject.status_code);
		event_data->data_len = sizeof(data->auth_reject);
		break;
	case SUPPLICANT_EVENT_SSID_TEMP_DISABLED:
		ret = sscanf(supplicant_status + sizeof("CTRL-EVENT-SSID-TEMP-DISABLED ") - 1,
			     "id=%d ssid=%s auth_failures=%u duration=%d reason=%s",
			     &data->temp_disabled.id, data->temp_disabled.ssid,
			     &data->temp_disabled.auth_failures,
			     &data->temp_disabled.duration,
			     data->temp_disabled.reason_code);
		event_data->data_len = sizeof(data->temp_disabled);
		break;
	case SUPPLICANT_EVENT_SSID_REENABLED:
		ret = sscanf(supplicant_status + sizeof("CTRL-EVENT-SSID-REENABLED ") - 1,
			     "id=%d ssid=%s", &data->reenabled.id,
			     data->reenabled.ssid);
		event_data->data_len = sizeof(data->reenabled);
		break;
	case SUPPLICANT_EVENT_BSS_ADDED:
		mac = data->bss_added.bssid;
		ret = sscanf(supplicant_status + sizeof("CTRL-EVENT-BSS-ADDED ") - 1,
			     "%u "MACSTR,
			     &data->bss_added.id,
			     MAC2STR(data->bss_added.bssid));
		event_data->data_len = sizeof(data->bss_added);
		break;
	case SUPPLICANT_EVENT_BSS_REMOVED:
		mac = data->bss_removed.bssid;
		ret = sscanf(supplicant_status + sizeof("CTRL-EVENT-BSS-REMOVED ") - 1,
			     "%u "MACSTR,
			     &data->bss_removed.id,
			     MAC2STR(data->bss_removed.bssid));
		event_data->data_len = sizeof(data->bss_removed);
		break;
	case SUPPLICANT_EVENT_TERMINATING:
	case SUPPLICANT_EVENT_SCAN_STARTED:
	case SUPPLICANT_EVENT_SCAN_FAILED:
	case SUPPLICANT_EVENT_NETWORK_NOT_FOUND:
	case SUPPLICANT_EVENT_NETWORK_ADDED:
	case SUPPLICANT_EVENT_NETWORK_REMOVED:
		strncpy(data->supplicant_event_str, supplicant_event_map[event],
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
			   supplicant_event_map[event_data->event], strerror(errno));
	}

	return ret;
}

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
		wifi_mgmt_raise_connect_result_event(iface, *(int *)supplicant_status);
		break;
	case NET_EVENT_WIFI_CMD_DISCONNECT_RESULT:
		wifi_mgmt_raise_disconnect_result_event(iface, *(int *)supplicant_status);
		break;
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
