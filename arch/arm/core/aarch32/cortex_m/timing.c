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

#include <init.h>
#include <timing/timing.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

/* Define DWT LSR masks which are currently not defined by the CMSIS V5.1.2.
 * (LSR register is defined but not its bitfields).
 * Reuse ITM LSR mask as it is the same offset than DWT LSR one.
 */
#if !defined DWT_LSR_Present_Msk
#define DWT_LSR_Present_Msk ITM_LSR_Present_Msk
#endif
#if !defined DWT_LSR_Access_Msk
#define DWT_LSR_Access_Msk ITM_LSR_Access_Msk
#endif

static void dwt_access(bool ena)
{
#if defined(CONFIG_CPU_CORTEX_M7)
	/*
	 * In case of Cortex M7, we need to check the optional presence of
	 * Lock Access Register (LAR) which is indicated in Lock Status
	 * Register (LSR). When present, a special access token must be written
	 * to unlock DWT registers.
	 */
	uint32_t lsr = DWT->LSR;

	if ((lsr & DWT_LSR_Present_Msk) != 0) {
		if (ena) {
			if ((lsr & DWT_LSR_Access_Msk) != 0) {
				/* Access is locked. unlock it */
				DWT->LAR = 0xC5ACCE55;
			}
		} else {
			if ((lsr & DWT_LSR_Access_Msk) == 0) {
				/* Acess is unlocked. Lock it */
				DWT->LAR = 0;
			}
		}
	}
#else /* CONFIG_CPU_CORTEX_M7 */
	ARG_UNUSED(ena);
#endif /* CONFIG_CPU_CORTEX_M7 */
}

/**
 * @brief Initialize and Enable the DWT cycle counter
 *
 * This routine enables the cycle counter and initializes its value to zero.
 *
 * @return 0
 */
static inline int z_arm_dwt_init(const struct device *arg)
{
	/* Enable tracing */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	/* Unlock DWT access if any */
	dwt_access(true);

	/* Clear and enable the cycle counter */
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	/* Assert that the cycle counter is indeed implemented. */
	__ASSERT(DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk != 0,
		"DWT implements no cycle counter. "
		"Cannot be used for cycle counting\n");

	return 0;
}

/**
 * @brief Return the current value of the cycle counter
 *
 * This routine returns the current value of the DWT Cycle Counter (DWT.CYCCNT)
 *
 * @return the cycle counter value
 */
static inline uint32_t z_arm_dwt_get_cycles(void)
{
	return DWT->CYCCNT;
}

/**
 * @brief Reset and start the DWT cycle counter
 *
 * This routine starts the cycle counter and resets its value to zero.
 *
 * @return 0
 */
static inline void z_arm_dwt_cycle_count_start(void)
{
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

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
#if defined(CONFIG_SOC_FAMILY_NRF)
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
	return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#else
	static uint64_t dwt_frequency;

	if (!dwt_frequency) {

		z_arm_dwt_init(NULL);

		uint32_t cyc_start = k_cycle_get_32();
		uint64_t dwt_start = z_arm_dwt_get_cycles();

		k_busy_wait(10 * USEC_PER_MSEC);

		uint32_t cyc_end = k_cycle_get_32();
		uint64_t dwt_end = z_arm_dwt_get_cycles();

		uint64_t cyc_freq = sys_clock_hw_cycles_per_sec();

		/*
		 * cycles are in 32-bit, and delta must be
		 * calculated in 32-bit percision. Or it would
		 * wrapping around in 64-bit.
		 */
		uint64_t dcyc = (uint32_t)cyc_end - (uint32_t)cyc_start;

		uint64_t dtsc = dwt_end - dwt_start;

		dwt_frequency = (cyc_freq * dtsc) / dcyc;

	}
	return dwt_frequency;
#endif /* CONFIG_SOC_FAMILY_NRF */
}

void arch_timing_init(void)
{
	z_arm_dwt_init(NULL);
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
	return (uint32_t)(arch_timing_freq_get() / 1000000);
}
