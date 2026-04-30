/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#define WIFI_SSID_KEY 1
#define WIFI_PASSWORD_KEY 2
#define WIFI_CREDENTIALS_VALID_KEY 3

#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64

/**
 * @brief Initialize NVS storage
 * @return 0 on success, negative error code on failure
 */
int nvs_storage_init(void);

/**
 * @brief Save WiFi credentials to NVS
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return 0 on success, negative error code on failure
 */
int nvs_save_wifi_credentials(const char *ssid, const char *password);

/**
 * @brief Read WiFi credentials from NVS
 * @param ssid Buffer to store SSID (must be at least MAX_SSID_LEN bytes)
 * @param password Buffer to store password (must be at least MAX_PASSWORD_LEN bytes)
 * @return 0 on success, negative error code on failure
 */
int nvs_read_wifi_credentials(char *ssid, char *password);

/**
 * @brief Check if valid WiFi credentials exist in NVS
 * @return true if credentials exist, false otherwise
 */
bool nvs_has_wifi_credentials(void);

/**
 * @brief Clear WiFi credentials from NVS
 * @return 0 on success, negative error code on failure
 */
int nvs_clear_wifi_credentials(void);

#endif /* NVS_STORAGE_H */
