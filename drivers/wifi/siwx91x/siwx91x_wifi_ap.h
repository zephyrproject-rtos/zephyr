/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_AP_H
#define SIWX91X_WIFI_AP_H

#include "sl_wifi_types.h"

struct device;
struct wifi_connect_req_params;
struct wifi_ap_config_params;

int siwx91x_ap_disable(const struct device *dev);
int siwx91x_ap_enable(const struct device *dev, struct wifi_connect_req_params *params);
int siwx91x_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params);
int siwx91x_ap_sta_disconnect(const struct device *dev, const uint8_t *mac_addr);

sl_status_t siwx91x_on_ap_sta_connect(sl_wifi_event_t event, void *data,
				      uint32_t data_length, void *arg);
sl_status_t siwx91x_on_ap_sta_disconnect(sl_wifi_event_t event, void *data,
					 uint32_t data_length, void *arg);

#endif
