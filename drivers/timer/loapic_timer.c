/* loApicTimer.c - Intel Local APIC driver */

/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
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
This module implements a kernel device driver for the Intel local APIC
device, and provides the standard "system clock driver" interfaces.
This library contains routines for the timer in the Intel local APIC/xAPIC
(Advanced Programmable Interrupt Controller) in P6 (PentiumPro, II, III)
and P7 (Pentium4) family processor.
The local APIC contains a 32-bit programmable timer for use by the local
processor.  The time base is derived from the processor's bus clock,
divided by a value specified in the divide configuration register.
After reset, the timer is initialized to zero.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>
#include <drivers/loapic.h> /* LOAPIC registers */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#endif /* CONFIG_MICROKERNEL */

/*
 * A board support package's board.h header must provide definitions for the
 * following constants:
 *
 *    LOAPIC_BASE_ADRS
 *    LOAPIC_TIMER_IRQ
 *    LOAPIC_TIMER_INT_PRI
 *
 * NOTE: CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC must be provided by either the
 * platform configuration file or the project configuration file.
 */

#include <board.h>

/* Local APIC Timer Bits */

#define LOAPIC_TIMER_DIVBY_2 0x0	 /* Divide by 2 */
#define LOAPIC_TIMER_DIVBY_4 0x1	 /* Divide by 4 */
#define LOAPIC_TIMER_DIVBY_8 0x2	 /* Divide by 8 */
#define LOAPIC_TIMER_DIVBY_16 0x3	/* Divide by 16 */
#define LOAPIC_TIMER_DIVBY_32 0x8	/* Divide by 32 */
#define LOAPIC_TIMER_DIVBY_64 0x9	/* Divide by 64 */
#define LOAPIC_TIMER_DIVBY_128 0xa       /* Divide by 128 */
#define LOAPIC_TIMER_DIVBY_1 0xb	 /* Divide by 1 */
#define LOAPIC_TIMER_DIVBY_MASK 0xf      /* mask bits */
#define LOAPIC_TIMER_PERIODIC 0x00020000 /* Timer Mode: Periodic */


#if defined(CONFIG_MICROKERNEL) && defined(CONFIG_TICKLESS_IDLE)
#define TIMER_SUPPORTS_TICKLESS
#endif /* CONFIG_MICROKERNEL && CONFIG_TICKLESS_IDLE */

/* Helpful macros and inlines for programming timer */
#define _REG_TIMER ((volatile uint32_t *)(LOAPIC_BASE_ADRS + LOAPIC_TIMER))
#define _REG_TIMER_ICR \
	((volatile uint32_t *)(LOAPIC_BASE_ADRS + LOAPIC_TIMER_ICR))
#define _REG_TIMER_CCR \
	((volatile uint32_t *)(LOAPIC_BASE_ADRS + LOAPIC_TIMER_CCR))
#define _REG_TIMER_CFG \
	((volatile uint32_t *)(LOAPIC_BASE_ADRS + LOAPIC_TIMER_CONFIG))

#if defined(TIMER_SUPPORTS_TICKLESS)
#define TIMER_MODE_PERIODIC 0
#define TIMER_MODE_PERIODIC_ENT 1
#else /* !TIMER_SUPPORTS_TICKLESS */
#define _loApicTimerTicklessIdleInit() \
	do {/* nothing */              \
	} while (0)
#define _loApicTimerTicklessIdleSkew() \
	do {/* nothing */              \
	} while (0)
#endif /* !TIMER_SUPPORTS_TICKLESS */
#if defined(TIMER_SUPPORTS_TICKLESS)
extern int32_t _sys_idle_elapsed_ticks;
#endif /* TIMER_SUPPORTS_TICKLESS */

IRQ_CONNECT_STATIC(loapic, LOAPIC_TIMER_IRQ, LOAPIC_TIMER_INT_PRI,
		   _timer_int_handler, 0);

static uint32_t __noinit counterLoadVal; /* computed counter 0
							  initial count value */
static uint32_t clock_accumulated_count = 0;

#if defined(TIMER_SUPPORTS_TICKLESS)
static uint32_t idle_original_count = 0;
static uint32_t __noinit max_system_ticks;
static uint32_t idle_original_ticks = 0;
static uint32_t __noinit max_load_value;
static uint32_t __noinit timer_idle_skew;
static unsigned char timer_mode = TIMER_MODE_PERIODIC;
#endif /* TIMER_SUPPORTS_TICKLESS */

/* externs */

#ifdef CONFIG_MICROKERNEL
extern struct nano_stack _k_command_stack;
#endif /* CONFIG_MICROKERNEL */

/**
 *
 * @brief Set the timer for periodic mode
 *
 * This routine sets the timer for periodic mode.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void _loApicTimerPeriodic(void)
{
	*_REG_TIMER |= LOAPIC_TIMER_PERIODIC;
}

#if defined(TIMER_SUPPORTS_TICKLESS) ||              \
	defined(LOAPIC_TIMER_PERIODIC_WORKAROUND) || \
	defined(CONFIG_SYSTEM_TIMER_DISABLE)
/**
 *
 * @brief Stop the timer
 *
 * This routine stops the timer.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void _loApicTimerStop(void)
{
	*_REG_TIMER |= LOAPIC_LVT_MASKED;
}
#endif

#if defined(TIMER_SUPPORTS_TICKLESS) || \
	defined(LOAPIC_TIMER_PERIODIC_WORKAROUND)
/**
 *
 * @brief Start the timer
 *
 * This routine starts the timer.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void _loApicTimerStart(void)
{
	*_REG_TIMER &= ~LOAPIC_LVT_MASKED;
}
#endif

/**
 *
 * @brief Set countdown value
 *
 * This routine sets value from which the timer will count down.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void _loApicTimerSetCount(
	uint32_t count /* count from which timer is to count down */
	)
{
	*_REG_TIMER_ICR = count;
}

#if defined(TIMER_SUPPORTS_TICKLESS)
/**
 *
 * @brief Set the timer for one shot mode
 *
 * This routine sets the timer for one shot mode.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void _loApicTimerOneShot(void)
{
	*_REG_TIMER &= ~LOAPIC_TIMER_PERIODIC;
}
#endif /* TIMER_SUPPORTS_TICKLESS */

/**
 *
 * @brief Set the rate at which the timer is decremented
 *
 * This routine sets rate at which the timer is decremented to match the
 * external bus frequency.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void _loApicTimerSetDivider(void)
{
	*_REG_TIMER_CFG = (*_REG_TIMER_CFG & ~0xf) | LOAPIC_TIMER_DIVBY_1;
}

/**
 *
 * @brief Get the value from the current count register
 *
 * This routine gets the value from the timer's current count register.  This
 * value is the 'time' remaining to decrement before the timer triggers an
 * interrupt.
 *
 * @return N/A
 *
 * \NOMANUAL
 */
static inline uint32_t _loApicTimerGetRemaining(void)
{
	return *_REG_TIMER_CCR;
}

#if defined(TIMER_SUPPORTS_TICKLESS)
/**
 *
 * @brief Get the value from the initial count register
 *
 * This routine gets the value from the initial count register.
 *
 * @return N/A
 *
 * \NOMANUAL
 */
static inline uint32_t _loApicTimerGetCount(void)
{
	return *_REG_TIMER_ICR;
}
#endif /* TIMER_SUPPORTS_TICKLESS */

/**
 *
 * @brief System clock tick handler
 *
 * This routine handles the system clock tick interrupt.  A TICK_EVENT event
 * is pushed onto the microkernel stack.
 *
 * @return N/A
 */

void _timer_int_handler(void *unused /* parameter is not used */
				 )
{
	ARG_UNUSED(unused);

#ifdef TIMER_SUPPORTS_TICKLESS
	if (timer_mode == TIMER_MODE_PERIODIC_ENT) {
		_loApicTimerStop();
		_loApicTimerPeriodic();
		_loApicTimerSetCount(counterLoadVal);
		_loApicTimerStart();
		timer_mode = TIMER_MODE_PERIODIC;
	}

	/*
	 * Increment the tick because _timer_idle_exit does not account
	 * for the tick due to the timer interrupt itself. Also, if not in
	 * tickless mode,
	 * _SysIdleElpasedTicks will be 0.
	 */
	_sys_idle_elapsed_ticks++;

	/* accumulate total counter value */
	clock_accumulated_count += counterLoadVal * _sys_idle_elapsed_ticks;

	/*
	 * If we transistion from 0 elapsed ticks to 1 we need to announce the
	 * tick event to the microkernel. Other cases will have already been
	 * covered by _timer_idle_exit
	 */

	if (_sys_idle_elapsed_ticks == 1) {
		_sys_clock_tick_announce();
	}

#else
	/* accumulate total counter value */
	clock_accumulated_count += counterLoadVal;

#if defined(CONFIG_MICROKERNEL)
	_sys_clock_tick_announce();
#endif

#endif /*TIMER_SUPPORTS_TICKLESS*/

#if defined(CONFIG_NANOKERNEL)
	_sys_clock_tick_announce();
#endif /*  CONFIG_NANOKERNEL */

#ifdef LOAPIC_TIMER_PERIODIC_WORKAROUND
	/*
	 * On platforms where the LOAPIC timer periodic mode is broken,
	 * re-program
	 * the ICR register with the initial count value.  This is only a
	 * temporary
	 * workaround.
	 */

	_loApicTimerStop();
	_loApicTimerPeriodic();
	_loApicTimerSetCount(counterLoadVal);
	_loApicTimerStart();
#endif /* LOAPIC_TIMER_PERIODIC_WORKAROUND */
}

#if defined(TIMER_SUPPORTS_TICKLESS)
/**
 *
 * @brief Initialize the tickless idle feature
 *
 * This routine initializes the tickless idle feature.  Note that the maximum
 * number of ticks that can elapse during a "tickless idle" is limited by
 * <counterLoadVal>.  The larger the value (the lower the tick frequency), the
 * fewer elapsed ticks during a "tickless idle".  Conversely, the smaller the
 * value (the higher the tick frequency), the more elapsed ticks during a
 * "tickless idle".
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static void _loApicTimerTicklessIdleInit(void)
{
	max_system_ticks = 0xffffffff / counterLoadVal;
	/* this gives a count that gives the max number of full ticks */
	max_load_value = max_system_ticks * counterLoadVal;
}

/**
 *
 * @brief Calculate the skew from idle mode switching
 *
 * This routine calculates the skew from switching the timer in and out of idle
 * mode.  The typical sequence is:
 *    1. Stop timer.
 *    2. Load new counter value.
 *    3. Set timer mode to periodic/one-shot
 *    4. Start timer.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static void _loApicTimerTicklessIdleSkew(void)
{
	volatile uint32_t dummy; /* used to replicate the 'skew time' */

	/* Timer must be running for this to work */
	timer_idle_skew = _loApicTimerGetRemaining();

	_loApicTimerStart(); /* This is normally a stop operation */
	dummy = _loApicTimerGetRemaining(); /*_loApicTimerSetCount
					       (counterLoadVal);*/
	_loApicTimerPeriodic();
	_loApicTimerStart();
	timer_mode = TIMER_MODE_PERIODIC;

	/* Down counter */
	timer_idle_skew -= _loApicTimerGetRemaining();
}

/**
 *
 * @brief Place system timer into idle state
 *
 * Re-program the timer to enter into the idle state for the given number of
 * ticks. It is placed into one shot mode where it will fire in the number of
 * ticks supplied or the maximum number of ticks that can be programmed into
 * hardware. A value of -1 means inifinite number of ticks.
 *
 * @return N/A
 */

void _timer_idle_enter(int32_t ticks /* system ticks */
				)
{
	_loApicTimerStop();
	/*
	 * We're being asked to have the timer fire in "ticks" from now. To
	 * maintain accuracy we must account for the remaining time left in the
	 * timer. So we read the count out of it and add it to the requested
	 * time out
	 */
	idle_original_count = _loApicTimerGetRemaining() - timer_idle_skew;

	if ((ticks == -1) || (ticks > max_system_ticks)) {
		/*
		 * We've been asked to fire the timer so far in the future that
		 * the
		 * required count value would not fit in the 32 bit counter
		 * register.
		 * Instead, we program for the maximum programmable interval
		 * minus one
		 * system tick to prevent overflow when the left over count read
		 * earlier
		 * is added.
		 */
		idle_original_count += max_load_value - counterLoadVal;
		idle_original_ticks = max_system_ticks - 1;
	} else {
		/* leave one tick of buffer to have to time react when coming
		 * back ? */
		idle_original_ticks = ticks - 1;
		idle_original_count += idle_original_ticks * counterLoadVal;
	}

	timer_mode = TIMER_MODE_PERIODIC_ENT;

	/* Set timer to one shot mode */
	_loApicTimerOneShot();
	_loApicTimerSetCount(idle_original_count);
	_loApicTimerStart();
}

/**
 *
 * @brief Handling of tickless idle when interrupted
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
 * @return N/A
 */

void _timer_idle_exit(void)
{
	uint32_t count; /* timer's current count register value */

	_loApicTimerStop();

	/* timer is in idle or off mode, adjust the ticks expired */

	count = _loApicTimerGetRemaining();

	if ((count == 0) || (count >= idle_original_count)) {
		/* Timer expired and/or wrapped around. Place back in periodic
		 * mode */
		_loApicTimerPeriodic();
		_loApicTimerSetCount(counterLoadVal);
		_sys_idle_elapsed_ticks = idle_original_ticks - 1;
		timer_mode = TIMER_MODE_PERIODIC;
		/*
		 * Announce elapsed ticks to the microkernel. Note we are
		 * guaranteed
		 * that the timer ISR will execute first before the tick event
		 * is
		 * serviced.
		 */
		_sys_clock_tick_announce();
	} else {
		uint32_t elapsed;   /* elapsed "counter time" */
		uint32_t remaining; /* remaining "counter time" */

		elapsed = idle_original_count - count;

		remaining = elapsed % counterLoadVal;

		/* switch timer to periodic mode */
		if (remaining == 0) {
			_loApicTimerPeriodic();
			_loApicTimerSetCount(counterLoadVal);
			timer_mode = TIMER_MODE_PERIODIC;
		} else if (count > remaining) {
			/* less time remaining to the next tick than was
			 * programmed. Leave in one shot mode */
			_loApicTimerSetCount(remaining);
		}

		_sys_idle_elapsed_ticks = elapsed / counterLoadVal;

		if (_sys_idle_elapsed_ticks) {
			_sys_clock_tick_announce();
		}
	}
	_loApicTimerStart();
}
#endif /* TIMER_SUPPORTS_TICKLESS */

/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the timer to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */

int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	/* determine the timer counter value (in timer clock cycles/system tick)
	 */

	counterLoadVal = sys_clock_hw_cycles_per_tick - 1;

	_loApicTimerTicklessIdleInit();

	_loApicTimerSetDivider();
	_loApicTimerSetCount(counterLoadVal);
	_loApicTimerPeriodic();

	/*
	 * Although the stub has already been "connected", the vector number
	 * still
	 * has to be programmed into the interrupt controller.
	 */
	IRQ_CONFIG(loapic, LOAPIC_TIMER_IRQ);

	_loApicTimerTicklessIdleSkew();

	/* Everything has been configured. It is now safe to enable the
	 * interrupt */
	irq_enable(LOAPIC_TIMER_IRQ);

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

uint32_t timer_read(void)
{
	uint32_t val; /* system clock value */

#if !defined(TIMER_SUPPORTS_TICKLESS)
	/* counter is a down counter so need to subtact from counterLoadVal */
	val = clock_accumulated_count - _loApicTimerGetRemaining() + counterLoadVal;
#else
	/*
	 * counter is a down counter so need to subtact from what was programmed
	 * in the reload register
	 */
	val = clock_accumulated_count - _loApicTimerGetRemaining() +
	      _loApicTimerGetCount();
#endif

	return val;
}

#if defined(CONFIG_SYSTEM_TIMER_DISABLE)
/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine simply disables the LOAPIC counter such that interrupts are no
 * longer delivered.
 *
 * @return N/A
 */

void timer_disable(void)
{
	unsigned int key; /* interrupt lock level */

	key = irq_lock();

	_loApicTimerStop();
	_loApicTimerSetCount(0);

	irq_unlock(key);

	/* disable interrupt in the interrupt controller */

	irq_disable(LOAPIC_TIMER_IRQ);
}

#endif /* CONFIG_SYSTEM_TIMER_DISABLE */
