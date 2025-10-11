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

#if defined(STM32_CK48_ENABLED)
/**
 * @brief calculate the CK48 frequency depending on its clock source
 */
__unused
uint32_t get_ck48_frequency(void)
{
	uint32_t source;

	if (LL_RCC_GetCK48MClockSource(LL_RCC_CK48M_CLKSOURCE) ==
			LL_RCC_CK48M_CLKSOURCE_PLL) {
		/* Get the PLL48CK source : HSE or HSI */
		source = (LL_RCC_PLL_GetMainSource() == LL_RCC_PLLSOURCE_HSE)
			? HSE_VALUE
			: HSI_VALUE;
		/* Get the PLL48CK Q freq. No HAL macro for that */
		return __LL_RCC_CALC_PLLCLK_48M_FREQ(source,
						LL_RCC_PLL_GetDivider(),
						LL_RCC_PLL_GetN(),
						LL_RCC_PLL_GetQ()
						);
	} else if (LL_RCC_GetCK48MClockSource(LL_RCC_CK48M_CLKSOURCE) ==
			LL_RCC_CK48M_CLKSOURCE_PLLI2S) {
		/* Get the PLL I2S source : HSE or HSI */
		source = (LL_RCC_PLLI2S_GetMainSource() == LL_RCC_PLLSOURCE_HSE)
			? HSE_VALUE
			: HSI_VALUE;
		/* Get the PLL I2S Q freq. No HAL macro for that */
		return __LL_RCC_CALC_PLLI2S_48M_FREQ(source,
						LL_RCC_PLLI2S_GetDivider(),
						LL_RCC_PLLI2S_GetN(),
						LL_RCC_PLLI2S_GetQ()
						);
	}

	__ASSERT(0, "Invalid source");
	return 0;
}
#endif

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

#if STM32_PLL_Q_ENABLED
	/* There is a Q divider on the PLL to configure the PLL48CK */
	LL_RCC_PLL_ConfigDomain_48M(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllq(STM32_PLL_Q_DIVISOR));
#endif /* STM32_PLLI2S_Q_ENABLED */

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
	LL_RCC_PLLI2S_ConfigDomain_I2S(get_pll_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sr(STM32_PLLI2S_R_DIVISOR));

#if STM32_PLLI2S_Q_ENABLED && \
	(defined(RCC_PLLI2SCFGR_PLLI2SQ) && !defined(RCC_DCKCFGR_PLLI2SDIVQ))
	/* There is a Q divider on the PLLI2S to configure the PLL48CK */
	LL_RCC_PLLI2S_ConfigDomain_48M(get_pll_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sq(STM32_PLLI2S_Q_DIVISOR));
#endif /* STM32_PLLI2S_Q_ENABLED */
}

#endif /* STM32_PLLI2S_ENABLED */

#if defined(STM32_PLLSAI_ENABLED)

/**
 * @brief Return PLLSAI source
 */
__unused
static uint32_t get_pllsai_source(void)
{
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLLSAI_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLLSAI_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Get the PLLSAI source frequency
 */
__unused
uint32_t get_pllsaisrc_frequency(void)
{
	if (IS_ENABLED(STM32_PLLSAI_SRC_HSI)) {
		return STM32_HSI_FREQ;
	} else if (IS_ENABLED(STM32_PLLSAI_SRC_HSE)) {
		return STM32_HSE_FREQ;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up PLLSAI configuration
 */
__unused
void config_pllsai(void)
{
	/*
	 * In case there is no dedicated M_DIVISOR for PLLSAI, the input is shared
	 * with PLL and PLLI2S. Ensure that if they exist, they have the same value
	 */
#if !defined(RCC_PLLSAICFGR_PLLSAIM)
#if defined(STM32_PLL_M_DIVISOR) && (STM32_PLL_M_DIVISOR != STM32_PLLSAI_M_DIVISOR)
#error "PLLSAI M divisor must have same value as PLL M divisor"
#endif
#endif

#if STM32_PLLSAI_P_ENABLED
#if defined(RCC_PLLSAICFGR_PLLSAIP)
	LL_RCC_PLLSAI_ConfigDomain_48M(get_pllsai_source(),
				       pllsaim(STM32_PLLSAI_M_DIVISOR),
				       STM32_PLLSAI_N_MULTIPLIER,
				       pllsaip(STM32_PLLSAI_P_DIVISOR));
#else
#error "PLLSAI do not have P output on this SOC"
#endif
#endif /* STM32_PLLSAI_P_ENABLED */

#if STM32_PLLSAI_Q_ENABLED && STM32_PLLSAI_DIVQ_ENABLED
#if defined(RCC_PLLSAICFGR_PLLSAIQ)
	LL_RCC_PLLSAI_ConfigDomain_SAI(get_pllsai_source(),
				       pllsaim(STM32_PLLSAI_M_DIVISOR),
				       STM32_PLLSAI_N_MULTIPLIER,
				       pllsaiq(STM32_PLLSAI_Q_DIVISOR),
				       pllsaidivq(STM32_PLLSAI_DIVQ_DIVISOR));
#else
#error "PLLSAI do not have Q output on this SOC"
#endif
#endif /* STM32_PLLSAI_Q_ENABLED && STM32_PLLSAI_DIVQ_ENABLED */

#if STM32_PLLSAI_R_ENABLED && STM32_PLLSAI_DIVR_ENABLED
#if defined(RCC_PLLSAICFGR_PLLSAIR)
	LL_RCC_PLLSAI_ConfigDomain_LTDC(get_pllsai_source(),
					pllsaim(STM32_PLLSAI_M_DIVISOR),
					STM32_PLLSAI_N_MULTIPLIER,
					pllsair(STM32_PLLSAI_R_DIVISOR),
					pllsaidivr(STM32_PLLSAI_DIVR_DIVISOR));
#else
#error "PLLSAI do not have R output on this SOC"
#endif
#endif /* STM32_PLLSAI_R_ENABLED && STM32_PLLSAI_DIVR_ENABLED */
}

#endif /* STM32_PLLSAI_ENABLED */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Power Interface clock enabled by default */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}
