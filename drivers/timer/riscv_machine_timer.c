/*
 * Copyright (c) 2024-2026 MASSDRIVER EI (massdriver.space)
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <zephyr/arch/riscv/csr.h>

#define DT_DRV_COMPAT riscv_machine_timer

/* If mtime is absent (T-Head C906), use TIME CSR to fetch mtime */
#if DT_INST_REG_HAS_NAME(0, mtime)
#define MTIME_REG DT_INST_REG_ADDR_BY_NAME(0, mtime)
#endif
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
#if defined(CONFIG_64BIT) && !DT_INST_PROP(0, 32bit_mmio_mtimecmp)
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
#if !defined(MTIME_REG) && defined(CONFIG_64BIT)
	return csr_read(time);
#elif !defined(MTIME_REG)
	uint32_t lo, hi;

	do {
		hi = csr_read(timeh);
		lo = csr_read(time);
	} while (csr_read(timeh) != hi);

	return ((uint64_t)hi << 32U) | lo;
#elif defined(CONFIG_64BIT)
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
 * backend. mtime is 64 bits wide. The generic core owns the tick accounting;
 * this driver only reads the counter and arms the comparator. The public
 * cycle counter returns raw mtime through the core's default getters.
 */
#define TIMER_CORE_BACKEND_COMPARE_ORDERED
#define TIMER_CORE_COUNTER_WIDTH 64

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

	/* Disarm the IRQ. MTIP is level sensitive: it stays asserted while
	 * mtime is past mtimecmp, and writing mtimecmp is the only way to
	 * clear it. Push the compare out of reach so this interrupt fires
	 * once for the deadline it was armed with, whether or not another
	 * one is programmed afterwards.
	 */
	set_mtimecmp(UINT64_MAX);

	timer_core_announce();
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
