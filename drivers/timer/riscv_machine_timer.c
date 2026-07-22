/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT riscv_machine_timer

#define MTIME_REG    DT_INST_REG_ADDR_BY_NAME(0, mtime)
#define MTIMECMP_REG DT_INST_REG_ADDR_BY_NAME(0, mtimecmp)
#define TIMER_IRQN   DT_INST_IRQN(0)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQN;
#endif

static uintptr_t get_hart_mtimecmp(void)
{
	return MTIMECMP_REG + (arch_proc_id() * 8);
}

static void set_mtimecmp(uint64_t time)
{
#ifdef CONFIG_64BIT
	*(volatile uint64_t *)get_hart_mtimecmp() = time;
#else
	volatile uint32_t *r = (uint32_t *)get_hart_mtimecmp();

	/* Per spec, the RISC-V MTIME/MTIMECMP registers are 64 bit,
	 * but are NOT internally latched for multiword transfers.  So
	 * we have to be careful about sequencing to avoid triggering
	 * spurious interrupts: always set the high word to a max
	 * value first.
	 */
	r[1] = 0xffffffff;
	r[0] = (uint32_t)time;
	r[1] = (uint32_t)(time >> 32);
#endif
}

static uint64_t mtime(void)
{
#ifdef CONFIG_64BIT
	return *(volatile uint64_t *)MTIME_REG;
#else
	volatile uint32_t *r = (uint32_t *)MTIME_REG;
	uint32_t lo, hi;

	/* Likewise, must guard against rollover when reading */
	do {
		hi = r[1];
		lo = r[0];
	} while (r[1] != hi);

	return (((uint64_t)hi) << 32) | lo;
#endif
}

/*
 * Free-running mtime counter plus an absolute mtimecmp compare: a COMPARE
 * backend. The generic core owns the tick accounting; this driver only reads
 * the counter and arms the comparator. The public cycle counter is the raw
 * mtime scaled by the DT divider, so it overrides the core's default.
 */
#define TIMER_CORE_BACKEND_COMPARE
#define TIMER_CORE_HAVE_CYCLE_GET_32
#define TIMER_CORE_HAVE_CYCLE_GET_64

static inline uint64_t timer_driver_cycle_get(void)
{
	return mtime();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	set_mtimecmp(cycles);
}

#include "system_timer_generic.h"

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_core_announce();
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ((uint32_t)mtime()) << CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return mtime() << CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER;
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(TIMER_IRQN, 0, timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(TIMER_IRQN);
	return 0;
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	timer_core_smp_prime();
	irq_enable(TIMER_IRQN);
}
#endif

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
