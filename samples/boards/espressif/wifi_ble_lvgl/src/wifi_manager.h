/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

/**
 * @brief Initialize WiFi manager
 * @return 0 on success, negative error code on failure
 */
int wifi_manager_init(void);

/**
 * @brief Connect to WiFi using provided credentials
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return 0 on success, negative error code on failure
 */
int wifi_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from WiFi
 * @return 0 on success, negative error code on failure
 */
int wifi_disconnect(void);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool wifi_is_connected(void);

/**
 * @brief Get current WiFi status
 * @return WiFi status
 */
int wifi_get_status(void);

/**
 * @brief Try to connect using stored credentials
 * @return 0 on success, negative error code on failure
 */
int wifi_connect_stored(void);

/**
 * @brief Get device IP address as string
 * @param ip_str Buffer to store IP address string
 * @param len Buffer length
 * @return 0 on success, negative error code on failure
 */
int wifi_get_ip_address(char *ip_str, size_t len);

#endif /* WIFI_MANAGER_H */
