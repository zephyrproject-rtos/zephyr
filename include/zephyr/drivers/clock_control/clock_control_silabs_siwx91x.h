/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_H_

#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>

struct silabs_siwx91x_clock_control_config {
	uint32_t clkid;
	uint32_t ref_clkid;
	uint32_t clock_div;
};

struct silabs_siwx91x_clock_control_pll_config {
	uint32_t clkid;
	uint32_t ref_clkid;
	uint32_t frequency;
};

const struct device *siwx91x_clock_control_get_device(uint32_t clkid);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_H_ */
