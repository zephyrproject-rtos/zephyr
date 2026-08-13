/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32N6 family processors.
 *
 */


#ifndef _STM32N6_SOC_H_
#define _STM32N6_SOC_H_

#ifndef _ASMLANGUAGE

#include <stdbool.h>
#include <stdint.h>

#include <stm32n6xx.h>

#define STM32N6_CLOCK_READY_TIMEOUT_US 100000U

/**
 * Configure the external VCORE supply and select run-mode VOS0.
 *
 * The STM32N6 resets the run voltage selection after STOP. Call this before
 * configuring the high-speed clock tree at boot and after STOP exit.
 */
void stm32n6_configure_run_power(void);

/**
 * Wait for an STM32N6 clock status without using the Zephyr system timer.
 *
 * System PM stops SysTick before entering the SoC power hooks and restarts it
 * after post operations. The DWT counter runs from the CPU clock, which is HSI
 * immediately after STOP, so it remains suitable while restoring the clocks.
 * A poll limit also bounds the wait if the DWT counter is unavailable.
 */
static inline bool stm32n6_wait_for_clock(uint32_t (*read_status)(void), uint32_t expected_status)
{
	const uint32_t timeout_cycles =
		(uint32_t)(((uint64_t)HSI_VALUE * STM32N6_CLOCK_READY_TIMEOUT_US) / 1000000U);
	uint32_t remaining_polls = timeout_cycles;
	uint32_t start_cycles;

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	start_cycles = DWT->CYCCNT;

	do {
		if (read_status() == expected_status) {
			return true;
		}
	} while ((uint32_t)(DWT->CYCCNT - start_cycles) < timeout_cycles && --remaining_polls > 0U);

	return false;
}

#endif /* !_ASMLANGUAGE */

#endif /* _STM32N6_SOC_H_ */
