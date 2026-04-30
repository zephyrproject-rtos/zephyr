/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 * Copyright (c) 2018-2023 Intel Corporation
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/sbi.h>

#define CYC_PER_TICK (uint32_t)(sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* the unsigned long cast limits divisions to native CPU register width */
#define cycle_diff_t   unsigned long
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
#define CYCLES_MAX_1 ((uint64_t)INT32_MAX * (uint64_t)CYC_PER_TICK)
#define CYCLES_MAX_2 ((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3 MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4 (CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX   (CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = IRQ_S_TIMER;
#endif

static struct k_spinlock lock;
static uint64_t last_count;
static uint64_t last_ticks;
static uint32_t last_elapsed;

static void sbi_set_timer(uint64_t deadline)
{
	register unsigned long a0 __asm__("a0") = (unsigned long)deadline;
	register unsigned long a6 __asm__("a6") = SBI_FUNC_SET_TIMER;
	register unsigned long a7 __asm__("a7") = SBI_EXT_TIME;

	__asm__ volatile("ecall"
					: "+r"(a0)
					: "r"(a6), "r"(a7)
					: "a1", "memory");
}

static uint64_t stime(void)
{
	return csr_read(time);
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t now = stime();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYC_PER_TICK;

	last_count += (cycle_diff_t)dticks * CYC_PER_TICK;
	last_ticks += dticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next = last_count + CYC_PER_TICK;

		sbi_set_timer(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(dticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t cyc;

	if (ticks == K_TICKS_FOREVER) {
		cyc = last_count + CYCLES_MAX;
	} else {
		cyc = (last_ticks + last_elapsed + ticks) * CYC_PER_TICK;
		if ((cyc - last_count) > CYCLES_MAX) {
			cyc = last_count + CYCLES_MAX;
		}
	}
	sbi_set_timer(cyc);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = stime();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYC_PER_TICK;

	last_elapsed = dticks;
	k_spin_unlock(&lock, key);
	return dticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)stime();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return stime();
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(IRQ_S_TIMER, 0, timer_isr, NULL, 0);
	last_ticks = stime() / CYC_PER_TICK;
	last_count = last_ticks * CYC_PER_TICK;
	sbi_set_timer(last_count + CYC_PER_TICK);
	irq_enable(IRQ_S_TIMER);
	return 0;
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	sbi_set_timer(last_count + CYC_PER_TICK);
	irq_enable(IRQ_S_TIMER);
}
#endif

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
