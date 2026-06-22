/*
 *
 * Copyright (c) 2019 Ilya Tagunov
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_crs.h>
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
	/* Configure PLL source */
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
	LL_RCC_PLL_ConfigDomain_SYS(get_pll_source(),
				    pllm(STM32_PLL_M_DIVISOR),
				    STM32_PLL_N_MULTIPLIER,
				    pllr(STM32_PLL_R_DIVISOR));

	LL_RCC_PLL_EnableDomain_SYS();
}

#endif /* defined(STM32_PLL_ENABLED) */

#if defined(STM32_CK48_ENABLED)
/**
 * @brief calculate the CK48 frequency depending on its clock source
 */
__unused
uint32_t get_ck48_frequency(void)
{
	switch (LL_RCC_GetRNGClockSource(LL_RCC_RNG_CLKSOURCE)) {
	case LL_RCC_RNG_CLKSOURCE_PLLQ:
		/* Get the PLL48CK source : HSE or HSI */
		uint32_t pll_source = (LL_RCC_PLL_GetMainSource() == LL_RCC_PLLSOURCE_HSE)
					      ? HSE_VALUE
					      : HSI_VALUE;

		/* Get the PLL48CK Q freq. No HAL macro for that */
		return __LL_RCC_CALC_PLLCLK_Q_FREQ(pll_source, LL_RCC_PLL_GetM(), LL_RCC_PLL_GetN(),
						   LL_RCC_PLL_GetQ());
	case LL_RCC_RNG_CLKSOURCE_MSI:
		return __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGESEL_RUN, LL_RCC_MSI_GetRange());
#if defined(USB_DRD_FS)
	case LL_RCC_RNG_CLKSOURCE_HSI48:
		return MHZ(48);
#endif
	case LL_RCC_RNG_CLKSOURCE_NONE:
		/* Clock source not configured */
		return 0;
	default:
		__ASSERT(0, "Invalid source");
		break;
	}

	return 0;
}
#endif

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

#if defined(CRS)
	if (IS_ENABLED(STM32_HSI48_CRS_USB_SOF)) {
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_CRS);
		/*
		 * After reset the CRS configuration register
		 * (CRS_CFGR) value corresponds to an USB SOF
		 * synchronization.  FIXME: write it anyway.
		 */
		LL_CRS_EnableAutoTrimming();
		LL_CRS_EnableFreqErrorCounter();
	}
#endif /* defined(CRS) */

}
