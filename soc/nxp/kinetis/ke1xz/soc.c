/*
 * Copyright 2024 NXP
 * Copyright (c) 2019-2021 Vestas Wind Systems A/S
 *
 * Based on NXP k6x soc.c, which is:
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <fsl_clock.h>
#include <cmsis_core.h>

#define ASSERT_WITHIN_RANGE(val, min, max, str) \
	BUILD_ASSERT(val >= min && val <= max, str)

#define ASSERT_ASYNC_CLK_DIV_VALID(val, str) \
	BUILD_ASSERT(val == 0 || val == 1 || val == 2 || val == 4 ||	\
		     val == 8 || val == 16 || val == 2 || val == 64, str)

#define TO_SYS_CLK_DIV(val) _DO_CONCAT(kSCG_SysClkDivBy, val)

#define kSCG_AsyncClkDivBy0 kSCG_AsyncClkDisable
#define TO_ASYNC_CLK_DIV(val) _DO_CONCAT(kSCG_AsyncClkDivBy, val)

#define SCG_CLOCK_NODE(name) DT_CHILD(DT_INST(0, nxp_kinetis_scg), name)
#define SCG_CLOCK_DIV(name) DT_PROP(SCG_CLOCK_NODE(name), clock_div)

/* System Clock configuration */
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(bus_clk), 2, 8,
		    "Invalid SCG bus clock divider value");
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(core_clk), 1, 16,
		    "Invalid SCG core clock divider value");

static const scg_sys_clk_config_t scg_sys_clk_config = {
	.divSlow = TO_SYS_CLK_DIV(SCG_CLOCK_DIV(bus_clk)),
	.divCore = TO_SYS_CLK_DIV(SCG_CLOCK_DIV(core_clk)),
#if DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(core_clk)), SCG_CLOCK_NODE(sirc_clk))
	.src     = kSCG_SysClkSrcSirc,
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(core_clk)), SCG_CLOCK_NODE(firc_clk))
	.src     = kSCG_SysClkSrcFirc,
#endif
};

/* Slow Internal Reference Clock (SIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(sircdiv2_clk),
		       "Invalid SCG SIRC divider 2 value");
static const scg_sirc_config_t scg_sirc_config = {
	.enableMode = kSCG_SircEnable,
	.div2       = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(sircdiv2_clk)),
#if MHZ(2) == DT_PROP(SCG_CLOCK_NODE(sirc_clk), clock_frequency)
	.range      = kSCG_SircRangeLow
#elif MHZ(8) == DT_PROP(SCG_CLOCK_NODE(sirc_clk), clock_frequency)
	.range      = kSCG_SircRangeHigh
#else
#error Invalid SCG SIRC clock frequency
#endif
};

/* Fast Internal Reference Clock (FIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(fircdiv2_clk),
		       "Invalid SCG FIRC divider 2 value");
static const scg_firc_config_t scg_firc_config = {
	.enableMode = kSCG_FircEnable,
	.div2       = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(fircdiv2_clk)), /* b20253 */
#if MHZ(48) == DT_PROP(SCG_CLOCK_NODE(firc_clk), clock_frequency)
	.range      = kSCG_FircRange48M,
#elif MHZ(52) == DT_PROP(SCG_CLOCK_NODE(firc_clk), clock_frequency)
	.range      = kSCG_FircRange52M,
#elif MHZ(56) == DT_PROP(SCG_CLOCK_NODE(firc_clk), clock_frequency)
	.range      = kSCG_FircRange56M,
#elif MHZ(60) == DT_PROP(SCG_CLOCK_NODE(firc_clk), clock_frequency)
	.range      = kSCG_FircRange60M,
#else
#error Invalid SCG FIRC clock frequency
#endif
	.trimConfig = NULL
};

static ALWAYS_INLINE void clk_init(void)
{
	const scg_sys_clk_config_t scg_sys_clk_config_safe = {
		.divSlow = kSCG_SysClkDivBy4,
		.divCore = kSCG_SysClkDivBy1,
		.src     = kSCG_SysClkSrcSirc
	};
	scg_sys_clk_config_t current;

	/* Configure SIRC */
	CLOCK_InitSirc(&scg_sirc_config);

	/* Temporary switch to safe SIRC in order to configure FIRC */
	CLOCK_SetRunModeSysClkConfig(&scg_sys_clk_config_safe);
	do {
		CLOCK_GetCurSysClkConfig(&current);
	} while (current.src != scg_sys_clk_config_safe.src);
	CLOCK_InitFirc(&scg_firc_config);

	/* Only RUN mode supported for now */
	CLOCK_SetRunModeSysClkConfig(&scg_sys_clk_config);
	do {
		CLOCK_GetCurSysClkConfig(&current);
	} while (current.src != scg_sys_clk_config.src);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart0), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpuart0,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpuart0), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart1), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpuart1,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpuart1), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart2), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpuart2,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpuart2), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c0), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpi2c0,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpi2c0), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c1), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpi2c1,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpi2c1), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexio), okay)
	CLOCK_SetIpSrc(kCLOCK_Flexio0,
		       DT_CLOCKS_CELL(DT_NODELABEL(flexio), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi0), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpspi0,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpspi0), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi1), okay)
	CLOCK_SetIpSrc(kCLOCK_Lpspi1,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpspi1), ip_source));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc0), okay)
	CLOCK_SetIpSrc(kCLOCK_Adc0,
		       DT_CLOCKS_CELL(DT_NODELABEL(adc0), ip_source));
#endif
}

void soc_early_init_hook(void)

{
	/* Initialize system clocks and PLL */
	clk_init();
}

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	/* SystemInit is provided by the NXP SDK */
	SystemInit();
}

#endif /* CONFIG_SOC_RESET_HOOK */
