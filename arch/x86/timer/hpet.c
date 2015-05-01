/* hpet.c - Intel HPET device driver */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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
This module implements a VxMicro device driver for the Intel High Precision
Event Timer (HPET) device, and provides the standard "system clock driver"
interfaces.

The driver utilizes HPET timer0 to provide kernel ticks.

\INTERNAL IMPLEMENTATION DETAILS
The HPET device driver makes no assumption about the initial state of the HPET,
and explicitly puts the device into a reset-like state. It also assumes that
the main up counter never wraps around to 0 during the lifetime of the system.

The BSP can configure the HPET to use level rather than the default edge
sensitive interrupts by adding the following to board.h
    #define HPET_USE_LEVEL_INTS

When not configured to support tickless idle timer0 is programmed in periodic
mode so it automatically generates a single interrupt per kernel tick interval.

When configured to support tickless idle timer0 is programmed in one-shot mode.
When the CPU is not idling the timer interrupt handler sets the timer to expire
when the next kernel tick is due, waits for this to occur, and then repeats
this "ad infinitum". When the CPU begins idling the timer driver reprograms
the expiry time for the timer (thereby overriding the previously scheduled
timer interrupt) and waits for the timer to expire or for a non-timer interrupt
to occur. When the CPU ceases idling the driver determines how many complete
ticks have elapsed, reprograms the timer so that it expires on the next tick,
and announces the number of elapsed ticks (if any) to the microkernel.

In a nanokernel-only system this device driver omits more complex capabilities
(such as tickless idle support) that are only used with a microkernel.
*/

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <clock_vars.h>
#include <drivers/system_timer.h>

#ifdef CONFIG_MICROKERNEL

#include <microkernel.h>
#include <cputype.h>

extern struct nano_stack _k_command_stack;

#ifdef CONFIG_TICKLESS_IDLE
#define TIMER_SUPPORTS_TICKLESS
#endif

#endif /*  CONFIG_NANOKERNEL */

/*
 * A board support package's board.h header must provide definitions for the
 * following constants:
 *
 *    HPET_BASE_ADRS
 *    HPET_CLOCK_FREQ
 *    HPET_TIMER0_IRQ
 *    HPET_TIMER0_INT_PRI
 */

#include <board.h>

/* defines */

/* HPET register offsets */

#define GENERAL_CAPS_REG 0	  /* 64-bit register */
#define GENERAL_CONFIG_REG 0x10     /* 64-bit register */
#define GENERAL_INT_STATUS_REG 0x20 /* 64-bit register */
#define MAIN_COUNTER_VALUE_REG 0xf0 /* 64-bit register */

#define TIMER0_CONFIG_CAPS_REG 0x100   /* 64-bit register */
#define TIMER0_COMPARATOR_REG 0x108    /* 64-bit register */
#define TIMER0_FSB_INT_ROUTE_REG 0x110 /* 64-bit register */

/* read the GENERAL_CAPS_REG to determine # of timers actually implemented */

#define TIMER1_CONFIG_CAP_REG 0x120    /* 64-bit register */
#define TIMER1_COMPARATOR_REG 0x128    /* 64-bit register */
#define TIMER1_FSB_INT_ROUTE_REG 0x130 /* 64-bit register */

#define TIMER2_CONFIG_CAP_REG 0x140    /* 64-bit register */
#define TIMER2_COMPARATOR_REG 0x148    /* 64-bit register */
#define TIMER2_FSB_INT_ROUTE_REG 0x150 /* 64-bit register */

/* convenience macros for accessing specific HPET registers */

#define _HPET_GENERAL_CAPS \
	((volatile uint64_t *)(HPET_BASE_ADRS + GENERAL_CAPS_REG))

/*
 * Although the general configuration register is 64-bits, only a 32-bit access
 * is performed since the most significant bits contain no useful information.
 */

#define _HPET_GENERAL_CONFIG \
	((volatile uint32_t *)(HPET_BASE_ADRS + GENERAL_CONFIG_REG))

/*
 * Although the general interrupt status is 64-bits, only a 32-bit access
 * is performed since this driver only utilizes timer0
 * (i.e. there is no need to determine the interrupt status of other timers).
 */

#define _HPET_GENERAL_INT_STATUS \
	((volatile uint32_t *)(HPET_BASE_ADRS + GENERAL_INT_STATUS_REG))

#define _HPET_MAIN_COUNTER_VALUE \
	((volatile uint64_t *)(HPET_BASE_ADRS + MAIN_COUNTER_VALUE_REG))
#define _HPET_MAIN_COUNTER_LSW \
	((volatile uint32_t *)(HPET_BASE_ADRS + MAIN_COUNTER_VALUE_REG))
#define _HPET_MAIN_COUNTER_MSW \
	((volatile uint32_t *)(HPET_BASE_ADRS + MAIN_COUNTER_VALUE_REG + 0x4))

#define _HPET_TIMER0_CONFIG_CAPS \
	((volatile uint64_t *)(HPET_BASE_ADRS + TIMER0_CONFIG_CAPS_REG))
#define _HPET_TIMER0_COMPARATOR \
	((volatile uint64_t *)(HPET_BASE_ADRS + TIMER0_COMPARATOR_REG))
#define _HPET_TIMER0_FSB_INT_ROUTE \
	((volatile uint64_t *)(HPET_BASE_ADRS + TIMER0_FSB_INT_ROUTE_REG))

/* general capabilities register macros */

#define HPET_COUNTER_CLK_PERIOD(caps) (caps >> 32)
#define HPET_NUM_TIMERS(caps) (((caps >> 8) & 0x1f) + 1)
#define HPET_IS64BITS(caps) (caps & 0x1000)

/* general configuration register macros */

#define HPET_ENABLE_CNF (1 << 0)
#define HPET_LEGACY_RT_CNF (1 << 1)

/* timer N configuration and capabilities register macros */

#define HPET_Tn_INT_ROUTE_CAP(caps) (caps > 32)
#define HPET_Tn_FSB_INT_DEL_CAP(caps) (caps & (1 << 15))
#define HPET_Tn_FSB_EN_CNF (1 << 14)
#define HPET_Tn_INT_ROUTE_CNF_MASK (0x1f << 9)
#define HPET_Tn_INT_ROUTE_CNF_SHIFT 9
#define HPET_Tn_32MODE_CNF (1 << 8)
#define HPET_Tn_VAL_SET_CNF (1 << 6)
#define HPET_Tn_SIZE_CAP(caps) (caps & (1 << 5))
#define HPET_Tn_PER_INT_CAP(caps) (caps & (1 << 4))
#define HPET_Tn_TYPE_CNF (1 << 3)
#define HPET_Tn_INT_ENB_CNF (1 << 2)
#define HPET_Tn_INT_TYPE_CNF (1 << 1)

/*
 * HPET comparator delay factor; this is the minimum value by which a new
 * timer expiration setting must exceed the current main counter value when
 * programming a timer in one-shot mode. Failure to allow for delays incurred
 * in programming a timer may result in the HPET not generating an interrupt
 * when the desired expiration time is reached. (See HPET documentation for
 * a more complete description of this issue.)
 *
 * The value is expressed in main counter units. For example, if the HPET main
 * counter increments at a rate of 19.2 MHz, this delay corresponds to 10 us
 * (or about 0.1% of a system clock tick, assuming a tick rate of 100 Hz).
 */

#define HPET_COMP_DELAY 192

/* locals */
#ifdef CONFIG_DYNAMIC_INT_STUBS
static NANO_CPU_INT_STUB_DECL(_hpetIntStub); /* interrupt stub memory */
#else					     /* !CONFIG_DYNAMIC_INT_STUBS */
extern void *_hpetIntStub(void); /* interrupt stub code */
SYS_INT_REGISTER(_hpetIntStub, HPET_TIMER0_IRQ, HPET_TIMER0_INT_PRI);
#endif					     /* CONFIG_DYNAMIC_INT_STUBS */

#ifdef CONFIG_INT_LATENCY_BENCHMARK
static uint32_t main_count_first_irq_value = 0;
static uint32_t main_count_expected_value = 0;
#endif

#ifdef CONFIG_INT_LATENCY_BENCHMARK
extern uint32_t _hw_irq_to_c_handler_latency;
#endif

#ifdef TIMER_SUPPORTS_TICKLESS

/* additional globals, locals, and forward declarations */

extern int32_t _sys_idle_elapsed_ticks;

static uint32_t __noinit counter_load_value; /* main counter units
							    per system tick */
static uint64_t counter_last_value =
	0; /* counter value for most recent tick */
static int32_t programmed_ticks =
	1; /* # ticks timer is programmed for */
static int stale_irq_check =
	0; /* is stale interrupt possible? */

/*******************************************************************************
*
* _hpetMainCounterAtomic - safely read the main HPET up counter
*
* This routine simulates an atomic read of the 64-bit system clock on CPUs
* that only support 32-bit memory accesses. The most significant word
* of the counter is read twice to ensure it doesn't change while the least
* significant word is being retrieved (as per HPET documentation).
*
* RETURNS: current 64-bit counter value
*
* \NOMANUAL
*/

static uint64_t _hpetMainCounterAtomic(void)
{
	uint32_t highBits;
	uint32_t lowBits;

	do {
		highBits = *_HPET_MAIN_COUNTER_MSW;
		lowBits = *_HPET_MAIN_COUNTER_LSW;
	} while (highBits != *_HPET_MAIN_COUNTER_MSW);

	return ((uint64_t)highBits << 32) | lowBits;
}

#endif /* TIMER_SUPPORTS_TICKLESS */

/*******************************************************************************
*
* _timer_int_handler - system clock tick handler
*
* This routine handles the system clock tick interrupt. A TICK_EVENT event
* is pushed onto the microkernel stack.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);

#ifdef HPET_USE_LEVEL_INTS
	/* Acknowledge interrupt */
	*_HPET_GENERAL_INT_STATUS = 1;
#endif

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	uint32_t delta = *_HPET_MAIN_COUNTER_VALUE - main_count_expected_value;

	if (_hw_irq_to_c_handler_latency > delta) {
		/* keep the lowest value observed */
		_hw_irq_to_c_handler_latency = delta;
	}
	/* compute the next expected main counter value */
	main_count_expected_value += main_count_first_irq_value;
#endif

#ifdef CONFIG_MICROKERNEL

#ifndef TIMER_SUPPORTS_TICKLESS

	/*
	 * one more tick has occurred -- don't need to do anything special since
	 * timer is already configured to interrupt on the following tick
	 */

	_sys_clock_tick_announce();

#else

	/* see if interrupt was triggered while timer was being reprogrammed */

	if (stale_irq_check) {
		stale_irq_check = 0;
		if (_hpetMainCounterAtomic() < *_HPET_TIMER0_COMPARATOR) {
			return; /* ignore "stale" interrupt */
		}
	}

	/* configure timer to expire on next tick */

	counter_last_value = *_HPET_TIMER0_COMPARATOR;
	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR = counter_last_value + counter_load_value;
	programmed_ticks = 1;

	/*
	 * Increment the tick because _timer_idle_exit does not account
	 * for the tick due to the timer interrupt itself. Also, if not in
	 * tickless mode,
	 * _SysIdleElpasedTicks will be 0.
	 */
	_sys_idle_elapsed_ticks++;

	/*
	 * If we transistion from 0 elapsed ticks to 1 we need to announce the
	 * tick
	 * event to the microkernel. Other cases will have already been covered
	 * by
	 * _timer_idle_exit
	 */

	if (_sys_idle_elapsed_ticks == 1) {
		_sys_clock_tick_announce();
	}

#endif /* !TIMER_SUPPORTS_TICKLESS */

#else
	_sys_clock_tick_announce();
#endif /* CONFIG_MICROKERNEL */
}

#ifdef TIMER_SUPPORTS_TICKLESS

/*
 * Ensure that _timer_idle_enter() is never asked to idle for fewer than 2
 * ticks (since this might require the timer to be reprogrammed for a deadline
 * too close to the current time, resulting in a missed interrupt which would
 * permanently disable the tick timer)
 */

#if (CONFIG_TICKLESS_IDLE_THRESH < 2)
#error Tickless idle threshold is too small (must be at least 2)
#endif

/*******************************************************************************
*
* _timer_idle_enter - Place system timer into idle state
*
* Re-program the timer to enter into the idle state for the given number of
* ticks (-1 means infinite number of ticks).
*
* RETURNS: N/A
*
* \INTERNAL IMPLEMENTATION DETAILS
* Called while interrupts are locked.
*/

void _timer_idle_enter(int32_t ticks /* system ticks */
				)
{
	/*
	 * reprogram timer to expire at the desired time (which is guaranteed
	 * to be at least one full tick from the current counter value)
	 */

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR =
		(ticks >= 0) ? counter_last_value + ticks * counter_load_value
			     : ~(uint64_t)0;
	stale_irq_check = 1;
	programmed_ticks = ticks;
}

/*******************************************************************************
*
* _timer_idle_exit - Take system timer out of idle state
*
* Determine how long timer has been idling and reprogram it to interrupt at the
* next tick.
*
* Note that in this routine, _SysTimerElapsedTicks must be zero because the
* ticker has done its work and consumed all the ticks. This has to be true
* otherwise idle mode wouldn't have been entered in the first place.
*
* RETURNS: N/A
*
* \INTERNAL IMPLEMENTATION DETAILS
* Called by _IntEnt() while interrupts are locked.
*/

void _timer_idle_exit(void)
{
	uint64_t currTime = _hpetMainCounterAtomic();
	int32_t elapsedTicks;
	uint64_t counterNextValue;

	/* see if idling ended because timer expired at the desired tick */

	if (currTime >= *_HPET_TIMER0_COMPARATOR) {
		/*
		 * update # of ticks since last tick event was announced,
		 * so that this value is available to ISRs that run before the
		 * timer
		 * interrupt handler runs (which is unlikely, but could happen)
		 */

		_sys_idle_elapsed_ticks = programmed_ticks - 1;

		/*
		 * Announce elapsed ticks to the microkernel. Note we are
		 * guaranteed
		 * that the timer ISR will execute first before the tick event
		 * is
		 * serviced.
		 */
		_sys_clock_tick_announce();

		/* timer interrupt handler reprograms the timer for the next
		 * tick */

		return;
	}

	/*
	 * idling ceased because a non-timer interrupt occurred
	 *
	 * compute how much idle time has elapsed and reprogram the timer
	 * to expire on the next tick; if the next tick will happen so soon
	 * that HPET might miss the interrupt declare that tick prematurely
	 * and program the timer for the tick after that
	 *
	 * note: a premature tick declaration has no significant impact on
	 * the microkernel, which gets informed of the correct number of elapsed
	 * ticks when the following tick finally occurs; however, any ISRs that
	 * access _sys_idle_elapsed_ticks to determine the current time may be
	 *misled
	 * during the (very brief) interval before the tick-in-progress finishes
	 * and the following tick begins
	 */

	elapsedTicks =
		(int32_t)((currTime - counter_last_value) / counter_load_value);
	counter_last_value += (uint64_t)elapsedTicks * counter_load_value;

	counterNextValue = counter_last_value + counter_load_value;

	if ((counterNextValue - currTime) <= HPET_COMP_DELAY) {
		elapsedTicks++;
		counterNextValue += counter_load_value;
		counter_last_value += counter_load_value;
	}

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR = counterNextValue;
	stale_irq_check = 1;

	/*
	 * update # of ticks since last tick event was announced,
	 * so that this value is available to ISRs that run before the timer
	 * expires and the timer interrupt handler runs
	 */

	_sys_idle_elapsed_ticks = elapsedTicks;

	if (_sys_idle_elapsed_ticks) {
		/* Announce elapsed ticks to the microkernel */
		_sys_clock_tick_announce();
	}

	/*
	 * Any elapsed ticks have been accounted for so simply set the
	 * programmed
	 * ticks to 1 since the timer has been programmed to fire on the next
	 * tick
	 * boundary.
	 */

	programmed_ticks = 1;
}

#endif /* TIMER_SUPPORTS_TICKLESS */

/*******************************************************************************
*
* timer_driver - initialize and enable the system clock
*
* This routine is used to program the HPET to deliver interrupts at the
* rate specified via the 'sys_clock_us_per_tick' global variable.
*
* RETURNS: N/A
*/

void timer_driver(int priority /* priority parameter is ignored by this driver
				  */
		  )
{
	uint64_t hpetClockPeriod;
	uint64_t tickFempto;
#ifndef TIMER_SUPPORTS_TICKLESS
	uint32_t counter_load_value;
#endif

	ARG_UNUSED(priority);

	/*
	 * Initial state of HPET is unknown, so put it back in a reset-like
	 * state
	 * (i.e. set main counter to 0 and disable interrupts)
	 */

	*_HPET_GENERAL_CONFIG &= ~HPET_ENABLE_CNF;
	*_HPET_MAIN_COUNTER_VALUE = 0;

/*
 * Determine the comparator load value (based on a start count of 0)
 * to achieve the configured tick rate.
 */

	/*
	 * Convert the 'sys_clock_us_per_tick' value
	 * from microseconds to femptoseconds
	 */

	tickFempto = (uint64_t)sys_clock_us_per_tick * 1000000000;

	/*
	 * This driver shall read the COUNTER_CLK_PERIOD value from the general
	 * capabilities register rather than rely on a board.h provide macro
	 * (or the global variable 'sys_clock_hw_cycles_per_tick')
	 * to determine the frequency of clock applied to the HPET device.
	 */

	/* read the clock period: units are fempto (10^-15) seconds */

	hpetClockPeriod = HPET_COUNTER_CLK_PERIOD(*_HPET_GENERAL_CAPS);

	/*
	 * compute value for the comparator register to achieve
	 * 'sys_clock_us_per_tick' period
	 */

	counter_load_value = (uint32_t)(tickFempto / hpetClockPeriod);

	/* Initialize "sys_clock_hw_cycles_per_tick" */

	sys_clock_hw_cycles_per_tick = counter_load_value;

	/*
	 * Set the comparator register for timer0.  The write to the comparator
	 * register is allowed due to setting the HPET_Tn_VAL_SET_CNF bit.
	 */

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR = counter_load_value;

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	main_count_first_irq_value = counter_load_value;
	main_count_expected_value = main_count_first_irq_value;
#endif

#ifndef TIMER_SUPPORTS_TICKLESS
	/* set timer0 to periodic mode, ready to expire every tick */

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_TYPE_CNF;
#else
	/* set timer0 to one-shot mode, ready to expire on the first tick */

	*_HPET_TIMER0_CONFIG_CAPS &= ~HPET_Tn_TYPE_CNF;
#endif /* !TIMER_SUPPORTS_TICKLESS */

	/*
	 * Route interrupts to the I/O APIC. If HPET_Tn_INT_TYPE_CNF is set this
	 * means edge triggered interrupt mode is utilized; Otherwise level
	 * sensitive interrupts are used.
	 */

	/*
	 * HPET timers IRQ field is 5 bits wide, and hence, can support only
	 * IRQ's
	 * up to 31. Some BSPs, however, use IRQs greater than 31. In this case
	 * program leaves the IRQ fields blank.
	 */

	*_HPET_TIMER0_CONFIG_CAPS =
#if HPET_TIMER0_IRQ < 32
		(*_HPET_TIMER0_CONFIG_CAPS & ~HPET_Tn_INT_ROUTE_CNF_MASK) |
		(HPET_TIMER0_IRQ << HPET_Tn_INT_ROUTE_CNF_SHIFT)
#else
		(*_HPET_TIMER0_CONFIG_CAPS & ~HPET_Tn_INT_ROUTE_CNF_MASK)
#endif

#ifdef HPET_USE_LEVEL_INTS
		| HPET_Tn_INT_TYPE_CNF;
#else
		;
#endif

#ifdef CONFIG_DYNAMIC_INT_STUBS
	/*
	 * Connect specified routine/parameter to LOAPIC interrupt vector.
	 * The "connect" will result in the LOAPIC interrupt controller being
	 * programmed with the allocated vector, i.e. there is no need for
	 * an explicit setting of the interrupt vector in this driver.
	 */

	irq_connect(HPET_TIMER0_IRQ,
			  HPET_TIMER0_INT_PRI,
			  _hpetIntHandler,
			  0,
			  _hpetIntStub);
#else
	/*
	 * Although the stub has already been "connected", the vector number
	 * still
	 * has to be programmed into the interrupt controller.
	 */

	_SysIntVecProgram(HPET_TIMER0_VEC, HPET_TIMER0_IRQ);
#endif

#ifdef CONFIG_MICROKERNEL

/* timer_read() is available for microkernel libraries to call directly */

/* K_ticker() is the pre-defined event handler for TICK_EVENT */

#endif /* CONFIG_MICROKERNEL */

	/* enable the IRQ in the interrupt controller */

	irq_enable(HPET_TIMER0_IRQ);

	/* enable the HPET generally, and timer0 specifically */

	*_HPET_GENERAL_CONFIG |= HPET_ENABLE_CNF;
	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_INT_ENB_CNF;
}

/*******************************************************************************
*
* timer_read - read the BSP timer hardware
*
* This routine returns the current time in terms of timer hardware clock cycles.
*
* RETURNS: up counter of elapsed clock cycles
*
* \INTERNAL WARNING
* If this routine is ever enhanced to return all 64 bits of the counter
* it will need to call _hpetMainCounterAtomic().
*/

uint32_t timer_read(void)
{
	return (uint32_t) *_HPET_MAIN_COUNTER_VALUE;
}

#ifdef CONFIG_SYSTEM_TIMER_DISABLE

/*******************************************************************************
*
* timer_disable - stop announcing ticks into the kernel
*
* This routine disables the HPET so that timer interrupts are no
* longer delivered.
*
* RETURNS: N/A
*/

void timer_disable(void)
{
	/*
	 * disable the main HPET up counter and all timer interrupts;
	 * there is no need to lock interrupts before doing this since
	 * no other code alters the HPET's main configuration register
	 * once the driver has been initialized
	 */

	*_HPET_GENERAL_CONFIG &= ~HPET_ENABLE_CNF;
}

#endif /* CONFIG_SYSTEM_TIMER_DISABLE */
