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

/*
 * Kconfig to device tree transition for clocks on STM32 targets:
 *
 * Following definitions are provided to allow a smooth transition
 * between Kconfig based to dts based clocks configuration.
 * These symbols allow to have both configuration schemes used simultaneoulsy
 * while giving precedence to dts based configuration once available on a
 * target.
 * Finally, once all in-tree users are converted to dts based configuration,
 * we'll be able to generate deprecation warnings for out of tree users of
 * Kconfig related symbols.
 */

#if defined(STM32_AHB_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_APB1_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_APB2_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_AHB3_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_AHB4_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_CPU1_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_CPU2_PRESCALER) || \
	defined(CONFIG_CLOCK_STM32_PLL_M_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER) || \
	defined(CONFIG_CLOCK_STM32_PLL_P_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL_Q_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL_R_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL_XTPRE) || \
	defined(CONFIG_CLOCK_STM32_PLL_MULTIPLIER) || \
	defined(CONFIG_CLOCK_STM32_PLL_PREDIV1) || \
	defined(CONFIG_CLOCK_STM32_PLL_PREDIV) || \
	defined(CONFIG_CLOCK_STM32_PLL_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL) || \
	defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_MSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_MSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_PLL2) || \
	defined(CONFIG_CLOCK_STM32_LSE) || \
	defined(CONFIG_CLOCK_STM32_MSI_RANGE) || \
	defined(CONFIG_CLOCK_STM32_MSI_PLL_MODE) || \
	defined(CONFIG_CLOCK_STM32_HSE_BYPASS) || \
	defined(CONFIG_CLOCK_STM32_D1CPRE) || \
	defined(CONFIG_CLOCK_STM32_HPRE) || \
	defined(CONFIG_CLOCK_STM32_D2PPRE1) || \
	defined(CONFIG_CLOCK_STM32_D2PPRE2) || \
	defined(CONFIG_CLOCK_STM32_D1PPRE) || \
	defined(CONFIG_CLOCK_STM32_D3PPRE) || \
	defined(CONFIG_CLOCK_STM32_PLL3_ENABLE) || \
	defined(CONFIG_CLOCK_STM32_PLL3_M_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL3_N_MULTIPLIER) || \
	defined(CONFIG_CLOCK_STM32_PLL3_P_ENABLE) || \
	defined(CONFIG_CLOCK_STM32_PLL3_P_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL3_Q_ENABLE) || \
	defined(CONFIG_CLOCK_STM32_PLL3_Q_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_PLL3_R_ENABLE) || \
	defined(CONFIG_CLOCK_STM32_PLL3_R_DIVISOR) || \
	defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_CSI) || \
	defined(CONFIG_CLOCK_STM32_HSI_DIVISOR)
#warning "Deprecated: Please use device tree for STM32 clock_control configuration"
/*
 * Use of Kconfig for STM32 clock_control configuration is deprecated.
 * It is replaced by use of device tree.
 * For more information, see:
 * https://github.com/zephyrproject-rtos/zephyr/pull/34120
 * https://github.com/zephyrproject-rtos/zephyr/pull/34609
 * https://github.com/zephyrproject-rtos/zephyr/pull/34701
 * and
 * https://github.com/zephyrproject-rtos/zephyr/issues/34633
 */
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_rcc), ahb_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32f0_rcc), ahb_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32u5_rcc), ahb_prescaler)
#define STM32_AHB_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb_prescaler)
#else
#define STM32_AHB_PRESCALER	CONFIG_CLOCK_STM32_AHB_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_rcc), apb1_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32f0_rcc), apb1_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32u5_rcc), apb1_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32wb_rcc), apb1_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32wl_rcc), apb1_prescaler)
#define STM32_APB1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb1_prescaler)
#else
#define STM32_APB1_PRESCALER	CONFIG_CLOCK_STM32_APB1_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_rcc), apb2_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32u5_rcc), apb2_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32wb_rcc), apb2_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32wl_rcc), apb2_prescaler)
#define STM32_APB2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb2_prescaler)
#elif !DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32f0_rcc, okay)
	/* This should not be defined in F0 binding case */
#define STM32_APB2_PRESCALER	CONFIG_CLOCK_STM32_APB2_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32u5_rcc), apb3_prescaler)
#define STM32_APB3_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb3_prescaler)
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32wl_rcc), ahb3_prescaler)
#define STM32_AHB3_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
#else
#define STM32_AHB3_PRESCALER	CONFIG_CLOCK_STM32_AHB3_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32wb_rcc), ahb4_prescaler)
#define STM32_AHB4_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
#else
#define STM32_AHB4_PRESCALER	CONFIG_CLOCK_STM32_AHB4_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32wb_rcc), cpu1_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32wl_rcc), cpu1_prescaler)
#define STM32_CPU1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), cpu1_prescaler)
#else
#define STM32_CPU1_PRESCALER	CONFIG_CLOCK_STM32_CPU1_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32wb_rcc), cpu2_prescaler) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32wl_rcc), cpu2_prescaler)
#define STM32_CPU2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), cpu2_prescaler)
#else
#define STM32_CPU2_PRESCALER	CONFIG_CLOCK_STM32_CPU2_PRESCALER
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32h7_rcc, okay) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(rcc), d1cpre)
#define STM32_D1CPRE	DT_PROP(DT_NODELABEL(rcc), d1cpre)
#define STM32_HPRE	DT_PROP(DT_NODELABEL(rcc), hpre)
#define STM32_D2PPRE1	DT_PROP(DT_NODELABEL(rcc), d2ppre1)
#define STM32_D2PPRE2	DT_PROP(DT_NODELABEL(rcc), d2ppre2)
#define STM32_D1PPRE	DT_PROP(DT_NODELABEL(rcc), d1ppre)
#define STM32_D3PPRE	DT_PROP(DT_NODELABEL(rcc), d3ppre)
#else
#define STM32_D1CPRE	CONFIG_CLOCK_STM32_D1CPRE
#define STM32_HPRE	CONFIG_CLOCK_STM32_HPRE
#define STM32_D2PPRE1	CONFIG_CLOCK_STM32_D2PPRE1
#define STM32_D2PPRE2	CONFIG_CLOCK_STM32_D2PPRE2
#define STM32_D1PPRE	CONFIG_CLOCK_STM32_D1PPRE
#define STM32_D3PPRE	CONFIG_CLOCK_STM32_D3PPRE
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
#else
#define STM32_PLL_M_DIVISOR	CONFIG_CLOCK_STM32_PLL_M_DIVISOR
#define STM32_PLL_N_MULTIPLIER	CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER
#define STM32_PLL_P_DIVISOR	CONFIG_CLOCK_STM32_PLL_P_DIVISOR
#define STM32_PLL_Q_DIVISOR	CONFIG_CLOCK_STM32_PLL_Q_DIVISOR
#define STM32_PLL_R_DIVISOR	CONFIG_CLOCK_STM32_PLL_R_DIVISOR
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
#else
#define STM32_PLL3_ENABLE	CONFIG_CLOCK_STM32_PLL3_ENABLE
#define STM32_PLL3_M_DIVISOR	CONFIG_CLOCK_STM32_PLL3_M_DIVISOR
#define STM32_PLL3_N_MULTIPLIER	CONFIG_CLOCK_STM32_PLL3_N_MULTIPLIER
#define STM32_PLL3_P_ENABLE	CONFIG_CLOCK_STM32_PLL3_P_ENABLE
#define STM32_PLL3_P_DIVISOR	CONFIG_CLOCK_STM32_PLL3_P_DIVISOR
#define STM32_PLL3_Q_ENABLE	CONFIG_CLOCK_STM32_PLL3_Q_ENABLE
#define STM32_PLL3_Q_DIVISOR	CONFIG_CLOCK_STM32_PLL3_Q_DIVISOR
#define STM32_PLL3_R_ENABLE	CONFIG_CLOCK_STM32_PLL3_R_ENABLE
#define STM32_PLL3_R_DIVISOR	CONFIG_CLOCK_STM32_PLL3_R_DIVISOR
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f1_pll_clock, okay)
#define STM32_PLL_XTPRE		DT_PROP(DT_NODELABEL(pll), xtre)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f100_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f105_pll_clock, okay)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#define STM32_PLL_PREDIV1	DT_PROP(DT_NODELABEL(pll), prediv)
/* We don't need to make a disctinction between PREDIV and PREDIV1 in dts */
/* As PREDIV and PREDIV1 have the same description we can use prop prediv for both */
#define STM32_PLL_PREDIV	STM32_PLL_PREDIV1
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l0_pll_clock, okay)
#define STM32_PLL_DIVISOR	DT_PROP(DT_NODELABEL(pll), div)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#else
#define STM32_PLL_XTPRE		CONFIG_CLOCK_STM32_PLL_XTPRE
#define STM32_PLL_MULTIPLIER	CONFIG_CLOCK_STM32_PLL_MULTIPLIER
#define STM32_PLL_PREDIV1	CONFIG_CLOCK_STM32_PLL_PREDIV1
#define STM32_PLL_PREDIV	CONFIG_CLOCK_STM32_PLL_PREDIV
#define STM32_PLL_DIVISOR	CONFIG_CLOCK_STM32_PLL_DIVISOR
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32f0_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32h7_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32u5_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32wb_rcc, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(rcc), st_stm32wl_rcc, okay)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(rcc), clocks)
#define DT_RCC_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(rcc))
#define STM32_SYSCLK_SRC_PLL	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(pll))
#define STM32_SYSCLK_SRC_HSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_SYSCLK_SRC_HSE	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_SYSCLK_SRC_MSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_SYSCLK_SRC_CSI	DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_csi))
#else
#define STM32_SYSCLK_SRC_PLL	CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL
#define STM32_SYSCLK_SRC_HSI	CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI
#define STM32_SYSCLK_SRC_HSE	CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE
#define STM32_SYSCLK_SRC_MSI	CONFIG_CLOCK_STM32_SYSCLK_SRC_MSI
#define STM32_SYSCLK_SRC_CSI	CONFIG_CLOCK_STM32_SYSCLK_SRC_CSI
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f1_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f100_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f105_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f2_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32g0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32g4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32h7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l4_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32u5_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32wb_pll_clock, okay)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll), clocks)
#define DT_PLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#define STM32_PLL_SRC_MSI	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_PLL_SRC_MSIS	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#define STM32_PLL_SRC_HSI	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_PLL_SRC_HSE	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_PLL_SRC_PLL2	DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(pll2))
#else
#define STM32_PLL_SRC_MSI	CONFIG_CLOCK_STM32_PLL_SRC_MSI
#define STM32_PLL_SRC_HSI	CONFIG_CLOCK_STM32_PLL_SRC_HSI
#define STM32_PLL_SRC_HSE	CONFIG_CLOCK_STM32_PLL_SRC_HSE
#define STM32_PLL_SRC_PLL2	CONFIG_CLOCK_STM32_PLL_SRC_PLL2
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lse), fixed_clock, okay)
#define STM32_LSE_CLOCK		DT_PROP(DT_NODELABEL(clk_lse), clock_frequency)
#else
#define STM32_LSE_CLOCK		CONFIG_CLOCK_STM32_LSE
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32_msi_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32l0_msi_clock, okay)
#define STM32_MSI_RANGE		DT_PROP(DT_NODELABEL(clk_msi), msi_range)
#elif !DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msis), st_stm32u5_msi_clock, okay)
#define STM32_MSI_RANGE		CONFIG_CLOCK_STM32_MSI_RANGE
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32_msi_clock, okay)
#define STM32_MSI_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msi), msi_pll_mode)
#elif !DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msis), st_stm32u5_msi_clock, okay)
#define STM32_MSI_PLL_MODE	CONFIG_CLOCK_STM32_MSI_PLL_MODE
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msis), st_stm32u5_msi_clock, okay)
#define STM32_MSIS_RANGE	DT_PROP(DT_NODELABEL(clk_msis), msi_range)
#define STM32_MSIS_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msis), msi_pll_mode)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), st_stm32h7_hsi_clock, okay)
#define STM32_HSI_DIVISOR	DT_PROP(DT_NODELABEL(clk_hsi), hsi_div)
#else
#define STM32_HSI_DIVISOR	CONFIG_CLOCK_STM32_HSI_DIVISOR
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32_hse_clock, okay)
#define STM32_HSE_BYPASS	DT_PROP(DT_NODELABEL(clk_hse), hse_bypass)
#else
#define STM32_HSE_BYPASS	CONFIG_CLOCK_STM32_HSE_BYPASS
#endif

struct stm32_pclken {
	uint32_t bus;
	uint32_t enr;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_ */
