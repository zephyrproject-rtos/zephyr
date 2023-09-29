/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_wifi_ap, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"

struct wifi_data {
	struct net_mgmt_event_callback wifi_mgmt_cb;
};

static struct wifi_data wifi_ctx;
static uint8_t wifi_ssid[] = CONFIG_WIFI_SSID;
static size_t wifi_ssid_len = sizeof(CONFIG_WIFI_SSID) - 1;
static uint8_t wifi_psk[] = CONFIG_WIFI_PSK;
static size_t wifi_psk_len = sizeof(CONFIG_WIFI_PSK) - 1;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	struct wifi_data *wifi_cb = CONTAINER_OF(cb, struct wifi_data, wifi_mgmt_cb);
	const struct wifi_status *status = cb->info;

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		LOG_INF("Station connected");
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("Station disconnected");
		break;
	default:
		break;
	}
}

void wifi_init_ap(void)
{
	struct net_if *iface = net_if_get_default();
	int err;

	net_mgmt_init_event_callback(
		&wifi_ctx.wifi_mgmt_cb, wifi_mgmt_event_handler,
		(NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT));
	net_mgmt_add_event_callback(&wifi_ctx.wifi_mgmt_cb);

	struct wifi_connect_req_params params = {
		.ssid = wifi_ssid,
		.ssid_length = wifi_ssid_len,
		.channel = WIFI_CHANNEL_ANY,
#if defined(CONFIG_STA_AUTH_MODE_WPA2)
		.psk = wifi_psk,
		.psk_length = wifi_psk_len,
		.security = WIFI_SECURITY_TYPE_PSK,
#else
		.security = WIFI_SECURITY_TYPE_NONE,
#endif
	};

	LOG_INF("Enabling Wi-Fi AP %s with auth mode: %s", params.ssid,
		wifi_security_txt(params.security));

	err = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params, sizeof(params));
	if (err) {
		LOG_ERR("AP mode enable failed: %s\n", strerror(-err));
		return;
	}
}

int main(void)
{
	wifi_init_ap();

	return 0;
}
