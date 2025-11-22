/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx93_video_pll
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(video_pll, CONFIG_VIDEO_PLL_LOG_LEVEL);
#include <fsl_common.h>

struct video_pll_config {
	uint32_t rdiv;
	uint32_t mfi;
	uint32_t mfn;
	uint32_t mfd;
	uint32_t odiv;
	uint32_t freq;
};

#define VIDEO_PLL_INIT(inst)                                                                       \
	static int video_pll_init_##inst(const struct device *dev)                                 \
	{                                                                                          \
		const struct video_pll_config *cfg = dev->config;                                  \
		const fracn_pll_init_t pll_cfg = {                                                 \
			.rdiv = cfg->rdiv,                                                         \
			.mfi = cfg->mfi,                                                           \
			.mfn = cfg->mfn,                                                           \
			.mfd = cfg->mfd,                                                           \
			.odiv = cfg->odiv,                                                         \
		};                                                                                 \
		CLOCK_PllInit(VIDEOPLL, &pll_cfg);                                                 \
		g_clockSourceFreq[kCLOCK_VideoPll1] = cfg->freq;                                   \
		g_clockSourceFreq[kCLOCK_VideoPll1Out] = cfg->freq;                                \
		printk("Initialized VIDEO PLL to %d Hz\n", cfg->freq);                             \
		return 0;                                                                          \
	};                                                                                         \
	static const struct video_pll_config video_pll_cfg_##inst = {                              \
		.rdiv = DT_INST_PROP(inst, rdiv),                                                  \
		.mfi = DT_INST_PROP(inst, mfi),                                                    \
		.mfn = DT_INST_PROP(inst, mfn),                                                    \
		.mfd = DT_INST_PROP(inst, mfd),                                                    \
		.odiv = DT_INST_PROP(inst, odiv),                                                  \
		.freq = DT_INST_PROP(inst, pll_frequency),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, video_pll_init_##inst, NULL, NULL, &video_pll_cfg_##inst,      \
			      PRE_KERNEL_1, CONFIG_IMX93_VIDEO_PLL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_PLL_INIT)
