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
#ifdef CONFIG_STM32_HAL2
void assert_dbg_state_failed(uint8_t *file, uint32_t line)
{
	/*
	 * New assertion hook exclusive to STM32Cube HAL2.
	 * Called when state validation checks fail. For example, calling
	 * a function that modifies a peripheral's configuration while the
	 * peripheral is active would trigger this assertion.
	 */
	__ASSERT(false, "HAL assertion failed (illegal state) @ %s:%d\n", file, line);
}

void assert_dbg_param_failed(uint8_t *file, uint32_t line)
{
	/*
	 * The assertion hook called when parameter validation checks fail
	 * still exists in STM32Cube HAL2, but under a different name.
	 */
	__ASSERT(false, "HAL assertion failed (invalid parameter) @ %s:%d\n", file, line);
}
#else /* CONFIG_STM32_HAL2 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* Called when parameter validation checks fail */
	__ASSERT(false, "HAL assertion failed (invalid parameter) @ %s:%d\n", file, line);
}
#endif /* CONFIG_STM32_HAL2 */
#endif /* CONFIG_USE_STM32_ASSERT */
