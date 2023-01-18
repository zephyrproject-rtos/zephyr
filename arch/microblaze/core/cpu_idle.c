/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

static ALWAYS_INLINE void microblaze_idle(unsigned int key)
{
	sys_trace_idle();

	/* wait for interrupt */
#if defined(CONFIG_MICROBLAZE_IDLE_SLEEP)
	{
		__asm__ __volatile__("sleep\t");
	}
#elif defined(CONFIG_MICROBLAZE_IDLE_HIBERNATE)
	{
		__asm__ __volatile__("hibernate\t");
	}
#elif defined(CONFIG_MICROBLAZE_IDLE_SUSPEND)
	{
		__asm__ __volatile__("suspend\t");
	}
#elif defined(CONFIG_MICROBLAZE_IDLE_NOP)
	{
		__asm__ __volatile__("nop\t");
	}
#endif
	/* unlock interrupts */
	irq_unlock(key);
}

void arch_cpu_idle(void)
{
	microblaze_idle(1);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	microblaze_idle(key);
}

/**
 * @brief Defined weak so SoCs/Boards with timers can override.
 * This is an approximate busy wait implementation that executes
 * a number of NOPs to obtain an approximate of the desired delay.
 *
 * @param usec_to_wait
 */
static void ALWAYS_INLINE arch_busy_wait_1ms(void)
{
#define LOOP_LIMIT (427ULL * 200000000ULL / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
	for (uint64_t counter = 0; counter < LOOP_LIMIT; counter++) {
		arch_nop();
	}
}

__weak void arch_busy_wait(uint32_t usec_to_wait)
{
	for (uint32_t msecs = 0; msecs < usec_to_wait / USEC_PER_MSEC; msecs++) {
		arch_busy_wait_1ms();
	}
}
