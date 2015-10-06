/* arcv2_timer0.c - ARC timer 0 device driver */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module implements a kernel device driver for the ARCv2 processor timer 0
and provides the standard "system clock driver" interfaces.

\INTERNAL IMPLEMENTATION DETAILS
The ARCv2 processor timer provides a 32-bit incrementing, wrap-to-zero counter.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>
#include <arch/arc/v2/aux_regs.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>

/*
 * A board support package's board.h header must provide definitions for the
 * following constants:
 *
 *    CONFIG_ARCV2_TIMER0_CLOCK_FREQ
 *
 * This is the sysTick input clock frequency.
 */

#include <board.h>

#define _ARC_V2_TMR_CTRL_IE 0x1 /* interrupt enable */
#define _ARC_V2_TMR_CTRL_NH 0x2 /* count only while not halted */
#define _ARC_V2_TMR_CTRL_W  0x4 /* watchdog mode enable */
#define _ARC_V2_TMR_CTRL_IP 0x8 /* interrupt pending flag */

/* running total of timer count */
static uint32_t clock_accumulated_count = 0;


/**
 *
 * @brief Enable the timer with the given limit/countup value
 *
 * This routine sets up the timer for operation by:
 * - setting value to which the timer will count up to;
 * - setting the timer's start value to zero; and
 * - enabling interrupt generation.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static ALWAYS_INLINE void enable(
	uint32_t count /* interrupt triggers when up-counter reaches this value */
)
{
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_LIMIT, count); /* write limit value */

	/* count only when not halted for debug and enable interrupts */
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL,
		      _ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

	_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, 0); /* write the start value */
}

/**
 *
 * @brief Get the current counter value
 *
 * This routine gets the value from the timer's count register.  This
 * value is the 'time' elapsed from the starting count (assumed to be 0).
 *
 * @return the current counter value
 *
 * \NOMANUAL
 */
static ALWAYS_INLINE uint32_t count_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
}

/**
 *
 * @brief Get the limit/countup value
 *
 * This routine gets the value from the timer's limit register, which is the
 * value to which the timer will count up to.
 *
 * @return the limit value
 *
 * \NOMANUAL
 */
static ALWAYS_INLINE uint32_t limit_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
}

/**
 *
 * @brief System clock periodic tick handler
 *
 * This routine handles the system clock periodic tick interrupt.  A TICK_EVENT
 * event is pushed onto the microkernel stack.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void _timer_int_handler(void *unused)
{
	uint32_t zero_ip_bit = _ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE;

	ARG_UNUSED(unused);

	/* clear the interrupt by writing 0 to IP bit of the control register */
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, zero_ip_bit);

	clock_accumulated_count += sys_clock_hw_cycles_per_tick;

	_sys_clock_tick_announce();
}

/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the ARCv2 timer to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */

int _sys_clock_driver_init(struct device *device)
{
	int irq = CONFIG_ARCV2_TIMER0_INT_LVL;
	int prio = CONFIG_ARCV2_TIMER0_INT_PRI;

	ARG_UNUSED(device);

	/* ensure that the timer will not generate interrupts */
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, 0);
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, 0); /* clear the count value */

	(void)irq_connect(irq, prio, _timer_int_handler, 0);

	/*
	 * Set the reload value to achieve the configured tick rate, enable the
	 * counter and interrupt generation.
	 */

	enable(sys_clock_hw_cycles_per_tick - 1);

	/* everything has been configured: safe to enable the interrupt */
	irq_enable(CONFIG_ARCV2_TIMER0_INT_LVL);

	return 0;
}

/**
 *
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles.
 *
 * @return up counter of elapsed clock cycles
 */

uint32_t _sys_clock_cycle_get(void)
{
	return (clock_accumulated_count + count_get());
}

FUNC_ALIAS(_sys_clock_cycle_get, nano_cycle_get_32, uint32_t);
FUNC_ALIAS(_sys_clock_cycle_get, task_cycle_get_32, uint32_t);

#if defined(CONFIG_SYSTEM_TIMER_DISABLE)
/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine disables timer interrupt generation and delivery.
 * Note that the timer's counting cannot be stopped by software.
 *
 * @return N/A
 */

void timer_disable(void)
{
	unsigned int key;  /* interrupt lock level */
	uint32_t ctrl_val; /* timer control register value */

	key = irq_lock();

	/* disable interrupt generation */

	ctrl_val = _arc_v2_aux_reg_read(_ARC_V2_TMR0_CONTROL);
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, ctrl_val & ~_ARC_V2_TMR_CTRL_IE);

	irq_unlock(key);

	/* disable interrupt in the interrupt controller */

	irq_disable(CONFIG_ARCV2_TIMER0_INT_LVL);
}
#endif /* CONFIG_SYSTEM_TIMER_DISABLE */
