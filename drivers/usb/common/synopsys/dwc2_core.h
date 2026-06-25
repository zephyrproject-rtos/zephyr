/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/device.h>

int dwc2_core_soft_reset(const struct device *dev, void *const base,
			 int (*is_phy_clk_off)(const struct device *dev));
