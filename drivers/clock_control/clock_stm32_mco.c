/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2024, Joakim Andersson
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <stm32_ll_rcc.h>

#define DT_DRV_COMPAT st_stm32_clock_mco

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mco1), okay)
#if DT_PROP(DT_NODELABEL(mco1), source) == NO_SEL
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_NOCLOCK
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_EXT_HSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_EXT_HSE
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_LSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_LSE
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_HSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSE
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_LSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_LSI
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_MSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_MSI
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_MSIK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_MSIK
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_MSIS
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_MSIS
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_HSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_HSI16
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_HSI48
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI48
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_PLLCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLCLK
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_PLLQCLK
	#if (CONFIG_SOC_SERIES_STM32G0X || CONFIG_SOC_SERIES_STM32WLX)
		#define MCO1_SOURCE	LL_RCC_MCO1SOURCE_PLLQCLK
	#elif (CONFIG_SOC_SERIES_STM32H5X || \
		 CONFIG_SOC_SERIES_STM32H7X || CONFIG_SOC_SERIES_STM32H7RSX)
		#define MCO1_SOURCE	LL_RCC_MCO1SOURCE_PLL1QCLK
	#else
		#error "PLLQCLK is not a valid clock source on your SOC"
	#endif
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_PLLCLK_DIV2
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLCLK_DIV_2
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_PLL2CLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLL2CLK
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_PLLI2SCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLI2SCLK
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_PLLI2SCLK_DIV2
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLI2SCLK_DIV2
#elif DT_PROP(DT_NODELABEL(mco1), source) == STM32_SRC_SYSCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_SYSCLK
#else
#error "MCO1 source not a valid clock source"
#endif
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(mco1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mco2), okay)
#if DT_PROP(DT_NODELABEL(mco2), source) == NO_SEL
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_SYSCLK
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_PLLI2S
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLLI2S
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_HSE
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_HSE
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_LSI
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_LSI
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_CSI
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_CSI
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_PLLCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLLCLK
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_PLLPCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLL1PCLK
#elif DT_PROP(DT_NODELABEL(mco2), source) == STM32_SRC_PLL2PCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLL2PCLK
#else
#error "MCO2 source not a valid clock source"
#endif
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(mco2), okay) */

#define fn_mco_prescaler(i, v) LL_RCC_MCO ## i ## _DIV_ ## v
#define mco_prescaler(i, v) fn_mco_prescaler(i, v)
#define MCO_INST_PRESCALER(inst) mco_prescaler(DT_INST_REG_ADDR(inst), DT_INST_PROP(inst, prescaler))

#define fn_mco_source(i) MCO ## i ## _SOURCE
#define mco_source(i) fn_mco_source(i)
#define MCO_INST_SOURCE(inst) mco_source(DT_INST_REG_ADDR(inst))

struct stm32_mco_config {
	const struct pinctrl_dev_config *pcfg;
};

struct stm32_mco_data {
	uint32_t source;
	uint32_t prescaler;
};

static int stm32_mco_init(const struct device *dev)
{
	const struct stm32_mco_config *config = dev->config;
	struct stm32_mco_data *data = dev->data;

	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

#ifdef CONFIG_SOC_SERIES_STM32F1X
	LL_RCC_ConfigMCO(data->source);
#else
	LL_RCC_ConfigMCO(data->source, data->prescaler);
#endif

	return 0;
}

#define STM32_MCO_INIT(inst)                                            \
									\
PINCTRL_DT_INST_DEFINE(inst);                                           \
									\
const static struct stm32_mco_config stm32_mco_config_##inst = {        \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                   \
};                                                                      \
static struct stm32_mco_data stm32_mco_data_##inst = {                  \
	.source = MCO_INST_SOURCE(inst),                                \
	.prescaler = MCO_INST_PRESCALER(inst),                          \
};                                                                      \
									\
DEVICE_DT_INST_DEFINE(inst, stm32_mco_init, NULL,                       \
	&stm32_mco_data_##inst,                                         \
	&stm32_mco_config_##inst,                                       \
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,               \
	NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_MCO_INIT);
