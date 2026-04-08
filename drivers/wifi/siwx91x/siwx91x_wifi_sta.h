/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_STA_H
#define SIWX91X_WIFI_STA_H

struct device;
struct wifi_connect_req_params;

int siwx91x_wifi_connect(const struct device *dev, struct wifi_connect_req_params *params);
int siwx91x_wifi_disconnect(const struct device *dev);

#endif
