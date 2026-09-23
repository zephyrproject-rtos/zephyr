/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Synopsys DWC3 core host-mode init.
 */

#ifndef ZEPHYR_USB_DWC3_HOST_H
#define ZEPHYR_USB_DWC3_HOST_H

#include <zephyr/device.h>

#include "xhci_dwc3_priv.h"

int dwc3_host_burst_init(const struct device *dev);
void dwc3_host_susphy_post_run(const struct device *dev, bool enable);

#endif /* ZEPHYR_USB_DWC3_HOST_H */
