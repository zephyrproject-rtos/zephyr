/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32v00x_rcc

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/dt-bindings/clock/ch32v00x-clocks.h>

#include <hal_ch32fun.h>

#include "clock_control_wch_rcc_common.h"

struct clock_control_wch_config {
	RCC_TypeDef *rcc_regs;
	struct wch_clk_config clocks_data[CH32_CLKID_COUNT];
};

static int clock_control_wch_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_wch_config *config = dev->config;

	return clock_control_wch_common_clock_on(config->rcc_regs, sys);
}

static int clock_control_wch_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_wch_config *config = dev->config;

	return clock_control_wch_common_clock_off(config->rcc_regs, sys);
}

static int clock_control_wch_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
					    uint32_t *rate)
{
	const struct clock_control_wch_config *config = dev->config;
	uint32_t sysclk = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	uint8_t id = (uintptr_t)sys;

	sysclk /= config->clocks_data[CH32_CLKID_CLK_HB].clk_div;

	if (id == CH32V00X_CLOCK_ADC1) {
		sysclk /= config->clocks_data[CH32_CLKID_CLK_ADC].clk_div;
	}
	if (id == CH32V00X_CLOCK_WWDG) {
		/* static prescaler on the WWDG clock */
		sysclk /= 4096;
	}

	*rate = sysclk;
	return 0;
}

static DEVICE_API(clock_control, clock_control_wch_api) = {
	.on = clock_control_wch_clock_on,
	.off = clock_control_wch_clock_off,
	.get_rate = clock_control_wch_clock_get_rate,
};

#define WCH_RCC_SYSCLK DT_PROP(DT_NODELABEL(cpu0), clock_frequency)
static void clock_control_wch_clock_setup_flash(void)
{
#if defined(FLASH_ACTLR_LATENCY)
	uint32_t latency;

#if defined(CONFIG_SOC_CH32V003)
	if (WCH_RCC_SYSCLK <= 24000000) {
		latency = FLASH_ACTLR_LATENCY_0;
	} else {
		latency = FLASH_ACTLR_LATENCY_1;
	}
#elif defined(CONFIG_SOC_SERIES_CH32V00X)
	if (WCH_RCC_SYSCLK <= 15000000) {
		latency = FLASH_ACTLR_LATENCY_0;
	} else if (WCH_RCC_SYSCLK <= 24000000) {
		latency = FLASH_ACTLR_LATENCY_1;
	} else {
		latency = FLASH_ACTLR_LATENCY_2;
	}
#else
#error Unrecognised SOC family
#endif
	FLASH->ACTLR = (FLASH->ACTLR & ~FLASH_ACTLR_LATENCY) | latency;
#endif
}

static void wch_set_hb_prescaler(const struct clock_control_wch_config *config)
{
	uint16_t clk_div = config->clocks_data[CH32_CLKID_CLK_HB].clk_div;
	uint32_t hpre;

	if (clk_div <= 8) {
		/* divisor smaller or equal to 8 is div - 1 */
		hpre = clk_div - 1;
	} else {
		/* divisor bigger 8 has the 4. bit set to one and log2 */
		hpre = find_msb_set(clk_div - 2) | 0b1000;
	}

	config->rcc_regs->CFGR0 =
		(config->rcc_regs->CFGR0 & ~RCC_HPRE) | (hpre << (find_lsb_set(RCC_HPRE) - 1));
}

static void wch_set_adc_prescaler(const struct clock_control_wch_config *config)
{
	uint16_t clk_div = config->clocks_data[CH32_CLKID_CLK_ADC].clk_div;
	uint32_t adcpre = 0;

	/* TODO: find better calculation than this lookup table */
	switch (clk_div) {
	case 2:
		adcpre = RCC_ADCPRE_DIV2;
		break;
	case 4:
		adcpre = RCC_ADCPRE_DIV4;
		break;
	case 6:
		adcpre = RCC_ADCPRE_DIV6;
		break;
	case 8:
		adcpre = RCC_ADCPRE_DIV8;
		break;
	case 12:
		adcpre = RCC_ADCPRE_DIV12;
		break;
	case 16:
		adcpre = RCC_ADCPRE_DIV16;
		break;
	case 24:
		adcpre = RCC_ADCPRE_DIV24;
		break;
	case 32:
		adcpre = RCC_ADCPRE_DIV32;
		break;
	case 48:
		adcpre = RCC_ADCPRE_DIV48;
		break;
	case 64:
		adcpre = RCC_ADCPRE_DIV64;
		break;
	case 96:
		adcpre = RCC_ADCPRE_DIV96;
		break;
	case 128:
		adcpre = RCC_ADCPRE_DIV128;
		break;
	}

	config->rcc_regs->CFGR0 = (config->rcc_regs->CFGR0 & ~RCC_ADCPRE) | adcpre;
}

static int clock_control_wch_init(const struct device *dev)
{
	const struct clock_control_wch_config *config = dev->config;
	RCC_TypeDef *rcc_regs = config->rcc_regs;

	clock_control_wch_clock_setup_flash();

	/* Disable the PLL before potentially changing the input clocks. */
	rcc_regs->CTLR &= ~RCC_PLLON;

	if (CH32_CLK_OKAY(clk_lsi) != 0) {
		CH32_CLKBIT_ENABLE(rcc_regs->RSTSCKR, LSI);
	} else {
		rcc_regs->RSTSCKR &= ~RCC_LSION;
	}

	if (CH32_CLK_OKAY(clk_hsi) != 0) {
		CH32_CLKBIT_ENABLE(rcc_regs->CTLR, HSI);
	}
	/* don't disable HSI yet, we don't wanna kill ourself */

	if (CH32_CLK_OKAY(clk_hse)) {
		CH32_CLKBIT_ENABLE(rcc_regs->CTLR, HSE);
	}

	if (CH32_CLK_OKAY(pll)) {
		/* select pll src */
		rcc_regs->CFGR0 = (rcc_regs->CFGR0 & ~RCC_PLLSRC) |
				  config->clocks_data[CH32_CLKID_CLK_PLL].source;
		CH32_CLKBIT_ENABLE(rcc_regs->CTLR, PLL);
	}

	/* select clk_sys source */
	rcc_regs->CFGR0 =
		(rcc_regs->CFGR0 & ~RCC_SW) | config->clocks_data[CH32_CLKID_CLK_SYS].source;

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_WCH_RCC_CSS)) {
		rcc_regs->CTLR |= RCC_CSSON;
	}

	/* Clear the interrupt flags. */
	rcc_regs->INTR = RCC_CSSC | RCC_PLLRDYC | RCC_HSERDYC | RCC_LSIRDYC;

	/* set prescaler */
	wch_set_hb_prescaler(config);
	wch_set_adc_prescaler(config);

	return 0;
}

static const struct clock_control_wch_config clock_control_wch_config = {
	.rcc_regs = (RCC_TypeDef *)DT_INST_REG_ADDR_BY_NAME(0, rcc),
	.clocks_data = {
		[CH32_CLKID_CLK_SYS] = {
#if CH32_CLK_SRC_IS(clk_sys, clk_hse)
			.source = RCC_SW_HSE,
#elif CH32_CLK_SRC_IS(clk_sys, clk_hsi)
			.source = RCC_SW_HSI
#elif CH32_CLK_SRC_IS(clk_sys, pll)
			.source = RCC_SW_PLL,
#else
#error "Invalid clk_sys source"
#endif
		},
		[CH32_CLKID_CLK_PLL] = {
#if CH32_CLK_SRC_IS(pll, clk_hsi)
		        .source = RCC_PLLSRC_HSI_Mul2,
#elif CH32_CLK_SRC_IS(pll, clk_hse)
			.source = RCC_PLLSRC_HSE_Mul2,
#else
#error "Invalid pll source"
#endif
	        },
		[CH32_CLKID_CLK_HB] = {
			.clk_div = DT_PROP(CH32_GET_CLK(clk_hb), div),
	        },
		[CH32_CLKID_CLK_ADC] = {
			.clk_div = DT_PROP(CH32_GET_CLK(clk_adc), div),
		},
        },
};

DEVICE_DT_INST_DEFINE(0, clock_control_wch_init, NULL, NULL, &clock_control_wch_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_wch_api);
