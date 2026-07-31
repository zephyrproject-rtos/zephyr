/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

struct wifi_test_context {
	struct net_if *iface;
	uint8_t mac_addr[WIFI_MAC_ADDR_LEN];
};

static struct wifi_test_context wifi_context;
static struct wifi_pmksa_cache_entry backend_entry;
static struct wifi_pmksa_cache_entry added_entry;
static int backend_get_ret;
static int backend_add_ret;
static bool backend_get_called;
static bool backend_add_called;

static void wifi_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, wifi_context.mac_addr,
			     sizeof(wifi_context.mac_addr), NET_LINK_ETHERNET);
	net_eth_set_if_type_wifi(iface);
	ethernet_init(iface);
}

static int wifi_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	wifi_context.mac_addr[0] = 0x02;
	wifi_context.mac_addr[1] = 0x00;
	wifi_context.mac_addr[2] = 0x00;
	wifi_context.mac_addr[3] = 0x00;
	wifi_context.mac_addr[4] = 0x00;
	wifi_context.mac_addr[5] = 0x01;

	return 0;
}

static int wifi_pmksa_get(const struct device *dev, struct net_if *iface,
			  struct wifi_pmksa_cache_entry *entry)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	backend_get_called = true;
	if (backend_get_ret == 0) {
		*entry = backend_entry;
	}

	return backend_get_ret;
}

static int wifi_pmksa_add(const struct device *dev, struct net_if *iface,
			  const struct wifi_pmksa_cache_entry *entry)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	backend_add_called = true;
	if (backend_add_ret == 0) {
		added_entry = *entry;
	}

	return backend_add_ret;
}

static struct wifi_mgmt_ops wifi_mgmt_api = {
	.pmksa_get = wifi_pmksa_get,
	.pmksa_add = wifi_pmksa_add,
};

static struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface.iface_api.init = wifi_iface_init,
	.wifi_mgmt_api = &wifi_mgmt_api,
};

ETH_NET_DEVICE_INIT(wlan0, "wifi_pmksa_test",
		    wifi_init, NULL,
		    &wifi_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

static void fill_valid_entry(struct wifi_pmksa_cache_entry *entry)
{
	memset(entry, 0, sizeof(*entry));
	entry->bssid[0] = 0x02;
	entry->bssid[5] = 0x01;
	entry->sta_addr[0] = 0x02;
	entry->sta_addr[5] = 0x02;
	memset(entry->pmkid, 0x11, sizeof(entry->pmkid));
	memset(entry->pmk, 0x22, 32);
	entry->pmk_len = 32;
	entry->akm_suite = WIFI_PMKSA_AKM_802_1X;
	entry->reauth_remaining_s = 120;
	entry->expiration_remaining_s = 300;
}

static void reset_backend(void)
{
	fill_valid_entry(&backend_entry);
	memset(&added_entry, 0, sizeof(added_entry));
	backend_get_ret = 0;
	backend_add_ret = 0;
	backend_get_called = false;
	backend_add_called = false;
	wifi_mgmt_api.pmksa_get = wifi_pmksa_get;
	wifi_mgmt_api.pmksa_add = wifi_pmksa_add;
}

static void *pmksa_setup(void)
{
	wifi_context.iface = net_if_get_first_wifi();
	zassert_not_null(wifi_context.iface, "Wi-Fi test interface is unavailable");

	if (!net_if_is_admin_up(wifi_context.iface)) {
		zassert_equal(net_if_up(wifi_context.iface), 0,
			      "Failed to bring Wi-Fi interface up");
	}

	return &wifi_context;
}

static void pmksa_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_backend();
	if (!net_if_is_admin_up(wifi_context.iface)) {
		zassert_equal(net_if_up(wifi_context.iface), 0,
			      "Failed to restore Wi-Fi interface state");
	}
}

ZTEST(net_wifi_pmksa, test_add_dispatch)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	fill_valid_entry(&entry);
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));

	zassert_equal(ret, 0, "PMKSA add failed: %d", ret);
	zassert_true(backend_add_called, "PMKSA add callback was not called");
	zassert_mem_equal(&added_entry, &entry, sizeof(entry));
}

ZTEST(net_wifi_pmksa, test_add_accepts_unspecified_station_address)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	fill_valid_entry(&entry);
	memset(entry.sta_addr, 0, sizeof(entry.sta_addr));
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));

	zassert_equal(ret, 0, "PMKSA add failed: %d", ret);
	zassert_true(backend_add_called, "PMKSA add callback was not called");
	zassert_mem_equal(&added_entry, &entry, sizeof(entry));
}

ZTEST(net_wifi_pmksa, test_get_dispatch)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	memset(&entry, 0xaa, sizeof(entry));
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_GET, wifi_context.iface,
		       &entry, sizeof(entry));

	zassert_equal(ret, 0, "PMKSA get failed: %d", ret);
	zassert_true(backend_get_called, "PMKSA get callback was not called");
	zassert_mem_equal(&entry, &backend_entry, sizeof(entry));
}

ZTEST(net_wifi_pmksa, test_get_error_clears_output)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	backend_get_ret = -ENOENT;
	memset(&entry, 0xaa, sizeof(entry));
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_GET, wifi_context.iface,
		       &entry, sizeof(entry));

	zassert_equal(ret, -ENOENT, "Unexpected PMKSA get result: %d", ret);
	for (size_t i = 0; i < sizeof(entry); i++) {
		zassert_equal(((const uint8_t *)&entry)[i], 0,
			      "Output was not cleared at byte %zu", i);
	}
}

ZTEST(net_wifi_pmksa, test_missing_callback_and_interface_state)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	wifi_mgmt_api.pmksa_get = NULL;
	memset(&entry, 0xaa, sizeof(entry));
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_GET, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -ENOTSUP, "Unexpected missing-get result: %d", ret);
	zassert_mem_equal(&entry, &(struct wifi_pmksa_cache_entry){0}, sizeof(entry));

	wifi_mgmt_api.pmksa_get = wifi_pmksa_get;
	zassert_equal(net_if_down(wifi_context.iface), 0,
		      "Failed to bring interface down");
	memset(&entry, 0xaa, sizeof(entry));
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_GET, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -ENETDOWN, "Unexpected admin-down get result: %d", ret);
	zassert_mem_equal(&entry, &(struct wifi_pmksa_cache_entry){0}, sizeof(entry));

	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &backend_entry, sizeof(backend_entry));
	zassert_equal(ret, -ENETDOWN, "Unexpected admin-down add result: %d", ret);
}

ZTEST(net_wifi_pmksa, test_malformed_add_precedes_backend_checks)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	fill_valid_entry(&entry);
	memset(entry.bssid, 0, sizeof(entry.bssid));
	wifi_mgmt_api.pmksa_add = NULL;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));

	zassert_equal(ret, -EINVAL, "Malformed record did not take precedence: %d", ret);
	zassert_false(backend_add_called, "Malformed record reached backend");
}

ZTEST(net_wifi_pmksa, test_invalid_requests)
{
	struct wifi_pmksa_cache_entry entry;
	int ret;

	fill_valid_entry(&entry);
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       NULL, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Null PMKSA record accepted");
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry) - 1);
	zassert_equal(ret, -EINVAL, "Short PMKSA record accepted");

	entry.bssid[0] = 0;
	entry.bssid[5] = 0;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Zero BSSID accepted");

	fill_valid_entry(&entry);
	entry.bssid[0] = 0x01;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Multicast BSSID accepted");

	fill_valid_entry(&entry);
	entry.sta_addr[0] = 0x01;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Multicast station address accepted");

	fill_valid_entry(&entry);
	entry.pmk_len = 0;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Zero-length PMK accepted");

	fill_valid_entry(&entry);
	entry.pmk_len = WIFI_PMKSA_PMK_MAX_LEN + 1U;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Oversized PMK accepted");

	fill_valid_entry(&entry);
	entry.akm_suite = 0x000fac03U;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -ENOTSUP, "Unsupported AKM accepted");

	fill_valid_entry(&entry);
	entry.expiration_remaining_s = UINT32_MAX;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Excessive lifetime accepted");

	fill_valid_entry(&entry);
	entry.expiration_remaining_s = 0;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Zero expiration accepted");

	fill_valid_entry(&entry);
	entry.reauth_remaining_s = entry.expiration_remaining_s + 1U;
	ret = net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_context.iface,
		       &entry, sizeof(entry));
	zassert_equal(ret, -EINVAL, "Invalid lifetime order accepted");
}

ZTEST_SUITE(net_wifi_pmksa, NULL, pmksa_setup, pmksa_before, NULL, NULL);
