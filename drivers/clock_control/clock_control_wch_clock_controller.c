/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_clock_controller

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/arch/common/ffs.h>

#if defined(CONFIG_SOC_CH32V003) || defined(CONFIG_SOC_CH32V006)
#include <zephyr/dt-bindings/clock/wch_ch32v00x_clock.h>
#else
#error "unknown wch soc config defined"
#endif

#include <hal_ch32fun.h>

#define CLK_SRC_IS(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

static inline uint16_t wch_hb_clk_prescaler_get(const struct device *dev);
static inline uint16_t wch_adc_clk_prescaler_get(const struct device *dev);

struct wch_clk_config {
	uint32_t source;
	uint16_t div;
	uint16_t mul;
};

struct clock_control_wch_config {
	RCC_TypeDef *rcc_regs;
	struct wch_clk_config clocks_data[WCH_CLOCK_COUNT];
};

static int clock_control_wch_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_wch_config *config = dev->config;
	uint8_t id = (uintptr_t)sys;
	uint32_t reg = (uint32_t)(&config->rcc_regs->AHBPCENR + WCH_CLOCK_CONFIG_BUS(id));
	uint32_t val = sys_read32(reg);

	val |= BIT(WCH_CLOCK_CONFIG_BIT(id));
	sys_write32(val, reg);

	return 0;
}

static int clock_control_wch_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_wch_config *config = dev->config;
	uint8_t id = (uintptr_t)sys;
	uint32_t reg = (uint32_t)(&config->rcc_regs->AHBPCENR + WCH_CLOCK_CONFIG_BUS(id));
	uint32_t val = sys_read32(reg);

	val &= ~BIT(WCH_CLOCK_CONFIG_BIT(id));
	sys_write32(val, reg);

	return 0;
}

static int clock_control_wch_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
					    uint32_t *rate)
{
	uint32_t sysclk = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	uint32_t clk_config = (uint32_t)sys;
	uint32_t clk_bit = WCH_CLOCK_CONFIG_BIT(clk_config);

	sysclk /= wch_hb_clk_prescaler_get(dev);

	/* adc prescaler if adc clock */
	if (WCH_CLOCK_CONFIG_BUS(clk_config) == WCH_CLOCK_CONFIG_BUS_PB2 &&
	    (clk_bit == 9 || clk_bit == 10)) {
		sysclk /= wch_adc_clk_prescaler_get(dev);
	}

	*rate = sysclk;
	return 0;
}

static DEVICE_API(clock_control, clock_control_wch_api) = {
	.on = clock_control_wch_clock_on,
	.off = clock_control_wch_clock_off,
	.get_rate = clock_control_wch_clock_get_rate,
};

#if defined(CONFIG_SOC_CH32V003) || defined(CONFIG_SOC_CH32V006)
static inline void wch_hb_clk_prescaler_set(const struct device *dev)
{
	const struct clock_control_wch_config *config = dev->config;
	uint16_t div = config->clocks_data[WCH_CLKID_CLK_HB].div;
	uint32_t ret;

	if (div <= 8) {
		ret = div - 1;
	} else {
		ret = find_msb_set(div - 2) | 0b1000;
	}

	config->rcc_regs->CFGR0 = (config->rcc_regs->CFGR0 & ~RCC_HPRE) | (ret << 4);
}

static inline uint16_t wch_hb_clk_prescaler_get(const struct device *dev)
{
	const struct clock_control_wch_config *config = dev->config;
	uint32_t cfgr0 = config->rcc_regs->CFGR0;

	if ((cfgr0 & RCC_HPRE_3) != 0) {
		/* The range 0b1000 divides by a power of 2, where 0b1000 is /2, 0b1001 is /4, etc.
		 */
		return 2 << ((cfgr0 & (RCC_HPRE_0 | RCC_HPRE_1 | RCC_HPRE_2)) >> 4);
	} else {
		/* The range 0b0nnn divides by n + 1, where 0b0000 is /1, 0b001 is /2, etc. */
		return ((cfgr0 & (RCC_HPRE_0 | RCC_HPRE_1 | RCC_HPRE_2)) >> 4) + 1;
	}
}
#endif

static inline void wch_adc_clk_prescaler_set(const struct device *dev)
{
	const struct clock_control_wch_config *config = dev->config;

	switch (config->clocks_data[WCH_CLKID_CLK_ADC].div) {
	case 2:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV2;
		return;
	case 4:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV4;
		return;
	case 6:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV6;
		return;
	case 8:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV8;
		return;
	case 12:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV12;
		return;
	case 16:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV16;
		return;
	case 24:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV24;
		return;
	case 32:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV32;
		return;
	case 48:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV48;
		return;
	case 64:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV64;
		return;
	case 96:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV96;
		return;
	case 128:
		config->rcc_regs->CFGR0 |= RCC_ADCPRE_DIV128;
		return;
	default:
		/* TODO: error reporting */
		return;
	}
}

static inline uint16_t wch_adc_clk_prescaler_get(const struct device *dev)
{
	/* trust the config as the calculation is very confusing */
	const struct clock_control_wch_config *config = dev->config;

	return config->clocks_data[WCH_CLKID_CLK_ADC].div;
}

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

static int clock_control_wch_init(const struct device *dev)
{
	const struct clock_control_wch_config *config = dev->config;
	RCC_TypeDef *rcc_regs = config->rcc_regs;

	clock_control_wch_clock_setup_flash();

	/* Disable the PLL before potentially changing the input clocks. */
	rcc_regs->CTLR &= ~RCC_PLLON;

	if (DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_lsi))) {
		rcc_regs->RSTSCKR |= RCC_LSION;
		while ((rcc_regs->RSTSCKR & RCC_LSIRDY) == 0) {
		}
	} else {
		rcc_regs->RSTSCKR &= ~RCC_LSION;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_hsi))) {
		rcc_regs->CTLR |= RCC_HSION;
		while ((rcc_regs->CTLR & RCC_HSIRDY) == 0) {
		}
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_hse))) {
		rcc_regs->CTLR |= RCC_HSEON;
		while ((rcc_regs->CTLR & RCC_HSERDY) == 0) {
		}
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll))) {
		/* select pll SRC */
		rcc_regs->CFGR0 = (rcc_regs->CFGR0 & ~RCC_PLLSRC) |
				  config->clocks_data[WCH_CLKID_CLK_PLL].source;
		rcc_regs->CTLR |= RCC_PLLON;
		while ((rcc_regs->CTLR & RCC_PLLRDY) == 0) {
		}
	}

	/* Select systick source */
	rcc_regs->CFGR0 =
		(rcc_regs->CFGR0 & ~RCC_SW) | config->clocks_data[WCH_CLKID_CLK_SYS].source;
	/* TODO: check if we want to enable CSSON or not */
	rcc_regs->CTLR |= RCC_CSSON;

	/* Clear the interrupt flags. */
	rcc_regs->INTR = RCC_CSSC | RCC_PLLRDYC | RCC_HSERDYC | RCC_LSIRDYC;

	/* set hbprescaler */
	wch_hb_clk_prescaler_set(dev);
	wch_adc_clk_prescaler_set(dev);

	/* TODO: disable HSI if not enabled (reset value is 1) */
	return 0;
}

static const struct clock_control_wch_config clock_control_wch_config = {
	.rcc_regs = (RCC_TypeDef *)DT_INST_REG_ADDR_BY_NAME(0, rcc),
	.clocks_data =
		{
			[WCH_CLKID_CLK_SYS] =
				{
#if CLK_SRC_IS(clk_sys, clk_hse)
					.source = RCC_SW_HSE,
#elif CLK_SRC_IS(clk_sys, pll)
					.source = RCC_SW_PLL,
#else
					.source = RCC_SW_HSI,
#endif
					.mul = 0,
					.div = 0,
				},
			[WCH_CLKID_CLK_PLL] =
				{
#if CLK_SRC_IS(pll, clk_hse)
					.source = RCC_PLLSRC,
#else
					.source = 0,
#endif
					.mul = DT_PROP_OR(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), mul,
							  1),
					.div = DT_PROP_OR(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), div,
							  1),
				},
			[WCH_CLKID_CLK_HB] =
				{
					.source = 0,
					.mul = 0,
					.div = DT_PROP_OR(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_hb),
							  div, 1),
				},
			[WCH_CLKID_CLK_ADC] =
				{
					.source = 0,
					.mul = 0,
					.div = DT_PROP_OR(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_adc),
							  div, 1),
				},
		},
};

DEVICE_DT_INST_DEFINE(0, clock_control_wch_init, NULL, NULL, &clock_control_wch_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_wch_api);
