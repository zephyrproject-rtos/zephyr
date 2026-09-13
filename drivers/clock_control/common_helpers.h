/*
 * Copyright (c) 2026 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_COMMON_HELPERS_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_COMMON_HELPERS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

int clock_control_always_running_clk_on(const struct device *dev, clock_control_subsys_t sys);

int clock_control_always_running_clk_off(const struct device *dev, clock_control_subsys_t sys);

enum clock_control_status clock_control_always_running_clk_get_status(const struct device *dev,
								      clock_control_subsys_t sys);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_COMMON_HELPERS_H_ */
