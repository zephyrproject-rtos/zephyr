/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_GD32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_GD32_CLOCK_CONTROL_H_

#include <drivers/clock_control.h>
#include <dt-bindings/clock/gd32_clock.h>

/* common clock control device node for all GD32 chips */
#define GD32_CLOCK_CONTROL_NODE DT_NODELABEL(rcc)

#if DT_NODE_HAS_PROP(DT_INST(0, gd_gd32_rcc), ahb_prescaler)
#define GD32_AHB_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb_prescaler)
#else
#define GD32_AHB_PRESCALER	CONFIG_CLOCK_GD32_AHB_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, gd_gd32_rcc), apb1_prescaler)
#define GD32_APB1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb1_prescaler)
#else
#define GD32_APB1_PRESCALER	CONFIG_CLOCK_GD32_APB1_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, gd_gd32_rcc), apb2_prescaler)
#define GD32_APB2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb2_prescaler)
#elif !DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_gd32f0_rcc, okay)
	/* This should not be defined in F0 binding case */
#define GD32_APB2_PRESCALER	CONFIG_CLOCK_GD32_APB2_PRESCALER
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f1_pll_clock, okay)
#define GD32_PLL_XTPRE		DT_PROP(DT_NODELABEL(pll), xtre)
#define GD32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f100_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f105_pll_clock, okay)
#define GD32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#define GD32_PLL_PREDIV1	DT_PROP(DT_NODELABEL(pll), prediv)
/* We don't need to make a disctinction between PREDIV and PREDIV1 in dts */
/* As PREDIV and PREDIV1 have the same description we can use prop prediv for both */
#define GD32_PLL_PREDIV	GD32_PLL_PREDIV1
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32l0_pll_clock, okay)
#define GD32_PLL_DIVISOR	DT_PROP(DT_NODELABEL(pll), div)
#define GD32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#else
#define GD32_PLL_XTPRE		CONFIG_CLOCK_GD32_PLL_XTPRE
#define GD32_PLL_MULTIPLIER	CONFIG_CLOCK_GD32_PLL_MULTIPLIER
#define GD32_PLL_PREDIV1	CONFIG_CLOCK_GD32_PLL_PREDIV1
#define GD32_PLL_PREDIV	CONFIG_CLOCK_GD32_PLL_PREDIV
#define GD32_PLL_DIVISOR	CONFIG_CLOCK_GD32_PLL_DIVISOR
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), gd_gd32_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_gd32f0_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_gd32h7_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_gd32wb_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_gd32wl_rcc, okay)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(rcc), clocks)
#define DT_RCC_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(rcc))
#define GD32_SYSCLK_SRC_PLL	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(pll))
#define GD32_SYSCLK_SRC_HSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define GD32_SYSCLK_SRC_HSE	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define GD32_SYSCLK_SRC_MSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define GD32_SYSCLK_SRC_CSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_csi))
#else
#define GD32_SYSCLK_SRC_PLL	CONFIG_CLOCK_GD32_SYSCLK_SRC_PLL
#define GD32_SYSCLK_SRC_HSI	CONFIG_CLOCK_GD32_SYSCLK_SRC_HSI
#define GD32_SYSCLK_SRC_HSE	CONFIG_CLOCK_GD32_SYSCLK_SRC_HSE
#define GD32_SYSCLK_SRC_MSI	CONFIG_CLOCK_GD32_SYSCLK_SRC_MSI
#define GD32_SYSCLK_SRC_CSI	CONFIG_CLOCK_GD32_SYSCLK_SRC_CSI
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f1_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f100_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f105_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f2_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32f7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32g0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32g4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32h7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32l0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32l4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_gd32wb_pll_clock, okay)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll), clocks)
#define DT_PLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#define GD32_PLL_SRC_MSI	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define GD32_PLL_SRC_HSI	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define GD32_PLL_SRC_HSE	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define GD32_PLL_SRC_PLL2	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(pll2))
#else
#define GD32_PLL_SRC_MSI	CONFIG_CLOCK_GD32_PLL_SRC_MSI
#define GD32_PLL_SRC_HSI	CONFIG_CLOCK_GD32_PLL_SRC_HSI
#define GD32_PLL_SRC_HSE	CONFIG_CLOCK_GD32_PLL_SRC_HSE
#define GD32_PLL_SRC_PLL2	CONFIG_CLOCK_GD32_PLL_SRC_PLL2
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lse), fixed_clock, okay)
#define GD32_LSE_CLOCK		DT_PROP(DT_NODELABEL(clk_lse), clock_frequency)
#else
#define GD32_LSE_CLOCK		CONFIG_CLOCK_GD32_LSE
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_gd32_msi_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_gd32l0_msi_clock, okay)
#define GD32_MSI_RANGE		DT_PROP(DT_NODELABEL(clk_msi), msi_range)
#else
#define GD32_MSI_RANGE		CONFIG_CLOCK_GD32_MSI_RANGE
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_gd32_msi_clock, okay)
#define GD32_MSI_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msi), msi_pll_mode)
#else
#define GD32_MSI_PLL_MODE	CONFIG_CLOCK_GD32_MSI_PLL_MODE
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), st_gd32h7_hsi_clock, okay)
#define GD32_HSI_DIVISOR	DT_PROP(DT_NODELABEL(clk_hsi), hsi_div)
#else
#define GD32_HSI_DIVISOR	CONFIG_CLOCK_GD32_HSI_DIVISOR
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_gd32_hse_clock, okay)
#define GD32_HSE_BYPASS	DT_PROP(DT_NODELABEL(clk_hse), hse_bypass)
#else
#define GD32_HSE_BYPASS	CONFIG_CLOCK_GD32_HSE_BYPASS
#endif

struct gd32_pclken {
	uint32_t bus;
	uint32_t enr;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_GD32_CLOCK_CONTROL_H_ */
