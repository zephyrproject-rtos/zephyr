/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M Timing functions interface based on DWT
 *
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include <aarch32/cortex_m/dwt.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/sys_clock.h>

/**
 * @brief Return the current frequency of the cycle counter
 *
 * This routine returns the current frequency of the DWT Cycle Counter
 * in DWT cycles per second (Hz).
 *
 * @return the cycle counter frequency value
 */
static inline uint64_t z_arm_dwt_freq_get(void)
{
#if defined(CONFIG_SOC_FAMILY_NRF) || \
	defined(CONFIG_SOC_SERIES_IMX_RT6XX)
	/*
	 * DWT frequency is taken directly from the
	 * System Core clock (CPU) frequency, if the
	 * CMSIS SystemCoreClock symbols is available.
	 */
	SystemCoreClockUpdate();

	return SystemCoreClock;
#elif defined(CONFIG_CORTEX_M_SYSTICK)
	/* SysTick and DWT both run at CPU frequency,
	 * reflected in the system timer HW cycles/sec.
	 */
	return sys_clock_hw_cycles_per_sec();
#else
	static uint64_t dwt_frequency;
	uint32_t cyc_start, cyc_end;
	uint64_t dwt_start, dwt_end;
	uint64_t cyc_freq = sys_clock_hw_cycles_per_sec();
	uint64_t dcyc, ddwt;

	if (!dwt_frequency) {

		z_arm_dwt_init();

		do {
			cyc_start = k_cycle_get_32();
			dwt_start = z_arm_dwt_get_cycles();

			k_busy_wait(10 * USEC_PER_MSEC);

			cyc_end = k_cycle_get_32();
			dwt_end = z_arm_dwt_get_cycles();

			/*
			 * cycles are in 32-bit, and delta must be
			 * calculated in 32-bit precision. Or it would be
			 * wrapping around in 64-bit.
			 */
			dcyc = (uint32_t)cyc_end - (uint32_t)cyc_start;

			ddwt = dwt_end - dwt_start;
		} while ((dcyc == 0) || (ddwt == 0));

		dwt_frequency = (cyc_freq * ddwt) / dcyc;

	}
	return dwt_frequency;
#endif /* CONFIG_SOC_FAMILY_NRF */
}

void arch_timing_init(void)
{
	z_arm_dwt_init();
	z_arm_dwt_init_cycle_counter();
}

void arch_timing_start(void)
{
	z_arm_dwt_cycle_count_start();
}

void arch_timing_stop(void)
{
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

timing_t arch_timing_counter_get(void)
{
	return (timing_t)z_arm_dwt_get_cycles();
}

uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end)
{
	return (*end - *start);
}

uint64_t arch_timing_freq_get(void)
{
	return z_arm_dwt_freq_get();
}

uint64_t arch_timing_cycles_to_ns(uint64_t cycles)
{
	return (cycles) * (NSEC_PER_USEC) / arch_timing_freq_get_mhz();
}

uint64_t arch_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return arch_timing_cycles_to_ns(cycles) / count;
}

uint32_t arch_timing_freq_get_mhz(void)
{
	return (uint32_t)(arch_timing_freq_get() / 1000000U);
}
