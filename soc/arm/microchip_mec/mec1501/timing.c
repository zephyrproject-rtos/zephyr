/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/arm/aarch32/arch.h>
#include <kernel.h>
#include <sys_clock.h>
#include <timing/timing.h>
#include <soc.h>

void timing_init(void)
{
	/* Setup counter */
	B32TMR1_REGS->CTRL = MCHP_BTMR_CTRL_ENABLE |
			     MCHP_BTMR_CTRL_AUTO_RESTART |
			     MCHP_BTMR_CTRL_COUNT_UP;

	B32TMR1_REGS->PRLD = 0; /* Preload */
	B32TMR1_REGS->CNT = 0; /* Counter value */

	B32TMR1_REGS->IEN = 0; /* Disable interrupt */
	B32TMR1_REGS->STS = 1; /* Clear interrupt */
}

void timing_start(void)
{
	B32TMR1_REGS->CTRL |= MCHP_BTMR_CTRL_START;
}

void timing_stop(void)
{
	B32TMR1_REGS->CTRL &= ~MCHP_BTMR_CTRL_START;
}

timing_t timing_counter_get(void)
{
	return B32TMR1_REGS->CNT;
}

uint64_t timing_cycles_get(volatile timing_t *const start,
			   volatile timing_t *const end)
{
	return (*end - *start);
}

uint64_t timing_freq_get(void)
{
	return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

uint64_t timing_cycles_to_ns(uint64_t cycles)
{
	return (cycles) * (NSEC_PER_SEC) / (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
}

uint64_t timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return (uint32_t)timing_cycles_to_ns(cycles) / count;
}

uint32_t timing_freq_get_mhz(void)
{
	return (uint32_t)(timing_freq_get() / 1000000);
}
