/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_credentials.h>

#define SSID_A "ssid_a"
#define SSID_B "ssid_b"
#define PASSWORD "password123"

#ifdef CONFIG_WIFI_CREDENTIALS_STATIC
#define STATIC_SSID CONFIG_WIFI_CREDENTIALS_STATIC_SSID
#else
#define STATIC_SSID ""
#endif

/* The static network is tried after the stored ones if none accepted the request */
#define STATIC_CALLS (IS_ENABLED(CONFIG_WIFI_CREDENTIALS_STATIC) ? 1 : 0)

#define MAX_CONNECT_CALLS 4

struct wifi_drv_context {
	uint8_t mac_addr[6];
};

static struct wifi_drv_context wifi_context;

/* Return values of the fake connect op, one for each call */
static int connect_rc[MAX_CONNECT_CALLS];
/* SSIDs passed to the fake connect op, one for each call */
static char connect_ssid[MAX_CONNECT_CALLS][WIFI_SSID_MAX_LEN + 1];
static int connect_calls;

static void wifi_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct wifi_drv_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr, sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	net_eth_set_if_type_wifi(iface);

	ethernet_init(iface);
}

static int wifi_connect(const struct device *dev, struct net_if *iface,
			struct wifi_connect_req_params *params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	zassert_true(connect_calls < MAX_CONNECT_CALLS, "Too many connect calls");

	memcpy(connect_ssid[connect_calls], params->ssid, params->ssid_length);

	return connect_rc[connect_calls++];
}

static struct wifi_mgmt_ops wifi_mgmt_api = {
	.connect = wifi_connect,
};

static DEVICE_API(wifi_native, api_funcs) = {
	.wifi_iface.iface_api.init = wifi_iface_init,
	.wifi_mgmt_api = &wifi_mgmt_api,
};

static int wifi_init(const struct device *dev)
{
	struct wifi_drv_context *context = dev->data;

	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	context->mac_addr[0] = 0x00;
	context->mac_addr[1] = 0x00;
	context->mac_addr[2] = 0x5E;
	context->mac_addr[3] = 0x00;
	context->mac_addr[4] = 0x53;
	context->mac_addr[5] = sys_rand8_get();

	return 0;
}

ETH_NET_DEVICE_INIT(wlan0, "wifi_test", wifi_init, NULL, &wifi_context, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);

static void store_network(const char *ssid)
{
	int ret;

	ret = wifi_credentials_set_personal(ssid, strlen(ssid), WIFI_SECURITY_TYPE_PSK, NULL, 0,
					    PASSWORD, strlen(PASSWORD), 0, 0, 0);
	zassert_equal(ret, 0, "Failed to store credentials for %s: %d", ssid, ret);
}

static int connect_stored(void)
{
	return net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, net_if_get_first_wifi(), NULL, 0);
}

static void connect_stored_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_equal(wifi_credentials_delete_all(), 0, "Failed to delete credentials");
	memset(connect_rc, 0, sizeof(connect_rc));
	memset(connect_ssid, 0, sizeof(connect_ssid));
	connect_calls = 0;
}

ZTEST(wifi_connect_stored, test_no_credentials)
{
	zassert_equal(connect_stored(), 0);
	zassert_equal(connect_calls, STATIC_CALLS);
}

ZTEST(wifi_connect_stored, test_connect)
{
	store_network(SSID_A);

	/* A stored network is used before the static one */
	zassert_equal(connect_stored(), 0);
	zassert_equal(connect_calls, 1);
	zassert_str_equal(connect_ssid[0], SSID_A);
}

ZTEST(wifi_connect_stored, test_already_connected)
{
	store_network(SSID_A);
	connect_rc[0] = -EALREADY;

	zassert_equal(connect_stored(), 0);
	zassert_equal(connect_calls, 1);
}

ZTEST(wifi_connect_stored, test_connect_error)
{
	store_network(SSID_A);
	connect_rc[0] = -EIO;
	connect_rc[1] = -EIO;

	zassert_equal(connect_stored(), -EIO);
	zassert_equal(connect_calls, 1 + STATIC_CALLS);
}

ZTEST(wifi_connect_stored, test_stop_at_first_accepted)
{
	store_network(SSID_A);
	store_network(SSID_B);

	zassert_equal(connect_stored(), 0);
	zassert_equal(connect_calls, 1);
}

ZTEST(wifi_connect_stored, test_fall_back_to_next_network)
{
	store_network(SSID_A);
	store_network(SSID_B);
	connect_rc[0] = -EIO;

	zassert_equal(connect_stored(), 0);
	zassert_equal(connect_calls, 2);
}

ZTEST(wifi_connect_stored, test_all_networks_fail)
{
	store_network(SSID_A);
	store_network(SSID_B);
	connect_rc[0] = -EIO;
	connect_rc[1] = -EINVAL;
	connect_rc[2] = -ETIMEDOUT;

	/* The error from the last network tried is reported */
	zassert_equal(connect_stored(),
		      IS_ENABLED(CONFIG_WIFI_CREDENTIALS_STATIC) ? -ETIMEDOUT : -EINVAL);
	zassert_equal(connect_calls, 2 + STATIC_CALLS);
}

ZTEST(wifi_connect_stored, test_static_fallback)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_WIFI_CREDENTIALS_STATIC);

	store_network(SSID_A);
	connect_rc[0] = -EIO;

	zassert_equal(connect_stored(), 0);
	zassert_equal(connect_calls, 2);
	zassert_str_equal(connect_ssid[0], SSID_A);
	zassert_str_equal(connect_ssid[1], STATIC_SSID);
}

ZTEST(wifi_connect_stored, test_static_overridden_by_storage)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_WIFI_CREDENTIALS_STATIC);

	/* The stored entry replaces the static network, so it is not tried again */
	store_network(STATIC_SSID);
	connect_rc[0] = -EIO;

	zassert_equal(connect_stored(), -EIO);
	zassert_equal(connect_calls, 1);
}

ZTEST_SUITE(wifi_connect_stored, NULL, NULL, connect_stored_before, NULL, NULL);
