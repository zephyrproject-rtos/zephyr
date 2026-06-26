/*
 * Copyright (c) 2024-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Espressif ESP32 devices.
 * @ingroup clock_control_esp32
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_CONTROL_H_

/**
 * @defgroup clock_control_esp32 Espressif ESP32
 * @ingroup clock_control_interface_ext
 * @{
 */

#if defined(CONFIG_SOC_SERIES_ESP32)
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#include <zephyr/dt-bindings/clock/esp32s3_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C2)
#include <zephyr/dt-bindings/clock/esp32c2_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C5)
#include <zephyr/dt-bindings/clock/esp32c5_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
#include <zephyr/dt-bindings/clock/esp32h2_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32P4)
#include <zephyr/dt-bindings/clock/esp32p4_clock.h>
#endif /* CONFIG_SOC_SERIES_ESP32xx */

/** @name ESP32 clock control subsystem identifiers */
/** @{ */
#define ESP32_CLOCK_CONTROL_SUBSYS_CPU 50              /**< CPU clock. */
#define ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST 51         /**< RTC fast clock. */
#define ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW 52         /**< RTC slow clock. */
#define ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST_NOMINAL 53 /**< RTC fast clock at nominal rate. */
#define ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW_NOMINAL 54 /**< RTC slow clock at nominal rate. */
/** @} */

/** @brief ESP32 CPU clock configuration. */
struct esp32_cpu_clock_config {
	int clk_src;        /**< CPU clock source. */
	uint32_t cpu_freq;  /**< Target CPU frequency, in MHz. */
	uint32_t xtal_freq; /**< External crystal frequency, in MHz. */
};

/** @brief ESP32 RTC clock configuration. */
struct esp32_rtc_clock_config {
	uint32_t rtc_fast_clock_src; /**< RTC fast clock source. */
	uint32_t rtc_slow_clock_src; /**< RTC slow clock source. */
};

/** @brief ESP32 aggregate clock configuration. */
struct esp32_clock_config {
	struct esp32_cpu_clock_config cpu; /**< CPU clock configuration. */
	struct esp32_rtc_clock_config rtc; /**< RTC clock configuration. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_CONTROL_H_ */
