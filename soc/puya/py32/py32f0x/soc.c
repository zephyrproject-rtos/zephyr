/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <py32f0xx_ll_rcc.h>
#include <py32f0xx_ll_system.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hsi)),
	     "PY32F0 requires the HSI clock to be enabled");
BUILD_ASSERT(DT_PROP(DT_NODELABEL(clk_hsi), clock_frequency) == 24000000U,
	     "PY32F0 supports only the 24 MHz HSI clock");
BUILD_ASSERT(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(rcc)), DT_NODELABEL(clk_hsi)),
	     "PY32F0 requires HSI as the system clock source");

void soc_early_init_hook(void)
{
	/* Raise flash latency before increasing the HSI frequency. */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
	while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {
	}
	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_24MHz);
}
