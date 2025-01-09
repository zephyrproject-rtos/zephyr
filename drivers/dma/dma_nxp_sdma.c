/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include "fsl_sdma.h"

LOG_MODULE_REGISTER(nxp_sdma);

#define DMA_NXP_SDMA_BD_COUNT 2
#define DMA_NXP_SDMA_CHAN_DEFAULT_PRIO 4

#define DT_DRV_COMPAT nxp_sdma

AT_NONCACHEABLE_SECTION_ALIGN(static sdma_context_data_t
			      sdma_contexts[FSL_FEATURE_SDMA_MODULE_CHANNEL], 4);

struct sdma_dev_cfg {
	SDMAARM_Type *base;
	void (*irq_config)(void);
};

struct sdma_channel_data {
	sdma_handle_t handle;
	sdma_transfer_config_t transfer_cfg;
	sdma_peripheral_t peripheral;
	uint32_t direction;
	uint32_t index;
	const struct device *dev;
	sdma_buffer_descriptor_t *bd_pool; /*pre-allocated list of BD used for transfer */
	uint32_t bd_count; /* number of bd */
	uint32_t capacity; /* total transfer capacity for this channel */
	struct dma_config *dma_cfg;
	uint32_t event_source; /* DMA REQ number that trigger this channel */
	struct dma_status stat;

	void *arg; /* argument passed to user-defined DMA callback */
	dma_callback_t cb; /* user-defined callback for DMA transfer completion */
};

struct sdma_dev_data {
	struct dma_context dma_ctx;
	atomic_t *channels_atomic;
	struct sdma_channel_data chan[FSL_FEATURE_SDMA_MODULE_CHANNEL];
	sdma_buffer_descriptor_t bd_pool[FSL_FEATURE_SDMA_MODULE_CHANNEL][DMA_NXP_SDMA_BD_COUNT]
		__aligned(64);
};

static int dma_nxp_sdma_init_stat(struct sdma_channel_data *chan_data)
{
	chan_data->stat.read_position = 0;
	chan_data->stat.write_position = 0;

	switch (chan_data->direction) {
	case MEMORY_TO_PERIPHERAL:
		/* buffer is full */
		chan_data->stat.pending_length = chan_data->capacity;
		chan_data->stat.free = 0;
		break;
	case PERIPHERAL_TO_MEMORY:
		/* buffer is empty */
		chan_data->stat.pending_length = 0;
		chan_data->stat.free = chan_data->capacity;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dma_nxp_sdma_consume(struct sdma_channel_data *chan_data, uint32_t bytes)
{
	if (bytes > chan_data->stat.pending_length)
		return -EINVAL;

	chan_data->stat.read_position += bytes;
	chan_data->stat.read_position %= chan_data->capacity;

	if (chan_data->stat.read_position > chan_data->stat.write_position)
		chan_data->stat.free = chan_data->stat.read_position -
			chan_data->stat.write_position;
	else
		chan_data->stat.free = chan_data->capacity -
			(chan_data->stat.write_position - chan_data->stat.read_position);

	chan_data->stat.pending_length = chan_data->capacity - chan_data->stat.free;

	return 0;
}

static int dma_nxp_sdma_produce(struct sdma_channel_data *chan_data, uint32_t bytes)
{
	if (bytes > chan_data->stat.free)
		return -EINVAL;

	chan_data->stat.write_position += bytes;
	chan_data->stat.write_position %= chan_data->capacity;

	if (chan_data->stat.write_position > chan_data->stat.read_position)
		chan_data->stat.pending_length = chan_data->stat.write_position -
			chan_data->stat.read_position;
	else
		chan_data->stat.pending_length = chan_data->capacity -
			(chan_data->stat.read_position - chan_data->stat.write_position);

	chan_data->stat.free = chan_data->capacity - chan_data->stat.pending_length;

	return 0;
}

static void dma_nxp_sdma_isr(const void *data)
{
	uint32_t val;
	uint32_t i = 1;
	struct sdma_channel_data *chan_data;
	struct device *dev = (struct device *)data;
	struct sdma_dev_data *dev_data = dev->data;
	const struct sdma_dev_cfg *dev_cfg = dev->config;

	/* Clear channel 0 */
	SDMA_ClearChannelInterruptStatus(dev_cfg->base, 1U);

	/* Ignore channel 0, is used only for download */
	val = SDMA_GetChannelInterruptStatus(dev_cfg->base) >> 1U;
	while (val) {
		if ((val & 0x1) != 0) {
			chan_data = &dev_data->chan[i];
			SDMA_ClearChannelInterruptStatus(dev_cfg->base, 1 << i);
			SDMA_HandleIRQ(&chan_data->handle);

			if (chan_data->cb)
				chan_data->cb(chan_data->dev, chan_data->arg, i, DMA_STATUS_BLOCK);
		}
		i++;
		val >>= 1;
	}
}

void sdma_set_transfer_type(struct dma_config *config, sdma_transfer_type_t *type)
{
	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
		*type = kSDMA_MemoryToMemory;
		break;
	case MEMORY_TO_PERIPHERAL:
		*type = kSDMA_MemoryToPeripheral;
		break;
	case PERIPHERAL_TO_MEMORY:
		*type = kSDMA_PeripheralToMemory;
		break;
	case PERIPHERAL_TO_PERIPHERAL:
		*type = kSDMA_PeripheralToPeripheral;
		break;
	default:
		LOG_ERR("%s: channel direction not supported %d", __func__,
			config->channel_direction);
		return;
	}
	LOG_DBG("%s: dir %d type = %d", __func__, config->channel_direction, *type);
}

int sdma_set_peripheral_type(struct dma_config *config, sdma_peripheral_t *type)
{
	switch (config->dma_slot) {
	case kSDMA_PeripheralNormal_SP:
	case kSDMA_PeripheralMultiFifoPDM:
		*type = config->dma_slot;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void dma_nxp_sdma_callback(sdma_handle_t *handle, void *userData, bool TransferDone,
			   uint32_t bdIndex)
{
	const struct sdma_dev_cfg *dev_cfg;
	struct sdma_channel_data *chan_data = userData;
	sdma_buffer_descriptor_t *bd;
	int xfer_size;

	dev_cfg = chan_data->dev->config;

	xfer_size = chan_data->capacity / chan_data->bd_count;

	switch (chan_data->direction) {
	case MEMORY_TO_PERIPHERAL:
		dma_nxp_sdma_consume(chan_data, xfer_size);
		break;
	case PERIPHERAL_TO_MEMORY:
		dma_nxp_sdma_produce(chan_data, xfer_size);
		break;
	default:
		break;
	}

	bd = &chan_data->bd_pool[bdIndex];
	bd->status |= (uint8_t)kSDMA_BDStatusDone;

	SDMA_StartChannelSoftware(dev_cfg->base, chan_data->index);
}

static int dma_nxp_sdma_channel_init(const struct device *dev, uint32_t channel)
{
	const struct sdma_dev_cfg *dev_cfg = dev->config;
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;

	chan_data = &dev_data->chan[channel];
	SDMA_CreateHandle(&chan_data->handle, dev_cfg->base, channel, &sdma_contexts[channel]);

	SDMA_SetCallback(&chan_data->handle, dma_nxp_sdma_callback, chan_data);

	return 0;
}

static void dma_nxp_sdma_setup_bd(const struct device *dev, uint32_t channel,
				struct dma_config *config)
{
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;
	sdma_buffer_descriptor_t *crt_bd;
	struct dma_block_config *block_cfg;
	int i;

	chan_data = &dev_data->chan[channel];

	/* initialize bd pool */
	chan_data->bd_pool = &dev_data->bd_pool[channel][0];
	chan_data->bd_count = config->block_count;

	memset(chan_data->bd_pool, 0, sizeof(sdma_buffer_descriptor_t) * chan_data->bd_count);
	SDMA_InstallBDMemory(&chan_data->handle, chan_data->bd_pool, chan_data->bd_count);

	crt_bd = chan_data->bd_pool;
	block_cfg = config->head_block;

	for (i = 0; i < config->block_count; i++) {
		bool is_last = false;
		bool is_wrap = false;

		if (i == config->block_count - 1) {
			is_last = true;
			is_wrap = true;
		}

		SDMA_ConfigBufferDescriptor(crt_bd,
			block_cfg->source_address, block_cfg->dest_address,
			config->source_data_size, block_cfg->block_size,
			is_last, true, is_wrap, chan_data->transfer_cfg.type);

		chan_data->capacity += block_cfg->block_size;
		block_cfg = block_cfg->next_block;
		crt_bd++;
	}
}

static int dma_nxp_sdma_config(const struct device *dev, uint32_t channel,
			       struct dma_config *config)
{
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;
	struct dma_block_config *block_cfg;
	int ret;

	if (channel >= FSL_FEATURE_SDMA_MODULE_CHANNEL) {
		LOG_ERR("sdma_config() invalid channel %d", channel);
		return -EINVAL;
	}

	dma_nxp_sdma_channel_init(dev, channel);

	chan_data = &dev_data->chan[channel];
	chan_data->dev = dev;
	chan_data->direction = config->channel_direction;

	chan_data->cb = config->dma_callback;
	chan_data->arg = config->user_data;

	sdma_set_transfer_type(config, &chan_data->transfer_cfg.type);

	ret = sdma_set_peripheral_type(config, &chan_data->peripheral);
	if (ret < 0) {
		LOG_ERR("%s: failed to set peripheral type", __func__);
		return ret;
	}

	dma_nxp_sdma_setup_bd(dev, channel, config);
	ret = dma_nxp_sdma_init_stat(chan_data);
	if (ret < 0) {
		LOG_ERR("%s: failed to init stat", __func__);
		return ret;
	}

	block_cfg = config->head_block;

	/* prepare first block for transfer ...*/
	SDMA_PrepareTransfer(&chan_data->transfer_cfg,
			     block_cfg->source_address,
			     block_cfg->dest_address,
			     config->source_data_size, config->dest_data_size,
			     /* watermark = */64,
			     block_cfg->block_size, chan_data->event_source,
			     chan_data->peripheral, chan_data->transfer_cfg.type);

	/*... and submit it to SDMA engine.
	 * Note that SDMA transfer is later manually started by the dma_nxp_sdma_start()
	 */
	chan_data->transfer_cfg.isEventIgnore = false;
	chan_data->transfer_cfg.isSoftTriggerIgnore = false;
	SDMA_SubmitTransfer(&chan_data->handle, &chan_data->transfer_cfg);

	return 0;
}

static int dma_nxp_sdma_start(const struct device *dev, uint32_t channel)
{
	const struct sdma_dev_cfg *dev_cfg = dev->config;
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;

	if (channel >= FSL_FEATURE_SDMA_MODULE_CHANNEL) {
		LOG_ERR("%s: invalid channel %d", __func__, channel);
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	SDMA_SetChannelPriority(dev_cfg->base, channel, DMA_NXP_SDMA_CHAN_DEFAULT_PRIO);
	SDMA_StartChannelSoftware(dev_cfg->base, channel);

	return 0;
}

static int dma_nxp_sdma_stop(const struct device *dev, uint32_t channel)
{
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;

	if (channel >= FSL_FEATURE_SDMA_MODULE_CHANNEL) {
		LOG_ERR("%s: invalid channel %d", __func__, channel);
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	SDMA_StopTransfer(&chan_data->handle);
	return 0;
}

static int dma_nxp_sdma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;

	chan_data = &dev_data->chan[channel];

	stat->free = chan_data->stat.free;
	stat->pending_length = chan_data->stat.pending_length;

	return 0;
}

static int dma_nxp_sdma_reload(const struct device *dev, uint32_t channel, uint32_t src,
			       uint32_t dst, size_t size)
{
	struct sdma_dev_data *dev_data = dev->data;
	struct sdma_channel_data *chan_data;

	chan_data = &dev_data->chan[channel];

	if (!size)
		return 0;

	if (chan_data->direction == MEMORY_TO_PERIPHERAL)
		dma_nxp_sdma_produce(chan_data, size);
	else
		dma_nxp_sdma_consume(chan_data, size);

	return 0;
}

static int dma_nxp_sdma_get_attribute(const struct device *dev, uint32_t type, uint32_t *val)
{
	switch (type) {
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*val = 4;
		break;
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*val = 128; /* should be dcache_align */
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*val = DMA_NXP_SDMA_BD_COUNT;
		break;
	default:
		LOG_ERR("invalid attribute type: %d", type);
		return -EINVAL;
	}
	return 0;
}

static bool sdma_channel_filter(const struct device *dev, int chan_id, void *param)
{
	struct sdma_dev_data *dev_data = dev->data;

	/* chan 0 is reserved for boot channel */
	if (chan_id == 0)
		return false;

	if (chan_id >= FSL_FEATURE_SDMA_MODULE_CHANNEL)
		return false;

	dev_data->chan[chan_id].event_source = *((int *)param);
	dev_data->chan[chan_id].index = chan_id;

	return true;
}

static DEVICE_API(dma, sdma_api) = {
	.reload = dma_nxp_sdma_reload,
	.config = dma_nxp_sdma_config,
	.start = dma_nxp_sdma_start,
	.stop = dma_nxp_sdma_stop,
	.suspend = dma_nxp_sdma_stop,
	.resume = dma_nxp_sdma_start,
	.get_status = dma_nxp_sdma_get_status,
	.get_attribute = dma_nxp_sdma_get_attribute,
	.chan_filter = sdma_channel_filter,
};

static int dma_nxp_sdma_init(const struct device *dev)
{
	struct sdma_dev_data *data = dev->data;
	const struct sdma_dev_cfg *cfg = dev->config;
	sdma_config_t defconfig;

	data->dma_ctx.magic = DMA_MAGIC;
	data->dma_ctx.dma_channels = FSL_FEATURE_SDMA_MODULE_CHANNEL;
	data->dma_ctx.atomic = data->channels_atomic;

	SDMA_GetDefaultConfig(&defconfig);
	defconfig.ratio = kSDMA_ARMClockFreq;

	SDMA_Init(cfg->base, &defconfig);

	/* configure interrupts */
	cfg->irq_config();

	return 0;
}

#define DMA_NXP_SDMA_INIT(inst)						\
	static ATOMIC_DEFINE(dma_nxp_sdma_channels_atomic_##inst,	\
			     FSL_FEATURE_SDMA_MODULE_CHANNEL);		\
	static struct sdma_dev_data sdma_data_##inst = {		\
		.channels_atomic = dma_nxp_sdma_channels_atomic_##inst,	\
	};								\
	static void dma_nxp_sdma_##inst_irq_config(void);		\
	static const struct sdma_dev_cfg sdma_cfg_##inst = {		\
		.base = (SDMAARM_Type *)DT_INST_REG_ADDR(inst),				\
		.irq_config = dma_nxp_sdma_##inst_irq_config,		\
	};								\
	static void dma_nxp_sdma_##inst_irq_config(void)		\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ_(inst, priority),		\
			    dma_nxp_sdma_isr, DEVICE_DT_INST_GET(inst), 0);	\
		irq_enable(DT_INST_IRQN(inst));				\
	}								\
	DEVICE_DT_INST_DEFINE(inst, &dma_nxp_sdma_init, NULL,		\
			      &sdma_data_##inst, &sdma_cfg_##inst,	\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,	\
			      &sdma_api);				\

DT_INST_FOREACH_STATUS_OKAY(DMA_NXP_SDMA_INIT);
