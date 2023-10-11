/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_wifi_test, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t wifi_test_ssid[] = CONFIG_WIFI_TEST_SSID;
static size_t wifi_test_ssid_len = sizeof(CONFIG_WIFI_TEST_SSID) - 1;
static uint8_t wifi_test_psk[] = CONFIG_WIFI_TEST_PSK;
static size_t wifi_test_psk_len = sizeof(CONFIG_WIFI_TEST_PSK) - 1;
static struct in_addr offload_recv_addr_4 = { { { 192, 168, 4, 1 } } };

struct test_ctx {
	struct net_mgmt_event_callback wifi_mgmt_cb;
	struct net_mgmt_event_callback net_mgmt_cb;
	struct k_sem scan_done;
	struct k_sem connected;
	struct k_sem disconnected;
	struct k_sem dhcp_bond;
};

static struct test_ctx ctx;

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		LOG_ERR("Connection failed (%d)", status->status);
	} else {
		k_sem_give(&ctx.connected);
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		k_sem_give(&ctx.disconnected);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		k_sem_give(&ctx.scan_done);
		break;
	default:
		break;
	}
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		k_sem_give(&ctx.dhcp_bond);
		break;
	default:
		break;
	}
}

static void *wifi_setup(void)
{
	struct net_if *iface = net_if_get_default();

	k_sem_init(&ctx.scan_done, 0, 1);
	k_sem_init(&ctx.connected, 0, 1);
	k_sem_init(&ctx.dhcp_bond, 0, 1);
	k_sem_init(&ctx.disconnected, 0, 1);

	net_mgmt_init_event_callback(
		&ctx.wifi_mgmt_cb, wifi_mgmt_event_handler,
		(NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT));
	net_mgmt_add_event_callback(&ctx.wifi_mgmt_cb);


	net_mgmt_init_event_callback(&ctx.net_mgmt_cb,
				     net_mgmt_event_handler,
				     NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&ctx.net_mgmt_cb);

	return NULL;
}

static int wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params params = {
		.ssid = wifi_test_ssid,
		.ssid_length = wifi_test_ssid_len,
		.channel = WIFI_CHANNEL_ANY,
#if defined(CONFIG_WIFI_TEST_AUTH_MODE_WPA2)
		.psk = wifi_test_psk,
		.psk_length = wifi_test_psk_len,
		.security = WIFI_SECURITY_TYPE_PSK,
#elif defined(CONFIG_WIFI_TEST_AUTH_MODE_WPA3)
		.sae_password = wifi_test_psk,
		.sae_password_length = wifi_test_psk_len,
		.security = WIFI_SECURITY_TYPE_SAE,
#else
		.security = WIFI_SECURITY_TYPE_NONE,
#endif
	};

	LOG_INF("Connecting to Wi-Fi network %s with auth mode: %d (%s)", params.ssid,
		params.security, params.psk);

	return net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
}

ZTEST(esp_wifi, test_wifi_scan)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
	zassert_equal(ret, 0, "Scan request failed");

	ret = k_sem_take(&ctx.scan_done, K_SECONDS(5));
	zassert_equal(ret, 0, "Scan Timeout");
}

ZTEST(esp_wifi, test_wifi_connect)
{
	int ret;
	struct net_if *iface = net_if_get_default();

	ret = wifi_connect();
	zassert_equal(ret, 0, "Connect request failed");

	ret = k_sem_take(&ctx.connected, K_SECONDS(20));
	zassert_equal(ret, 0, "Connect Timeout");

	ret = k_sem_take(&ctx.dhcp_bond, K_SECONDS(5));
	zassert_equal(ret, 0, "DHCP Timeout");

	ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
	zassert_equal(ret, 0, "Disconnect request failed");

	ret = k_sem_take(&ctx.disconnected, K_SECONDS(5));
	zassert_equal(ret, 0, "Disconnect Timeout");
}

ZTEST_SUITE(esp_wifi, NULL, wifi_setup, NULL, NULL, NULL);
