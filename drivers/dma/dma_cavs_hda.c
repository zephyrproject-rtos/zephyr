/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Intel CAVS HDA DMA (Stream) driver
 *
 * HDA is effectively, from the DSP, a ringbuffer (fifo) where the read
 * and write positions are maintained by the hardware and the software may
 * commit read/writes by writing to another register (DGFPBI) the length of
 * the read or write.
 *
 * It's important that the software knows the position in the ringbuffer to read
 * or write from. It's also important that the buffer be placed in the correct
 * memory region and aligned to 128 bytes. Lastly it's important the host and
 * dsp coordinate the order in which operations takes place. Doing all that
 * HDA streams are a fantastic bit of hardware and do their job well.
 *
 * There are 4 types of streams, with a set of each available to be used to
 * communicate to or from the Host or Link. Each stream set is uni directional.
 */

#include <zephyr/drivers/dma.h>

#include "dma_cavs_hda.h"

/* Define low level driver required values */
#define HDA_HOST_IN_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_in), reg, 0)
#define HDA_HOST_OUT_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 0)
#define HDA_STREAM_COUNT DT_PROP(DT_NODELABEL(hda_host_out), dma_channels)
#define HDA_REGBLOCK_SIZE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 1)
#include <cavs_hda.h>

int cavs_hda_dma_host_in_config(const struct device *dev,
				       uint32_t channel,
				       struct dma_config *dma_cfg)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA host in supports "
		 "MEMORY_TO_HOST");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->source_address);
	res = cavs_hda_set_buffer(cfg->base, channel, buf,
				  blk_cfg->block_size);

	if (res == 0 && dma_cfg->source_data_size <= 3) {
		/* set the sample container set bit to 16bits */
		*DGCS(cfg->base, channel) |= DGCS_SCS;
	}

	return res;
}


int cavs_hda_dma_host_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA host out supports "
		 "HOST_TO_MEMORY");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->dest_address);

	res = cavs_hda_set_buffer(cfg->base, channel, buf,
				  blk_cfg->block_size);

	if (res == 0 && dma_cfg->dest_data_size <= 3) {
		/* set the sample container set bit to 16bits */
		*DGCS(cfg->base, channel) |= DGCS_SCS;
	}

	return res;
}

int cavs_hda_dma_link_in_config(const struct device *dev,
				       uint32_t channel,
				       struct dma_config *dma_cfg)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA link in supports "
		 "PERIPHERAL_TO_MEMORY");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->source_address);
	res = cavs_hda_set_buffer(cfg->base, channel, buf,
				  blk_cfg->block_size);

	if (res == 0 && dma_cfg->source_data_size <= 3) {
		/* set the sample container set bit to 16bits */
		*DGCS(cfg->base, channel) |= DGCS_SCS;
	}

	return res;
}


int cavs_hda_dma_link_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA link out supports "
		 "MEMORY_TO_PERIPHERAL");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->dest_address);

	res = cavs_hda_set_buffer(cfg->base, channel, buf,
				  blk_cfg->block_size);

	if (res == 0 && dma_cfg->dest_data_size <= 3) {
		/* set the sample container set bit to 16bits */
		*DGCS(cfg->base, channel) |= DGCS_SCS;
	}

	return res;
}


int cavs_hda_dma_link_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	cavs_hda_link_commit(cfg->base, channel, size);

	return 0;
}

int cavs_hda_dma_host_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	cavs_hda_host_commit(cfg->base, channel, size);

	return 0;
}

int cavs_hda_dma_status(const struct device *dev, uint32_t channel,
	struct dma_status *stat)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	uint32_t unused = cavs_hda_unused(cfg->base, channel);
	uint32_t used = *DGBS(cfg->base, channel) - unused;

	stat->dir = cfg->direction;
	stat->busy = *DGCS(cfg->base, channel) & DGCS_GBUSY;
	stat->write_position = *DGBWP(cfg->base, channel);
	stat->read_position = *DGBRP(cfg->base, channel);
	stat->pending_length = used;
	stat->free = unused;

	return 0;
}

bool cavs_hda_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t requested_channel;

	if (!filter_param) {
		return true;
	}

	requested_channel = *(uint32_t *)filter_param;

	if (channel == requested_channel) {
		return true;
	}

	return false;
}

int cavs_hda_dma_start(const struct device *dev, uint32_t channel)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	cavs_hda_enable(cfg->base, channel);

	return 0;
}

int cavs_hda_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct cavs_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	cavs_hda_disable(cfg->base, channel);

	return 0;
}

int cavs_hda_dma_init(const struct device *dev)
{
	struct cavs_hda_dma_data *data = dev->data;
	const struct cavs_hda_dma_cfg *const cfg = dev->config;

	for (uint32_t i = 0; i < cfg->dma_channels; i++) {
		cavs_hda_init(cfg->base, i);
	}

	data->ctx.dma_channels = cfg->dma_channels;
	data->ctx.atomic = data->channels_atomic;
	data->ctx.magic = DMA_MAGIC;

	return 0;
}
