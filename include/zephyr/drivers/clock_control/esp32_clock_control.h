/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_CONTROL_H_

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
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>
#endif /* CONFIG_SOC_SERIES_ESP32xx */

#define ESP32_CLOCK_CONTROL_SUBSYS_CPU 50
#define ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST 51
#define ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW 52

struct esp32_cpu_clock_config {
	int clk_src;
	uint32_t cpu_freq;
	uint32_t xtal_freq;
};

struct esp32_rtc_clock_config {
	uint32_t rtc_fast_clock_src;
	uint32_t rtc_slow_clock_src;
};

struct esp32_clock_config {
	struct esp32_cpu_clock_config cpu;
	struct esp32_rtc_clock_config rtc;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_CONTROL_H_ */
