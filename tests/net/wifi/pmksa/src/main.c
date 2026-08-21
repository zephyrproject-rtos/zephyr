/*
 * Copyright (c) 2026 Siddhant Modi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/ztest.h>

static const uint8_t wifi_mac_addr[WIFI_MAC_ADDR_LEN] = {
	0x02, 0x00, 0x00, 0x00, 0x00, 0x02,
};

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
static const uint8_t ssid[] = "pmksa-test";
#endif
static struct net_if *wifi_iface;
static const struct wifi_pmksa_cache_entry empty_entry;
static const struct wifi_pmksa_cache_entry valid_entry = {
	.expiration_remaining_s = 300U,
	.reauth_remaining_s = 120U,
	.akm = WIFI_AKM_SUITE_802_1X_SHA256,
	.pmk = {0x22},
	.pmkid = {0x11},
	.bssid = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
	.spa = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02},
	.fils_cache_id = {0x12, 0x34},
	.pmk_len = 32U,
	.fils_cache_id_set = true,
	.opportunistic = true,
};

static bool pmksa_entry_equal(const struct wifi_pmksa_cache_entry *left,
			      const struct wifi_pmksa_cache_entry *right)
{
	return left->expiration_remaining_s == right->expiration_remaining_s &&
	       left->reauth_remaining_s == right->reauth_remaining_s && left->akm == right->akm &&
	       memcmp(left->pmk, right->pmk, sizeof(left->pmk)) == 0 &&
	       memcmp(left->pmkid, right->pmkid, sizeof(left->pmkid)) == 0 &&
	       memcmp(left->bssid, right->bssid, sizeof(left->bssid)) == 0 &&
	       memcmp(left->spa, right->spa, sizeof(left->spa)) == 0 &&
	       memcmp(left->fils_cache_id, right->fils_cache_id, sizeof(left->fils_cache_id)) ==
		       0 &&
	       left->pmk_len == right->pmk_len &&
	       left->fils_cache_id_set == right->fils_cache_id_set &&
	       left->opportunistic == right->opportunistic;
}

static struct wifi_pmksa_cache_entry backend_entries[2];
static size_t backend_entry_count;
static int connect_result;
static bool connect_called;
static int flush_result;
static bool flush_called;
static bool flush_external_called;
static int status_result;
#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
static enum wifi_pmksa_cache_usage status_usage_before_dispatch;
#endif

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
static struct wifi_pmksa_cache_entry captured_entries[2];
static size_t captured_count;

static int fake_connect(const struct device *dev, struct net_if *iface,
			struct wifi_connect_req_params *params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	connect_called = true;
	captured_count = params->pmksa_entry_count;
	if (captured_count != 0U) {
		memcpy(captured_entries, params->pmksa_entries,
		       captured_count * sizeof(captured_entries[0]));
	}
	return connect_result;
}
#else
static int fake_connect(const struct device *dev, struct net_if *iface,
			struct wifi_connect_req_params *params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	connect_called = true;
	return connect_result;
}
#endif

#ifdef CONFIG_WIFI_MGMT_PMKSA_EXPORT
static bool get_called;
static bool get_output_was_clear;
static int get_result;
static uint64_t event_seen;
static struct net_if *event_iface;
static uint8_t event_bssid[WIFI_MAC_ADDR_LEN];
static size_t event_info_length;
static struct net_mgmt_event_callback event_cb;
K_SEM_DEFINE(pmksa_event_received, 0, 1);

static int fake_pmksa_get(const struct device *dev, struct net_if *iface,
			  struct wifi_pmksa_cache_query *query)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	get_called = true;
	get_output_was_clear =
		query->entry_count == 0U && pmksa_entry_equal(&query->entry, &empty_entry);
	query->entry_count = backend_entry_count;
	if (get_result != 0) {
		return get_result;
	}
	if (query->index >= backend_entry_count) {
		return -ENOENT;
	}

	query->entry = backend_entries[query->index];
	return get_result;
}

static void pmksa_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
				struct net_if *iface)
{
	const struct wifi_pmksa_cache_event *info = cb->info;

	event_seen = event;
	event_iface = iface;
	event_info_length = cb->info_length;
	memcpy(event_bssid, info->bssid, sizeof(event_bssid));
	k_sem_give(&pmksa_event_received);
}
#endif

static int fake_pmksa_flush(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	flush_called = true;
	return flush_result;
}

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
static int fake_pmksa_flush_external(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	flush_external_called = true;
	return flush_result;
}
#endif

static int fake_iface_status(const struct device *dev, struct net_if *iface,
			     struct wifi_iface_status *status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
	status_usage_before_dispatch = status->pmksa_cache_usage;
#else
	ARG_UNUSED(status);
#endif
	return status_result;
}

static struct wifi_mgmt_ops wifi_mgmt_api = {
	.connect = fake_connect,
	.iface_status = fake_iface_status,
	.pmksa_flush = fake_pmksa_flush,
#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
	.pmksa_flush_external = fake_pmksa_flush_external,
#endif
#ifdef CONFIG_WIFI_MGMT_PMKSA_EXPORT
	.pmksa_get = fake_pmksa_get,
#endif
};

static void wifi_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, wifi_mac_addr, sizeof(wifi_mac_addr), NET_LINK_ETHERNET);
	net_eth_set_if_type_wifi(iface);
	ethernet_init(iface);
}

static int wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	net_pkt_unref(pkt);

	return 0;
}

static struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface.iface_api.init = wifi_iface_init,
	.wifi_iface.send = wifi_send,
	.wifi_mgmt_api = &wifi_mgmt_api,
};

ETH_NET_DEVICE_INIT(wlan0, "wifi_pmksa_test", NULL, NULL, NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

static void reset_backend(void)
{
	backend_entries[0] = valid_entry;
	backend_entries[1] = valid_entry;
	backend_entries[1].bssid[5] = 3U;
	backend_entry_count = 2U;
	connect_result = 0;
	connect_called = false;
	flush_result = 0;
	flush_called = false;
	flush_external_called = false;
	status_result = 0;
#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
	status_usage_before_dispatch = WIFI_PMKSA_CACHE_USAGE_UNKNOWN;
	memset(captured_entries, 0, sizeof(captured_entries));
	captured_count = 0U;
	wifi_mgmt_api.pmksa_flush_external = fake_pmksa_flush_external;
#endif
#ifdef CONFIG_WIFI_MGMT_PMKSA_EXPORT
	get_result = 0;
	get_called = false;
	get_output_was_clear = false;
	memset(event_bssid, 0, sizeof(event_bssid));
	event_seen = 0U;
	event_iface = NULL;
	event_info_length = 0U;
	wifi_mgmt_api.pmksa_get = fake_pmksa_get;
#endif
}

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
static int request_connect(const struct wifi_pmksa_cache_entry *entries, size_t entry_count)
{
	struct wifi_connect_req_params params = {
		.ssid = ssid,
		.ssid_length = sizeof(ssid) - 1U,
		.channel = WIFI_CHANNEL_ANY,
		.security = WIFI_SECURITY_TYPE_NONE,
	};
	params.pmksa_entries = entries;
	params.pmksa_entry_count = entry_count;

	return net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params));
}
#endif

static void *pmksa_setup(void)
{
	wifi_iface = net_if_get_first_wifi();
	zassert_not_null(wifi_iface, "Wi-Fi test interface was not registered");

#ifdef CONFIG_WIFI_MGMT_PMKSA_EXPORT
	net_mgmt_init_event_callback(&event_cb, pmksa_event_handler,
				     NET_EVENT_WIFI_PMKSA_CACHE_ADDED |
					     NET_EVENT_WIFI_PMKSA_CACHE_REMOVED);
	net_mgmt_add_event_callback(&event_cb);
#endif

	return NULL;
}

static void pmksa_before(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)net_if_up(wifi_iface);
	reset_backend();
}

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
ZTEST(net_wifi_pmksa, test_connect_rejects_invalid_request_size)
{
	struct wifi_connect_req_params params = {
		.ssid = ssid,
		.ssid_length = sizeof(ssid) - 1U,
		.channel = WIFI_CHANNEL_ANY,
		.security = WIFI_SECURITY_TYPE_NONE,
	};

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params) - 1U),
		      -EINVAL, "CONNECT accepted an invalid request size");
	zassert_false(connect_called, "Invalid CONNECT reached the backend");
}

ZTEST(net_wifi_pmksa, test_connect_imports_zero_one_and_two_records)
{
	const struct wifi_pmksa_cache_entry entries[] = {valid_entry, backend_entries[1]};

	zassert_equal(request_connect(NULL, 0U), 0, "Zero-entry CONNECT failed");
	zassert_true(connect_called, "Zero-entry CONNECT did not dispatch");
	zassert_equal(captured_count, 0U, "Zero-entry CONNECT count changed");

	connect_called = false;
	zassert_equal(request_connect(&entries[0], 1U), 0, "One-entry CONNECT failed");
	zassert_true(connect_called, "One-entry CONNECT did not dispatch");
	zassert_equal(captured_count, 1U, "One-entry CONNECT count changed");
	zassert_true(pmksa_entry_equal(&captured_entries[0], &entries[0]),
		     "One-entry CONNECT was not copied");

	connect_called = false;
	zassert_equal(request_connect(entries, 2U), 0, "Two-entry CONNECT failed");
	zassert_true(connect_called, "Two-entry CONNECT did not dispatch");
	zassert_equal(captured_count, 2U, "Two-entry CONNECT count changed");
	for (size_t i = 0U; i < ARRAY_SIZE(entries); ++i) {
		zassert_true(pmksa_entry_equal(&captured_entries[i], &entries[i]),
			     "Two-entry CONNECT entry %zu was not copied", i);
	}
}

ZTEST(net_wifi_pmksa, test_connect_import_propagates_backend_error)
{
	connect_result = -EIO;

	zassert_equal(request_connect(&valid_entry, 1U), -EIO,
		      "CONNECT backend error was not propagated");
	zassert_true(connect_called, "CONNECT backend was not called");
}

ZTEST(net_wifi_pmksa, test_connect_import_rejects_null_count_mismatch)
{
	struct wifi_connect_req_params params = {
		.ssid = ssid,
		.ssid_length = sizeof(ssid) - 1U,
		.channel = WIFI_CHANNEL_ANY,
		.security = WIFI_SECURITY_TYPE_NONE,
	};

	params.pmksa_entries = &valid_entry;
	params.pmksa_entry_count = 0U;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params)),
		      -EINVAL, "Null/count mismatch accepted");
	zassert_false(connect_called, "Mismatch reached CONNECT backend");

	params.pmksa_entries = NULL;
	params.pmksa_entry_count = 1U;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params)),
		      -EINVAL, "Count/null mismatch accepted");
	zassert_false(connect_called, "Mismatch reached CONNECT backend");
}

ZTEST(net_wifi_pmksa, test_connect_import_rejects_malformed_records)
{
	struct wifi_pmksa_cache_entry malformed[] = {
		valid_entry, valid_entry, valid_entry, valid_entry, valid_entry, valid_entry,
		valid_entry, valid_entry, valid_entry, valid_entry, valid_entry, valid_entry,
	};

	memset(malformed[0].bssid, 0, sizeof(malformed[0].bssid));
	malformed[1].bssid[0] = 0x01U;
	memset(malformed[2].spa, 0, sizeof(malformed[2].spa));
	malformed[3].spa[0] = 0x01U;
	malformed[4].pmk_len = WIFI_PMKSA_PMK_MIN_LEN - 1U;
	malformed[5].pmk_len = WIFI_PMKSA_PMK_MAX_LEN + 1U;
	malformed[6].akm = WIFI_AKM_SUITE_UNKNOWN;
	malformed[7].akm = (enum wifi_akm_suite)0x000FACFFU;
	malformed[8].expiration_remaining_s = 0U;
	malformed[9].reauth_remaining_s = malformed[9].expiration_remaining_s + 1U;
	malformed[10].fils_cache_id_set = false;
	malformed[10].fils_cache_id[0] = 1U;
	malformed[11].fils_cache_id_set = false;
	malformed[11].fils_cache_id[1] = 1U;

	for (size_t i = 0U; i < ARRAY_SIZE(malformed); ++i) {
		connect_called = false;
		zassert_equal(request_connect(&malformed[i], 1U), -EINVAL,
			      "Malformed record %zu was accepted", i);
		zassert_false(connect_called, "Malformed record reached CONNECT");
	}
}

ZTEST(net_wifi_pmksa, test_connect_import_accepts_immediate_reauth)
{
	struct wifi_pmksa_cache_entry entry = valid_entry;

	entry.reauth_remaining_s = 0U;
	zassert_equal(request_connect(&entry, 1U), 0, "Immediate reauthentication was rejected");
	zassert_true(connect_called, "Immediate reauthentication did not dispatch");
}
#endif

#ifdef CONFIG_WIFI_MGMT_PMKSA_EXPORT
static int request_get(struct wifi_pmksa_cache_query *query)
{
	return net_mgmt(NET_REQUEST_WIFI_PMKSA_GET, wifi_iface, query, sizeof(*query));
}

ZTEST(net_wifi_pmksa, test_indexed_export_preserves_index_and_clears_output)
{
	struct wifi_pmksa_cache_query query = {
		.entry = valid_entry,
		.index = 1U,
		.entry_count = 99U,
	};

	zassert_equal(request_get(&query), 0, "GET request failed");
	zassert_true(get_called, "GET callback was not called");
	zassert_true(get_output_was_clear, "GET callback saw uncleared output");
	zassert_equal(query.index, 1U, "GET index was not preserved");
	zassert_equal(query.entry_count, backend_entry_count, "GET count missing");
	zassert_true(pmksa_entry_equal(&query.entry, &backend_entries[1]),
		     "GET selected entry missing");
}

static void expect_get_error(int expected_error)
{
	struct wifi_pmksa_cache_query query = {
		.entry = valid_entry,
		.index = 1U,
		.entry_count = 99U,
	};

	zassert_equal(request_get(&query), expected_error, "GET returned an unexpected error");
	zassert_equal(query.index, 1U, "GET error changed index");
	zassert_equal(query.entry_count, 0U, "GET error left count set");
	zassert_true(pmksa_entry_equal(&query.entry, &empty_entry), "GET error left entry set");
}

ZTEST(net_wifi_pmksa, test_indexed_export_clears_all_error_outputs)
{
	get_result = -ENOENT;
	expect_get_error(-ENOENT);

	wifi_mgmt_api.pmksa_get = NULL;
	expect_get_error(-ENOTSUP);
	wifi_mgmt_api.pmksa_get = fake_pmksa_get;

	zassert_equal(net_if_down(wifi_iface), 0, "Failed to bring interface down");
	expect_get_error(-ENETDOWN);
}

ZTEST(net_wifi_pmksa, test_indexed_export_propagates_backend_errors)
{
	const int errors[] = {-ENOTCONN, -EIO};

	for (size_t i = 0U; i < ARRAY_SIZE(errors); ++i) {
		get_result = errors[i];
		expect_get_error(errors[i]);
	}
}

ZTEST(net_wifi_pmksa, test_cache_events_carry_only_bssid_and_iface)
{
	const uint8_t bssid[WIFI_MAC_ADDR_LEN] = {
		0x02, 0x00, 0x00, 0x00, 0x00, 0x09,
	};
	const uint64_t events[] = {
		NET_EVENT_WIFI_PMKSA_CACHE_ADDED,
		NET_EVENT_WIFI_PMKSA_CACHE_REMOVED,
	};

	for (size_t i = 0U; i < ARRAY_SIZE(events); ++i) {
		event_seen = 0U;
		k_sem_reset(&pmksa_event_received);
		wifi_mgmt_raise_pmksa_cache_event(wifi_iface, events[i], bssid);
		zassert_equal(k_sem_take(&pmksa_event_received, K_SECONDS(1)), 0,
			      "Timed out waiting for PMKSA event");
		zassert_equal(event_seen, events[i], "Unexpected PMKSA event");
		zassert_equal(event_iface, wifi_iface, "PMKSA event iface changed");
		zassert_equal(event_info_length, sizeof(struct wifi_pmksa_cache_event),
			      "PMKSA event payload length changed");
		zassert_mem_equal(event_bssid, bssid, sizeof(bssid), "PMKSA event BSSID changed");
	}
}
#endif

ZTEST(net_wifi_pmksa, test_flush_all_dispatches)
{
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_PMKSA_FLUSH, wifi_iface, NULL, 0), 0,
		      "Flush-all failed");
	zassert_true(flush_called, "Flush-all was not dispatched");
}

#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
ZTEST(net_wifi_pmksa, test_flush_external_is_distinct_from_flush_all)
{
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_PMKSA_FLUSH_EXTERNAL, wifi_iface, NULL, 0), 0,
		      "External flush failed");
	zassert_true(flush_external_called, "External flush was not dispatched");
	zassert_false(flush_called, "External flush changed flush-all callback");
}
#endif

ZTEST(net_wifi_pmksa, test_flush_errors_are_propagated)
{
	flush_result = -EIO;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_PMKSA_FLUSH, wifi_iface, NULL, 0), -EIO,
		      "Flush-all error was not propagated");
#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_PMKSA_FLUSH_EXTERNAL, wifi_iface, NULL, 0), -EIO,
		      "External flush error was not propagated");
#endif
}

ZTEST(net_wifi_pmksa, test_iface_status_initializes_usage_and_propagates_errors)
{
	struct wifi_iface_status status;
	const int results[] = {0, -EIO};

	for (size_t i = 0U; i < ARRAY_SIZE(results); ++i) {
		status_result = results[i];
		memset(&status, 0xA5, sizeof(status));
		zassert_equal(net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, wifi_iface, &status,
				       sizeof(status)),
			      status_result, "Status result was not propagated");
#ifdef CONFIG_WIFI_MGMT_PMKSA_IMPORT
		zassert_equal(status_usage_before_dispatch, WIFI_PMKSA_CACHE_USAGE_NOT_ATTEMPTED,
			      "Status usage was not initialized");
#endif
	}
}

ZTEST(net_wifi_pmksa, test_secret_clear_wipes_array_and_handles_null)
{
	struct wifi_pmksa_cache_entry entries[2] = {valid_entry, valid_entry};

	wifi_pmksa_cache_entries_clear(entries, ARRAY_SIZE(entries));
	for (size_t i = 0U; i < ARRAY_SIZE(entries); ++i) {
		zassert_true(pmksa_entry_equal(&entries[i], &empty_entry),
			     "PMKSA entry was not cleared");
	}
	wifi_pmksa_cache_entries_clear(NULL, 0U);
}

ZTEST_SUITE(net_wifi_pmksa, NULL, pmksa_setup, pmksa_before, NULL, NULL);
