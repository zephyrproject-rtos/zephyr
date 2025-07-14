/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_CREDS_H__
#define WIFI_CREDS_H__

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <utils/common.h>
#include <eap_peer/eap_config.h>
#include <ctrl_iface_zephyr.h>
#include <wpa_supplicant/config.h>

extern struct wifi_enterprise_creds_params enterprise_creds_params;

/*
 * Set Wi-Fi enterprise credentials
 * @param iface Network interface
 * @param AP or Station mode
 */
int wifi_set_enterprise_credentials(struct net_if *iface,
						bool is_ap);

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
/*
 * Clear Wi-Fi enterprise credentials
 * @param Wi-Fi enterprise params
 */
void clear_enterprise_creds_params(struct wifi_enterprise_creds_params *params);
#endif

#endif /* WIFI_CREDS_H__ */
