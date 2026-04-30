/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_isi

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "video_mcux_isi_priv.h"

LOG_MODULE_REGISTER(video_mcux_isi_core, CONFIG_VIDEO_LOG_LEVEL);

const struct video_mcux_isi_core_data *video_mcux_isi_core_get_data(const struct device *core_dev)
{
	return core_dev->data;
}

static int video_mcux_isi_core_init(const struct device *dev)
{
	const struct video_mcux_isi_core_config *cfg = dev->config;
	struct video_mcux_isi_core_data *data = dev->data;
	int ret;

	data->base = cfg->base;

	if (cfg->clock_dev != NULL) {
		if (!device_is_ready(cfg->clock_dev)) {
			LOG_ERR("%s: clock device not ready", dev->name);
			return -ENODEV;
		}

		ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		if (ret) {
			LOG_ERR("%s: failed to enable clock (%d)", dev->name, ret);
			return ret;
		}
	}

	return 0;
}

#define VIDEO_MCUX_ISI_CORE_DEVICE(inst)                                                           \
	static const struct video_mcux_isi_core_config video_mcux_isi_core_config_##inst = {       \
		.base = (ISI_Type *)DT_INST_REG_ADDR(inst),                                        \
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks), \
			(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))), (NULL)),                        \
			 .clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks), \
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name)), (NULL)), \
	};                                                                                         \
	static struct video_mcux_isi_core_data video_mcux_isi_core_data_##inst;                    \
	DEVICE_DT_INST_DEFINE(inst, video_mcux_isi_core_init, NULL,                                \
			      &video_mcux_isi_core_data_##inst,                                    \
			      &video_mcux_isi_core_config_##inst, POST_KERNEL, 59, NULL);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_MCUX_ISI_CORE_DEVICE)
