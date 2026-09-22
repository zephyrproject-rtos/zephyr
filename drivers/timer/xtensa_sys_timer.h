/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMER_XTENSA_SYS_TIMER_H_
#define ZEPHYR_DRIVERS_TIMER_XTENSA_SYS_TIMER_H_

#include <zephyr/types.h>

/**
 * @brief Hook invoked before entering low-power mode
 *
 * Called by the Xtensa system timer driver when the kernel is about
 * to enter low-power mode and a finite timeout is programmed.
 *
 * The SoC-specific implementation may:
 *  - Configure an always-on low-power timer (LPTIM) that continues
 *    running while the CPU CCOUNT register is halted.
 *  - Program the LPTIM to wake the system after at most
 *    @p max_lpm_time_us microseconds.
 *  - Perform any additional platform-specific preparation required
 *    prior to entering low-power mode.
 *
 * The function must return the current timestamp from the LPTIM,
 * expressed in ticks (raw count). This value is used at LPM exit to
 * determine how long CCOUNT remained stalled during low-power mode,
 * allowing the driver to compensate for missed cycles.
 *
 * @param max_lpm_time_us Maximum allowed low-power residency in µs.
 *
 * @return Current LPTIM counter value in raw ticks.
 */
uint64_t z_xtensa_lptim_hook_on_lpm_entry(uint64_t max_lpm_time_us);

/**
 * @brief Hook invoked after exiting low-power mode
 *
 * Called by the Xtensa system timer driver immediately after the CPU
 * resumes from low-power mode.
 *
 * The function must return the current timestamp from the LPTIM,
 * expressed in ticks (raw count). The difference between this value
 * and the one returned by z_xtensa_lptim_hook_on_lpm_entry() is used
 * to compute how long the CPU CCOUNT was stalled, so the driver
 * timer can apply the appropriate cycle compensation.
 *
 * @return Current LPTIM counter value in raw ticks.
 */
uint64_t z_xtensa_lptim_hook_on_lpm_exit(void);

/**
 * @brief Hook function for LPTIM clock frequency.
 *
 * @return LPTIM clock frequency in Hz.
 */
uint64_t z_xtensa_lptim_hook_get_freq(void);

#endif /* ZEPHYR_DRIVERS_TIMER_XTENSA_SYS_TIMER_H_ */
