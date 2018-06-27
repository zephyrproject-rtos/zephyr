/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC Timer0 device driver
 *
 * This module implements a kernel device driver for the ARCv2 processor Timer0
 * and provides the standard "system clock driver" interfaces.
 *
 * If the TICKLESS_IDLE kernel configuration option is enabled, the timer may
 * be programmed to wake the system in N >= TICKLESS_IDLE_THRESH ticks. The
 * kernel invokes _timer_idle_enter() to program the up counter to trigger an
 * interrupt in N ticks.  When the timer expires (or when another interrupt is
 * detected), the kernel's interrupt stub invokes _timer_idle_exit() to leave
 * the tickless idle state.
 *
 * @internal
 * The ARCv2 processor timer provides a 32-bit incrementing, wrap-to-zero
 * counter.
 *
 * Factors that increase the driver's tickless idle complexity:
 * 1. As the Timer0 up-counter is a 32-bit value, the number of ticks for which
 * the system can be in tickless idle is limited to 'max_system_ticks'.
 *
 * 2. The act of entering tickless idle may potentially straddle a tick
 * boundary. This can be detected in _timer_idle_enter() after Timer0 is
 * programmed with the new limit and acted upon in _timer_idle_exit().
 *
 * 3. Tickless idle may be prematurely aborted due to a straddled tick. See
 * previous factor.
 *
 * 4. Tickless idle may end naturally.  This is detected and handled in
 * _timer_idle_exit().
 *
 * 5. Tickless idle may be prematurely aborted due to a non-timer interrupt.
 * If this occurs, Timer0 is reprogrammed to trigger at the next tick.
 * @endinternal
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <misc/__assert.h>
#include <arch/arc/v2/aux_regs.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>
#include <stdbool.h>
#include <misc/__assert.h>

/*
 * note: This implementation assumes Timer0 is present. Be sure
 * to build the ARC CPU with Timer0.
 */

#include <board.h>

#define _ARC_V2_TMR_CTRL_IE 0x1 /* interrupt enable */
#define _ARC_V2_TMR_CTRL_NH 0x2 /* count only while not halted */
#define _ARC_V2_TMR_CTRL_W  0x4 /* watchdog mode enable */
#define _ARC_V2_TMR_CTRL_IP 0x8 /* interrupt pending flag */

/* running total of timer count */
static u32_t __noinit cycles_per_tick;
static volatile u32_t accumulated_cycle_count;

#ifdef CONFIG_TICKLESS_IDLE
static u32_t __noinit max_system_ticks;
static u32_t __noinit programmed_ticks;
extern s32_t _sys_idle_elapsed_ticks;
#ifndef CONFIG_TICKLESS_KERNEL
static u32_t __noinit programmed_limit;
static int straddled_tick_on_idle_enter;
#endif
#endif

#ifdef CONFIG_TICKLESS_KERNEL
static volatile int timer_expired;
#endif

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static u32_t arcv2_timer0_device_power_state;
static u32_t saved_limit;
static u32_t saved_control;
#endif

/**
 *
 * @brief Get contents of Timer0 count register
 *
 * @return Current Timer0 count
 */
static ALWAYS_INLINE u32_t timer0_count_register_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
}

/**
 *
 * @brief Set Timer0 count register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_count_register_set(u32_t value)
{
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, value);
}

/**
 *
 * @brief Get contents of Timer0 control register
 *
 * @return N/A
 */
static ALWAYS_INLINE u32_t timer0_control_register_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_CONTROL);
}

/**
 *
 * @brief Set Timer0 control register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_control_register_set(u32_t value)
{
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, value);
}

/**
 *
 * @brief Get contents of Timer0 limit register
 *
 * @return N/A
 */
static ALWAYS_INLINE u32_t timer0_limit_register_get(void)
{
	return _arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
}

/**
 *
 * @brief Set Timer0 limit register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_limit_register_set(u32_t count)
{
	_arc_v2_aux_reg_write(_ARC_V2_TMR0_LIMIT, count);
}

#ifdef CONFIG_TICKLESS_IDLE
static ALWAYS_INLINE void update_accumulated_count(void)
{
	accumulated_cycle_count += (_sys_idle_elapsed_ticks * cycles_per_tick);
}
#else /* CONFIG_TICKLESS_IDLE */
static ALWAYS_INLINE void update_accumulated_count(void)
{
	accumulated_cycle_count += cycles_per_tick;
}
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_TICKLESS_KERNEL
static inline void program_max_cycles(void)
{
	timer0_limit_register_set(max_system_ticks * cycles_per_tick);
	timer_expired = 0;
}
#endif

/**
 *
 * @brief System clock periodic tick handler
 *
 * This routine handles the system clock periodic tick interrupt. It always
 * announces one tick.
 *
 * @return N/A
 */
void _timer_int_handler(void *unused)
{
#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_tick_handler(void);
	read_timer_start_of_tick_handler();
#endif
	ARG_UNUSED(unused);
	/* clear the interrupt by writing 0 to IP bit of the control register */
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

#ifdef CONFIG_TICKLESS_KERNEL
	if (!programmed_ticks) {
		if (_sys_clock_always_on) {
			_sys_clock_tick_count = _get_elapsed_clock_time();
			program_max_cycles();
		}
		return;
	}

	_sys_idle_elapsed_ticks = programmed_ticks;

	/*
	 * Clear programmed ticks before announcing elapsed time so
	 * that recursive calls to _update_elapsed_time() will not
	 * announce already consumed elapsed time
	 */
	programmed_ticks = 0;
	timer_expired = 1;

	_sys_clock_tick_announce();

	/* _sys_clock_tick_announce() could cause new programming */
	if (!programmed_ticks && _sys_clock_always_on) {
		_sys_clock_tick_count = _get_elapsed_clock_time();
		program_max_cycles();
	}
#else
#if defined(CONFIG_TICKLESS_IDLE)
	timer0_limit_register_set(cycles_per_tick - 1);
	__ASSERT_EVAL({},
		      u32_t timer_count = timer0_count_register_get(),
		      timer_count <= (cycles_per_tick - 1),
		      "timer_count: %d, limit %d\n", timer_count, cycles_per_tick - 1);

	_sys_clock_final_tick_announce();
#else
	_sys_clock_tick_announce();
#endif

	update_accumulated_count();
#endif
#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_end_of_tick_handler(void);
	read_timer_end_of_tick_handler();
#endif
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

	if (timer0_control_register_get() & _ARC_V2_TMR_CTRL_IP) {
		return 0;
	}
	return programmed_ticks -
	    (timer0_count_register_get() / cycles_per_tick);
}

u32_t _get_elapsed_program_time(void)
{
	if (programmed_ticks == 0) {
		return 0;
	}

	if (timer0_control_register_get() & _ARC_V2_TMR_CTRL_IP) {
		return programmed_ticks;
	}
	return timer0_count_register_get() / cycles_per_tick;
}

void _set_time(u32_t time)
{
	if (!time) {
		programmed_ticks = 0;
		return;
	}

	programmed_ticks = time > max_system_ticks ? max_system_ticks : time;

	_sys_clock_tick_count = _get_elapsed_clock_time();

	timer0_limit_register_set(programmed_ticks * cycles_per_tick);
	timer0_count_register_set(0);

	timer_expired = 0;
}

void _enable_sys_clock(void)
{
	if (!programmed_ticks) {
		program_max_cycles();
	}
}

static inline u64_t get_elapsed_count(void)
{
	u64_t elapsed;

	if (timer_expired
	    || (timer0_control_register_get() & _ARC_V2_TMR_CTRL_IP)) {
		elapsed = timer0_limit_register_get();
	} else {
		elapsed = timer0_count_register_get();
	}

	elapsed += _sys_clock_tick_count * cycles_per_tick;

	return elapsed;
}

u64_t _get_elapsed_clock_time(void)
{
	return get_elapsed_count() / cycles_per_tick;
}
#endif

#if defined(CONFIG_TICKLESS_IDLE)
/*
 * @brief initialize the tickless idle feature
 *
 * This routine initializes the tickless idle feature.
 *
 * @return N/A
 */

static void tickless_idle_init(void)
{
	/* calculate the max number of ticks with this 32-bit H/W counter */
	max_system_ticks = 0xffffffff / cycles_per_tick;
}

/*
 * @brief Place the system timer into idle state
 *
 * Re-program the timer to enter into the idle state for either the given
 * number of ticks or the maximum number of ticks that can be programmed
 * into hardware.
 *
 * @return N/A
 */

void _timer_idle_enter(s32_t ticks)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (ticks != K_FOREVER) {
		/* Need to reprogram only if current program is smaller */
		if (ticks > programmed_ticks) {
			_set_time(ticks);
		}
	} else {
		programmed_ticks = 0;
		timer0_control_register_set(timer0_control_register_get() &
					    ~_ARC_V2_TMR_CTRL_IE);
	}
#else
	u32_t  status;

	if ((ticks == K_FOREVER) || (ticks > max_system_ticks)) {
		/*
		 * The number of cycles until the timer must fire next might not fit
		 * in the 32-bit counter register. To work around this, program
		 * the counter to fire in the maximum number of ticks.
		 */
		ticks = max_system_ticks;
	}

	programmed_ticks = ticks;
	programmed_limit = (programmed_ticks * cycles_per_tick) - 1;

	timer0_limit_register_set(programmed_limit);

	/*
	 * If Timer0's IP bit is set, then it is known that we have straddled
	 * a tick boundary while entering tickless idle.
	 */

	status = timer0_control_register_get();
	if (status & _ARC_V2_TMR_CTRL_IP) {
		straddled_tick_on_idle_enter = 1;
	}
	__ASSERT_EVAL({},
		      u32_t timer_count = timer0_count_register_get(),
		      timer_count <= programmed_limit,
		      "timer_count: %d, limit %d\n", timer_count, programmed_limit);
#endif
}

/*
 * @brief handling of tickless idle when interrupted
 *
 * The routine, called by _SysPowerSaveIdleExit, is responsible for taking the
 * timer out of idle mode and generating an interrupt at the next tick
 * interval.  It is expected that interrupts have been disabled.
 *
 * RETURNS: N/A
 */

void _timer_idle_exit(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (!programmed_ticks && _sys_clock_always_on) {
		if (!(timer0_control_register_get() & _ARC_V2_TMR_CTRL_IE)) {
			timer0_control_register_set(_ARC_V2_TMR_CTRL_NH |
						    _ARC_V2_TMR_CTRL_IE);
		}
		program_max_cycles();
	}
#else
	if (straddled_tick_on_idle_enter) {
		/* Aborting the tickless idle due to a straddled tick. */
		straddled_tick_on_idle_enter = 0;
		__ASSERT_EVAL({},
			      u32_t timer_count = timer0_count_register_get(),
			      timer_count <= programmed_limit,
			      "timer_count: %d, limit %d\n", timer_count, programmed_limit);
		return;
	}

	u32_t  control;
	u32_t  current_count;

	current_count = timer0_count_register_get();
	control = timer0_control_register_get();
	if (control & _ARC_V2_TMR_CTRL_IP) {
		/*
		 * The timer has expired. The handler _timer_int_handler() is
		 * guaranteed to execute. Track the number of elapsed ticks. The
		 * handler _timer_int_handler() will account for the final tick.
		 */

		_sys_idle_elapsed_ticks = programmed_ticks - 1;
		update_accumulated_count();
		_sys_clock_tick_announce();

		__ASSERT_EVAL({},
			      u32_t timer_count = timer0_count_register_get(),
			      timer_count <= programmed_limit,
			      "timer_count: %d, limit %d\n", timer_count, programmed_limit);
		return;
	}

	/*
	 * A non-timer interrupt occurred.  Announce any
	 * ticks that have elapsed during the tickless idle.
	 */
	_sys_idle_elapsed_ticks = current_count / cycles_per_tick;
	if (_sys_idle_elapsed_ticks > 0) {
		update_accumulated_count();
		_sys_clock_tick_announce();
	}

	/*
	 * Ensure the timer will expire at the end of the next tick in case
	 * the ISR makes any threads ready to run.
	 */
	timer0_limit_register_set(cycles_per_tick - 1);
	timer0_count_register_set(current_count % cycles_per_tick);

	__ASSERT_EVAL({},
		      u32_t timer_count = timer0_count_register_get(),
		      timer_count <= (cycles_per_tick - 1),
		      "timer_count: %d, limit %d\n", timer_count, cycles_per_tick-1);
#endif
}
#else
static void tickless_idle_init(void) {}
#endif /* CONFIG_TICKLESS_IDLE */


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
	ARG_UNUSED(device);

	/* ensure that the timer will not generate interrupts */
	timer0_control_register_set(0);
	timer0_count_register_set(0);

	cycles_per_tick = sys_clock_hw_cycles_per_tick;

	IRQ_CONNECT(IRQ_TIMER0, CONFIG_ARCV2_TIMER_IRQ_PRIORITY,
		    _timer_int_handler, NULL, 0);

	/*
	 * Set the reload value to achieve the configured tick rate, enable the
	 * counter and interrupt generation.
	 */

	tickless_idle_init();

	timer0_limit_register_set(cycles_per_tick - 1);
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

	/* everything has been configured: safe to enable the interrupt */

	irq_enable(IRQ_TIMER0);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int sys_clock_suspend(struct device *dev)
{
	ARG_UNUSED(dev);

	saved_limit = timer0_limit_register_get();
	saved_control = timer0_control_register_get();

	arcv2_timer0_device_power_state = DEVICE_PM_SUSPEND_STATE;

	return 0;
}

static int sys_clock_resume(struct device *dev)
{
	ARG_UNUSED(dev);

	timer0_limit_register_set(saved_limit);
	timer0_control_register_set(saved_control);

	/*
	 * It is difficult to accurately know the time spent in DS.
	 * Expire the timer to get the scheduler called.
	 */
	timer0_count_register_set(saved_limit - 1);

	arcv2_timer0_device_power_state = DEVICE_PM_ACTIVE_STATE;

	return 0;
}

/*
 * Implements the driver control management functionality
 * the *context may include IN data or/and OUT data
 */
int sys_clock_device_ctrl(struct device *port, u32_t ctrl_command,
			  void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return sys_clock_suspend(port);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return sys_clock_resume(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = arcv2_timer0_device_power_state;
		return 0;
	}

	return 0;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

u32_t _timer_cycle_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
return (u32_t) get_elapsed_count();
#else
	u32_t acc, count;

	do {
		acc = accumulated_cycle_count;
		count = timer0_count_register_get();
	} while (acc != accumulated_cycle_count);

	return acc + count;
#endif
}

#if defined(CONFIG_SYSTEM_CLOCK_DISABLE)
/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine disables timer interrupt generation and delivery.
 * Note that the timer's counting cannot be stopped by software.
 *
 * @return N/A
 */
void sys_clock_disable(void)
{
	unsigned int key;  /* interrupt lock level */
	u32_t control; /* timer control register value */

	key = irq_lock();

	/* disable interrupt generation */

	control = timer0_control_register_get();
	timer0_control_register_set(control & ~_ARC_V2_TMR_CTRL_IE);

	irq_unlock(key);

	/* disable interrupt in the interrupt controller */

	irq_disable(ARCV2_TIMER0_INT_LVL);
}
#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
