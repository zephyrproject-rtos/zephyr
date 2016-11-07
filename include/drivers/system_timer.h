/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
/*
 * Timer interrupt handler is one of the routines that the driver
 * has to implement, but it is not necessarily an external function.
 * The driver may implement it and use only when setting an
 * interrupt handler by calling irq_connect.
 */
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
 * Currently regarding timers, only loapic timer implements
 * device pm functionality. For other timers, use default handler in case
 * the app enables CONFIG_DEVICE_POWER_MANAGEMENT.
 */
#ifndef CONFIG_LOAPIC_TIMER
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
