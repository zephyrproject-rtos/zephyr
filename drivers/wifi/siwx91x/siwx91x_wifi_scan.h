/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_SCAN_H
#define SIWX91X_WIFI_SCAN_H

#include <zephyr/net/wifi_mgmt.h>

struct siwx91x_nwp_wifi_cb;

int siwx91x_wifi_scan(const struct device *dev,
		      struct wifi_scan_params *params, scan_result_cb_t cb);
void siwx91x_wifi_on_scan_results(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf);

#endif
