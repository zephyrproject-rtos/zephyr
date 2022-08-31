/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_adsp_hda_host_in

#include <zephyr/drivers/dma.h>
#include "dma_intel_adsp_hda.h"

static const struct dma_driver_api intel_adsp_hda_dma_host_in_api = {
	.config = intel_adsp_hda_dma_host_in_config,
	.reload = intel_adsp_hda_dma_host_reload,
	.start = intel_adsp_hda_dma_start,
	.stop = intel_adsp_hda_dma_stop,
	.get_status = intel_adsp_hda_dma_status,
	.chan_filter = intel_adsp_hda_dma_chan_filter,
};

#define INTEL_ADSP_HDA_DMA_HOST_IN_INIT(inst)                                                      \
	static const struct intel_adsp_hda_dma_cfg intel_adsp_hda_dma##inst##_config = {           \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.regblock_size  = DT_INST_REG_SIZE(inst),					   \
		.dma_channels = DT_INST_PROP(inst, dma_channels),                                  \
		.direction = MEMORY_TO_HOST                                                        \
	};                                                                                         \
												   \
	static struct intel_adsp_hda_dma_data intel_adsp_hda_dma##inst##_data = {};                \
												   \
	DEVICE_DT_INST_DEFINE(inst, &intel_adsp_hda_dma_init, NULL,                                \
			      &intel_adsp_hda_dma##inst##_data,                                    \
			      &intel_adsp_hda_dma##inst##_config, POST_KERNEL,                     \
			      CONFIG_DMA_INIT_PRIORITY,                                            \
			      &intel_adsp_hda_dma_host_in_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_ADSP_HDA_DMA_HOST_IN_INIT)
