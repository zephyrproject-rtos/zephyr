/* arcv2_timer0.c - ARC timer 0 device driver */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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
This module implements a VxMicro device driver for the ARCv2 processor timer 0
and provides the standard "system clock driver" interfaces.

\INTERNAL IMPLEMENTATION DETAILS
The ARCv2 processor timer provides a 32-bit incrementing, wrap-to-zero counter.
*/

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>
#include <nanokernel/arc/v2/aux_regs.h>
#include <clock_vars.h>
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
static uint32_t accumulatedCount = 0;


/*******************************************************************************
*
* enable - enable the timer with the given limit/countup value
*
* This routine sets up the timer for operation by:
* - setting value to which the timer will count up to;
* - setting the timer's start value to zero; and
* - enabling interrupt generation.
*
* RETURNS: N/A
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

/*******************************************************************************
*
* count_get - get the current counter value
*
* This routine gets the value from the timer's count register.  This
* value is the 'time' elapsed from the starting count (assumed to be 0).
*
* RETURNS: the current counter value
*
* \NOMANUAL
*/
static ALWAYS_INLINE uint32_t count_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
}

/*******************************************************************************
*
* limit_get - get the limit/countup value
*
* This routine gets the value from the timer's limit register, which is the
* value to which the timer will count up to.
*
* RETURNS: the limit value
*
* \NOMANUAL
*/
static ALWAYS_INLINE uint32_t limit_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
}

/*******************************************************************************
*
* _timer_int_handler - system clock periodic tick handler
*
* This routine handles the system clock periodic tick interrupt.  A TICK_EVENT
* event is pushed onto the microkernel stack.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _timer_int_handler(void *unused)
{
	uint32_t zero_ip_bit = _ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE;

	ARG_UNUSED(unused);

	/* clear the interrupt by writing 0 to IP bit of the control register */
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, zero_ip_bit);

	accumulatedCount += sys_clock_hw_cycles_per_tick;

	_nano_ticks++;

	if (_nano_timer_list) {
		_nano_timer_list->ticks--;

		while (_nano_timer_list && (!_nano_timer_list->ticks)) {
			struct nano_timer *expired = _nano_timer_list;
			struct nano_lifo *lifo = &expired->lifo;

			_nano_timer_list = expired->link;
			nano_fiber_lifo_put(lifo, expired->userData);
		}
	}
}

/*******************************************************************************
*
* timer_driver - initialize and enable the system clock
*
* This routine is used to program the ARCv2 timer to deliver interrupts at the
* rate specified via the 'sys_clock_us_per_tick' global variable.
*
* RETURNS: N/A
*/

void timer_driver(
	int priority /* priority parameter ignored by this driver */
)
{
	int irq = CONFIG_ARCV2_TIMER0_INT_LVL;
	int prio = CONFIG_ARCV2_TIMER0_INT_PRI;

	ARG_UNUSED(priority);

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
}

/*******************************************************************************
*
* timer_read - read the BSP timer hardware
*
* This routine returns the current time in terms of timer hardware clock cycles.
*
* RETURNS: up counter of elapsed clock cycles
*/

uint32_t timer_read(void)
{
	return (accumulatedCount + count_get());
}

#if defined(CONFIG_SYSTEM_TIMER_DISABLE)
/*******************************************************************************
*
* timer_disable - stop announcing ticks into the kernel
*
* This routine disables timer interrupt generation and delivery.
* Note that the timer's counting cannot be stopped by software.
*
* RETURNS: N/A
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
