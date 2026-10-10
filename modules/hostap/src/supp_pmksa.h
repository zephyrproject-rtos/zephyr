/*
 * Copyright (c) 2026 Siddhant Modi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUPP_PMKSA_H
#define ZEPHYR_SUPP_PMKSA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/wifi_mgmt.h>

struct os_reltime;
struct rsn_pmksa_cache_entry;
struct wpa_ssid;
struct wpa_supplicant;

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_PMKSA_CACHE)
/** Convert a Zephyr PMKSA entry to hostap representation. */
int supplicant_pmksa_entry_from_wifi(const struct wifi_pmksa_cache_entry *source, void *network_ctx,
				     bool ft_eap_pmksa_caching, const uint8_t *expected_spa,
				     const struct os_reltime *now,
				     struct rsn_pmksa_cache_entry *destination);

/** Convert a hostap PMKSA entry to Zephyr representation. */
int supplicant_pmksa_entry_to_wifi(const struct rsn_pmksa_cache_entry *source,
				   const void *network_ctx, bool ft_eap_pmksa_caching,
				   const struct os_reltime *now,
				   struct wifi_pmksa_cache_entry *destination);

/** Return an indexed exportable PMKSA entry. */
int supplicant_pmksa_query_entries(const struct rsn_pmksa_cache_entry *head,
				   const void *network_ctx, bool ft_eap_pmksa_caching,
				   const struct os_reltime *now,
				   struct wifi_pmksa_cache_query *query);

/** Check whether a profile permits exporting an AKM. */
bool supplicant_pmksa_policy_allows(bool ft_eap_pmksa_caching, int akmp);

/** Map a hostap PMKSA notification to a Wi-Fi event command. */
int supplicant_pmksa_event_command(const char *text, enum net_event_wifi_cmd *event);

/** Parse a hostap PMKSA notification. */
int supplicant_pmksa_parse_event(const char *text, struct wifi_pmksa_cache_event *event,
				 int *network_id);

#if defined(CONFIG_WIFI_MGMT_PMKSA_IMPORT)
/** Classify use of an externally supplied PMKSA entry. */
enum wifi_pmksa_cache_usage
supplicant_pmksa_usage_result(bool entries_supplied, bool connection_succeeded,
			      const struct rsn_pmksa_cache_entry *current);

/**
 * Import usable PMKSA entries for a profile.
 *
 * @return Number of entries imported.
 */
size_t supplicant_pmksa_import_entries(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
				       const struct wifi_pmksa_cache_entry *entries,
				       size_t entry_count);

/** Flush externally supplied PMKSA entries while holding the supplicant lock. */
void supplicant_pmksa_flush_external_locked(struct wpa_supplicant *wpa_s, void *network_ctx);

/** Record PMKSA use for a completed connection attempt. */
void supplicant_pmksa_connection_result(struct wpa_supplicant *wpa_s, enum wifi_conn_status status);
#endif

#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_PMKSA_CACHE */

#endif /* ZEPHYR_SUPP_PMKSA_H */
