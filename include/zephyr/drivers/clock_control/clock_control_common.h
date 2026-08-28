/*
 * Copyright (c) 2026 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup clock_control_interface
 * @brief Header file with common helpers for clock control driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

/**
 * @brief Common @ref clock_control_on() implementation for always-running clocks.
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sys Opaque data representing the clock.
 * @retval 0 Always-running clock is always on.
 */
int clock_control_always_running_clk_on(const struct device *dev, clock_control_subsys_t sys);

/**
 * @brief Common @ref clock_control_off() implementation for always-running clocks.
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sys Opaque data representing the clock.
 * @retval -ENOTSUP Always-running clock cannot be turned off.
 */
int clock_control_always_running_clk_off(const struct device *dev, clock_control_subsys_t sys);

/**
 * @brief Common @ref clock_control_get_status() implementation for always-running clocks.
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sys Opaque data representing the clock.
 * @retval CLOCK_CONTROL_STATUS_ON Always-running clock is always on.
 */
enum clock_control_status clock_control_always_running_clk_get_status(const struct device *dev,
								      clock_control_subsys_t sys);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_COMMON_H_ */
