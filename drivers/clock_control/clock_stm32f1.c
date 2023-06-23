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
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

#if defined(RCC_CFGR_USBPRE)
#define STM32_USB_PRE_ENABLED	RCC_CFGR_USBPRE
#elif defined(RCC_CFGR_OTGFSPRE)
#define STM32_USB_PRE_ENABLED	RCC_CFGR_OTGFSPRE
#endif

#define z_adc_prescaler(v) LL_RCC_ADC_CLKSRC_PCLK2_DIV_ ## v
#define adc_prescaler(v) z_adc_prescaler(v)

#if defined(STM32_PLL_ENABLED)

/*
 * Select PLL source for STM32F1 Connectivity line devices (STM32F105xx and
 * STM32F107xx).
 * Both flags are defined in STM32Cube LL API. Keep only the selected one.
 */

/**
 * @brief Set up pll configuration
 */
__unused
void config_pll_sysclock(void)
{
	uint32_t pll_source, pll_mul, pll_div;

	/*
	 * PLLMUL on SOC_STM32F10X_DENSITY_DEVICE
	 * 2  -> LL_RCC_PLL_MUL_2  -> 0x00000000
	 * 3  -> LL_RCC_PLL_MUL_3  -> 0x00040000
	 * 4  -> LL_RCC_PLL_MUL_4  -> 0x00080000
	 * ...
	 * 16 -> LL_RCC_PLL_MUL_16 -> 0x00380000
	 *
	 * PLLMUL on SOC_STM32F10X_CONNECTIVITY_LINE_DEVICE
	 * 4  -> LL_RCC_PLL_MUL_4   -> 0x00080000
	 * ...
	 * 9  -> LL_RCC_PLL_MUL_9   -> 0x001C0000
	 * 13 -> LL_RCC_PLL_MUL_6_5 -> 0x00340000
	 */
	pll_mul = ((STM32_PLL_MULTIPLIER - 2) << RCC_CFGR_PLLMULL_Pos);

	if (!IS_ENABLED(STM32_PLL_SRC_HSI)) {
		/* In case PLL source is not HSI, set prediv case by case */
#ifdef CONFIG_SOC_STM32F10X_DENSITY_DEVICE
		/* PLL prediv */
		if (IS_ENABLED(STM32_PLL_XTPRE)) {
			/*
			 * SOC_STM32F10X_DENSITY_DEVICE:
			 * PLLXPTRE (depends on PLL source HSE)
			 * HSE/2 used as PLL source
			 */
			pll_div = LL_RCC_PREDIV_DIV_2;
		} else {
			/*
			 * SOC_STM32F10X_DENSITY_DEVICE:
			 * PLLXPTRE (depends on PLL source HSE)
			 * HSE used as direct PLL source
			 */
			pll_div = LL_RCC_PREDIV_DIV_1;
		}
#else
		/*
		 * SOC_STM32F10X_CONNECTIVITY_LINE_DEVICE
		 * 1  -> LL_RCC_PREDIV_DIV_1  -> 0x00000000
		 * 2  -> LL_RCC_PREDIV_DIV_2  -> 0x00000001
		 * 3  -> LL_RCC_PREDIV_DIV_3  -> 0x00000002
		 * ...
		 * 16 -> LL_RCC_PREDIV_DIV_16 -> 0x0000000F
		 */
		pll_div = STM32_PLL_PREDIV - 1;
#endif /* CONFIG_SOC_STM32F10X_DENSITY_DEVICE */
	}

	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_source = LL_RCC_PLLSOURCE_HSI_DIV_2;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_source = LL_RCC_PLLSOURCE_HSE | pll_div;
#if defined(RCC_CFGR2_PREDIV1SRC)
	} else if (IS_ENABLED(STM32_PLL_SRC_PLL2)) {
		pll_source = LL_RCC_PLLSOURCE_PLL2 | pll_div;
#endif
	} else {
		__ASSERT(0, "Invalid source");
	}

	LL_RCC_PLL_ConfigDomain_SYS(pll_source, pll_mul);

#ifdef STM32_USB_PRE_ENABLED
	/* Prescaler is enabled: PLL clock is not divided */
	LL_RCC_SetUSBClockSource(IS_ENABLED(STM32_PLL_USBPRE) ?
				 STM32_USB_PRE_ENABLED : 0);
#endif
}

#endif /* defined(STM32_PLL_ENABLED) */

#if defined(STM32_PLL2_ENABLED)

/**
 * @brief Set up pll2 configuration
 */
__unused
void config_pll2(void)
{
	uint32_t pll_mul, pll_div;

	/*
	 * PLL2MUL on SOC_STM32F10X_CONNECTIVITY_LINE_DEVICE
	 * 8  -> LL_RCC_PLL2_MUL_8  -> 0x00000600
	 * 9  -> LL_RCC_PLL2_MUL_9  -> 0x00000700
	 * ...
	 * 14 -> LL_RCC_PLL2_MUL_14 -> 0x00000C00
	 * 16 -> LL_RCC_PLL2_MUL_16 -> 0x00000E00
	 * 20 -> LL_RCC_PLL2_MUL_20 -> 0x00000F00
	 */
	if (STM32_PLL2_MULTIPLIER == 20) {
		pll_mul = RCC_CFGR2_PLL2MUL20;
	} else {
		pll_mul = ((STM32_PLL2_MULTIPLIER - 2) << RCC_CFGR2_PLL2MUL_Pos);
	}

	/*
	 * SOC_STM32F10X_CONNECTIVITY_LINE_DEVICE
	 * 1  -> LL_RCC_HSE_PREDIV2_DIV_1  -> 0x00000000
	 * 2  -> LL_RCC_HSE_PREDIV2_DIV_2  -> 0x00000010
	 * ...
	 * 16 -> LL_RCC_HSE_PREDIV2_DIV_16 -> 0x000000F0
	 */
	pll_div = ((STM32_PLL2_PREDIV - 1) << RCC_CFGR2_PREDIV2_Pos);

	/* Check PLL2 source */
	if (!IS_ENABLED(STM32_PLL2_SRC_HSE)) {
		__ASSERT(0, "Invalid source");
	}

	LL_RCC_PLL_ConfigDomain_PLL2(pll_div, pll_mul);
}

#endif /* defined(STM32_PLL2_ENABLED) */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		/* Set the PWREN and BKPEN bits in the RCC_APB1ENR register */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_BKP);
	}
}
