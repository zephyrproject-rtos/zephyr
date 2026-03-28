/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

/* On all STM32F2x, F4x and F7x, the PLLs share the same source.
 * Ensure that it is the case for those enabled.
 */
#if defined(STM32_PLL_ENABLED) && defined(STM32_PLLI2S_ENABLED)
BUILD_ASSERT(DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_PLLI2S_CLOCKS_CTRL),
	     "PLL and PLLI2S must have the same source");
#endif /* pll && plli2s */

#if defined(STM32_PLL_ENABLED) && defined(STM32_PLLSAI_ENABLED)
BUILD_ASSERT(DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_PLLSAI_CLOCKS_CTRL),
	     "PLL and PLLSAI must have the same source");
#endif /* pll && pllsai */

#if defined(STM32_PLLI2S_ENABLED) && defined(STM32_PLLSAI_ENABLED)
BUILD_ASSERT(DT_SAME_NODE(DT_PLLI2S_CLOCKS_CTRL, DT_PLLSAI_CLOCKS_CTRL),
	     "PLLI2S and PLLSAI must have the same source");
#endif /* plli2s && pllsai */

#if !defined(RCC_PLLI2SCFGR_PLLI2SM)
/* Except for STM32F411 / F412 / F413 / F423 and F446, all PLLs on F2x, F4x and F7x share
 * the same M divisor. If several PLLs are defined, their div-m must have the same value.
 */
#if defined(STM32_PLL_ENABLED) && defined(STM32_PLLI2S_ENABLED)
BUILD_ASSERT(STM32_PLL_M_DIVISOR == STM32_PLLI2S_M_DIVISOR,
	     "PLL M and PLLI2S M should have the same value");
#endif /* STM32_PLL_ENABLED && STM32_PLLI2S_ENABLED */

#if defined(STM32_PLL_ENABLED) && defined(STM32_PLLSAI_ENABLED)
BUILD_ASSERT(STM32_PLL_M_DIVISOR == STM32_PLLSAI_M_DIVISOR,
	     "PLL M and PLLSAI M should have the same value");
#endif /* STM32_PLL_ENABLED && STM32_PLLSAI_ENABLED */

#if defined(STM32_PLLI2S_ENABLED) && defined(STM32_PLLSAI_ENABLED)
BUILD_ASSERT(STM32_PLLI2S_M_DIVISOR == STM32_PLLSAI_M_DIVISOR,
	     "PLLI2S M and PLLSAI M should have the same value");
#endif /* STM32_PLLI2S_ENABLED && STM32_PLLSAI_ENABLED */
#endif /* RCC_PLLI2SCFGR_PLLI2SM */

/* Some SoCs have a secondary divisor for some PLL outputs.
 * When that's the case, ensure that if one is defined, the other also is.
 */
#if defined(STM32_PLL_ENABLED) && defined(RCC_DCKCFGR_PLLDIVR)
BUILD_ASSERT(STM32_PLL_R_ENABLED == STM32_PLL_POST_R_ENABLED,
	     "For the PLL, both div-r and post-div-r must be present if one of them is present");
#endif /* STM32_PLL_ENABLED && RCC_DCKCFGR_PLLDIVR */

#if defined(STM32_PLLI2S_ENABLED) && defined(RCC_DCKCFGR_PLLI2SDIVQ)
BUILD_ASSERT(STM32_PLLI2S_Q_ENABLED == STM32_PLLI2S_POST_Q_ENABLED,
	     "For the PLLI2S, both div-q and post-div-q must be present if one of them is present");
#endif /* STM32_PLLI2S_ENABLED && RCC_DCKCFGR_PLLI2SDIVQ */

#if defined(STM32_PLLI2S_ENABLED) && defined(RCC_DCKCFGR_PLLI2SDIVR)
BUILD_ASSERT(STM32_PLLI2S_R_ENABLED == STM32_PLLI2S_POST_R_ENABLED,
	     "For the PLLI2S, both div-r and post-div-r must be present if one of them is present");
#endif /* STM32_PLLI2S_ENABLED && RCC_DCKCFGR_PLLI2SDIVR */

#if defined(STM32_PLLSAI_ENABLED)
BUILD_ASSERT(STM32_PLLSAI_Q_ENABLED == STM32_PLLSAI_POST_Q_ENABLED,
	     "For the PLLSAI, both div-q and post-div-q must be present if one of them is present");
#endif /* STM32_PLLSAI_ENABLED */

#if defined(STM32_PLLSAI_ENABLED) && defined(RCC_PLLSAICFGR_PLLSAIR)
BUILD_ASSERT(STM32_PLLSAI_R_ENABLED == STM32_PLLSAI_POST_R_ENABLED,
	     "For the PLLSAI, both div-r and post-div-r must be present if one of them is present");
#endif /* STM32_PLLSAI_ENABLED && RCC_PLLSAICFGR_PLLSAIR */

#ifdef STM32_PLL_ENABLED

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
void config_pll_sysclock(void)
{
#if STM32_PLL_P_ENABLED
	/* All STM32F2x, F4x and F7x */
	LL_RCC_PLL_ConfigDomain_SYS(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllp(STM32_PLL_P_DIVISOR));
#endif /* STM32_PLL_P_ENABLED */

#if STM32_PLL_Q_ENABLED
	/* All STM32F2x, F4x and F7x */
	LL_RCC_PLL_ConfigDomain_48M(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllq(STM32_PLL_Q_DIVISOR));
#endif /* STM32_PLLI2S_Q_ENABLED */

#if STM32_PLL_R_ENABLED
#if defined(RCC_DCKCFGR_PLLDIVR)
	/* STM32F413 / F423 */
	LL_RCC_PLL_ConfigDomain_SAI(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllr(STM32_PLL_R_DIVISOR)
				    plldivr(STM32_PLL_POST_R_DIVISOR));
#elif defined(RCC_PLLR_I2S_CLKSOURCE_SUPPORT) /* RCC_DCKCFGR_PLLDIVR */
	/* STM32F410 / F412 / F446 */
	LL_RCC_PLL_ConfigDomain_I2S(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllr(STM32_PLL_R_DIVISOR));
#elif defined(DSI) /* RCC_PLLR_I2S_CLKSOURCE_SUPPORT */
	/* STM32F469 / F479 / F769 / F779 */
	LL_RCC_PLL_ConfigDomain_DSI(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllr(STM32_PLL_R_DIVISOR));
#else /* DSI */
#error "PLL doesn't have R output on this SOC"
#endif /* DSI */
#endif /* STM32_PLL_R_ENABLED */

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

#endif /* STM32_PLL_ENABLED */

#ifdef STM32_PLLI2S_ENABLED

/**
 * @brief Return PLLI2S source
 */
__unused
static uint32_t get_plli2s_source(void)
{
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLLI2S_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLLI2S_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Get the PLLI2S source frequency
 */
uint32_t get_plli2ssrc_frequency(void)
{
	if (IS_ENABLED(STM32_PLLI2S_SRC_HSI)) {
		return STM32_HSI_FREQ;
	} else if (IS_ENABLED(STM32_PLLI2S_SRC_HSE)) {
		return STM32_HSE_FREQ;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up PLL I2S configuration
 */
void config_plli2s(void)
{
#if STM32_PLLI2S_P_ENABLED
#if defined(SPDIFRX)
	/* STM32F446 / F74x and higher */
	LL_RCC_PLLI2S_ConfigDomain_SPDIFRX(get_plli2s_source(),
					   plli2sm(STM32_PLLI2S_M_DIVISOR),
					   STM32_PLLI2S_N_MULTIPLIER,
					   plli2sp(STM32_PLLI2S_P_DIVISOR));
#else /* SPDIFRX */
#error "PLLI2S doesn't have P output on this SOC"
#endif /* SPDIFRX */
#endif /* STM32_PLLI2S_P_ENABLED */

#if STM32_PLLI2S_Q_ENABLED
#if defined(RCC_DCKCFGR_PLLI2SDIVQ)
	/* STM32F427 / F429 / F437 / F439 / F446 / F469 / F479 / F7x */
	LL_RCC_PLLI2S_ConfigDomain_SAI(get_plli2s_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sq(STM32_PLLI2S_Q_DIVISOR),
				       plli2sdivq(STM32_PLLI2S_POST_Q_DIVISOR));
#elif defined(RCC_PLLI2SCFGR_PLLI2SQ) /* RCC_DCKCFGR_PLLI2SDIVQ */
	/* STM32F412 / F413 / F423 */
	LL_RCC_PLLI2S_ConfigDomain_48M(get_plli2s_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sq(STM32_PLLI2S_Q_DIVISOR));
#else /* RCC_PLLI2SCFGR_PLLI2SQ */
#error "PLLI2S doesn't have Q output on this SOC"
#endif /* RCC_PLLI2SCFGR_PLLI2SQ */
#endif /* STM32_PLLI2S_Q_ENABLED */

#if STM32_PLLI2S_R_ENABLED
#if defined(RCC_DCKCFGR_PLLI2SDIVR)
	/* STM32F413 / F423 */
	LL_RCC_PLLI2S_ConfigDomain_SAI(get_plli2s_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sr(STM32_PLLI2S_R_DIVISOR),
				       plli2sdivr(STM32_PLLI2S_POST_R_DIVISOR));
#elif defined(RCC_PLLI2SCFGR_PLLI2SR) /* RCC_DCKCFGR_PLLI2SDIVR */
	/* All STM32F2x, F4x (except F410 / F413 / F423) and F7x */
	LL_RCC_PLLI2S_ConfigDomain_I2S(get_plli2s_source(),
				       plli2sm(STM32_PLLI2S_M_DIVISOR),
				       STM32_PLLI2S_N_MULTIPLIER,
				       plli2sr(STM32_PLLI2S_R_DIVISOR));
#else /* RCC_PLLI2SCFGR_PLLI2SR */
#error "PLLI2S doesn't have R output on this SOC"
#endif /* RCC_PLLI2SCFGR_PLLI2SR */
#endif /* STM32_PLLI2S_R_ENABLED */
}

#endif /* STM32_PLLI2S_ENABLED */

#ifdef STM32_PLLSAI_ENABLED

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
void config_pllsai(void)
{
#if STM32_PLLSAI_P_ENABLED
#if defined(RCC_PLLSAICFGR_PLLSAIP)
	LL_RCC_PLLSAI_ConfigDomain_48M(get_pllsai_source(),
				       pllsaim(STM32_PLLSAI_M_DIVISOR),
				       STM32_PLLSAI_N_MULTIPLIER,
				       pllsaip(STM32_PLLSAI_P_DIVISOR));
#else
#error "PLLSAI doesn't have P output on this SOC"
#endif
#endif /* STM32_PLLSAI_P_ENABLED */

#if STM32_PLLSAI_Q_ENABLED && STM32_PLLSAI_POST_Q_ENABLED
#if defined(RCC_PLLSAICFGR_PLLSAIQ)
	LL_RCC_PLLSAI_ConfigDomain_SAI(get_pllsai_source(),
				       pllsaim(STM32_PLLSAI_M_DIVISOR),
				       STM32_PLLSAI_N_MULTIPLIER,
				       pllsaiq(STM32_PLLSAI_Q_DIVISOR),
				       pllsaidivq(STM32_PLLSAI_POST_Q_DIVISOR));
#else
#error "PLLSAI doesn't have Q output on this SOC"
#endif
#endif /* STM32_PLLSAI_Q_ENABLED && STM32_PLLSAI_POST_Q_ENABLED */

#if STM32_PLLSAI_R_ENABLED && STM32_PLLSAI_POST_R_ENABLED
#if defined(RCC_PLLSAICFGR_PLLSAIR)
	LL_RCC_PLLSAI_ConfigDomain_LTDC(get_pllsai_source(),
					pllsaim(STM32_PLLSAI_M_DIVISOR),
					STM32_PLLSAI_N_MULTIPLIER,
					pllsair(STM32_PLLSAI_R_DIVISOR),
					pllsaidivr(STM32_PLLSAI_POST_R_DIVISOR));
#else
#error "PLLSAI doesn't have R output on this SOC"
#endif
#endif /* STM32_PLLSAI_R_ENABLED && STM32_PLLSAI_POST_R_ENABLED */
}

#endif /* STM32_PLLSAI_ENABLED */

#ifdef STM32_CK48_ENABLED
/**
 * @brief calculate the CK48 frequency depending on its clock source
 */
uint32_t get_ck48_frequency(void)
{
	uint32_t source = LL_RCC_GetCK48MClockSource(LL_RCC_CK48M_CLKSOURCE);

	if (source == LL_RCC_CK48M_CLKSOURCE_PLL) {
		/* Get the PLL48CK source : HSE or HSI */
		source = (LL_RCC_PLL_GetMainSource() == LL_RCC_PLLSOURCE_HSE) ?
			 HSE_VALUE : HSI_VALUE;
		/* Get the PLL48CK Q freq. No HAL macro for that */
		return __LL_RCC_CALC_PLLCLK_48M_FREQ(source,
						     LL_RCC_PLL_GetDivider(),
						     LL_RCC_PLL_GetN(),
						     LL_RCC_PLL_GetQ());
#ifdef LL_RCC_CK48M_CLKSOURCE_PLLI2S
	} else if (source == LL_RCC_CK48M_CLKSOURCE_PLLI2S) {
		/* Get the PLL I2S source : HSE or HSI */
		source = (LL_RCC_PLLI2S_GetMainSource() == LL_RCC_PLLSOURCE_HSE) ?
			 HSE_VALUE : HSI_VALUE;
		/* Get the PLL I2S Q freq. No HAL macro for that */
		return __LL_RCC_CALC_PLLI2S_48M_FREQ(source,
						     LL_RCC_PLLI2S_GetDivider(),
						     LL_RCC_PLLI2S_GetN(),
						     LL_RCC_PLLI2S_GetQ());
#endif /* LL_RCC_CK48M_CLKSOURCE_PLLI2S */
#ifdef LL_RCC_CK48M_CLKSOURCE_PLLSAI
	} else if (source == LL_RCC_CK48M_CLKSOURCE_PLLSAI) {
		/* Get the PLL SAI source : HSE or HSI */
		source = (LL_RCC_PLLSAI_GetMainSource() == LL_RCC_PLLSOURCE_HSE) ?
			 HSE_VALUE : HSI_VALUE;
		/* Get the PLL SAI P freq. No HAL macro for that */
		return __LL_RCC_CALC_PLLSAI_48M_FREQ(source,
						     LL_RCC_PLLSAI_GetDivider(),
						     LL_RCC_PLLSAI_GetN(),
						     LL_RCC_PLLSAI_GetP());
#endif /* LL_RCC_CK48M_CLKSOURCE_PLLSAI */
	} else {
		__ASSERT(0, "Invalid source");
	}

	return 0;
}
#endif

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Power Interface clock enabled by default */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}
