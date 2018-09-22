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

#ifdef _ASMLANGUAGE

GTEXT(_timer_int_handler)

#else /* _ASMLANGUAGE */

#include <device.h>

extern int _sys_clock_driver_init(struct device *device);

extern void _timer_int_handler(void *arg);

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE
extern void sys_clock_disable(void);
#endif

#ifdef CONFIG_TICKLESS_IDLE
extern void _timer_idle_enter(s32_t ticks);
extern void _timer_idle_exit(void);
#else
#define _timer_idle_enter(ticks) do { } while ((0))
#define _timer_idle_exit() do { } while ((0))
#endif /* CONFIG_TICKLESS_IDLE */

extern void _nano_sys_clock_tick_announce(s32_t ticks);
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

extern s32_t _sys_idle_elapsed_ticks;
#define _sys_clock_tick_announce() \
		_nano_sys_clock_tick_announce(_sys_idle_elapsed_ticks)

/**
 * @brief Account for the tick due to the timer interrupt
 *
 * @return N/A
 */
static inline void _sys_clock_final_tick_announce(void)
{
	/*
	 * Ticks are both announced and immediately processed at interrupt
	 * level. Thus there is only one tick left to announce (and process).
	 */
	_sys_idle_elapsed_ticks = 1;
	_sys_clock_tick_announce();
}
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_ */
