/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/ztest.h>

static const uint8_t wifi_mac_addr[WIFI_MAC_ADDR_LEN] = {
	0x02, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static struct net_if *wifi_iface;
static struct wifi_pmksa_cache_entry backend_entry;
static const struct wifi_pmksa_cache_entry empty_entry;
static struct wifi_pmksa_cache_entry added_entry;
static int get_result;
static int add_result;
static bool get_called;
static bool get_entry_zeroed;
static bool add_called;

static bool pmksa_entries_equal(const struct wifi_pmksa_cache_entry *a,
				const struct wifi_pmksa_cache_entry *b)
{
	return memcmp(a->bssid, b->bssid, sizeof(a->bssid)) == 0 &&
	       memcmp(a->pmkid, b->pmkid, sizeof(a->pmkid)) == 0 &&
	       memcmp(a->pmk, b->pmk, sizeof(a->pmk)) == 0 &&
	       a->pmk_len == b->pmk_len &&
	       a->akm_suite == b->akm_suite &&
	       a->reauth_remaining_s == b->reauth_remaining_s &&
	       a->expiration_remaining_s == b->expiration_remaining_s;
}

static void wifi_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, wifi_mac_addr, sizeof(wifi_mac_addr),
			     NET_LINK_ETHERNET);
	net_eth_set_if_type_wifi(iface);
	ethernet_init(iface);
}

static int wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	net_pkt_unref(pkt);

	return 0;
}

static int wifi_pmksa_get(const struct device *dev, struct net_if *iface,
			  struct wifi_pmksa_cache_entry *entry)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	get_called = true;
	get_entry_zeroed = pmksa_entries_equal(entry, &empty_entry);
	memcpy(entry, &backend_entry, sizeof(*entry));

	return get_result;
}

static int wifi_pmksa_add(const struct device *dev, struct net_if *iface,
			  const struct wifi_pmksa_cache_entry *entry)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	add_called = true;
	memcpy(&added_entry, entry, sizeof(added_entry));

	return add_result;
}

static struct wifi_mgmt_ops wifi_mgmt_api = {
	.pmksa_get = wifi_pmksa_get,
	.pmksa_add = wifi_pmksa_add,
};

static struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface.iface_api.init = wifi_iface_init,
	.wifi_iface.send = wifi_send,
	.wifi_mgmt_api = &wifi_mgmt_api,
};

ETH_NET_DEVICE_INIT(wlan0, "wifi_pmksa_test", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

static const struct wifi_pmksa_cache_entry valid_entry = {
	.bssid = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 },
	.pmkid = { [0 ... WIFI_PMKSA_PMKID_LEN - 1] = 0x11 },
	.pmk = { [0 ... 31] = 0x22 },
	.pmk_len = 32,
	.akm_suite = 0x000fac01U,
	.reauth_remaining_s = 120U,
	.expiration_remaining_s = 300U,
};

static void reset_backend(void)
{
	backend_entry = valid_entry;
	memset(&added_entry, 0, sizeof(added_entry));
	get_result = 0;
	add_result = 0;
	get_called = false;
	get_entry_zeroed = false;
	add_called = false;
	wifi_mgmt_api.pmksa_get = wifi_pmksa_get;
	wifi_mgmt_api.pmksa_add = wifi_pmksa_add;
}

static int request_get(struct wifi_pmksa_cache_entry *entry, size_t len)
{
	return net_mgmt(NET_REQUEST_WIFI_PMKSA_GET, wifi_iface, entry, len);
}

static int request_add(const struct wifi_pmksa_cache_entry *entry, size_t len)
{
	return net_mgmt(NET_REQUEST_WIFI_PMKSA_ADD, wifi_iface,
		       (void *)entry, len);
}

static void *pmksa_setup(void)
{
	wifi_iface = net_if_get_first_wifi();

	return NULL;
}

static void pmksa_before(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)net_if_up(wifi_iface);
	reset_backend();
}

ZTEST(net_wifi_pmksa, test_get_dispatch)
{
	struct wifi_pmksa_cache_entry entry;

	memset(&entry, 0xA5, sizeof(entry));

	zassert_equal(request_get(&entry, sizeof(entry)), 0,
		      "GET request failed");
	zassert_true(get_called, "GET callback was not called");
	zassert_true(get_entry_zeroed, "GET callback received nonzero output");
	zassert_true(pmksa_entries_equal(&entry, &valid_entry),
		     "GET result was not returned");
}

static void expect_get_invalid(struct wifi_pmksa_cache_entry *entry, size_t len)
{
	get_called = false;
	zassert_equal(request_get(entry, len), -EINVAL,
		      "Invalid GET record was accepted");
	zassert_false(get_called, "Invalid GET record reached the backend");
}

ZTEST(net_wifi_pmksa, test_get_rejects_invalid_records)
{
	struct wifi_pmksa_cache_entry entry = valid_entry;
	uint8_t oversized_record[sizeof(entry) + 1];

	expect_get_invalid(NULL, 0);
	expect_get_invalid(&entry, sizeof(entry) - 1);
	memcpy(oversized_record, &entry, sizeof(entry));
	expect_get_invalid((struct wifi_pmksa_cache_entry *)oversized_record,
			   sizeof(oversized_record));
}

ZTEST(net_wifi_pmksa, test_add_dispatch_and_error)
{
	struct wifi_pmksa_cache_entry entry = valid_entry;

	add_result = -EIO;

	zassert_equal(request_add(&entry, sizeof(entry)), -EIO,
		      "ADD backend error was not propagated");
	zassert_true(add_called, "ADD callback was not called");
	zassert_true(pmksa_entries_equal(&added_entry, &entry),
		     "ADD record was not passed to the backend");
}

static void expect_get_error(int expected_error)
{
	struct wifi_pmksa_cache_entry entry;

	memset(&entry, 0xA5, sizeof(entry));
	zassert_equal(request_get(&entry, sizeof(entry)), expected_error,
		      "GET returned an unexpected error");
	zassert_true(pmksa_entries_equal(&entry, &empty_entry),
		     "GET output was not cleared");
}

ZTEST(net_wifi_pmksa, test_get_clears_result_on_backend_error)
{
	get_result = -EIO;
	expect_get_error(-EIO);
}

ZTEST(net_wifi_pmksa, test_get_clears_result_without_callback)
{
	wifi_mgmt_api.pmksa_get = NULL;
	expect_get_error(-ENOTSUP);
}

ZTEST(net_wifi_pmksa, test_get_clears_result_when_interface_down)
{
	zassert_equal(net_if_down(wifi_iface), 0,
		"Failed to bring interface down");
	expect_get_error(-ENETDOWN);
}

static void expect_add_invalid(const struct wifi_pmksa_cache_entry *entry,
				size_t len)
{
	zassert_equal(request_add(entry, len), -EINVAL,
		      "Invalid ADD record was accepted");
	zassert_false(add_called, "Invalid ADD record reached the backend");
}

ZTEST(net_wifi_pmksa, test_add_rejects_invalid_records)
{
	struct wifi_pmksa_cache_entry entry = valid_entry;
	uint8_t oversized_record[sizeof(entry) + 1];

	expect_add_invalid(NULL, 0);
	expect_add_invalid(&entry, sizeof(entry) - 1);
	memcpy(oversized_record, &entry, sizeof(entry));
	expect_add_invalid((const struct wifi_pmksa_cache_entry *)oversized_record,
			   sizeof(oversized_record));

	memset(entry.bssid, 0, sizeof(entry.bssid));
	expect_add_invalid(&entry, sizeof(entry));
	entry = valid_entry;
	entry.bssid[0] = 0x01;
	expect_add_invalid(&entry, sizeof(entry));
	entry = valid_entry;
	entry.pmk_len = 0;
	expect_add_invalid(&entry, sizeof(entry));
	entry = valid_entry;
	entry.pmk_len = WIFI_PMKSA_PMK_MAX_LEN + 1U;
	expect_add_invalid(&entry, sizeof(entry));
	entry = valid_entry;
	entry.akm_suite = 0;
	expect_add_invalid(&entry, sizeof(entry));
	entry = valid_entry;
	entry.expiration_remaining_s = 0;
	expect_add_invalid(&entry, sizeof(entry));
	entry = valid_entry;
	entry.reauth_remaining_s = entry.expiration_remaining_s + 1U;
	expect_add_invalid(&entry, sizeof(entry));
}

ZTEST(net_wifi_pmksa, test_add_accepts_unknown_akm_selector)
{
	struct wifi_pmksa_cache_entry entry = valid_entry;

	entry.akm_suite = UINT32_MAX;

	zassert_equal(request_add(&entry, sizeof(entry)), 0,
		      "Unknown nonzero AKM selector was rejected");
	zassert_true(add_called, "ADD callback was not called");
	zassert_equal(added_entry.akm_suite, UINT32_MAX,
		      "AKM selector was not passed through");
}

static void expect_add_valid(const struct wifi_pmksa_cache_entry *entry)
{
	add_called = false;
	zassert_equal(request_add(entry, sizeof(*entry)), 0,
		      "Valid ADD record was rejected");
	zassert_true(add_called, "Valid ADD record did not reach the backend");
}

ZTEST(net_wifi_pmksa, test_add_accepts_boundary_values)
{
	struct wifi_pmksa_cache_entry entry = valid_entry;

	entry.pmk_len = 1;
	expect_add_valid(&entry);
	entry = valid_entry;
	entry.pmk_len = WIFI_PMKSA_PMK_MAX_LEN;
	expect_add_valid(&entry);
	entry = valid_entry;
	entry.reauth_remaining_s = 0;
	expect_add_valid(&entry);
	entry = valid_entry;
	entry.reauth_remaining_s = entry.expiration_remaining_s;
	expect_add_valid(&entry);
	entry = valid_entry;
	entry.expiration_remaining_s = UINT32_MAX;
	expect_add_valid(&entry);
}

ZTEST_SUITE(net_wifi_pmksa, NULL, pmksa_setup, pmksa_before, NULL, NULL);
