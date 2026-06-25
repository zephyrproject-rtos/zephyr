/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <cmsis_core.h>
#include <soc.h>

#define DWT_CYCLES_PER_MS (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000U)

static uint32_t dwt_last;
static uint32_t elapsed_ms;

/**
 * @brief This function computes the elapsed cycles between two DWT cycle counter values.
 * @return The number of elapsed cycles between the two DWT cycle counter values.
 */
static uint32_t stm32_xspi_dwt_elapsed_cycles(uint32_t current, uint32_t previous)
{
	if (current >= previous) {
		return current - previous;
	}

	return (UINT32_MAX - previous) + 1U + current;
}

/**
 * @brief This function initializes the DWT cycle counter for time measurement in milliseconds.
 */
static int stm32_xspi_hal_dwt_init(void)
{
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;

	DWT->CYCCNT = 0U;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	dwt_last = DWT->CYCCNT;

	return 0;
}

/**
 * @brief This function provides the current tick value in milliseconds
 * based on the DWT cycle counter.
 * @return The current tick value in milliseconds.
 */
uint32_t HAL_GetTick(void)
{
	uint32_t current_cycles;
	uint32_t elapsed_cycles;
	uint32_t elapsed_whole_ms;

	current_cycles = DWT->CYCCNT;
	elapsed_cycles = stm32_xspi_dwt_elapsed_cycles(current_cycles, dwt_last);
	dwt_last = current_cycles;

	elapsed_whole_ms = elapsed_cycles / DWT_CYCLES_PER_MS;
	elapsed_ms += elapsed_whole_ms;

	return elapsed_ms;
}

/**
 * @brief This function provides minimum delay (in milliseconds) based
 *	  on variable incremented.
 * @param Delay: specifies the delay time length, in milliseconds.
 * @return None
 */
void HAL_Delay(__IO uint32_t Delay)
{
	const uint32_t start = HAL_GetTick();

	while ((HAL_GetTick() - start) < Delay) {
	}
}

SYS_INIT(stm32_xspi_hal_dwt_init, PRE_KERNEL_1, 0);
