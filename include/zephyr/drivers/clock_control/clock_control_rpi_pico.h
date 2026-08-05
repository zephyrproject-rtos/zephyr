/*
 * SPDX-FileCopyrightText: 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Raspberry Pi Pico devices.
 * @ingroup clock_control_rpi_pico
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RPI_PICO_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RPI_PICO_H_

#include <zephyr/device.h>

/**
 * @defgroup clock_control_rpi_pico Raspberry Pi Pico
 * @ingroup clock_control_interface_ext
 * @{
 */

/**
 * @brief Reapply the devicetree-declared XOSC/PLL/clock topology.
 *
 * Used to restore clocks after a low-power mode resets them (e.g. RP2350
 * SUSPEND_TO_RAM).
 *
 * @param dev clock-controller device
 * @return 0 on success, -EINVAL if a clock could not be configured
 */
int clock_control_rpi_pico_reconfigure(const struct device *dev);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RPI_PICO_H_ */
