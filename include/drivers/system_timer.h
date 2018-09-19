/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Timer driver API
 *
 *
 * Declare API implemented by system timer driver and used by kernel components.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <stdbool.h>

extern int _sys_clock_driver_init(struct device *device);

extern void _timer_int_handler(void *arg);

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE
extern void sys_clock_disable(void);
#endif

#ifdef CONFIG_TICKLESS_IDLE
extern void _timer_idle_enter(s32_t ticks);
extern void _timer_idle_exit(void);
#else
#define _timer_idle_enter(ticks) do { } while (false)
#define _timer_idle_exit() do { } while (false)
#endif /* CONFIG_TICKLESS_IDLE */

/**
 * @brief Announce time progress to the kernel
 *
 * Informs the kernel that the specified number of ticks have elapsed
 * since the last call to z_clock_announce() (or system startup for
 * the first call).
 *
 * @param ticks Elapsed time, in ticks
 */
extern void z_clock_announce(s32_t ticks);

#ifdef CONFIG_TICKLESS_KERNEL
extern void _set_time(u32_t time);
extern u32_t _get_program_time(void);
extern u32_t _get_remaining_program_time(void);
extern u32_t _get_elapsed_program_time(void);
extern u64_t _get_elapsed_clock_time(void);
#endif

extern int sys_clock_device_ctrl(struct device *device,
				 u32_t ctrl_command, void *context);

/*
 * Currently regarding timers, only loapic timer and arcv2_timer0 implements
 * device pm functionality. For other timers, use default handler in case
 * the app enables CONFIG_DEVICE_POWER_MANAGEMENT.
 */
#if !defined(CONFIG_LOAPIC_TIMER) && !defined(CONFIG_ARCV2_TIMER)
#define sys_clock_device_ctrl device_pm_control_nop
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_ */
