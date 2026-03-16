/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
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
#define SW_TRIG_ASSIGN   0

struct infineon_dmac_channel {
	dma_callback_t user_cb;
	void *user_data;
	enum dma_channel_direction direction;
	struct dma_block_config block_copies[CONFIG_INFINEON_DMA_MAX_XFERS];
	uint32_t total_blocks;
	uint32_t blocks_transferred;
	uint32_t current_block_index;
	uint32_t complete_callback_en: 1;
	uint32_t error_callback_dis: 1;
	uint32_t cyclic: 1;
	uint32_t source_data_size;
	uint32_t dest_data_size;
	uint32_t channel_priority;
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
		LOG_WRN("Requested data size is not supported, using byte size as default");
		return CY_DMAC_BYTE;
	}
}

static inline bool infineon_dmac_channel_is_busy(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;

	return (Cy_DMAC_GetActiveChannel(config->base) & BIT(channel)) != 0;
}

static inline int infineon_dmac_sw_trigger(void)
{
	cy_en_trigmux_status_t trig_status;

	trig_status = Cy_TrigMux_SwTrigger(SW_TRIG_ASSIGN, MIN_TRIG_CYCLES);
	if (trig_status != CY_TRIGMUX_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int trigger_connect_setup(uint32_t channel, struct dma_config *cfg)
{
	cy_en_trigmux_status_t trig_status;

	/* For M2M connect to SW trigger by default */
	if (cfg->channel_direction == MEMORY_TO_MEMORY) {
		cfg->dma_slot = SW_TRIG_ASSIGN;
	}

	/* Assign peripheral or SW trigger to respective DMA request assginments */
	trig_status = Cy_TrigMux_Connect(cfg->dma_slot, TRIG0_OUT_CPUSS_DMAC_TR_IN0 + channel);
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
	uint32_t mask = Cy_DMAC_GetInterruptMask(config->base);

	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));
	Cy_DMAC_SetInterruptMask(config->base, (mask & (~BIT(channel))));
	Cy_DMAC_Channel_Disable(config->base, channel);

	ch->blocks_transferred = 0;
	ch->total_blocks = 0;
	ch->current_block_index = 0;
	ch->user_cb = NULL;
	ch->user_data = NULL;

	return 0;
}

/* Descriptor configuration */
static int infineon_dmac_cfg_desc(const struct device *dev, uint32_t channel,
				  cy_en_dmac_descriptor_t desc_type, struct dma_block_config *block,
				  enum dma_channel_direction direction, uint32_t source_data_size,
				  uint32_t dest_data_size, bool cyclic)
{
	const struct infineon_dmac_config *config = dev->config;
	cy_stc_dmac_descriptor_config_t desc_config;
	cy_en_dmac_status_t status;

	/* Calculate data count */
	uint32_t data_count = block->block_size / source_data_size;

	/* Invalidate the descriptor before modify to prevent execution */
	Cy_DMAC_Descriptor_SetState(config->base, channel, desc_type, false);

	/* Setup descriptor config */
	memset(&desc_config, 0, sizeof(desc_config));
	desc_config.srcAddress = (void *)(uintptr_t)block->source_address;
	desc_config.dstAddress = (void *)(uintptr_t)block->dest_address;
	desc_config.dataCount = data_count;
	desc_config.dataSize = dma_size_to_pdl(source_data_size);
	desc_config.srcAddrIncrement = (block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT);
	desc_config.dstAddrIncrement = (block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT);
	desc_config.retrigger = CY_DMAC_RETRIG_IM;
	desc_config.preemptable = true;
	desc_config.interrupt = true;
	desc_config.flipping = false;
	desc_config.cpltState = cyclic;

	/* By default, transfer what the user set for dataSize. However,
	 * if transferring between memory and a peripheral, make sure the
	 * peripheral access is using words.
	 */
	desc_config.srcTransferSize = CY_DMAC_TRANSFER_SIZE_DATA;
	desc_config.dstTransferSize = CY_DMAC_TRANSFER_SIZE_DATA;

	if (direction == PERIPHERAL_TO_MEMORY) {
		/* Peripheral is source */
		desc_config.srcTransferSize = CY_DMAC_TRANSFER_SIZE_WORD;
		desc_config.triggerType = CY_DMAC_SINGLE_ELEMENT;
	} else if (direction == MEMORY_TO_PERIPHERAL) {
		/* Peripheral is destination */
		desc_config.dstTransferSize = CY_DMAC_TRANSFER_SIZE_WORD;
		desc_config.triggerType = CY_DMAC_SINGLE_ELEMENT;
	} else if (direction == MEMORY_TO_MEMORY) {
		desc_config.triggerType = CY_DMAC_SINGLE_DESCR;
	} else {
		/* Future or unsupported direction */
		LOG_ERR("Unsupported DMA direction: %d", direction);
		return -EINVAL;
	}

	/* Initialize descriptor */
	status = Cy_DMAC_Descriptor_Init(config->base, channel, desc_type, &desc_config);
	if (status != CY_DMAC_SUCCESS) {
		if (desc_type == CY_DMAC_DESCRIPTOR_PING) {
			LOG_ERR("Failed to init descriptor PING (status=0x%x)", status);
		} else {
			LOG_ERR("Failed to init descriptor PONG (status=0x%x)", status);
		}
		return -EIO;
	}

	/* Validate descriptor after modifying */
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

	if (block_count > CONFIG_INFINEON_DMA_MAX_XFERS) {
		LOG_ERR("Too many blocks: %u (max: %u)", block_count,
			CONFIG_INFINEON_DMA_MAX_XFERS);
		return -EINVAL;
	}

	infineon_dmac_cleanup_channel(dev, channel);

	ch->user_cb = cfg->dma_callback;
	ch->user_data = cfg->user_data;
	ch->direction = cfg->channel_direction;
	ch->complete_callback_en = cfg->complete_callback_en;
	ch->error_callback_dis = cfg->error_callback_dis;
	ch->cyclic = cfg->cyclic;
	ch->total_blocks = block_count;
	ch->blocks_transferred = 0;
	ch->current_block_index = 0;
	ch->source_data_size = cfg->source_data_size;
	ch->dest_data_size = cfg->dest_data_size;
	ch->channel_priority = cfg->channel_priority;

	/* Copy all blocks internally */
	struct dma_block_config *user_block = cfg->head_block;

	for (uint32_t i = 0; i < block_count && user_block != NULL; i++) {
		memcpy(ch->block_copies + i, user_block, sizeof(struct dma_block_config));
		user_block = user_block->next_block;
	}

	/* Setup first descriptor with first block */
	struct dma_block_config *first_block = &ch->block_copies[0];

	ret = infineon_dmac_cfg_desc(dev, channel, CY_DMAC_DESCRIPTOR_PING, first_block,
				     ch->direction, ch->source_data_size, ch->dest_data_size,
				     ch->cyclic);
	if (ret != 0) {
		LOG_ERR("Descriptor initialization fails");
		infineon_dmac_cleanup_channel(dev, channel);
		return ret;
	}

	/* Channel config */
	cy_stc_dmac_channel_config_t ch_config = {
		.priority = cfg->channel_priority & 0x3,
		.enable = false,
		.descriptor = CY_DMAC_DESCRIPTOR_PING,
	};

	/* Channel Init */
	status = Cy_DMAC_Channel_Init(config->base, channel, &ch_config);
	if (status != CY_DMAC_SUCCESS) {
		LOG_ERR("Descriptor Channel init fails");
		infineon_dmac_cleanup_channel(dev, channel);
		return -EIO;
	}

	/* Trigger setup for sw trigger and peripherals */
	ret = trigger_connect_setup(channel, cfg);
	if (ret != 0) {
		LOG_ERR("Trigger setup fails");
		infineon_dmac_cleanup_channel(dev, channel);
		return ret;
	}

	return 0;
}

static void infineon_dmac_isr(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;
	struct infineon_dmac_data *data = dev->data;
	struct infineon_dmac_channel *ch = data->channels + channel;
	dma_callback_t callback = ch->user_cb;
	void *user_data = ch->user_data;
	cy_en_dmac_response_t response;
	cy_en_dmac_descriptor_t completed_desc;
	cy_en_dmac_descriptor_t reconfigure_desc;
	int ret;
	int status = -EIO;

	/* For cyclic transfers, cpltState (INV_DESCR) invalidates PING on
	 * completion.  GetCurrentDescriptor would then return PONG (which
	 * was never executed), so force PING as the completed descriptor.
	 */
	if (ch->cyclic) {
		completed_desc = CY_DMAC_DESCRIPTOR_PING;
	} else {
		completed_desc = Cy_DMAC_Channel_GetCurrentDescriptor(config->base, channel);
	}

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

	/* block_transferred has to be incremented here to handle per block callback */
	ch->blocks_transferred++;

	/* Callback only if registered */
	if (callback != NULL) {
		/* Return error callback if enabled */
		if (status != 0) {
			/* Channel clean up should be happen before callbacks if error */
			infineon_dmac_cleanup_channel(dev, channel);

			/* error callback */
			if (!ch->error_callback_dis) {
				callback(dev, user_data, channel, status);
			}
			return;
		}

		/* Cyclic: descriptor was invalidated by hardware (cpltState).
		 * Report block completion; resume() will re-arm.
		 */
		if (ch->cyclic) {
			callback(dev, user_data, channel, DMA_STATUS_BLOCK);
			return;
		}

		/* Handle per block callback */
		if (ch->blocks_transferred < ch->total_blocks) {
			/* Still have blocks to transfer (BLOCK completion call back if enabled) */
			if (ch->complete_callback_en) {
				callback(dev, user_data, channel, DMA_STATUS_BLOCK);
			}
		} else {
			/* All blocks transferred (DMA completion call back) */
			callback(dev, user_data, channel, DMA_STATUS_COMPLETE);
		}
	}

	/* if Callback not registered */
	if (status != 0) {
		infineon_dmac_cleanup_channel(dev, channel);
		return;
	}

	/* Handle multi-block transfers (2+ blocks), handle chaining */
	if (ch->total_blocks >= 2 && ch->blocks_transferred < ch->total_blocks) {
		/* Get next block from copied array */
		ch->current_block_index++;
		if (ch->current_block_index < ch->total_blocks) {
			struct dma_block_config *block_to_exec =
				ch->block_copies + ch->current_block_index;

			if (completed_desc == CY_DMAC_DESCRIPTOR_PING) {
				reconfigure_desc = CY_DMAC_DESCRIPTOR_PONG;
			} else {
				reconfigure_desc = CY_DMAC_DESCRIPTOR_PING;
			}

			ret = infineon_dmac_cfg_desc(dev, channel, reconfigure_desc, block_to_exec,
						     ch->direction, ch->source_data_size,
						     ch->dest_data_size, false);

			if (ret == 0) {
				if (ch->direction == MEMORY_TO_MEMORY ||
				    ch->direction == MEMORY_TO_PERIPHERAL) {
					infineon_dmac_sw_trigger();
				}
			} else {
				LOG_ERR("Descriptor initialization fails for next block");
				infineon_dmac_cleanup_channel(dev, channel);

				/* error callback for next block configuration */
				if (callback != NULL && !ch->error_callback_dis) {
					callback(dev, user_data, channel, ret);
				}
			}
		}
	}

	if (ch->blocks_transferred == ch->total_blocks) {
		infineon_dmac_cleanup_channel(dev, channel);
	}
}

static void infineon_dmac_shared_isr(const struct device *dev)
{
	const struct infineon_dmac_config *config = dev->config;
	uint32_t intr_status = Cy_DMAC_GetInterruptStatusMasked(config->base);

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

	if (infineon_dmac_channel_is_busy(dev, channel)) {
		LOG_WRN("DMA channel %u already running", channel);
		return 0;
	}

	ch->blocks_transferred = 0;
	ch->current_block_index = 0;

	Cy_DMAC_Enable(config->base);
	current_mask = Cy_DMAC_GetInterruptMask(config->base);
	Cy_DMAC_SetInterruptMask(config->base, current_mask | BIT(channel));
	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));
	Cy_DMAC_Channel_Enable(config->base, channel);

	if (ch->direction == MEMORY_TO_MEMORY || ch->direction == MEMORY_TO_PERIPHERAL) {
		return infineon_dmac_sw_trigger();
	}

	return 0;
}

static int infineon_dmac_stop(const struct device *dev, uint32_t channel)
{
	const struct infineon_dmac_config *config = dev->config;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number %u", channel);

	Cy_DMAC_Channel_Disable(config->base, channel);
	Cy_DMAC_ClearInterrupt(config->base, BIT(channel));

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
	struct infineon_dmac_channel *ch = data->channels + channel;

	__ASSERT((channel < config->num_channels), "Invalid DMA channel number");

	if (infineon_dmac_channel_is_busy(dev, channel)) {
		LOG_WRN("DMA channel %u already running", channel);
		return 0;
	}

	/* For cyclic transfers, cpltState (INV_DESCR) invalidated PING
	 * on completion.  Re-validate it (which also clears CURR_DATA_NR
	 * so the transfer restarts from element 0) and set it as current.
	 */
	if (ch->cyclic) {
		Cy_DMAC_Descriptor_SetState(config->base, channel, CY_DMAC_DESCRIPTOR_PING, true);
		Cy_DMAC_Channel_SetCurrentDescriptor(config->base, channel,
						     CY_DMAC_DESCRIPTOR_PING);
	}

	Cy_DMAC_Channel_Enable(config->base, channel);

	if (ch->direction == MEMORY_TO_MEMORY || ch->direction == MEMORY_TO_PERIPHERAL) {
		return infineon_dmac_sw_trigger();
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
	ch->current_block_index = 0;

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

static DEVICE_API(dma, infineon_dmac_api) = {
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

#define INFINEON_DMAC_INIT(n)                                                                      \
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

DT_INST_FOREACH_STATUS_OKAY(INFINEON_DMAC_INIT)
