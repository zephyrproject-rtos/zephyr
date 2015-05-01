/* i8253.c - Intel 8253 PIT (Programmable Interval Timer) driver */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
This module implements a VxMicro device driver for the Intel 8253 PIT
(Programmable Interval Timer) device, and provides the standard "system
clock driver" interfaces.

Channel 0 is programmed to operate in "Interrupt on Terminal Count" mode,
with the interrupt rate determined by the 'sys_clock_us_per_tick'
global variable.
Changing the interrupt rate at runtime is not supported.

Generally, this module is not utilized in Wind River Hypervisor systems;
instead the Hypervisor tick timer service is utilized to deliver system clock
ticks into the guest operating system.  However, this driver has been modified
to access the PIT in scenarios where the PIT registers are mapped into a guest.
An interrupt controller driver will not be utilized, so this driver will
directly invoke the VIOAPIC APIs to configure/unmask the IRQ.
*/

/* includes */

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <limits.h>
#include <clock_vars.h>
#include <drivers/system_timer.h>

#ifdef CONFIG_MICROKERNEL

#include <microkernel.h>
#include <cputype.h>

#endif /* CONFIG_MICROKERNEL */

/*
 * A board support package's board.h header must provide definitions for the
 * following constants:
 *
 *    PIC_REG_ADDR_INTERVAL
 *    PIT_BASE_ADRS
 *    PIT_CLOCK_FREQ
 *    PIT_INT_LVL
 *    PIT_INT_VEC
 *
 * ...and the following register access macros:
 *
 *   PLB_BYTE_REG_WRITE
 *   PLB_BYTE_REG_READ
 */

#include <board.h>

/* defines */

#if defined(CONFIG_TICKLESS_IDLE)
#define TIMER_SUPPORTS_TICKLESS
#endif

#if defined(TIMER_SUPPORTS_TICKLESS)

#define TIMER_MODE_PERIODIC 0
#define TIMER_MODE_PERIODIC_ENT 1

#else /* !TIMER_SUPPORTS_TICKLESS */

#define _i8253TicklessIdleInit() \
	do {/* nothing */        \
	} while ((0))
#define _i8253TicklessIdleSkew() \
	do {/* nothing */        \
	} while (0)

#endif /* !TIMER_SUPPORTS_TICKLESS */

/* register definitions */

#define PIT_ADRS(base, reg) (base + (reg * PIT_REG_ADDR_INTERVAL))

#define PIT_CNT0(base) PIT_ADRS(base, 0x00) /* counter/channel 0 */
#define PIT_CNT1(base) PIT_ADRS(base, 0x01) /* counter/channel 1 */
#define PIT_CNT2(base) PIT_ADRS(base, 0x02) /* counter/channel 2 */
#define PIT_CMD(base) PIT_ADRS(base, 0x03)  /* control word */

/* globals */

#if defined(TIMER_SUPPORTS_TICKLESS)
extern int32_t _sys_idle_elapsed_ticks;
#endif

/* locals */

/* interrupt stub memory for irq_connect() */

#ifndef CONFIG_DYNAMIC_INT_STUBS
extern void *_i8253_interrupt_stub;
SYS_INT_REGISTER(_i8253_interrupt_stub, PIT_INT_LVL, PIT_INT_PRI);
#else
static NANO_CPU_INT_STUB_DECL(_i8253_interrupt_stub);
#endif

static uint16_t __noinit counterLoadVal; /* computed counter */
static volatile uint32_t clock_accumulated_count = 0;
static uint16_t _currentLoadVal = 0;

#if defined(TIMER_SUPPORTS_TICKLESS)

static uint16_t idle_original_count = 0;
static uint16_t idle_original_ticks = 0;
static uint16_t __noinit max_system_ticks;
static uint16_t __noinit max_load_value;
static uint16_t __noinit timer_idle_skew;

/* Used to determine if the timer ISR should place the timer in periodic mode */
static unsigned char timer_mode = TIMER_MODE_PERIODIC;
#endif /* TIMER_SUPPORTS_TICKLESS */

static uint32_t old_count = 0; /* previous system clock value */
static uint32_t old_accumulated_count = 0; /* previous accumulated value value */

/* externs */

#ifdef CONFIG_MICROKERNEL
extern struct nano_stack _k_command_stack;
#endif /* ! CONFIG_MICROKERNEL */

/*******************************************************************************
*
* _i8253CounterRead - read the i8253 counter register's value
*
* This routine reads the 16 bit value from the i8253 counter register.
*
* RETURNS: counter register's 16 bit value
*
* \NOMANUAL
*/

static inline uint16_t _i8253CounterRead(void)
{
	unsigned char lsb; /* least significant byte of counter register */
	unsigned char msb; /* most significant byte of counter register */
	uint16_t count;    /* value read from i8253 counter register */

	PLB_BYTE_REG_WRITE(0x00, PIT_CMD(PIT_BASE_ADRS));

	/* read counter 0 latched LSB value followed by MSB */
	lsb = PLB_BYTE_REG_READ(PIT_CNT0(PIT_BASE_ADRS));
	msb = PLB_BYTE_REG_READ(PIT_CNT0(PIT_BASE_ADRS));

	count = lsb | (((uint16_t)msb) << 8);
	return count;
}

/*******************************************************************************
*
* _i8253CounterSet - set the i8253 counter register's value
*
* This routine sets the 16 bit value from which the i8253 timer will
* decrement and sets that counter register to its value.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline void _i8253CounterSet(
	uint16_t count /* value from which the counter will decrement */
	)
{
	PLB_BYTE_REG_WRITE((unsigned char)(count & 0xff),
			   PIT_CNT0(PIT_BASE_ADRS));
	PLB_BYTE_REG_WRITE((unsigned char)((count >> 8) & 0xff),
			   PIT_CNT0(PIT_BASE_ADRS));
	_currentLoadVal = count;
}

/*******************************************************************************
*
* _i8253CounterPeriodic - set the i8253 timer to fire periodically
*
* This routine sets the i8253 to fire on a periodic basis.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline void _i8253CounterPeriodic(
	uint16_t count /* value from which the counter will decrement */
	)
{
	PLB_BYTE_REG_WRITE(0x36, PIT_CMD(PIT_BASE_ADRS));
	_i8253CounterSet(count);
}

#if defined(TIMER_SUPPORTS_TICKLESS)
/*******************************************************************************
*
* _i8253CounterOneShot - set the i8253 timer to fire once only
*
* This routine sets the i8253 to fire once only.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline void _i8253CounterOneShot(
	uint16_t count /* value from which the counter will decrement */
	)
{
	PLB_BYTE_REG_WRITE(0x30, PIT_CMD(PIT_BASE_ADRS));
	_i8253CounterSet(count);
}
#endif /* !TIMER_SUPPORTS_TICKLESS */

/*******************************************************************************
*
* _i8253IntHandlerPeriodic - system clock periodic tick handler
*
* This routine handles the system clock periodic tick interrupt.  A TICK_EVENT
* event is pushed onto the microkernel stack.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _timer_int_handler(void *unusedArg /* not used */
				 )
{
	ARG_UNUSED(unusedArg);

#ifdef TIMER_SUPPORTS_TICKLESS
	if (timer_mode == TIMER_MODE_PERIODIC_ENT) {
		_i8253CounterPeriodic(counterLoadVal);
		timer_mode = TIMER_MODE_PERIODIC;
	}

	/*
	 * Increment the tick because _timer_idle_exit does not account
	 * for the tick due to the timer interrupt itself. Also, if not in
	 * tickless mode,
	 * _SysIdleElpasedTicks will be 0.
	 */
	_sys_idle_elapsed_ticks++;

	/*
	 * If we transistion from 0 elapsed ticks to 1 we need to announce the
	 * tick event to the microkernel. Other cases will have already been
	 * covered by _timer_idle_exit
	 */

	if (_sys_idle_elapsed_ticks == 1) {
		_sys_clock_tick_announce();
	}

	/* accumulate total counter value */
	clock_accumulated_count += counterLoadVal * _sys_idle_elapsed_ticks;

#else
#if defined(CONFIG_MICROKERNEL)
	_sys_clock_tick_announce();
#endif

	/* accumulate total counter value */
	clock_accumulated_count += counterLoadVal;
#endif /* TIMER_SUPPORTS_TICKLESS */

	/*
	 * Algorithm tries to compensate lost interrupts if any happened and
	 * prevent the timer from counting backwards
	 * ULONG_MAX / 2 is the maximal value that old_count can be more than
	 * clock_accumulated_count. If it is more -- consider it as an clock_accumulated_count
	 * wrap and do not try to compensate.
	 */
	if (clock_accumulated_count < old_count) {
		uint32_t tmp = old_count - clock_accumulated_count;
		if ((tmp >= counterLoadVal) && (tmp < (ULONG_MAX / 2))) {
			clock_accumulated_count += tmp - tmp % counterLoadVal;
		}
	}

#if defined(CONFIG_NANOKERNEL)
	_sys_clock_tick_announce();
#endif /*  CONFIG_NANOKERNEL */
}

#if defined(TIMER_SUPPORTS_TICKLESS)
/*******************************************************************************
*
* _i8253TicklessIdleInit - initialize the tickless idle feature
*
* This routine initializes the tickless idle feature.  Note that maximum
* number of ticks that can elapse during a "tickless idle" is limited by
* <counterLoadVal>.  The larger the value (the lower the tick frequency), the
* fewer elapsed ticks during a "tickless idle".  Conversely, the smaller the
* value (the higher the tick frequency), the more elapsed ticks during a
* "tickless idle".
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _i8253TicklessIdleInit(void)
{
	max_system_ticks = 0xffff / counterLoadVal;
	/* this gives a count that gives the max number of full ticks */
	max_load_value = max_system_ticks * counterLoadVal;
}

/*******************************************************************************
*
* _i8253TicklessIdleSkew -
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _i8253TicklessIdleSkew(void)
{
	/* TBD */
	timer_idle_skew = 0;
}

/*******************************************************************************
*
* _timer_idle_enter - Place system timer into idle state
*
* Re-program the timer to enter into the idle state for the given number of
* ticks. It is placed into one shot mode where it will fire in the number of
* ticks supplied or the maximum number of ticks that can be programmed into
* hardware. A value of -1 means inifinite number of ticks.
*/

void _timer_idle_enter(int32_t ticks /* system ticks */
				)
{
	uint16_t newCount;

	/*
	 * We're being asked to have the timer fire in "ticks" from now. To
	 * maintain accuracy we must account for the remain time left in the
	 * timer. So we read the count out of it and add it to the requested
	 * time out
	 */
	newCount = _i8253CounterRead();

	if (ticks == -1 || ticks > max_system_ticks) {
		/*
		 * We've been asked to fire the timer so far in the future that
		 * the
		 * required count value would not fit in the 16 bit counter
		 * register.
		 * Instead, we program for the maximum programmable interval
		 * minus one
		 * system tick to prevent overflow when the left over count read
		 * earlier
		 * is added.
		 */
		newCount += max_load_value - counterLoadVal;
		idle_original_ticks = max_system_ticks - 1;
	} else {
		/* leave one tick of buffer to have to time react when coming
		 * back ? */
		idle_original_ticks = ticks - 1;
		newCount += idle_original_ticks * counterLoadVal;
	}

	idle_original_count = newCount - timer_idle_skew;

	/* Stop/start the timer instead of disabling/enabling the interrupt? */
	irq_disable(PIT_INT_LVL);

	timer_mode = TIMER_MODE_PERIODIC_ENT;

	/* Program for terminal mode. The PIT equivalent of one shot */
	_i8253CounterOneShot(newCount);

	irq_enable(PIT_INT_LVL);
}

/*******************************************************************************
*
* _timer_idle_exit - handling of tickless idle when interrupted
*
* The routine is responsible for taking the timer out of idle mode and
* generating an interrupt at the next tick interval.
*
* Note that in this routine, _SysTimerElapsedTicks must be zero because the
* ticker has done its work and consumed all the ticks. This has to be true
* otherwise idle mode wouldn't have been entered in the first place.
*
* Called in _IntEnt()
*
* RETURNS: N/A
*/

void _timer_idle_exit(void)
{
	uint16_t count; /* current value in i8253 counter register */

	/* timer is in idle or off mode, adjust the ticks expired */

	/* request counter 0 to be latched */
	count = _i8253CounterRead();

	if ((count == 0) || (count >= idle_original_count)) {
		/* Timer expired. Place back in periodic mode */
		_i8253CounterPeriodic(counterLoadVal);
		timer_mode = TIMER_MODE_PERIODIC;
		_sys_idle_elapsed_ticks = idle_original_ticks - 1;
		/*
		 * Announce elapsed ticks to the microkernel. Note we are
		 * guaranteed
		 * that the timer ISR will execute first before the tick event
		 * is
		 * serviced.
		 */
		_sys_clock_tick_announce();
	} else {
		uint16_t elapsed;   /* elapsed "counter time" */
		uint16_t remaining; /* remaing "counter time" */

		elapsed = idle_original_count - count;

		remaining = elapsed % counterLoadVal;

		/* switch timer to periodic mode */
		if (remaining == 0) {
			_i8253CounterPeriodic(counterLoadVal);
			timer_mode = TIMER_MODE_PERIODIC;
		} else if (count > remaining) {
			/* less time remaining to the next tick than was
			 * programmed */
			_i8253CounterOneShot(remaining);
		}

		_sys_idle_elapsed_ticks = elapsed / counterLoadVal;

		if (_sys_idle_elapsed_ticks) {
			/* Announce elapsed ticks to the microkernel */
			_sys_clock_tick_announce();
		}
	}
}
#endif /* !TIMER_SUPPORTS_TICKLESS */

/*******************************************************************************
*
* timer_driver - initialize and enable the system clock
*
* This routine is used to program the PIT to deliver interrupts at the
* rate specified via the 'sys_clock_us_per_tick' global variable.
*
* RETURNS: N/A
*/

void timer_driver(int priority /* priority parameter ignored by this driver */
		  )
{
	ARG_UNUSED(priority);

	/* determine the PIT counter value (in timer clock cycles/system tick)
	 */

	counterLoadVal = sys_clock_hw_cycles_per_tick;

	_i8253TicklessIdleInit();

	/* init channel 0 to generate interrupt at the rate of SYS_CLOCK_RATE */

	_i8253CounterPeriodic(counterLoadVal);

#ifdef CONFIG_DYNAMIC_INT_STUBS
	/* connect specified routine/parameter to PIT interrupt vector */

	(void)irq_connect(PIT_INT_LVL,
				PIT_INT_PRI,
				_i8253IntHandlerPeriodic,
				0,
				_i8253_interrupt_stub);
#endif /* CONFIG_DYNAMIC_INT_STUBS */

	_i8253TicklessIdleSkew();

#if defined(CONFIG_MICROKERNEL)

/* timer_read() is available for microkernel libraries to call directly */

/* K_ticker() is the pre-defined event handler for TICK_EVENT */

#endif /* CONFIG_MICROKERNEL */

	/* Everything has been configured. It is now safe to enable the
	 * interrupt */
	irq_enable(PIT_INT_LVL);
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
	unsigned int key;  /* interrupt lock level */
	uint32_t newCount; /* new system clock value */

#ifdef CONFIG_INT_LATENCY_BENCHMARK
/*
 * Expending irq_lock_inline() code since directly calling it would
 * would end up in infinite recursion.
 */
	key = _do_irq_lock_inline();
#else
	key = irq_lock_inline();
#endif

	/* counters are down counters so need to subtact from counterLoadVal */
	newCount = clock_accumulated_count + _currentLoadVal - _i8253CounterRead();

	/*
	 * This algorithm fixes the situation when the timer counter reset
	 * happened before the timer interrupt (due to possible interrupt
	 * disable)
	 */
	if ((newCount < old_count) && (clock_accumulated_count == old_accumulated_count)) {
		uint32_t tmp = old_count - newCount;
		newCount += tmp - tmp % _currentLoadVal + _currentLoadVal;
	}

	old_count = newCount;
	old_accumulated_count = clock_accumulated_count;

#ifdef CONFIG_INT_LATENCY_BENCHMARK
/*
 * Expending irq_unlock_inline() code since directly calling it would
 * would end up in infinite recursion.
 */
	if (key & 0x200)
		_do_irq_unlock_inline();
#else
	irq_unlock_inline(key);
#endif

	return newCount;
}

#if defined(CONFIG_SYSTEM_TIMER_DISABLE)
/*******************************************************************************
*
* timer_disable - stop announcing ticks into the kernel
*
* This routine simply disables the PIT counter such that interrupts are no
* longer delivered.
*
* RETURNS: N/A
*/

void timer_disable(void)
{
	unsigned int key; /* interrupt lock level */

	key = irq_lock();

	PLB_BYTE_REG_WRITE(0x38, PIT_CMD(PIT_BASE_ADRS));
	PLB_BYTE_REG_WRITE(0, PIT_CNT0(PIT_BASE_ADRS));
	PLB_BYTE_REG_WRITE(0, PIT_CNT0(PIT_BASE_ADRS));

	irq_unlock(key);

	/* disable interrupt in the interrupt controller */

	irq_disable(PIT_INT_LVL);
}
#endif /* CONFIG_SYSTEM_TIMER_DISABLE */
