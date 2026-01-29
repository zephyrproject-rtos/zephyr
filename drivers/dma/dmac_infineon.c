/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_dmac

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "cy_dmac.h"
#include "cy_device.h"
#include "cy_trigmux.h"

LOG_MODULE_REGISTER(dmac_infineon, CONFIG_DMA_LOG_LEVEL);

#define MAX_DMA_CHANNELS CPUSS_DMAC_CH_NR
#define MIN_TRIG_CYCLES  2

struct infineon_dmac_channel {
	dma_callback_t user_cb;
	void *user_data;
	enum dma_channel_direction direction;
	struct dma_block_config *current_block;
	uint32_t total_blocks;
	uint32_t blocks_transferred;
	uint32_t complete_callback_en: 1;
	uint32_t error_callback_dis: 1;
	struct dma_config *config;
};

struct infineon_dmac_config {
	DMAC_Type *base;
	void (*irq_configure)(void);
	uint8_t num_channels;
};

struct infineon_dmac_data {
	struct dma_context dma_ctx;
	ATOMIC_DEFINE(channels_atomic, MAX_DMA_CHANNELS);
	struct infineon_dmac_channel channels[MAX_DMA_CHANNELS];
};

static inline cy_en_dmac_data_size_t dma_size_to_pdl(uint32_t size)
{
	switch (size) {
	case 1:
		return CY_DMAC_BYTE;
	case 2:
		return CY_DMAC_HALFWORD;
	case 4:
		return CY_DMAC_WORD;
	default:
		return CY_DMAC_BYTE;
	}
}

static inline bool dma_addr_adj_to_increment(enum dma_addr_adj adj)
{
	if (adj == DMA_ADDR_ADJ_INCREMENT) {
		return true;
	}

	return false;
}

static bool infineon_dmac_channel_is_busy(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;

	if ((Cy_DMAC_GetActiveChannel(config->base) & BIT(channel)) != 0) {
		return true;
	}

	return false;
}

static inline int infineon_dmac_sw_trigger(const struct device *dev, uint32_t channel)
{
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;
	cy_en_trigmux_status_t trig_status;

	if (ch->direction == MEMORY_TO_MEMORY || ch->direction == MEMORY_TO_PERIPHERAL) {
		trig_status = Cy_TrigMux_SwTrigger(TRIG0_IN_CPUSS_ZERO, MIN_TRIG_CYCLES);
		if (trig_status != CY_TRIGMUX_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

static int trigger_connect_setup_m2m(uint32_t channel, const struct infineon_dmac_config *config)
{
	cy_en_trigmux_status_t trig_status;

	trig_status =
		Cy_TrigMux_Connect(TRIG0_IN_CPUSS_ZERO, TRIG0_OUT_CPUSS_DMAC_TR_IN0 + channel);
	if (trig_status != CY_TRIGMUX_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int infineon_dmac_cleanup_channel(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;

	if (Cy_DMAC_Channel_GetPriority(config->base, channel) & DMAC_CH_CTL_ENABLED_Msk) {
		Cy_DMAC_Channel_Disable(config->base, channel);
	}

	Cy_DMAC_Descriptor_SetState(config->base, channel, CY_DMAC_DESCRIPTOR_PING, false);
	Cy_DMAC_Descriptor_SetState(config->base, channel, CY_DMAC_DESCRIPTOR_PONG, false);
	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));

	ch->blocks_transferred = 0;
	ch->total_blocks = 0;
	ch->current_block = NULL;
	ch->config = NULL;

	return 0;
}

/* Common helper to configure a single descriptor */
static int infineon_dmac_cfg_desc(const struct device *dev, uint32_t channel,
				  cy_en_dmac_descriptor_t desc_type, struct dma_block_config *block,
				  const struct dma_config *cfg,
				  enum dma_channel_direction direction)
{
	const struct infineon_dmac_config *config = dev->config;
	cy_stc_dmac_descriptor_config_t desc_config;
	cy_en_dmac_status_t status;

	/* Calculate data count */
	uint32_t data_count = block->block_size / cfg->source_data_size;

	/* Setup descriptor config */
	memset(&desc_config, 0, sizeof(desc_config));
	desc_config.srcAddress = (void *)(uintptr_t)block->source_address;
	desc_config.dstAddress = (void *)(uintptr_t)block->dest_address;
	desc_config.dataCount = data_count;
	desc_config.dataSize = dma_size_to_pdl(cfg->source_data_size);
	desc_config.srcAddrIncrement = dma_addr_adj_to_increment(block->source_addr_adj);
	desc_config.dstAddrIncrement = dma_addr_adj_to_increment(block->dest_addr_adj);
	desc_config.retrigger = CY_DMAC_RETRIG_IM;
	desc_config.preemptable = true;
	desc_config.interrupt = true;
	desc_config.flipping = false;
	desc_config.cpltState = false;

	/* By default, transfer what the user set for dataSize. However,
	 * if transferring between memory and a peripheral, make sure the
	 * peripheral access is using words.
	 */
	desc_config.srcTransferSize = CY_DMAC_TRANSFER_SIZE_DATA;
	desc_config.dstTransferSize = CY_DMAC_TRANSFER_SIZE_DATA;

	if (direction == PERIPHERAL_TO_MEMORY) {
		/* Peripheral is source */
		desc_config.srcTransferSize = CY_DMAC_TRANSFER_SIZE_WORD;
		desc_config.triggerType     = CY_DMAC_SINGLE_ELEMENT;
	} else if (direction == MEMORY_TO_PERIPHERAL) {
		/* Peripheral is destination */
		desc_config.dstTransferSize = CY_DMAC_TRANSFER_SIZE_WORD;
		desc_config.triggerType     = CY_DMAC_SINGLE_ELEMENT;
	} else if (direction == MEMORY_TO_MEMORY) {
		desc_config.triggerType     = CY_DMAC_SINGLE_DESCR;
	} else {
		/* Future or unsupported direction */
		LOG_ERR("Unsupported DMA direction: %d", direction);
		return -EINVAL;
	}

	/* Initialize descriptor */
	Cy_DMAC_Descriptor_SetState(config->base, channel, desc_type, false);
	status = Cy_DMAC_Descriptor_Init(config->base, channel, desc_type, &desc_config);
	if (status != CY_DMAC_SUCCESS) {
		if (desc_type == CY_DMAC_DESCRIPTOR_PING) {
			LOG_ERR("Failed to init descriptor PING (status=0x%x)", status);
		} else {
			LOG_ERR("Failed to init descriptor PONG (status=0x%x)", status);
		}
		return -EIO;
	}
	Cy_DMAC_Descriptor_SetState(config->base, channel, desc_type, true);
	Cy_DMAC_Channel_SetCurrentDescriptor(config->base, channel, desc_type);

	return 0;
}

static int infineon_dmac_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;
	uint32_t block_count = cfg->block_count;
	cy_en_dmac_status_t status;
	int ret;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	if (cfg->head_block == NULL) {
		return -EINVAL;
	}

	if (infineon_dmac_channel_is_busy(dev, channel)) {
		return -EBUSY;
	}

	infineon_dmac_cleanup_channel(dev, channel);

	/* Store config */
	ch->config = cfg;
	ch->user_cb = cfg->dma_callback;
	ch->user_data = cfg->user_data;
	ch->direction = cfg->channel_direction;
	ch->complete_callback_en = cfg->complete_callback_en;
	ch->error_callback_dis = cfg->error_callback_dis;
	ch->total_blocks = block_count;
	ch->blocks_transferred = 0;

	/* Setup descriptors in a loop */
	struct dma_block_config *block = cfg->head_block;

	ch->current_block = block->next_block;
	ret = infineon_dmac_cfg_desc(dev, channel, CY_DMAC_DESCRIPTOR_PING, block, cfg,
				     ch->direction);
	if (ret != 0) {
		infineon_dmac_cleanup_channel(dev, channel);
		return ret;
	}

	/* Channel config */
	cy_stc_dmac_channel_config_t ch_config = {
		.priority = cfg->channel_priority & 0x3,
		.enable = false,
		.descriptor = CY_DMAC_DESCRIPTOR_PING,
	};

	status = Cy_DMAC_Channel_Init(config->base, channel, &ch_config);
	if (status != CY_DMAC_SUCCESS) {
		infineon_dmac_cleanup_channel(dev, channel);
		return -EIO;
	}

	/* M2M trigger setup */
	if (cfg->channel_direction == MEMORY_TO_MEMORY) {
		ret = trigger_connect_setup_m2m(channel, config);
		if (ret != 0) {
			infineon_dmac_cleanup_channel(dev, channel);
			return ret;
		}
	}

	return 0;
}

static void infineon_dmac_isr(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;
	dma_callback_t callback = ch->user_cb;
	volatile cy_en_dmac_response_t response;
	cy_en_dmac_descriptor_t completed_desc;
	cy_en_dmac_descriptor_t reconfigure_desc;
	int ret;
	int status = -EIO;

	completed_desc = Cy_DMAC_Channel_GetCurrentDescriptor(config->base, channel);
	response = Cy_DMAC_Descriptor_GetResponse(config->base, channel, completed_desc);
	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));

	switch (response) {
	case CY_DMAC_DONE:
		status = 0;
		break;
	case CY_DMAC_NO_ERROR:
		/* In progress */
		return;
	case CY_DMAC_SRC_BUS_ERROR:
		LOG_ERR("DMA error: Source bus error (cause=0x%x)", response);
		status = -EIO;
		break;
	case CY_DMAC_DST_BUS_ERROR:
		LOG_ERR("DMA error: Destination bus error (cause=0x%x)", response);
		status = -EIO;
		break;
	case CY_DMAC_SRC_MISAL:
		LOG_ERR("DMA error: Source misaligned (cause=0x%x)", response);
		status = -EIO;
		break;
	case CY_DMAC_DST_MISAL:
		LOG_ERR("DMA error: Destination misaligned (cause=0x%x)", response);
		status = -EIO;
		break;
	case CY_DMAC_INVALID_DESCR:
		LOG_ERR("DMA error: Invalid descriptor (cause=0x%x)", response);
		status = -EIO;
		break;
	default:
		LOG_ERR("DMA unknown interrupt (cause: 0x%x)", response);
		status = -EIO;
		break;
	}

	ch->blocks_transferred++;

	/* handle multi-block transfers (2+ blocks), handle chaining */
	if (ch->total_blocks >= 2 && ch->blocks_transferred < ch->total_blocks) {
		/* Find current block in chain */
		struct dma_block_config *next_block = ch->current_block;

		if (next_block != NULL) {
			ch->current_block = next_block->next_block;

			if (completed_desc == CY_DMAC_DESCRIPTOR_PING) {
				reconfigure_desc = CY_DMAC_DESCRIPTOR_PONG;
			} else {
				reconfigure_desc = CY_DMAC_DESCRIPTOR_PING;
			}

			ret = infineon_dmac_cfg_desc(dev, channel, reconfigure_desc, next_block,
						     ch->config, ch->direction);

			if (ret == 0) {
				infineon_dmac_sw_trigger(dev, channel);
			}
		}
	}

	/* return if callback is not registered */
	if (callback == NULL) {
		return;
	}

	/* return error callback if enabled */
	if (status != 0) {
		if (!ch->error_callback_dis) {
			callback(dev, ch->user_data, channel, status);
			infineon_dmac_cleanup_channel(dev, channel);
		}
		return;
	}

	/* Handle per block callback */
	if (ch->complete_callback_en && ch->blocks_transferred < ch->total_blocks) {
		callback(dev, ch->user_data, channel, DMA_STATUS_BLOCK);
	} else {
		callback(dev, ch->user_data, channel, DMA_STATUS_COMPLETE);
	}
}

static void infineon_dmac_shared_isr(const struct device *dev)
{
	const struct infineon_dmac_config *config = dev->config;
	uint32_t intr_status = Cy_DMAC_GetInterruptStatus(config->base);

	if (intr_status == 0) {
		return;
	}

	for (uint32_t channel = 0; channel < MAX_DMA_CHANNELS; channel++) {
		if (intr_status & BIT(channel)) {
			infineon_dmac_isr(dev, channel);
		}
	}
}

static int infineon_dmac_start(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;
	uint32_t current_mask;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	for (uint32_t ch_idx = 0; ch_idx < MAX_DMA_CHANNELS; ch_idx++) {
		if (ch_idx != channel) {
			Cy_DMAC_Channel_Disable(config->base, ch_idx);
		}
	}

	ch->blocks_transferred = 0;

	Cy_DMAC_Enable(config->base);
	current_mask = Cy_DMAC_GetInterruptMask(config->base);
	Cy_DMAC_SetInterruptMask(config->base, current_mask | BIT(channel));
	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));
	Cy_DMAC_Channel_Enable(config->base, channel);

	return infineon_dmac_sw_trigger(dev, channel);
}

static int infineon_dmac_stop(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	Cy_DMAC_Channel_Disable(config->base, channel);
	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));

	if (infineon_dmac_channel_is_busy(dev, channel)) {
		Cy_DMAC_Descriptor_SetState(config->base, channel, CY_DMAC_DESCRIPTOR_PING, false);
		Cy_DMAC_Descriptor_SetState(config->base, channel, CY_DMAC_DESCRIPTOR_PONG, false);
	}

	return 0;
}

static int infineon_dmac_suspend(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	Cy_DMAC_Channel_Disable(config->base, channel);
	return 0;
}

static int infineon_dmac_resume(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number");

	Cy_DMAC_Channel_Enable(config->base, channel);

	if (data->channels[channel].direction == MEMORY_TO_MEMORY ||
	    data->channels[channel].direction == MEMORY_TO_PERIPHERAL) {
		return infineon_dmac_sw_trigger(dev, channel);
	}

	return 0;
}

static int infineon_dmac_reload(const struct device *dev, uint32_t channel, uint32_t src,
				uint32_t dst, size_t size)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	if ((size == 0) || infineon_dmac_channel_is_busy(dev, channel)) {
		return -EINVAL;
	}

	Cy_DMAC_Descriptor_SetSrcAddress(config->base, channel, CY_DMAC_DESCRIPTOR_PING,
					 (void *)(uintptr_t)src);
	Cy_DMAC_Descriptor_SetDstAddress(config->base, channel, CY_DMAC_DESCRIPTOR_PING,
					 (void *)(uintptr_t)dst);

	uint32_t data_size =
		BIT(Cy_DMAC_Descriptor_GetDataSize(config->base, channel, CY_DMAC_DESCRIPTOR_PING));
	Cy_DMAC_Descriptor_SetDataCount(config->base, channel, CY_DMAC_DESCRIPTOR_PING,
					size / data_size);
	Cy_DMAC_Descriptor_SetState(config->base, channel, CY_DMAC_DESCRIPTOR_PING, true);

	ch->blocks_transferred = 0;

	return 0;
}

static int infineon_dmac_get_status(const struct device *dev, uint32_t channel,
				    struct dma_status *status)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	uint32_t total;
	uint32_t current;
	uint32_t data_size;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	if (status == NULL) {
		return -EINVAL;
	}

	memset(status, 0, sizeof(*status));
	status->busy = infineon_dmac_channel_is_busy(dev, channel);
	status->dir = data->channels[channel].direction;

	if (status->busy) {
		cy_en_dmac_descriptor_t current_desc =
			Cy_DMAC_Channel_GetCurrentDescriptor(config->base, channel);
		total = Cy_DMAC_Descriptor_GetDataCount(config->base, channel, current_desc);
		current = Cy_DMAC_Descriptor_GetCurrentIndex(config->base, channel, current_desc);
		data_size =
			BIT(Cy_DMAC_Descriptor_GetDataSize(config->base, channel, current_desc));
		status->pending_length = (total - current) * data_size;
	}

	return 0;
}

static bool infineon_dmac_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	struct infineon_dmac_data *data = dev->data;
	(void)filter_param;

	__ASSERT((channel < ((const struct infineon_dmac_config *)(dev->config))->num_channels),
		 "Invalid DMA channel number %u", channel);

	if (atomic_test_bit(data->channels_atomic, channel)) {
		return false;
	}
	return true;
}

static void infineon_dmac_chan_release(const struct device *dev, uint32_t channel)
{
	struct infineon_dmac_data *data = dev->data;

	__ASSERT((channel < ((const struct infineon_dmac_config *)(dev->config))->num_channels),
		 "Invalid DMA channel number %u", channel);

	infineon_dmac_cleanup_channel(dev, channel);
	atomic_clear_bit(data->channels_atomic, channel);
}

static int infineon_dmac_init(const struct device *dev)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;

	data->dma_ctx.magic = DMA_MAGIC;
	data->dma_ctx.dma_channels = config->num_channels;
	data->dma_ctx.atomic = data->channels_atomic;

	memset(data->channels, 0, sizeof(data->channels));
	if (config->irq_configure != NULL) {
		config->irq_configure();
	}

	Cy_DMAC_ClearInterrupt(config->base, CY_DMAC_INTR_MASK);
	Cy_DMAC_SetInterruptMask(config->base, 0);

	return 0;
}

static const struct dma_driver_api infineon_dmac_api = {
	.config = infineon_dmac_config,
	.reload = infineon_dmac_reload,
	.start = infineon_dmac_start,
	.stop = infineon_dmac_stop,
	.suspend = infineon_dmac_suspend,
	.resume = infineon_dmac_resume,
	.get_status = infineon_dmac_get_status,
	.chan_filter = infineon_dmac_chan_filter,
	.chan_release = infineon_dmac_chan_release,
};

#define PSOC4_DMAC_INIT(n)                                                                         \
	static void infineon_dmac_irq_config_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), infineon_dmac_shared_isr,   \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct infineon_dmac_data infineon_dmac_data_##n;                                   \
	static const struct infineon_dmac_config infineon_dmac_config_##n = {                      \
		.base = (DMAC_Type *)DT_INST_REG_ADDR(n),                                          \
		.irq_configure = infineon_dmac_irq_config_##n,                                     \
		.num_channels = DT_INST_PROP(n, dma_channels),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, infineon_dmac_init, NULL, &infineon_dmac_data_##n,                \
			      &infineon_dmac_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &infineon_dmac_api);

DT_INST_FOREACH_STATUS_OKAY(PSOC4_DMAC_INIT)
