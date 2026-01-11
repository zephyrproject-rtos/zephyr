/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <fsl_common.h>

#define VIDEO_PLL_FREQ 400000000

#if defined(CONFIG_INIT_VIDEO_PLL)
static int soc_video_pll_init(void)
{
	/* Configure Video PLL to 400MHz */
	const fracn_pll_init_t videoPllCfg = {
		.rdiv = 1,
		.mfi = 200,
		.mfn = 0,
		.mfd = 100,
		.odiv = 12,
	};

	/** PLL_CLKx = (24M / rdiv * (mfi + mfn/mfd) / odiv) */
	CLOCK_PllInit(VIDEOPLL, &videoPllCfg);
	g_clockSourceFreq[kCLOCK_VideoPll1] = VIDEO_PLL_FREQ;
	g_clockSourceFreq[kCLOCK_VideoPll1Out] = VIDEO_PLL_FREQ;

	printf("Initialized VIDEO PLL to %d\n", g_clockSourceFreq[kCLOCK_VideoPll1Out]);

	return 0;
}
#endif /* CONFIG_INIT_VIDEO_PLL */

static int soc_init(void)
{
#if defined(CONFIG_INIT_VIDEO_PLL)
	int ret = soc_video_pll_init();

	if (ret) {
		printf("SoC VIDEO PLL init failed");
		return ret;
	}
#endif /* CONFIG_INIT_VIDEO_PLL */

	return 0;
}
/*
 * Init video pll based on config
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);
