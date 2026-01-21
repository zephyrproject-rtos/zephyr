/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_COMMON_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

/*
 * SoC-specific functions - implemented in per-SoC files.
 * Each ESP32 variant must provide these functions.
 */

/**
 * @brief Initialize peripheral clocks to a known state
 *
 * Disables unused peripheral clocks and resets peripherals.
 * Called during clock controller initialization.
 */
void esp32_clock_peripheral_init(void);

/**
 * @brief Early clock initialization (PMU/RTC)
 *
 * Performs SoC-specific early initialization such as PMU init
 * for ESP32-C6/H2 or RTC init for other variants.
 *
 * @return 0 on success, negative errno on failure
 */
int esp32_clock_early_init(void);

/**
 * @brief Configure CPU clock
 *
 * Sets up the CPU clock source and frequency based on the
 * provided configuration.
 *
 * @param cpu_cfg CPU clock configuration
 * @return 0 on success, negative errno on failure
 */
int esp32_cpu_clock_configure(const struct esp32_cpu_clock_config *cpu_cfg);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_COMMON_H_ */
