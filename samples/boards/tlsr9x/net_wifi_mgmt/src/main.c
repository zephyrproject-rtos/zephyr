/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>

#ifdef CONFIG_NET_SOCKETS_POSIX_NAMES
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sock_app, LOG_LEVEL_INF);

#define NET_EVENT_WIFI_MASK (NET_EVENT_WIFI_SCAN_RESULT |   \
				NET_EVENT_WIFI_SCAN_DONE |                  \
				NET_EVENT_WIFI_CONNECT_RESULT |             \
				NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback mgmt_cb;
static bool connected;
static volatile uint32_t last_net_event;
K_SEM_DEFINE(event_sem, 0, 1);

static void net_event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT: {
		const struct wifi_scan_result *scan_result =
			(const struct wifi_scan_result *)cb->info;
		char ssid[WIFI_SSID_MAX_LEN + 1];

		if (!scan_result) {
			return;
		}

		memcpy(ssid, scan_result->ssid, scan_result->ssid_length);
		ssid[scan_result->ssid_length] = '\0';
		LOG_INF("Network scan device (channel = %d): ssid = %s, rssi = %d,"
			"security = %d, mac = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x",
			scan_result->channel, ssid, scan_result->rssi, scan_result->security,
			scan_result->mac[0], scan_result->mac[1], scan_result->mac[2],
			scan_result->mac[3], scan_result->mac[4], scan_result->mac[5]);
		break;
	}
	case NET_EVENT_WIFI_SCAN_DONE:
		LOG_INF("Network scan done");
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		LOG_INF("Network connected");

		connected = true;
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		if (connected == false) {
			LOG_INF("Waiting network to be connected");
		} else {
			LOG_INF("Network disconnected");
			connected = false;
		}
		break;
	default:
		break;
	}

	last_net_event = mgmt_event;
	k_sem_give(&event_sem);
}

void waiting_net_event(uint32_t event)
{
	last_net_event = 0;

	while (event != last_net_event) {
		k_sem_take(&event_sem, K_FOREVER);
	}
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	LOG_INF("app started");

	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);

	/* Scanning part */
	LOG_INF("Start scanning...");

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		LOG_ERR("Scan request failed\n");
		return -ENOEXEC;
	}

	/* Wait for the scanning done */
	waiting_net_event(NET_EVENT_WIFI_SCAN_DONE);

	/* Connection part */
	LOG_INF("Start wifi connecting...");

	static struct wifi_connect_req_params connect_req_params = {0};

	connect_req_params.ssid = CONFIG_SAMPLE_CONNECT_PARAM_WIFI_SSID;
	connect_req_params.ssid_length = strlen(CONFIG_SAMPLE_CONNECT_PARAM_WIFI_SSID);
	connect_req_params.security = WIFI_SECURITY_TYPE_PSK;
	connect_req_params.psk = CONFIG_SAMPLE_CONNECT_PARAM_WIFI_PSK;
	connect_req_params.psk_length = strlen(CONFIG_SAMPLE_CONNECT_PARAM_WIFI_PSK);

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &connect_req_params,
			sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Connection request failed\n");
		return -ENOEXEC;
	}

	/* Wait for the connection result */
	waiting_net_event(NET_EVENT_WIFI_CONNECT_RESULT);

	/* Part of enable the access point */
	LOG_INF("Access point enabling...");

	static struct wifi_connect_req_params ap_enable_req_params = {0};

	ap_enable_req_params.ssid = CONFIG_SAMPLE_AP_ENABLE_PARAM_WIFI_SSID;
	ap_enable_req_params.ssid_length = strlen(CONFIG_SAMPLE_AP_ENABLE_PARAM_WIFI_SSID);
	ap_enable_req_params.channel = CONFIG_SAMPLE_AP_ENABLE_PARAM_CHANNEL;
	ap_enable_req_params.security = WIFI_SECURITY_TYPE_PSK;
	ap_enable_req_params.psk = CONFIG_SAMPLE_AP_ENABLE_PARAM_WIFI_PSK;
	ap_enable_req_params.psk_length = strlen(CONFIG_SAMPLE_AP_ENABLE_PARAM_WIFI_PSK);

	if (net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &ap_enable_req_params,
			sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Disable access point failed\n");
		return -ENOEXEC;
	}

	/* Time of the access point availability */
	k_sleep(K_MSEC(30000));

	/* Part of disable the access point */
	LOG_INF("Access point disabling...");

	if (net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0)) {
		LOG_ERR("Disable access point failed\n");
		return -ENOEXEC;
	}

	/* Connection part */
	LOG_INF("Start wifi disconnecting...");

	if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0)) {
		LOG_ERR("Disconnection request failed\n");
		return -ENOEXEC;
	}

	/* Wait for the connection result */
	waiting_net_event(NET_EVENT_WIFI_DISCONNECT_RESULT);

	LOG_INF("app finished");

	return 0;
}
