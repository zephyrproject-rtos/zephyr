/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel Local APIC timer driver
 *
 * Typically, the local APIC timer operates in periodic mode. That is, after
 * its down counter reaches zero and triggers a timer interrupt, it is reset
 * to its initial value and the down counting continues.
 *
 * If the TICKLESS_IDLE kernel configuration option is enabled, the timer may
 * be programmed to wake the system in N >= TICKLESS_IDLE_THRESH ticks.  The
 * kernel invokes z_timer_idle_enter() to program the down counter in one-shot
 * mode to trigger an interrupt in N ticks.  When the timer expires or when
 * another interrupt is detected, the kernel's interrupt stub invokes
 * z_clock_idle_exit() to leave the tickless idle state.
 *
 * @internal
 * Factors that increase the driver's complexity:
 *
 * 1. As the down-counter is a 32-bit value, the number of ticks for which the
 * system can be in tickless idle is limited to 'max_system_ticks'; This
 * corresponds to 'cycles_per_max_ticks' (as the timer is programmed in cycles).
 *
 * 2. When the request to enter tickless arrives, any remaining cycles until
 * the next tick must be accounted for to maintain accuracy.
 *
 * 3. The act of entering tickless idle may potentially straddle a tick
 * boundary. Thus the number of remaining cycles to the next tick read from
 * the down counter is suspect as it could occur before or after the tick
 * boundary (thus before or after the counter is reset). If the tick is
 * straddled, the following will occur:
 *    a. Enter tickless idle in one-shot mode
 *    b. Immediately leave tickless idle
 *    c. Process the tick event in the timer_int_handler() and revert
 *       to periodic mode.
 *    d. Re-run the scheduler and possibly re-enter tickless idle
 *
 * 4. Tickless idle may be prematurely aborted due to a straddled tick.  See
 * previous factor.
 *
 * 5. Tickless idle may be prematurely aborted due to a non-timer interrupt.
 * Its handler may make a thread ready to run, so any elapsed ticks
 * must be accounted for and the timer must also expire at the end of the
 * next logical tick so timer_int_handler() can put it back in periodic mode.
 * This can only be distinguished from the previous factor by the execution of
 * timer_int_handler().
 *
 * 6. Tickless idle may end naturally.  The down counter should be zero in
 * this case. However, some targets do not implement the local APIC timer
 * correctly and the down-counter continues to decrement.
 * @endinternal
 */

#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sys_clock.h>
#include <drivers/timer/system_timer.h>
#include <power/power.h>
#include <device.h>
#include <kernel_structs.h>

#include "legacy_api.h"

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


#if defined(CONFIG_TICKLESS_IDLE)
#define TIMER_MODE_ONE_SHOT     0
#define TIMER_MODE_PERIODIC     1
#else /* !CONFIG_TICKLESS_IDLE */
#define tickless_idle_init() \
	do {/* nothing */              \
	} while (0)
#endif /* !CONFIG_TICKLESS_IDLE */

static s32_t _sys_idle_elapsed_ticks = 1;

/* computed counter 0 initial count value */
static u32_t __noinit cycles_per_tick;

#if defined(CONFIG_TICKLESS_IDLE)
static u32_t programmed_cycles;
static u32_t programmed_full_ticks;
static u32_t __noinit max_system_ticks;
static u32_t __noinit cycles_per_max_ticks;
#ifndef CONFIG_TICKLESS_KERNEL
static bool timer_known_to_have_expired;
static unsigned char timer_mode = TIMER_MODE_PERIODIC;
#endif
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static u32_t loapic_timer_device_power_state;
static u32_t reg_timer_save;
static u32_t reg_timer_cfg_save;
#endif

/**
 *
 * @brief Set the timer for periodic mode
 *
 * This routine sets the timer for periodic mode.
 *
 * @return N/A
 */
static inline void periodic_mode_set(void)
{
	x86_write_loapic(LOAPIC_TIMER,
		x86_read_loapic(LOAPIC_TIMER) | LOAPIC_TIMER_PERIODIC);
}


/**
 *
 * @brief Set the initial count register
 *
 * This routine sets value from which the timer will count down.
 * Note that setting the value to zero stops the timer.
 *
 * @param count Count from which timer is to count down
 * @return N/A
 */
static inline void initial_count_register_set(u32_t count)
{
	x86_write_loapic(LOAPIC_TIMER_ICR, count);
}

#if defined(CONFIG_TICKLESS_IDLE)
/**
 *
 * @brief Set the timer for one shot mode
 *
 * This routine sets the timer for one shot mode.
 *
 * @return N/A
 */
static inline void one_shot_mode_set(void)
{
	x86_write_loapic(LOAPIC_TIMER,
		x86_read_loapic(LOAPIC_TIMER) & ~LOAPIC_TIMER_PERIODIC);
}
#endif /* CONFIG_TICKLESS_IDLE */

#if defined(CONFIG_TICKLESS_KERNEL) || defined(CONFIG_TICKLESS_IDLE)
/**
 *
 * @brief Get the value from the current count register
 *
 * This routine gets the value from the timer's current count register.  This
 * value is the 'time' remaining to decrement before the timer triggers an
 * interrupt.
 *
 * @return N/A
 */
static inline u32_t current_count_register_get(void)
{
	return x86_read_loapic(LOAPIC_TIMER_CCR);
}
#endif

#if defined(CONFIG_TICKLESS_IDLE)
/**
 *
 * @brief Get the value from the initial count register
 *
 * This routine gets the value from the initial count register.
 *
 * @return N/A
 */
static inline u32_t initial_count_register_get(void)
{
	return x86_read_loapic(LOAPIC_TIMER_ICR);
}
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_TICKLESS_KERNEL
static inline void program_max_cycles(void)
{
	programmed_cycles = cycles_per_max_ticks;
	initial_count_register_set(programmed_cycles);
}
#endif

void timer_int_handler(void *unused /* parameter is not used */
				 )
{
#ifdef CONFIG_EXECUTION_BENCHMARKING
	__asm__ __volatile__ (
		"pushl %eax\n\t"
		"pushl %edx\n\t"
		"rdtsc\n\t"
		"mov %eax, z_arch_timing_tick_start\n\t"
		"mov %edx, z_arch_timing_tick_start+4\n\t"
		"pop %edx\n\t"
		"pop %eax\n\t");
#endif
	ARG_UNUSED(unused);

#if defined(CONFIG_TICKLESS_KERNEL)
	if (!programmed_full_ticks) {
		if (_sys_clock_always_on) {
			z_tick_set(z_clock_uptime());
			program_max_cycles();
		}
		return;
	}

	u32_t cycles = current_count_register_get();

	if ((cycles > 0) && (cycles < programmed_cycles)) {
		/* stale interrupt */
		return;
	}

	_sys_idle_elapsed_ticks = programmed_full_ticks;

	/*
	 * Clear programmed ticks before announcing elapsed time so
	 * that recursive calls to _update_elapsed_time() will not
	 * announce already consumed elapsed time
	 */
	programmed_full_ticks = 0U;

	z_clock_announce(_sys_idle_elapsed_ticks);

	/* z_clock_announce() could cause new programming */
	if (!programmed_full_ticks && _sys_clock_always_on) {
		z_tick_set(z_clock_uptime());
		program_max_cycles();
	}
#else
#ifdef CONFIG_TICKLESS_IDLE
	if (timer_mode == TIMER_MODE_ONE_SHOT) {
		if (!timer_known_to_have_expired) {
			u32_t  cycles;

			/*
			 * The timer fired unexpectedly. This is due
			 * to one of two cases:
			 *   1. Entering tickless idle straddled a tick.
			 *   2. Leaving tickless idle straddled the final tick.
			 * Due to the timer reprogramming in
			 * z_clock_idle_exit(), case #2 can be handled
			 * as a fall-through.
			 *
			 * NOTE: Although the cycle count is supposed
			 * to stop decrementing once it hits zero in
			 * one-shot mode, not all targets implement
			 * this properly (and continue to decrement).
			 * Thus, we have to perform a second
			 * comparison to check for wrap-around.
			 */

			cycles = current_count_register_get();
			if ((cycles > 0) && (cycles < programmed_cycles)) {
				/* Case 1 */
				_sys_idle_elapsed_ticks = 0;
			}
		}

		/* Return the timer to periodic mode */
		periodic_mode_set();
		initial_count_register_set(cycles_per_tick - 1);
		timer_known_to_have_expired = false;
		timer_mode = TIMER_MODE_PERIODIC;
	}

	_sys_idle_elapsed_ticks = 1;
	z_clock_announce(_sys_idle_elapsed_ticks);
#else
	z_clock_announce(_sys_idle_elapsed_ticks);
#endif /*CONFIG_TICKLESS_IDLE*/
#endif
#ifdef CONFIG_EXECUTION_BENCHMARKING
	__asm__ __volatile__ (
		"pushl %eax\n\t"
		"pushl %edx\n\t"
		"rdtsc\n\t"
		"mov %eax, z_arch_timing_tick_end\n\t"
		"mov %edx, z_arch_timing_tick_end+4\n\t"
		"pop %edx\n\t"
		"pop %eax\n\t");
#endif /* CONFIG_EXECUTION_BENCHMARKING */
}

#ifdef CONFIG_TICKLESS_KERNEL
u32_t z_get_program_time(void)
{
	return programmed_full_ticks;
}

u32_t z_get_remaining_program_time(void)
{
	if (programmed_full_ticks == 0U) {
		return 0;
	}

	return current_count_register_get() / cycles_per_tick;
}

u32_t z_get_elapsed_program_time(void)
{
	if (programmed_full_ticks == 0U) {
		return 0;
	}

	return programmed_full_ticks -
	    (current_count_register_get() / cycles_per_tick);
}

void z_set_time(u32_t time)
{
	if (!time) {
		programmed_full_ticks = 0U;
		return;
	}

	programmed_full_ticks =
	    time > max_system_ticks ? max_system_ticks : time;

	z_tick_set(z_clock_uptime());

	programmed_cycles = programmed_full_ticks * cycles_per_tick;
	initial_count_register_set(programmed_cycles);
}

void z_enable_sys_clock(void)
{
	if (!programmed_full_ticks) {
		program_max_cycles();
	}
}

u64_t z_clock_uptime(void)
{
	u64_t elapsed;

	elapsed = z_tick_get();
	if (programmed_cycles) {
		elapsed +=
		    (programmed_cycles -
		     current_count_register_get()) / cycles_per_tick;
	}

	return elapsed;
}
#endif

#if defined(CONFIG_TICKLESS_IDLE)
/**
 *
 * @brief Initialize the tickless idle feature
 *
 * This routine initializes the tickless idle feature.  Note that the maximum
 * number of ticks that can elapse during a "tickless idle" is limited by
 * <cycles_per_tick>.  The larger the value (the lower the tick frequency),
 * the fewer elapsed ticks during a "tickless idle".  Conversely, the smaller
 * the value (the higher the tick frequency), the more elapsed ticks during a
 * "tickless idle".
 *
 * @return N/A
 */
static void tickless_idle_init(void)
{
	/*
	 * Calculate the maximum number of system ticks less one. This
	 * guarantees that an overflow will not occur when any remaining
	 * cycles are added to <cycles_per_max_ticks> when calculating
	 * <programmed_cycles>.
	 */
	max_system_ticks = (0xffffffff / cycles_per_tick) - 1;
	cycles_per_max_ticks = max_system_ticks * cycles_per_tick;
}

/**
 *
 * @brief Place system timer into idle state
 *
 * Re-program the timer to enter into the idle state for the given number of
 * ticks. It is placed into one shot mode where it will fire in the number of
 * ticks supplied or the maximum number of ticks that can be programmed into
 * hardware. A value of -1 means infinite number of ticks.
 *
 * @return N/A
 */
void z_timer_idle_enter(s32_t ticks /* system ticks */
				)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (ticks != K_FOREVER) {
		/* Need to reprogram only if current program is smaller */
		if (ticks > programmed_full_ticks) {
			z_set_time(ticks);
		}
	} else {
		programmed_full_ticks = 0U;
		programmed_cycles = 0U;
		initial_count_register_set(0); /* 0 disables timer */
	}
#else
	u32_t  cycles;

	/*
	 * Although interrupts are disabled, the LOAPIC timer is still counting
	 * down. Take a snapshot of current count register to get the number of
	 * cycles remaining in the timer before it signals an interrupt and apply
	 * that towards the one-shot calculation to maintain accuracy.
	 *
	 * NOTE: If entering tickless idle straddles a tick, 'programmed_cycles'
	 * and 'programmmed_full_ticks' may be incorrect as we do not know which
	 * side of the tick the snapshot occurred.  This is not a problem as the
	 * values will be corrected once the straddling is detected.
	 */

	cycles = current_count_register_get();

	if ((ticks == K_FOREVER) || (ticks > max_system_ticks)) {
		/*
		 * The number of cycles until the timer must fire next might not fit
		 * in the 32-bit counter register. To work around this, program
		 * the counter to fire in the maximum number of ticks (plus any
		 * remaining cycles).
		 */

		programmed_full_ticks = max_system_ticks;
		programmed_cycles = cycles + cycles_per_max_ticks;
	} else {
		programmed_full_ticks = ticks - 1;
		programmed_cycles = cycles + (programmed_full_ticks * cycles_per_tick);
	}

	/* Set timer to one-shot mode */
	one_shot_mode_set();
	initial_count_register_set(programmed_cycles);
	timer_mode = TIMER_MODE_ONE_SHOT;
#endif
}

/**
 *
 * @brief Handling of tickless idle when interrupted
 *
 * The routine is responsible for taking the timer out of idle mode and
 * generating an interrupt at the next tick interval.
 *
 * Note that in this routine, _sys_idle_elapsed_ticks must be zero because the
 * ticker has done its work and consumed all the ticks. This has to be true
 * otherwise idle mode wouldn't have been entered in the first place.
 *
 * @return N/A
 */
void z_clock_idle_exit(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (!programmed_full_ticks && _sys_clock_always_on) {
		program_max_cycles();
	}
#else
	u32_t remaining_cycles;
	u32_t remaining_full_ticks;

	/*
	 * Interrupts are locked and idling has ceased. The cause of the cessation
	 * is unknown. It may be due to one of three cases.
	 *  1. The timer, which was previously placed into one-shot mode has
	 *     counted down to zero and signaled an interrupt.
	 *  2. A non-timer interrupt occurred. Note that the LOAPIC timer will
	 *     still continue to decrement and may yet signal an interrupt.
	 *  3. The LOAPIC timer signaled an interrupt while the timer was being
	 *     programmed for one-shot mode.
	 *
	 * NOTE: Although the cycle count is supposed to stop decrementing once it
	 * hits zero in one-shot mode, not all targets implement this properly
	 * (and continue to decrement).  Thus a second comparison is required to
	 * check for wrap-around.
	 */

	remaining_cycles = current_count_register_get();

	if ((remaining_cycles == 0U) ||
		(remaining_cycles >= programmed_cycles)) {
		/*
		 * The timer has expired. The handler timer_int_handler() is
		 * guaranteed to execute. Track the number of elapsed ticks. The
		 * handler timer_int_handler() will account for the final tick.
		 */

		_sys_idle_elapsed_ticks = programmed_full_ticks;

		/*
		 * Announce elapsed ticks to the kernel. Note we are guaranteed
		 * that the timer ISR will execute before the tick event is serviced.
		 * (The timer ISR reprograms the timer for the next tick.)
		 */

		z_clock_announce(_sys_idle_elapsed_ticks);

		timer_known_to_have_expired = true;

		return;
	}

	timer_known_to_have_expired = false;

	/*
	 * Either a non-timer interrupt occurred, or we straddled a tick when
	 * entering tickless idle. It is impossible to determine which occurred
	 * at this point. Regardless of the cause, ensure that the timer will
	 * expire at the end of the next tick in case the ISR makes any threads
	 * ready to run.
	 *
	 * NOTE #1: In the case of a straddled tick, the '_sys_idle_elapsed_ticks'
	 * calculation below may result in either 0 or 1. If 1, then this may
	 * result in a harmless extra call to z_clock_announce().
	 *
	 * NOTE #2: In the case of a straddled tick, it is assumed that when the
	 * timer is reprogrammed, it will be reprogrammed with a cycle count
	 * sufficiently close to one tick that the timer will not expire before
	 * timer_int_handler() is executed.
	 */

	remaining_full_ticks = remaining_cycles / cycles_per_tick;

	_sys_idle_elapsed_ticks = programmed_full_ticks - remaining_full_ticks;

	if (_sys_idle_elapsed_ticks > 0) {
		z_clock_announce(_sys_idle_elapsed_ticks);
	}

	if (remaining_full_ticks > 0) {
		/*
		 * Re-program the timer (still in one-shot mode) to fire at the end of
		 * the tick, being careful to not program zero thus stopping the timer.
		 */

		programmed_cycles = 1 + ((remaining_cycles - 1) % cycles_per_tick);

		initial_count_register_set(programmed_cycles);
	}
#endif
}
#endif /* CONFIG_TICKLESS_IDLE */

/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the timer to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */
int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	/* determine the timer counter value (in timer clock cycles/system tick)
	 */

	cycles_per_tick = sys_clock_hw_cycles_per_tick();

	tickless_idle_init();

	x86_write_loapic(LOAPIC_TIMER_CONFIG,
		     (x86_read_loapic(LOAPIC_TIMER_CONFIG) & ~0xf)
		     | LOAPIC_TIMER_DIVBY_1);

#ifdef CONFIG_TICKLESS_KERNEL
	one_shot_mode_set();
#else
	periodic_mode_set();
#endif
	initial_count_register_set(cycles_per_tick - 1);
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	loapic_timer_device_power_state = DEVICE_PM_ACTIVE_STATE;
#endif

	IRQ_CONNECT(CONFIG_LOAPIC_TIMER_IRQ, CONFIG_LOAPIC_TIMER_IRQ_PRIORITY,
		    timer_int_handler, 0, 0);
	irq_enable(CONFIG_LOAPIC_TIMER_IRQ);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int sys_clock_suspend(struct device *dev)
{
	ARG_UNUSED(dev);

	reg_timer_save = x86_read_loapic(LOAPIC_TIMER);
	reg_timer_cfg_save = x86_read_loapic(LOAPIC_TIMER_CONFIG);
	loapic_timer_device_power_state = DEVICE_PM_SUSPEND_STATE;

	return 0;
}

static int sys_clock_resume(struct device *dev)
{
	ARG_UNUSED(dev);

	x86_write_loapic(LOAPIC_TIMER, reg_timer_save);
	x86_write_loapic(LOAPIC_TIMER_CONFIG, reg_timer_cfg_save);

	/*
	 * It is difficult to accurately know the time spent in DS.
	 * We can use TSC or RTC but that will create a dependency
	 * on those components. Other issue is about what to do
	 * with pending timers. Following are some options :-
	 *
	 * 1) Expire all timers based on time spent found using some
	 *    source like TSC
	 * 2) Expire all timers anyway
	 * 3) Expire only the timer at the top
	 * 4) Continue from where the timer left
	 *
	 * 1 and 2 require change to how timers are handled. 4 may not
	 * give a good user experience. After waiting for a long period
	 * in DS, the system would appear dead if it waits again.
	 *
	 * Current implementation uses option 3. The top most timer is
	 * expired. Following code will set the counter to a low number
	 * so it would immediately expire and generate timer interrupt
	 * which will process the top most timer. Note that timer IC
	 * cannot be set to 0.  Setting it to 0 will stop the timer.
	 */

	initial_count_register_set(1);
	loapic_timer_device_power_state = DEVICE_PM_ACTIVE_STATE;

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
int z_clock_device_ctrl(struct device *port, u32_t ctrl_command,
			  void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			ret = sys_clock_suspend(port);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			ret = sys_clock_resume(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = loapic_timer_device_power_state;
	}

	if (cb) {
		cb(port, ret, context, arg);
	}

	return ret;
}
#endif

/**
 *
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles. We use the x86 TSC as the LOAPIC timer can't be used as a periodic
 * system clock and a timestamp source at the same time.
 *
 * @return up counter of elapsed clock cycles
 */
u32_t z_timer_cycle_get_32(void)
{
#if CONFIG_TSC_CYCLES_PER_SEC != 0
	u64_t tsc;

	/* 64-bit math to avoid overflows */
	tsc = z_tsc_read() * (u64_t)sys_clock_hw_cycles_per_sec() /
		(u64_t) CONFIG_TSC_CYCLES_PER_SEC;
	return (u32_t)tsc;
#else
	/* TSC runs same as the bus speed, nothing to do but return the TSC
	 * value
	 */
	return z_do_read_cpu_timestamp32();
#endif
}

#if defined(CONFIG_SYSTEM_CLOCK_DISABLE)
/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine simply disables the LOAPIC counter such that interrupts are no
 * longer delivered.
 *
 * @return N/A
 */
void sys_clock_disable(void)
{
	unsigned int key; /* interrupt lock level */

	key = irq_lock();

	irq_disable(CONFIG_LOAPIC_TIMER_IRQ);

	initial_count_register_set(0);
	irq_unlock(key);
}

#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
