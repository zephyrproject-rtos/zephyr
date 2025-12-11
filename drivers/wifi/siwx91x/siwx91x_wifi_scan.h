/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_SCAN_H
#define SIWX91X_WIFI_SCAN_H

#include <zephyr/net/wifi_mgmt.h>

#include "sl_wifi_types.h"

int siwx91x_scan(const struct device *dev,
		 struct wifi_scan_params *params,
		 scan_result_cb_t cb);

unsigned int siwx91x_on_scan(sl_wifi_event_t event, sl_wifi_scan_result_t *result,
			     uint32_t result_size, void *arg);

#endif
