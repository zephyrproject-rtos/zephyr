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
#include <zephyr/sys/time_units.h>
#include "clock_stm32_ll_common.h"

#if defined(STM32_PLL_ENABLED) || defined(STM32_PLLSAI1_ENABLED) || defined(STM32_PLLSAI2_ENABLED)

#if defined(LL_RCC_MSIRANGESEL_RUN)
#define CALC_RUN_MSI_FREQ(range) __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGESEL_RUN, \
							range << RCC_CR_MSIRANGE_Pos);
#else
#define CALC_RUN_MSI_FREQ(range) __LL_RCC_CALC_MSI_FREQ(range << RCC_CR_MSIRANGE_Pos);
#endif

#endif

#if defined(STM32_PLL_ENABLED)
/**
 * @brief Return PLL source
 */
__unused
static uint32_t get_pll_source(void)
{
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	} else if (IS_ENABLED(STM32_PLL_SRC_MSI)) {
		return LL_RCC_PLLSOURCE_MSI;
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
#if defined(STM32_MSI_ENABLED)
	} else if (IS_ENABLED(STM32_PLL_SRC_MSI)) {
		return CALC_RUN_MSI_FREQ(STM32_MSI_RANGE);
#endif
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up pll configuration
 */
void config_pll_sysclock(void)
{
#ifdef PWR_CR5_R1MODE
	/* set power boost mode for sys clock greater than 80MHz */
	if (sys_clock_hw_cycles_per_sec() >= MHZ(80)) {
		LL_PWR_EnableRange1BoostMode();
	}
#endif /* PWR_CR5_R1MODE */

	LL_RCC_PLL_ConfigDomain_SYS(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllr(STM32_PLL_R_DIVISOR));

	LL_RCC_PLL_EnableDomain_SYS();
}

#endif /* defined(STM32_PLL_ENABLED) */

#if defined(STM32_PLLSAI1_ENABLED)

/**
 * @brief Return PLLSAI1 source
 */
__unused
static uint32_t get_pllsai1_source(void)
{
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLLSAI1_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLLSAI1_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	} else if (IS_ENABLED(STM32_PLLSAI1_SRC_MSI)) {
		return LL_RCC_PLLSOURCE_MSI;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Get the PLLSAI1 source frequency
 */
__unused
uint32_t get_pllsai1src_frequency(void)
{
	if (IS_ENABLED(STM32_PLLSAI1_SRC_HSI)) {
		return STM32_HSI_FREQ;
	} else if (IS_ENABLED(STM32_PLLSAI1_SRC_HSE)) {
		return STM32_HSE_FREQ;
#if defined(STM32_MSI_ENABLED)
	} else if (IS_ENABLED(STM32_PLLSAI1_SRC_MSI)) {
		return CALC_RUN_MSI_FREQ(STM32_MSI_RANGE);
#endif
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up PLLSAI1 configuration
 */
__unused
void config_pllsai1(void)
{
#ifndef RCC_PLLSAI1M_DIV_1_16_SUPPORT
	/*
	 * On some L4 series, there is no dedicated M_DIVISOR for PLLSAIs
	 * and it is shared with PLL and other PLLSAIs. Ensure that if they
	 * exist, they have the same value
	 */
#if defined(STM32_PLL_M_DIVISOR) && (STM32_PLL_M_DIVISOR != STM32_PLLSAI1_M_DIVISOR)
#error "PLLSAI1 M divisor must have same value as PLL M divisor"
#elif defined(STM32_PLLSAI2_M_DIVISOR) && (STM32_PLLSAI2_M_DIVISOR != STM32_PLLSAI1_M_DIVISOR)
#error "PLLSAI1 M divisor must have same value as PLLSAI2 M divisor"
#endif
#endif
#if STM32_PLLSAI1_P_ENABLED
	LL_RCC_PLLSAI1_ConfigDomain_SAI(get_pllsai1_source(),
					pllsai1m(STM32_PLLSAI1_M_DIVISOR),
					STM32_PLLSAI1_N_MULTIPLIER,
					pllsai1p(STM32_PLLSAI1_P_DIVISOR));

	LL_RCC_PLLSAI1_EnableDomain_SAI();
#endif /* STM32_PLLSAI1_P_ENABLED */

#if STM32_PLLSAI1_Q_ENABLED
	LL_RCC_PLLSAI1_ConfigDomain_48M(get_pllsai1_source(),
					pllsai1m(STM32_PLLSAI1_M_DIVISOR),
					STM32_PLLSAI1_N_MULTIPLIER,
					pllsai1q(STM32_PLLSAI1_Q_DIVISOR));

	LL_RCC_PLLSAI1_EnableDomain_48M();
#endif /* STM32_PLLSAI1_Q_ENABLED */

#if STM32_PLLSAI1_R_ENABLED
	LL_RCC_PLLSAI1_ConfigDomain_ADC(get_pllsai1_source(),
					pllsai1m(STM32_PLLSAI1_M_DIVISOR),
					STM32_PLLSAI1_N_MULTIPLIER,
					pllsai1r(STM32_PLLSAI1_R_DIVISOR));

	LL_RCC_PLLSAI1_EnableDomain_ADC();
#endif /* STM32_PLLSAI1_R_ENABLED */
}

#endif /* STM32_PLLSAI1_ENABLED */

#if defined(STM32_PLLSAI2_ENABLED)
#if defined(RCC_PLLSAI2_SUPPORT)

/**
 * @brief Return PLLSAI2 source
 */
__unused
static uint32_t get_pllsai2_source(void)
{
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLLSAI2_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLLSAI2_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	} else if (IS_ENABLED(STM32_PLLSAI2_SRC_MSI)) {
		return LL_RCC_PLLSOURCE_MSI;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Get the PLLSAI2 source frequency
 */
__unused
uint32_t get_pllsai2src_frequency(void)
{
	if (IS_ENABLED(STM32_PLLSAI2_SRC_HSI)) {
		return STM32_HSI_FREQ;
	} else if (IS_ENABLED(STM32_PLLSAI2_SRC_HSE)) {
		return STM32_HSE_FREQ;
#if defined(STM32_MSI_ENABLED)
	} else if (IS_ENABLED(STM32_PLLSAI2_SRC_MSI)) {
		return CALC_RUN_MSI_FREQ(STM32_MSI_RANGE);
#endif
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up PLLSAI2 configuration
 */
__unused
void config_pllsai2(void)
{
#ifndef RCC_PLLSAI2M_DIV_1_16_SUPPORT
	/*
	 * On some L4 series, there is no dedicated M_DIVISOR for PLLSAIs
	 * and it is shared with PLL and other PLLSAIs. Ensure that if they
	 * exist, they have the same value
	 */
#if defined(STM32_PLL_M_DIVISOR) && (STM32_PLL_M_DIVISOR != STM32_PLLSAI2_M_DIVISOR)
#error "PLLSAI2 M divisor must have same value as PLL M divisor"
#elif defined(STM32_PLLSAI1_M_DIVISOR) && (STM32_PLLSAI1_M_DIVISOR != STM32_PLLSAI2_M_DIVISOR)
#error "PLLSAI2 M divisor must have same value as PLLSAI1 M divisor"
#endif
#endif
#if STM32_PLLSAI2_P_ENABLED
	LL_RCC_PLLSAI2_ConfigDomain_SAI(get_pllsai2_source(),
					pllsai2m(STM32_PLLSAI2_M_DIVISOR),
					STM32_PLLSAI2_N_MULTIPLIER,
					pllsai2p(STM32_PLLSAI2_P_DIVISOR));

	LL_RCC_PLLSAI2_EnableDomain_SAI();
#endif /* STM32_PLLSAI2_P_ENABLED */

#if STM32_PLLSAI2_Q_ENABLED && defined(RCC_PLLSAI2Q_DIV_SUPPORT)
	LL_RCC_PLLSAI2_ConfigDomain_DSI(get_pllsai2_source(),
					pllsai2m(STM32_PLLSAI2_M_DIVISOR),
					STM32_PLLSAI2_N_MULTIPLIER,
					pllsai2q(STM32_PLLSAI2_Q_DIVISOR));

	LL_RCC_PLLSAI2_EnableDomain_DSI();
#endif /* STM32_PLLSAI2_Q_ENABLED */

#if STM32_PLLSAI2_R_ENABLED
#if defined(RCC_CCIPR2_PLLSAI2DIVR)
#if STM32_PLLSAI2_DIVR_ENABLED
	LL_RCC_PLLSAI2_ConfigDomain_LTDC(get_pllsai2_source(),
					 pllsai2m(STM32_PLLSAI2_M_DIVISOR),
					 STM32_PLLSAI2_N_MULTIPLIER,
					 pllsai2r(STM32_PLLSAI2_R_DIVISOR),
					 pllsai2divr(STM32_PLLSAI2_DIVR_DIVISOR));

	LL_RCC_PLLSAI2_EnableDomain_LTDC();
#else
#error "On PLLSAI2, div_divr device-tree properties is needed"
#endif
#else
	LL_RCC_PLLSAI2_ConfigDomain_ADC(get_pllsai2_source(),
					pllsai2m(STM32_PLLSAI2_M_DIVISOR),
					STM32_PLLSAI2_N_MULTIPLIER,
					pllsai2r(STM32_PLLSAI2_R_DIVISOR));

	LL_RCC_PLLSAI2_EnableDomain_ADC();
#endif
#endif /* STM32_PLLSAI2_R_ENABLED */
}
#else
#error "PLLSAI2 is not available in this platform"
#endif

#endif /* STM32_PLLSAI2_ENABLED */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
#ifdef LL_APB1_GRP1_PERIPH_PWR
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif
#if defined(CONFIG_SOC_SERIES_STM32WBX)
	/* HW semaphore Clock enable */
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_HSEM);
#endif
}
