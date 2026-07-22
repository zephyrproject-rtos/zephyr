/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 * Copyright (c) 2018-2023 Intel Corporation
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/sbi.h>

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = IRQ_S_TIMER;
#endif

static void sbi_set_timer(uint64_t deadline)
{
	register unsigned long a0 __asm__("a0") = (unsigned long)deadline;
#ifndef CONFIG_64BIT
	register unsigned long a1 __asm__("a1") = (unsigned long)(deadline >> 32);
#endif
	register unsigned long a6 __asm__("a6") = SBI_FUNC_SET_TIMER;
	register unsigned long a7 __asm__("a7") = SBI_EXT_TIME;

#ifdef CONFIG_64BIT
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "a1", "memory");
#else
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a6), "r"(a7) : "memory");
#endif
}

static uint64_t stime(void)
{
#ifdef CONFIG_64BIT
	return csr_read(time);
#else
	/* guard against lower half rollover */
	uint32_t hi, lo;

	do {
		hi = csr_read(timeh);
		lo = csr_read(time);
	} while (csr_read(timeh) != hi);
	return ((uint64_t)hi << 32) | lo;
#endif
}

/*
 * Free-running "time" counter plus an absolute SBI set-timer deadline: a
 * COMPARE backend. The generic core owns the tick accounting and the clock
 * lock; the driver only reads the counter and programs the deadline.
 */
#define TIMER_CORE_BACKEND_COMPARE
#define TIMER_CORE_64BIT_CYCLES

static inline uint64_t timer_driver_cycle_get(void)
{
	return stime();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	sbi_set_timer(cycles);
}

#include "system_timer_generic.h"

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(IRQ_S_TIMER, 0, timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(IRQ_S_TIMER);
	return 0;
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	timer_core_smp_prime();
	irq_enable(IRQ_S_TIMER);
}
#endif

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
