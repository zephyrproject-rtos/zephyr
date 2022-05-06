/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_cavs_hda_host_out

#include <zephyr/drivers/dma.h>
#include "dma_cavs_hda.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_cavs_hda_dma_host_out);

static const struct dma_driver_api cavs_hda_dma_host_out_api = {
	.config = cavs_hda_dma_host_out_config,
	.reload = cavs_hda_dma_host_reload,
	.start = cavs_hda_dma_start,
	.stop = cavs_hda_dma_stop,
	.get_status = cavs_hda_dma_status,
	.chan_filter = cavs_hda_dma_chan_filter,
};

#define CAVS_HDA_DMA_HOST_OUT_INIT(inst)                                                           \
	static const struct cavs_hda_dma_cfg cavs_hda_dma##inst##_config = {                       \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.dma_channels = DT_INST_PROP(inst, dma_channels),                                  \
		.direction = HOST_TO_MEMORY                                                        \
	};                                                                                         \
												   \
	static struct cavs_hda_dma_data cavs_hda_dma##inst##_data = {};                            \
												   \
	DEVICE_DT_INST_DEFINE(inst, &cavs_hda_dma_init, NULL, &cavs_hda_dma##inst##_data,          \
			      &cavs_hda_dma##inst##_config, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, \
			      &cavs_hda_dma_host_out_api);

DT_INST_FOREACH_STATUS_OKAY(CAVS_HDA_DMA_HOST_OUT_INIT)
