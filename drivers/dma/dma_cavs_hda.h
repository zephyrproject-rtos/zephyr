/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DMA_DMA_CAVS_HDA_COMMON_H_
#define ZEPHYR_DRIVERS_DMA_DMA_CAVS_HDA_COMMON_H_

#define CAVS_HDA_MAX_CHANNELS DT_PROP(DT_NODELABEL(hda_host_out), dma_channels)

#include <zephyr/drivers/dma.h>

struct cavs_hda_dma_data {
	struct dma_context ctx;

	ATOMIC_DEFINE(channels_atomic, CAVS_HDA_MAX_CHANNELS);
};

struct cavs_hda_dma_cfg {
	uint32_t base;
	uint32_t dma_channels;
	enum dma_channel_direction direction;
};

int cavs_hda_dma_host_in_config(const struct device *dev,
				       uint32_t channel,
				struct dma_config *dma_cfg);

int cavs_hda_dma_host_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg);

int cavs_hda_dma_link_in_config(const struct device *dev,
				       uint32_t channel,
				struct dma_config *dma_cfg);

int cavs_hda_dma_link_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg);

int cavs_hda_dma_link_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size);

int cavs_hda_dma_host_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size);

int cavs_hda_dma_status(const struct device *dev, uint32_t channel,
			struct dma_status *stat);

bool cavs_hda_dma_chan_filter(const struct device *dev, int channel,
			      void *filter_param);

int cavs_hda_dma_start(const struct device *dev, uint32_t channel);

int cavs_hda_dma_stop(const struct device *dev, uint32_t channel);

int cavs_hda_dma_init(const struct device *dev);


#endif /* ZEPHYR_DRIVERS_DMA_DMA_CAVS_HDA_COMMON_H_ */
