/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_PMSTATS_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_PMSTATS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Account one sleep fragment, called just before entering light sleep.
 *
 * @param sleep_time_us    Time left until the wake deadline, in microseconds.
 * @param min_sleep_us     Minimum time needed to enter sleep (the sleep floor),
 *                         used to estimate when the wait is effectively over.
 * @param deadline_abs_us  Absolute wake deadline in the esp_timer frame,
 *                         anchored at low-power-idle entry, used to measure
 *                         wake margin.
 */
void esp32_sleep_stats_before(uint64_t sleep_time_us, uint64_t min_sleep_us,
			      uint64_t deadline_abs_us);

/**
 * @brief Account one sleep fragment, called right after returning from light sleep.
 */
void esp32_sleep_stats_after(void);

/**
 * @brief Summary of one completed light sleep window.
 */
struct esp32_sleep_window {
	uint32_t fragments;     /**< sleep fragments in the window */
	uint32_t slept;         /**< fragments that actually slept */
	uint32_t skipped;       /**< fragments rejected/too-short */
	uint64_t req_us;        /**< time to the wake deadline at window start */
	uint64_t exec_us;       /**< time spent in the sleeps that really slept */
	int64_t wake_margin_us; /**< wake - deadline; > 0 late, < 0 early */
	int32_t err;            /**< last non-reject HAL error, 0 if none */
	bool top_down;          /**< TOP peripheral domain powered down */
	bool flash_down;        /**< flash supply (VDDSDIO) powered down */
	bool cpu_down;          /**< CPU domain powered down */
};

/**
 * @brief Copy the most recently completed light sleep window.
 *
 * @param out Destination, filled with the last completed window (zeroed before
 *            the first window completes).
 *
 * @return A sequence number that increments once per completed window, so a
 *         caller can detect that a fresh window was reported.
 */
uint32_t esp32_sleep_stats_get(struct esp32_sleep_window *out);

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_PMSTATS_H_ */
