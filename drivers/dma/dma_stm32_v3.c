/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dma_stm32_v3.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma/dma_stm32.h>

#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v3, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT            st_stm32_dma_v3
#define DMA_STM32_MAX_DATA_ITEMS 0xffff

/* Since the descriptors pool is allocated statically, we define the number of
 * descriptors per channel to be used for linked list trasnfers.
 */
#define DMA_STM32_NUM_DESCRIPTORS_PER_CHANNEL 24

static const uint32_t table_src_size[] = {
	LL_DMA_SRC_DATAWIDTH_BYTE,
	LL_DMA_SRC_DATAWIDTH_HALFWORD,
	LL_DMA_SRC_DATAWIDTH_WORD,
	LL_DMA_SRC_DATAWIDTH_DOUBLEWORD,
};

static const uint32_t table_dst_size[] = {
	LL_DMA_DEST_DATAWIDTH_BYTE,
	LL_DMA_DEST_DATAWIDTH_HALFWORD,
	LL_DMA_DEST_DATAWIDTH_WORD,
	LL_DMA_DEST_DATAWIDTH_DOUBLEWORD,
};

static const uint32_t table_priority[] = {
	LL_DMA_LOW_PRIORITY_LOW_WEIGHT,
	LL_DMA_LOW_PRIORITY_MID_WEIGHT,
	LL_DMA_LOW_PRIORITY_HIGH_WEIGHT,
	LL_DMA_HIGH_PRIORITY,
};

uint32_t dma_stm32_id_to_channel(uint32_t id)
{
	static const uint32_t channel_nr[] = {
		LL_DMA_CHANNEL_0,  LL_DMA_CHANNEL_1,  LL_DMA_CHANNEL_2,  LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,  LL_DMA_CHANNEL_5,  LL_DMA_CHANNEL_6,  LL_DMA_CHANNEL_7,
		LL_DMA_CHANNEL_8,  LL_DMA_CHANNEL_9,  LL_DMA_CHANNEL_10, LL_DMA_CHANNEL_11,
		LL_DMA_CHANNEL_12, LL_DMA_CHANNEL_13, LL_DMA_CHANNEL_14, LL_DMA_CHANNEL_15,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(channel_nr));

	return channel_nr[id];
}

static int dma_stm32_get_src_inc_mode(enum dma_addr_adj increment, uint32_t *ll_increment)
{
	switch (increment) {
	case DMA_ADDR_ADJ_INCREMENT:
		*ll_increment = LL_DMA_SRC_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*ll_increment = LL_DMA_SRC_FIXED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		LOG_ERR("Decrement mode not supported for source address adjustment");
		return -ENOTSUP;
	default:
		LOG_ERR("Invalid source increment mode: %d", increment);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_get_dest_inc_mode(enum dma_addr_adj increment, uint32_t *ll_increment)
{
	switch (increment) {
	case DMA_ADDR_ADJ_INCREMENT:
		*ll_increment = LL_DMA_DEST_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*ll_increment = LL_DMA_DEST_FIXED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		LOG_ERR("Decrement mode not supported for destination address adjustment");
		return -ENOTSUP;
	default:
		LOG_ERR("Invalid destination increment mode: %d", increment);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_get_ll_direction(enum dma_channel_direction direction, uint32_t *ll_direction)
{
	switch (direction) {
	case MEMORY_TO_MEMORY:
		*ll_direction = LL_DMA_DIRECTION_MEMORY_TO_MEMORY;
		break;
	case MEMORY_TO_PERIPHERAL:
		*ll_direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
		break;
	case PERIPHERAL_TO_MEMORY:
		*ll_direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
		break;
	default:
		LOG_ERR("Direction error. %d", direction);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_get_priority(uint8_t priority, uint32_t *ll_priority)
{
	if (priority > ARRAY_SIZE(table_priority)) {
		LOG_ERR("Priority error.");
		return -EINVAL;
	}

	*ll_priority = table_priority[priority];
	return 0;
}

static bool dma_stm32_is_tc_irq_active(const DMA_TypeDef *dma, uint32_t channel)
{
	return LL_DMA_IsEnabledIT_TC(dma, channel) && LL_DMA_IsActiveFlag_TC(dma, channel);
}

static bool dma_stm32_is_ht_irq_active(const DMA_TypeDef *dma, uint32_t channel)
{
	return LL_DMA_IsEnabledIT_HT(dma, channel) && LL_DMA_IsActiveFlag_HT(dma, channel);
}

static void dma_stm32_disable_it(const DMA_TypeDef *dma, uint32_t channel)
{
	LL_DMA_DisableIT_TC(dma, channel);
	LL_DMA_DisableIT_HT(dma, channel);
	LL_DMA_DisableIT_USE(dma, channel);
	LL_DMA_DisableIT_ULE(dma, channel);
	LL_DMA_DisableIT_DTE(dma, channel);
	LL_DMA_DisableIT_SUSP(dma, channel);
}

void dma_stm32_dump_channel_irq(const DMA_TypeDef *dma, uint32_t channel)
{
	LOG_INF("tc: %d, ht: %d, dte: %d, ule: %d, use: %d", LL_DMA_IsActiveFlag_TC(dma, channel),
		LL_DMA_IsActiveFlag_HT(dma, channel), LL_DMA_IsActiveFlag_DTE(dma, channel),
		LL_DMA_IsActiveFlag_ULE(dma, channel), LL_DMA_IsActiveFlag_USE(dma, channel));
}

void dma_stm32_clear_channel_irq(const DMA_TypeDef *dma, uint32_t channel)
{
	LL_DMA_ClearFlag_TC(dma, channel);
	LL_DMA_ClearFlag_HT(dma, channel);
	LL_DMA_ClearFlag_USE(dma, channel);
	LL_DMA_ClearFlag_ULE(dma, channel);
	LL_DMA_ClearFlag_DTE(dma, channel);
	LL_DMA_ClearFlag_TO(dma, channel);
	LL_DMA_ClearFlag_SUSP(dma, channel);
}

int dma_stm32_disable_channel(const DMA_TypeDef *dma, uint32_t channel)
{
	uint32_t timeout = 10;

	LL_DMA_DisableChannel(dma, channel);
	while (timeout-- > 0) {
		if (!LL_DMA_IsEnabledChannel(dma, channel)) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	}

	LOG_ERR("Timeout waiting for channel %d to disable", channel);
	return -ETIMEDOUT;
}

static int dma_stm32_validate_arguments(const struct device *dev, struct dma_config *config)
{
	if (dev == NULL || config == NULL) {
		LOG_ERR("Invalid arguments: dev=%p, config=%p", dev, config);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_validate_transfer_sizes(struct dma_config *config)
{
	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		LOG_ERR("Data size exceeds the maximum limit: %d>%d",
			config->head_block->block_size, DMA_STM32_MAX_DATA_ITEMS);
		return -EINVAL;
	}

	if (config->source_data_size != 8U && config->source_data_size != 4U &&
	    config->source_data_size != 2U && config->source_data_size != 1U) {
		LOG_ERR("Invalid source data size: %u, only 1, 2, 4, 8 bytes supported",
			config->source_data_size);
		return -EINVAL;
	}
	LOG_DBG("Source data size: %u", config->source_data_size);

	if (config->dest_data_size != 8U && config->dest_data_size != 4U &&
	    config->dest_data_size != 2U && config->dest_data_size != 1U) {
		LOG_ERR("Invalid destination data size: %u, only 1, 2, 4, 8 bytes supported",
			config->dest_data_size);
		return -EINVAL;
	}
	LOG_DBG("Destination data size: %u", config->dest_data_size);

	/* For starters, we'll only support the same data size */
	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source and destination data sizes do not match: %u != %u",
			config->source_data_size, config->dest_data_size);
		return -ENOTSUP; /* TODO: support different sizes */
	}

	return 0;
}

static void dma_stm32_configure_linked_list(uint32_t id, struct dma_config *config,
					    volatile uint32_t *linked_list_node,
					    const DMA_TypeDef *dma)
{
	uint32_t next_desc = 1;
	struct dma_block_config *block_config;

	uint32_t registers_update = 0;
	uint32_t addr_offset = 0;

	uint32_t descriptor_index = 0;
	uint32_t base_addr = 0;
	uint32_t next_desc_addr = 0;

	uint32_t channel = dma_stm32_id_to_channel(id);
	block_config = config->head_block;
	base_addr = (uint32_t)&linked_list_node[descriptor_index];
	LL_DMA_SetLinkedListBaseAddr(dma, channel, base_addr);

	for (int i = 0; i < config->block_count; i++) {
		registers_update = 0;
		LOG_DBG("Configuring block descriptor %d for channel %d", i, channel);

		linked_list_node[descriptor_index] = block_config->source_address;
		descriptor_index++;
		linked_list_node[descriptor_index] = block_config->dest_address;
		descriptor_index++;

		if (i < config->block_count - 1) {
			registers_update |=
				LL_DMA_UPDATE_CSAR | LL_DMA_UPDATE_CDAR | LL_DMA_UPDATE_CLLR;
			block_config = block_config->next_block;
			next_desc_addr = (uint32_t)&linked_list_node[descriptor_index + 1];
		} else if (config->cyclic) {
			LOG_DBG("Last descriptor %d for channel %d, linking to first", i, channel);
			registers_update |=
				LL_DMA_UPDATE_CSAR | LL_DMA_UPDATE_CDAR | LL_DMA_UPDATE_CLLR;
			next_desc_addr = base_addr;
		} else {
			LOG_DBG("Last descriptor %d for channel %d, no link", i, channel);
			registers_update = 0;
			next_desc = 0;
		}

		if (next_desc != 0) {
			addr_offset = next_desc_addr & GENMASK(15, 2);
			registers_update |= addr_offset;
		}

		linked_list_node[descriptor_index] = registers_update;
		descriptor_index++;

		if (i == 0) {
			LL_DMA_ConfigLinkUpdate(dma, channel, registers_update, addr_offset);
		}
	}

	LL_DMA_EnableIT_HT(dma, channel);
}

int dma_stm32_configure(const struct device *dev, uint32_t id, struct dma_config *config)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	struct dma_stm32_descriptor hwdesc;
	uint32_t channel;
	const DMA_TypeDef *dma;
	uint32_t src_inc_mode;
	uint32_t dest_inc_mode;
	uint32_t ll_priority;
	uint32_t ll_direction;
	int src_index;
	int dest_index;
	int ret = 0;
	uint32_t ccr = 0;

	ret = dma_stm32_validate_arguments(dev, config);
	if (ret < 0) {
		return ret;
	}

	dev_config = dev->config;
	if (id >= dev_config->max_channels) {
		LOG_ERR("Invalid channel ID %d, max channels %d", id, dev_config->max_channels);
		return -EINVAL;
	}

	channel_config = &dev_config->channels[id];
	if (channel_config->busy) {
		LOG_ERR("Channel %d is busy", id);
		return -EBUSY;
	}

	channel = dma_stm32_id_to_channel(id);
	dma = dev_config->base;
	if (dma_stm32_disable_channel(dma, channel) != 0) {
		LOG_ERR("Failed to disable DMA channel %d", id);
		return -EBUSY;
	}

	dma_stm32_clear_channel_irq(dma, channel);

	ret = dma_stm32_validate_transfer_sizes(config);
	if (ret < 0) {
		return ret;
	}

	ret = dma_stm32_get_src_inc_mode(config->head_block->source_addr_adj, &src_inc_mode);
	if (ret < 0) {
		return ret;
	}
	LOG_DBG("Source address increment: %d", src_inc_mode);

	ret = dma_stm32_get_dest_inc_mode(config->head_block->dest_addr_adj, &dest_inc_mode);
	if (ret < 0) {
		return ret;
	}
	LOG_DBG("Destination address increment: %d", dest_inc_mode);

	ret = dma_stm32_get_priority(config->channel_priority, &ll_priority);
	if (ret < 0) {
		return ret;
	}

	ret = dma_stm32_get_ll_direction(config->channel_direction, &ll_direction);
	if (ret < 0) {
		return ret;
	}

	channel_config->busy = true;
	channel_config->dma_callback = config->dma_callback;
	channel_config->direction = config->channel_direction;
	channel_config->user_data = config->user_data;
	channel_config->src_size = config->source_data_size;
	channel_config->dst_size = config->dest_data_size;
	channel_config->cyclic = config->cyclic;

	src_index = LOG2(channel_config->src_size);
	dest_index = LOG2(channel_config->dst_size);

	dma_stm32_disable_it(dma, channel);

	if (config->block_count == 1 && !config->cyclic) {
		/* DMAx CCR Configuration */
		ccr |= LL_DMA_LSM_1LINK_EXECUTION;
	} else {
		/* DMAx CCR Configuration */
		ccr |= LL_DMA_LSM_FULL_EXECUTION;

		volatile uint32_t *linked_list_node =
			&dev_config->linked_list_buffer[id * DMA_STM32_NUM_DESCRIPTORS_PER_CHANNEL];
		dma_stm32_configure_linked_list(id, config, linked_list_node, dma);
	}

	/* DMAx CCR Configuration */
	ccr |= ll_priority;
	ccr |= LL_DMA_LINK_ALLOCATED_PORT0;

	LL_DMA_ConfigControl(dma, channel, ccr);

	/* DMAx CTR1 Configuration */
	hwdesc.channel_tr1 = 0;
	hwdesc.channel_tr1 |= dest_inc_mode;
	hwdesc.channel_tr1 |= table_dst_size[dest_index];
	hwdesc.channel_tr1 |= src_inc_mode;
	hwdesc.channel_tr1 |= table_src_size[src_index];

	LL_DMA_ConfigTransfer(dma, channel, hwdesc.channel_tr1);

	LL_DMA_ConfigBurstLength(dma, channel, config->source_burst_length,
				 config->dest_burst_length);

	/* DMAx CTR2 Configuration */
	hwdesc.channel_tr2 = 0;
	hwdesc.channel_tr2 |= ll_direction;
	hwdesc.channel_tr2 |= config->complete_callback_en ? LL_DMA_TCEM_BLK_TRANSFER
							   : LL_DMA_TCEM_EACH_LLITEM_TRANSFER;

	LL_DMA_ConfigChannelTransfer(dma, channel, hwdesc.channel_tr2);

	if (ll_direction != LL_DMA_DIRECTION_MEMORY_TO_MEMORY) {
		LL_DMA_SetPeriphRequest(dma, channel, config->dma_slot);
	}

	/* DMAx CBR1 Configuration */
	hwdesc.channel_br1 = config->head_block->block_size;

	LL_DMA_SetBlkDataLength(dma, channel, hwdesc.channel_br1);

	/* DMAx CSAR and CDAR Configuration*/
	hwdesc.channel_sar = config->head_block->source_address;
	hwdesc.channel_dar = config->head_block->dest_address;

	LL_DMA_ConfigAddresses(dma, channel, hwdesc.channel_sar, hwdesc.channel_dar);

	/* Enable various interrupts */
	LL_DMA_EnableIT_TC(dma, channel);
	LL_DMA_EnableIT_USE(dma, channel);
	LL_DMA_EnableIT_ULE(dma, channel);
	LL_DMA_EnableIT_DTE(dma, channel);

	return 0;
}

int dma_stm32_reload(const struct device *dev, uint32_t id, uint32_t src, uint32_t dst, size_t size)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	const DMA_TypeDef *dma;

	if (dev == NULL) {
		LOG_ERR("Invalid device pointer");
		return -EINVAL;
	}

	dev_config = dev->config;

	if (id >= dev_config->max_channels) {
		LOG_ERR("Invalid channel ID %d, max channels %d", id, dev_config->max_channels);
		return -EINVAL;
	}

	channel_config = &dev_config->channels[id];
	dma = dev_config->base;
	channel = dma_stm32_id_to_channel(id);

	if (dma_stm32_disable_channel(dma, channel) != 0) {
		return -EBUSY;
	}

	if (channel_config->direction > PERIPHERAL_TO_MEMORY) {
		LOG_ERR("Invalid DMA direction %d", channel_config->direction);
		return -EINVAL;
	}

	LL_DMA_ConfigAddresses(dma, channel, src, dst);

	LL_DMA_SetBlkDataLength(dma, channel, size);

	channel_config->busy = true;

	LL_DMA_EnableChannel(dma, channel);

	return 0;
}

int dma_stm32_start(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel = dma_stm32_id_to_channel(id);
	const DMA_TypeDef *dma = dev_config->base;

	if (LL_DMA_IsEnabledChannel(dma, channel)) {
		LOG_INF("Channel %d is already enabled", id);
		return 0;
	}

	/* When starting the dma, the stream is busy before enabling */
	channel_config = &dev_config->channels[id];
	channel_config->busy = true;

	dma_stm32_clear_channel_irq(dma, channel);

	LL_DMA_EnableChannel(dma, channel);

	return 0;
}

int dma_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_channel *channel_config = &dev_config->channels[id];
	uint32_t channel = dma_stm32_id_to_channel(id);
	const DMA_TypeDef *dma = dev_config->base;

	if (channel_config->hal_override) {
		channel_config->busy = false;
		return 0;
	}

	if (!LL_DMA_IsEnabledChannel(dma, channel)) {
		return 0;
	}

	dma_stm32_clear_channel_irq(dma, channel);

	dma_stm32_disable_it(dma, channel);

	if (dma_stm32_disable_channel(dma, channel)) {
		LOG_ERR("Failed to disable DMA channel %d", id);
		return -EBUSY;
	}

	channel_config->busy = false;

	return 0;
}

int dma_stm32_get_status(const struct device *dev, uint32_t id, struct dma_status *status)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel = dma_stm32_id_to_channel(id);
	const DMA_TypeDef *dma = dev_config->base;

	if (id >= dev_config->max_channels) {
		return -EINVAL;
	}

	channel_config = &dev_config->channels[id];

	status->pending_length = LL_DMA_GetBlkDataLength(dma, channel);
	status->dir = channel_config->direction;
	status->busy = channel_config->busy;

	return 0;
}

int dma_stm32_suspend(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config = dev->config;
	uint32_t channel = dma_stm32_id_to_channel(id);
	const DMA_TypeDef *dma = dev_config->base;
	uint32_t timeout = 10;

	if (id >= dev_config->max_channels) {
		return -EINVAL;
	}

	LL_DMA_SuspendChannel(dma, channel);
	while (timeout-- > 0) {
		if (LL_DMA_IsActiveFlag_SUSP(dma, channel)) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	}

	LOG_ERR("Timeout waiting for channel %d to suspend", channel);
	return -ETIMEDOUT;
}

int dma_stm32_resume(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config = dev->config;
	uint32_t channel;
	const DMA_TypeDef *dma;

	if (id >= dev_config->max_channels) {
		return -EINVAL;
	}

	channel = dma_stm32_id_to_channel(id);
	dma = dev_config->base;

	LL_DMA_ResumeChannel(dma, channel);

	return 0;
}

static void dma_stm32_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	const DMA_TypeDef *dma;
	uint32_t callback_arg;

	if (id >= dev_config->max_channels) {
		return;
	}

	channel_config = &dev_config->channels[id];
	channel = dma_stm32_id_to_channel(id);
	dma = dev_config->base;

	/* The busy channel is pertinent if not overridden by the HAL */
	if ((channel_config->hal_override != true) && (channel_config->busy == false)) {
		/*
		 * When DMA channel is not overridden by HAL,
		 * ignore irq if the channel is not busy anymore
		 */
		dma_stm32_clear_channel_irq(dma, channel);
		return;
	}
	callback_arg = id;

	if (dma_stm32_is_ht_irq_active(dma, channel)) {
		if (!channel_config->hal_override) {
			LL_DMA_ClearFlag_HT(dma, channel);
		}

		channel_config->dma_callback(dev, channel_config->user_data, callback_arg,
					     DMA_STATUS_BLOCK);
	} else if (dma_stm32_is_tc_irq_active(dma, channel)) {
		if (channel_config->cyclic == false) {
			channel_config->busy = false;
		}

		if (!channel_config->hal_override) {
			LL_DMA_ClearFlag_TC(dma, channel);
		}

		channel_config->dma_callback(dev, channel_config->user_data, callback_arg,
					     DMA_STATUS_COMPLETE);
	} else {
		LOG_ERR("Transfer Error.");
		channel_config->busy = false;
		dma_stm32_dump_channel_irq(dma, channel);
		dma_stm32_clear_channel_irq(dma, channel);
		channel_config->dma_callback(dev, channel_config->user_data, callback_arg, -EIO);
	}
}

static int dma_stm32_init(const struct device *dev)
{
	const struct dma_stm32_config *dev_config = dev->config;

	for (uint32_t i = 0; i < dev_config->max_channels; i++) {
		dev_config->channels[i].busy = false;
	}

	((struct dma_stm32_data *)dev->data)->dma_ctx.magic = 0;
	((struct dma_stm32_data *)dev->data)->dma_ctx.dma_channels = 0;
	((struct dma_stm32_data *)dev->data)->dma_ctx.atomic = 0;

	dev_config->config_irq(dev);

	return 0;
}

static DEVICE_API(dma, dma_funcs) = {
	.config = dma_stm32_configure,
	.reload = dma_stm32_reload,
	.start = dma_stm32_start,
	.stop = dma_stm32_stop,
	.get_status = dma_stm32_get_status,
	.suspend = dma_stm32_suspend,
	.resume = dma_stm32_resume,
};

#define DMA_STM32_IRQ_CONNECT_CHANNEL(chan, dma)                                                   \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(dma, chan, irq),                                    \
			    DT_INST_IRQ_BY_IDX(dma, chan, priority), dma_stm32_irq_##dma##_##chan, \
			    DEVICE_DT_INST_GET(dma), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_IDX(dma, chan, irq));                                    \
	} while (0)

#define DMA_STM32_IRQ_CONNECT(index)                                                               \
	static void dma_stm32_config_irq_##index(const struct device *dev)                         \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		LISTIFY(DT_INST_PROP(index, dma_channels),			\
		DMA_STM32_IRQ_CONNECT_CHANNEL, (;), index); \
	}

#define DMA_STM32_DEFINE_IRQ_HANDLER(chan, dma)                                                    \
	static void dma_stm32_irq_##dma##_##chan(const struct device *dev)                         \
	{                                                                                          \
		dma_stm32_irq_handler(dev, chan);                                                  \
	}

#define DMA_STM32_INIT_DEV(index)                                                                  \
	BUILD_ASSERT(DT_INST_PROP(index, dma_channels) == DT_NUM_IRQS(DT_DRV_INST(index)),         \
		     "Nb of Channels and IRQ mismatch");                                           \
                                                                                                   \
	LISTIFY(DT_INST_PROP(index, dma_channels),				\
	DMA_STM32_DEFINE_IRQ_HANDLER, (;), index);          \
                                                                                                   \
	DMA_STM32_IRQ_CONNECT(index);                                                              \
                                                                                                   \
	static struct dma_stm32_channel dma_stm32_channels_##index[DT_INST_PROP_OR(                \
		index, dma_channels, DT_NUM_IRQS(DT_DRV_INST(index)))];                            \
                                                                                                   \
	static volatile uint32_t dma_stm32_linked_list_buffer##index                               \
		[DMA_STM32_NUM_DESCRIPTORS_PER_CHANNEL *                                           \
		 DT_INST_PROP_OR(index, dma_channels, DT_NUM_IRQS(DT_DRV_INST(index)))] __nocache; \
                                                                                                   \
	const struct dma_stm32_config dma_stm32_config_##index = {COND_CODE_1(DT_NODE_HAS_PROP(__node, clocks),		       \
				(.pclken = { .bus = __bus, \
					 .enr = __cenr },),       \
				(/* Nothing if clocks not present */)) .config_irq =                              \
			  dma_stm32_config_irq_##index,                                            \
		 .base = (DMA_TypeDef *)DT_INST_REG_ADDR(index),                                   \
		 .max_channels =                                                                   \
			 DT_INST_PROP_OR(index, dma_channels, DT_NUM_IRQS(DT_DRV_INST(index))),    \
		 .channels = dma_stm32_channels_##index,                                           \
		 .linked_list_buffer = dma_stm32_linked_list_buffer##index};                       \
	static struct dma_stm32_data dma_stm32_data_##index = {};                                  \
	DEVICE_DT_INST_DEFINE(index, dma_stm32_init, NULL, &dma_stm32_data_##index,                \
			      &dma_stm32_config_##index, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_STM32_INIT_DEV)
