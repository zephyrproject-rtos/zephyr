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
#define MIN_DELAY 512
#define COUNTER_MAX 0xffffffff
#define TIMER_STOPPED 0x0
#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

static struct k_spinlock lock;


#ifdef CONFIG_SMP
volatile static u64_t last_time;
volatile static u64_t start_time;

#else
static u32_t last_load;

static u32_t cycle_count;
#endif

/**
 *
 * @brief Get contents of Timer0 count register
 *
 * @return Current Timer0 count
 */
static ALWAYS_INLINE u32_t timer0_count_register_get(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
}

/**
 *
 * @brief Set Timer0 count register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_count_register_set(u32_t value)
{
	z_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, value);
}

/**
 *
 * @brief Get contents of Timer0 control register
 *
 * @return N/A
 */
static ALWAYS_INLINE u32_t timer0_control_register_get(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_TMR0_CONTROL);
}

/**
 *
 * @brief Set Timer0 control register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_control_register_set(u32_t value)
{
	z_arc_v2_aux_reg_write(_ARC_V2_TMR0_CONTROL, value);
}

/**
 *
 * @brief Get contents of Timer0 limit register
 *
 * @return N/A
 */
static ALWAYS_INLINE u32_t timer0_limit_register_get(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
}

/**
 *
 * @brief Set Timer0 limit register to the specified value
 *
 * @return N/A
 */
static ALWAYS_INLINE void timer0_limit_register_set(u32_t count)
{
	z_arc_v2_aux_reg_write(_ARC_V2_TMR0_LIMIT, count);
}

#ifndef CONFIG_SMP
static u32_t elapsed(void)
{
	u32_t val, ov, ctrl;

	do {
		val =  timer0_count_register_get();
		ctrl = timer0_control_register_get();
	} while (timer0_count_register_get() < val);

	ov = (ctrl & _ARC_V2_TMR_CTRL_IP) ? last_load : 0;
	return val + ov;
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
static void timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);
	u32_t dticks;

	/* clear the interrupt by writing 0 to IP bit of the control register */
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

#ifdef CONFIG_SMP
	u64_t curr_time;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	/* gfrc is the wall clock */
	curr_time = z_arc_connect_gfrc_read();

	dticks = (curr_time - last_time) / CYC_PER_TICK;
	last_time = curr_time;

	k_spin_unlock(&lock, key);

	z_clock_announce(dticks);
#else
	cycle_count += last_load;
	dticks = last_load / CYC_PER_TICK;
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
int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	/* ensure that the timer will not generate interrupts */
	timer0_control_register_set(0);

#ifdef CONFIG_SMP
	IRQ_CONNECT(IRQ_TIMER0, CONFIG_ARCV2_TIMER_IRQ_PRIORITY,
		    timer_int_handler, NULL, 0);

	timer0_limit_register_set(CYC_PER_TICK - 1);
	last_time = z_arc_connect_gfrc_read();
	start_time = last_time;
#else
	last_load = CYC_PER_TICK;

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

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	/* If the kernel allows us to miss tick announcements in idle,
	 * then shut off the counter. (Note: we can assume if idle==true
	 * that interrupts are already disabled)
	 */
#ifdef CONFIG_SMP
	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && idle && ticks == K_FOREVER) {
		timer0_control_register_set(0);
		timer0_count_register_set(0);
		timer0_limit_register_set(0);
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	u32_t delay;
	u32_t key;

	ticks = MIN(MAX_TICKS, MAX(ticks - 1, 0));

	/* Desired delay in the future */
	delay = (ticks == 0) ? CYC_PER_TICK : ticks * CYC_PER_TICK;

	key = z_arch_irq_lock();

	timer0_limit_register_set(delay - 1);
	timer0_count_register_set(0);
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH |
						_ARC_V2_TMR_CTRL_IE);

	z_arch_irq_unlock(key);
#endif
#else
	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && idle && ticks == K_FOREVER) {
		timer0_control_register_set(0);
		timer0_count_register_set(0);
		timer0_limit_register_set(0);
		last_load = TIMER_STOPPED;
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	u32_t delay;

	ticks = MIN(MAX_TICKS, MAX(ticks - 1, 0));

	/* Desired delay in the future */
	delay = (ticks == 0) ? MIN_DELAY : ticks * CYC_PER_TICK;

	k_spinlock_key_t key = k_spin_lock(&lock);

	delay += elapsed();

	/* Round delay up to next tick boundary */
	delay = ((delay + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;

	if (last_load != delay) {
		if (timer0_control_register_get() & _ARC_V2_TMR_CTRL_IP) {
			delay -= last_load;
		}
		timer0_limit_register_set(delay - 1);
		last_load = delay;
		timer0_control_register_set(_ARC_V2_TMR_CTRL_NH |
							 _ARC_V2_TMR_CTRL_IE);
	}

	k_spin_unlock(&lock, key);
#endif
#endif
}

u32_t z_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	u32_t cyc;
	k_spinlock_key_t key = k_spin_lock(&lock);

#ifdef CONFIG_SMP
	cyc = (z_arc_connect_gfrc_read() - last_time) / CYC_PER_TICK;
#else
	cyc = elapsed() / CYC_PER_TICK;
#endif

	k_spin_unlock(&lock, key);

	return cyc;
}

u32_t z_timer_cycle_get_32(void)
{
#ifdef CONFIG_SMP
	return z_arc_connect_gfrc_read() - start_time;
#else
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = elapsed() + cycle_count;

	k_spin_unlock(&lock, key);
	return ret;
#endif
}

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

	irq_disable(IRQ_TIMER0);
}


#ifdef CONFIG_SMP
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
