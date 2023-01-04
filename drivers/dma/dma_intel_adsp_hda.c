/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Intel ADSP HDA DMA (Stream) driver
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

#include "dma_intel_adsp_hda.h"
#include <intel_adsp_hda.h>

int intel_adsp_hda_dma_host_in_config(const struct device *dev,
				       uint32_t channel,
				       struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
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
	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);

	if (res == 0) {
		*DGMBS(cfg->base, cfg->regblock_size, channel) =
			blk_cfg->block_size & HDA_ALIGN_MASK;

		if (dma_cfg->source_data_size <= 3) {
			/* set the sample container set bit to 16bits */
			*DGCS(cfg->base, cfg->regblock_size, channel) |= DGCS_SCS;
		}
	}

	return res;
}


int intel_adsp_hda_dma_host_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
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

	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);

	if (res == 0) {
		*DGMBS(cfg->base, cfg->regblock_size, channel) =
			blk_cfg->block_size & HDA_ALIGN_MASK;

		if (dma_cfg->dest_data_size <= 3) {
			/* set the sample container set bit to 16bits */
			*DGCS(cfg->base, cfg->regblock_size, channel) |= DGCS_SCS;
		}
	}

	return res;
}

int intel_adsp_hda_dma_link_in_config(const struct device *dev,
				       uint32_t channel,
				       struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
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
	buf = (uint8_t *)(uintptr_t)(blk_cfg->dest_address);
	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);

	if (res == 0 && dma_cfg->dest_data_size <= 3) {
		/* set the sample container set bit to 16bits */
		*DGCS(cfg->base, cfg->regblock_size, channel) |= DGCS_SCS;
	}

	return res;
}


int intel_adsp_hda_dma_link_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
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
	buf = (uint8_t *)(uintptr_t)(blk_cfg->source_address);

	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);

	if (res == 0 && dma_cfg->source_data_size <= 3) {
		/* set the sample container set bit to 16bits */
		*DGCS(cfg->base, cfg->regblock_size, channel) |= DGCS_SCS;
	}

	return res;
}


int intel_adsp_hda_dma_link_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	intel_adsp_hda_link_commit(cfg->base, cfg->regblock_size, channel, size);

	return 0;
}

int intel_adsp_hda_dma_host_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	intel_adsp_hda_host_commit(cfg->base, cfg->regblock_size, channel, size);

	return 0;
}

int intel_adsp_hda_dma_status(const struct device *dev, uint32_t channel,
	struct dma_status *stat)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	bool xrun_det;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	uint32_t unused = intel_adsp_hda_unused(cfg->base, cfg->regblock_size, channel);
	uint32_t used = *DGBS(cfg->base, cfg->regblock_size, channel) - unused;

	stat->dir = cfg->direction;
	stat->busy = *DGCS(cfg->base, cfg->regblock_size, channel) & DGCS_GBUSY;
	stat->write_position = *DGBWP(cfg->base, cfg->regblock_size, channel);
	stat->read_position = *DGBRP(cfg->base, cfg->regblock_size, channel);
	stat->pending_length = used;
	stat->free = unused;

	switch (cfg->direction) {
	case MEMORY_TO_PERIPHERAL:
		xrun_det = intel_adsp_hda_is_buffer_underrun(cfg->base, cfg->regblock_size,
							     channel);
		if (xrun_det) {
			intel_adsp_hda_underrun_clear(cfg->base, cfg->regblock_size, channel);
			return -EPIPE;
		}
	case PERIPHERAL_TO_MEMORY:
		xrun_det = intel_adsp_hda_is_buffer_overrun(cfg->base, cfg->regblock_size,
							    channel);
		if (xrun_det) {
			intel_adsp_hda_overrun_clear(cfg->base, cfg->regblock_size, channel);
			return -EPIPE;
		}
	default:
		break;
	}

	return 0;
}

bool intel_adsp_hda_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
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

int intel_adsp_hda_dma_start(const struct device *dev, uint32_t channel)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	uint32_t size;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	if (intel_adsp_hda_is_enabled(cfg->base, cfg->regblock_size, channel)) {
		return 0;
	}

	intel_adsp_hda_enable(cfg->base, cfg->regblock_size, channel);
	if (cfg->direction == MEMORY_TO_PERIPHERAL) {
		size = intel_adsp_hda_get_buffer_size(cfg->base, cfg->regblock_size, channel);
		intel_adsp_hda_link_commit(cfg->base, cfg->regblock_size, channel, size);
	}

	return pm_device_runtime_get(dev);
}

int intel_adsp_hda_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	if (!intel_adsp_hda_is_enabled(cfg->base, cfg->regblock_size, channel)) {
		return 0;
	}

	intel_adsp_hda_disable(cfg->base, cfg->regblock_size, channel);

	return pm_device_runtime_put(dev);
}

int intel_adsp_hda_dma_init(const struct device *dev)
{
	struct intel_adsp_hda_dma_data *data = dev->data;
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	for (uint32_t i = 0; i < cfg->dma_channels; i++) {
		intel_adsp_hda_init(cfg->base, cfg->regblock_size, i);
	}

	data->ctx.dma_channels = cfg->dma_channels;
	data->ctx.atomic = data->channels_atomic;
	data->ctx.magic = DMA_MAGIC;

	pm_device_init_suspended(dev);
	return pm_device_runtime_enable(dev);
}

int intel_adsp_hda_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = DMA_BUF_ADDR_ALIGNMENT(
				DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_hda_link_out));
		break;
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMA_BUF_SIZE_ALIGNMENT(
				DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_hda_link_out));
		break;
	case DMA_ATTR_COPY_ALIGNMENT:
		*value = DMA_COPY_ALIGNMENT(DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_hda_link_out));
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
int intel_adsp_hda_dma_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif
