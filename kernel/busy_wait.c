/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <kernel_arch_interface.h>

#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
/* Convert microseconds to cycles using a caller-supplied frequency sample.
 *
 * When the system timer frequency can change at runtime, conversions must use
 * the same stable frequency sample that was paired with the cycle counter read.
 */
static inline uint32_t busy_wait_us_to_cyc_ceil32(uint32_t usec, uint32_t hz)
{
	/* microseconds per second */
	const uint64_t denom = 1000000ULL;
	uint64_t num = (uint64_t)usec * (uint64_t)hz;

	/* Round up to avoid returning early. */
	return (uint32_t)((num + denom - 1U) / denom);
}
#endif /* CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE */

void z_impl_k_busy_wait(uint32_t usec_to_wait)
{
	SYS_PORT_TRACING_FUNC_ENTER(k_thread, busy_wait, usec_to_wait);
	if (usec_to_wait == 0U) {
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, busy_wait, usec_to_wait);
		return;
	}

#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
	arch_busy_wait(usec_to_wait);
#elif defined(CONFIG_SYS_CLOCK_EXISTS)
#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
	/*
	 * When the system timer frequency can change at runtime, the frequency
	 * used for k_cycle_get_32() conversions may change during the busy wait.
	 * Sample frequency and cycle count consistently, and adjust if the
	 * frequency changes.
	 */
	uint32_t hz;
	uint32_t start_cycles;

	/* Get a consistent sample of the current frequency and cycle count. */
	for (;;) {
		uint32_t hz1 = sys_clock_hw_cycles_per_sec();
		uint32_t current_cycles = k_cycle_get_32();
		uint32_t hz2 = sys_clock_hw_cycles_per_sec();

		if (hz1 != hz2) {
			/* Frequency changed while sampling; retry. */
			continue;
		}

		hz = hz1;
		start_cycles = current_cycles;

		break;
	}

	uint32_t cycles_to_wait = busy_wait_us_to_cyc_ceil32(usec_to_wait, hz);

	/* Now busy wait, adjusting if the frequency changes. */
	for (;;) {
		uint32_t hz1 = sys_clock_hw_cycles_per_sec();
		uint32_t current_cycles = k_cycle_get_32();
		uint32_t hz2 = sys_clock_hw_cycles_per_sec();

		if (hz1 != hz2) {
			/* Frequency changed while sampling; retry. */
			continue;
		}

		if (hz1 != hz) {
			/* Keep our reference point in the same scale as the cycle counter. */
			start_cycles = (uint32_t)(((uint64_t)start_cycles * (uint64_t)hz1) /
						  (uint64_t)hz);
			hz = hz1;
			cycles_to_wait = busy_wait_us_to_cyc_ceil32(usec_to_wait, hz);
		}

		/* This handles the rollover on an unsigned 32-bit value. */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
#else
	uint32_t start_cycles = k_cycle_get_32();
	uint32_t cycles_to_wait = k_us_to_cyc_ceil32(usec_to_wait);

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* This handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
#endif /* CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE */
#else
	/*
	 * Crude busy loop for the purpose of being able to configure out
	 * system timer support.
	 */
	unsigned int loops_per_usec = CONFIG_BUSYWAIT_CPU_LOOPS_PER_USEC;
	unsigned int loops = loops_per_usec * usec_to_wait;

	while (loops-- > 0) {
		arch_nop();
	}
#endif

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, busy_wait, usec_to_wait);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_busy_wait(uint32_t usec_to_wait)
{
	z_impl_k_busy_wait(usec_to_wait);
}
#include <zephyr/syscalls/k_busy_wait_mrsh.c>
#endif /* CONFIG_USERSPACE */
