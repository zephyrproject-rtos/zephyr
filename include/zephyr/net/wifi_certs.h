/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_CERTS_H__
#define WIFI_CERTS_H__

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

/**
 * Set Wi-Fi Enterprise credentials.
 *
 * Sets up the required credentials for Enterprise mode in both
 * Access Point and Station modes.
 *
 * Certificates typically used:
 * - CA certificate
 * - Client certificate
 * - Client private key
 * - Server certificate and server key (for AP mode)
 *
 * @param iface Network interface
 * @param is_ap AP or Station mode
 *
 * @return 0 if ok, < 0 if error
 */
int wifi_set_enterprise_credentials(struct net_if *iface, bool is_ap);

/**
 * Clear Wi-Fi enterprise credentials
 */
void wifi_clear_enterprise_credentials(void);

#endif /* WIFI_CERTS_H__ */
