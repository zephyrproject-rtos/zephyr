/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Silicon Labs SiWx91x devices.
 * @ingroup clock_control_silabs_siwx91x
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>

/**
 * @defgroup clock_control_silabs_siwx91x Silicon Labs SiWx91x
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief Mux / gate clock configuration for SiWx91x clock managers. */
struct silabs_siwx91x_clock_control_config {
	uint32_t clkid;     /**< Clock identifier (@c SIWX91X_CLK_*). */
	uint32_t ref_clkid; /**< Parent / reference clock identifier. */
	uint32_t clock_div; /**< Optional divider applied after the mux. */
};

/** @brief PLL configuration for SiWx91x HP clock manager. */
struct silabs_siwx91x_clock_control_pll_config {
	uint32_t clkid;     /**< PLL clock identifier (@c SIWX91X_CLK_PLL_*). */
	uint32_t ref_clkid; /**< PLL reference clock identifier. */
	uint32_t frequency; /**< Target PLL output frequency, in Hz. */
};

/**
 * @brief Get the clock control device that owns @p clkid.
 *
 * @param clkid Clock identifier (@c SIWX91X_CLK_*).
 *
 * @return Pointer to the owning clock control device, or @c NULL if @p clkid
 *         is out of range or has no associated device.
 */
const struct device *siwx91x_clock_control_get_device(uint32_t clkid);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_H_ */
