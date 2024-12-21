/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_rcc

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util_macro.h>

#include <ch32fun.h>

#define WCH_RCC_CLOCK_ID_OFFSET(id) (((id) >> 5) & 0xFF)
#define WCH_RCC_CLOCK_ID_BIT(id)    ((id) & 0x1F)

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll)) && DT_NODE_HAS_PROP(DT_NODELABEL(pll), clocks)
#define DT_PLL_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define WCH_RCC_PLL_SRC_IS_HSI 1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define WCH_RCC_PLL_SRC_IS_HSE 1
#endif
#endif

#define DT_RCC_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_NODELABEL(rcc))
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(pll))
#define WCH_RCC_SRC_IS_PLL 1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define WCH_RCC_SRC_IS_HSI 1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define WCH_RCC_SRC_IS_HSE 1
#endif

struct clock_control_wch_rcc_config {
	RCC_TypeDef *regs;
};

static int clock_control_wch_rcc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_wch_rcc_config *config = dev->config;
	RCC_TypeDef *regs = config->regs;
	uint8_t id = (uintptr_t)sys;
	uint32_t reg = (uint32_t)(&regs->AHBPCENR + WCH_RCC_CLOCK_ID_OFFSET(id));
	uint32_t val = sys_read32(reg);

	val |= BIT(WCH_RCC_CLOCK_ID_BIT(id));
	sys_write32(val, reg);

	return 0;
}

static int clock_control_wch_rcc_get_rate(const struct device *dev, clock_control_subsys_t sys,
					  uint32_t *rate)
{
	const struct clock_control_wch_rcc_config *config = dev->config;
	RCC_TypeDef *regs = config->regs;
	uint32_t cfgr0 = regs->CFGR0;
	uint32_t sysclk = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	uint32_t ahbclk = sysclk;

	if ((cfgr0 & RCC_HPRE_3) != 0) {
		/* The range 0b1000 divides by a power of 2, where 0b1000 is /2, 0b1001 is /4, etc.
		 */
		ahbclk /= 2 << ((cfgr0 & (RCC_HPRE_0 | RCC_HPRE_1 | RCC_HPRE_2)) >> 4);
	} else {
		/* The range 0b0nnn divides by n + 1, where 0b0000 is /1, 0b001 is /2, etc. */
		ahbclk /= ((cfgr0 & (RCC_HPRE_0 | RCC_HPRE_1 | RCC_HPRE_2)) >> 4) + 1;
	}

	/* The datasheet says that AHB == APB1 == APB2, but the registers imply that APB1 and APB2
	 * can be divided from the AHB clock. Assume that the clock tree diagram is correct and
	 * always return AHB.
	 */
	*rate = ahbclk;
	return 0;
}

static DEVICE_API(clock_control, clock_control_wch_rcc_api) = {
	.on = clock_control_wch_rcc_on,
	.get_rate = clock_control_wch_rcc_get_rate,
};

static int clock_control_wch_rcc_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_DT_HAS_WCH_CH32V00X_PLL_CLOCK_ENABLED)) {
		/* Disable the PLL before potentially changing the input clocks. */
		RCC->CTLR &= ~RCC_PLLON;
	}

	/* Always enable the LSI. */
	RCC->RSTSCKR |= RCC_LSION;
	while ((RCC->RSTSCKR & RCC_LSIRDY) == 0) {
	}

	if (IS_ENABLED(CONFIG_DT_HAS_WCH_CH32V00X_HSI_CLOCK_ENABLED)) {
		RCC->CTLR |= RCC_HSION;
		while ((RCC->CTLR & RCC_HSIRDY) == 0) {
		}
	}

	if (IS_ENABLED(CONFIG_DT_HAS_WCH_CH32V00X_HSE_CLOCK_ENABLED)) {
		RCC->CTLR |= RCC_HSEON;
		while ((RCC->CTLR & RCC_HSERDY) == 0) {
		}
	}

	if (IS_ENABLED(CONFIG_DT_HAS_WCH_CH32V00X_PLL_CLOCK_ENABLED)) {
		if (IS_ENABLED(WCH_RCC_PLL_SRC_IS_HSE)) {
			RCC->CFGR0 |= RCC_PLLSRC;
		} else if (IS_ENABLED(WCH_RCC_PLL_SRC_IS_HSI)) {
			RCC->CFGR0 &= ~RCC_PLLSRC;
		}
		RCC->CTLR |= RCC_PLLON;
		while ((RCC->CTLR & RCC_PLLRDY) == 0) {
		}
	}

	if (IS_ENABLED(WCH_RCC_SRC_IS_HSI)) {
		RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSI;
	} else if (IS_ENABLED(WCH_RCC_SRC_IS_HSE)) {
		RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSE;
	} else if (IS_ENABLED(WCH_RCC_SRC_IS_PLL)) {
		RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_PLL;
	}
	RCC->CTLR |= RCC_CSSON;

	/* Clear the interrupt flags. */
	RCC->INTR = RCC_CSSC | RCC_PLLRDYC | RCC_HSERDYC | RCC_LSIRDYC;
	/* HCLK = SYSCLK = APB1 */
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | RCC_HPRE_DIV1;
	/* Set the Flash to 0 wait state */
	FLASH->ACTLR = (FLASH->ACTLR & ~FLASH_ACTLR_LATENCY) | FLASH_ACTLR_LATENCY_1;

	return 0;
}

#define CLOCK_CONTROL_WCH_RCC_INIT(idx)                                                            \
	static const struct clock_control_wch_rcc_config clock_control_wch_rcc_##idx##_config = {  \
		.regs = (RCC_TypeDef *)DT_INST_REG_ADDR(idx),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, clock_control_wch_rcc_init, NULL, NULL,                         \
			      &clock_control_wch_rcc_##idx##_config, PRE_KERNEL_1,                 \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_wch_rcc_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_CONTROL_WCH_RCC_INIT)
