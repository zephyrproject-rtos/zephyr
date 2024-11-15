/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_credentials.h>

#define ENTRY_MAX_LEN sizeof(struct wifi_credentials_personal)

/**
 * @brief Write entry to SSID cache.
 *
 * @param idx credential index
 * @param buf encoded settings entry
 * @param buf_len length of buf
 */
void wifi_credentials_cache_ssid(size_t idx, const struct wifi_credentials_header *buf);

/**
 * @brief Clear entry in SSID cache.
 *
 * @param idx credential index
 */
void wifi_credentials_uncache_ssid(size_t idx);

/**
 * @brief Stores settings entry in flash.
 *
 * @param idx credential index
 * @param buf encoded settings entry
 * @param buf_len length of buf
 * @return 0 on success, otherwise a negative error code
 */
int wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len);

/**
 * @brief Deletes settings entry from flash.
 *
 * @param idx credential index
 * @return 0 on success, otherwise a negative error code
 */
int wifi_credentials_delete_entry(size_t idx);

/**
 * @brief Loads settings entry from flash.
 *
 * @param idx credential index
 * @param buf encoded settings entry
 * @param buf_len length of buf
 * @return 0 on success, otherwise a negative error code
 */
int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len);

/**
 * @brief Initialize backend.
 * @note Is called by the library on system startup.
 *
 */
int wifi_credentials_backend_init(void);
