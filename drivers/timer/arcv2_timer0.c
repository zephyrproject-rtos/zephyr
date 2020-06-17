/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2018 Synopsys Inc, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/arc/v2/aux_regs.h>
#include <soc.h>
/*
 * note: This implementation assumes Timer0 is present. Be sure
 * to build the ARC CPU with Timer0.
 *
 * If secureshield is present and secure firmware is configured,
 * use secure Timer 0
 */

#ifdef CONFIG_ARC_SECURE_FIRMWARE

#undef _ARC_V2_TMR0_COUNT
#undef _ARC_V2_TMR0_CONTROL
#undef _ARC_V2_TMR0_LIMIT
#undef IRQ_TIMER0

#define _ARC_V2_TMR0_COUNT _ARC_V2_S_TMR0_COUNT
#define _ARC_V2_TMR0_CONTROL _ARC_V2_S_TMR0_CONTROL
#define _ARC_V2_TMR0_LIMIT _ARC_V2_S_TMR0_LIMIT
#define IRQ_TIMER0 IRQ_SEC_TIMER0

#endif

#define _ARC_V2_TMR_CTRL_IE 0x1 /* interrupt enable */
#define _ARC_V2_TMR_CTRL_NH 0x2 /* count only while not halted */
#define _ARC_V2_TMR_CTRL_W  0x4 /* watchdog mode enable */
#define _ARC_V2_TMR_CTRL_IP 0x8 /* interrupt pending flag */

/* Minimum cycles in the future to try to program. */
#define MIN_DELAY 1024
/* arc timer has 32 bit, here use 31 bit to avoid the possible
 * overflow,e.g, 0xffffffff + any value will cause overflow
 */
#define COUNTER_MAX 0x7fffffff
#define TIMER_STOPPED 0x0
#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

#define SMP_TIMER_DRIVER (CONFIG_SMP && CONFIG_MP_NUM_CPUS > 1)

static struct k_spinlock lock;


#if SMP_TIMER_DRIVER
volatile static uint64_t last_time;
volatile static uint64_t start_time;

#else
static uint32_t last_load;


/*
 * This local variable holds the amount of timer cycles elapsed
 * and it is updated in z_timer_int_handler and z_clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the
 *  HW timer is calculated as:
 *
 * t = cycle_counter + elapsed();
 */
static uint32_t cycle_count;

/*
 * This local variable holds the amount of elapsed HW cycles
 * that have been announced to the kernel.
 */
static uint32_t announced_cycles;


/*
 * This local variable holds the amount of elapsed HW cycles due to
 * timer wraps ('overflows') and is used in the calculation
 * in elapsed() function, as well as in the updates to cycle_count.
 *
 * Note:
 * Each time cycle_count is updated with the value from overflow_cycles,
 * the overflow_cycles must be reset to zero.
 */
static volatile uint32_t overflow_cycles;
#endif

/**
 *
 * @brief Get contents of Timer0 count register
 *
 * @return Current Timer0 count
 */
static ALWAYS_INLINE uint32_t timer0_count_register_get(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
}

/**
 *
 * @brief Set Timer0 count register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_count_register_set(uint32_t value)
{
	z_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, value);
}

/**
 *
 * @brief Get contents of Timer0 control register
 *
 * @return N/A
 */
static ALWAYS_INLINE uint32_t timer0_control_register_get(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_TMR0_CONTROL);
}

/**
 *
 * @brief Set Timer0 control register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_control_register_set(uint32_t value)
{
	z_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, value);
}

/**
 *
 * @brief Get contents of Timer0 limit register
 *
 * @return N/A
 */
static ALWAYS_INLINE uint32_t timer0_limit_register_get(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
}

/**
 *
 * @brief Set Timer0 limit register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_limit_register_set(uint32_t count)
{
	z_arc_v2_aux_reg_write(_ARC_V2_TMR0_LIMIT, count);
}

#if !SMP_TIMER_DRIVER
/* This internal function calculates the amount of HW cycles that have
 * elapsed since the last time the absolute HW cycles counter has been
 * updated. 'cycle_count' may be updated either by the ISR, or
 * in z_clock_set_timeout().
 *
 * Additionally, the function updates the 'overflow_cycles' counter, that
 * holds the amount of elapsed HW cycles due to (possibly) multiple
 * timer wraps (overflows).
 *
 * Prerequisites:
 * - reprogramming of LIMIT must be clearing the COUNT
 * - ISR must be clearing the 'overflow_cycles' counter.
 * - no more than one counter-wrap has occurred between
 *     - the timer reset or the last time the function was called
 *     - and until the current call of the function is completed.
 * - the function is invoked with interrupts disabled.
 */
static uint32_t elapsed(void)
{
	uint32_t val, ctrl;

	do {
		val =  timer0_count_register_get();
		ctrl = timer0_control_register_get();
	} while (timer0_count_register_get() < val);

	if (ctrl & _ARC_V2_TMR_CTRL_IP) {
		overflow_cycles += last_load;
		/* clear the IP bit of the control register */
		timer0_control_register_set(_ARC_V2_TMR_CTRL_NH |
					    _ARC_V2_TMR_CTRL_IE);
		/* use sw triggered irq to remember the timer irq request
		 * which may be cleared by the above operation. when elapsed ()
		 * is called in z_timer_int_handler, no need to do this.
		 */
		if (!z_arc_v2_irq_unit_is_in_isr() ||
		    z_arc_v2_aux_reg_read(_ARC_V2_ICAUSE) != IRQ_TIMER0) {
			z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT,
					       IRQ_TIMER0);
		}
	}

	return val + overflow_cycles;
}
#endif

/**
 *
 * @brief System clock periodic tick handler
 *
 * This routine handles the system clock tick interrupt. It always
 * announces one tick when TICKLESS is not enabled, or multiple ticks
 * when TICKLESS is enabled.
 *
 * @return N/A
 */
static void timer_int_handler(const void *unused)
{
	ARG_UNUSED(unused);
	uint32_t dticks;

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	uint64_t curr_time;
	k_spinlock_key_t key;

	/* clear the IP bit of the control register */
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH |
				    _ARC_V2_TMR_CTRL_IE);
	key = k_spin_lock(&lock);
	/* gfrc is the wall clock */
	curr_time = z_arc_connect_gfrc_read();

	dticks = (curr_time - last_time) / CYC_PER_TICK;
	/* last_time should be aligned to ticks */
	last_time += dticks * CYC_PER_TICK;

	k_spin_unlock(&lock, key);

	z_clock_announce(dticks);
#else
	/* timer_int_handler may be triggered by timer irq or
	 * software helper irq
	 */

	/* irq with higher priority may call z_clock_set_timeout
	 * so need a lock here
	 */
	uint32_t key;

	key = arch_irq_lock();

	elapsed();
	cycle_count += overflow_cycles;
	overflow_cycles = 0;

	arch_irq_unlock(key);

	dticks = (cycle_count - announced_cycles) / CYC_PER_TICK;
	announced_cycles += dticks * CYC_PER_TICK;
	z_clock_announce(TICKLESS ? dticks : 1);
#endif

}


/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the ARCv2 timer to deliver interrupts at the
 * rate specified via the CYC_PER_TICK.
 *
 * @return 0
 */
int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	/* ensure that the timer will not generate interrupts */
	timer0_control_register_set(0);

#if SMP_TIMER_DRIVER
	IRQ_CONNECT(IRQ_TIMER0, CONFIG_ARCV2_TIMER_IRQ_PRIORITY,
		    timer_int_handler, NULL, 0);

	timer0_limit_register_set(CYC_PER_TICK - 1);
	last_time = z_arc_connect_gfrc_read();
	start_time = last_time;
#else
	last_load = CYC_PER_TICK;
	overflow_cycles = 0;
	announced_cycles = 0;

	IRQ_CONNECT(IRQ_TIMER0, CONFIG_ARCV2_TIMER_IRQ_PRIORITY,
		    timer_int_handler, NULL, 0);

	timer0_limit_register_set(last_load - 1);
#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	cycle_count = timer0_count_register_get();
#endif
#endif
	timer0_count_register_set(0);
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

	/* everything has been configured: safe to enable the interrupt */

	irq_enable(IRQ_TIMER0);

	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	/* If the kernel allows us to miss tick announcements in idle,
	 * then shut off the counter. (Note: we can assume if idle==true
	 * that interrupts are already disabled)
	 */
#if SMP_TIMER_DRIVER
	/* as 64-bits GFRC is used as wall clock, it's ok to ignore idle
	 * systick will not be missed.
	 * However for single core using 32-bits arc timer, idle cannot
	 * be ignored, as 32-bits timer will overflow in a not-long time.
	 */
	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && ticks == K_TICKS_FOREVER) {
		timer0_control_register_set(0);
		timer0_count_register_set(0);
		timer0_limit_register_set(0);
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t delay;
	uint32_t key;

	ticks = MIN(MAX_TICKS, ticks);

	/* Desired delay in the future
	 * use MIN_DEALY here can trigger the timer
	 * irq more soon, no need to go to CYC_PER_TICK
	 * later.
	 */
	delay = MAX(ticks * CYC_PER_TICK, MIN_DELAY);

	key = arch_irq_lock();

	timer0_limit_register_set(delay - 1);
	timer0_count_register_set(0);
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH |
						_ARC_V2_TMR_CTRL_IE);

	arch_irq_unlock(key);
#endif
#else
	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && idle
	    && ticks == K_TICKS_FOREVER) {
		timer0_control_register_set(0);
		timer0_count_register_set(0);
		timer0_limit_register_set(0);
		last_load = TIMER_STOPPED;
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t delay;
	uint32_t unannounced;

	ticks = MIN(MAX_TICKS, (uint32_t)(MAX((int32_t)(ticks - 1), 0)));

	k_spinlock_key_t key = k_spin_lock(&lock);


	cycle_count += elapsed();
	/* clear counter early to avoid cycle loss as few as possible,
	 * between cycle_count and clearing 0, few cycles are possible
	 * to loss
	 */
	timer0_count_register_set(0);
	overflow_cycles = 0U;


	/* normal case */
	unannounced = cycle_count - announced_cycles;

	if ((int32_t)unannounced < 0) {
		/* We haven't announced for more than half the 32-bit
		 * wrap duration, because new timeouts keep being set
		 * before the existing one fires. Force an announce
		 * to avoid loss of a wrap event, making sure the
		 * delay is at least the minimum delay possible.
		 */
		last_load = MIN_DELAY;
	} else {
		/* Desired delay in the future */
		delay = ticks * CYC_PER_TICK;

		/* Round delay up to next tick boundary */
		delay += unannounced;
		delay =
		 ((delay + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;

		delay -= unannounced;
		delay = MAX(delay, MIN_DELAY);

		last_load = MIN(delay, MAX_CYCLES);
	}

	timer0_limit_register_set(last_load - 1);
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

	k_spin_unlock(&lock, key);
#endif
#endif
}

uint32_t z_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	uint32_t cyc;
	k_spinlock_key_t key = k_spin_lock(&lock);

#if SMP_TIMER_DRIVER
	cyc = (z_arc_connect_gfrc_read() - last_time);
#else
	cyc =  elapsed() + cycle_count - announced_cycles;
#endif

	k_spin_unlock(&lock, key);

	return cyc / CYC_PER_TICK;
}

uint32_t z_timer_cycle_get_32(void)
{
#if SMP_TIMER_DRIVER
	return z_arc_connect_gfrc_read() - start_time;
#else
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = elapsed() + cycle_count;

	k_spin_unlock(&lock, key);
	return ret;
#endif
}

#if SMP_TIMER_DRIVER
void smp_timer_init(void)
{
	/* set the initial status of timer0 of each slave core
	 */
	timer0_control_register_set(0);
	timer0_count_register_set(0);
	timer0_limit_register_set(0);

	z_irq_priority_set(IRQ_TIMER0, CONFIG_ARCV2_TIMER_IRQ_PRIORITY, 0);
	irq_enable(IRQ_TIMER0);
}
#endif
