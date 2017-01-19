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

#ifndef _TIMER__H_
#define _TIMER__H_

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
extern void _timer_idle_enter(int32_t ticks);
extern void _timer_idle_exit(void);
#endif /* CONFIG_TICKLESS_IDLE */

extern void _nano_sys_clock_tick_announce(int32_t ticks);

extern int sys_clock_device_ctrl(struct device *device,
				 uint32_t ctrl_command, void *context);

/*
 * Currently regarding timers, only loapic timer and arcv2_timer0 implements
 * device pm functionality. For other timers, use default handler in case
 * the app enables CONFIG_DEVICE_POWER_MANAGEMENT.
 */
#if !defined(CONFIG_LOAPIC_TIMER) && !defined(CONFIG_ARCV2_TIMER)
#define sys_clock_device_ctrl device_pm_control_nop
#endif

extern int32_t _sys_idle_elapsed_ticks;
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

#endif /* _TIMER__H_ */
