/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel HPET device driver
 *
 * This module implements a kernel device driver for the Intel High Precision
 * Event Timer (HPET) device, and provides the standard "system clock driver"
 * interfaces.
 *
 * The driver utilizes HPET timer0 to provide kernel ticks.
 *
 * \INTERNAL IMPLEMENTATION DETAILS
 * The HPET device driver makes no assumption about the initial state of the
 * HPET, and explicitly puts the device into a reset-like state. It also assumes
 * that the main up counter never wraps around to 0 during the lifetime of the
 * system.
 *
 * The platform can configure the HPET to use level rather than the default edge
 * sensitive interrupts by enabling the following configuration parameters:
 * CONFIG_HPET_TIMER_LEVEL_HIGH or CONFIG_HPET_TIMER_LEVEL_LOW
 *
 * When not configured to support tickless idle timer0 is programmed in periodic
 * mode so it automatically generates a single interrupt per kernel tick
 * interval.
 *
 * When configured to support tickless idle timer0 is programmed in one-shot
 * mode.  When the CPU is not idling the timer interrupt handler sets the timer
 * to expire when the next kernel tick is due, waits for this to occur, and then
 * repeats this "ad infinitum". When the CPU begins idling the timer driver
 * reprograms the expiry time for the timer (thereby overriding the previously
 * scheduled timer interrupt) and waits for the timer to expire or for a
 * non-timer interrupt to occur. When the CPU ceases idling the driver
 * determines how many complete ticks have elapsed, reprograms the timer so that
 * it expires on the next tick, and announces the number of elapsed ticks (if
 * any) to the kernel.
 *
 */

#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sys_clock.h>
#include <drivers/ioapic.h>
#include <drivers/system_timer.h>
#include <kernel_structs.h>

#include <board.h>

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

#define _HPET_GENERAL_CAPS ((volatile u64_t *) \
			(CONFIG_HPET_TIMER_BASE_ADDRESS + GENERAL_CAPS_REG))

/*
 * Although the general configuration register is 64-bits, only a 32-bit access
 * is performed since the most significant bits contain no useful information.
 */

#define _HPET_GENERAL_CONFIG ((volatile u32_t *) \
			(CONFIG_HPET_TIMER_BASE_ADDRESS + GENERAL_CONFIG_REG))

/*
 * Although the general interrupt status is 64-bits, only a 32-bit access
 * is performed since this driver only utilizes timer0
 * (i.e. there is no need to determine the interrupt status of other timers).
 */

#define _HPET_GENERAL_INT_STATUS ((volatile u32_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + GENERAL_INT_STATUS_REG))

#define _HPET_MAIN_COUNTER_VALUE ((volatile u64_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + MAIN_COUNTER_VALUE_REG))
#define _HPET_MAIN_COUNTER_LSW ((volatile u32_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + MAIN_COUNTER_VALUE_REG))
#define _HPET_MAIN_COUNTER_MSW ((volatile u32_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + MAIN_COUNTER_VALUE_REG + 0x4))

#define _HPET_TIMER0_CONFIG_CAPS ((volatile u64_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + TIMER0_CONFIG_CAPS_REG))
#define _HPET_TIMER0_COMPARATOR ((volatile u64_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + TIMER0_COMPARATOR_REG))
#define _HPET_TIMER0_FSB_INT_ROUTE ((volatile u64_t *) \
		(CONFIG_HPET_TIMER_BASE_ADDRESS + TIMER0_FSB_INT_ROUTE_REG))

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

#if defined(CONFIG_HPET_TIMER_FALLING_EDGE)
#define HPET_IOAPIC_FLAGS  (IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_HPET_TIMER_RISING_EDGE)
#define HPET_IOAPIC_FLAGS  (IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_HPET_TIMER_LEVEL_HIGH)
#define HPET_IOAPIC_FLAGS  (IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_HPET_TIMER_LEVEL_LOW)
#define HPET_IOAPIC_FLAGS  (IOAPIC_LEVEL | IOAPIC_LOW)
#endif


#ifdef CONFIG_INT_LATENCY_BENCHMARK
static u32_t main_count_first_irq_value;
static u32_t main_count_expected_value;
extern u32_t _hw_irq_to_c_handler_latency;
#endif

#ifdef CONFIG_HPET_TIMER_DEBUG
#include <misc/printk.h>
#define DBG(...) printk(__VA_ARGS__)
#else
#define DBG(...)
#endif

#ifdef CONFIG_TICKLESS_IDLE

/* additional globals, locals, and forward declarations */

extern s32_t _sys_idle_elapsed_ticks;

/* main counter units per system tick */
static u32_t __noinit counter_load_value;
/* counter value for most recent tick */
static u64_t counter_last_value;
/* # ticks timer is programmed for */
static s32_t programmed_ticks = 1;
/* is stale interrupt possible? */
static int stale_irq_check;

/**
 *
 * @brief Safely read the main HPET up counter
 *
 * This routine simulates an atomic read of the 64-bit system clock on CPUs
 * that only support 32-bit memory accesses. The most significant word
 * of the counter is read twice to ensure it doesn't change while the least
 * significant word is being retrieved (as per HPET documentation).
 *
 * @return current 64-bit counter value
 */
static u64_t _hpetMainCounterAtomic(void)
{
	u32_t highBits;
	u32_t lowBits;

	do {
		highBits = *_HPET_MAIN_COUNTER_MSW;
		lowBits = *_HPET_MAIN_COUNTER_LSW;
	} while (highBits != *_HPET_MAIN_COUNTER_MSW);

	return ((u64_t)highBits << 32) | lowBits;
}

#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_TICKLESS_KERNEL
static inline void program_max_cycles(void)
{
	stale_irq_check = 1;
	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	counter_last_value = *_HPET_TIMER0_COMPARATOR;
	*_HPET_TIMER0_COMPARATOR = counter_last_value - 1;
}
#endif

/**
 *
 * @brief System clock tick handler
 *
 * This routine handles the system clock tick interrupt. A TICK_EVENT event
 * is pushed onto the kernel stack.
 *
 * @return N/A
 */
void _timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);

#if defined(CONFIG_HPET_TIMER_LEVEL_LOW) || defined(CONFIG_HPET_TIMER_LEVEL_HIGH)
	/* Acknowledge interrupt */
	*_HPET_GENERAL_INT_STATUS = 1;
#endif

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	u32_t delta = *_HPET_MAIN_COUNTER_VALUE - main_count_expected_value;

	if (_hw_irq_to_c_handler_latency > delta) {
		/* keep the lowest value observed */
		_hw_irq_to_c_handler_latency = delta;
	}
	/* compute the next expected main counter value */
	main_count_expected_value += main_count_first_irq_value;
#endif


#ifndef CONFIG_TICKLESS_IDLE

	/*
	 * one more tick has occurred -- don't need to do anything special since
	 * timer is already configured to interrupt on the following tick
	 */

	_sys_clock_tick_announce();

#else

	/* see if interrupt was triggered while timer was being reprogrammed */

#if defined(CONFIG_TICKLESS_KERNEL)
	/* If timer not programmed or already consumed exit */
	if (!programmed_ticks) {
		if (_sys_clock_always_on) {
			_sys_clock_tick_count = _get_elapsed_clock_time();
			program_max_cycles();
		}
		return;
	}
#endif
	if (stale_irq_check) {
		stale_irq_check = 0;
		if (_hpetMainCounterAtomic() < *_HPET_TIMER0_COMPARATOR) {
			return; /* ignore "stale" interrupt */
		}
	}

	/* configure timer to expire on next tick for tick based kernel */

#if defined(CONFIG_TICKLESS_KERNEL)

	_sys_idle_elapsed_ticks = programmed_ticks;

	/*
	 * Clear programmed ticks before announcing elapsed time so
	 * that recursive calls to _update_elapsed_time() will not
	 * announce already consumed elapsed time
	 */
	programmed_ticks = 0;
	_sys_clock_tick_announce();

	/* _sys_clock_tick_announce() could cause new programming */
	if (!programmed_ticks && _sys_clock_always_on) {
		_sys_clock_tick_count = _get_elapsed_clock_time();
		program_max_cycles();
	}
#else
	counter_last_value = *_HPET_TIMER0_COMPARATOR;
	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR = counter_last_value + counter_load_value;
	programmed_ticks = 1;
	_sys_clock_final_tick_announce();
#endif
#endif /* !CONFIG_TICKLESS_IDLE */

}

#ifdef CONFIG_TICKLESS_KERNEL
u32_t _get_program_time(void)
{
	return programmed_ticks;
}

u32_t _get_remaining_program_time(void)
{
	if (programmed_ticks == 0) {
		return 0;
	}

	return (u32_t) ((s64_t)
			  (*_HPET_TIMER0_COMPARATOR -
			   _hpetMainCounterAtomic()) / counter_load_value);
}

u32_t _get_elapsed_program_time(void)
{
	if (programmed_ticks == 0) {
		return 0;
	}

	return (u32_t) (programmed_ticks -
		       ((s64_t)(*_HPET_TIMER0_COMPARATOR -
			 _hpetMainCounterAtomic()) / counter_load_value));
}

void _set_time(u32_t time)
{
	/* Assumes cycles in one time unit is greater than HPET_COMP_DELAY */

	if (!time) {
		programmed_ticks = 0;
		return;
	}

	programmed_ticks = time;

	_sys_clock_tick_count = _get_elapsed_clock_time();

	stale_irq_check = 1;

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	counter_last_value = _hpetMainCounterAtomic();
	*_HPET_TIMER0_COMPARATOR =
	    counter_last_value + time * counter_load_value;
}

void _enable_sys_clock(void)
{
	if (!programmed_ticks) {
		program_max_cycles();
	}
}

u64_t _get_elapsed_clock_time(void)
{
	u64_t elapsed;

	elapsed = _sys_clock_tick_count;
	elapsed +=  ((s64_t)(_hpetMainCounterAtomic() -
			counter_last_value) / counter_load_value);

	return elapsed;
}
#endif

#ifdef CONFIG_TICKLESS_IDLE

/*
 * Ensure that _timer_idle_enter() is never asked to idle for fewer than 2
 * ticks (since this might require the timer to be reprogrammed for a deadline
 * too close to the current time, resulting in a missed interrupt which would
 * permanently disable the tick timer)
 */

#if (CONFIG_TICKLESS_IDLE_THRESH < 2)
#error Tickless idle threshold is too small (must be at least 2)
#endif

/**
 *
 * @brief Place system timer into idle state
 *
 * Re-program the timer to enter into the idle state for the given number of
 * ticks (-1 means infinite number of ticks).
 *
 * @return N/A
 *
 * \INTERNAL IMPLEMENTATION DETAILS
 * Called while interrupts are locked.
 */

void _timer_idle_enter(s32_t ticks /* system ticks */
				)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (ticks != K_FOREVER) {
		/* Need to reprogram only if current program is smaller */
		if (ticks > programmed_ticks) {
			_set_time(ticks);
		}
	} else {
		programmed_ticks = 0;
		counter_last_value = *_HPET_TIMER0_COMPARATOR;
		*_HPET_GENERAL_CONFIG &= ~HPET_ENABLE_CNF;
	}
#else
	/*
	 * reprogram timer to expire at the desired time (which is guaranteed
	 * to be at least one full tick from the current counter value)
	 */

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR =
		(ticks >= 0) ? counter_last_value + ticks * counter_load_value
			     : ~(u64_t)0;
	programmed_ticks = ticks;
#endif
	stale_irq_check = 1;
}

/**
 *
 * @brief Take system timer out of idle state
 *
 * Determine how long timer has been idling and reprogram it to interrupt at the
 * next tick.
 *
 * Note that in this routine, _sys_idle_elapsed_ticks must be zero because the
 * ticker has done its work and consumed all the ticks. This has to be true
 * otherwise idle mode wouldn't have been entered in the first place.
 *
 * @return N/A
 *
 */

void _timer_idle_exit(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (!programmed_ticks && _sys_clock_always_on) {
		program_max_cycles();
	}
#else
	u64_t currTime = _hpetMainCounterAtomic();
	s32_t elapsedTicks;
	u64_t counterNextValue;

	/* see if idling ended because timer expired at the desired tick */

	if (currTime >= *_HPET_TIMER0_COMPARATOR) {
		/*
		 * update # of ticks since last tick event was announced,
		 * so that this value is available to ISRs that run before the
		 * timer interrupt handler runs (which is unlikely, but could
		 * happen)
		 */

		_sys_idle_elapsed_ticks = programmed_ticks - 1;

		/*
		 * Announce elapsed ticks to the kernel. Note we are guaranteed
		 * that the timer ISR will execute first before the tick event
		 * is serviced.
		 */
		_sys_clock_tick_announce();

		/* timer interrupt handler reprograms the timer for the next
		 * tick
		 */

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
	 * the kernel, which gets informed of the correct number of elapsed
	 * ticks when the following tick finally occurs; however, any ISRs that
	 * access _sys_idle_elapsed_ticks to determine the current time may be
	 * misled during the (very brief) interval before the tick-in-progress
	 * finishes and the following tick begins
	 */

	elapsedTicks =
		(s32_t)((currTime - counter_last_value) / counter_load_value);
	counter_last_value += (u64_t)elapsedTicks * counter_load_value;

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
		/* Announce elapsed ticks to the kernel */
		_sys_clock_tick_announce();
	}

	/*
	 * Any elapsed ticks have been accounted for so simply set the
	 * programmed ticks to 1 since the timer has been programmed to fire on
	 * the next tick boundary.
	 */

	programmed_ticks = 1;
#endif
}

#endif /* CONFIG_TICKLESS_IDLE */

/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the HPET to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */

int _sys_clock_driver_init(struct device *device)
{
	u64_t hpetClockPeriod;
	u64_t tickFempto;
#ifndef CONFIG_TICKLESS_IDLE
	u32_t counter_load_value;
#endif

	ARG_UNUSED(device);

	/*
	 * Initial state of HPET is unknown, so put it back in a reset-like
	 * state (i.e. set main counter to 0 and disable interrupts)
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

	tickFempto = (u64_t)sys_clock_us_per_tick * 1000000000;

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

	counter_load_value = (u32_t)(tickFempto / hpetClockPeriod);

	DBG("\n\nHPET: configuration: 0x%x, clock period: 0x%x (%d pico-s)\n",
	       (u32_t)(*_HPET_GENERAL_CAPS),
	       (u32_t)hpetClockPeriod, (u32_t)hpetClockPeriod / 1000);

	DBG("HPET: timer0: available interrupts mask 0x%x\n",
	       (u32_t)(*_HPET_TIMER0_CONFIG_CAPS >> 32));

	/* Initialize sys_clock_hw_cycles_per_tick/sec */

	sys_clock_hw_cycles_per_tick = counter_load_value;
	sys_clock_hw_cycles_per_sec = sys_clock_hw_cycles_per_tick *
									sys_clock_ticks_per_sec;


#ifdef CONFIG_INT_LATENCY_BENCHMARK
	main_count_first_irq_value = counter_load_value;
	main_count_expected_value = main_count_first_irq_value;
#endif

#ifdef CONFIG_HPET_TIMER_LEGACY_EMULATION
	/*
	 * Configure HPET replace legacy 8254 timer.
	 * In this case the timer0 interrupt is routed to IRQ2
	 * and legacy timer generates no interrupts
	 */
	*_HPET_GENERAL_CONFIG |= HPET_LEGACY_RT_CNF;
#endif /* CONFIG_HPET_TIMER_LEGACY_EMULATION */

#ifndef CONFIG_TICKLESS_IDLE
	/*
	 * Set timer0 to periodic mode, ready to expire every tick
	 * Setting 32-bit mode during the first load of the comparator
	 * value is required to work around some hardware that otherwise
	 * does not work properly.
	 */

	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_TYPE_CNF | HPET_Tn_32MODE_CNF;
#else
	/* set timer0 to one-shot mode, ready to expire on the first tick */

	*_HPET_TIMER0_CONFIG_CAPS &= ~HPET_Tn_TYPE_CNF;
#endif /* !CONFIG_TICKLESS_IDLE */

	/*
	 * Set the comparator register for timer0.  The write to the comparator
	 * register is allowed due to setting the HPET_Tn_VAL_SET_CNF bit.
	 */
	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_VAL_SET_CNF;
	*_HPET_TIMER0_COMPARATOR = counter_load_value;
	/*
	 * After the comparator is loaded, 32-bit mode can be safely
	 * switched off
	 */
	*_HPET_TIMER0_CONFIG_CAPS &= ~HPET_Tn_32MODE_CNF;

	/*
	 * Route interrupts to the I/O APIC. If HPET_Tn_INT_TYPE_CNF is set this
	 * means edge triggered interrupt mode is utilized; Otherwise level
	 * sensitive interrupts are used.
	 */

	/*
	 * HPET timers IRQ field is 5 bits wide, and hence, can support only
	 * IRQ's up to 31. Some platforms, however, use IRQs greater than 31. In
	 * this case program leaves the IRQ fields blank.
	 */

	*_HPET_TIMER0_CONFIG_CAPS =
#if CONFIG_HPET_TIMER_IRQ < 32
		(*_HPET_TIMER0_CONFIG_CAPS & ~HPET_Tn_INT_ROUTE_CNF_MASK) |
		(CONFIG_HPET_TIMER_IRQ << HPET_Tn_INT_ROUTE_CNF_SHIFT)
#else
		(*_HPET_TIMER0_CONFIG_CAPS & ~HPET_Tn_INT_ROUTE_CNF_MASK)
#endif

#if defined(CONFIG_HPET_TIMER_LEVEL_LOW) || defined(CONFIG_HPET_TIMER_LEVEL_HIGH)
		| HPET_Tn_INT_TYPE_CNF;
#else
		;
#endif

	/*
	 * Although the stub has already been "connected", the vector number
	 * still has to be programmed into the interrupt controller.
	 */
	IRQ_CONNECT(CONFIG_HPET_TIMER_IRQ, CONFIG_HPET_TIMER_IRQ_PRIORITY,
		   _timer_int_handler, 0, HPET_IOAPIC_FLAGS);

	/* enable the IRQ in the interrupt controller */

	irq_enable(CONFIG_HPET_TIMER_IRQ);

	/* enable the HPET generally, and timer0 specifically */

	*_HPET_GENERAL_CONFIG |= HPET_ENABLE_CNF;
	*_HPET_TIMER0_CONFIG_CAPS |= HPET_Tn_INT_ENB_CNF;

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
 *
 * \INTERNAL WARNING
 * If this routine is ever enhanced to return all 64 bits of the counter
 * it will need to call _hpetMainCounterAtomic().
 */

u32_t _timer_cycle_get_32(void)
{
	return (u32_t) *_HPET_MAIN_COUNTER_VALUE;
}

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE

/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine disables the HPET so that timer interrupts are no
 * longer delivered.
 *
 * @return N/A
 */

void sys_clock_disable(void)
{
	/*
	 * disable the main HPET up counter and all timer interrupts;
	 * there is no need to lock interrupts before doing this since
	 * no other code alters the HPET's main configuration register
	 * once the driver has been initialized
	 */

	*_HPET_GENERAL_CONFIG &= ~HPET_ENABLE_CNF;
}

#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
