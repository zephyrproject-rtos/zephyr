/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_

#include <drivers/clock_control.h>
#include <dt-bindings/clock/stm32_clock.h>

/* common clock control device node for all STM32 chips */
#define STM32_CLOCK_CONTROL_NODE DT_NODELABEL(rcc)

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb_prescaler)
#define STM32_AHB_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), apb1_prescaler)
#define STM32_APB1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb1_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), apb2_prescaler)
#define STM32_APB2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb2_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), apb3_prescaler)
#define STM32_APB3_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb3_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
#define STM32_AHB3_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
#define STM32_AHB4_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), cpu1_prescaler)
#define STM32_CPU1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), cpu1_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), cpu2_prescaler)
#define STM32_CPU2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), cpu2_prescaler)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32h7_rcc, okay) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(rcc), d1cpre)
#define STM32_D1CPRE	DT_PROP(DT_NODELABEL(rcc), d1cpre)
#define STM32_HPRE	DT_PROP(DT_NODELABEL(rcc), hpre)
#define STM32_D2PPRE1	DT_PROP(DT_NODELABEL(rcc), d2ppre1)
#define STM32_D2PPRE2	DT_PROP(DT_NODELABEL(rcc), d2ppre2)
#define STM32_D1PPRE	DT_PROP(DT_NODELABEL(rcc), d1ppre)
#define STM32_D3PPRE	DT_PROP(DT_NODELABEL(rcc), d3ppre)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f2_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32g0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32g4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32u5_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32wb_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32h7_pll_clock, okay)
#define STM32_PLL_M_DIVISOR	DT_PROP(DT_NODELABEL(pll), div_m)
#define STM32_PLL_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul_n)
#define STM32_PLL_P_DIVISOR	DT_PROP(DT_NODELABEL(pll), div_p)
#define STM32_PLL_Q_DIVISOR	DT_PROP(DT_NODELABEL(pll), div_q)
#define STM32_PLL_R_DIVISOR	DT_PROP(DT_NODELABEL(pll), div_r)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll3), st_stm32h7_pll_clock, okay)
#define STM32_PLL3_ENABLE	1
#define STM32_PLL3_M_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_m)
#define STM32_PLL3_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll3), mul_n)
#define STM32_PLL3_P_ENABLE	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_p)
#define STM32_PLL3_P_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_p)
#define STM32_PLL3_Q_ENABLE	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_q)
#define STM32_PLL3_Q_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_q)
#define STM32_PLL3_R_ENABLE	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_r)
#define STM32_PLL3_R_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_r)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f1_pll_clock, okay)
#define STM32_PLL_XTPRE		DT_PROP(DT_NODELABEL(pll), xtre)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f100_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f105_pll_clock, okay)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#define STM32_PLL_PREDIV	DT_PROP(DT_NODELABEL(pll), prediv)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l0_pll_clock, okay)
#define STM32_PLL_DIVISOR	DT_PROP(DT_NODELABEL(pll), div)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), clocks)
#define DT_RCC_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(rcc))
#define STM32_SYSCLK_SRC_PLL	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(pll))
#define STM32_SYSCLK_SRC_HSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_SYSCLK_SRC_HSE	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_SYSCLK_SRC_MSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_SYSCLK_SRC_CSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_csi))
#define STM32_SYSCLK_SRC_MSIS	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll), clocks)
#define DT_PLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#define STM32_PLL_SRC_MSI	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_PLL_SRC_MSIS	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#define STM32_PLL_SRC_HSI	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_PLL_SRC_HSE	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_PLL_SRC_PLL2	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(pll2))
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lse), fixed_clock, okay)
#define STM32_LSE_CLOCK		DT_PROP(DT_NODELABEL(clk_lse), clock_frequency)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32_msi_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32l0_msi_clock, okay)
#define STM32_MSI_RANGE		DT_PROP(DT_NODELABEL(clk_msi), msi_range)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32_msi_clock, okay)
#define STM32_MSI_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msi), msi_pll_mode)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msis), st_stm32u5_msi_clock, okay)
#define STM32_MSIS_RANGE	DT_PROP(DT_NODELABEL(clk_msis), msi_range)
#define STM32_MSIS_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msis), msi_pll_mode)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), st_stm32h7_hsi_clock, okay)
#define STM32_HSI_DIVISOR	DT_PROP(DT_NODELABEL(clk_hsi), hsi_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32_hse_clock, okay)
#define STM32_HSE_BYPASS	DT_PROP(DT_NODELABEL(clk_hse), hse_bypass)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32wl_hse_clock, okay)
#define STM32_HSE_TCXO	DT_PROP(DT_NODELABEL(clk_hse), hse_tcxo)
#define STM32_HSE_DIV2	DT_PROP(DT_NODELABEL(clk_hse), hse_div2)
#endif

struct stm32_pclken {
	uint32_t bus;
	uint32_t enr;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_ */
