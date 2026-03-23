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

#define STM32U5_DMA_LINKED_LIST_NODE_SIZE (2)
#define STM32U5_DMA_MAX_BURST_LENGTH      (64) /* Maximum number of beats in a burst */

#ifdef CONFIG_STM32_HAL2
#define STM32_DMA_SRC_DATA_WIDTH_BYTE		LL_DMA_SRC_DATA_WIDTH_BYTE
#define STM32_DMA_SRC_DATA_WIDTH_HALFWORD	LL_DMA_SRC_DATA_WIDTH_HALFWORD
#define STM32_DMA_SRC_DATA_WIDTH_WORD		LL_DMA_SRC_DATA_WIDTH_WORD
#define STM32_DMA_DEST_DATA_WIDTH_BYTE		LL_DMA_DEST_DATA_WIDTH_BYTE
#define STM32_DMA_DEST_DATA_WIDTH_HALFWORD	LL_DMA_DEST_DATA_WIDTH_HALFWORD
#define STM32_DMA_DEST_DATA_WIDTH_WORD		LL_DMA_DEST_DATA_WIDTH_WORD
#define STM32_DMA_PRIORITY_LOW_WEIGHT_LOW	LL_DMA_PRIORITY_LOW_WEIGHT_LOW
#define STM32_DMA_PRIORITY_LOW_WEIGHT_MID	LL_DMA_PRIORITY_LOW_WEIGHT_MID
#define STM32_DMA_PRIORITY_LOW_WEIGHT_HIGH	LL_DMA_PRIORITY_LOW_WEIGHT_HIGH
#define STM32_DMA_PRIORITY_HIGH			LL_DMA_PRIORITY_HIGH
#define STM32_DMA_SRC_ADDR_FIXED		LL_DMA_SRC_ADDR_FIXED
#define STM32_DMA_SRC_ADDR_INCREMENTED		LL_DMA_SRC_ADDR_INCREMENTED
#define STM32_DMA_DEST_ADDR_FIXED		LL_DMA_DEST_ADDR_FIXED
#define STM32_DMA_DEST_ADDR_INCREMENTED		LL_DMA_DEST_ADDR_INCREMENTED
#define STM32_DMA_LINKEDLIST_EXECUTION_Q	LL_DMA_LINKEDLIST_EXECUTION_Q
#define STM32_DMA_LINKEDLIST_EXECUTION_NODE	LL_DMA_LINKEDLIST_EXECUTION_NODE

/* Macro to give the LL dmax_Channel<y> from the DMAx base and channel 0..12 */
#define STM32_DMA_GET_CHANNEL(dmax, idx) \
	LL_DMA_GET_CHANNEL_INSTANCE((dmax), dma_stm32_id_to_stream(idx))
#else /* CONFIG_STM32_HAL2 */
#define STM32_DMA_SRC_DATA_WIDTH_BYTE		LL_DMA_SRC_DATAWIDTH_BYTE
#define STM32_DMA_SRC_DATA_WIDTH_HALFWORD	LL_DMA_SRC_DATAWIDTH_HALFWORD
#define STM32_DMA_SRC_DATA_WIDTH_WORD		LL_DMA_SRC_DATAWIDTH_WORD
#define STM32_DMA_DEST_DATA_WIDTH_BYTE		LL_DMA_DEST_DATAWIDTH_BYTE
#define STM32_DMA_DEST_DATA_WIDTH_HALFWORD	LL_DMA_DEST_DATAWIDTH_HALFWORD
#define STM32_DMA_DEST_DATA_WIDTH_WORD		LL_DMA_DEST_DATAWIDTH_WORD
#define STM32_DMA_PRIORITY_LOW_WEIGHT_LOW	LL_DMA_LOW_PRIORITY_LOW_WEIGHT
#define STM32_DMA_PRIORITY_LOW_WEIGHT_MID	LL_DMA_LOW_PRIORITY_MID_WEIGHT
#define STM32_DMA_PRIORITY_LOW_WEIGHT_HIGH	LL_DMA_LOW_PRIORITY_HIGH_WEIGHT
#define STM32_DMA_PRIORITY_HIGH			LL_DMA_HIGH_PRIORITY
#define STM32_DMA_SRC_ADDR_FIXED		LL_DMA_SRC_FIXED
#define STM32_DMA_SRC_ADDR_INCREMENTED		LL_DMA_SRC_INCREMENT
#define STM32_DMA_DEST_ADDR_FIXED		LL_DMA_DEST_FIXED
#define STM32_DMA_DEST_ADDR_INCREMENTED		LL_DMA_DEST_INCREMENT
#define STM32_DMA_LINKEDLIST_EXECUTION_Q	LL_DMA_LSM_FULL_EXECUTION
#define STM32_DMA_LINKEDLIST_EXECUTION_NODE	LL_DMA_LSM_1LINK_EXECUTION

#define STM32_DMA_GET_CHANNEL(dmax, idx) (dmax), dma_stm32_id_to_stream(idx)
#endif /* CONFIG_STM32_HAL2 */

static const uint32_t table_src_size[] = {
	STM32_DMA_SRC_DATA_WIDTH_BYTE,
	STM32_DMA_SRC_DATA_WIDTH_HALFWORD,
	STM32_DMA_SRC_DATA_WIDTH_WORD,
};

static const uint32_t table_dst_size[] = {
	STM32_DMA_DEST_DATA_WIDTH_BYTE,
	STM32_DMA_DEST_DATA_WIDTH_HALFWORD,
	STM32_DMA_DEST_DATA_WIDTH_WORD,
};

static const uint32_t table_priority[4] = {
	STM32_DMA_PRIORITY_LOW_WEIGHT_LOW,
	STM32_DMA_PRIORITY_LOW_WEIGHT_MID,
	STM32_DMA_PRIORITY_LOW_WEIGHT_HIGH,
	STM32_DMA_PRIORITY_HIGH,
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
#ifdef LL_DMA_CHANNEL_8
		LL_DMA_CHANNEL_8,
		LL_DMA_CHANNEL_9,
		LL_DMA_CHANNEL_10,
		LL_DMA_CHANNEL_11,
		LL_DMA_CHANNEL_12,
		LL_DMA_CHANNEL_13,
		LL_DMA_CHANNEL_14,
		LL_DMA_CHANNEL_15,
#endif /* LL_DMA_CHANNEL_8 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(stream_nr));

	return stream_nr[id];
}

bool dma_stm32_is_tc_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_TC(STM32_DMA_GET_CHANNEL(dma, id));
}

void dma_stm32_clear_tc(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_ClearFlag_TC(STM32_DMA_GET_CHANNEL(dma, id));
}

/* data transfer error */
static inline bool dma_stm32_is_dte_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_DTE(STM32_DMA_GET_CHANNEL(dma, id));
}

/* link transfer error */
static inline bool dma_stm32_is_ule_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_ULE(STM32_DMA_GET_CHANNEL(dma, id));
}

/* user setting error */
static inline bool dma_stm32_is_use_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_USE(STM32_DMA_GET_CHANNEL(dma, id));
}

/* transfer error either a data or user or link error */
bool dma_stm32_is_te_active(DMA_TypeDef *dma, uint32_t id)
{
	return (
		LL_DMA_IsActiveFlag_DTE(STM32_DMA_GET_CHANNEL(dma, id)) ||
		LL_DMA_IsActiveFlag_ULE(STM32_DMA_GET_CHANNEL(dma, id)) ||
		LL_DMA_IsActiveFlag_USE(STM32_DMA_GET_CHANNEL(dma, id))
	);
}
/* clear transfer error either a data or user or link error */
void dma_stm32_clear_te(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_ClearFlag_DTE(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_ClearFlag_ULE(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_ClearFlag_USE(STM32_DMA_GET_CHANNEL(dma, id));
}

bool dma_stm32_is_ht_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsActiveFlag_HT(STM32_DMA_GET_CHANNEL(dma, id));
}

void dma_stm32_clear_ht(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_ClearFlag_HT(STM32_DMA_GET_CHANNEL(dma, id));
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
	return (LL_DMA_IsEnabledIT_TC(STM32_DMA_GET_CHANNEL(dma, id)) &&
		LL_DMA_IsActiveFlag_TC(STM32_DMA_GET_CHANNEL(dma, id)));
}

bool stm32_dma_is_ht_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (LL_DMA_IsEnabledIT_HT(STM32_DMA_GET_CHANNEL(dma, id)) &&
		LL_DMA_IsActiveFlag_HT(STM32_DMA_GET_CHANNEL(dma, id)));
}

/* check if and irq of any type occurred on the channel */
#define stm32_dma_is_irq_active LL_DMA_IsActiveFlag_MIS

void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	dma_stm32_clear_te(dma, id);

	LL_DMA_ClearFlag_TO(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_ClearFlag_SUSP(STM32_DMA_GET_CHANNEL(dma, id));
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
	LL_DMA_EnableChannel(STM32_DMA_GET_CHANNEL(dma, id));
}

bool stm32_dma_is_enabled_stream(DMA_TypeDef *dma, uint32_t id)
{
	if (LL_DMA_IsEnabledChannel(STM32_DMA_GET_CHANNEL(dma, id)) == 1) {
		return true;
	}
	return false;
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	/* GPDMA channel abort sequence */
	LL_DMA_SuspendChannel(STM32_DMA_GET_CHANNEL(dma, id));

	/* reset the channel will disable it */
	LL_DMA_ResetChannel(STM32_DMA_GET_CHANNEL(dma, id));

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
	LL_DMA_ConfigAddresses(STM32_DMA_GET_CHANNEL(dma, channel), src_addr, dest_addr);
}

/* same function to set periph/mem addresses */
void stm32_dma_set_periph_mem_address(DMA_TypeDef *dma,
					     uint32_t channel,
					     uint32_t src_addr,
					     uint32_t dest_addr)
{
	LL_DMA_ConfigAddresses(STM32_DMA_GET_CHANNEL(dma, channel), src_addr, dest_addr);
}

static void dma_stm32_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;
	uint32_t callback_arg;
	int status;

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
	callback_arg = id;

	/* The dma stream id is in range from 0..<dma-requests> */
	if (stm32_dma_is_ht_irq_active(dma, id)) {
		/* Let HAL DMA handle flags on its own */
		if (!stream->hal_override) {
			dma_stm32_clear_ht(dma, id);
		}
		status = DMA_STATUS_BLOCK;
	} else if (stm32_dma_is_tc_irq_active(dma, id)) {
		/* Assuming not cyclic transfer */
		if (stream->cyclic == false) {
			stream->busy = false;
		}
		/* Let HAL DMA handle flags on its own */
		if (!stream->hal_override) {
			dma_stm32_clear_tc(dma, id);
		}
		status = DMA_STATUS_COMPLETE;
	} else {
		LOG_ERR("Transfer Error.");
		stream->busy = false;
		dma_stm32_dump_stream_irq(dev, id);
		dma_stm32_clear_stream_irq(dev, id);
		status = -EIO;
	}

	if (stream->dma_callback != NULL) {
		stream->dma_callback(dev, stream->user_data, callback_arg, status);
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
	struct dma_stm32_stream *stream = &dev_config->streams[id];
	DMA_TypeDef *dma = (DMA_TypeDef *)dev_config->base;
	uint32_t ll_priority;
	uint32_t ll_direction;
	int ret;

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

#if !defined(CONFIG_SOC_SERIES_STM32C5X)
	if ((config->source_burst_length % config->source_data_size) != 0) {
		LOG_ERR("Source burst length %d is not aligned to source data size %d",
			config->source_burst_length, config->source_data_size);
		return -EINVAL;
	}

	if ((config->dest_burst_length % config->dest_data_size) != 0) {
		LOG_ERR("Destination burst length %d is not aligned to destination data size %d",
			config->dest_burst_length, config->dest_data_size);
		return -EINVAL;
	}

	uint32_t burst_beats = config->source_burst_length / config->source_data_size;

	if (burst_beats > STM32U5_DMA_MAX_BURST_LENGTH) {
		LOG_ERR("Source burst length %d is invalid", config->source_burst_length);
		return -EINVAL;
	} else if (burst_beats > 0) {
		LL_DMA_SetSrcBurstLength(STM32_DMA_GET_CHANNEL(dma, id), burst_beats);
	} else {
		/* Default HW behavior (upon reset) is a single beat burst */
		LOG_WRN("Accepting source burst length 0 for backwards compatibility");
		LL_DMA_SetSrcBurstLength(STM32_DMA_GET_CHANNEL(dma, id), 1U);
	}

	burst_beats = config->dest_burst_length / config->dest_data_size;

	if (burst_beats > STM32U5_DMA_MAX_BURST_LENGTH) {
		LOG_ERR("Destination burst length %d is invalid", config->dest_burst_length);
		return -EINVAL;
	} else if (burst_beats > 0) {
		LL_DMA_SetDestBurstLength(STM32_DMA_GET_CHANNEL(dma, id), burst_beats);
	} else {
		/* Default HW behavior (upon reset) is a single beat burst */
		LOG_WRN("Accepting destination burst length 0 for backwards compatibility");
		LL_DMA_SetDestBurstLength(STM32_DMA_GET_CHANNEL(dma, id), 1U);
	}
#endif /* !defined(CONFIG_SOC_SERIES_STM32C5X) */

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

	LL_DMA_ConfigAddresses(STM32_DMA_GET_CHANNEL(dma, id),
		config->head_block->source_address,
		config->head_block->dest_address);

	ret = dma_stm32_get_priority(config->channel_priority, &ll_priority);
	if (ret < 0) {
		return ret;
	}

	LL_DMA_SetChannelPriorityLevel(STM32_DMA_GET_CHANNEL(dma, id), ll_priority);

	ret = dma_stm32_get_direction(config->channel_direction, &ll_direction);
	if (ret < 0) {
		return ret;
	}
	LL_DMA_SetDataTransferDirection(STM32_DMA_GET_CHANNEL(dma, id), ll_direction);

	/* This part is for source */
	switch (config->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		LL_DMA_SetSrcIncMode(STM32_DMA_GET_CHANNEL(dma, id),
				     STM32_DMA_SRC_ADDR_INCREMENTED);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		LL_DMA_SetSrcIncMode(STM32_DMA_GET_CHANNEL(dma, id), STM32_DMA_SRC_ADDR_FIXED);
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		return -ENOTSUP;
	default:
		LOG_ERR("Memory increment error. %d",
			config->head_block->source_addr_adj);
		return -EINVAL;
	}
	LOG_DBG("Channel (%d) src inc (%x).", id,
		LL_DMA_GetSrcIncMode(STM32_DMA_GET_CHANNEL(dma, id)));

	/* This part is for dest */
	switch (config->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		LL_DMA_SetDestIncMode(STM32_DMA_GET_CHANNEL(dma, id),
				      STM32_DMA_DEST_ADDR_INCREMENTED);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		LL_DMA_SetDestIncMode(STM32_DMA_GET_CHANNEL(dma, id), STM32_DMA_DEST_ADDR_FIXED);
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		return -ENOTSUP;
	default:
		LOG_ERR("Periph increment error. %d",
			config->head_block->dest_addr_adj);
		return -EINVAL;
	}
	LOG_DBG("Channel (%d) dest inc (%x).", id,
		LL_DMA_GetDestIncMode(STM32_DMA_GET_CHANNEL(dma, id)));

	stream->source_periph = (stream->direction == PERIPHERAL_TO_MEMORY);

	/* Set the data width, when source_data_size equals dest_data_size */
	int index = find_lsb_set(config->source_data_size) - 1;

	LL_DMA_SetSrcDataWidth(STM32_DMA_GET_CHANNEL(dma, id), table_src_size[index]);

	index = find_lsb_set(config->dest_data_size) - 1;
	LL_DMA_SetDestDataWidth(STM32_DMA_GET_CHANNEL(dma, id), table_dst_size[index]);

	LL_DMA_SetBlkDataLength(STM32_DMA_GET_CHANNEL(dma, id), config->head_block->block_size);

	/* The request ID is stored in the dma_slot */
	LL_DMA_SetPeriphRequest(STM32_DMA_GET_CHANNEL(dma, id), config->dma_slot);

	if (config->head_block->source_reload_en == 0) {
		LL_DMA_SetLinkStepMode(STM32_DMA_GET_CHANNEL(dma, id),
				       STM32_DMA_LINKEDLIST_EXECUTION_NODE);
		/* Initialize the DMA structure in non-cyclic mode only */
		LL_DMA_SetLinkedListAddrOffset(STM32_DMA_GET_CHANNEL(dma, id), 0);
	} else {/* cyclic mode */
		uint32_t linked_list_flags = 0;
		volatile uint32_t *linked_list_node =
			&dev_config->linked_list_buffer[id * STM32U5_DMA_LINKED_LIST_NODE_SIZE];
		/* We use "linked list" to emulate circular mode.
		 * The linked list can consists of just source and/or destination address.
		 * Other registers can remain the same. Linked list itself doesn't contain
		 * pointer to other item, since LLR register is not updated (ULL bit = 0).
		 */
		if (config->head_block->source_addr_adj == config->head_block->dest_addr_adj) {
			/* We update both source and destination address */
			linked_list_node[0] = config->head_block->source_address;
			linked_list_node[1] = config->head_block->dest_address;
			linked_list_flags = LL_DMA_UPDATE_CSAR | LL_DMA_UPDATE_CDAR;
		} else if (config->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			/* We update only source address */
			linked_list_node[0] = config->head_block->source_address;
			linked_list_flags = LL_DMA_UPDATE_CSAR;
		} else if (config->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			/* We update only destination address */
			linked_list_node[0] = config->head_block->dest_address;
			linked_list_flags = LL_DMA_UPDATE_CDAR;
		}
		/* We update only destination address */
		LL_DMA_SetLinkedListBaseAddr(STM32_DMA_GET_CHANNEL(dma, id),
					     (uint32_t)&linked_list_node[0]);
		LL_DMA_ConfigLinkUpdate(STM32_DMA_GET_CHANNEL(dma, id), linked_list_flags,
					(uint32_t)&linked_list_node[0]);

		/* Continuous transfers with Linked List */
		stream->cyclic = true;
		LL_DMA_SetLinkStepMode(STM32_DMA_GET_CHANNEL(dma, id),
				       STM32_DMA_LINKEDLIST_EXECUTION_Q);

		LL_DMA_EnableIT_HT(STM32_DMA_GET_CHANNEL(dma, id));
	}

#ifdef CONFIG_ARM_SECURE_FIRMWARE
#ifdef CONFIG_STM32_HAL2
	LL_DMA_SetChannelSecurity(STM32_DMA_GET_CHANNEL(dma, id), LL_DMA_ATTR_SEC);
	LL_DMA_SetChannelPrivilege(STM32_DMA_GET_CHANNEL(dma, id), LL_DMA_ATTR_PRIV);
	LL_DMA_ConfigChannelAccessSecurity(STM32_DMA_GET_CHANNEL(dma, id),
		LL_DMA_ATTR_SEC, LL_DMA_ATTR_SEC);
#else /* CONFIG_STM32_HAL2 */
	LL_DMA_ConfigChannelSecure(STM32_DMA_GET_CHANNEL(dma, id),
		LL_DMA_CHANNEL_SEC | LL_DMA_CHANNEL_SRC_SEC | LL_DMA_CHANNEL_DEST_SEC);
	LL_DMA_EnableChannelPrivilege(STM32_DMA_GET_CHANNEL(dma, id));
#endif /* CONFIG_STM32_HAL2 */
#endif /* CONFIG_ARM_SECURE_FIRMWARE */


#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	if (dma == HPDMA1) {
		/* Overwrite the config in case of HPDMA */
		if (ll_direction == LL_DMA_DIRECTION_MEMORY_TO_PERIPH) {
			/* Assuming the Memory (Src) is sram0 on AXI bus : PORT0 else PORT1 */
			LL_DMA_SetSrcAllocatedPort(STM32_DMA_GET_CHANNEL(dma, id),
				LL_DMA_SRC_ALLOCATED_PORT0);
			LL_DMA_SetDestAllocatedPort(STM32_DMA_GET_CHANNEL(dma, id),
				LL_DMA_DEST_ALLOCATED_PORT1);
		} else if (ll_direction == LL_DMA_DIRECTION_PERIPH_TO_MEMORY) {
			/* Assuming the Memory (Dest) is sram0 on AXI bus : PORT0 else PORT1 */
			LL_DMA_SetSrcAllocatedPort(STM32_DMA_GET_CHANNEL(dma, id),
				LL_DMA_SRC_ALLOCATED_PORT1);
			LL_DMA_SetDestAllocatedPort(STM32_DMA_GET_CHANNEL(dma, id),
				LL_DMA_DEST_ALLOCATED_PORT0);
		} else {
			/* MEM_TO_MEM: Assuming the Memory is sram0 on AXI bus : PORT0 else PORT1 */
			LL_DMA_SetSrcAllocatedPort(STM32_DMA_GET_CHANNEL(dma, id),
				LL_DMA_SRC_ALLOCATED_PORT0);
			LL_DMA_SetDestAllocatedPort(STM32_DMA_GET_CHANNEL(dma, id),
				LL_DMA_DEST_ALLOCATED_PORT0);
			/* Set Config burst to 8 for Src and Dest on this Channel */
			LL_DMA_ConfigBurstLength(STM32_DMA_GET_CHANNEL(dma, id), 8, 8);
		}
	}
#endif /* CONFIG_SOC_SERIES_STM32H7RSX */

	LL_DMA_EnableIT_TC(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_EnableIT_USE(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_EnableIT_ULE(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_EnableIT_DTE(STM32_DMA_GET_CHANNEL(dma, id));

	return ret;
}

static int dma_stm32_reload(const struct device *dev, uint32_t id,
					  uint32_t src, uint32_t dst,
					  size_t size)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;

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

	LL_DMA_ConfigAddresses(STM32_DMA_GET_CHANNEL(dma, id), src, dst);
	LL_DMA_SetBlkDataLength(STM32_DMA_GET_CHANNEL(dma, id), size);

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
	const struct dma_stm32_stream *stream = &config->streams[id];

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	/* Suspend the channel and wait for suspend Flag set */
	LL_DMA_SuspendChannel(STM32_DMA_GET_CHANNEL(dma, id));
	/* It's not enough to wait for the SUSPF bit with LL_DMA_IsActiveFlag_SUSP */
	do {
		k_busy_wait(800); /* A delay is needed (800us is valid) */
	} while (LL_DMA_IsActiveFlag_SUSP(STM32_DMA_GET_CHANNEL(dma, id)) != 1 &&
			stream->busy == true);

	/* Do not Reset the channel to allow resuming later */
	return 0;
}

static int dma_stm32_resume(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	/* Resume the channel : it's enough after suspend */
	LL_DMA_ResumeChannel(STM32_DMA_GET_CHANNEL(dma, id));

	return 0;
}

static int dma_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	struct dma_stm32_stream *stream = &config->streams[id];
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

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

	LL_DMA_DisableIT_TC(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_DisableIT_USE(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_DisableIT_ULE(STM32_DMA_GET_CHANNEL(dma, id));
	LL_DMA_DisableIT_DTE(STM32_DMA_GET_CHANNEL(dma, id));

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

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	stream = &config->streams[id];
	stat->pending_length = LL_DMA_GetBlkDataLength(STM32_DMA_GET_CHANNEL(dma, id));
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
#define DMA_STM32_IRQ_CONNECT(index)						\
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
#define DMA_STM32_DEFINE_IRQ_HANDLER(chan, dma)					\
	static void dma_stm32_irq_##dma##_##chan(const struct device *dev)	\
	{									\
		dma_stm32_irq_handler(dev, chan);				\
	}

#define DMA_STM32_INIT_DEV(index)						\
	BUILD_ASSERT(DT_INST_PROP(index, dma_channels) ==			\
		     DT_NUM_IRQS(DT_DRV_INST(index)),				\
		     "Nb of Channels and IRQ mismatch");			\
										\
	LISTIFY(DT_INST_PROP(index, dma_channels),				\
		DMA_STM32_DEFINE_IRQ_HANDLER, (;), index);			\
										\
	DMA_STM32_IRQ_CONNECT(index);						\
										\
	static struct dma_stm32_stream dma_stm32_streams_##index		\
		[DT_INST_PROP_OR(index, dma_channels,				\
				 DT_NUM_IRQS(DT_DRV_INST(index)))];		\
										\
	static volatile uint32_t dma_stm32_linked_list_buffer##index		\
		[STM32U5_DMA_LINKED_LIST_NODE_SIZE *				\
		 DT_INST_PROP_OR(index, dma_channels,				\
				 DT_NUM_IRQS(DT_DRV_INST(index)))]		\
		__nocache_noinit;						\
										\
	const struct dma_stm32_config dma_stm32_config_##index = {		\
		.pclken = STM32_DT_INST_CLOCK_INFO(index),			\
		.config_irq = dma_stm32_config_irq_##index,			\
		.base = DT_INST_REG_ADDR(index),				\
		.max_streams = DT_INST_PROP_OR(index, dma_channels,		\
					       DT_NUM_IRQS(DT_DRV_INST(index))),\
		.streams = dma_stm32_streams_##index,				\
		.linked_list_buffer = dma_stm32_linked_list_buffer##index	\
	};									\
										\
	static struct dma_stm32_data dma_stm32_data_##index;			\
										\
	DEVICE_DT_INST_DEFINE(index, dma_stm32_init, NULL,			\
			      &dma_stm32_data_##index,				\
			      &dma_stm32_config_##index,			\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
			      &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_STM32_INIT_DEV)
