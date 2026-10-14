/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_credentials.h>
#include <sys/types.h>

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

static K_MUTEX_DEFINE(wifi_credentials_mutex);

/* SSID cache: maps SSIDs to their storage indices */
static char ssid_cache[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES][WIFI_SSID_MAX_LEN];
static size_t ssid_cache_lengths[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES];

/* Whether the stored entries have been loaded into the SSID cache. They are loaded on first
 * use, because the storage used by the backend may only become usable late during boot.
 */
static bool entries_loaded;

/**
 * @brief Finds index of given SSID if it exists.
 *
 * @param ssid		SSID to look for (buffer of WIFI_SSID_MAX_LEN length)
 * @return		index if entry is found, -1 otherwise
 */
static inline ssize_t lookup_idx(const uint8_t *ssid, size_t ssid_len)
{
	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (ssid_len != ssid_cache_lengths[i]) {
			continue;
		}

		if (strncmp(ssid, ssid_cache[i], ssid_len) == 0) {
			return i;
		}
	}

	return -1;
}

/**
 * @brief Determine whether an index is currently used for storing network credentials.
 *
 * @param idx credential index
 * @return true if index is used, false otherwise
 */
static inline bool is_entry_used(size_t idx)
{
	return ssid_cache_lengths[idx] != 0;
}

/**
 * @brief Finds unused index to store new entry at.
 *
 * @return		index if empty slot is found, -1 otherwise
 */
static inline ssize_t lookup_unused_idx(void)
{
	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (!is_entry_used(i)) {
			return i;
		}
	}

	return -1;
}

/**
 * @brief Load the stored entries into the SSID cache if not done yet.
 *
 * Must be called with wifi_credentials_mutex held. If loading fails, it is retried on next use.
 *
 * @return		0 if the entries are loaded, -EACCES otherwise
 */
static int load_entries(void)
{
	int ret;

	if (entries_loaded) {
		return 0;
	}

	ret = wifi_credentials_backend_init();
	if (ret != 0) {
		LOG_ERR("Loading WiFi credentials from storage backend failed, err: %d", ret);
		/* Do not keep the entries cached before the failure. */
		memset(ssid_cache_lengths, 0, sizeof(ssid_cache_lengths));
		return -EACCES;
	}

	entries_loaded = true;

	return 0;
}

void wifi_credentials_cache_ssid(size_t idx, const struct wifi_credentials_header *buf)
{
	memcpy(ssid_cache[idx], buf->ssid, buf->ssid_len);
	ssid_cache_lengths[idx] = buf->ssid_len;
}

/**
 * @brief Clear entry in SSID cache.
 *
 * @param idx credential index
 */
void wifi_credentials_uncache_ssid(size_t idx)
{
	ssid_cache_lengths[idx] = 0;
}

int wifi_credentials_get_by_ssid_personal_struct(const char *ssid, size_t ssid_len,
						 struct wifi_credentials_personal *buf)
{
	ssize_t idx;
	int ret;

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "retrieve", "SSID has invalid format");
		return -EINVAL;
	}

	if (buf == NULL) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "retrieve", "destination is NULL");
		return -EINVAL;
	}

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	ret = load_entries();
	if (ret != 0) {
		goto exit;
	}

	idx = lookup_idx(ssid, ssid_len);
	if (idx == -1) {
		LOG_DBG("Cannot %s WiFi credentials, %s", "retrieve", "SSID not found");
		ret = -ENOENT;
		goto exit;
	}

	ret = wifi_credentials_load_entry(idx, buf, sizeof(struct wifi_credentials_personal));
	if (ret) {
		LOG_ERR("Failed to %s WiFi credentials at index %d, err: %d", "load", idx, ret);
		goto exit;
	}

	if (buf->header.type < 0 || buf->header.type > WIFI_SECURITY_TYPE_MAX) {
		LOG_ERR("Requested WiFi credentials entry is corrupted");
		ret = -EPROTO;
		goto exit;
	}

exit:
	k_mutex_unlock(&wifi_credentials_mutex);

	return ret;
}

int wifi_credentials_set_personal_struct(const struct wifi_credentials_personal *creds)
{
	ssize_t idx;
	int ret;

	if (creds == NULL) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "set", "credential not set");
		return -EINVAL;
	}

	if (creds->header.ssid_len > WIFI_SSID_MAX_LEN || creds->header.ssid_len == 0) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "set", "SSID has invalid format");
		return -EINVAL;
	}

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	/* Do not store before the entries are loaded, as it could overwrite a stored entry. */
	ret = load_entries();
	if (ret != 0) {
		goto exit;
	}

	idx = lookup_idx(creds->header.ssid, creds->header.ssid_len);
	if (idx == -1) {
		idx = lookup_unused_idx();
		if (idx == -1) {
			LOG_ERR("Cannot %s WiFi credentials, %s", "store", "no space left");
			ret = -ENOBUFS;
			goto exit;
		}
	}

	ret = wifi_credentials_store_entry(idx, creds, sizeof(struct wifi_credentials_personal));
	if (ret) {
		LOG_ERR("Failed to %s WiFi credentials at index %d, err: %d", "store", idx, ret);
		goto exit;
	}

	wifi_credentials_cache_ssid(idx, &creds->header);

exit:
	k_mutex_unlock(&wifi_credentials_mutex);

	return ret;
}

int wifi_credentials_set_personal(const char *ssid, size_t ssid_len, enum wifi_security_type type,
				  const uint8_t *bssid, size_t bssid_len, const char *password,
				  size_t password_len, uint32_t flags, uint8_t channel,
				  uint32_t timeout)
{
	int ret = 0;
	uint8_t buf[ENTRY_MAX_LEN] = {0};
	struct wifi_credentials_header *header;

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "set", "SSID has invalid format");
		return -EINVAL;
	}

	if (flags & WIFI_CREDENTIALS_FLAG_BSSID &&
	    (bssid_len != WIFI_MAC_ADDR_LEN || bssid == NULL)) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "set",
			"provided flags indicated BSSID but no BSSID provided");
		return -EINVAL;
	}

	if (((type != WIFI_SECURITY_TYPE_NONE && type != WIFI_SECURITY_TYPE_OWE) &&
		(password_len == 0 || password == NULL)) ||
	    (password_len > WIFI_CREDENTIALS_MAX_PASSWORD_LEN)) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "set",
			"password not provided or invalid");
		return -EINVAL;
	}

	/* pack entry */
	header = (struct wifi_credentials_header *)buf;

	header->type = type;
	memcpy(header->ssid, ssid, ssid_len);
	header->ssid_len = ssid_len;
	header->flags = flags;
	header->channel = channel;
	header->timeout = timeout;

	if (flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		memcpy(header->bssid, bssid, WIFI_MAC_ADDR_LEN);
	}

	switch (type) {
	case WIFI_SECURITY_TYPE_NONE:
		break;
	case WIFI_SECURITY_TYPE_OWE:
		break;
	case WIFI_SECURITY_TYPE_PSK:
	case WIFI_SECURITY_TYPE_PSK_SHA256:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_SAE: {
		struct wifi_credentials_personal *header_personal =
			(struct wifi_credentials_personal *)buf;

		memcpy(header_personal->password, password, password_len);
		header_personal->password_len = password_len;
		break;
	}
	default:
		LOG_ERR("Cannot %s WiFi credentials, provided security type %d is unsupported",
			"set", type);
		return -ENOTSUP;
	}

	/* store entry */
	ret = wifi_credentials_set_personal_struct((struct wifi_credentials_personal *)buf);

	return ret;
}

int wifi_credentials_get_by_ssid_personal(const char *ssid, size_t ssid_len,
					  enum wifi_security_type *type, uint8_t *bssid_buf,
					  size_t bssid_buf_len, char *password_buf,
					  size_t password_buf_len, size_t *password_len,
					  uint32_t *flags, uint8_t *channel, uint32_t *timeout)
{
	int ret = 0;
	uint8_t buf[ENTRY_MAX_LEN] = {0};
	struct wifi_credentials_header *header;

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "retrieve", "SSID has invalid format");
		return -EINVAL;
	}

	if (bssid_buf_len != WIFI_MAC_ADDR_LEN || bssid_buf == NULL) {
		LOG_ERR("%s buffer needs to be provided", "BSSID");
		return -EINVAL;
	}

	if (password_buf == NULL || password_buf_len > WIFI_CREDENTIALS_MAX_PASSWORD_LEN ||
	    password_buf_len == 0) {
		LOG_ERR("%s buffer needs to be provided", "WiFi password");
		return -EINVAL;
	}

	/* load entry */
	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len,
							   (struct wifi_credentials_personal *)buf);
	if (ret) {
		return ret;
	}

	/* unpack entry*/
	header = (struct wifi_credentials_header *)buf;

	*type = header->type;
	*flags = header->flags;
	*channel = header->channel;
	*timeout = header->timeout;

	if (header->flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		memcpy(bssid_buf, header->bssid, WIFI_MAC_ADDR_LEN);
	}

	switch (header->type) {
	case WIFI_SECURITY_TYPE_NONE:
		break;
	case WIFI_SECURITY_TYPE_OWE:
		break;
	case WIFI_SECURITY_TYPE_PSK:
	case WIFI_SECURITY_TYPE_PSK_SHA256:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_SAE: {
		struct wifi_credentials_personal *header_personal =
			(struct wifi_credentials_personal *)buf;

		memcpy(password_buf, header_personal->password, header_personal->password_len);
		*password_len = header_personal->password_len;
		break;
	}
	default:
		LOG_ERR("Cannot %s WiFi credentials, %s", "get",
			"the requested credentials have invalid WIFI_SECURITY_TYPE");
		ret = -EPROTO;
	}
	return ret;
}

int wifi_credentials_delete_by_ssid(const char *ssid, size_t ssid_len)
{
	ssize_t idx;
	int ret = 0;

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot %s WiFi credentials, %s", "delete", "SSID has invalid format");
		return -EINVAL;
	}

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	ret = load_entries();
	if (ret != 0) {
		goto exit;
	}

	idx = lookup_idx(ssid, ssid_len);
	if (idx == -1) {
		LOG_DBG("WiFi credentials entry was not found");
		ret = -ENOENT;
		goto exit;
	}

	ret = wifi_credentials_delete_entry(idx);
	if (ret) {
		LOG_ERR("Failed to %s WiFi credentials index %d, err: %d", "delete", idx, ret);
		goto exit;
	}

	wifi_credentials_uncache_ssid(idx);

exit:
	k_mutex_unlock(&wifi_credentials_mutex);
	return ret;
}

void wifi_credentials_for_each_ssid(wifi_credentials_ssid_cb cb, void *cb_arg)
{
	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	(void)load_entries();

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (is_entry_used(i)) {
			cb(cb_arg, ssid_cache[i], ssid_cache_lengths[i]);
		}
	}

	k_mutex_unlock(&wifi_credentials_mutex);
}

bool wifi_credentials_is_empty(void)
{
	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	(void)load_entries();

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (is_entry_used(i)) {
			k_mutex_unlock(&wifi_credentials_mutex);
			return false;
		}
	}

	k_mutex_unlock(&wifi_credentials_mutex);
	return true;
}

int wifi_credentials_delete_all(void)
{
	int ret = 0;

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	ret = load_entries();
	if (ret != 0) {
		k_mutex_unlock(&wifi_credentials_mutex);
		return ret;
	}

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (is_entry_used(i)) {
			ret = wifi_credentials_delete_entry(i);
			if (ret) {
				LOG_ERR("Failed to %s WiFi credentials index %d, err: %d",
					"delete", i, ret);
				break;
			}

			wifi_credentials_uncache_ssid(i);
		}
	}

	k_mutex_unlock(&wifi_credentials_mutex);
	return ret;
}
