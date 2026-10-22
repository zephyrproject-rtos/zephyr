/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include <zephyr/arch/riscv/csr.h>
#include <esp_attr.h>
#include <esp_private/esp_clk.h>

/* Time with the mcycle CSR, which counts at the CPU clock, instead of
 * the 16MHz systimer the generic RISC-V backend would use. The older
 * cores in this family count cycles in a custom CSR and do not
 * implement mcycle, so the capability is selected per SoC.
 *
 * A span across a CPU frequency change is not meaningful, which these
 * SoCs never do at runtime. Light sleep tracks the kernel clock.
 */

void soc_timing_init(void)
{
}

void soc_timing_start(void)
{
}

void soc_timing_stop(void)
{
}

IRAM_ATTR timing_t soc_timing_counter_get(void)
{
	return (timing_t)csr_read(mcycle);
}

IRAM_ATTR uint64_t soc_timing_cycles_get(volatile timing_t *const start,
					 volatile timing_t *const end)
{
	/* mcycle is 32 bits wide here and mcycleh does not track it,
	 * so correct a single wrap.
	 */
	if (*end >= *start) {
		return (uint64_t)(*end - *start);
	}

	return (BIT64(32) + (uint64_t)*end) - (uint64_t)*start;
}

uint64_t soc_timing_freq_get(void)
{
	return (uint64_t)esp_clk_cpu_freq();
}

uint64_t soc_timing_cycles_to_ns(uint64_t cycles)
{
	return (cycles * NSEC_PER_SEC) / soc_timing_freq_get();
}

uint64_t soc_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return soc_timing_cycles_to_ns(cycles) / count;
}

uint32_t soc_timing_freq_get_mhz(void)
{
	return (uint32_t)(soc_timing_freq_get() / 1000000);
}
