/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cpuid.h> /* Header provided by the toolchain. */

#include <zephyr/init.h>
#include <zephyr/arch/x86/cpuid.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <zephyr/irq.h>

/*
 * This driver is selected when either CONFIG_APIC_TIMER_TSC or
 * CONFIG_APIC_TSC_DEADLINE_TIMER is selected. The later is preferred over
 * the former when the TSC deadline comparator is available.
 */
BUILD_ASSERT((!IS_ENABLED(CONFIG_APIC_TIMER_TSC) &&
	       IS_ENABLED(CONFIG_APIC_TSC_DEADLINE_TIMER)) ||
	     (!IS_ENABLED(CONFIG_APIC_TSC_DEADLINE_TIMER) &&
	       IS_ENABLED(CONFIG_APIC_TIMER_TSC)),
	     "one of CONFIG_APIC_TIMER_TSC or CONFIG_APIC_TSC_DEADLINE_TIMER must be set");

/*
 * If the TSC deadline comparator is not supported then the ICR in one-shot
 * mode is used as a fallback method to trigger the next timeout interrupt.
 * Those config symbols must then be defined:
 *
 * CONFIG_APIC_TIMER_TSC_N=<n>
 * CONFIG_APIC_TIMER_TSC_M=<m>
 *
 * These are set to indicate the ratio of the TSC frequency to the local
 * APIC timer frequency. This can be found via CPUID 0x15 (n = EBX, m = EAX)
 * on most CPUs.
 */
#ifdef CONFIG_APIC_TIMER_TSC
#define APIC_TIMER_TSC_M CONFIG_APIC_TIMER_TSC_M
#define APIC_TIMER_TSC_N CONFIG_APIC_TIMER_TSC_N
#else
#define APIC_TIMER_TSC_M 1
#define APIC_TIMER_TSC_N 1
#endif

#define IA32_TSC_DEADLINE_MSR 0x6e0
#define IA32_TSC_ADJUST_MSR   0x03b

#define CYC_PER_TICK (uint32_t)(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC \
				/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* the unsigned long cast limits divisors to native CPU register width */
#define cycle_diff_t unsigned long
#define CYCLE_DIFF_MAX (~(cycle_diff_t)0)

/*
 * We have two constraints on the maximum number of cycles we can wait for.
 *
 * 1) sys_clock_announce() accepts at most INT32_MAX ticks.
 *
 * 2) The number of cycles between two reports must fit in a cycle_diff_t
 *    variable before converting it to ticks.
 *
 * Then:
 *
 * 3) Pick the smallest between (1) and (2).
 *
 * 4) Take into account some room for the unavoidable IRQ servicing latency.
 *    Let's use 3/4 of the max range.
 *
 * Finally let's add the LSB value to the result so to clear out a bunch of
 * consecutive set bits coming from the original max values to produce a
 * nicer literal for assembly generation.
 */
#define CYCLES_MAX_1	((uint64_t)INT32_MAX * (uint64_t)CYC_PER_TICK)
#define CYCLES_MAX_2	((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3	MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4	(CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX	(CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

struct apic_timer_lvt {
	uint8_t vector   : 8;
	uint8_t unused0  : 8;
	uint8_t masked   : 1;
	enum { ONE_SHOT, PERIODIC, TSC_DEADLINE } mode: 2;
	uint32_t unused2 : 13;
};

static struct k_spinlock lock;
static uint64_t last_cycle;
static uint64_t last_tick;
static uint32_t last_elapsed;
static union { uint32_t val; struct apic_timer_lvt lvt; } lvt_reg;

static ALWAYS_INLINE uint64_t rdtsc(void)
{
	uint32_t hi, lo;

	__asm__ volatile("rdtsc" : "=d"(hi), "=a"(lo));
	return lo + (((uint64_t)hi) << 32);
}

static inline void wrmsr(int32_t msr, uint64_t val)
{
	uint32_t hi = (uint32_t) (val >> 32);
	uint32_t lo = (uint32_t) val;

	__asm__ volatile("wrmsr" :: "d"(hi), "a"(lo), "c"(msr));
}

static void set_trigger(uint64_t deadline)
{
	if (IS_ENABLED(CONFIG_APIC_TSC_DEADLINE_TIMER)) {
		wrmsr(IA32_TSC_DEADLINE_MSR, deadline);
	} else {
		/* use the timer ICR to trigger next interrupt */
		uint64_t curr_cycle = rdtsc();
		uint64_t delta_cycles = deadline - MIN(deadline, curr_cycle);
		uint64_t icr = (delta_cycles * APIC_TIMER_TSC_M) / APIC_TIMER_TSC_N;

		/* cap icr to 32 bits, and not zero */
		icr = CLAMP(icr, 1, UINT32_MAX);
		x86_write_loapic(LOAPIC_TIMER_ICR, icr);
	}
}

static void isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t curr_cycle = rdtsc();
	uint64_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (cycle_diff_t)delta_cycles / CYC_PER_TICK;

	last_cycle += (cycle_diff_t)delta_ticks * CYC_PER_TICK;
	last_tick += delta_ticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next_cycle = last_cycle + CYC_PER_TICK;

		set_trigger(next_cycle);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t next_cycle;

	if (ticks == K_TICKS_FOREVER) {
		next_cycle = last_cycle + CYCLES_MAX;
	} else {
		next_cycle = (last_tick + last_elapsed + ticks) * CYC_PER_TICK;
		if ((next_cycle - last_cycle) > CYCLES_MAX) {
			next_cycle = last_cycle + CYCLES_MAX;
		}
	}

	/*
	 * Interpreted strictly, the IA SDM description of the
	 * TSC_DEADLINE MSR implies that it will trigger an immediate
	 * interrupt if we try to set an expiration across the 64 bit
	 * rollover.  Unfortunately there's no way to test that as on
	 * real hardware it requires more than a century of uptime,
	 * but this is cheap and safe.
	 */
	if (next_cycle < last_cycle) {
		next_cycle = UINT64_MAX;
	}
	set_trigger(next_cycle);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t curr_cycle = rdtsc();
	uint64_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (cycle_diff_t)delta_cycles / CYC_PER_TICK;

	last_elapsed = delta_ticks;
	k_spin_unlock(&lock, key);
	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t) rdtsc();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return rdtsc();
}

static inline uint32_t timer_irq(void)
{
	/* The Zephyr APIC API is... idiosyncratic.  The timer is a
	 * "local vector table" interrupt.  These aren't system IRQs
	 * presented to the IO-APIC, they're indices into a register
	 * array in the local APIC.  By Zephyr convention they come
	 * after all the external IO-APIC interrupts, but that number
	 * changes depending on device configuration so we have to
	 * fetch it at runtime.  The timer happens to be the first
	 * entry in the table.
	 */
	return z_loapic_irq_base();
}

/* The TSC_ADJUST MSR implements a synchronized offset such that
 * multiple CPUs (within a socket, anyway) can synchronize exactly, or
 * implement managed timing spaces for guests in a recoverable way,
 * etc...  We set it to zero on all cores for simplicity, because
 * firmware often leaves it in an inconsistent state between cores.
 */
static void clear_tsc_adjust(void)
{
	/* But don't touch it on ACRN, where an hypervisor bug
	 * confuses the APIC emulation and deadline interrupts don't
	 * arrive.
	 */
#ifndef CONFIG_BOARD_ACRN
	wrmsr(IA32_TSC_ADJUST_MSR, 0);
#endif
}

void smp_timer_init(void)
{
	/* Copy the LVT configuration from CPU0, because IRQ_CONNECT()
	 * doesn't know how to manage LVT interrupts for anything
	 * other than the calling/initial CPU.  Same fence needed to
	 * prevent later MSR writes from reordering before the APIC
	 * configuration write.
	 */
	x86_write_loapic(LOAPIC_TIMER, lvt_reg.val);
	__asm__ volatile("mfence" ::: "memory");
	clear_tsc_adjust();
	irq_enable(timer_irq());
}

static int sys_clock_driver_init(void)
{
#ifdef CONFIG_ASSERT
	uint32_t eax, ebx, ecx, edx;

	if (IS_ENABLED(CONFIG_APIC_TSC_DEADLINE_TIMER)) {
		ecx = 0; /* prevent compiler warning */
		__get_cpuid(CPUID_BASIC_INFO_1, &eax, &ebx, &ecx, &edx);
		__ASSERT((ecx & BIT(24)) != 0, "No TSC Deadline support");
	}

	edx = 0; /* prevent compiler warning */
	__get_cpuid(0x80000007, &eax, &ebx, &ecx, &edx);
	__ASSERT((edx & BIT(8)) != 0, "No Invariant TSC support");

	if (IS_ENABLED(CONFIG_SMP)) {
		ebx = 0; /* prevent compiler warning */
		__get_cpuid_count(CPUID_EXTENDED_FEATURES_LVL, 0, &eax, &ebx, &ecx, &edx);
		__ASSERT((ebx & BIT(1)) != 0, "No TSC_ADJUST MSR support");
	}
#endif

	if (IS_ENABLED(CONFIG_SMP)) {
		clear_tsc_adjust();
	}

	/* Timer interrupt number is runtime-fetched, so can't use
	 * static IRQ_CONNECT()
	 */
	irq_connect_dynamic(timer_irq(), CONFIG_APIC_TIMER_IRQ_PRIORITY, isr, 0, 0);

	if (IS_ENABLED(CONFIG_APIC_TIMER_TSC)) {
		uint32_t timer_conf;

		timer_conf = x86_read_loapic(LOAPIC_TIMER_CONFIG);
		timer_conf &= ~0x0f; /* clear divider bits */
		timer_conf |=  0x0b; /* divide by 1 */
		x86_write_loapic(LOAPIC_TIMER_CONFIG, timer_conf);
	}

	lvt_reg.val = x86_read_loapic(LOAPIC_TIMER);
	lvt_reg.lvt.mode = IS_ENABLED(CONFIG_APIC_TSC_DEADLINE_TIMER) ?
		TSC_DEADLINE : ONE_SHOT;
	lvt_reg.lvt.masked = 0;
	x86_write_loapic(LOAPIC_TIMER, lvt_reg.val);

	/* Per the SDM, the TSC_DEADLINE MSR is not serializing, so
	 * this fence is needed to be sure that an upcoming MSR write
	 * (i.e. a timeout we're about to set) cannot possibly reorder
	 * around the initialization we just did.
	 */
	__asm__ volatile("mfence" ::: "memory");

	last_tick = rdtsc() / CYC_PER_TICK;
	last_cycle = last_tick * CYC_PER_TICK;
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		set_trigger(last_cycle + CYC_PER_TICK);
	}
	irq_enable(timer_irq());

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
