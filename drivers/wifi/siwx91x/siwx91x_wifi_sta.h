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

unsigned int siwx91x_wifi_module_stats_event_handler(sl_wifi_event_t event, unsigned int status,
						    void *data, uint32_t data_length, void *arg);
unsigned int siwx91x_on_join(sl_wifi_event_t event, unsigned int status,
			     void *data, uint32_t data_length, void *arg);

#endif
