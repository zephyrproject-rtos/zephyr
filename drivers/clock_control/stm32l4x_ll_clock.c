/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <clock_control/stm32_clock_control.h>
#include "stm32_ll_clock.h"


#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL

/**
 * @brief map PLLM setting to register value
 */
static uint32_t pllm(int prescaler)
{
	const struct regval_map map[] = {
		{1, LL_RCC_PLLM_DIV_1},
		{2, LL_RCC_PLLM_DIV_2},
		{3, LL_RCC_PLLM_DIV_3},
		{4, LL_RCC_PLLM_DIV_4},
		{5, LL_RCC_PLLM_DIV_5},
		{6, LL_RCC_PLLM_DIV_6},
		{7, LL_RCC_PLLM_DIV_7},
		{8, LL_RCC_PLLM_DIV_8},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

/**
 * @brief map PLLR setting to register value
 */
static uint32_t pllr(int prescaler)
{
	const struct regval_map map[] = {
		{2, LL_RCC_PLLR_DIV_2},
		{4, LL_RCC_PLLR_DIV_4},
		{6, LL_RCC_PLLR_DIV_6},
		{8, LL_RCC_PLLR_DIV_8},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

/**
 * @brief fill in pll configuration structure
 */
void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit)
{
	pllinit->PLLM = pllm(CONFIG_CLOCK_STM32_PLL_M_DIVISOR);
	pllinit->PLLN = CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER;
	pllinit->PLLR = pllr(CONFIG_CLOCK_STM32_PLL_R_DIVISOR);
}
#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL */
