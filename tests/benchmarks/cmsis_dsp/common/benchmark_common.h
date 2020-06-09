/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_BENCHMARK_CMSIS_DSP_COMMON_BENCHMARK_COMMON_H_
#define ZEPHYR_BENCHMARK_CMSIS_DSP_COMMON_BENCHMARK_COMMON_H_

#include <ztest.h>
#include <zephyr.h>

#if defined(CONFIG_CPU_CORTEX_M_HAS_DWT)
/* Use cycle counting on the Cortex-M devices that support DWT */

#include <arch/arm/aarch32/cortex_m/cmsis.h>

static ALWAYS_INLINE void benchmark_begin(uint32_t *irq_key, uint32_t *timestamp)
{
	ARG_UNUSED(timestamp);

	/* Lock interrupts to prevent preemption */
	*irq_key = irq_lock();

	/* Start DWT cycle counter */
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static ALWAYS_INLINE uint32_t benchmark_end(uint32_t irq_key, uint32_t timestamp)
{
	/* Stop DWT cycle counter */
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;

	/* Unlock interrupts */
	irq_unlock(irq_key);

	/* Return DWT cycle counter value */
	return DWT->CYCCNT;
}

#define BENCHMARK_TYPE		"Processor Cycles"

#else
/* Use system timer clock on other systems */

static ALWAYS_INLINE void benchmark_begin(uint32_t *irq_key, uint32_t *timestamp)
{
	volatile uint32_t now;

	/* Lock interrupts to prevent preemption */
	*irq_key = irq_lock();

	/* Read timestamp for the beginning of benchmark */
	now = k_cycle_get_32();

	/* Store timestamp */
	*timestamp = now;
}

static ALWAYS_INLINE uint32_t benchmark_end(uint32_t irq_key, uint32_t timestamp)
{
	volatile uint32_t now;

	/* Read timestamp for the end of benchmark */
	now = k_cycle_get_32();

	/* Unlock interrupts */
	irq_unlock(irq_key);

	/* Return timespan between the beginning and the end of benchmark */
	return now - timestamp;
}

#define BENCHMARK_TYPE		"System Timer Cycles"

#endif

#endif /* ZEPHYR_BENCHMARK_CMSIS_DSP_COMMON_BENCHMARK_COMMON_H_ */
