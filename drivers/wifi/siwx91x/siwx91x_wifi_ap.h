/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_AP_H
#define SIWX91X_WIFI_AP_H

#include <stdint.h>
#include <zephyr/net/wifi.h>

struct device;
struct net_buf;
struct wifi_connect_req_params;
struct wifi_ap_config_params;
struct siwx91x_nwp_wifi_cb;

int siwx91x_ap_disable(const struct device *dev);
int siwx91x_ap_enable(const struct device *dev, struct wifi_connect_req_params *params);
int siwx91x_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params);
int siwx91x_ap_sta_disconnect(const struct device *dev, const uint8_t *mac_addr);

void siwx91x_on_ap_sta_connect(const struct siwx91x_nwp_wifi_cb *ctxt,
			       uint8_t remote_addr[WIFI_MAC_ADDR_LEN]);
void siwx91x_on_ap_sta_disconnect(const struct siwx91x_nwp_wifi_cb *ctxt,
				  uint8_t remote_addr[WIFI_MAC_ADDR_LEN]);

#endif
