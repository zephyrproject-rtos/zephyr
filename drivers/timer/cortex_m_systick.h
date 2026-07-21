/**
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_CORTEX_M_SYSTICK_H_
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_CORTEX_M_SYSTICK_H_

/*
 * Deprecated compatibility shim for the legacy Cortex-M SysTick low-power
 * companion interface.
 *
 * Deprecated in Zephyr 4.4.0.
 * Scheduled for removal in Zephyr 4.6.0.
 */

#include <zephyr/toolchain.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>

#define z_cms_lptim_hook_on_lpm_entry z_sys_clock_lpm_enter __DEPRECATED_MACRO
#define z_cms_lptim_hook_on_lpm_exit z_sys_clock_lpm_exit __DEPRECATED_MACRO

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_CORTEX_M_SYSTICK_H_ */
