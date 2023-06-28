/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>
#include <fsl_clock.h>

/* array of all possible clock sources */
static const struct imx_ccm_source sources[] = {
	{
		.type = IMX_CCM_TYPE_FIXED,
		.source.fixed.name = "osc_24m",
		.source.fixed.id = kCLOCK_Osc24M,
		.source.fixed.freq = 24000000,
	},
	/* note: this clock source is a PLL but it's set to a fixed frequency
	 * by the ROM code after boot.
	 *
	 * our code will not touch it.
	 */
	{
		.type = IMX_CCM_TYPE_FIXED,
		.source.fixed.name = "sys_pll1_pfd0_div2",
		.source.fixed.id = kCLOCK_SysPll1Pfd0Div2,
		.source.fixed.freq = 500000000,
	},
	/* note: this clock source is a PLL but it's set to a fixed frequency
	 * by the ROM code after boot.
	 *
	 * our code will not touch it.
	 */
	{
		.type = IMX_CCM_TYPE_FIXED,
		.source.fixed.name = "sys_pll1_pfd1_div2",
		.source.fixed.id = kCLOCK_SysPll1Pfd1Div2,
		.source.fixed.freq = 400000000,
	},
	{
		.type = IMX_CCM_TYPE_PLL,
		.source.fixed.name = "video_pll",
		.source.pll.offset = 0x1400, /* TODO: this field may not be needed */
		.source.pll.max_freq = 594000000,
	},
};

/* array of all clock roots */
static const struct imx_ccm_clock_root roots[] = {
	{
		.name = "lpuart1_clk_root",
		.id = kCLOCK_Root_Lpuart1,
		/* the index of the source needs to match its MUX value. */
		.sources = { sources[0], sources[1], sources[2], sources[3] },
		.source_num = 4,
	},
	{
		.name = "lpuart2_clk_root",
		.id = kCLOCK_Root_Lpuart2,
		/* the index of the source needs to match its MUX value. */
		.sources = { sources[0], sources[1], sources[2], sources[3] },
		.source_num = 4,
	},
};

/* array of all IP clocks */
static struct imx_ccm_clock clocks[] = {
	{
		.name = "lpuart1_clock",
		.id = kCLOCK_Lpuart1,
		.root = roots[0],
	},
	{
		.name = "lpuart2_clock",
		.id = kCLOCK_Lpuart2,
		.root = roots[1],
	},
};

/* used by imx_ccm.c */
struct imx_ccm_clock_config clock_config = {
	.clock_num = ARRAY_SIZE(clocks),
	.clocks = clocks,
};
