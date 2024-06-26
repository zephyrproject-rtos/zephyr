/*
 * Copyright (c) 2020 Intel Corporation.
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/arch.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/timing/timing.h>
#include <soc.h>

/*
 * This code is conditionally built please refer to the SoC cmake file and
 * is not built normally. If this is not built then timer5 is available
 * for other uses.
 */
#define BTMR_XEC_REG_BASE						\
	((struct btmr_regs *)(DT_REG_ADDR(DT_NODELABEL(timer5))))

void soc_timing_init(void)
{
	struct btmr_regs *regs = BTMR_XEC_REG_BASE;

	/* Setup counter */
	regs->CTRL = MCHP_BTMR_CTRL_ENABLE | MCHP_BTMR_CTRL_AUTO_RESTART |
		     MCHP_BTMR_CTRL_COUNT_UP;

	regs->PRLD = 0; /* Preload */
	regs->CNT = 0; /* Counter value */

	regs->IEN = 0; /* Disable interrupt */
	regs->STS = 1; /* Clear interrupt */
}

void soc_timing_start(void)
{
	regs->CTRL |= MCHP_BTMR_CTRL_START;
}

void soc_timing_stop(void)
{
	regs->CTRL &= ~MCHP_BTMR_CTRL_START;
}

timing_t soc_timing_counter_get(void)
{
	return regs->CNT;
}

uint64_t soc_timing_cycles_get(volatile timing_t *const start,
			       volatile timing_t *const end)
{
	return *end - *start;
}

uint64_t soc_timing_freq_get(void)
{
	return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

uint64_t soc_timing_cycles_to_ns(uint64_t cycles)
{
	return cycles * NSEC_PER_SEC / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

uint64_t soc_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return (uint32_t)soc_timing_cycles_to_ns(cycles) / count;
}

uint32_t soc_timing_freq_get_mhz(void)
{
	return (uint32_t)(soc_timing_freq_get() / 1000000);
}
