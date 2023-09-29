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
LOG_MODULE_REGISTER(esp_wifi_sta, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"

struct wifi_data {
	struct net_mgmt_event_callback wifi_mgmt_cb;
	struct net_mgmt_event_callback net_mgmt_cb;
	bool connected;
	bool disconnect_requested;
	bool connect_result;
};

static struct wifi_data wifi_ctx;
static uint8_t wifi_ssid[] = CONFIG_WIFI_SSID;
static size_t wifi_ssid_len = sizeof(CONFIG_WIFI_SSID) - 1;
static uint8_t wifi_psk[] = CONFIG_WIFI_PSK;
static size_t wifi_psk_len = sizeof(CONFIG_WIFI_PSK) - 1;

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;
	if (wifi_ctx.connected) {
		return;
	}

	if (status->status) {
		LOG_ERR("Connection failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
		wifi_ctx.connected = true;
	}
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (!wifi_ctx.connected) {
		return;
	}

	if (wifi_ctx.disconnect_requested) {
		LOG_INF("Disconnection request %s (%d)",
			 status->status ? "failed" : "done",
					status->status);
		wifi_ctx.disconnect_requested = false;
	} else {
		LOG_INF("Received Disconnected");
		wifi_ctx.connected = false;
	}
}

static void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

	LOG_INF("DHCP IP address: %s", dhcp_info);
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}
	wifi_ctx.connect_result = true;
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		print_dhcp_ip(cb);
		break;
	default:
		break;
	}
}

void wifi_init(void)
{
	struct net_if *iface = net_if_get_default();

	wifi_ctx.connected = false;

	net_mgmt_init_event_callback(
		&wifi_ctx.wifi_mgmt_cb, wifi_mgmt_event_handler,
		(NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT));
	net_mgmt_add_event_callback(&wifi_ctx.wifi_mgmt_cb);


	net_mgmt_init_event_callback(&wifi_ctx.net_mgmt_cb,
				     net_mgmt_event_handler,
				     NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&wifi_ctx.net_mgmt_cb);
}

void wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();
	int err;

	wifi_ctx.connect_result = false;

	struct wifi_connect_req_params params = {
		.ssid = wifi_ssid,
		.ssid_length = wifi_ssid_len,
		.channel = WIFI_CHANNEL_ANY,
#if defined(CONFIG_STA_AUTH_MODE_WPA2)
		.psk = wifi_psk,
		.psk_length = wifi_psk_len,
		.security = WIFI_SECURITY_TYPE_PSK,
#elif defined(CONFIG_STA_AUTH_MODE_WPA3)
		.sae_password = wifi_psk,
		.sae_password_length = wifi_psk_len,
		.security = WIFI_SECURITY_TYPE_SAE,
#else
		.security = WIFI_SECURITY_TYPE_NONE,
#endif
	};

	LOG_INF("Connecting to Wi-Fi network %s with auth mode: %d", params.ssid, params.security);

	err = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
	if (err) {
		LOG_ERR("Failed to request Wi-Fi connect: %d", err);
		return;
	}
}

int main(void)
{
	wifi_init();

	wifi_connect();

	k_sleep(K_FOREVER);
}
