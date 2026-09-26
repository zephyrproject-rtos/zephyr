/*
 * Copyright (c) 2026 Siddhant Modi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "supp_pmksa.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "common/wpa_common.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "wpa.h"
#include "pmksa_cache.h"

#define PMKSA_CACHE_ADDED_EVENT   "PMKSA-CACHE-ADDED"
#define PMKSA_CACHE_REMOVED_EVENT "PMKSA-CACHE-REMOVED"

static int supplicant_pmksa_akm_to_hostap(enum wifi_akm_suite akm, int *akmp)
{
	if (akmp == NULL) {
		return -EINVAL;
	}

	switch (akm) {
	case WIFI_AKM_SUITE_802_1X:
		*akmp = WPA_KEY_MGMT_IEEE8021X;
		break;
	case WIFI_AKM_SUITE_PSK:
		*akmp = WPA_KEY_MGMT_PSK;
		break;
	case WIFI_AKM_SUITE_FT_802_1X:
		*akmp = WPA_KEY_MGMT_FT_IEEE8021X;
		break;
	case WIFI_AKM_SUITE_FT_PSK:
		*akmp = WPA_KEY_MGMT_FT_PSK;
		break;
	case WIFI_AKM_SUITE_802_1X_SHA256:
		*akmp = WPA_KEY_MGMT_IEEE8021X_SHA256;
		break;
	case WIFI_AKM_SUITE_PSK_SHA256:
		*akmp = WPA_KEY_MGMT_PSK_SHA256;
		break;
	case WIFI_AKM_SUITE_SAE:
		*akmp = WPA_KEY_MGMT_SAE;
		break;
	case WIFI_AKM_SUITE_FT_SAE:
		*akmp = WPA_KEY_MGMT_FT_SAE;
		break;
	case WIFI_AKM_SUITE_802_1X_SUITE_B:
		*akmp = WPA_KEY_MGMT_IEEE8021X_SUITE_B;
		break;
	case WIFI_AKM_SUITE_802_1X_SUITE_B_192:
		*akmp = WPA_KEY_MGMT_IEEE8021X_SUITE_B_192;
		break;
	case WIFI_AKM_SUITE_FT_802_1X_SHA384:
		*akmp = WPA_KEY_MGMT_FT_IEEE8021X_SHA384;
		break;
	case WIFI_AKM_SUITE_FILS_SHA256:
		*akmp = WPA_KEY_MGMT_FILS_SHA256;
		break;
	case WIFI_AKM_SUITE_FILS_SHA384:
		*akmp = WPA_KEY_MGMT_FILS_SHA384;
		break;
	case WIFI_AKM_SUITE_FT_FILS_SHA256:
		*akmp = WPA_KEY_MGMT_FT_FILS_SHA256;
		break;
	case WIFI_AKM_SUITE_FT_FILS_SHA384:
		*akmp = WPA_KEY_MGMT_FT_FILS_SHA384;
		break;
	case WIFI_AKM_SUITE_OWE:
		*akmp = WPA_KEY_MGMT_OWE;
		break;
	case WIFI_AKM_SUITE_802_1X_SHA384:
		*akmp = WPA_KEY_MGMT_IEEE8021X_SHA384;
		break;
	case WIFI_AKM_SUITE_SAE_EXT_KEY:
		*akmp = WPA_KEY_MGMT_SAE_EXT_KEY;
		break;
	case WIFI_AKM_SUITE_FT_SAE_EXT_KEY:
		*akmp = WPA_KEY_MGMT_FT_SAE_EXT_KEY;
		break;
	case WIFI_AKM_SUITE_DPP:
		*akmp = WPA_KEY_MGMT_DPP;
		break;
	default:
		return -EPROTONOSUPPORT;
	}

	return 0;
}

static bool supplicant_pmksa_is_suite_b(int akmp)
{
	return akmp == WPA_KEY_MGMT_IEEE8021X_SUITE_B || akmp == WPA_KEY_MGMT_IEEE8021X_SUITE_B_192;
}

static bool supplicant_pmksa_is_ft_eap(int akmp)
{
	return akmp == WPA_KEY_MGMT_FT_IEEE8021X || akmp == WPA_KEY_MGMT_FT_IEEE8021X_SHA384;
}

static int supplicant_pmksa_check_pmk_len(int akmp, size_t pmk_len)
{
	bool sha384 = akmp == WPA_KEY_MGMT_IEEE8021X_SUITE_B_192 ||
		      akmp == WPA_KEY_MGMT_FILS_SHA384 || akmp == WPA_KEY_MGMT_FT_FILS_SHA384 ||
		      akmp == WPA_KEY_MGMT_FT_IEEE8021X_SHA384 ||
		      akmp == WPA_KEY_MGMT_IEEE8021X_SHA384;
	bool variable_length = akmp == WPA_KEY_MGMT_OWE || akmp == WPA_KEY_MGMT_SAE_EXT_KEY ||
			       akmp == WPA_KEY_MGMT_FT_SAE_EXT_KEY || akmp == WPA_KEY_MGMT_DPP;

	if (variable_length) {
		return pmk_len == 32U || pmk_len == 48U || pmk_len == 64U ? 0 : -EPROTONOSUPPORT;
	}

	if (sha384) {
		return pmk_len == 48U ? 0 : -EPROTONOSUPPORT;
	}

	return pmk_len == 32U ? 0 : -EPROTONOSUPPORT;
}

bool supplicant_pmksa_policy_allows(bool ft_eap_pmksa_caching, int akmp)
{
	return !supplicant_pmksa_is_ft_eap(akmp) || ft_eap_pmksa_caching;
}

static int supplicant_pmksa_add_time(os_time_t now, uint32_t delta, os_time_t *result)
{
	if (result == NULL || now < 0 || (uint64_t)now > INT64_MAX - delta) {
		return -EINVAL;
	}

	*result = now + delta;
	return 0;
}

static int supplicant_pmksa_remaining_time(os_time_t expiration, os_time_t now, uint32_t *remaining)
{
	if (remaining == NULL || expiration <= now) {
		return -ENOENT;
	}

	if ((uint64_t)(expiration - now) > UINT32_MAX) {
		return -EPROTONOSUPPORT;
	}

	*remaining = expiration - now;
	return 0;
}

int supplicant_pmksa_entry_from_wifi(const struct wifi_pmksa_cache_entry *source, void *network_ctx,
				     bool ft_eap_pmksa_caching, const uint8_t *expected_spa,
				     const struct os_reltime *now,
				     struct rsn_pmksa_cache_entry *destination)
{
	int akmp;

	if (source == NULL || network_ctx == NULL || expected_spa == NULL || now == NULL ||
	    destination == NULL) {
		return -EINVAL;
	}
	if (is_zero_ether_addr(source->bssid) || is_multicast_ether_addr(source->bssid) ||
	    memcmp(source->spa, expected_spa, ETH_ALEN) != 0 ||
	    source->pmk_len > WIFI_PMKSA_PMK_MAX_LEN || source->expiration_remaining_s == 0U ||
	    source->reauth_remaining_s > source->expiration_remaining_s) {
		return -EINVAL;
	}
	if (supplicant_pmksa_akm_to_hostap(source->akm, &akmp) != 0 ||
	    !supplicant_pmksa_policy_allows(ft_eap_pmksa_caching, akmp) ||
	    (source->opportunistic && supplicant_pmksa_is_suite_b(akmp))) {
		return -EPROTONOSUPPORT;
	}
	if (supplicant_pmksa_check_pmk_len(akmp, source->pmk_len) != 0) {
		return -EPROTONOSUPPORT;
	}

	memset(destination, 0, sizeof(*destination));
	if (supplicant_pmksa_add_time(now->sec, source->expiration_remaining_s,
				      &destination->expiration) != 0 ||
	    supplicant_pmksa_add_time(now->sec, source->reauth_remaining_s,
				      &destination->reauth_time) != 0) {
		memset(destination, 0, sizeof(*destination));
		return -EINVAL;
	}

	memcpy(destination->aa, source->bssid, ETH_ALEN);
	memcpy(destination->spa, source->spa, ETH_ALEN);
	memcpy(destination->pmkid, source->pmkid, PMKID_LEN);
	memcpy(destination->pmk, source->pmk, source->pmk_len);
	destination->pmk_len = source->pmk_len;
	destination->akmp = akmp;
	destination->network_ctx = network_ctx;
	destination->external = true;
	destination->fils_cache_id_set = source->fils_cache_id_set;
	if (source->fils_cache_id_set) {
		memcpy(destination->fils_cache_id, source->fils_cache_id, FILS_CACHE_ID_LEN);
	}
	destination->opportunistic = source->opportunistic;

	return 0;
}

int supplicant_pmksa_entry_to_wifi(const struct rsn_pmksa_cache_entry *source,
				   const void *network_ctx, bool ft_eap_pmksa_caching,
				   const struct os_reltime *now,
				   struct wifi_pmksa_cache_entry *destination)
{
	uint32_t akm;
	uint32_t expiration;
	int ret;

	if (destination == NULL) {
		return -EINVAL;
	}
	wifi_pmksa_cache_entries_clear(destination, 1U);
	if (source == NULL || network_ctx == NULL || now == NULL) {
		return -EINVAL;
	}
	if (source->network_ctx != network_ctx || source->expiration <= now->sec) {
		return -ENOENT;
	}
	akm = wpa_akm_to_suite(source->akmp);
	if (akm == 0U || !supplicant_pmksa_policy_allows(ft_eap_pmksa_caching, source->akmp) ||
	    (source->opportunistic && supplicant_pmksa_is_suite_b(source->akmp))) {
		return -EPROTONOSUPPORT;
	}
	ret = supplicant_pmksa_check_pmk_len(source->akmp, source->pmk_len);
	if (ret != 0) {
		return ret;
	}
	ret = supplicant_pmksa_remaining_time(source->expiration, now->sec, &expiration);
	if (ret != 0) {
		return ret;
	}

	destination->expiration_remaining_s = expiration;
	destination->reauth_remaining_s =
		source->reauth_time <= now->sec
			? 0U
			: (uint32_t)MIN((uint64_t)(source->reauth_time - now->sec), UINT32_MAX);
	destination->reauth_remaining_s = MIN(destination->reauth_remaining_s, expiration);
	destination->akm = (enum wifi_akm_suite)akm;
	destination->pmk_len = source->pmk_len;
	memcpy(destination->pmkid, source->pmkid, PMKID_LEN);
	memcpy(destination->pmk, source->pmk, source->pmk_len);
	memcpy(destination->bssid, source->aa, ETH_ALEN);
	memcpy(destination->spa, source->spa, ETH_ALEN);
	destination->fils_cache_id_set = source->fils_cache_id_set;
	if (source->fils_cache_id_set) {
		memcpy(destination->fils_cache_id, source->fils_cache_id, FILS_CACHE_ID_LEN);
	}
	destination->opportunistic = source->opportunistic;

	return 0;
}

static bool supplicant_pmksa_entry_is_eligible(const struct rsn_pmksa_cache_entry *entry,
					       const void *network_ctx, bool ft_eap_pmksa_caching,
					       const struct os_reltime *now)
{
	return entry != NULL && entry->network_ctx == network_ctx && entry->expiration > now->sec &&
	       wpa_akm_to_suite(entry->akmp) != 0U &&
	       supplicant_pmksa_policy_allows(ft_eap_pmksa_caching, entry->akmp) &&
	       !(entry->opportunistic && supplicant_pmksa_is_suite_b(entry->akmp)) &&
	       supplicant_pmksa_check_pmk_len(entry->akmp, entry->pmk_len) == 0;
}

int supplicant_pmksa_query_entries(const struct rsn_pmksa_cache_entry *head,
				   const void *network_ctx, bool ft_eap_pmksa_caching,
				   const struct os_reltime *now,
				   struct wifi_pmksa_cache_query *query)
{
	const struct rsn_pmksa_cache_entry *entry;
	uint32_t eligible = 0U;
	int ret = -ENOENT;

	if (query == NULL) {
		return -EINVAL;
	}
	wifi_pmksa_cache_entries_clear(&query->entry, 1U);
	query->entry_count = 0U;
	if (network_ctx == NULL || now == NULL) {
		return -EINVAL;
	}

	for (entry = head; entry != NULL; entry = entry->next) {
		if (!supplicant_pmksa_entry_is_eligible(entry, network_ctx, ft_eap_pmksa_caching,
							now)) {
			continue;
		}
		if (eligible == query->index) {
			ret = supplicant_pmksa_entry_to_wifi(
				entry, network_ctx, ft_eap_pmksa_caching, now, &query->entry);
		}
		eligible++;
	}
	query->entry_count = eligible;
	return query->index < eligible ? ret : -ENOENT;
}

int supplicant_pmksa_event_command(const char *text, enum net_event_wifi_cmd *event)
{
	if (text == NULL || event == NULL) {
		return -EINVAL;
	}

	if (strncmp(text, PMKSA_CACHE_ADDED_EVENT, sizeof(PMKSA_CACHE_ADDED_EVENT) - 1U) == 0) {
		*event = NET_EVENT_WIFI_CMD_PMKSA_CACHE_ADDED;
		return 0;
	}
	if (strncmp(text, PMKSA_CACHE_REMOVED_EVENT, sizeof(PMKSA_CACHE_REMOVED_EVENT) - 1U) == 0) {
		*event = NET_EVENT_WIFI_CMD_PMKSA_CACHE_REMOVED;
		return 0;
	}

	return -EINVAL;
}

int supplicant_pmksa_parse_event(const char *text, struct wifi_pmksa_cache_event *event,
				 int *network_id)
{
	char type[32];
	char bssid[18];
	int id;
	int consumed;

	if (text == NULL || event == NULL || network_id == NULL) {
		return -EINVAL;
	}

	memset(event, 0, sizeof(*event));
	if (sscanf(text, "%31s %17s %d %n", type, bssid, &id, &consumed) != 3 ||
	    text[consumed] != '\0' || id < 0 ||
	    (strcmp(type, PMKSA_CACHE_ADDED_EVENT) != 0 &&
	     strcmp(type, PMKSA_CACHE_REMOVED_EVENT) != 0) ||
	    hwaddr_aton(bssid, event->bssid) != 0) {
		return -EINVAL;
	}

	*network_id = id;
	return 0;
}

#if defined(CONFIG_WIFI_MGMT_PMKSA_IMPORT)
enum wifi_pmksa_cache_usage
supplicant_pmksa_usage_result(bool entries_supplied, bool connection_succeeded,
			      const struct rsn_pmksa_cache_entry *current)
{
	if (!entries_supplied) {
		return WIFI_PMKSA_CACHE_USAGE_NOT_ATTEMPTED;
	}
	if (!connection_succeeded) {
		return WIFI_PMKSA_CACHE_USAGE_UNKNOWN;
	}
	return current != NULL && current->external ? WIFI_PMKSA_CACHE_USAGE_HIT
						    : WIFI_PMKSA_CACHE_USAGE_MISS;
}

static void supplicant_pmksa_free_entry(struct rsn_pmksa_cache_entry *entry)
{
	if (entry != NULL) {
		bin_clear_free(entry, sizeof(*entry));
	}
}

size_t supplicant_pmksa_import_entries(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
				       const struct wifi_pmksa_cache_entry *entries,
				       size_t entry_count)
{
	struct rsn_pmksa_cache_entry *converted;
	struct os_reltime now;
	size_t imported = 0U;
	size_t i;

	if (wpa_s == NULL || wpa_s->wpa == NULL || ssid == NULL ||
	    (entry_count != 0U && entries == NULL)) {
		return 0U;
	}
	if (wpa_sm_get_pmksa_cache(wpa_s->wpa) == NULL) {
		return 0U;
	}

	os_get_reltime(&now);
	for (i = 0; i < entry_count; i++) {
		converted = os_zalloc(sizeof(*converted));
		if (converted == NULL) {
			break;
		}
		if (supplicant_pmksa_entry_from_wifi(&entries[i], ssid, ssid->ft_eap_pmksa_caching,
						     wpa_s->own_addr, &now, converted) != 0) {
			supplicant_pmksa_free_entry(converted);
			continue;
		}

		if (wpa_sm_pmksa_cache_add_entry(wpa_s->wpa, converted) != NULL) {
			imported++;
		}
	}

	return imported;
}

void supplicant_pmksa_flush_external_locked(struct wpa_supplicant *wpa_s, void *network_ctx)
{
	if (wpa_s != NULL && wpa_s->wpa != NULL) {
		wpa_sm_external_pmksa_cache_flush(wpa_s->wpa, network_ctx);
	}
}
#endif /* CONFIG_WIFI_MGMT_PMKSA_IMPORT */
