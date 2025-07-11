/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_ll_dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v3, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT            st_stm32_dma_v3
#define DMA_STM32_MAX_DATA_ITEMS 0xffff

/* Since at this point we only support cyclic mode , we only need 3 descriptors
 * at most to update the source and destination addresses and the update
 * registers. TODO: Raise this number for larger linked lists.
 */
#define DMA_STM32_NUM_DESCRIPTORS_PER_CHANNEL 24
#define POLLING_TIMEOUT_US                    (10 * USEC_PER_MSEC)

struct dma_stm32_descriptor {
	uint32_t channel_tr1;
	uint32_t channel_tr2;
	uint32_t channel_br1;
	uint32_t channel_sar;
	uint32_t channel_dar;
	uint32_t channel_llr;
};

struct dma_stm32_channel {
	uint32_t direction;
	bool hal_override;
	bool busy;
	uint32_t state;
	uint32_t src_size;
	uint32_t dst_size;
	void *user_data;
	dma_callback_t dma_callback;
	bool cyclic;
	int block_count;
};

struct dma_stm32_data {
	struct dma_context dma_ctx;
};

struct dma_stm32_config {
	struct stm32_pclken pclken;
	void (*config_irq)(const struct device *dev);
	DMA_TypeDef *base;
	uint32_t max_channels;
	struct dma_stm32_channel *channels;
	uint32_t *linked_list_buffer;
};

static int dma_stm32_id_to_channel(uint32_t id, uint32_t *channel)
{
	static const uint32_t channel_nr[] = {
		LL_DMA_CHANNEL_0,  LL_DMA_CHANNEL_1,  LL_DMA_CHANNEL_2,  LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,  LL_DMA_CHANNEL_5,  LL_DMA_CHANNEL_6,  LL_DMA_CHANNEL_7,
		LL_DMA_CHANNEL_8,  LL_DMA_CHANNEL_9,  LL_DMA_CHANNEL_10, LL_DMA_CHANNEL_11,
		LL_DMA_CHANNEL_12, LL_DMA_CHANNEL_13, LL_DMA_CHANNEL_14, LL_DMA_CHANNEL_15,
	};

	if (id >= ARRAY_SIZE(channel_nr)) {
		LOG_ERR("Invalid channel ID %d", id);
		return -EINVAL;
	}

	*channel = channel_nr[id];
	return 0;
}

static DMA_Channel_TypeDef *dma_stm32_get_channel_addr(DMA_TypeDef *dma, uint32_t channel)
{
	return (DMA_Channel_TypeDef *)((uintptr_t)dma + LL_DMA_CH_OFFSET_TAB[channel]);
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

static void dma_stm32_get_src_data_width(uint32_t size, uint32_t *ll_src_data_width)
{
	static const uint32_t table_src_size[] = {
		LL_DMA_SRC_DATAWIDTH_BYTE,
		LL_DMA_SRC_DATAWIDTH_HALFWORD,
		LL_DMA_SRC_DATAWIDTH_WORD,
		LL_DMA_SRC_DATAWIDTH_DOUBLEWORD,
	};

	*ll_src_data_width = table_src_size[LOG2(size)];
}

static void dma_stm32_get_dest_data_width(uint32_t size, uint32_t *ll_dest_data_width)
{
	static const uint32_t table_dst_size[] = {
		LL_DMA_DEST_DATAWIDTH_BYTE,
		LL_DMA_DEST_DATAWIDTH_HALFWORD,
		LL_DMA_DEST_DATAWIDTH_WORD,
		LL_DMA_DEST_DATAWIDTH_DOUBLEWORD,
	};

	*ll_dest_data_width = table_dst_size[LOG2(size)];
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
	static const uint32_t table_priority[] = {
		LL_DMA_LOW_PRIORITY_LOW_WEIGHT,
		LL_DMA_LOW_PRIORITY_MID_WEIGHT,
		LL_DMA_LOW_PRIORITY_HIGH_WEIGHT,
		LL_DMA_HIGH_PRIORITY,
	};

	if (priority >= ARRAY_SIZE(table_priority)) {
		LOG_ERR("Priority error.");
		return -EINVAL;
	}

	*ll_priority = table_priority[priority];
	return 0;
}

static bool dma_stm32_is_tc_irq_active(DMA_TypeDef *dma, uint32_t channel)
{
	return LL_DMA_IsEnabledIT_TC(dma, channel) && LL_DMA_IsActiveFlag_TC(dma, channel);
}

static bool dma_stm32_is_ht_irq_active(DMA_TypeDef *dma, uint32_t channel)
{
	return LL_DMA_IsEnabledIT_HT(dma, channel) && LL_DMA_IsActiveFlag_HT(dma, channel);
}

static void dma_stm32_disable_it(DMA_TypeDef *dma, uint32_t channel)
{
	mem_addr_t dma_channel_ccr_reg = (mem_addr_t)&dma_stm32_get_channel_addr(dma, channel)->CCR;

	sys_clear_bits(dma_channel_ccr_reg, DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_USEIE |
						    DMA_CCR_ULEIE | DMA_CCR_DTEIE | DMA_CCR_SUSPIE);
}

static void dma_stm32_enable_it(DMA_TypeDef *dma, uint32_t channel)
{
	mem_addr_t dma_channel_ccr_reg = (mem_addr_t)&dma_stm32_get_channel_addr(dma, channel)->CCR;

	sys_set_bits(dma_channel_ccr_reg,
		     DMA_CCR_TCIE | DMA_CCR_USEIE | DMA_CCR_ULEIE | DMA_CCR_DTEIE);
}

static void dma_stm32_dump_channel_irq(DMA_TypeDef *dma, uint32_t channel)
{
	LOG_INF("tc: %d, ht: %d, dte: %d, ule: %d, use: %d", LL_DMA_IsActiveFlag_TC(dma, channel),
		LL_DMA_IsActiveFlag_HT(dma, channel), LL_DMA_IsActiveFlag_DTE(dma, channel),
		LL_DMA_IsActiveFlag_ULE(dma, channel), LL_DMA_IsActiveFlag_USE(dma, channel));
}

static void dma_stm32_clear_channel_irq(DMA_TypeDef *dma, uint32_t channel)
{
	mem_addr_t dma_channel_cfcr_reg =
		(mem_addr_t)&dma_stm32_get_channel_addr(dma, channel)->CFCR;

	sys_set_bits(dma_channel_cfcr_reg, DMA_CFCR_TCF | DMA_CFCR_HTF | DMA_CFCR_DTEF |
						   DMA_CFCR_ULEF | DMA_CFCR_USEF | DMA_CFCR_TOF |
						   DMA_CFCR_SUSPF);
}

static int dma_stm32_disable_channel(DMA_TypeDef *dma, uint32_t channel)
{
	LL_DMA_DisableChannel(dma, channel);
	if (WAIT_FOR(!LL_DMA_IsEnabledChannel(dma, channel), POLLING_TIMEOUT_US, k_msleep(1))) {
		return 0;
	}

	LOG_ERR("Timeout waiting for channel %d to disable", channel);
	return -ETIMEDOUT;
}

static int dma_stm32_validate_transfer_sizes(struct dma_config *config)
{
	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		LOG_ERR("Data size exceeds the maximum limit: %d>%d", config->head_block->block_size,
			DMA_STM32_MAX_DATA_ITEMS);
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

	/* TODO: support different data sizes */
	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source and destination data sizes do not match: (%u != %u) - not "
			"supported yet",
			config->source_data_size, config->dest_data_size);
		return -ENOTSUP;
	}

	return 0;
}

int dma_stm32_configure(const struct device *dev, uint32_t id, struct dma_config *config)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	struct dma_block_config *block_config;
	struct dma_stm32_descriptor hwdesc;
	uint32_t channel;
	DMA_TypeDef *dma;
	uint32_t src_inc_mode;
	uint32_t dest_inc_mode;
	uint32_t src_data_width_size;
	uint32_t dest_data_width_size;
	uint32_t ll_priority;
	uint32_t ll_direction;
	int ret = 0;
	uint32_t ccr = 0;

	if (dev == NULL || config == NULL) {
		LOG_ERR("Invalid arguments: dev=%p, config=%p", dev, config);
		return -EINVAL;
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

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

	dma = dev_config->base;

	ret = dma_stm32_disable_channel(dma, channel);
	if (ret < 0) {
		LOG_ERR("Failed to disable DMA channel %d", id);
		return ret;
	}

	dma_stm32_clear_channel_irq(dma, channel);

	ret = dma_stm32_validate_transfer_sizes(config);
	if (ret < 0) {
		return ret;
	}

	block_config = config->head_block;

	ret = dma_stm32_get_src_inc_mode(block_config->source_addr_adj, &src_inc_mode);
	if (ret < 0) {
		return ret;
	}
	LOG_DBG("Source address increment: %d", src_inc_mode);

	ret = dma_stm32_get_dest_inc_mode(block_config->dest_addr_adj, &dest_inc_mode);
	if (ret < 0) {
		return ret;
	}
	LOG_DBG("Destination address increment: %d", dest_inc_mode);

	dma_stm32_get_src_data_width(config->source_data_size, &src_data_width_size);
	dma_stm32_get_dest_data_width(config->dest_data_size, &dest_data_width_size);

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

	dma_stm32_disable_it(dma, channel);

	/* Reset any previous linked list configuration */
	LL_DMA_SetLinkedListBaseAddr(dma, channel, 0);

	int linked_list_needed = (config->block_count > 1) || config->cyclic;

	if (!linked_list_needed) {
		ccr |= LL_DMA_LSM_1LINK_EXECUTION;
	} else {
		LOG_ERR("Only single block transfers are supported for now");
		return -ENOTSUP;
	}

	/* TODO: support port specifier from configuration */
	ccr |= ll_priority;
	ccr |= LL_DMA_LINK_ALLOCATED_PORT0;

	LL_DMA_ConfigControl(dma, channel, ccr);

	hwdesc.channel_tr1 =
		dest_inc_mode | dest_data_width_size | src_inc_mode | src_data_width_size;

	LL_DMA_ConfigTransfer(dma, channel, hwdesc.channel_tr1);

	LL_DMA_ConfigBurstLength(dma, channel, config->source_burst_length,
				 config->dest_burst_length);

	hwdesc.channel_tr2 = ll_direction;
	hwdesc.channel_tr2 |= LL_DMA_TCEM_BLK_TRANSFER;

	LL_DMA_ConfigChannelTransfer(dma, channel, hwdesc.channel_tr2);

	if (ll_direction != LL_DMA_DIRECTION_MEMORY_TO_MEMORY) {
		LL_DMA_SetPeriphRequest(dma, channel, config->dma_slot);
	}

	hwdesc.channel_br1 = config->head_block->block_size;

	LL_DMA_SetBlkDataLength(dma, channel, hwdesc.channel_br1);

	hwdesc.channel_sar = config->head_block->source_address;
	hwdesc.channel_dar = config->head_block->dest_address;

	LL_DMA_ConfigAddresses(dma, channel, hwdesc.channel_sar, hwdesc.channel_dar);

	dma_stm32_enable_it(dma, channel);

	return 0;
}

static int dma_stm32_reload(const struct device *dev, uint32_t id, uint32_t src, uint32_t dst,
			    size_t size)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	int ret = 0;

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

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

	ret = dma_stm32_disable_channel(dma, channel);
	if (ret < 0) {
		return ret;
	}

	LL_DMA_ConfigAddresses(dma, channel, src, dst);

	LL_DMA_SetBlkDataLength(dma, channel, size);

	channel_config->busy = true;

	LL_DMA_EnableChannel(dma, channel);

	return 0;
}

static int dma_stm32_start(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	int ret = 0;

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

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

	if (LL_DMA_IsEnabledChannel(dma, channel)) {
		LOG_INF("Channel %d is already enabled", id);
		return 0;
	}

	/* When starting the dma, the stream is busy before enabling */
	channel_config->busy = true;

	dma_stm32_clear_channel_irq(dma, channel);

	LL_DMA_EnableChannel(dma, channel);

	return 0;
}

static int dma_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	int ret = 0;

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

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

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

static int dma_stm32_get_status(const struct device *dev, uint32_t id, struct dma_status *status)
{
	const struct dma_stm32_config *dev_config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	int ret = 0;

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

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

	status->pending_length = LL_DMA_GetBlkDataLength(dma, channel);
	status->dir = channel_config->direction;
	status->busy = channel_config->busy;

	return 0;
}

static int dma_stm32_suspend(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	int ret = 0;

	if (dev == NULL) {
		LOG_ERR("Invalid device pointer");
		return -EINVAL;
	}

	dev_config = dev->config;

	if (id >= dev_config->max_channels) {
		LOG_ERR("Invalid channel ID %d, max channels %d", id, dev_config->max_channels);
		return -EINVAL;
	}

	dma = dev_config->base;

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

	LL_DMA_SuspendChannel(dma, channel);
	if (WAIT_FOR(LL_DMA_IsActiveFlag_SUSP(dma, channel), POLLING_TIMEOUT_US, k_msleep(1))) {
		return 0;
	}

	LOG_ERR("Timeout waiting for channel %d to suspend", channel);
	return -ETIMEDOUT;
}

static int dma_stm32_resume(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	int ret = 0;

	if (dev == NULL) {
		LOG_ERR("Invalid device pointer");
		return -EINVAL;
	}

	dev_config = dev->config;

	if (id >= dev_config->max_channels) {
		LOG_ERR("Invalid channel ID %d, max channels %d", id, dev_config->max_channels);
		return -EINVAL;
	}

	dma = dev_config->base;

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return ret;
	}

	dma = dev_config->base;

	LL_DMA_ResumeChannel(dma, channel);

	return 0;
}

static void dma_stm32_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_channel *channel_config;
	uint32_t channel;
	DMA_TypeDef *dma;
	uint32_t callback_arg;
	int ret;

	channel_config = &dev_config->channels[id];

	ret = dma_stm32_id_to_channel(id, &channel);
	if (ret < 0) {
		return;
	}

	dma = dev_config->base;

	/* The busy channel is pertinent if not overridden by the HAL */
	if (!channel_config->hal_override && !channel_config->busy) {
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
		if (!channel_config->cyclic) {
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
	struct dma_stm32_data *dev_data = dev->data;

	for (uint32_t i = 0; i < dev_config->max_channels; i++) {
		dev_config->channels[i].busy = false;
	}

	memset(dev_data, 0, sizeof(*dev_data));
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
		LISTIFY(DT_INST_PROP(index, dma_channels),                                         \
			DMA_STM32_IRQ_CONNECT_CHANNEL, (;), index);                                \
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
	LISTIFY(DT_INST_PROP(index, dma_channels),                                                 \
		DMA_STM32_DEFINE_IRQ_HANDLER, (;), index);                                         \
                                                                                                   \
	DMA_STM32_IRQ_CONNECT(index);                                                              \
                                                                                                   \
	static struct dma_stm32_channel dma_stm32_channels_##index[DT_INST_PROP_OR(                \
		index, dma_channels, DT_NUM_IRQS(DT_DRV_INST(index)))];                            \
                                                                                                   \
	static uint32_t dma_stm32_linked_list_buffer##index                               \
		[DMA_STM32_NUM_DESCRIPTORS_PER_CHANNEL *                                           \
		 DT_INST_PROP_OR(index, dma_channels, DT_NUM_IRQS(DT_DRV_INST(index)))] __nocache; \
                                                                                                   \
	const struct dma_stm32_config dma_stm32_config_##index = {                                 \
		COND_CODE_1(DT_NODE_HAS_PROP(__node, clocks),                                      \
			    (.pclken = { .bus = __bus, .enr = __cenr },),                          \
			    (/* Nothing if clocks not present */))                                 \
		.config_irq = dma_stm32_config_irq_##index,                                        \
		.base = (DMA_TypeDef *)DT_INST_REG_ADDR(index),                                    \
		.max_channels =                                                                    \
			DT_INST_PROP_OR(index, dma_channels, DT_NUM_IRQS(DT_DRV_INST(index))),     \
		.channels = dma_stm32_channels_##index,                                            \
		.linked_list_buffer = dma_stm32_linked_list_buffer##index                          \
	};                                                                                         \
                                                                                                   \
	static struct dma_stm32_data dma_stm32_data_##index = {};                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, dma_stm32_init, NULL, &dma_stm32_data_##index,                \
			      &dma_stm32_config_##index, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_STM32_INIT_DEV)
