/**
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_wfx200_events, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

#include <sl_wfx.h>
#include "wfx200_internal.h"

const char *wfx200_event_to_str(enum wfx200_event event)
{
	switch (event) {
	case WFX200_CONNECT_EVENT:
		return "WFX200_CONNECT_EVENT";
	case WFX200_CONNECT_FAILED_EVENT:
		return "WFX200_CONNECT_FAILED_EVENT";
	case WFX200_DISCONNECT_EVENT:
		return "WFX200_DISCONNECT_EVENT";
	}
	return "UNKNOWN";
}

static void wfx200_event_handle_connect(struct wfx200_data *data)
{
	if (data->sta_enabled) {
		net_eth_carrier_on(data->iface);
		wifi_mgmt_raise_connect_result_event(data->iface, 0);
	}
}

static void wfx200_event_handle_connect_failed(struct wfx200_data *data)
{
	if (data->sta_enabled) {
		wifi_mgmt_raise_connect_result_event(data->iface, -1);
		data->sta_enabled = false;
	}
}

static void wfx200_event_handle_disconnect(struct wfx200_data *data)
{
	if (data->sta_enabled) {
		net_eth_carrier_off(data->iface);
		wifi_mgmt_raise_disconnect_result_event(data->iface, 0);
		data->sta_enabled = false;
	}
}

void wfx200_event_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct wfx200_data *data = p1;
	struct wfx200_queue_event *event;

	k_thread_name_set(NULL, "wfx200_ev");

	while (true) {
		event = k_queue_get(&data->event_queue, K_FOREVER);
		if (event == NULL) {
			LOG_ERR("Got no event!");
			continue;
		}

		LOG_DBG("Handling event: %s", wfx200_event_to_str(event->ev));

		switch (event->ev) {
		case WFX200_CONNECT_EVENT:
			wfx200_event_handle_connect(data);
			break;
		case WFX200_CONNECT_FAILED_EVENT:
			wfx200_event_handle_connect_failed(data);
			break;
		case WFX200_DISCONNECT_EVENT:
			wfx200_event_handle_disconnect(data);
			break;
		}

		k_heap_free(&data->heap, event);
		event = NULL;
	}
}
