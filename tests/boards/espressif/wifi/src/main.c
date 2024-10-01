/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/icmp.h>

LOG_MODULE_REGISTER(wifi_test, CONFIG_NET_L2_ETHERNET_LOG_LEVEL);

#include "net_private.h"

K_SEM_DEFINE(wifi_event, 0, 1);

#define WIFI_MGMT_EVENTS                                                                           \
	(NET_EVENT_WIFI_SCAN_DONE | NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_CONNECT_RESULT |   \
	 NET_EVENT_WIFI_DISCONNECT_RESULT)

#define TEST_DATA "ICMP dummy data"

static struct net_if *iface;
static uint32_t scan_result;
static bool connecting;
static int result;
static struct net_mgmt_event_callback wifi_mgmt_cb;

extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len, char *buf, int buflen);

static void wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
	uint8_t ssid_print[WIFI_SSID_MAX_LEN + 1];

	scan_result++;

	if (scan_result == 1U) {
		printk("\n%-4s | %-32s %-5s | %-13s | %-4s | %-15s | %-17s | %-8s\n", "Num", "SSID",
		       "(len)", "Chan (Band)", "RSSI", "Security", "BSSID", "MFP");
	}

	strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
	ssid_print[sizeof(ssid_print) - 1] = '\0';

	printk("%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-15s | %-17s | %-8s\n", scan_result,
	       ssid_print, entry->ssid_length, entry->channel, wifi_band_txt(entry->band),
	       entry->rssi, wifi_security_txt(entry->security),
	       ((entry->mac_length) ? net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN,
							     mac_string_buf, sizeof(mac_string_buf))
				    : ""),
	       wifi_mfp_txt(entry->mfp));
}

static void wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	result = status->status;

	if (result) {
		LOG_INF("Connection request failed (%d)", result);
	} else {
		LOG_INF("Connected");
	}
}

static void wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	result = status->status;

	if (!connecting) {
		if (result) {
			LOG_INF("Disconnect failed (%d)", result);
		} else {
			LOG_INF("Disconnected");
		}
	} else {
		/* Disconnect event while connecting is a failed attempt */
		result = WIFI_STATUS_CONN_FAIL;
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		k_sem_give(&wifi_event);
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		wifi_connect_result(cb);
		k_sem_give(&wifi_event);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		wifi_disconnect_result(cb);
		k_sem_give(&wifi_event);
		break;
	default:
		break;
	}
}

static int icmp_event(struct net_icmp_ctx *ctx, struct net_pkt *pkt, struct net_icmp_ip_hdr *hdr,
		      struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	struct net_ipv4_hdr *ip_hdr = hdr->ipv4;

	LOG_INF("Received ICMP reply from %s", net_sprint_ipv4_addr(&ip_hdr->src));

	k_sem_give(&wifi_event);

	return 0;
}

static int wifi_scan(void)
{
	int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);

	if (ret) {
		LOG_INF("Scan request failed with error: %d", ret);
		return ret;
	}

	LOG_INF("Wifi scan requested...");

	return 0;
}

static int wifi_connect(void)
{
	struct wifi_connect_req_params params = {0};
	int ret;

	/* Defaults */
	params.band = WIFI_FREQ_BAND_UNKNOWN;
	params.channel = WIFI_CHANNEL_ANY;
	params.mfp = WIFI_MFP_OPTIONAL;

	/* Input parameters */
	params.ssid = CONFIG_WIFI_TEST_SSID;
	params.ssid_length = strlen(params.ssid);
#if defined(CONFIG_WIFI_TEST_AUTH_MODE_WPA2)
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.psk = CONFIG_WIFI_TEST_PSK;
	params.psk_length = strlen(CONFIG_WIFI_TEST_PSK);
#elif defined(CONFIG_WIFI_TEST_AUTH_MODE_WPA3)
	params.security = WIFI_SECURITY_TYPE_SAE;
	params.sae_password = CONFIG_WIFI_TEST_PSK;
	params.sae_password_length = strlen(CONFIG_WIFI_TEST_PSK);
#else
	params.security = WIFI_SECURITY_TYPE_NONE;
#endif

	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params,
		       sizeof(struct wifi_connect_req_params));

	if (ret) {
		LOG_INF("Connection request failed with error: %d", ret);
		return ret;
	}

	LOG_INF("Connection requested...");

	return 0;
}

static int wifi_disconnect(void)
{
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (ret) {
		LOG_INF("Disconnect request failed with error: %d", ret);
		return ret;
	}

	return 0;
}

static int wifi_state(void)
{
	struct wifi_iface_status status = {0};

	net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(struct wifi_iface_status));

	return status.state;
}

ZTEST(wifi, test_wifi_0_scan)
{
	int ret;

	ret = wifi_scan();
	zassert_equal(ret, 0, "Scan request failed");

	zassert_equal(k_sem_take(&wifi_event, K_SECONDS(CONFIG_WIFI_SCAN_TIMEOUT)), 0,
		      "Wifi scan failed or timed out");

	LOG_INF("Scan done");
}

ZTEST(wifi, test_wifi_1_connect)
{
	int ret;
	int retry = CONFIG_WIFI_CONNECT_ATTEMPTS;

	/* Manage connect retry as disconnect event may happen */
	connecting = true;

	do {
		ret = wifi_connect();
		zassert_equal(ret, 0, "Connect request failed");

		zassert_equal(k_sem_take(&wifi_event, K_SECONDS(CONFIG_WIFI_CONNECT_TIMEOUT)), 0,
			      "Wifi connect timed out");

		if (result) {
			zassert(--retry, "Connect failed");
			LOG_INF("Failed attempt, retry %d", CONFIG_WIFI_CONNECT_ATTEMPTS - retry);
			k_sleep(K_SECONDS(1));
		} else {
			break;
		}
	} while (retry);

	connecting = false;

	/* Check interface state */
	int state = wifi_state();

	LOG_INF("Interface state: %s", wifi_state_txt(state));

	zassert_equal(state, WIFI_STATE_COMPLETED, "Interface state check failed");
}

ZTEST(wifi, test_wifi_2_icmp)
{
	struct net_icmp_ping_params params;
	struct net_icmp_ctx ctx;
	struct in_addr gw_addr_4;
	struct sockaddr_in dst4 = {0};
	int ret;

	gw_addr_4 = net_if_ipv4_get_gw(iface);
	zassert_not_equal(gw_addr_4.s_addr, 0, "Gateway address is not set");

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV4_ECHO_REPLY, 0, icmp_event);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	dst4.sin_family = AF_INET;
	memcpy(&dst4.sin_addr, &gw_addr_4, sizeof(gw_addr_4));

	params.identifier = 1234;
	params.sequence = 5678;
	params.tc_tos = 1;
	params.priority = 2;
	params.data = TEST_DATA;
	params.data_size = sizeof(TEST_DATA);

	LOG_INF("Pinging the gateway...");

	ret = net_icmp_send_echo_request(&ctx, iface, (struct sockaddr *)&dst4, &params, NULL);
	zassert_equal(ret, 0, "Cannot send ICMP echo request (%d)", ret);

	zassert_equal(k_sem_take(&wifi_event, K_SECONDS(CONFIG_WIFI_PING_TIMEOUT)), 0,
		      "Gateway ping (ICMP) timed out");

	net_icmp_cleanup_ctx(&ctx);
}

ZTEST(wifi, test_wifi_3_disconnect)
{
	int ret;

	ret = wifi_disconnect();
	zassert_equal(ret, 0, "Disconect request failed");

	zassert_equal(k_sem_take(&wifi_event, K_SECONDS(CONFIG_WIFI_DISCONNECT_TIMEOUT)), 0,
		      "Wifi disconnect timed out");

	zassert_equal(result, 0, "Disconnect failed");
}

static void *wifi_setup(void)
{
	iface = net_if_get_wifi_sta();

	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler, WIFI_MGMT_EVENTS);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	/* reset semaphore that tracks wifi events */
	k_sem_reset(&wifi_event);

	return NULL;
}

ZTEST_SUITE(wifi, NULL, wifi_setup, NULL, NULL, NULL);
