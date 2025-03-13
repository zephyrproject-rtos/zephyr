/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common part of DMA drivers for stm32U5.
 * @note  Functions named with stm32_dma_* are SoCs related functions
 *
 */

#include "dma_stm32.h"

#include <zephyr/init.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma/dma_stm32.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dma_stm32, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32u5_dma

static const uint32_t table_src_size[] = {
	LL_DMA_SRC_DATAWIDTH_BYTE,
	LL_DMA_SRC_DATAWIDTH_HALFWORD,
	LL_DMA_SRC_DATAWIDTH_WORD,
};

static const uint32_t table_dst_size[] = {
	LL_DMA_DEST_DATAWIDTH_BYTE,
	LL_DMA_DEST_DATAWIDTH_HALFWORD,
	LL_DMA_DEST_DATAWIDTH_WORD,
};

static const uint32_t table_priority[4] = {
	LL_DMA_LOW_PRIORITY_LOW_WEIGHT,
	LL_DMA_LOW_PRIORITY_MID_WEIGHT,
	LL_DMA_LOW_PRIORITY_HIGH_WEIGHT,
	LL_DMA_HIGH_PRIORITY,
};

static void dma_stm32_dump_stream_irq(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	stm32_dma_dump_stream_irq(dma, id);
}

static void dma_stm32_clear_stream_irq(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	dma_stm32_clear_tc(dma, id);
	dma_stm32_clear_ht(dma, id);
	stm32_dma_clear_stream_irq(dma, id);
}


uint32_t dma_stm32_id_to_stream(uint32_t id)
{
	static const uint32_t stream_nr[] = {
		LL_DMA_CHANNEL_0,
		LL_DMA_CHANNEL_1,
		LL_DMA_CHANNEL_2,
		LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,
		LL_DMA_CHANNEL_5,
		LL_DMA_CHANNEL_6,
		LL_DMA_CHANNEL_7,
		LL_DMA_CHANNEL_8,
		LL_DMA_CHANNEL_9,
		LL_DMA_CHANNEL_10,
		LL_DMA_CHANNEL_11,
		LL_DMA_CHANNEL_12,
		LL_DMA_CHANNEL_13,
		LL_DMA_CHANNEL_14,
		LL_DMA_CHANNEL_15,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(stream_nr));

	return stream_nr[id];
}

bool dma_stm32_is_tc_active(DMA_TypeDef *DMAx, uint32_t id)
{
	return LL_DMA_IsActiveFlag_TC(DMAx, dma_stm32_id_to_stream(id));
}

void dma_stm32_clear_tc(DMA_TypeDef *DMAx, uint32_t id)
{
	LL_DMA_ClearFlag_TC(DMAx, dma_stm32_id_to_stream(id));
}

/* data transfer error */
static inline bool dma_stm32_is_dte_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_DTE(dma, dma_stm32_id_to_stream(id));
}

/* link transfer error */
static inline bool dma_stm32_is_ule_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_ULE(dma, dma_stm32_id_to_stream(id));
}

/* user setting error */
static inline bool dma_stm32_is_use_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_USE(dma, dma_stm32_id_to_stream(id));
}

/* transfer error either a data or user or link error */
bool dma_stm32_is_te_active(DMA_TypeDef *DMAx, uint32_t id)
{
	return (
	LL_DMA_IsActiveFlag_DTE(DMAx, dma_stm32_id_to_stream(id)) ||
		LL_DMA_IsActiveFlag_ULE(DMAx, dma_stm32_id_to_stream(id)) ||
		LL_DMA_IsActiveFlag_USE(DMAx, dma_stm32_id_to_stream(id))
	);
}
/* clear transfer error either a data or user or link error */
void dma_stm32_clear_te(DMA_TypeDef *DMAx, uint32_t id)
{
	LL_DMA_ClearFlag_DTE(DMAx, dma_stm32_id_to_stream(id));
	LL_DMA_ClearFlag_ULE(DMAx, dma_stm32_id_to_stream(id));
	LL_DMA_ClearFlag_USE(DMAx, dma_stm32_id_to_stream(id));
}

bool dma_stm32_is_ht_active(DMA_TypeDef *DMAx, uint32_t id)
{
	return LL_DMA_IsActiveFlag_HT(DMAx, dma_stm32_id_to_stream(id));
}

void dma_stm32_clear_ht(DMA_TypeDef *DMAx, uint32_t id)
{
	LL_DMA_ClearFlag_HT(DMAx, dma_stm32_id_to_stream(id));
}

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	LOG_INF("tc: %d, ht: %d, dte: %d, ule: %d, use: %d",
		dma_stm32_is_tc_active(dma, id),
		dma_stm32_is_ht_active(dma, id),
		dma_stm32_is_dte_active(dma, id),
		dma_stm32_is_ule_active(dma, id),
		dma_stm32_is_use_active(dma, id)
	);
}

/* Check if nsecure masked interrupt is active on channel */
bool stm32_dma_is_tc_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (LL_DMA_IsEnabledIT_TC(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_TC(dma, dma_stm32_id_to_stream(id)));
}

bool stm32_dma_is_ht_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (LL_DMA_IsEnabledIT_HT(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_HT(dma, dma_stm32_id_to_stream(id)));
}

static inline bool stm32_dma_is_te_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (
		(LL_DMA_IsEnabledIT_DTE(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_DTE(dma, dma_stm32_id_to_stream(id))) ||
		(LL_DMA_IsEnabledIT_ULE(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_ULE(dma, dma_stm32_id_to_stream(id))) ||
		(LL_DMA_IsEnabledIT_USE(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_USE(dma, dma_stm32_id_to_stream(id)))
		);
}

/* check if and irq of any type occurred on the channel */
#define stm32_dma_is_irq_active LL_DMA_IsActiveFlag_MIS

void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	dma_stm32_clear_te(dma, id);

	LL_DMA_ClearFlag_TO(dma, dma_stm32_id_to_stream(id));
	LL_DMA_ClearFlag_SUSP(dma, dma_stm32_id_to_stream(id));
}

bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, uint32_t id)
{
	if (dma_stm32_is_te_active(dma, id)) {
		return true;
	}

	return false;
}

void stm32_dma_enable_stream(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_EnableChannel(dma, dma_stm32_id_to_stream(id));
}

bool stm32_dma_is_enabled_stream(DMA_TypeDef *dma, uint32_t id)
{
	if (LL_DMA_IsEnabledChannel(dma, dma_stm32_id_to_stream(id)) == 1) {
		return true;
	}
	return false;
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	/* GPDMA channel abort sequence */
	LL_DMA_SuspendChannel(dma, dma_stm32_id_to_stream(id));

	/* reset the channel will disable it */
	LL_DMA_ResetChannel(dma, dma_stm32_id_to_stream(id));

	if (!stm32_dma_is_enabled_stream(dma, id)) {
		return 0;
	}

	return -EAGAIN;
}

void stm32_dma_set_mem_periph_address(DMA_TypeDef *dma,
					     uint32_t channel,
					     uint32_t src_addr,
					     uint32_t dest_addr)
{
	LL_DMA_ConfigAddresses(dma, channel, src_addr, dest_addr);
}

/* same function to set periph/mem addresses */
void stm32_dma_set_periph_mem_address(DMA_TypeDef *dma,
					     uint32_t channel,
					     uint32_t src_addr,
					     uint32_t dest_addr)
{
	LL_DMA_ConfigAddresses(dma, channel, src_addr, dest_addr);
}

static void dma_stm32_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;
	uint32_t callback_arg;

	__ASSERT_NO_MSG(id < config->max_streams);

	stream = &config->streams[id];
	/* The busy channel is pertinent if not overridden by the HAL */
	if ((stream->hal_override != true) && (stream->busy == false)) {
		/*
		 * When DMA channel is not overridden by HAL,
		 * ignore irq if the channel is not busy anymore
		 */
		dma_stm32_clear_stream_irq(dev, id);
		return;
	}
	callback_arg = id + STM32_DMA_STREAM_OFFSET;

	/* The dma stream id is in range from STM32_DMA_STREAM_OFFSET..<dma-requests> */
	if (stm32_dma_is_ht_irq_active(dma, id)) {
		/* Let HAL DMA handle flags on its own */
		if (!stream->hal_override) {
			dma_stm32_clear_ht(dma, id);
		}
		stream->dma_callback(dev, stream->user_data, callback_arg, DMA_STATUS_BLOCK);
	} else if (stm32_dma_is_tc_irq_active(dma, id)) {
		/* Assuming not cyclic transfer */
		if (stream->cyclic == false) {
			stream->busy = false;
		}
		/* Let HAL DMA handle flags on its own */
		if (!stream->hal_override) {
			dma_stm32_clear_tc(dma, id);
		}
		stream->dma_callback(dev, stream->user_data, callback_arg, DMA_STATUS_COMPLETE);
	} else {
		LOG_ERR("Transfer Error.");
		stream->busy = false;
		dma_stm32_dump_stream_irq(dev, id);
		dma_stm32_clear_stream_irq(dev, id);
		stream->dma_callback(dev, stream->user_data,
				     callback_arg, -EIO);
	}
}

static int dma_stm32_get_priority(uint8_t priority, uint32_t *ll_priority)
{
	if (priority > ARRAY_SIZE(table_priority)) {
		LOG_ERR("Priority error. %d", priority);
		return -EINVAL;
	}

	*ll_priority = table_priority[priority];
	return 0;
}

static int dma_stm32_get_direction(enum dma_channel_direction direction,
				   uint32_t *ll_direction)
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

static int dma_stm32_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	int count = 0;

	for (;;) {
		if (stm32_dma_disable_stream(dma, id) == 0) {
			return 0;
		}
		/* After trying for 5 seconds, give up */
		if (count++ > (5 * 1000)) {
			return -EBUSY;
		}
		k_sleep(K_MSEC(1));
	}

	return 0;
}

static int dma_stm32_configure(const struct device *dev,
					     uint32_t id,
					     struct dma_config *config)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_stream *stream =
				&dev_config->streams[id - STM32_DMA_STREAM_OFFSET];
	DMA_TypeDef *dma = (DMA_TypeDef *)dev_config->base;
	LL_DMA_InitTypeDef DMA_InitStruct;
	int ret;

	/*  Linked list Node  and structure initialization */
	static LL_DMA_LinkNodeTypeDef Node_GPDMA_Channel;
	LL_DMA_InitLinkedListTypeDef DMA_InitLinkedListStruct;
	LL_DMA_InitNodeTypeDef NodeConfig;

	LL_DMA_ListStructInit(&DMA_InitLinkedListStruct);
	LL_DMA_NodeStructInit(&NodeConfig);
	LL_DMA_StructInit(&DMA_InitStruct);

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	if (id >= dev_config->max_streams) {
		LOG_ERR("cannot configure the dma stream %d.", id);
		return -EINVAL;
	}

	if (stream->busy) {
		LOG_ERR("dma stream %d is busy.", id);
		return -EBUSY;
	}

	if (dma_stm32_disable_stream(dma, id) != 0) {
		LOG_ERR("could not disable dma stream %d.", id);
		return -EBUSY;
	}

	dma_stm32_clear_stream_irq(dev, id);

	/* Check potential DMA override (if id parameters and stream are valid) */
	if (config->linked_channel == STM32_DMA_HAL_OVERRIDE) {
		/* DMA channel is overridden by HAL DMA
		 * Retain that the channel is busy and proceed to the minimal
		 * configuration to properly route the IRQ
		 */
		stream->busy = true;
		stream->hal_override = true;
		stream->dma_callback = config->dma_callback;
		stream->user_data = config->user_data;
		return 0;
	}

	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		LOG_ERR("Data size too big: %d\n",
		       config->head_block->block_size);
		return -EINVAL;
	}

	/* Support only the same data width for source and dest */
	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("source and dest data size differ.");
		return -EINVAL;
	}

	if (config->source_data_size != 4U &&
	    config->source_data_size != 2U &&
	    config->source_data_size != 1U) {
		LOG_ERR("source and dest unit size error, %d",
			config->source_data_size);
		return -EINVAL;
	}

	stream->busy		= true;
	stream->dma_callback	= config->dma_callback;
	stream->direction	= config->channel_direction;
	stream->user_data       = config->user_data;
	stream->src_size	= config->source_data_size;
	stream->dst_size	= config->dest_data_size;

	/* Check dest or source memory address, warn if 0 */
	if (config->head_block->source_address == 0) {
		LOG_WRN("source_buffer address is null.");
	}

	if (config->head_block->dest_address == 0) {
		LOG_WRN("dest_buffer address is null.");
	}

	DMA_InitStruct.SrcAddress = config->head_block->source_address;
	DMA_InitStruct.DestAddress = config->head_block->dest_address;
	NodeConfig.SrcAddress = config->head_block->source_address;
	NodeConfig.DestAddress = config->head_block->dest_address;
	NodeConfig.BlkDataLength = config->head_block->block_size;

	ret = dma_stm32_get_priority(config->channel_priority,
				     &DMA_InitStruct.Priority);
	if (ret < 0) {
		return ret;
	}

	ret = dma_stm32_get_direction(config->channel_direction,
				      &DMA_InitStruct.Direction);
	if (ret < 0) {
		return ret;
	}

	/* This part is for source */
	switch (config->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		DMA_InitStruct.SrcIncMode = LL_DMA_SRC_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		DMA_InitStruct.SrcIncMode = LL_DMA_SRC_FIXED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		return -ENOTSUP;
	default:
		LOG_ERR("Memory increment error. %d",
			config->head_block->source_addr_adj);
		return -EINVAL;
	}
	LOG_DBG("Channel (%d) src inc (%x).",
				id, DMA_InitStruct.SrcIncMode);

	/* This part is for dest */
	switch (config->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		DMA_InitStruct.DestIncMode = LL_DMA_DEST_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		DMA_InitStruct.DestIncMode = LL_DMA_DEST_FIXED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		return -ENOTSUP;
	default:
		LOG_ERR("Periph increment error. %d",
			config->head_block->dest_addr_adj);
		return -EINVAL;
	}
	LOG_DBG("Channel (%d) dest inc (%x).",
				id, DMA_InitStruct.DestIncMode);

	stream->source_periph = (stream->direction == PERIPHERAL_TO_MEMORY);

	/* Set the data width, when source_data_size equals dest_data_size */
	int index = find_lsb_set(config->source_data_size) - 1;

	DMA_InitStruct.SrcDataWidth = table_src_size[index];

	index = find_lsb_set(config->dest_data_size) - 1;
	DMA_InitStruct.DestDataWidth = table_dst_size[index];

	DMA_InitStruct.BlkDataLength = config->head_block->block_size;

	/* The request ID is stored in the dma_slot */
	DMA_InitStruct.Request = config->dma_slot;

	if (config->head_block->source_reload_en == 0) {
		/* Initialize the DMA structure in non-cyclic mode only */
		LL_DMA_Init(dma, dma_stm32_id_to_stream(id), &DMA_InitStruct);
	} else {/* cyclic mode */
		/* Setting GPDMA request */
		NodeConfig.DestDataWidth = DMA_InitStruct.DestDataWidth;
		NodeConfig.SrcDataWidth = DMA_InitStruct.SrcDataWidth;
		NodeConfig.DestIncMode = DMA_InitStruct.DestIncMode;
		NodeConfig.SrcIncMode = DMA_InitStruct.SrcIncMode;
		NodeConfig.Direction = DMA_InitStruct.Direction;
		NodeConfig.Request = DMA_InitStruct.Request;

		/* Continuous transfers with Linked List */
		stream->cyclic = true;
		LL_DMA_List_Init(dma, dma_stm32_id_to_stream(id), &DMA_InitLinkedListStruct);
		LL_DMA_CreateLinkNode(&NodeConfig, &Node_GPDMA_Channel);
		LL_DMA_ConnectLinkNode(&Node_GPDMA_Channel, LL_DMA_CLLR_OFFSET5,
				       &Node_GPDMA_Channel, LL_DMA_CLLR_OFFSET5);
		LL_DMA_SetLinkedListBaseAddr(dma, dma_stm32_id_to_stream(id),
					     (uint32_t)&Node_GPDMA_Channel);
		LL_DMA_ConfigLinkUpdate(dma, dma_stm32_id_to_stream(id),
					(LL_DMA_UPDATE_CTR1 | LL_DMA_UPDATE_CTR2 |
					 LL_DMA_UPDATE_CBR1 | LL_DMA_UPDATE_CSAR |
					 LL_DMA_UPDATE_CDAR | LL_DMA_UPDATE_CLLR),
					(uint32_t)&Node_GPDMA_Channel);

		LL_DMA_EnableIT_HT(dma, dma_stm32_id_to_stream(id));
	}

#ifdef CONFIG_ARM_SECURE_FIRMWARE
	LL_DMA_ConfigChannelSecure(dma, dma_stm32_id_to_stream(id),
		LL_DMA_CHANNEL_SEC | LL_DMA_CHANNEL_SRC_SEC | LL_DMA_CHANNEL_DEST_SEC);
	LL_DMA_EnableChannelPrivilege(dma, dma_stm32_id_to_stream(id));
#endif

	LL_DMA_EnableIT_TC(dma, dma_stm32_id_to_stream(id));
	LL_DMA_EnableIT_USE(dma, dma_stm32_id_to_stream(id));
	LL_DMA_EnableIT_ULE(dma, dma_stm32_id_to_stream(id));
	LL_DMA_EnableIT_DTE(dma, dma_stm32_id_to_stream(id));

	return ret;
}

static int dma_stm32_reload(const struct device *dev, uint32_t id,
					  uint32_t src, uint32_t dst,
					  size_t size)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	stream = &config->streams[id];

	if (dma_stm32_disable_stream(dma, id) != 0) {
		return -EBUSY;
	}

	if (stream->direction > PERIPHERAL_TO_MEMORY) {
		return -EINVAL;
	}

	LL_DMA_ConfigAddresses(dma,
				dma_stm32_id_to_stream(id),
				src, dst);

	LL_DMA_SetBlkDataLength(dma, dma_stm32_id_to_stream(id), size);

	/* When reloading the dma, the stream is busy again before enabling */
	stream->busy = true;

	stm32_dma_enable_stream(dma, id);

	return 0;
}

static int dma_stm32_start(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	/* Only M2P or M2M mode can be started manually. */
	if (id >= config->max_streams) {
		return -EINVAL;
	}

	/* Repeated start : return now if channel is already started */
	if (stm32_dma_is_enabled_stream(dma, id)) {
		return 0;
	}

	/* When starting the dma, the stream is busy before enabling */
	stream = &config->streams[id];
	stream->busy = true;

	dma_stm32_clear_stream_irq(dev, id);

	stm32_dma_enable_stream(dma, id);

	return 0;
}

static int dma_stm32_suspend(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	/* Suspend the channel and wait for suspend Flag set */
	LL_DMA_SuspendChannel(dma, dma_stm32_id_to_stream(id));
	/* It's not enough to wait for the SUSPF bit with LL_DMA_IsActiveFlag_SUSP */
	do {
		k_msleep(1); /* A delay is needed (1ms is valid) */
	} while (LL_DMA_IsActiveFlag_SUSP(dma, dma_stm32_id_to_stream(id)) != 1);

	/* Do not Reset the channel to allow resuming later */
	return 0;
}

static int dma_stm32_resume(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	/* Resume the channel : it's enough after suspend */
	LL_DMA_ResumeChannel(dma, dma_stm32_id_to_stream(id));

	return 0;
}

static int dma_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	struct dma_stm32_stream *stream = &config->streams[id - STM32_DMA_STREAM_OFFSET];
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	if (stream->hal_override) {
		stream->busy = false;
		return 0;
	}

	/* Repeated stop : return now if channel is already stopped */
	if (!stm32_dma_is_enabled_stream(dma, id)) {
		return 0;
	}

	LL_DMA_DisableIT_TC(dma, dma_stm32_id_to_stream(id));
	LL_DMA_DisableIT_USE(dma, dma_stm32_id_to_stream(id));
	LL_DMA_DisableIT_ULE(dma, dma_stm32_id_to_stream(id));
	LL_DMA_DisableIT_DTE(dma, dma_stm32_id_to_stream(id));

	dma_stm32_clear_stream_irq(dev, id);
	dma_stm32_disable_stream(dma, id);

	/* Finally, flag stream as free */
	stream->busy = false;

	return 0;
}

static int dma_stm32_init(const struct device *dev)
{
	const struct dma_stm32_config *config = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_on(clk,
		(clock_control_subsys_t) &config->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	config->config_irq(dev);

	for (uint32_t i = 0; i < config->max_streams; i++) {
		config->streams[i].busy = false;
	}

	((struct dma_stm32_data *)dev->data)->dma_ctx.magic = 0;
	((struct dma_stm32_data *)dev->data)->dma_ctx.dma_channels = 0;
	((struct dma_stm32_data *)dev->data)->dma_ctx.atomic = 0;

	return 0;
}

static int dma_stm32_get_status(const struct device *dev,
				uint32_t id, struct dma_status *stat)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;
	if (id >= config->max_streams) {
		return -EINVAL;
	}

	stream = &config->streams[id];
	stat->pending_length = LL_DMA_GetBlkDataLength(dma, dma_stm32_id_to_stream(id));
	stat->dir = stream->direction;
	stat->busy = stream->busy;

	return 0;
}

static DEVICE_API(dma, dma_funcs) = {
	.reload		 = dma_stm32_reload,
	.config		 = dma_stm32_configure,
	.start		 = dma_stm32_start,
	.stop		 = dma_stm32_stop,
	.get_status	 = dma_stm32_get_status,
	.suspend	 = dma_stm32_suspend,
	.resume		 = dma_stm32_resume,
};

/*
 * Macro to CONNECT and enable each irq (order is given by the 'listify')
 * chan: channel of the DMA instance (assuming one irq per channel)
 *       stm32U5x has 16 channels
 * dma : dma instance (one GPDMA instance on stm32U5x)
 */
#define DMA_STM32_IRQ_CONNECT_CHANNEL(chan, dma)			\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(dma, chan, irq),		\
			    DT_INST_IRQ_BY_IDX(dma, chan, priority),	\
			    dma_stm32_irq_##dma##_##chan,		\
			    DEVICE_DT_INST_GET(dma), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(dma, chan, irq));		\
	} while (0)

/*
 * Macro to configure the irq for each dma instance (index)
 * Loop to CONNECT and enable each irq for each channel
 * Expecting as many irq as property <dma_channels>
 */
#define DMA_STM32_IRQ_CONNECT(index) \
static void dma_stm32_config_irq_##index(const struct device *dev)	\
{									\
	ARG_UNUSED(dev);						\
									\
	LISTIFY(DT_INST_PROP(index, dma_channels),			\
		DMA_STM32_IRQ_CONNECT_CHANNEL, (;), index);		\
}

/*
 * Macro to instanciate the irq handler (order is given by the 'listify')
 * chan: channel of the DMA instance (assuming one irq per channel)
 *       stm32U5x has 16 channels
 * dma : dma instance (one GPDMA instance on stm32U5x)
 */
#define DMA_STM32_DEFINE_IRQ_HANDLER(chan, dma)				\
static void dma_stm32_irq_##dma##_##chan(const struct device *dev)	\
{									\
	dma_stm32_irq_handler(dev, chan);				\
}

#define DMA_STM32_INIT_DEV(index)					\
BUILD_ASSERT(DT_INST_PROP(index, dma_channels)				\
	== DT_NUM_IRQS(DT_DRV_INST(index)),				\
	"Nb of Channels and IRQ mismatch");				\
									\
LISTIFY(DT_INST_PROP(index, dma_channels),				\
	DMA_STM32_DEFINE_IRQ_HANDLER, (;), index);			\
									\
DMA_STM32_IRQ_CONNECT(index);						\
									\
static struct dma_stm32_stream						\
	dma_stm32_streams_##index[DT_INST_PROP_OR(index, dma_channels,	\
		DT_NUM_IRQS(DT_DRV_INST(index)))];	\
									\
const struct dma_stm32_config dma_stm32_config_##index = {		\
	.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),		\
		    .enr = DT_INST_CLOCKS_CELL(index, bits) },		\
	.config_irq = dma_stm32_config_irq_##index,			\
	.base = DT_INST_REG_ADDR(index),				\
	.max_streams = DT_INST_PROP_OR(index, dma_channels,		\
		DT_NUM_IRQS(DT_DRV_INST(index))				\
	),		\
	.streams = dma_stm32_streams_##index,				\
};									\
									\
static struct dma_stm32_data dma_stm32_data_##index = {			\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &dma_stm32_init,					\
		    NULL,						\
		    &dma_stm32_data_##index, &dma_stm32_config_##index,	\
		    PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
		    &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_STM32_INIT_DEV)
