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
#endif /* TIMER_SUPPORTS_TICKLESS */

extern uint32_t _nano_get_earliest_deadline(void);

extern void _nano_sys_clock_tick_announce(int32_t ticks);

#ifdef CONFIG_MICROKERNEL
	#define _sys_clock_tick_announce() \
		isr_event_send(TICK_EVENT)
#else
	extern int32_t _sys_idle_elapsed_ticks;
	#define _sys_clock_tick_announce() \
		_nano_sys_clock_tick_announce(_sys_idle_elapsed_ticks)
#endif

#endif /* _ASMLANGUAGE */

#endif /* _TIMER__H_ */
