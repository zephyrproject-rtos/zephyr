/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017-2022 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>

#if defined(CONFIG_SOC_SERIES_STM32C0X)
#include <zephyr/dt-bindings/clock/stm32c0_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32F0X)
#include <zephyr/dt-bindings/clock/stm32f0_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
#if defined(CONFIG_SOC_STM32F10X_CONNECTIVITY_LINE_DEVICE)
#include <zephyr/dt-bindings/clock/stm32f10x_clock.h>
#else
#include <zephyr/dt-bindings/clock/stm32f1_clock.h>
#endif
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
#include <zephyr/dt-bindings/clock/stm32f3_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X)
#include <zephyr/dt-bindings/clock/stm32f4_clock.h>
#include <zephyr/dt-bindings/clock/stm32f410_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
#include <zephyr/dt-bindings/clock/stm32f7_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
#include <zephyr/dt-bindings/clock/stm32g0_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32G4X)
#include <zephyr/dt-bindings/clock/stm32g4_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
#include <zephyr/dt-bindings/clock/stm32l0_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32L1X)
#include <zephyr/dt-bindings/clock/stm32l1_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X)
#include <zephyr/dt-bindings/clock/stm32l4_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32WBX)
#include <zephyr/dt-bindings/clock/stm32wb_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32WB0X)
#include <zephyr/dt-bindings/clock/stm32wb0_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32WLX)
#include <zephyr/dt-bindings/clock/stm32wl_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32H5X)
#include <zephyr/dt-bindings/clock/stm32h5_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
#include <zephyr/dt-bindings/clock/stm32h7_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32H7RSX)
#include <zephyr/dt-bindings/clock/stm32h7rs_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
#include <zephyr/dt-bindings/clock/stm32n6_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32U0X)
#include <zephyr/dt-bindings/clock/stm32u0_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
#include <zephyr/dt-bindings/clock/stm32u5_clock.h>
#elif defined(CONFIG_SOC_SERIES_STM32WBAX)
#include <zephyr/dt-bindings/clock/stm32wba_clock.h>
#else
#include <zephyr/dt-bindings/clock/stm32_clock.h>
#endif

/** Common clock control device node for all STM32 chips */
#define STM32_CLOCK_CONTROL_NODE DT_NODELABEL(rcc)

/** RCC node related symbols */

#define STM32_AHB_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb_prescaler)
#define STM32_APB1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb1_prescaler)
#define STM32_APB2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb2_prescaler)
#define STM32_APB3_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb3_prescaler)
#define STM32_APB4_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb4_prescaler)
#define STM32_APB5_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb5_prescaler)
#define STM32_APB7_PRESCALER	DT_PROP(DT_NODELABEL(rcc), apb7_prescaler)
#define STM32_AHB3_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
#define STM32_AHB4_PRESCALER	DT_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
#define STM32_AHB5_PRESCALER	DT_PROP_OR(DT_NODELABEL(rcc), ahb5_prescaler, 1)
#define STM32_CPU1_PRESCALER	DT_PROP(DT_NODELABEL(rcc), cpu1_prescaler)
#define STM32_CPU2_PRESCALER	DT_PROP(DT_NODELABEL(rcc), cpu2_prescaler)

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb_prescaler)
#define STM32_CORE_PRESCALER	STM32_AHB_PRESCALER
#elif DT_NODE_HAS_PROP(DT_NODELABEL(rcc), cpu1_prescaler)
#define STM32_CORE_PRESCALER	STM32_CPU1_PRESCALER
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
#define STM32_FLASH_PRESCALER	STM32_AHB3_PRESCALER
#elif DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
#define STM32_FLASH_PRESCALER	STM32_AHB4_PRESCALER
#else
#define STM32_FLASH_PRESCALER	STM32_CORE_PRESCALER
#endif

#define STM32_ADC_PRESCALER	DT_PROP(DT_NODELABEL(rcc), adc_prescaler)
#define STM32_ADC12_PRESCALER	DT_PROP(DT_NODELABEL(rcc), adc12_prescaler)
#define STM32_ADC34_PRESCALER	DT_PROP(DT_NODELABEL(rcc), adc34_prescaler)

/** STM2H7RS specific RCC dividers */
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
#define STM32_D1CPRE	DT_PROP(DT_NODELABEL(rcc), dcpre)
#define STM32_HPRE	DT_PROP(DT_NODELABEL(rcc), hpre)
#define STM32_PPRE1	DT_PROP(DT_NODELABEL(rcc), ppre1)
#define STM32_PPRE2	DT_PROP(DT_NODELABEL(rcc), ppre2)
#define STM32_PPRE4	DT_PROP(DT_NODELABEL(rcc), ppre4)
#define STM32_PPRE5	DT_PROP(DT_NODELABEL(rcc), ppre5)
#else
#define STM32_D1CPRE	DT_PROP(DT_NODELABEL(rcc), d1cpre)
#define STM32_HPRE	DT_PROP(DT_NODELABEL(rcc), hpre)
#define STM32_D2PPRE1	DT_PROP(DT_NODELABEL(rcc), d2ppre1)
#define STM32_D2PPRE2	DT_PROP(DT_NODELABEL(rcc), d2ppre2)
#define STM32_D1PPRE	DT_PROP(DT_NODELABEL(rcc), d1ppre)
#define STM32_D3PPRE	DT_PROP(DT_NODELABEL(rcc), d3ppre)
#endif /* CONFIG_SOC_SERIES_STM32H7RSX */

/** STM2WBA specifics RCC dividers */
#define STM32_AHB5_DIV	DT_PROP(DT_NODELABEL(rcc), ahb5_div)

#define DT_RCC_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(rcc))

/* To enable use of IS_ENABLED utility macro, these symbols
 * should not be defined directly using DT_SAME_NODE.
 */
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(pll))
#define STM32_SYSCLK_SRC_PLL	1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_SYSCLK_SRC_HSI	1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_SYSCLK_SRC_HSE	1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_SYSCLK_SRC_MSI	1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#define STM32_SYSCLK_SRC_MSIS	1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(clk_csi))
#define STM32_SYSCLK_SRC_CSI	1
#endif
#if DT_SAME_NODE(DT_RCC_CLOCKS_CTRL, DT_NODELABEL(ic2))
#define STM32_SYSCLK_SRC_IC2	1
#endif


/** PLL node related symbols */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f2_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f4_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f7_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32g0_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32g4_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l4_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32u0_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32u5_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32wb_pll_clock, okay)  || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32wba_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32h7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32h7rs_pll_clock, okay)
#define STM32_PLL_ENABLED	1
#define STM32_PLL_M_DIVISOR	DT_PROP(DT_NODELABEL(pll), div_m)
#define STM32_PLL_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul_n)
#define STM32_PLL_P_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll), div_p)
#define STM32_PLL_P_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll), div_p, 1)
#define STM32_PLL_Q_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll), div_q)
#define STM32_PLL_Q_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll), div_q, 1)
#define STM32_PLL_R_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll), div_r)
#define STM32_PLL_R_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll), div_r, 1)
#define STM32_PLL_S_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll), div_s)
#define STM32_PLL_S_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll), div_s, 1)
#define STM32_PLL_FRACN_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll), fracn)
#define STM32_PLL_FRACN_VALUE	DT_PROP_OR(DT_NODELABEL(pll), fracn, 1)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(plli2s), st_stm32f4_plli2s_clock, okay)
#define STM32_PLLI2S_ENABLED	1
#define STM32_PLLI2S_M_DIVISOR		STM32_PLL_M_DIVISOR
#define STM32_PLLI2S_N_MULTIPLIER	DT_PROP(DT_NODELABEL(plli2s), mul_n)
#define STM32_PLLI2S_R_ENABLED		DT_NODE_HAS_PROP(DT_NODELABEL(plli2s), div_r)
#define STM32_PLLI2S_R_DIVISOR		DT_PROP_OR(DT_NODELABEL(plli2s), div_r, 1)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(plli2s), st_stm32f411_plli2s_clock, okay)
#define STM32_PLLI2S_ENABLED	1
#define STM32_PLLI2S_M_DIVISOR		DT_PROP(DT_NODELABEL(plli2s), div_m)
#define STM32_PLLI2S_N_MULTIPLIER	DT_PROP(DT_NODELABEL(plli2s), mul_n)
#define STM32_PLLI2S_Q_ENABLED		DT_NODE_HAS_PROP(DT_NODELABEL(plli2s), div_q)
#define STM32_PLLI2S_Q_DIVISOR		DT_PROP_OR(DT_NODELABEL(plli2s), div_q, 1)
#define STM32_PLLI2S_R_ENABLED		DT_NODE_HAS_PROP(DT_NODELABEL(plli2s), div_r)
#define STM32_PLLI2S_R_DIVISOR		DT_PROP_OR(DT_NODELABEL(plli2s), div_r, 1)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll2), st_stm32u5_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll2), st_stm32h7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll2), st_stm32h7rs_pll_clock, okay)
#define STM32_PLL2_ENABLED	1
#define STM32_PLL2_M_DIVISOR	DT_PROP(DT_NODELABEL(pll2), div_m)
#define STM32_PLL2_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll2), mul_n)
#define STM32_PLL2_P_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), div_p)
#define STM32_PLL2_P_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll2), div_p, 1)
#define STM32_PLL2_Q_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), div_q)
#define STM32_PLL2_Q_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll2), div_q, 1)
#define STM32_PLL2_R_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), div_r)
#define STM32_PLL2_R_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll2), div_r, 1)
#define STM32_PLL2_S_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), div_s)
#define STM32_PLL2_S_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll2), div_s, 1)
#define STM32_PLL2_T_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), div_t)
#define STM32_PLL2_T_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll2), div_t, 1)
#define STM32_PLL2_FRACN_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), fracn)
#define STM32_PLL2_FRACN_VALUE	DT_PROP_OR(DT_NODELABEL(pll2), fracn, 1)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll3), st_stm32h7_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll3), st_stm32u5_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll3), st_stm32h7rs_pll_clock, okay)
#define STM32_PLL3_ENABLED	1
#define STM32_PLL3_M_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_m)
#define STM32_PLL3_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll3), mul_n)
#define STM32_PLL3_P_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_p)
#define STM32_PLL3_P_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll3), div_p, 1)
#define STM32_PLL3_Q_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_q)
#define STM32_PLL3_Q_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll3), div_q, 1)
#define STM32_PLL3_R_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_r)
#define STM32_PLL3_R_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll3), div_r, 1)
#define STM32_PLL3_S_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), div_s)
#define STM32_PLL3_S_DIVISOR	DT_PROP_OR(DT_NODELABEL(pll3), div_s, 1)
#define STM32_PLL3_FRACN_ENABLED	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), fracn)
#define STM32_PLL3_FRACN_VALUE	DT_PROP_OR(DT_NODELABEL(pll3), fracn, 1)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f1_pll_clock, okay)
#define STM32_PLL_ENABLED	1
#define STM32_PLL_XTPRE		DT_PROP(DT_NODELABEL(pll), xtpre)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#define STM32_PLL_USBPRE	DT_PROP(DT_NODELABEL(pll), usbpre)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f0_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f100_pll_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32f105_pll_clock, okay)
#define STM32_PLL_ENABLED	1
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#define STM32_PLL_PREDIV	DT_PROP(DT_NODELABEL(pll), prediv)
#define STM32_PLL_USBPRE	DT_PROP(DT_NODELABEL(pll), otgfspre)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), st_stm32l0_pll_clock, okay)
#define STM32_PLL_ENABLED	1
#define STM32_PLL_DIVISOR	DT_PROP(DT_NODELABEL(pll), div)
#define STM32_PLL_MULTIPLIER	DT_PROP(DT_NODELABEL(pll), mul)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll2), st_stm32f105_pll2_clock, okay)
#define STM32_PLL2_ENABLED	1
#define STM32_PLL2_MULTIPLIER	DT_PROP(DT_NODELABEL(pll2), mul)
#define STM32_PLL2_PREDIV	DT_PROP(DT_NODELABEL(pll2), prediv)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll1), st_stm32n6_pll_clock, okay)
#define STM32_PLL1_ENABLED	1
#define STM32_PLL1_M_DIVISOR	DT_PROP(DT_NODELABEL(pll1), div_m)
#define STM32_PLL1_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll1), mul_n)
#define STM32_PLL1_P1_DIVISOR	DT_PROP(DT_NODELABEL(pll1), div_p1)
#define STM32_PLL1_P2_DIVISOR	DT_PROP(DT_NODELABEL(pll1), div_p2)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll2), st_stm32n6_pll_clock, okay)
#define STM32_PLL2_ENABLED	1
#define STM32_PLL2_M_DIVISOR	DT_PROP(DT_NODELABEL(pll2), div_m)
#define STM32_PLL2_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll2), mul_n)
#define STM32_PLL2_P1_DIVISOR	DT_PROP(DT_NODELABEL(pll2), div_p1)
#define STM32_PLL2_P2_DIVISOR	DT_PROP(DT_NODELABEL(pll2), div_p2)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll3), st_stm32n6_pll_clock, okay)
#define STM32_PLL3_ENABLED	1
#define STM32_PLL3_M_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_m)
#define STM32_PLL3_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll3), mul_n)
#define STM32_PLL3_P1_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_p1)
#define STM32_PLL3_P2_DIVISOR	DT_PROP(DT_NODELABEL(pll3), div_p2)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll4), st_stm32n6_pll_clock, okay)
#define STM32_PLL4_ENABLED	1
#define STM32_PLL4_M_DIVISOR	DT_PROP(DT_NODELABEL(pll4), div_m)
#define STM32_PLL4_N_MULTIPLIER	DT_PROP(DT_NODELABEL(pll4), mul_n)
#define STM32_PLL4_P1_DIVISOR	DT_PROP(DT_NODELABEL(pll4), div_p1)
#define STM32_PLL4_P2_DIVISOR	DT_PROP(DT_NODELABEL(pll4), div_p2)
#endif

/** PLL/PLL1 clock source */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll), clocks)
#define DT_PLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_PLL_SRC_MSI	1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#define STM32_PLL_SRC_MSIS	1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_PLL_SRC_HSI	1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_csi))
#define STM32_PLL_SRC_CSI	1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_PLL_SRC_HSE	1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(pll2))
#define STM32_PLL_SRC_PLL2	1
#endif

#endif

/** PLL2 clock source */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll2)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll2), clocks)
#define DT_PLL2_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll2))
#if DT_SAME_NODE(DT_PLL2_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_PLL2_SRC_MSI	1
#endif
#if DT_SAME_NODE(DT_PLL2_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#define STM32_PLL2_SRC_MSIS	1
#endif
#if DT_SAME_NODE(DT_PLL2_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_PLL2_SRC_HSI	1
#endif
#if DT_SAME_NODE(DT_PLL2_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_PLL2_SRC_HSE	1
#endif

#endif

/** PLL3 clock source */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll3)) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll3), clocks)
#define DT_PLL3_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll3))
#if DT_SAME_NODE(DT_PLL3_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_PLL3_SRC_MSI	1
#endif
#if DT_SAME_NODE(DT_PLL3_CLOCKS_CTRL, DT_NODELABEL(clk_msis))
#define STM32_PLL3_SRC_MSIS	1
#endif
#if DT_SAME_NODE(DT_PLL3_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_PLL3_SRC_HSI	1
#endif
#if DT_SAME_NODE(DT_PLL3_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_PLL3_SRC_HSE	1
#endif

#endif

/** PLL4 clock source */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll4), okay) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll4), clocks)
#define DT_PLL4_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(pll4))
#if DT_SAME_NODE(DT_PLL4_CLOCKS_CTRL, DT_NODELABEL(clk_msi))
#define STM32_PLL4_SRC_MSI	1
#endif
#if DT_SAME_NODE(DT_PLL4_CLOCKS_CTRL, DT_NODELABEL(clk_hsi))
#define STM32_PLL4_SRC_HSI	1
#endif
#if DT_SAME_NODE(DT_PLL4_CLOCKS_CTRL, DT_NODELABEL(clk_hse))
#define STM32_PLL4_SRC_HSE	1
#endif

#endif


/** Fixed clocks related symbols */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lse), fixed_clock, okay)
#define STM32_LSE_ENABLED	1
#define STM32_LSE_FREQ		DT_PROP(DT_NODELABEL(clk_lse), clock_frequency)
#define STM32_LSE_DRIVING	0
#define STM32_LSE_BYPASS	DT_PROP(DT_NODELABEL(clk_lse), lse_bypass)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lse), st_stm32_lse_clock, okay)
#define STM32_LSE_ENABLED	1
#define STM32_LSE_FREQ		DT_PROP(DT_NODELABEL(clk_lse), clock_frequency)
#define STM32_LSE_DRIVING	DT_PROP(DT_NODELABEL(clk_lse), driving_capability)
#define STM32_LSE_BYPASS	DT_PROP(DT_NODELABEL(clk_lse), lse_bypass)
#else
#define STM32_LSE_ENABLED	0
#define STM32_LSE_FREQ		0
#define STM32_LSE_DRIVING	0
#define STM32_LSE_BYPASS	0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32_msi_clock, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32l0_msi_clock, okay)
#define STM32_MSI_ENABLED	1
#define STM32_MSI_RANGE		DT_PROP(DT_NODELABEL(clk_msi), msi_range)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msi), st_stm32_msi_clock, okay)
#define STM32_MSI_ENABLED	1
#define STM32_MSI_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msi), msi_pll_mode)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msis), st_stm32u5_msi_clock, okay)
#define STM32_MSIS_ENABLED	1
#define STM32_MSIS_RANGE	DT_PROP(DT_NODELABEL(clk_msis), msi_range)
#define STM32_MSIS_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msis), msi_pll_mode)
#else
#define STM32_MSIS_ENABLED	0
#define STM32_MSIS_RANGE	0
#define STM32_MSIS_PLL_MODE	0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_msik), st_stm32u5_msi_clock, okay)
#define STM32_MSIK_ENABLED	1
#define STM32_MSIK_RANGE	DT_PROP(DT_NODELABEL(clk_msik), msi_range)
#define STM32_MSIK_PLL_MODE	DT_PROP(DT_NODELABEL(clk_msik), msi_pll_mode)
#else
#define STM32_MSIK_ENABLED	0
#define STM32_MSIK_RANGE	0
#define STM32_MSIK_PLL_MODE	0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_csi), fixed_clock, okay)
#define STM32_CSI_ENABLED	1
#define STM32_CSI_FREQ		DT_PROP(DT_NODELABEL(clk_csi), clock_frequency)
#else
#define STM32_CSI_FREQ		0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lsi), fixed_clock, okay)
#define STM32_LSI_ENABLED	1
#define STM32_LSI_FREQ		DT_PROP(DT_NODELABEL(clk_lsi), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lsi1), fixed_clock, okay)
#define STM32_LSI_ENABLED	1
#define STM32_LSI_FREQ		DT_PROP(DT_NODELABEL(clk_lsi1), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lsi2), fixed_clock, okay)
#define STM32_LSI_ENABLED	1
#define STM32_LSI_FREQ		DT_PROP(DT_NODELABEL(clk_lsi2), clock_frequency)
#else
#define STM32_LSI_FREQ		0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), fixed_clock, okay)
#define STM32_HSI_DIV_ENABLED	0
#define STM32_HSI_ENABLED	1
#define STM32_HSI_FREQ		DT_PROP(DT_NODELABEL(clk_hsi), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), st_stm32h7_hsi_clock, okay) \
	|| DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), st_stm32g0_hsi_clock, okay) \
	|| DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi), st_stm32c0_hsi_clock, okay)
#define STM32_HSI_DIV_ENABLED	1
#define STM32_HSI_ENABLED	1
#define STM32_HSI_DIVISOR	DT_PROP(DT_NODELABEL(clk_hsi), hsi_div)
#define STM32_HSI_FREQ		DT_PROP(DT_NODELABEL(clk_hsi), clock_frequency)
#else
#define STM32_HSI_DIV_ENABLED	0
#define STM32_HSI_DIVISOR	1
#define STM32_HSI_FREQ		0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), fixed_clock, okay)
#define STM32_HSE_ENABLED	1
#define STM32_HSE_FREQ		DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32_hse_clock, okay)
#define STM32_HSE_ENABLED	1
#define STM32_HSE_BYPASS	DT_PROP(DT_NODELABEL(clk_hse), hse_bypass)
#define STM32_HSE_FREQ		DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)
#define STM32_HSE_CSS		DT_PROP(DT_NODELABEL(clk_hse), css_enabled)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32wl_hse_clock, okay)
#define STM32_HSE_ENABLED	1
#define STM32_HSE_TCXO		DT_PROP(DT_NODELABEL(clk_hse), hse_tcxo)
#define STM32_HSE_DIV2		DT_PROP(DT_NODELABEL(clk_hse), hse_div2)
#define STM32_HSE_FREQ		DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32wba_hse_clock, okay)
#define STM32_HSE_ENABLED	1
#define STM32_HSE_DIV2		DT_PROP(DT_NODELABEL(clk_hse), hse_div2)
#define STM32_HSE_FREQ		DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hse), st_stm32n6_hse_clock, okay)
#define STM32_HSE_ENABLED	1
#define STM32_HSE_BYPASS	DT_PROP(DT_NODELABEL(clk_hse), hse_bypass)
#define STM32_HSE_DIV2		DT_PROP(DT_NODELABEL(clk_hse), hse_div2)
#define STM32_HSE_FREQ		DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)
#else
#define STM32_HSE_FREQ		0
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi48), fixed_clock, okay)
#define STM32_HSI48_ENABLED	1
#define STM32_HSI48_FREQ	DT_PROP(DT_NODELABEL(clk_hsi48), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hsi48), st_stm32_hsi48_clock, okay)
#define STM32_HSI48_ENABLED	1
#define STM32_HSI48_FREQ	DT_PROP(DT_NODELABEL(clk_hsi48), clock_frequency)
#define STM32_HSI48_CRS_USB_SOF	DT_PROP(DT_NODELABEL(clk_hsi48), crs_usb_sof)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(perck), st_stm32_clock_mux, okay)
#define STM32_CKPER_ENABLED	1
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(cpusw), st_stm32_clock_mux, okay)
#define STM32_CPUSW_ENABLED	1
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic1), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC1_ENABLED	1
#define STM32_IC1_PLL_SRC	DT_PROP(DT_NODELABEL(ic1), pll_src)
#define STM32_IC1_DIV		DT_PROP(DT_NODELABEL(ic1), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic2), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC2_ENABLED	1
#define STM32_IC2_PLL_SRC	DT_PROP(DT_NODELABEL(ic2), pll_src)
#define STM32_IC2_DIV		DT_PROP(DT_NODELABEL(ic2), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic3), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC3_ENABLED	1
#define STM32_IC3_PLL_SRC	DT_PROP(DT_NODELABEL(ic3), pll_src)
#define STM32_IC3_DIV		DT_PROP(DT_NODELABEL(ic3), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic4), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC4_ENABLED	1
#define STM32_IC4_PLL_SRC	DT_PROP(DT_NODELABEL(ic4), pll_src)
#define STM32_IC4_DIV		DT_PROP(DT_NODELABEL(ic4), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic5), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC5_ENABLED	1
#define STM32_IC5_PLL_SRC	DT_PROP(DT_NODELABEL(ic5), pll_src)
#define STM32_IC5_DIV		DT_PROP(DT_NODELABEL(ic5), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic6), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC6_ENABLED	1
#define STM32_IC6_PLL_SRC	DT_PROP(DT_NODELABEL(ic6), pll_src)
#define STM32_IC6_DIV		DT_PROP(DT_NODELABEL(ic6), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic7), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC7_ENABLED	1
#define STM32_IC7_PLL_SRC	DT_PROP(DT_NODELABEL(ic7), pll_src)
#define STM32_IC7_DIV		DT_PROP(DT_NODELABEL(ic7), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic8), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC8_ENABLED	1
#define STM32_IC8_PLL_SRC	DT_PROP(DT_NODELABEL(ic8), pll_src)
#define STM32_IC8_DIV		DT_PROP(DT_NODELABEL(ic8), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic9), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC9_ENABLED	1
#define STM32_IC9_PLL_SRC	DT_PROP(DT_NODELABEL(ic9), pll_src)
#define STM32_IC9_DIV		DT_PROP(DT_NODELABEL(ic9), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic10), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC10_ENABLED	1
#define STM32_IC10_PLL_SRC	DT_PROP(DT_NODELABEL(ic10), pll_src)
#define STM32_IC10_DIV		DT_PROP(DT_NODELABEL(ic10), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic11), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC11_ENABLED	1
#define STM32_IC11_PLL_SRC	DT_PROP(DT_NODELABEL(ic11), pll_src)
#define STM32_IC11_DIV		DT_PROP(DT_NODELABEL(ic11), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic12), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC12_ENABLED	1
#define STM32_IC12_PLL_SRC	DT_PROP(DT_NODELABEL(ic12), pll_src)
#define STM32_IC12_DIV		DT_PROP(DT_NODELABEL(ic12), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic13), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC13_ENABLED	1
#define STM32_IC13_PLL_SRC	DT_PROP(DT_NODELABEL(ic13), pll_src)
#define STM32_IC13_DIV		DT_PROP(DT_NODELABEL(ic13), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic14), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC14_ENABLED	1
#define STM32_IC14_PLL_SRC	DT_PROP(DT_NODELABEL(ic14), pll_src)
#define STM32_IC14_DIV		DT_PROP(DT_NODELABEL(ic14), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic15), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC15_ENABLED	1
#define STM32_IC15_PLL_SRC	DT_PROP(DT_NODELABEL(ic15), pll_src)
#define STM32_IC15_DIV		DT_PROP(DT_NODELABEL(ic15), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic16), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC16_ENABLED	1
#define STM32_IC16_PLL_SRC	DT_PROP(DT_NODELABEL(ic16), pll_src)
#define STM32_IC16_DIV		DT_PROP(DT_NODELABEL(ic16), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic17), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC17_ENABLED	1
#define STM32_IC17_PLL_SRC	DT_PROP(DT_NODELABEL(ic17), pll_src)
#define STM32_IC17_DIV		DT_PROP(DT_NODELABEL(ic17), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic18), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC18_ENABLED	1
#define STM32_IC18_PLL_SRC	DT_PROP(DT_NODELABEL(ic18), pll_src)
#define STM32_IC18_DIV		DT_PROP(DT_NODELABEL(ic18), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic19), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC19_ENABLED	1
#define STM32_IC19_PLL_SRC	DT_PROP(DT_NODELABEL(ic19), pll_src)
#define STM32_IC19_DIV		DT_PROP(DT_NODELABEL(ic19), ic_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ic20), st_stm32n6_ic_clock_mux, okay)
#define STM32_IC20_ENABLED	1
#define STM32_IC20_PLL_SRC	DT_PROP(DT_NODELABEL(ic20), pll_src)
#define STM32_IC20_DIV		DT_PROP(DT_NODELABEL(ic20), ic_div)
#endif

/** Driver structure definition */

struct stm32_pclken {
	uint32_t bus : STM32_CLOCK_DIV_SHIFT;
	uint32_t div : (32 - STM32_CLOCK_DIV_SHIFT);
	uint32_t enr;
};

/** Device tree clocks helpers  */

#define STM32_CLOCK_INFO(clk_index, node_id)				\
	{								\
	.enr = DT_CLOCKS_CELL_BY_IDX(node_id, clk_index, bits),		\
	.bus = DT_CLOCKS_CELL_BY_IDX(node_id, clk_index, bus) &         \
		GENMASK(STM32_CLOCK_DIV_SHIFT - 1, 0),                   \
	.div = DT_CLOCKS_CELL_BY_IDX(node_id, clk_index, bus) >>	\
		STM32_CLOCK_DIV_SHIFT,					\
	}
#define STM32_DT_CLOCKS(node_id)					\
	{								\
		LISTIFY(DT_NUM_CLOCKS(node_id),				\
			STM32_CLOCK_INFO, (,), node_id)			\
	}

#define STM32_DT_INST_CLOCKS(inst)					\
	STM32_DT_CLOCKS(DT_DRV_INST(inst))

#define STM32_DOMAIN_CLOCK_INST_SUPPORT(inst) DT_INST_CLOCKS_HAS_IDX(inst, 1) ||
#define STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT				\
		(DT_INST_FOREACH_STATUS_OKAY(STM32_DOMAIN_CLOCK_INST_SUPPORT) 0)

#define STM32_DOMAIN_CLOCK_SUPPORT(id) DT_CLOCKS_HAS_IDX(DT_NODELABEL(id), 1) ||
#define STM32_DT_DEV_DOMAIN_CLOCK_SUPPORT					\
		(DT_FOREACH_STATUS_OKAY(STM32_DOMAIN_CLOCK_SUPPORT) 0)

/** Clock source binding accessors */

/**
 * @brief Obtain register field from clock configuration.
 *
 * @param clock clock bit field value.
 */
#define STM32_CLOCK_REG_GET(clock) \
	(((clock) >> STM32_CLOCK_REG_SHIFT) & STM32_CLOCK_REG_MASK)

/**
 * @brief Obtain position field from clock configuration.
 *
 * @param clock Clock bit field value.
 */
#define STM32_CLOCK_SHIFT_GET(clock) \
	(((clock) >> STM32_CLOCK_SHIFT_SHIFT) & STM32_CLOCK_SHIFT_MASK)

/**
 * @brief Obtain mask field from clock configuration.
 *
 * @param clock Clock bit field value.
 */
#define STM32_CLOCK_MASK_GET(clock) \
	(((clock) >> STM32_CLOCK_MASK_SHIFT) & STM32_CLOCK_MASK_MASK)

/**
 * @brief Obtain value field from clock configuration.
 *
 * @param clock Clock bit field value.
 */
#define STM32_CLOCK_VAL_GET(clock) \
	(((clock) >> STM32_CLOCK_VAL_SHIFT) & STM32_CLOCK_VAL_MASK)

/**
 * @brief Obtain register field from MCO configuration.
 *
 * @param mco_cfgr MCO configuration bit field value.
 */
#define STM32_MCO_CFGR_REG_GET(mco_cfgr) \
	(((mco_cfgr) >> STM32_MCO_CFGR_REG_SHIFT) & STM32_MCO_CFGR_REG_MASK)

/**
 * @brief Obtain position field from MCO configuration.
 *
 * @param mco_cfgr MCO configuration bit field value.
 */
#define STM32_MCO_CFGR_SHIFT_GET(mco_cfgr) \
	(((mco_cfgr) >> STM32_MCO_CFGR_SHIFT_SHIFT) & STM32_MCO_CFGR_SHIFT_MASK)

/**
 * @brief Obtain mask field from MCO configuration.
 *
 * @param mco_cfgr MCO configuration bit field value.
 */
#define STM32_MCO_CFGR_MASK_GET(mco_cfgr) \
	(((mco_cfgr) >> STM32_MCO_CFGR_MASK_SHIFT) & STM32_MCO_CFGR_MASK_MASK)

/**
 * @brief Obtain value field from MCO configuration.
 *
 * @param mco_cfgr MCO configuration bit field value.
 */
#define STM32_MCO_CFGR_VAL_GET(mco_cfgr) \
	(((mco_cfgr) >> STM32_MCO_CFGR_VAL_SHIFT) & STM32_MCO_CFGR_VAL_MASK)

#if defined(STM32_HSE_CSS)
/**
 * @brief Called if the HSE clock security system detects a clock fault.
 *
 * The function is called in interrupt context.
 *
 * The default (weakly-linked) implementation does nothing and should be
 * overridden.
 */
void stm32_hse_css_callback(void);
#endif

#ifdef CONFIG_SOC_SERIES_STM32WB0X
/**
 * @internal
 * @brief Type definition for LSI frequency update callbacks
 */
typedef void (*lsi_update_cb_t)(uint32_t new_lsi_frequency);

/**
 * @internal
 * @brief Registers a callback to invoke after each runtime measure and
 * update of the LSI frequency is completed.
 *
 * @param cb		Callback to invoke
 * @return 0		Registration successful
 * @return ENOMEM	Too many callbacks registered
 *
 * @note Callbacks are NEVER invoked if runtime LSI measurement is disabled
 */
int stm32wb0_register_lsi_update_callback(lsi_update_cb_t cb);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_ */
