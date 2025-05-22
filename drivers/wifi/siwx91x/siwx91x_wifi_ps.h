/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_PS_H
#define SIWX91X_WIFI_PS_H

struct device;
struct siwx91x_dev;
struct wifi_twt_params;
struct wifi_ps_config;
struct wifi_ps_params;

int siwx91x_set_twt(const struct device *dev, struct wifi_twt_params *params);
int siwx91x_set_power_save(const struct device *dev, struct wifi_ps_params *params);
int siwx91x_get_power_save_config(const struct device *dev, struct wifi_ps_config *config);

int siwx91x_apply_power_save(struct siwx91x_dev *sidev);

#endif
