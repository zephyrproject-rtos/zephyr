/**
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_CORTEX_M_SYSTICK_H_
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_CORTEX_M_SYSTICK_H_

#include <zephyr/types.h>

/**
 * Hooks/callbacks definitions for interaction between the Cortex-M
 * SysTick driver and a platform-specific low-power timer driver.
 *
 * These functions are invoked by the Cortex-M SysTick driver to
 * configure a platform-specific timer that remains active when
 * the system enters a low-power mode.
 *
 * In the rest of this file, the "platform-specific low-power mode timer"
 * is named "LPTIM", and the platform-specific driver that configures the
 * LPTIM (and implements these hooks/callbacks) is named "LPTIM driver".
 *
 * The following behavior is observed when this option is enabled:
 *
 * |------(1)---(2)--------------------(3)-------(4)--------------> Time
 *
 * (1) cmsd_hook_on_lpm_entry() is invoked
 * (2) The system enters in low-power mode
 * (3) The system exits low-power mode (due to timeout or HW event)
 * (4) cmsd_hook_lpm_time_elapsed() is called
 *
 * The return value of cmsd_hook_lpm_time_elapsed() should be as close
 * as possible to the real time interval between events (1) and (4).
 *
 * These hooks should be implemented by the application if and only if
 * CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_HOOKS is enabled.
 *
 * NOTE: the hooks are not invoked when the system enters in low-power
 * mode for an indefinite amount of time (requires CONFIG_TICKLESS_KERNEL
 * and no thread PENDING with timeout).
 */

/**
 * @brief Hook invoked when the system is about to enter low-power mode
 *
 * @param max_lpm_time_us Maximal time allowed in low-power mode (µs)
 *
 * The LPTIM driver should configure the LPTIM to wake up the system
 * after at most @a{max_lpm_time_us} elapses; depending on hardware
 * capabilities, the LPTIM may have to be configured to wake up the
 * system earlier than requested (but no later!).
 *
 * Note that this hook is not called if the system enters low-power
 * mode for an indefinite amount of time (i.e., when no threads are
 * runnable or waiting with timeout).
 */
void z_cms_lptim_hook_on_lpm_entry(uint64_t max_lpm_time_us);

/**
 * @brief Callback invoked after system exits low-power mode
 *
 * This function should return the time elapsed, in microseconds,
 * since entry in low-power mode occurred (i.e., since the call
 * to @ref{cmsd_hook_on_lpm_entry}).
 */
uint64_t z_cms_lptim_hook_on_lpm_exit(void);
#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_CORTEX_M_SYSTICK_H_ */
