/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2018 Synopsys Inc, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/arc/v2/aux_regs.h>
#include <soc.h>
/*
 * note: This implementation assumes Timer0 is present. Be sure
 * to build the ARC CPU with Timer0.
 */


#define _ARC_V2_TMR_CTRL_IE 0x1 /* interrupt enable */
#define _ARC_V2_TMR_CTRL_NH 0x2 /* count only while not halted */
#define _ARC_V2_TMR_CTRL_W  0x4 /* watchdog mode enable */
#define _ARC_V2_TMR_CTRL_IP 0x8 /* interrupt pending flag */

/* Minimum cycles in the future to try to program. */
#define MIN_DELAY 512
#define COUNTER_MAX 0xffffffff
#define TIMER_STOPPED 0x0
#define CYC_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

static struct k_spinlock lock;

static u32_t last_load;

static u32_t cycle_count;


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
static void _timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);
	u32_t dticks;

	/* clear the interrupt by writing 0 to IP bit of the control register */
	timer0_control_register_set(_ARC_V2_TMR_CTRL_NH | _ARC_V2_TMR_CTRL_IE);

	cycle_count += last_load;
	dticks = last_load / CYC_PER_TICK;

	z_clock_announce(TICKLESS ? dticks : 1);

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

	last_load = CYC_PER_TICK;

	IRQ_CONNECT(IRQ_TIMER0, CONFIG_ARCV2_TIMER_IRQ_PRIORITY,
		    _timer_int_handler, NULL, 0);

	timer0_limit_register_set(last_load - 1);
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
}

u32_t z_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t cyc = elapsed();

	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
}

u32_t _timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = elapsed() + cycle_count;

	k_spin_unlock(&lock, key);
	return ret;
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
