/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LEON2 has two timers, timer 1 and timer 2.  We use timer 1 as the
 * system tick timer and timer 2 as a HPET.
 *
 * LEON2 Timer unit only has 24 bit counters, which means, in case of
 * 50 MHz system clock, the timer resolution is 20 ns and rollover
 * time is about 335 ms.  335 ms is a bit too short; the boot delay
 * value for tests/kernel/common/ is 500 ms.  We use 12.5 MHz as the
 * default value of CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC along with a
 * prescaler value of 4.  The actual value to write to the register
 * file is 3 because the timer unit reloads the value when
 * underflows).  This configuration yields 1340 ms or 1.34 sec
 * duration and 80 ns resolution.
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

#define SYSTEM_CLOCK_IN_HZ 50000000 /* 50 MHz */

#define TIMER_RELOAD_VALUE (0x00ffffff)
#define TIMER_PRESCALER_VALUE                                                  \
	((SYSTEM_CLOCK_IN_HZ / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) - 1)

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
	/* Set prescaler */
	sys_write32(TIMER_PRESCALER_VALUE, LEON2_PSCL_RLD);

	if (IS_ENABLED(CONFIG_SYS_CLOCK_EXISTS)) {
		int ticks;

		IRQ_CONNECT(TIMER1_IRQ, 0, timer_handler, NULL, 0);
		irq_enable(TIMER1_IRQ);

		/* LEON2 Timer triggers when it underflows (-1) */
		ticks = k_ticks_to_cyc_floor32(1) - 1;
		sys_write32(ticks, LEON2_TIMER1_RLD);
		sys_write32(TIMER_EN | TIMER_RL | TIMER_LD, LEON2_TIMER1_CTL);
	}

	sys_write32(TIMER_RELOAD_VALUE, LEON2_TIMER2_RLD);
	sys_write32(TIMER_EN | TIMER_RL | TIMER_LD, LEON2_TIMER2_CTL);

	return 0;
}
