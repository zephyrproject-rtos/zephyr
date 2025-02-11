/*
 * Copyright (c) 2018, I-SENSE group of ICCS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr's implementation for STM32Cube HAL core initialization
 *        functions. These functions are declared as __weak in
 *        STM32Cube HAL in order to be overwritten in case of other
 *        implementations.
 */

#include <zephyr/kernel.h>
#include <soc.h>
/**
 * @brief This function configures the source of stm32cube time base.
 *        Cube HAL expects a 1ms tick which matches with k_uptime_get_32.
 *        Tick interrupt priority is not used
 * @return HAL status
 */
uint32_t HAL_GetTick(void)
{
	return k_uptime_get_32();
}

/**
 * @brief This function provides minimum delay (in milliseconds) based
 *	  on variable incremented.
 * @param Delay: specifies the delay time length, in milliseconds.
 * @return None
 */
void HAL_Delay(__IO uint32_t Delay)
{
	k_msleep(Delay);
}

#ifdef CONFIG_USE_STM32_ASSERT
/**
 * @brief Generates an assert on STM32Cube HAL/LL assert trigger.
 * @param file: specifies the file name where assert expression failed.
 * @param line: specifies the line number where assert expression failed.
 * @return None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* Assert condition have been verified at Cube level, force
	 * generation here.
	 */
	__ASSERT(false, "Invalid value line %d @ %s\n", line, file);
}
#endif /* CONFIG_USE_STM32_ASSERT */
