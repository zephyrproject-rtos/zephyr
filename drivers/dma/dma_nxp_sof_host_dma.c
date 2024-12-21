/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>

/* used for driver binding */
#define DT_DRV_COMPAT nxp_sof_host_dma

/* macros used to parse DTS properties */
#define IDENTITY_VARGS(V, ...) IDENTITY(V)

#define _SOF_HOST_DMA_CHANNEL_INDEX_ARRAY(inst)\
	LISTIFY(DT_INST_PROP_OR(inst, dma_channels, 0), IDENTITY_VARGS, (,))

#define _SOF_HOST_DMA_CHANNEL_DECLARE(idx) {}

#define SOF_HOST_DMA_CHANNELS_DECLARE(inst)\
	FOR_EACH(_SOF_HOST_DMA_CHANNEL_DECLARE,\
		 (,), _SOF_HOST_DMA_CHANNEL_INDEX_ARRAY(inst))

LOG_MODULE_REGISTER(nxp_sof_host_dma);

/* note: This driver doesn't attempt to provide
 * a generic software-based DMA engine implementation.
 * As its name suggests, its only usage is in SOF
 * (Sound Open Firmware) for NXP plaforms which are
 * able to access the host memory directly from the
 * core on which the firmware is running.
 */

enum channel_state {
	CHAN_STATE_INIT = 0,
	CHAN_STATE_CONFIGURED,
};

struct sof_host_dma_channel {
	uint32_t src;
	uint32_t dest;
	uint32_t size;
	uint32_t direction;
	enum channel_state state;
};

struct sof_host_dma_data {
	/* this needs to be first */
	struct dma_context ctx;
	atomic_t channel_flags;
	struct sof_host_dma_channel *channels;
};

static int channel_change_state(struct sof_host_dma_channel *chan,
				enum channel_state next)
{
	enum channel_state prev = chan->state;

	/* validate transition */
	switch (prev) {
	case CHAN_STATE_INIT:
	case CHAN_STATE_CONFIGURED:
		if (next != CHAN_STATE_CONFIGURED) {
			return -EPERM;
		}
		break;
	default:
		LOG_ERR("invalid channel previous state: %d", prev);
		return -EINVAL;
	}

	chan->state = next;

	return 0;
}

static int sof_host_dma_reload(const struct device *dev, uint32_t chan_id,
			       uint32_t src, uint32_t dst, size_t size)
{
	ARG_UNUSED(src);
	ARG_UNUSED(dst);
	ARG_UNUSED(size);

	struct sof_host_dma_data *data;
	struct sof_host_dma_channel *chan;
	int ret;

	data = dev->data;

	if (chan_id >= data->ctx.dma_channels) {
		LOG_ERR("channel %d is not a valid channel ID", chan_id);
		return -EINVAL;
	}

	/* fetch channel data */
	chan = &data->channels[chan_id];

	/* validate state */
	if (chan->state != CHAN_STATE_CONFIGURED) {
		LOG_ERR("attempting to reload unconfigured DMA channel %d", chan_id);
		return -EINVAL;
	}

	if (chan->direction == HOST_TO_MEMORY) {
		/* the host may have modified the region we're about to copy
		 * to local memory. In this case, the data cache holds stale
		 * data so invalidate it to force a read from the main memory.
		 */
		ret = sys_cache_data_invd_range(UINT_TO_POINTER(chan->src),
						chan->size);
		if (ret < 0) {
			LOG_ERR("failed to invalidate data cache range");
			return ret;
		}
	}

	memcpy(UINT_TO_POINTER(chan->dest), UINT_TO_POINTER(chan->src), chan->size);

	/*
	 * MEMORY_TO_HOST transfer: force range to main memory so that
	 * the host doesn't read any stale data.
	 *
	 * HOST_TO_MEMORY transfer:
	 *	SOF assumes that data is copied from host to local memory via
	 *	DMA, which is not the case for imx platforms. For these
	 *	platforms, the DSP is in charge of copying the data from host to
	 *	local memory.
	 *
	 *	Additionally, because of the aforementioned assumption,
	 *	SOF performs a cache invalidation on the destination
	 *	memory chunk before data is copied further down the
	 *	pipeline.
	 *
	 *	If the destination memory chunk is cacheable what seems
	 *	to happen is that the invalidation operation forces the
	 *	DSP to fetch the data from RAM instead of the cache.
	 *	Since a writeback was never performed on the destination
	 *	memory chunk, the RAM will contain stale data.
	 *
	 *	With this in mind, the writeback should also be
	 *	performed in HOST_TO_MEMORY transfers (aka playback)
	 *	to keep the cache and RAM in sync. This way, the DSP
	 *	will read the correct data from RAM (when forced to do
	 *	so by the cache invalidation operation).
	 *
	 *	TODO: this is NOT optimal since we perform two unneeded
	 *	cache management operations and should be addressed in
	 *	SOF at some point.
	 */
	ret = sys_cache_data_flush_range(UINT_TO_POINTER(chan->dest), chan->size);
	if (ret < 0) {
		LOG_ERR("failed to flush data cache range");
		return ret;
	}

	return 0;
}


static int sof_host_dma_config(const struct device *dev, uint32_t chan_id,
			       struct dma_config *config)
{
	struct sof_host_dma_data *data;
	struct sof_host_dma_channel *chan;
	int ret;

	data = dev->data;

	if (chan_id >= data->ctx.dma_channels) {
		LOG_ERR("channel %d is not a valid channel ID", chan_id);
		return -EINVAL;
	}

	/* fetch channel data */
	chan = &data->channels[chan_id];

	/* attempt a state transition */
	ret = channel_change_state(chan, CHAN_STATE_CONFIGURED);
	if (ret < 0) {
		LOG_ERR("failed to change channel %d's state to CONFIGURED", chan_id);
		return ret;
	}

	/* SG configurations are not currently supported */
	if (config->block_count != 1) {
		LOG_ERR("invalid number of blocks: %d", config->block_count);
		return -EINVAL;
	}

	if (!config->head_block->source_address) {
		LOG_ERR("got NULL source address");
		return -EINVAL;
	}

	if (!config->head_block->dest_address) {
		LOG_ERR("got NULL destination address");
		return -EINVAL;
	}

	if (!config->head_block->block_size) {
		LOG_ERR("got 0 bytes to copy");
		return -EINVAL;
	}

	/* for now, only H2M and M2H transfers are supported */
	if (config->channel_direction != HOST_TO_MEMORY &&
	    config->channel_direction != MEMORY_TO_HOST) {
		LOG_ERR("invalid channel direction: %d",
			config->channel_direction);
		return -EINVAL;
	}

	/* latch onto the passed configuration */
	chan->src = config->head_block->source_address;
	chan->dest = config->head_block->dest_address;
	chan->size = config->head_block->block_size;
	chan->direction = config->channel_direction;

	LOG_DBG("configured channel %d with SRC 0x%x DST 0x%x SIZE 0x%x",
		chan_id, chan->src, chan->dest, chan->size);

	return 0;
}

static int sof_host_dma_start(const struct device *dev, uint32_t chan_id)
{
	/* nothing to be done here */
	return 0;
}

static int sof_host_dma_stop(const struct device *dev, uint32_t chan_id)
{
	/* nothing to be done here */
	return 0;
}

static int sof_host_dma_suspend(const struct device *dev, uint32_t chan_id)
{
	/* nothing to be done here */
	return 0;
}

static int sof_host_dma_resume(const struct device *dev, uint32_t chan_id)
{
	/* nothing to be done here */
	return 0;
}

static int sof_host_dma_get_status(const struct device *dev,
				   uint32_t chan_id, struct dma_status *stat)
{
	/* nothing to be done here */
	return 0;
}

static int sof_host_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *val)
{
	switch (type) {
	case DMA_ATTR_COPY_ALIGNMENT:
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*val = CONFIG_DMA_NXP_SOF_HOST_DMA_ALIGN;
		break;
	default:
		LOG_ERR("invalid attribute type: %d", type);
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(dma, sof_host_dma_api) = {
	.reload = sof_host_dma_reload,
	.config = sof_host_dma_config,
	.start = sof_host_dma_start,
	.stop = sof_host_dma_stop,
	.suspend = sof_host_dma_suspend,
	.resume = sof_host_dma_resume,
	.get_status = sof_host_dma_get_status,
	.get_attribute = sof_host_dma_get_attribute,
};

static int sof_host_dma_init(const struct device *dev)
{
	struct sof_host_dma_data *data = dev->data;

	data->channel_flags = ATOMIC_INIT(0);
	data->ctx.atomic = &data->channel_flags;

	return 0;
}

static struct sof_host_dma_channel channels[] = {
	SOF_HOST_DMA_CHANNELS_DECLARE(0),
};

static struct sof_host_dma_data sof_host_dma_data = {
	.ctx.magic = DMA_MAGIC,
	.ctx.dma_channels = ARRAY_SIZE(channels),
	.channels = channels,
};

/* assumption: only 1 SOF_HOST_DMA instance */
DEVICE_DT_INST_DEFINE(0, sof_host_dma_init, NULL,
		      &sof_host_dma_data, NULL,
		      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,
		      &sof_host_dma_api);
