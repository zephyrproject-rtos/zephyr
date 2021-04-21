/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"


#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL

/**
 * @brief fill in pll configuration structure
 */
void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit)
{
	/*
	 * PLL MUL
	 * 2  -> LL_RCC_PLL_MUL_2  -> 0x00000000
	 * 3  -> LL_RCC_PLL_MUL_3  -> 0x00040000
	 * 4  -> LL_RCC_PLL_MUL_4  -> 0x00080000
	 * ...
	 * 16 -> LL_RCC_PLL_MUL_16 -> 0x00380000
	 */
	pllinit->PLLMul = ((CONFIG_CLOCK_STM32_PLL_MULTIPLIER - 2)
					<< RCC_CFGR_PLLMUL_Pos);

	/*
	 * PLL PREDIV
	 * 1  -> LL_RCC_PREDIV_DIV_1  -> 0x00000000
	 * 2  -> LL_RCC_PREDIV_DIV_2  -> 0x00000001
	 * 3  -> LL_RCC_PREDIV_DIV_3  -> 0x00000002
	 * ...
	 * 16 -> LL_RCC_PREDIV_DIV_16 -> 0x0000000F
	 */
#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
	/*
	 * PREDIV1 support is a specific RCC configuration present on
	 * following SoCs: STM32F04xx, STM32F07xx, STM32F09xx,
	 * STM32F030xC, STM32F302xE, STM32F303xE and STM32F39xx
	 * cf Reference manual for more details
	 */
	pllinit->PLLDiv = CONFIG_CLOCK_STM32_PLL_PREDIV1 - 1;
#else
	pllinit->Prediv = CONFIG_CLOCK_STM32_PLL_PREDIV - 1;
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
}

#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
#ifndef CONFIG_SOC_SERIES_STM32F3X
#if defined(CONFIG_EXTI_STM32) || defined(CONFIG_USB_DC_STM32)
	/* Enable System Configuration Controller clock. */
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
#endif
#else
#if defined(CONFIG_USB_DC_STM32) && defined(SYSCFG_CFGR1_USB_IT_RMP)
	/* Enable System Configuration Controller clock. */
	/* SYSCFG is required to remap IRQ to avoid conflicts with CAN */
	/* cf §14.1.3, RM0316 */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#endif
#endif /* !CONFIG_SOC_SERIES_STM32F3X */
}

/**
 * @brief Function kept for driver genericity
 */
void LL_RCC_MSI_Disable(void)
{
	/* Do nothing */
}
