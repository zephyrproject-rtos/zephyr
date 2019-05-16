/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * GPTimer has two timers, timer 1 and timer 2.  We use timer 1 as the
 * system tick timer and timer 2 as a HPET.
 */

#include <device.h>
#include <irq.h>
#include <soc.h>
#include <drivers/timer/system_timer.h>

#define TIMER1_IRQ (8)
#define TIMER2_IRQ (9)
#define TIMER_EN (BIT(0))
#define TIMER_RL (BIT(1))
#define TIMER_LD (BIT(2))

/* NOTE: The datasheet for AT697 specifies that it has full 32 bits
 * for both counter register and reload register, but some
 * implementation of QEMU SPARC doesn't handle all 32 bits but masks
 * counter and reload register with 0x00ffffff, which is, in case of
 * 50 MHz system clock, the timer resolution is 20 ns and rollover
 * time is about 335 ms.  */
#define TIMER_RELOAD_VALUE (0x00ffffff)

uint32_t z_timer_cycle_get_32(void)
{
	return TIMER_RELOAD_VALUE - sys_read32(LEON2_TIMER2_CTR);
}

/* tickless kernel is not yet supported */
u32_t z_clock_elapsed(void)
{
	return 0;
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void timer_handler(int irq, void *arg)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(arg);
	z_clock_announce(1);
}
#endif

int z_clock_driver_init(struct device *device)
{
	/* We don't use prescaler for now.  Timers count down at the
	 * speed of the system clock */
	sys_write32(0, LEON2_PSCL_RLD);
	sys_write32(0, LEON2_PSCL_CTR);

	if (IS_ENABLED(CONFIG_SYS_CLOCK_EXISTS)) {
		int ticks;

		IRQ_CONNECT(TIMER1_IRQ, 0, timer_handler, NULL, 0);
		irq_enable(TIMER1_IRQ);

		/* GPTimer triggers at underflow (-1) */
		ticks = k_ticks_to_cyc_floor32(1) - 1;
		sys_write32(ticks, LEON2_TIMER1_RLD);
		sys_write32(TIMER_EN | TIMER_RL | TIMER_LD, LEON2_TIMER1_CTL);
	}

	sys_write32(TIMER_RELOAD_VALUE, LEON2_TIMER2_RLD);
	sys_write32(TIMER_EN | TIMER_RL | TIMER_LD, LEON2_TIMER2_CTL);

	return 0;
}
