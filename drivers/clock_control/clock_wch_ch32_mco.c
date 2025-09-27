/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32_clock_mco

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/common/ffs.h>

#include <hal_ch32fun.h>

#define CH32_MCO_SRC_IS(src)                                                                       \
	DT_SAME_NODE(DT_INST_CLOCKS_CTLR_BY_IDX(0, 0),                                             \
		     DT_CLOCKS_CTLR_BY_NAME(DT_NODELABEL(rcc), src))

struct ch32_mco_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t src;
};

static int ch32_mco_init(const struct device *dev)
{
	const struct ch32_mco_config *config = dev->config;
	RCC_TypeDef *rcc_regs = (RCC_TypeDef *)DT_REG_ADDR_BY_NAME(DT_NODELABEL(rcc), rcc);

	rcc_regs->CFGR0 = (rcc_regs->CFGR0 & ~RCC_MCO_NOCLOCK) |
			  (config->src << (find_lsb_set(RCC_CFGR0_MCO) - 1));

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

/* There is only support for one mco pin */
PINCTRL_DT_INST_DEFINE(0);

const static struct ch32_mco_config ch32_mco_config = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#if CH32_MCO_SRC_IS(clk_sys)
	.src = RCC_MCO_SYSCLK,
#elif CH32_MCO_SRC_IS(clk_hsi)
	.src = RCC_MCO_HSI,
#elif CH32_MCO_SRC_IS(clk_hse)
	.src = RCC_MCO_HSE,
#elif CH32_MCO_SRC_IS(pll)
	.src = RCC_MCO_PLLCLK_Div2,
#elif defined(RCC_MCO_PLL2CLK) && CH32_MCO_SRC_IS(pll2)
	.src = RCC_MCO_PLL2CLK,
#elif defined(RCC_MCO_PLL3CLK_Div2) && CH32_MCO_SRC_IS(pll3)
	.src = RCC_MCO_PLL3CLK_Div2,

#endif
};

DEVICE_DT_INST_DEFINE(0, ch32_mco_init, NULL, NULL, &ch32_mco_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
