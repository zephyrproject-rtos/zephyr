/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_WDT_DW_COMMON_H_
#define ZEPHYR_DRIVERS_WATCHDOG_WDT_DW_COMMON_H_

#include <stdint.h>

/**
 * @brief Check watchdog configuration options
 *
 * Check options value passed to a watchdog setup function. Returns error if unsupported option
 * is used.
 *
 * @param options options value passed to a watchdog setup function.
 * @return Error code, 0 on success.
 */
int dw_wdt_check_options(const uint8_t options);

/**
 * @brief Configure watchdog device
 *
 * @param base Device base address.
 * @param config device configuration word
 * @return Error code, 0 on success.
 */
int dw_wdt_configure(const uint32_t base, const uint32_t config);

/**
 * @brief Calculate period
 *
 * @param [in]base Device base address.
 * @param [in]clk_freq frequency of a clock used by watchdog device
 * @param [in]config pointer to a watchdog configuration structure
 * @param [out]period_out pointer to a variable in which the period configuration word will be
 *        placed
 * @return Error code, 0 on success.
 */
int dw_wdt_calc_period(const uint32_t base, const uint32_t clk_freq,
		       const struct wdt_timeout_cfg *config, uint32_t *period_out);

/**
 * @brief Watchdog probe
 *
 * Checks device id register and configure a reset pulse length.
 *
 * @param base Device base address.
 * @param reset_pulse_length Length of a reset pulse produced by watchdog
 * @return Error code, 0 on success.
 */
int dw_wdt_probe(const uint32_t base, const uint32_t reset_pulse_length);

/**
 * @brief Watchdog disable function
 *
 * @param dev Device structure.
 * @return -ENOTSUP. The hardware does not support disabling.
 */
int dw_wdt_disable(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_WATCHDOG_WDT_DW_COMMON_H_ */
