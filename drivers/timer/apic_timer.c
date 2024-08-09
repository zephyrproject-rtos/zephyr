/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <zephyr/irq.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_TICKLESS_KERNEL), "this is a tickfull driver");

/*
 * Overview:
 *
 * This driver enables the local APIC as the Zephyr system timer. It supports
 * legacy ("tickful") mode only. The driver will work with any APIC that has
 * the ARAT "always running APIC timer" feature (CPUID 0x06, EAX bit 2).
 *
 * Configuration:
 *
 * CONFIG_APIC_TIMER=y			enables this timer driver.
 * CONFIG_APIC_TIMER_IRQ=<irq>		which IRQ to configure for the timer.
 * CONFIG_APIC_TIMER_IRQ_PRIORITY=<p>	priority for IRQ_CONNECT()
 *
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=<hz> must contain the frequency seen
 *     by the local APIC timer block (before it gets to the timer divider).
 */

/* These should be merged into include/drivers/interrupt_controller/loapic.h. */

#define DCR_DIVIDER_MASK	0x0000000F	/* divider bits */
#define DCR_DIVIDER		0x0000000B	/* divide by 1 */
#define LVT_MODE_MASK		0x00060000	/* timer mode bits */
#define LVT_MODE		0x00020000	/* periodic mode */

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = CONFIG_APIC_TIMER_IRQ;
#endif

#define CYCLES_PER_TICK \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

BUILD_ASSERT(CYCLES_PER_TICK >= 1, "APIC timer: bad CYCLES_PER_TICK");

static volatile uint64_t total_cycles;

static void isr(const void *arg)
{
	ARG_UNUSED(arg);

	total_cycles += CYCLES_PER_TICK;
	sys_clock_announce(1);
}

uint32_t sys_clock_elapsed(void)
{
	return 0U;
}

uint64_t sys_clock_cycle_get_64(void)
{
	uint32_t ccr_1st, ccr_2nd;
	uint64_t cycles;

       /*
	* We may race with CCR reaching 0 and reloading, and the interrupt
	* handler updating total_cycles. Let's make sure we sample everything
	* away from this roll-over transition by ensuring consecutive CCR
	* values are descending so we're sure the enclosed (volatile)
	* total_cycles sample and CCR value are coherent with each other.
	*/
	do {
		ccr_1st = x86_read_loapic(LOAPIC_TIMER_CCR);
		cycles = total_cycles;
		ccr_2nd = x86_read_loapic(LOAPIC_TIMER_CCR);
	} while (ccr_2nd > ccr_1st);

	return cycles + (CYCLES_PER_TICK - ccr_2nd);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)sys_clock_cycle_get_64();
}

int init_sys_clock_driver(void)
{
	uint32_t val;


	val = x86_read_loapic(LOAPIC_TIMER_CONFIG);	/* set divider */
	val &= ~DCR_DIVIDER_MASK;
	val |= DCR_DIVIDER;
	x86_write_loapic(LOAPIC_TIMER_CONFIG, val);

	val = x86_read_loapic(LOAPIC_TIMER);		/* set timer mode */
	val &= ~LVT_MODE_MASK;
	val |= LVT_MODE;
	x86_write_loapic(LOAPIC_TIMER, val);

	/* remember, wiring up the interrupt will mess with the LVT, too */

	IRQ_CONNECT(CONFIG_APIC_TIMER_IRQ,
		CONFIG_APIC_TIMER_IRQ_PRIORITY,
		isr, 0, 0);

	x86_write_loapic(LOAPIC_TIMER_ICR, CYCLES_PER_TICK);
	irq_enable(CONFIG_APIC_TIMER_IRQ);

	return 0;
}
