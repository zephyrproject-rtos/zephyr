/*
 * Copyright (c) 2025 STMicroelectronics
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System timer low-power companion interface
 *
 * Declare the internal API used to hand off timekeeping across low-power
 * states when the primary system timer cannot remain active.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_SYSTEM_TIMER_LPM_H_
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_SYSTEM_TIMER_LPM_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System timer low-power companion interface
 * @defgroup clock_lpm System Timer Low-power Companion Interface
 * @ingroup clock_apis
 * @kconfig_dep{CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER,CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS}
 *
 * This interface is used when a system timer low-power companion is
 * configured. Providers may come either from the generic Counter API
 * path or from SoC/platform-specific hook implementations.
 * @{
 */

/**
 * @brief Prepare a low-power-capable companion timer before entering idle
 *
 * This interface is owned by the system timer subsystem. The provider may be
 * implemented by the system timer driver itself or by SoC/platform code.
 *
 * The provider must ensure the system wakes up no later than
 * @p max_lpm_time_us after low-power entry and must capture enough state to
 * report elapsed time from z_sys_clock_lpm_exit().
 *
 * @note This is an internal kernel/platform interface. Application code must
 * not call it.
 *
 * @param max_lpm_time_us Maximum time allowed in low-power mode, in
 * microseconds.
 */
void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us);

/**
 * @brief Report elapsed time after low-power exit
 *
 * Return the time elapsed, in microseconds, since the matching call to
 * z_sys_clock_lpm_enter().
 *
 * @note This is an internal kernel/platform interface. Application code must
 * not call it.
 *
 * @return Time elapsed in low-power mode, in microseconds.
 */
uint64_t z_sys_clock_lpm_exit(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_SYSTEM_TIMER_LPM_H_ */
