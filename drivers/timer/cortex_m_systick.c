/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M systick device driver
 *
 * This module implements the kernel's CORTEX-M ARM's systick device driver.
 * It provides the standard kernel "system clock driver" interfaces.
 *
 * The driver utilizes systick to provide kernel ticks.
 *
 * \INTERNAL IMPLEMENTATION DETAILS
 * The systick device provides a 24-bit clear-on-write, decrementing,
 * wrap-on-zero counter. Only edge sensitive triggered interrupt is supported.
 *
 */

#include <kernel.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>
#include <arch/arm/cortex_m/cmsis.h>

/* running total of timer count */
static uint32_t clock_accumulated_count;

/*
 * A board support package's board.h header must provide definitions for the
 * following constants:
 *
 *    CONFIG_SYSTICK_CLOCK_FREQ
 *
 * This is the sysTick input clock frequency.
 */

#include <board.h>

#ifdef CONFIG_TICKLESS_IDLE
#define TIMER_MODE_PERIODIC 0 /* normal running mode */
#define TIMER_MODE_ONE_SHOT 1 /* emulated, since sysTick has 1 mode */

#define IDLE_NOT_TICKLESS 0 /* non-tickless idle mode */
#define IDLE_TICKLESS 1     /* tickless idle  mode */
#endif			    /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
extern int32_t _NanoIdleValGet(void);
extern void _NanoIdleValClear(void);
extern void _sys_power_save_idle_exit(int32_t ticks);
#endif

#ifdef CONFIG_TICKLESS_IDLE
extern int32_t _sys_idle_elapsed_ticks;
#endif

#ifdef CONFIG_TICKLESS_IDLE
static uint32_t __noinit default_load_value; /* default count */
static uint32_t idle_original_count;
static uint32_t __noinit max_system_ticks;
static uint32_t idle_original_ticks;
static uint32_t __noinit max_load_value;
static uint32_t __noinit timer_idle_skew;
static unsigned char timer_mode = TIMER_MODE_PERIODIC;
static unsigned char idle_mode = IDLE_NOT_TICKLESS;
#endif /* CONFIG_TICKLESS_IDLE */

#if defined(CONFIG_TICKLESS_IDLE) || \
	defined(CONFIG_SYSTEM_CLOCK_DISABLE)

/**
 *
 * @brief Stop the timer
 *
 * This routine disables the systick counter.
 *
 * @return N/A
 */
static ALWAYS_INLINE void sysTickStop(void)
{
	uint32_t reg;

	/*
	 * Disable the counter and its interrupt while preserving the
	 * remaining bits.
	 */
	reg = SysTick->CTRL;
	reg &= ~(SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
	SysTick->CTRL = reg;
}

#endif /* CONFIG_TICKLESS_IDLE || CONFIG_SYSTEM_CLOCK_DISABLE */

#ifdef CONFIG_TICKLESS_IDLE

/**
 *
 * @brief Start the timer
 *
 * This routine enables the systick counter.
 *
 * @return N/A
 */
static ALWAYS_INLINE void sysTickStart(void)
{
	uint32_t reg;

	/*
	 * Enable the counter, its interrupt and set the clock source to be
	 * the system clock while preserving the remaining bits.
	 */
	reg = SysTick->CTRL; /* countflag is cleared by this read */

	reg |= SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk |
	       SysTick_CTRL_CLKSOURCE_Msk;
	SysTick->CTRL = reg;
}

/**
 *
 * @brief Get the current counter value
 *
 * This routine gets the value from the timer's current value register.  This
 * value is the 'time' remaining to decrement before the timer triggers an
 * interrupt.
 *
 * @return the current counter value
 */
static ALWAYS_INLINE uint32_t sysTickCurrentGet(void)
{
	return SysTick->VAL;
}

/**
 *
 * @brief Get the reload/countdown value
 *
 * This routine returns the value from the reload value register.
 *
 * @return the counter's initial count/wraparound value
 */
static ALWAYS_INLINE uint32_t sysTickReloadGet(void)
{
	return SysTick->LOAD;
}

#endif /* CONFIG_TICKLESS_IDLE */

/**
 *
 * @brief Set the reload/countdown value
 *
 * This routine sets value from which the timer will count down and also
 * sets the timer's current value register to zero.
 * Note that the value given is assumed to be valid (i.e., count < (1<<24)).
 *
 * @return N/A
 */
static ALWAYS_INLINE void sysTickReloadSet(
	uint32_t count /* count from which timer is to count down */
	)
{
	/*
	 * Write the reload value and clear the current value in preparation
	 * for enabling the timer.
	 * The countflag in the control/status register is also cleared by
	 * this operation.
	 */
	SysTick->LOAD = count;
	SysTick->VAL = 0; /* also clears the countflag */
}

/**
 *
 * @brief System clock tick handler
 *
 * This routine handles the system clock tick interrupt. A TICK_EVENT event
 * is pushed onto the kernel stack.
 *
 * The symbol for this routine is either _timer_int_handler.
 *
 * @return N/A
 */
void _timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
	extern void _sys_k_event_logger_interrupt(void);
	_sys_k_event_logger_interrupt();
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	int32_t numIdleTicks;

	/*
	 * All interrupts are disabled when handling idle wakeup.
	 * For tickless idle, this ensures that the calculation and programming
	 * of
	 * the device for the next timer deadline is not interrupted.
	 * For non-tickless idle, this ensures that the clearing of the kernel
	 * idle
	 * state is not interrupted.
	 * In each case, _sys_power_save_idle_exit is called with interrupts
	 * disabled.
	 */
	__asm__(" cpsid i"); /* PRIMASK = 1 */

#ifdef CONFIG_TICKLESS_IDLE
	/*
	 * If this a wakeup from a completed tickless idle or after
	 *  _timer_idle_exit has processed a partial idle, return
	 *  to the normal tick cycle.
	 */
	if (timer_mode == TIMER_MODE_ONE_SHOT) {
		sysTickStop();
		sysTickReloadSet(default_load_value);
		sysTickStart();
		timer_mode = TIMER_MODE_PERIODIC;
	}

	/* set the number of elapsed ticks and announce them to the kernel */

	if (idle_mode == IDLE_TICKLESS) {
		/* tickless idle completed without interruption */
		idle_mode = IDLE_NOT_TICKLESS;
		_sys_idle_elapsed_ticks =
			idle_original_ticks + 1; /* actual # of idle ticks */
		_sys_clock_tick_announce();
	} else {
		_sys_clock_final_tick_announce();
	}

	/* accumulate total counter value */
	clock_accumulated_count += default_load_value * _sys_idle_elapsed_ticks;
#else  /* !CONFIG_TICKLESS_IDLE */
	/*
	 * No tickless idle:
	 * Update the total tick count and announce this tick to the kernel.
	 */
	clock_accumulated_count += sys_clock_hw_cycles_per_tick;

	_sys_clock_tick_announce();
#endif /* CONFIG_TICKLESS_IDLE */

	numIdleTicks = _NanoIdleValGet(); /* get # of idle ticks requested */

	if (numIdleTicks) {
		_NanoIdleValClear(); /* clear kernel idle setting */

		/*
		 * Complete idle processing.
		 * Note that for tickless idle, nothing will be done in
		 * _timer_idle_exit.
		 */
		_sys_power_save_idle_exit(numIdleTicks);
	}

	__asm__(" cpsie i"); /* re-enable interrupts (PRIMASK = 0) */

#else /* !CONFIG_SYS_POWER_MANAGEMENT */

	/* accumulate total counter value */
	clock_accumulated_count += sys_clock_hw_cycles_per_tick;

	/*
	 * one more tick has occurred -- don't need to do anything special since
	 * timer is already configured to interrupt on the following tick
	 */
	_sys_clock_tick_announce();

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

	extern void _ExcExit(void);
	_ExcExit();
}

#ifdef CONFIG_TICKLESS_IDLE

/**
 *
 * @brief Initialize the tickless idle feature
 *
 * This routine initializes the tickless idle feature by calculating the
 * necessary hardware-specific parameters.
 *
 * Note that the maximum number of ticks that can elapse during a "tickless idle"
 * is limited by <default_load_value>.  The larger the value (the lower the
 * tick frequency), the fewer elapsed ticks during a "tickless idle".
 * Conversely, the smaller the value (the higher the tick frequency), the
 * more elapsed ticks during a "tickless idle".
 *
 * @return N/A
 */
static void sysTickTicklessIdleInit(void)
{
	/* enable counter, disable interrupt and set clock src to system clock
	 */
	uint32_t ctrl = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;

	volatile uint32_t dummy; /* used to help determine the 'skew time' */

	/* store the default reload value (which has already been set) */
	default_load_value = sysTickReloadGet();

	/* calculate the max number of ticks with this 24-bit H/W counter */
	max_system_ticks = 0x00ffffff / default_load_value;

	/* determine the associated load value */
	max_load_value = max_system_ticks * default_load_value;

	/*
	 * Calculate the skew from switching the timer in and out of idle mode.
	 * The following sequence is emulated:
	 *    1. Stop the timer.
	 *    2. Read the current counter value.
	 *    3. Calculate the new/remaining counter reload value.
	 *    4. Load the new counter value.
	 *    5. Set the timer mode to periodic/one-shot.
	 *    6. Start the timer.
	 *
	 * The timer must be running for this to work, so enable the
	 * systick counter without generating interrupts, using the processor
	 *clock.
	 * Note that the reload value has already been set by the caller.
	 */

	SysTick->CTRL |= ctrl;
	__ISB();

	timer_idle_skew = sysTickCurrentGet(); /* start of skew time */

	SysTick->CTRL |= ctrl; /* normally sysTickStop() */

	dummy = sysTickCurrentGet(); /* emulate sysTickReloadSet() */

	/* emulate calculation of the new counter reload value */
	if ((dummy == 1) || (dummy == default_load_value)) {
		dummy = max_system_ticks - 1;
		dummy += max_load_value - default_load_value;
	} else {
		dummy = dummy - 1;
		dummy += dummy * default_load_value;
	}

	/* _sysTickStart() without interrupts */
	SysTick->CTRL |= ctrl;

	timer_mode = TIMER_MODE_PERIODIC;

	/* skew time calculation for down counter (assumes no rollover) */
	timer_idle_skew -= sysTickCurrentGet();

	/* restore the previous sysTick state */
	sysTickStop();
	sysTickReloadSet(default_load_value);
}

/**
 *
 * @brief Place the system timer into idle state
 *
 * Re-program the timer to enter into the idle state for the given number of
 * ticks. It is set to a "one shot" mode where it will fire in the number of
 * ticks supplied or the maximum number of ticks that can be programmed into
 * hardware. A value of -1 will result in the maximum number of ticks.
 *
 * @return N/A
 */
void _timer_idle_enter(int32_t ticks /* system ticks */
				)
{
	sysTickStop();

	/*
	 * We're being asked to have the timer fire in "ticks" from now. To
	 * maintain accuracy we must account for the remaining time left in the
	 * timer. So we read the count out of it and add it to the requested
	 * time out
	 */
	idle_original_count = sysTickCurrentGet() - timer_idle_skew;

	if ((ticks == -1) || (ticks > max_system_ticks)) {
		/*
		 * We've been asked to fire the timer so far in the future that
		 * the required count value would not fit in the 24-bit reload
		 * register.
		 * Instead, we program for the maximum programmable interval
		 * minus one system tick to prevent overflow when the left over
		 * count read earlier is added.
		 */
		idle_original_count += max_load_value - default_load_value;
		idle_original_ticks = max_system_ticks - 1;
	} else {
		/*
		 * leave one tick of buffer to have to time react when coming
		 * back
		 */
		idle_original_ticks = ticks - 1;
		idle_original_count += idle_original_ticks * default_load_value;
	}

	/*
	 * Set timer to virtual "one shot" mode - sysTick does not have multiple
	 * modes, so the reload value is simply changed.
	 */
	timer_mode = TIMER_MODE_ONE_SHOT;
	idle_mode = IDLE_TICKLESS;
	sysTickReloadSet(idle_original_count);
	sysTickStart();
}

/**
 *
 * @brief Handling of tickless idle when interrupted
 *
 * The routine, called by _sys_power_save_idle_exit, is responsible for taking
 * the timer out of idle mode and generating an interrupt at the next
 * tick interval.  It is expected that interrupts have been disabled.
 *
 * Note that in this routine, _sys_idle_elapsed_ticks must be zero because the
 * ticker has done its work and consumed all the ticks. This has to be true
 * otherwise idle mode wouldn't have been entered in the first place.
 *
 * @return N/A
 */
void _timer_idle_exit(void)
{
	uint32_t count; /* timer's current count register value */

	if (timer_mode == TIMER_MODE_PERIODIC) {
		/*
		 * The timer interrupt handler is handling a completed tickless
		 * idle or this has been called by mistake; there's nothing to
		 * do here.
		 */
		return;
	}

	sysTickStop();

	/* timer is in idle mode, adjust the ticks expired */

	count = sysTickCurrentGet();

	if ((count == 0) || (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {
		/*
		 * The timer expired and/or wrapped around. Re-set the timer to
		 * its default value and mode.
		 */
		sysTickReloadSet(default_load_value);
		timer_mode = TIMER_MODE_PERIODIC;

		/*
		 * Announce elapsed ticks to the kernel. Note we are guaranteed
		 * that the timer ISR will execute before the tick event is
		 * serviced, so _sys_idle_elapsed_ticks is adjusted to account
		 * for it.
		 */
		_sys_idle_elapsed_ticks = idle_original_ticks - 1;
		_sys_clock_tick_announce();
	} else {
		uint32_t elapsed;   /* elapsed "counter time" */
		uint32_t remaining; /* remaining "counter time" */

		elapsed = idle_original_count - count;

		remaining = elapsed % default_load_value;

		/* ensure that the timer will interrupt at the next tick */

		if (remaining == 0) {
			/*
			 * Idle was interrupted on a tick boundary. Re-set the
			 * timer to its default value and mode.
			 */
			sysTickReloadSet(default_load_value);
			timer_mode = TIMER_MODE_PERIODIC;
		} else if (count > remaining) {
			/*
			 * There is less time remaining to the next tick
			 * boundary than time left for idle. Leave in "one
			 * shot" mode.
			 */
			sysTickReloadSet(remaining);
		}

		_sys_idle_elapsed_ticks = elapsed / default_load_value;

		if (_sys_idle_elapsed_ticks) {
			_sys_clock_tick_announce();
		}
	}

	idle_mode = IDLE_NOT_TICKLESS;
	sysTickStart();
}

#endif /* CONFIG_TICKLESS_IDLE */

/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the systick to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */
int _sys_clock_driver_init(struct device *device)
{
	/* enable counter, interrupt and set clock src to system clock */
	uint32_t ctrl = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk |
			SysTick_CTRL_CLKSOURCE_Msk;

	ARG_UNUSED(device);

	/*
	 * Determine the reload value to achieve the configured tick rate.
	 */

	/* systick supports 24-bit H/W counter */
	__ASSERT(sys_clock_hw_cycles_per_tick <= (1 << 24),
		 "sys_clock_hw_cycles_per_tick too large");
	sysTickReloadSet(sys_clock_hw_cycles_per_tick - 1);

#ifdef CONFIG_TICKLESS_IDLE

	/* calculate hardware-specific parameters for tickless idle */

	sysTickTicklessIdleInit();

#endif /* CONFIG_TICKLESS_IDLE */

	_ScbExcPrioSet(_EXC_SYSTICK, _EXC_IRQ_DEFAULT_PRIO);

	SysTick->CTRL = ctrl;

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
 * systick counter is a 24-bit down counter which is reset to "reload" value
 * once it reaches 0.
 */
uint32_t k_cycle_get_32(void)
{
	return clock_accumulated_count + (SysTick->LOAD - SysTick->VAL);
}

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE

/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine disables the systick so that timer interrupts are no
 * longer delivered.
 *
 * @return N/A
 */
void sys_clock_disable(void)
{
	unsigned int key; /* interrupt lock level */

	key = irq_lock();

	/* disable the systick counter and systick interrupt */

	sysTickStop();

	irq_unlock(key);
}

#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
