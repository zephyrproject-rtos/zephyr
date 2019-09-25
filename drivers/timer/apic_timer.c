/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <drivers/interrupt_controller/loapic.h>

BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_SMP), "APIC timer doesn't support SMP");

/*
 * Overview:
 *
 * This driver enables the local APIC as the Zephyr system timer. It supports
 * both legacy ("tickful") mode as well as TICKLESS_KERNEL. The driver will
 * work with any APIC that has the ARAT "always running APIC timer" feature
 * (CPUID 0x06, EAX bit 2); for the more accurate z_timer_cycle_get_32(),
 * the invariant TSC feature (CPUID 0x80000007: EDX bit 8) is also required.
 * (Ultimately systems with invariant TSCs should use a TSC-based driver,
 * and the TSC-related parts should be stripped from this implementation.)
 *
 * Configuration:
 *
 * CONFIG_APIC_TIMER=y			enables this timer driver.
 * CONFIG_APIC_TIMER_IRQ=<irq>		which IRQ to configure for the timer.
 * CONFIG_APIC_TIMER_IRQ_PRIORITY=<p>	priority for IRQ_CONNECT()
 *
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=<hz> must contain the frequency seen
 *     by the local APIC timer block (before it gets to the timer divider).
 *
 * CONFIG_APIC_TIMER_TSC=y enables the more accurate TSC-based cycle counter
 *     for z_timer_cycle_get_32(). This also requires the next options be set.
 *
 * CONFIG_APIC_TIMER_TSC_N=<n>
 * CONFIG_APIC_TIMER_TSC_M=<m>
 *     When CONFIG_APIC_TIMER_TSC=y, these are set to indicate the ratio of
 *     the TSC frequency to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC. This can be
 *     found via CPUID 0x15 (n = EBX, m = EAX) on most CPUs.
 */

/* These should be merged into include/drivers/interrupt_controller/loapic.h. */

#define DCR_DIVIDER_MASK	0x0000000F	/* divider bits */
#define DCR_DIVIDER		0x0000000B	/* divide by 1 */
#define LVT_MODE_MASK		0x00060000	/* timer mode bits */
#define LVT_MODE		0x00000000	/* one-shot */

/*
 * CYCLES_PER_TICK must always be at least '2', otherwise MAX_TICKS
 * will overflow s32_t, which is how 'ticks' are currently represented.
 */

#define CYCLES_PER_TICK \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

BUILD_ASSERT_MSG(CYCLES_PER_TICK >= 2, "APIC timer: bad CYCLES_PER_TICK");

/* max number of ticks we can load into the timer in one shot */

#define MAX_TICKS (0xFFFFFFFFU / CYCLES_PER_TICK)

/*
 * The spinlock protects all access to the local APIC timer registers,
 * as well as 'total_cycles', 'last_announcement', and 'cached_icr'.
 *
 * One important invariant that must be observed: `total_cycles` + `cached_icr`
 * is always an integral multiple of CYCLE_PER_TICK; this is, timer interrupts
 * are only ever scheduled to occur at tick boundaries.
 */

static struct k_spinlock lock;
static u64_t total_cycles;
static u32_t cached_icr = CYCLES_PER_TICK;

#ifdef CONFIG_TICKLESS_KERNEL

static u64_t last_announcement;	/* last time we called z_clock_announce() */

void z_clock_set_timeout(s32_t n, bool idle)
{
	ARG_UNUSED(idle);

	u32_t ccr;
	int   full_ticks;	/* number of complete ticks we'll wait */
	u32_t full_cycles;	/* full_ticks represented as cycles */
	u32_t partial_cycles;	/* number of cycles to first tick boundary */

	if (n < 1) {
		full_ticks = 0;
	} else if ((n == K_FOREVER) || (n > MAX_TICKS)) {
		full_ticks = MAX_TICKS - 1;
	} else {
		full_ticks = n - 1;
	}

	full_cycles = full_ticks * CYCLES_PER_TICK;

	/*
	 * There's a wee race condition here. The timer may expire while
	 * we're busy reprogramming it; an interrupt will be queued at the
	 * local APIC and the ISR will be called too early, roughly right
	 * after we unlock, and not because the count we just programmed has
	 * counted down. Luckily this situation is easy to detect, which is
	 * why the ISR actually checks to be sure the CCR is 0 before acting.
	 */

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = x86_read_loapic(LOAPIC_TIMER_CCR);
	total_cycles += (cached_icr - ccr);
	partial_cycles = CYCLES_PER_TICK - (total_cycles % CYCLES_PER_TICK);
	cached_icr = full_cycles + partial_cycles;
	x86_write_loapic(LOAPIC_TIMER_ICR, cached_icr);

	k_spin_unlock(&lock, key);
}

u32_t z_clock_elapsed(void)
{
	u32_t ccr;
	u32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);
	ccr = x86_read_loapic(LOAPIC_TIMER_CCR);
	ticks = total_cycles - last_announcement;
	ticks += cached_icr - ccr;
	k_spin_unlock(&lock, key);
	ticks /= CYCLES_PER_TICK;

	return ticks;
}

static void isr(void *arg)
{
	ARG_UNUSED(arg);

	u32_t cycles;
	s32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);

	/*
	 * If we get here and the CCR isn't zero, then this interrupt is
	 * stale: it was queued while z_clock_set_timeout() was setting
	 * a new counter. Just ignore it. See above for more info.
	 */

	if (x86_read_loapic(LOAPIC_TIMER_CCR) != 0) {
		k_spin_unlock(&lock, key);
		return;
	}

	/* Restart the timer as early as possible to minimize drift... */
	x86_write_loapic(LOAPIC_TIMER_ICR, MAX_TICKS * CYCLES_PER_TICK);

	cycles = cached_icr;
	cached_icr = MAX_TICKS * CYCLES_PER_TICK;
	total_cycles += cycles;
	ticks = (total_cycles - last_announcement) / CYCLES_PER_TICK;
	last_announcement = total_cycles;
	k_spin_unlock(&lock, key);
	z_clock_announce(ticks);
}

#else

static void isr(void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	total_cycles += CYCLES_PER_TICK;
	x86_write_loapic(LOAPIC_TIMER_ICR, cached_icr);
	k_spin_unlock(&lock, key);

	z_clock_announce(1);
}

u32_t z_clock_elapsed(void)
{
	return 0U;
}

#endif /* CONFIG_TICKLESS_KERNEL */

#ifdef CONFIG_APIC_TIMER_TSC

u32_t z_timer_cycle_get_32(void)
{
	u64_t tsc = z_tsc_read();
	u32_t cycles;

	cycles = (tsc * CONFIG_APIC_TIMER_TSC_M) / CONFIG_APIC_TIMER_TSC_N;
	return cycles;
}

#else

u32_t z_timer_cycle_get_32(void)
{
	u32_t ret;
	u32_t ccr;

	k_spinlock_key_t key = k_spin_lock(&lock);
	ccr = x86_read_loapic(LOAPIC_TIMER_CCR);
	ret = total_cycles + (cached_icr - ccr);
	k_spin_unlock(&lock, key);

	return ret;
}

#endif

int z_clock_driver_init(struct device *device)
{
	u32_t val;

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

	x86_write_loapic(LOAPIC_TIMER_ICR, cached_icr);
	irq_enable(CONFIG_APIC_TIMER_IRQ);

	return 0;
}
