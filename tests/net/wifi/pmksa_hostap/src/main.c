/*
 * Copyright (c) 2026 Siddhant Modi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_WIFI_MGMT_PMKSA_IMPORT) || defined(CONFIG_WIFI_MGMT_PMKSA_EXPORT)
#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "eloop.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "wpa.h"
#include "wpa_i.h"
#include "pmksa_cache.h"
#include "supp_pmksa.h"
#endif /* CONFIG_WIFI_MGMT_PMKSA_IMPORT || CONFIG_WIFI_MGMT_PMKSA_EXPORT */

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_PMKSA_CACHE)
static const uint8_t spa[ETH_ALEN] = {0x02, 0, 0, 0, 0, 2};
static const uint8_t aa[ETH_ALEN] = {0x02, 0, 0, 0, 0, 1};

static uint8_t test_network;
static uint8_t other_network;

static struct wifi_pmksa_cache_entry wifi_entry(enum wifi_akm_suite akm, uint8_t pmk_len,
						bool opportunistic)
{
	struct wifi_pmksa_cache_entry entry = {0};

	entry.expiration_remaining_s = 60;
	entry.reauth_remaining_s = 30;
	entry.akm = akm;
	entry.pmk_len = pmk_len;
	entry.fils_cache_id[0] = 0x12;
	entry.fils_cache_id[1] = 0x34;
	entry.fils_cache_id_set = true;
	entry.opportunistic = opportunistic;
	memcpy(entry.bssid, aa, sizeof(aa));
	memcpy(entry.spa, spa, sizeof(spa));
	for (size_t i = 0; i < pmk_len; i++) {
		entry.pmk[i] = (uint8_t)(i + 1U);
	}
	for (size_t i = 0; i < WIFI_PMKSA_PMKID_LEN; i++) {
		entry.pmkid[i] = (uint8_t)(0xa0U + i);
	}
	return entry;
}

ZTEST(pmksa_hostap, test_akm_mapping_and_lengths)
{
	static const struct {
		enum wifi_akm_suite suite;
		int hostap_akm;
		uint8_t pmk_len;
		int expected;
	} cases[] = {
		{WIFI_AKM_SUITE_802_1X, WPA_KEY_MGMT_IEEE8021X, 32, 0},
		{WIFI_AKM_SUITE_PSK, WPA_KEY_MGMT_PSK, 32, 0},
		{WIFI_AKM_SUITE_FT_802_1X, WPA_KEY_MGMT_FT_IEEE8021X, 32, 0},
		{WIFI_AKM_SUITE_FT_PSK, WPA_KEY_MGMT_FT_PSK, 32, 0},
		{WIFI_AKM_SUITE_802_1X_SHA256, WPA_KEY_MGMT_IEEE8021X_SHA256, 32, 0},
		{WIFI_AKM_SUITE_PSK_SHA256, WPA_KEY_MGMT_PSK_SHA256, 32, 0},
		{WIFI_AKM_SUITE_SAE, WPA_KEY_MGMT_SAE, 32, 0},
		{WIFI_AKM_SUITE_FT_SAE, WPA_KEY_MGMT_FT_SAE, 32, 0},
		{WIFI_AKM_SUITE_802_1X_SUITE_B, WPA_KEY_MGMT_IEEE8021X_SUITE_B, 32, 0},
		{WIFI_AKM_SUITE_802_1X_SUITE_B_192, WPA_KEY_MGMT_IEEE8021X_SUITE_B_192, 48, 0},
		{WIFI_AKM_SUITE_FT_802_1X_SHA384, WPA_KEY_MGMT_FT_IEEE8021X_SHA384, 48, 0},
		{WIFI_AKM_SUITE_FILS_SHA256, WPA_KEY_MGMT_FILS_SHA256, 32, 0},
		{WIFI_AKM_SUITE_FILS_SHA384, WPA_KEY_MGMT_FILS_SHA384, 48, 0},
		{WIFI_AKM_SUITE_FT_FILS_SHA256, WPA_KEY_MGMT_FT_FILS_SHA256, 32, 0},
		{WIFI_AKM_SUITE_FT_FILS_SHA384, WPA_KEY_MGMT_FT_FILS_SHA384, 48, 0},
		{WIFI_AKM_SUITE_OWE, WPA_KEY_MGMT_OWE, 64, 0},
		{WIFI_AKM_SUITE_802_1X_SHA384, WPA_KEY_MGMT_IEEE8021X_SHA384, 48, 0},
		{WIFI_AKM_SUITE_SAE_EXT_KEY, WPA_KEY_MGMT_SAE_EXT_KEY, 32, 0},
		{WIFI_AKM_SUITE_SAE_EXT_KEY, WPA_KEY_MGMT_SAE_EXT_KEY, 48, 0},
		{WIFI_AKM_SUITE_SAE_EXT_KEY, WPA_KEY_MGMT_SAE_EXT_KEY, 64, 0},
		{WIFI_AKM_SUITE_FT_SAE_EXT_KEY, WPA_KEY_MGMT_FT_SAE_EXT_KEY, 32, 0},
		{WIFI_AKM_SUITE_FT_SAE_EXT_KEY, WPA_KEY_MGMT_FT_SAE_EXT_KEY, 48, 0},
		{WIFI_AKM_SUITE_FT_SAE_EXT_KEY, WPA_KEY_MGMT_FT_SAE_EXT_KEY, 64, 0},
		{WIFI_AKM_SUITE_DPP, WPA_KEY_MGMT_DPP, 64, 0},
		{WIFI_AKM_SUITE_FT_PSK_SHA384, 0, 32, -EPROTONOSUPPORT},
		{WIFI_AKM_SUITE_PSK_SHA384, 0, 32, -EPROTONOSUPPORT},
		{WIFI_AKM_SUITE_FT_802_1X_SHA384_UNRESTRICTED, 0, 48, -EPROTONOSUPPORT},
	};
	struct os_reltime now = {.sec = 1000};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct wifi_pmksa_cache_entry source = wifi_entry(
			cases[i].suite, cases[i].pmk_len, cases[i].suite == WIFI_AKM_SUITE_OWE);
		struct rsn_pmksa_cache_entry destination;
		int ret;

		ret = supplicant_pmksa_entry_from_wifi(&source, &test_network, true, spa, &now,
						       &destination);
		zassert_equal(ret, cases[i].expected, "AKM case %zu failed", i);
		if (ret == 0) {
			zassert_equal(destination.akmp, cases[i].hostap_akm, "AKM mismatch");
			zassert_equal(destination.pmk_len, cases[i].pmk_len, "PMK length mismatch");
		}
	}

	struct wifi_pmksa_cache_entry bad = wifi_entry(WIFI_AKM_SUITE_802_1X, 48, false);
	struct rsn_pmksa_cache_entry destination;
	int ret;

	ret = supplicant_pmksa_entry_from_wifi(&bad, &test_network, true, spa, &now, &destination);
	zassert_equal(ret, -EPROTONOSUPPORT, NULL);
	bad = wifi_entry(WIFI_AKM_SUITE_OWE, 31, false);
	ret = supplicant_pmksa_entry_from_wifi(&bad, &test_network, true, spa, &now, &destination);
	zassert_equal(ret, -EPROTONOSUPPORT, NULL);
	bad = wifi_entry(WIFI_AKM_SUITE_SAE_EXT_KEY, 31, false);
	ret = supplicant_pmksa_entry_from_wifi(&bad, &test_network, true, spa, &now, &destination);
	zassert_equal(ret, -EPROTONOSUPPORT, NULL);
}

ZTEST(pmksa_hostap, test_conversion_reanchors_and_clears)
{
	struct os_reltime now = {.sec = 500};
	struct wifi_pmksa_cache_entry source = wifi_entry(WIFI_AKM_SUITE_OWE, 64, true);
	struct rsn_pmksa_cache_entry hostap;
	struct wifi_pmksa_cache_entry destination;

	source.reauth_remaining_s = 0;
	zassert_equal(
		supplicant_pmksa_entry_from_wifi(&source, &test_network, true, spa, &now, &hostap),
		0, NULL);
	zassert_equal(hostap.expiration, 560, NULL);
	zassert_equal(hostap.reauth_time, 500, NULL);
	zassert_true(hostap.external, NULL);
	zassert_equal(hostap.network_ctx, &test_network, NULL);
	zassert_true(hostap.fils_cache_id_set, NULL);
	zassert_equal(hostap.opportunistic, 1, NULL);

	memset(&destination, 0xa5, sizeof(destination));
	zassert_equal(
		supplicant_pmksa_entry_to_wifi(&hostap, &test_network, true, &now, &destination), 0,
		NULL);
	zassert_equal(destination.expiration_remaining_s, 60, NULL);
	zassert_equal(destination.reauth_remaining_s, 0, NULL);
	zassert_mem_equal(destination.pmk, source.pmk, source.pmk_len, NULL);
	hostap.reauth_time = hostap.expiration + 30;
	zassert_equal(
		supplicant_pmksa_entry_to_wifi(&hostap, &test_network, true, &now, &destination), 0,
		NULL);
	zassert_equal(destination.reauth_remaining_s, destination.expiration_remaining_s, NULL);

	uint8_t wrong_spa[ETH_ALEN];

	memcpy(wrong_spa, spa, sizeof(wrong_spa));
	wrong_spa[0]++;
	zassert_equal(supplicant_pmksa_entry_from_wifi(&source, &test_network, true, wrong_spa,
						       &now, &hostap),
		      -EINVAL, NULL);
	hostap.expiration = now.sec;
	zassert_equal(
		supplicant_pmksa_entry_to_wifi(&hostap, &test_network, true, &now, &destination),
		-ENOENT, NULL);
	zassert_equal(destination.pmk_len, 0, NULL);
}

ZTEST(pmksa_hostap, test_policy_ft_eap_and_opportunistic_suite_b)
{
	zassert_true(supplicant_pmksa_policy_allows(false, WPA_KEY_MGMT_IEEE8021X), NULL);
	zassert_false(supplicant_pmksa_policy_allows(false, WPA_KEY_MGMT_FT_IEEE8021X), NULL);
	zassert_true(supplicant_pmksa_policy_allows(true, WPA_KEY_MGMT_FT_IEEE8021X), NULL);
}

ZTEST(pmksa_hostap, test_indexed_query_filters_and_counts)
{
	struct os_reltime now = {.sec = 100};
	struct rsn_pmksa_cache_entry entries[4] = {0};
	struct wifi_pmksa_cache_query query = {0};

	for (size_t i = 0; i < ARRAY_SIZE(entries); i++) {
		entries[i].network_ctx = &test_network;
		entries[i].expiration = 200;
		entries[i].reauth_time = 150;
		entries[i].akmp = WPA_KEY_MGMT_IEEE8021X;
		entries[i].pmk_len = 32;
		memcpy(entries[i].aa, aa, sizeof(aa));
		memcpy(entries[i].spa, spa, sizeof(spa));
		entries[i].aa[5] += i;
		entries[i].pmk[0] = i;
		entries[i].next = i + 1 < ARRAY_SIZE(entries) ? &entries[i + 1] : NULL;
	}
	entries[1].network_ctx = &other_network;
	entries[2].expiration = 100;

	query.index = 2;
	zassert_equal(supplicant_pmksa_query_entries(entries, &test_network, false, &now, &query),
		      -ENOENT, NULL);
	zassert_equal(query.entry_count, 2, NULL);
	zassert_equal(query.entry.pmk_len, 0, NULL);
	query.index = 1;
	zassert_equal(supplicant_pmksa_query_entries(entries, &test_network, false, &now, &query),
		      0, NULL);
	zassert_equal(query.entry_count, 2, NULL);
	zassert_equal(query.entry.pmk[0], 3, NULL);
}

#if defined(CONFIG_WIFI_MGMT_PMKSA_IMPORT)
ZTEST(pmksa_hostap, test_usage_classifier)
{
	struct rsn_pmksa_cache_entry current = {0};

	zassert_equal(supplicant_pmksa_usage_result(false, false, NULL),
		      WIFI_PMKSA_CACHE_USAGE_NOT_ATTEMPTED, NULL);
	current.external = true;
	zassert_equal(supplicant_pmksa_usage_result(true, true, &current),
		      WIFI_PMKSA_CACHE_USAGE_HIT, NULL);
	current.external = false;
	zassert_equal(supplicant_pmksa_usage_result(true, true, &current),
		      WIFI_PMKSA_CACHE_USAGE_MISS, NULL);
	zassert_equal(supplicant_pmksa_usage_result(true, false, &current),
		      WIFI_PMKSA_CACHE_USAGE_UNKNOWN, NULL);
}

ZTEST(pmksa_hostap, test_import_skips_unusable_entries)
{
	static struct wifi_pmksa_cache_entry entries[6];
	static struct wpa_supplicant wpa_s;
	static struct wpa_ssid ssid;
	static struct wpa_sm sm;
	struct rsn_pmksa_cache_entry *entry;
	size_t imported;
	size_t cached = 0U;

	zassert_equal(eloop_init(), 0, NULL);
	sm.pmksa = pmksa_cache_init(NULL, NULL, NULL, NULL, NULL);
	zassert_not_null(sm.pmksa, NULL);
	wpa_s.wpa = &sm;
	memcpy(wpa_s.own_addr, spa, sizeof(spa));

	entries[0] = wifi_entry(WIFI_AKM_SUITE_802_1X, 32, false);
	entries[1] = wifi_entry(WIFI_AKM_SUITE_FT_PSK_SHA384, 32, false);
	entries[1].bssid[5]++;
	entries[2] = wifi_entry(WIFI_AKM_SUITE_PSK, 32, false);
	entries[2].bssid[5] += 2U;
	entries[3] = wifi_entry(WIFI_AKM_SUITE_802_1X, 0, false);
	entries[3].bssid[5] += 3U;
	entries[4] = wifi_entry(WIFI_AKM_SUITE_802_1X, 32, false);
	entries[4].bssid[5] += 4U;
	entries[4].spa[5]++;
	entries[5] = entries[0];

	imported = supplicant_pmksa_import_entries(&wpa_s, &ssid, entries, ARRAY_SIZE(entries));
	zassert_equal(imported, 3U, NULL);

	for (entry = wpa_sm_pmksa_cache_head(&sm); entry != NULL; entry = entry->next) {
		zassert_true(entry->external, NULL);
		zassert_equal(entry->network_ctx, &ssid, NULL);
		cached++;
	}
	zassert_equal(cached, 2U, NULL);
	zassert_equal(supplicant_pmksa_import_entries(NULL, &ssid, entries, 1U), 0U, NULL);

	pmksa_cache_deinit(sm.pmksa);
	eloop_destroy();
}

ZTEST(pmksa_hostap, test_import_reuses_identical_native_entry)
{
	static struct wifi_pmksa_cache_entry source;
	static struct wpa_supplicant wpa_s;
	static struct wpa_ssid ssid;
	static struct wpa_sm sm;
	struct rsn_pmksa_cache_entry *original;
	size_t imported;

	zassert_equal(eloop_init(), 0, NULL);
	sm.pmksa = pmksa_cache_init(NULL, NULL, NULL, NULL, NULL);
	zassert_not_null(sm.pmksa, NULL);
	wpa_s.wpa = &sm;
	memcpy(wpa_s.own_addr, spa, sizeof(spa));
	source = wifi_entry(WIFI_AKM_SUITE_802_1X, 32, false);

	imported = supplicant_pmksa_import_entries(&wpa_s, &ssid, &source, 1U);
	zassert_equal(imported, 1U, NULL);
	original = wpa_sm_pmksa_cache_head(&sm);
	zassert_not_null(original, NULL);

	imported = supplicant_pmksa_import_entries(&wpa_s, &ssid, &source, 1U);
	zassert_equal(imported, 1U, NULL);
	zassert_equal_ptr(wpa_sm_pmksa_cache_head(&sm), original,
			  "Identical import replaced the native cache entry");

	pmksa_cache_deinit(sm.pmksa);
	eloop_destroy();
}
#endif

ZTEST(pmksa_hostap, test_event_parser_is_strict)
{
	static const uint8_t expected_bssid[ETH_ALEN] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	struct wifi_pmksa_cache_event event;
	enum net_event_wifi_cmd command;
	int network_id;

	zassert_equal(supplicant_pmksa_event_command("PMKSA-CACHE-ADDED", &command), 0, NULL);
	zassert_equal(command, NET_EVENT_WIFI_CMD_PMKSA_CACHE_ADDED, NULL);
	zassert_equal(supplicant_pmksa_event_command("PMKSA-CACHE-REMOVED", &command), 0, NULL);
	zassert_equal(command, NET_EVENT_WIFI_CMD_PMKSA_CACHE_REMOVED, NULL);
	memset(&event, 0xa5, sizeof(event));
	zassert_equal(supplicant_pmksa_event_command("UNKNOWN", &command), -EINVAL, NULL);

	zassert_equal(supplicant_pmksa_parse_event("PMKSA-CACHE-ADDED aa:bb:cc:dd:ee:ff 7", &event,
						   &network_id),
		      0, NULL);
	zassert_equal(network_id, 7, NULL);
	zassert_mem_equal(event.bssid, expected_bssid, ETH_ALEN, NULL);
	zassert_equal(event.ssid_length, 0U, NULL);
	zassert_equal(supplicant_pmksa_parse_event("PMKSA-CACHE-REMOVED 00:11:22:33:44:55 2",
						   &event, &network_id),
		      0, NULL);
	zassert_equal(supplicant_pmksa_parse_event("PMKSA-CACHE-ADDED aa:bb:cc:dd:ee", &event,
						   &network_id),
		      -EINVAL, NULL);
	zassert_equal(supplicant_pmksa_parse_event("PMKSA-CACHE-ADDED aa:bb:cc:dd:ee:ff 7 extra",
						   &event, &network_id),
		      -EINVAL, NULL);
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_PMKSA_CACHE */

ZTEST_SUITE(pmksa_hostap, NULL, NULL, NULL, NULL, NULL);
