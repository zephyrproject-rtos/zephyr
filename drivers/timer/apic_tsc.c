/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <zephyr/irq.h>

#define IA32_TSC_DEADLINE_MSR 0x6e0
#define IA32_TSC_ADJUST_MSR   0x03b

#define CYC_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC \
		      / (uint64_t) CONFIG_SYS_CLOCK_TICKS_PER_SEC)

struct apic_timer_lvt {
	uint8_t vector   : 8;
	uint8_t unused0  : 8;
	uint8_t masked   : 1;
	enum { ONE_SHOT, PERIODIC, TSC_DEADLINE } mode: 2;
	uint32_t unused2 : 13;
};

static struct k_spinlock lock;
static uint64_t last_announce;
static union { uint32_t val; struct apic_timer_lvt lvt; } lvt_reg;

static ALWAYS_INLINE uint64_t rdtsc(void)
{
	uint32_t hi, lo;

	__asm__ volatile("rdtsc" : "=d"(hi), "=a"(lo));
	return lo + (((uint64_t)hi) << 32);
}

static void isr(const void *arg)
{
	ARG_UNUSED(arg);
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ticks = (rdtsc() - last_announce) / CYC_PER_TICK;

	last_announce += ticks * CYC_PER_TICK;
	k_spin_unlock(&lock, key);
	sys_clock_announce(ticks);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		sys_clock_set_timeout(1, false);
	}
}

static inline void wrmsr(int32_t msr, uint64_t val)
{
	uint32_t hi = (uint32_t) (val >> 32);
	uint32_t lo = (uint32_t) val;

	__asm__ volatile("wrmsr" :: "d"(hi), "a"(lo), "c"(msr));
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	uint64_t now = rdtsc();
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t expires = now + MAX(ticks - 1, 0) * CYC_PER_TICK;

	expires = last_announce + (((expires - last_announce + CYC_PER_TICK - 1)
				    / CYC_PER_TICK) * CYC_PER_TICK);

	/* The second condition is to catch the wraparound.
	 * Interpreted strictly, the IA SDM description of the
	 * TSC_DEADLINE MSR implies that it will trigger an immediate
	 * interrupt if we try to set an expiration across the 64 bit
	 * rollover.  Unfortunately there's no way to test that as on
	 * real hardware it requires more than a century of uptime,
	 * but this is cheap and safe.
	 */
	if (ticks == K_TICKS_FOREVER || expires < last_announce) {
		expires = UINT64_MAX;
	}

	wrmsr(IA32_TSC_DEADLINE_MSR, expires);
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (rdtsc() - last_announce) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
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

static inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	__asm__ volatile("cpuid"
			 : "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
			 : "a"(*eax), "c"(*ecx));
}

static int sys_clock_driver_init(void)
{
#ifdef CONFIG_ASSERT
	uint32_t eax, ebx, ecx, edx;

	eax = 1; ecx = 0;
	cpuid(&eax, &ebx, &ecx, &edx);
	__ASSERT((ecx & BIT(24)) != 0, "No TSC Deadline support");

	eax = 0x80000007; ecx = 0;
	cpuid(&eax, &ebx, &ecx, &edx);
	__ASSERT((edx & BIT(8)) != 0, "No Invariant TSC support");

	eax = 7; ecx = 0;
	cpuid(&eax, &ebx, &ecx, &edx);
	__ASSERT((ebx & BIT(1)) != 0, "No TSC_ADJUST MSR support");
#endif

	clear_tsc_adjust();

	/* Timer interrupt number is runtime-fetched, so can't use
	 * static IRQ_CONNECT()
	 */
	irq_connect_dynamic(timer_irq(), CONFIG_APIC_TIMER_IRQ_PRIORITY, isr, 0, 0);

	lvt_reg.val = x86_read_loapic(LOAPIC_TIMER);
	lvt_reg.lvt.mode = TSC_DEADLINE;
	lvt_reg.lvt.masked = 0;
	x86_write_loapic(LOAPIC_TIMER, lvt_reg.val);

	/* Per the SDM, the TSC_DEADLINE MSR is not serializing, so
	 * this fence is needed to be sure that an upcoming MSR write
	 * (i.e. a timeout we're about to set) cannot possibly reorder
	 * around the initialization we just did.
	 */
	__asm__ volatile("mfence" ::: "memory");

	last_announce = rdtsc();
	irq_enable(timer_irq());

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		sys_clock_set_timeout(1, false);
	}

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
