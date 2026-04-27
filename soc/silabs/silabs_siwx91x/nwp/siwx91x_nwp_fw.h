/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_NWP_FW_H_
#define SIWX91X_NWP_FW_H_

struct device;

int siwx91x_nwp_fw_boot(const struct device *dev);
void siwx91x_nwp_fw_reset(const struct device *dev);
void siwx91x_nwp_request_xtal(const struct device *dev);
void siwx91x_nwp_release_xtal(const struct device *dev);

#endif
