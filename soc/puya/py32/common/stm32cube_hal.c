/*
 * Copyright (c) 2018, I-SENSE group of ICCS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr time base for the Puya PY32 HAL.
 */

#include <zephyr/kernel.h>
#include <soc.h>

/**
 * @brief Return elapsed milliseconds for the PY32 HAL.
 * @return Milliseconds since boot, truncated to 32 bits.
 */
uint32_t HAL_GetTick(void)
{
	return k_uptime_get_32();
}

/**
 * @brief Sleep for the delay requested by the PY32 HAL.
 * @param delay Delay in milliseconds.
 */
void HAL_Delay(uint32_t delay)
{
	k_msleep(delay);
}

#ifdef CONFIG_USE_STM32_ASSERT
/**
 * @brief Report a PY32 HAL assertion failure.
 * @param file Source file containing the assertion.
 * @param line Source line containing the assertion.
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	__ASSERT(false, "HAL assertion failed @ %s:%u", file, line);
}
#endif /* CONFIG_USE_STM32_ASSERT */
