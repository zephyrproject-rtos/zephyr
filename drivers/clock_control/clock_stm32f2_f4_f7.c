/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

#if defined(STM32_PLL_ENABLED)

/**
 * @brief Return PLL source
 */
__unused
static uint32_t get_pll_source(void)
{
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief get the pll source frequency
 */
__unused
uint32_t get_pllsrc_frequency(void)
{
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		return STM32_HSI_FREQ;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		return STM32_HSE_FREQ;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up pll configuration
 */
__unused
void config_pll_sysclock(void)
{
#if defined(STM32_SRC_PLL_R) && STM32_PLL_R_ENABLED && defined(RCC_PLLCFGR_PLLR)
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLR, pllr(STM32_PLL_R_DIVISOR));
#endif
	LL_RCC_PLL_ConfigDomain_SYS(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllp(STM32_PLL_P_DIVISOR));

#if defined(CONFIG_SOC_SERIES_STM32F7X)
	/* Assuming we stay on Power Scale default value: Power Scale 1 */
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 180000000) {
		/* Enable the PLL (PLLON) before setting overdrive. Skipping the PLL
		 * locking phase since the system will be stalled during the switch
		 * (ODSW) but the PLL clock system will be running during the locking
		 * phase. See reference manual (RM0431) §4.1.4 Voltage regulator
		 * Sub section: Entering Over-drive mode.
		 */
		LL_RCC_PLL_Enable();

		/* Set Overdrive if needed before configuring the Flash Latency */
		LL_PWR_EnableOverDriveMode();
		while (LL_PWR_IsActiveFlag_OD() != 1) {
			/* Wait for OverDrive mode ready */
		}
		LL_PWR_EnableOverDriveSwitching();
		while (LL_PWR_IsActiveFlag_ODSW() != 1) {
			/* Wait for OverDrive switch ready */
		}

		/* The PLL could still not be locked when returning to the caller
		 * function. But the caller doesn't know we've turned on the PLL
		 * for the overdrive function. The caller will try to turn on the PLL
		 * And start waiting for the PLL locking phase to complete.
		 */
	}
#endif /* CONFIG_SOC_SERIES_STM32F7X */
}

#endif /* defined(STM32_PLL_ENABLED) */

#ifdef STM32_PLLI2S_ENABLED

/**
 * @brief Set up PLL I2S configuration
 */
__unused
void config_plli2s(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_plli2s_clock)
	LL_RCC_PLLI2S_ConfigDomain_I2S(get_pll_source(),
				       pllm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sr(STM32_PLLI2S_R_DIVISOR));
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32f412_plli2s_clock)
	LL_RCC_PLL_ConfigDomain_I2S(get_pll_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sr(STM32_PLLI2S_R_DIVISOR));
#endif
}

#endif /* STM32_PLLI2S_ENABLED */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Power Interface clock enabled by default */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}
