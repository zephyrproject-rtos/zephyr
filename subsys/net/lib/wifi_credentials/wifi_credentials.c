/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/wifi_credentials.h>
#include <sys/types.h>

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

static K_MUTEX_DEFINE(wifi_credentials_mutex);

/* SSID cache: maps SSIDs to their storage indices */
static char ssid_cache[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES][WIFI_SSID_MAX_LEN];
static size_t ssid_cache_lengths[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES];

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

static int init(void)
{

	int ret;

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	ret = wifi_credentials_backend_init();
	if (ret) {
		LOG_ERR("Initializing WiFi credentials storage backend failed, err: %d", ret);
	}

	k_mutex_unlock(&wifi_credentials_mutex);

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
	int ret;

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot retrieve WiFi credentials, SSID has invalid format");
		return -EINVAL;
	}

	if (buf == NULL) {
		LOG_ERR("Cannot retrieve WiFi credentials, "
			"destination struct pointer cannot be NULL");
		return -EINVAL;
	}

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	ssize_t idx = lookup_idx(ssid, ssid_len);

	if (idx == -1) {
		LOG_DBG("Cannot retrieve WiFi credentials, no entry found for the provided SSID");
		ret = -ENOENT;
		goto exit;
	}

	ret = wifi_credentials_load_entry(idx, buf, sizeof(struct wifi_credentials_personal));

	if (ret) {
		LOG_ERR("Failed to load WiFi credentials at index %d, err: %d", idx, ret);
		goto exit;
	}

	if (buf->header.type != WIFI_SECURITY_TYPE_NONE &&
	    buf->header.type != WIFI_SECURITY_TYPE_PSK &&
	    buf->header.type != WIFI_SECURITY_TYPE_PSK_SHA256 &&
	    buf->header.type != WIFI_SECURITY_TYPE_SAE &&
	    buf->header.type != WIFI_SECURITY_TYPE_WPA_PSK) {
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
	int ret;

	if (creds->header.ssid_len > WIFI_SSID_MAX_LEN || creds->header.ssid_len == 0) {
		LOG_ERR("Cannot set WiFi credentials, SSID has invalid format");
		return -EINVAL;
	}

	if (creds == NULL) {
		LOG_ERR("Cannot set WiFi credentials, provided struct pointer cannot be NULL");
		return -EINVAL;
	}

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);

	ssize_t idx = lookup_idx(creds->header.ssid, creds->header.ssid_len);

	if (idx == -1) {
		idx = lookup_unused_idx();
		if (idx == -1) {
			LOG_ERR("Cannot store WiFi credentials, no space left");
			ret = -ENOBUFS;
			goto exit;
		}
	}

	ret = wifi_credentials_store_entry(idx, creds, sizeof(struct wifi_credentials_personal));

	if (ret) {
		LOG_ERR("Failed to store WiFi credentials at index %d, err: %d", idx, ret);
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

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot set WiFi credentials, SSID has invalid format");
		return -EINVAL;
	}

	if (flags & WIFI_CREDENTIALS_FLAG_BSSID &&
	    (bssid_len != WIFI_MAC_ADDR_LEN || bssid == NULL)) {
		LOG_ERR("Cannot set WiFi credentials, "
			"provided flags indicated BSSID, but no BSSID provided");
		return -EINVAL;
	}

	if ((type != WIFI_SECURITY_TYPE_NONE && (password_len == 0 || password == NULL)) ||
	    (password_len > WIFI_CREDENTIALS_MAX_PASSWORD_LEN)) {
		LOG_ERR("Cannot set WiFi credentials, password not provided or invalid");
		return -EINVAL;
	}

	/* pack entry */
	struct wifi_credentials_header *header = (struct wifi_credentials_header *)buf;

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
		LOG_ERR("Cannot set WiFi credentials, "
			"provided security type %d is unsupported",
			type);
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

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot retrieve WiFi credentials, SSID has invalid format");
		return -EINVAL;
	}

	if (bssid_buf_len != WIFI_MAC_ADDR_LEN || bssid_buf == NULL) {
		LOG_ERR("BSSID buffer needs to be provided");
		return -EINVAL;
	}

	if (password_buf == NULL || password_buf_len > WIFI_CREDENTIALS_MAX_PASSWORD_LEN ||
	    password_buf_len == 0) {
		LOG_ERR("WiFi password buffer needs to be provided");
		return -EINVAL;
	}

	/* load entry */
	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len,
							   (struct wifi_credentials_personal *)buf);
	if (ret) {
		return ret;
	}

	/* unpack entry*/
	struct wifi_credentials_header *header = (struct wifi_credentials_header *)buf;

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
		LOG_ERR("Cannot get WiFi credentials, "
			"the requested credentials have invalid WIFI_SECURITY_TYPE");
		ret = -EPROTO;
	}
	return ret;
}

int wifi_credentials_delete_by_ssid(const char *ssid, size_t ssid_len)
{
	int ret = 0;

	if (ssid == NULL || ssid_len > WIFI_SSID_MAX_LEN || ssid_len == 0) {
		LOG_ERR("Cannot delete WiFi credentials, SSID has invalid format");
		return -EINVAL;
	}

	k_mutex_lock(&wifi_credentials_mutex, K_FOREVER);
	ssize_t idx = lookup_idx(ssid, ssid_len);

	if (idx == -1) {
		LOG_DBG("WiFi credentials entry was not found");
		goto exit;
	}

	ret = wifi_credentials_delete_entry(idx);

	if (ret) {
		LOG_ERR("Failed to delete WiFi credentials index %d, err: %d", idx, ret);
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
	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (is_entry_used(i)) {
			ret = wifi_credentials_delete_entry(i);
			if (ret) {
				LOG_ERR("Failed to delete WiFi credentials index %d, err: %d", i,
					ret);
				break;
			}

			wifi_credentials_uncache_ssid(i);
		}
	}

	k_mutex_unlock(&wifi_credentials_mutex);
	return ret;
}

SYS_INIT(init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
