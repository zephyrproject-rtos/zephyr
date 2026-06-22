/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <fsl_clock.h>
#include <soc.h>
#ifdef CONFIG_USERSPACE
#include <fsl_trdc.h>
#include <fsl_sentinel.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_imx93_video_pll)
#define VIDEO_PLL_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_imx93_video_pll)
#endif

static int imx93_video_pll_init(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_imx93_video_pll)
	const fracn_pll_init_t pll_cfg = {
		.rdiv = DT_PROP(VIDEO_PLL_NODE, rdiv),
		.mfi = DT_PROP(VIDEO_PLL_NODE, mfi),
		.mfn = DT_PROP(VIDEO_PLL_NODE, mfn),
		.mfd = DT_PROP(VIDEO_PLL_NODE, mfd),
		.odiv = DT_PROP(VIDEO_PLL_NODE, odiv),
	};

	uint32_t freq = DT_PROP(VIDEO_PLL_NODE, pll_frequency);

	CLOCK_PllInit(VIDEOPLL, &pll_cfg);
	g_clockSourceFreq[kCLOCK_VideoPll1] = freq;
	g_clockSourceFreq[kCLOCK_VideoPll1Out] = freq;
#endif

	return 0;
}

#ifdef CONFIG_SOC_EARLY_INIT_HOOK
#ifdef CONFIG_USERSPACE
void soc_init_trdc(void)
{
	uint32_t i, j;

	/* Release TRDC(transfer owner of TRDC from s400 to m33) */
	SENTINEL_ReleaseRDC(TRDC_TYPE);

	/* Enable all access modes for MBC and MRC in all running mode */
	trdc_hardware_config_t hwConfig;
	trdc_memory_access_control_config_t memAccessConfig;

	(void)memset(&memAccessConfig, 0, sizeof(memAccessConfig));
	memAccessConfig.nonsecureUsrX = 1U;
	memAccessConfig.nonsecureUsrW = 1U;
	memAccessConfig.nonsecureUsrR = 1U;
	memAccessConfig.nonsecurePrivX = 1U;
	memAccessConfig.nonsecurePrivW = 1U;
	memAccessConfig.nonsecurePrivR = 1U;
	memAccessConfig.secureUsrX = 1U;
	memAccessConfig.secureUsrW = 1U;
	memAccessConfig.secureUsrR = 1U;
	memAccessConfig.securePrivX = 1U;
	memAccessConfig.securePrivW = 1U;
	memAccessConfig.securePrivR = 1U;

	TRDC_GetHardwareConfig(TRDC1, &hwConfig);
	for (i = 0U; i < hwConfig.mrcNumber; i++) {
		for (j = 0U; j < 8; j++) {
			TRDC_MrcSetMemoryAccessConfig(TRDC1, &memAccessConfig, i, j);
		}
	}

	for (i = 0U; i < hwConfig.mbcNumber; i++) {
		for (j = 0U; j < 8; j++) {
			TRDC_MbcSetMemoryAccessConfig(TRDC1, &memAccessConfig, i, j);
		}
	}
}
#endif

void soc_early_init_hook(void)
{
	/* Configure secure access to pin registers */
	GPIO1->PCNS = 0x0;
	GPIO2->PCNS = 0x0;
	GPIO3->PCNS = 0x0;
	GPIO4->PCNS = 0x0;

	/* keep system tick work fine during idle task */
	GPC_CTRL_CM33->CM_MISC &= ~GPC_CPU_CTRL_CM_MISC_SLEEP_HOLD_EN_MASK;
#ifdef CONFIG_USERSPACE
	/* atf configure basic trdc setting about m33 core
	 * init trdc for all running mode when enabled user space context
	 */
	soc_init_trdc();
#endif
	/* Init VIDEO PLL based on devicetree config */
	(void)imx93_video_pll_init();
}
#endif
