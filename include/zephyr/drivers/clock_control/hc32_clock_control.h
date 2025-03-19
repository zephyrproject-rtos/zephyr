/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HC32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HC32_CLOCK_CONTROL_H_

#include "soc.h"
#include <zephyr/drivers/clock_control.h>
#if defined (HC32F460)
#include <zephyr/dt-bindings/clock/hc32f460_clock.h>
#elif defined (HC32F4A0)
#include <zephyr/dt-bindings/clock/hc32f4a0_clock.h>
#endif

#define HC32_HCLK_DIV_FN(v)		CLK_HCLK_DIV##v
#define HC32_HCLK_DIV(v)		HC32_HCLK_DIV_FN(v)
#define HC32_EXCLK_DIV_FN(v)	CLK_EXCLK_DIV##v
#define HC32_EXCLK_DIV(v)		HC32_EXCLK_DIV_FN(v)
#define HC32_PCLK_FN(n,v)		CLK_PCLK##n##_DIV##v
#define HC32_PCLK(n,v)			HC32_PCLK_FN(n,v)

#define HC32_CLK_MODULES_BIT(bit)	(1UL << (bit))
#define HC32_CLK_MODULES_OFFSET(n)	(4 * n)

#define HC32_CLOCK_CONTROL_NODE DT_NODELABEL(clk_sys)
#define HC32_CLOCK_SYSTEM_NODE	DT_NODELABEL(clocks)

#define HC32_HCLK_PRESCALER		DT_PROP(DT_NODELABEL(clk_sys), div_hclk)
#define HC32_EXCLK_PRESCALER	DT_PROP(DT_NODELABEL(clk_sys), div_exclk)
#define HC32_PCLK0_PRESCALER	DT_PROP(DT_NODELABEL(clk_sys), div_pclk0)
#define HC32_PCLK1_PRESCALER	DT_PROP(DT_NODELABEL(clk_sys), div_pclk1)
#define HC32_PCLK2_PRESCALER	DT_PROP(DT_NODELABEL(clk_sys), div_pclk2)
#define HC32_PCLK3_PRESCALER	DT_PROP(DT_NODELABEL(clk_sys), div_pclk3)
#define HC32_PCLK4_PRESCALER	DT_PROP(DT_NODELABEL(clk_sys), div_pclk4)

#define SYS_CLK_FREQ	DT_PROP(DT_CLOCKS_CTLR(DT_NODELABEL(clk_sys)), clock_frequency)
#define CORE_CLK_FREQ	SYS_CLK_FREQ / DT_PROP(DT_NODELABEL(clk_sys), div_hclk)
#if (CORE_CLK_FREQ !=CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#error "please confirm current cpu clock"
#endif
#define PCLK0_FREQ		SYS_CLK_FREQ / DT_PROP(DT_NODELABEL(clk_sys), div_pclk0)
#define PCLK1_FREQ		SYS_CLK_FREQ / DT_PROP(DT_NODELABEL(clk_sys), div_pclk1)
#define PCLK2_FREQ		SYS_CLK_FREQ / DT_PROP(DT_NODELABEL(clk_sys), div_pclk2)
#define PCLK3_FREQ		SYS_CLK_FREQ / DT_PROP(DT_NODELABEL(clk_sys), div_pclk3)
#define PCLK4_FREQ		SYS_CLK_FREQ / DT_PROP(DT_NODELABEL(clk_sys), div_pclk4)

#define DT_HC32_CLOCKS_CTLR	DT_CLOCKS_CTLR(DT_NODELABEL(clk_sys))
#if DT_SAME_NODE(DT_HC32_CLOCKS_CTLR, DT_NODELABEL(clk_pll))
#define HC32_SYSCLK_SRC_PLL	1
#elif DT_SAME_NODE(DT_HC32_CLOCKS_CTLR, DT_NODELABEL(clk_xtal))
#define HC32_SYSCLK_SRC_XTAL	1
#elif DT_SAME_NODE(DT_HC32_CLOCKS_CTLR, DT_NODELABEL(clk_hrc))
#define HC32_SYSCLK_SRC_HRC	1
#elif DT_SAME_NODE(DT_HC32_CLOCKS_CTLR, DT_NODELABEL(clk_mrc))
#define HC32_SYSCLK_SRC_MRC	1
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_pll), xhsc_hc32_clock_pll, okay) \
	&& DT_NODE_HAS_PROP(DT_NODELABEL(clk_pll), clocks)
#if DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(clk_pll)), DT_NODELABEL(clk_xtal))
#define HC32_PLL_SRC_XTAL	1
#else
#define HC32_PLL_SRC_HRC	1
#endif
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_pll), xhsc_hc32_clock_pll, okay)
#define HC32_PLL_ENABLED	1
#define HC32_PLL_M_DIVISOR		DT_PROP(DT_NODELABEL(clk_pll), div_m)
#define HC32_PLL_N_MULTIPLIER	DT_PROP(DT_NODELABEL(clk_pll), mul_n)
#define HC32_PLL_P_ENABLED		DT_NODE_HAS_PROP(DT_NODELABEL(clk_pll), div_p)
#define HC32_PLL_P_DIVISOR		DT_PROP_OR(DT_NODELABEL(clk_pll), div_p, 1)
#define HC32_PLL_Q_ENABLED		DT_NODE_HAS_PROP(DT_NODELABEL(clk_pll), div_q)
#define HC32_PLL_Q_DIVISOR		DT_PROP_OR(DT_NODELABEL(clk_pll), div_q, 1)
#define HC32_PLL_R_ENABLED		DT_NODE_HAS_PROP(DT_NODELABEL(clk_pll), div_r)
#define HC32_PLL_R_DIVISOR		DT_PROP_OR(DT_NODELABEL(clk_pll), div_r, 1)
#define HC32_PLL_FREQ			DT_PROP(DT_NODELABEL(clk_pll), clock_frequency)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_xtal), fixed_clock, okay)
#define HC32_XTAL_ENABLED	1
#define HC32_XTAL_FREQ		DT_PROP(DT_NODELABEL(clk_xtal), clock_frequency)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_xtal32), fixed_clock, okay)
#define HC32_XTAL32_ENABLED	1
#define HC32_XTAL32_FREQ	DT_PROP(DT_NODELABEL(clk_xtal32), clock_frequency)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hrc), fixed_clock, okay)
#define HC32_HRC_ENABLED	1
#define HC32_HRC_FREQ		DT_PROP(DT_NODELABEL(clk_hrc), clock_frequency)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_mrc), fixed_clock, okay)
#define HC32_MRC_ENABLED	1
#define HC32_MRC_FREQ		DT_PROP(DT_NODELABEL(clk_mrc), clock_frequency)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_lrc), fixed_clock, okay)
#define HC32_LRC_ENABLED	1
#define HC32_LRC_FREQ		DT_PROP(DT_NODELABEL(clk_lrc), clock_frequency)
#endif

#define HC32_MODULES_CLOCK_INFO(clk_index, node_id)				\
	{								\
	.bus = DT_CLOCKS_CELL_BY_IDX(node_id, clk_index, bus),	\
	.fcg = DT_CLOCKS_CELL_BY_IDX(node_id, clk_index, fcg),		\
	.bits = DT_CLOCKS_CELL_BY_IDX(node_id, clk_index, bits)		\
	}

#define HC32_MODULES_CLOCKS(node_id)					\
	{								\
		LISTIFY(DT_NUM_CLOCKS(node_id),				\
			HC32_MODULES_CLOCK_INFO, (,), node_id)			\
	}

struct hc32_modules_clock_sys {
	uint32_t bus;
	uint32_t fcg;
	uint32_t bits;
};
#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HC32_CLOCK_CONTROL_H_ */
