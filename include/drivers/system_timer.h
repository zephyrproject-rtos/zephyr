/* system_timer.h - timer driver API */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION

Declare API implemented by system timer driver and used by kernel components.
 */

#ifndef _TIMER__H_
#define _TIMER__H_

#ifdef _ASMLANGUAGE

GTEXT(_timer_int_handler)

#else /* _ASMLANGUAGE */

extern uint32_t timer_read(void);
extern void timer_driver(int prio);
/*
 * Timer interrupt handler is one of the routines that the driver
 * has to implement, but it is not necessarily an external function.
 * The driver may implement it and use only when setting an
 * interrupt handler by calling irq_connect.
 */
extern void _timer_int_handler(void *arg);

#ifdef CONFIG_SYSTEM_TIMER_DISABLE
extern void timer_disable(void);
#endif

#ifdef CONFIG_TICKLESS_IDLE
extern void _timer_idle_enter(int32_t ticks);
extern void _timer_idle_exit(void);
#endif /* TIMER_SUPPORTS_TICKLESS */

extern uint32_t _nano_get_earliest_deadline(void);

#if defined(CONFIG_NANO_TIMEOUTS) || defined(CONFIG_NANO_TIMERS)
	extern void _nano_sys_clock_tick_announce(uint32_t ticks);
#else
	#define _nano_sys_clock_tick_announce(ticks) do { } while((0))
#endif

#ifdef CONFIG_MICROKERNEL
	#define _sys_clock_tick_announce() \
		nano_isr_stack_push(&_k_command_stack, TICK_EVENT)
#else
	extern uint32_t _sys_idle_elapsed_ticks;
	#define _sys_clock_tick_announce() \
		_nano_sys_clock_tick_announce(_sys_idle_elapsed_ticks)
#endif

#endif /* _ASMLANGUAGE */

#endif /* _TIMER__H_ */
