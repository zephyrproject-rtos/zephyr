/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_STA_H
#define SIWX91X_WIFI_STA_H

#include "sl_wifi_types.h"

struct device;
struct wifi_connect_req_params;

int siwx91x_connect(const struct device *dev, struct wifi_connect_req_params *params);
int siwx91x_disconnect(const struct device *dev);

sl_status_t siwx91x_wifi_module_stats_event_handler(sl_wifi_event_t event, void *response,
						    uint32_t result_length, void *arg);
sl_status_t siwx91x_on_join(sl_wifi_event_t event, char *result, uint32_t result_size, void *arg);

#endif
