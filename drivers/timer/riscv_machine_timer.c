/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <drivers/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <soc.h>

#define CYC_PER_TICK ((u64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
			      / (u64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((0xffffffffu - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL) &&		\
		  !IS_ENABLED(CONFIG_QEMU_TICKLESS_WORKAROUND))

static struct k_spinlock lock;
#ifdef CONFIG_SMP
static u64_t last_count[CONFIG_MP_NUM_CPUS];
#else
static u64_t last_count[1];
#endif

static void set_mtimecmp(u64_t time)
{
	/* Get a pointer to the MTIMECMP array */
	volatile u32_t *r = (u32_t *)RISCV_MTIMECMP_BASE;

	/* Get the current hart id */
#ifdef CONFIG_SMP
	u32_t hartid = _current_cpu->id;
#else
	u32_t hartid = 0;
#endif

	/* Calculate the indexes into the mtimecmp array for the current hart */
	u32_t low_index = hartid * 2;
	u32_t high_index = low_index + 1;

	/* Per spec, the RISC-V MTIME/MTIMECMP registers are 64 bit,
	 * but are NOT internally latched for multiword transfers.  So
	 * we have to be careful about sequencing to avoid triggering
	 * spurious interrupts: always set the high word to a max
	 * value first.
	 */
	r[high_index] = 0xffffffff;
	r[low_index] = (u32_t)time;
	r[high_index] = (u32_t)(time >> 32);
}

static u64_t mtime(void)
{
	volatile u32_t *r = (u32_t *)RISCV_MTIME_BASE;
	u32_t lo, hi;

	/* Likewise, must guard against rollover when reading */
	do {
		hi = r[1];
		lo = r[0];
	} while (r[1] != hi);

	return (((u64_t)hi) << 32) | lo;
}

static void timer_isr(void *arg)
{
	ARG_UNUSED(arg);
#ifdef CONFIG_SMP
	u32_t hartid = _current_cpu->id;
#else
	u32_t hartid = 0;
#endif

	k_spinlock_key_t key = k_spin_lock(&lock);
	u64_t now = mtime();
	u64_t dticks = (now - last_count[hartid]) / CYC_PER_TICK;

	last_count[hartid] += dticks * CYC_PER_TICK;

	if (!TICKLESS) {
		u64_t next = last_count[hartid] + CYC_PER_TICK;

		while((s64_t)(next - now) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_mtimecmp(next);
	}

	k_spin_unlock(&lock, key);

	if(hartid == 0) {
		z_clock_announce(dticks);
	}
}

int z_clock_driver_init(struct device *device)
{
	last_count[0] = mtime();
	IRQ_CONNECT(RISCV_MACHINE_TIMER_IRQ, 0, timer_isr, NULL, 0);
	set_mtimecmp(last_count[0] + CYC_PER_TICK);
	irq_enable(RISCV_MACHINE_TIMER_IRQ);
	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_QEMU_TICKLESS_WORKAROUND)
	/* RISCV has no idle handler yet, so if we try to spin on the
	 * logic below to reset the comparator, we'll always bump it
	 * forward to the "next tick" due to MIN_DELAY handling and
	 * the interrupt will never fire!  Just rely on the fact that
	 * the OS gave us the proper timeout already.
	 */
	if (idle) {
		return;
	}

#ifdef CONFIG_SMP
	u32_t hartid = _current_cpu->id;
#else
	u32_t hartid = 0;
#endif

	ticks = ticks == K_FOREVER ? MAX_TICKS : ticks;
	ticks = max(min(ticks - 1, (s32_t)MAX_TICKS), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	u64_t now = mtime();
	u32_t cyc = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary.  Note use of 32 bit math,
	 * max_ticks is calibrated to permit this.
	 */
	cyc += (u32_t)(now - last_count[hartid]) + (CYC_PER_TICK - 1);
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	if ((s32_t)(cyc + last_count[hartid] - now) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	set_mtimecmp(cyc + last_count[hartid]);
	k_spin_unlock(&lock, key);
#endif
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

#ifdef CONFIG_SMP
	u32_t hartid = _current_cpu->id;
#else
	u32_t hartid = 0;
#endif

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = ((u32_t)mtime() - (u32_t)last_count[hartid]) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

u32_t _timer_cycle_get_32(void)
{
	return (u32_t)mtime();
}

#ifdef CONFIG_SMP
void smp_timer_init(void) {
	u64_t now = mtime();
	u32_t mhartid = _current_cpu->id;
	last_count[mhartid] = now;
	set_mtimecmp(now + CYC_PER_TICK);
	irq_enable(RISCV_MACHINE_TIMER_IRQ);
}
#endif /* CONFIG_SMP */

