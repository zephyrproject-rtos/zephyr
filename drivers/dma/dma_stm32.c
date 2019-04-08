/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <device.h>
#include <dma.h>
#include <dt-bindings/dma/stm32_dma.h>
#include <errno.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <misc/util.h>

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_stm32);

#include <clock_control/stm32_clock_control.h>

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
static u32_t table_stream[] = {
	LL_DMA_STREAM_0,
	LL_DMA_STREAM_1,
	LL_DMA_STREAM_2,
	LL_DMA_STREAM_3,
	LL_DMA_STREAM_4,
	LL_DMA_STREAM_5,
	LL_DMA_STREAM_6,
	LL_DMA_STREAM_7,
};

static u32_t table_channel[] = {
	LL_DMA_CHANNEL_0,
	LL_DMA_CHANNEL_1,
	LL_DMA_CHANNEL_2,
	LL_DMA_CHANNEL_3,
	LL_DMA_CHANNEL_4,
	LL_DMA_CHANNEL_5,
	LL_DMA_CHANNEL_6,
	LL_DMA_CHANNEL_7,
};
#else
static u32_t table_stream[] = {
	LL_DMA_CHANNEL_1,
	LL_DMA_CHANNEL_2,
	LL_DMA_CHANNEL_3,
	LL_DMA_CHANNEL_4,
	LL_DMA_CHANNEL_5,
	LL_DMA_CHANNEL_6,
	LL_DMA_CHANNEL_7,
};
#endif

static u32_t (*func_is_active_ht[])(DMA_TypeDef * DMAx) = {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_IsActiveFlag_HT0,
#endif
	LL_DMA_IsActiveFlag_HT1,
	LL_DMA_IsActiveFlag_HT2,
	LL_DMA_IsActiveFlag_HT3,
	LL_DMA_IsActiveFlag_HT4,
	LL_DMA_IsActiveFlag_HT5,
	LL_DMA_IsActiveFlag_HT6,
	LL_DMA_IsActiveFlag_HT7,
};

static u32_t (*func_is_active_tc[])(DMA_TypeDef * DMAx) = {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_IsActiveFlag_TC0,
#endif
	LL_DMA_IsActiveFlag_TC1,
	LL_DMA_IsActiveFlag_TC2,
	LL_DMA_IsActiveFlag_TC3,
	LL_DMA_IsActiveFlag_TC4,
	LL_DMA_IsActiveFlag_TC5,
	LL_DMA_IsActiveFlag_TC6,
	LL_DMA_IsActiveFlag_TC7,
};

static u32_t (*func_is_active_te[])(DMA_TypeDef * DMAx) = {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_IsActiveFlag_TE0,
#endif
	LL_DMA_IsActiveFlag_TE1,
	LL_DMA_IsActiveFlag_TE2,
	LL_DMA_IsActiveFlag_TE3,
	LL_DMA_IsActiveFlag_TE4,
	LL_DMA_IsActiveFlag_TE5,
	LL_DMA_IsActiveFlag_TE6,
	LL_DMA_IsActiveFlag_TE7,
};

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
static u32_t (*func_is_active_dme[])(DMA_TypeDef * DMAx) = {
	LL_DMA_IsActiveFlag_DME0,
	LL_DMA_IsActiveFlag_DME1,
	LL_DMA_IsActiveFlag_DME2,
	LL_DMA_IsActiveFlag_DME3,
	LL_DMA_IsActiveFlag_DME4,
	LL_DMA_IsActiveFlag_DME5,
	LL_DMA_IsActiveFlag_DME6,
	LL_DMA_IsActiveFlag_DME7,
};

static u32_t (*func_is_active_fe[])(DMA_TypeDef * DMAx) = {
	LL_DMA_IsActiveFlag_FE0,
	LL_DMA_IsActiveFlag_FE1,
	LL_DMA_IsActiveFlag_FE2,
	LL_DMA_IsActiveFlag_FE3,
	LL_DMA_IsActiveFlag_FE4,
	LL_DMA_IsActiveFlag_FE5,
	LL_DMA_IsActiveFlag_FE6,
	LL_DMA_IsActiveFlag_FE7,
};
#endif

#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!defined(CONFIG_SOC_SERIES_STM32F4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X)
static u32_t (*func_is_active_gi[])(DMA_TypeDef * DMAx) = {
	LL_DMA_IsActiveFlag_GI1,
	LL_DMA_IsActiveFlag_GI2,
	LL_DMA_IsActiveFlag_GI3,
	LL_DMA_IsActiveFlag_GI4,
	LL_DMA_IsActiveFlag_GI5,
	LL_DMA_IsActiveFlag_GI6,
	LL_DMA_IsActiveFlag_GI7,
};
#endif

static void (*func_clear_ht[])(DMA_TypeDef * DMAx) = {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_ClearFlag_HT0,
#endif
	LL_DMA_ClearFlag_HT1,
	LL_DMA_ClearFlag_HT2,
	LL_DMA_ClearFlag_HT3,
	LL_DMA_ClearFlag_HT4,
	LL_DMA_ClearFlag_HT5,
	LL_DMA_ClearFlag_HT6,
	LL_DMA_ClearFlag_HT7,
};

static void (*func_clear_tc[])(DMA_TypeDef * DMAx) = {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_ClearFlag_TC0,
#endif
	LL_DMA_ClearFlag_TC1,
	LL_DMA_ClearFlag_TC2,
	LL_DMA_ClearFlag_TC3,
	LL_DMA_ClearFlag_TC4,
	LL_DMA_ClearFlag_TC5,
	LL_DMA_ClearFlag_TC6,
	LL_DMA_ClearFlag_TC7,
};

static void (*func_clear_te[])(DMA_TypeDef * DMAx) = {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_ClearFlag_TE0,
#endif
	LL_DMA_ClearFlag_TE1,
	LL_DMA_ClearFlag_TE2,
	LL_DMA_ClearFlag_TE3,
	LL_DMA_ClearFlag_TE4,
	LL_DMA_ClearFlag_TE5,
	LL_DMA_ClearFlag_TE6,
	LL_DMA_ClearFlag_TE7,
};

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
static void (*func_clear_dme[])(DMA_TypeDef * DMAx) = {
	LL_DMA_ClearFlag_DME0,
	LL_DMA_ClearFlag_DME1,
	LL_DMA_ClearFlag_DME2,
	LL_DMA_ClearFlag_DME3,
	LL_DMA_ClearFlag_DME4,
	LL_DMA_ClearFlag_DME5,
	LL_DMA_ClearFlag_DME6,
	LL_DMA_ClearFlag_DME7,
};

static void (*func_clear_fe[])(DMA_TypeDef * DMAx) = {
	LL_DMA_ClearFlag_FE0,
	LL_DMA_ClearFlag_FE1,
	LL_DMA_ClearFlag_FE2,
	LL_DMA_ClearFlag_FE3,
	LL_DMA_ClearFlag_FE4,
	LL_DMA_ClearFlag_FE5,
	LL_DMA_ClearFlag_FE6,
	LL_DMA_ClearFlag_FE7,
};
#endif

#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!defined(CONFIG_SOC_SERIES_STM32F4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X)
static void (*func_clear_gi[])(DMA_TypeDef * DMAx) = {
	LL_DMA_ClearFlag_GI1,
	LL_DMA_ClearFlag_GI2,
	LL_DMA_ClearFlag_GI3,
	LL_DMA_ClearFlag_GI4,
	LL_DMA_ClearFlag_GI5,
	LL_DMA_ClearFlag_GI6,
	LL_DMA_ClearFlag_GI7,
};
#endif

static u32_t table_m_size[] = {
	LL_DMA_MDATAALIGN_BYTE,
	LL_DMA_MDATAALIGN_HALFWORD,
	LL_DMA_MDATAALIGN_WORD,
};

static u32_t table_p_size[] = {
	LL_DMA_PDATAALIGN_BYTE,
	LL_DMA_PDATAALIGN_HALFWORD,
	LL_DMA_PDATAALIGN_WORD,
};

struct dma_stm32_stream {
	u32_t direction;
	bool busy;
	void *callback_arg;
	void (*dma_callback)(void *arg, u32_t id,
			     int error_code);
};

struct dma_stm32_data {
	int max_streams;
	struct dma_stm32_stream *streams;
};

struct dma_stm32_config {
	struct stm32_pclken pclken;
	void (*config_irq)(struct device *dev);

	bool support_m2m;
	u32_t base;
};

/* DMA burst length */
#define BURST_TRANS_LENGTH_1			0

/* Maximum data sent in single transfer (Bytes) */
#define DMA_STM32_MAX_DATA_ITEMS		0xffff

static void dma_stm32_dump_stream_irq(struct device *dev, u32_t id)
{
	const struct dma_stm32_config *config = dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LOG_INF("tc: %d, ht: %d, te: %d, dme: %d, fe: %d",
		 func_is_active_tc[id](dma),
		 func_is_active_ht[id](dma),
		 func_is_active_te[id](dma),
		 func_is_active_dme[id](dma),
		 func_is_active_fe[id](dma));
#else
	LOG_INF("tc: %d, ht: %d, te: %d, gi: %d",
		 func_is_active_tc[id](dma),
		 func_is_active_ht[id](dma),
		 func_is_active_te[id](dma),
		 func_is_active_gi[id](dma));
#endif
}

static void dma_stm32_clear_stream_irq(struct device *dev, u32_t id)
{
	const struct dma_stm32_config *config = dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	func_clear_tc[id](dma);
	func_clear_ht[id](dma);
	func_clear_te[id](dma);
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	func_clear_dme[id](dma);
	func_clear_fe[id](dma);
#else
	func_clear_gi[id](dma);
#endif
}

static void dma_stm32_irq_handler(void *arg)
{
	struct device *dev = arg;
	struct dma_stm32_data *data = dev->driver_data;
	const struct dma_stm32_config *config = dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;
	int id;

	for (id = 0; id < data->max_streams; id++) {
		if (func_is_active_tc[id](dma) ||
		    func_is_active_ht[id](dma) ||
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
		    func_is_active_fe[id](dma) ||
#endif
		    func_is_active_te[id](dma)) {
			break;
		}
	}

	if (id == data->max_streams) {
		LOG_ERR("Unknow interrupt happened.");
		return;
	}

	stream = &data->streams[id];
	stream->busy = false;
	if (func_is_active_tc[id](dma)) {
		func_clear_tc[id](dma);

		stream->dma_callback(stream->callback_arg, id, 0);
	} else if (func_is_active_ht[id](dma)) {
		/* Silently ignore spurious transfer half complete IRQ */
		LOG_INF("Half of the transfer has been completed.");
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	} else if (func_is_active_fe[id](dma)) {
		LOG_ERR("FiFo error.");
		dma_stm32_dump_stream_irq(dev, id);
		dma_stm32_clear_stream_irq(dev, id);

		stream->dma_callback(stream->callback_arg, id, -EIO);
#endif
	} else {
		LOG_ERR("Transfer Error.");
		dma_stm32_dump_stream_irq(dev, id);
		dma_stm32_clear_stream_irq(dev, id);

		stream->dma_callback(stream->callback_arg, id, -EIO);
	}
}

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
static void dma_stm32_config_direct_mode(DMA_TypeDef *dma, u32_t id, bool on)
{
	if (on) {
		LL_DMA_EnableIT_DME(dma, table_stream[id]);
		LL_DMA_DisableIT_FE(dma, table_stream[id]);
		LL_DMA_DisableFifoMode(dma, table_stream[id]);
	} else {
		LL_DMA_DisableIT_DME(dma, table_stream[id]);
		LL_DMA_EnableIT_FE(dma, table_stream[id]);
		LL_DMA_EnableFifoMode(dma, table_stream[id]);
	}
}
#endif

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
#define BURST_SIZE(dev, dev_short)					\
do {									\
	case 1:								\
		LL_DMA_Set##dev##Burstxfer(dma, table_stream[id],	\
				LL_DMA_##dev_short##BURST_SINGLE);	\
		break;							\
	case 4:								\
		LL_DMA_Set##dev##Burstxfer(dma, table_stream[id],	\
				LL_DMA_##dev_short##BURST_INC4);	\
		break;							\
	case 8:								\
		LL_DMA_Set##dev##Burstxfer(dma, table_stream[id],	\
				LL_DMA_##dev_short##BURST_INC8);	\
		break;							\
	case 16:							\
		LL_DMA_Set##dev##Burstxfer(dma, table_stream[id],	\
				LL_DMA_##dev_short##BURST_INC16);	\
		break;							\
	default:							\
		LOG_ERR("Burst size error");				\
		return -ENOTSUP;					\
} while (0)
#endif

static u32_t dma_stm32_width_config(struct dma_config *config,
				    bool source_periph,
				    DMA_TypeDef *dma,
				    u32_t id)
{
	u32_t periph, memory;
	u32_t m_size = 0, p_size = 0;
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	u32_t periph_burst, memory_burst;
#endif

	if (source_periph) {
		periph = config->source_data_size;
		memory = config->dest_data_size;
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
		periph_burst = config->source_burst_length;
		memory_burst = config->dest_burst_length;
#endif
	} else {
		periph = config->dest_data_size;
		memory = config->source_data_size;
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
		periph_burst = config->dest_burst_length;
		memory_burst = config->source_burst_length;
#endif
	}
	int index = find_lsb_set(config->source_data_size) - 1;

	m_size = table_m_size[index];
	index = find_lsb_set(config->dest_data_size) - 1;
	p_size = table_p_size[index];
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	switch (memory_burst) {
		BURST_SIZE(Memory, M);
	}
	switch (periph_burst) {
		BURST_SIZE(Periph, P);
	}
#endif

	return (m_size | p_size);
}

static int dma_stm32_get_priority(u8_t priority, u32_t *ll_priority)
{
	switch (priority) {
	case STM32_DMA_PRIORITY_LOW:
		*ll_priority = LL_DMA_PRIORITY_LOW;
		break;
	case STM32_DMA_PRIORITY_MEDIUM:
		*ll_priority = LL_DMA_PRIORITY_MEDIUM;
		break;
	case STM32_DMA_PRIORITY_HIGH:
		*ll_priority = LL_DMA_PRIORITY_HIGH;
		break;
	case STM32_DMA_PRIORITY_VERYHIGH:
		*ll_priority = LL_DMA_PRIORITY_VERYHIGH;
		break;
	default:
		LOG_ERR("Priority error. %d", priority);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_get_direction(enum dma_channel_direction direction,
				   u32_t *ll_direction)
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

static int dma_stm32_get_memory_increment(enum dma_addr_adj increment,
					  u32_t *ll_increment)
{
	switch (increment) {
	case DMA_ADDR_ADJ_INCREMENT:
		*ll_increment = LL_DMA_MEMORY_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*ll_increment = LL_DMA_MEMORY_NOINCREMENT;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		return -ENOTSUP;
	default:
	LOG_ERR("Memory increment error. %d", increment);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_get_periph_increment(enum dma_addr_adj increment,
					  u32_t *ll_increment)
{
	switch (increment) {
	case DMA_ADDR_ADJ_INCREMENT:
		*ll_increment = LL_DMA_PERIPH_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*ll_increment = LL_DMA_PERIPH_NOINCREMENT;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		return -ENOTSUP;
	default:
		LOG_ERR("Periph increment error. %d", increment);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_config(struct device *dev, u32_t id,
			    struct dma_config *config)
{
	struct dma_stm32_data *data = dev->driver_data;
	struct dma_stm32_stream *stream = &data->streams[id];
	const struct dma_stm32_config *dma_stm32_config =
					dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)dma_stm32_config->base;
	u32_t p_addr, m_addr;
	u16_t count;
	int ret;

	if (id >= data->max_streams) {
		return -EINVAL;
	}

	if (stream->busy) {
		return -EBUSY;
	}

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	if (LL_DMA_IsEnabledStream(dma, table_stream[id])) {
		LL_DMA_DisableStream(dma, table_stream[id]);
	}
#else
	if (LL_DMA_IsEnabledChannel(dma, table_stream[id])) {
		LL_DMA_DisableChannel(dma, table_stream[id]);
	}
#endif

	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		LOG_ERR("Data size too big: %d\n",
		       config->head_block->block_size);
		return -EINVAL;
	}

	if ((stream->direction == MEMORY_TO_MEMORY) &&
		(!dma_stm32_config->support_m2m)) {
		LOG_ERR("Memcopy not supported for device %s",
			dev->config->name);
		return -ENOTSUP;
	}

	if (config->source_data_size != 4U &&
	    config->source_data_size != 2U &&
	    config->source_data_size != 1U) {
		LOG_ERR("Source unit size error, %d",
			config->source_data_size);
		return -EINVAL;
		}

	if (config->dest_data_size != 4U &&
	    config->dest_data_size != 2U &&
	    config->dest_data_size != 1U) {
		LOG_ERR("Dest unit size error, %d",
			config->dest_data_size);
		return -EINVAL;
		}

	stream->busy		= true;
	stream->dma_callback	= config->dma_callback;
	stream->direction	= config->channel_direction;
	stream->callback_arg    = config->callback_arg;

	if (stream->direction == MEMORY_TO_PERIPHERAL) {
		m_addr = config->head_block->source_address;
		p_addr = config->head_block->dest_address;
	} else {
		p_addr = config->head_block->source_address;
		m_addr = config->head_block->dest_address;
	}

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_EnableFifoMode(dma, table_stream[id]);
	LL_DMA_EnableIT_FE(dma, table_stream[id]);
	LL_DMA_SetFIFOThreshold(dma, table_stream[id],
			LL_DMA_FIFOTHRESHOLD_FULL);
#endif
	u32_t size_config, mode_config;
	u32_t priority, direction, memory_increment, periph_increment;
	u16_t memory_addr_adj, periph_addr_adj;

	ret = dma_stm32_get_priority(config->channel_priority, &priority);
	if (ret < 0) {
		return ret;
	}

	ret = dma_stm32_get_direction(config->channel_direction, &direction);
	if (ret < 0) {
		return ret;
	}

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
	case PERIPHERAL_TO_MEMORY:
		memory_addr_adj = config->head_block->dest_addr_adj;
		periph_addr_adj = config->head_block->source_addr_adj;
		break;
	case MEMORY_TO_PERIPHERAL:
		memory_addr_adj = config->head_block->source_addr_adj;
		periph_addr_adj = config->head_block->dest_addr_adj;
		break;
	/* Direction has been asserted in dma_stm32_get_direction. */
	}

	ret = dma_stm32_get_memory_increment(memory_addr_adj,
					     &memory_increment);
	if (ret < 0) {
		return ret;
	}
	ret = dma_stm32_get_periph_increment(periph_addr_adj,
					     &periph_increment);
	if (ret < 0) {
		return ret;
	}

	size_config = dma_stm32_width_config(config, true, dma, id);
	mode_config = direction | memory_increment | periph_increment |
		      size_config | priority;
	LL_DMA_ConfigTransfer(dma, table_stream[id], mode_config);

	if (config->channel_direction != MEMORY_TO_MEMORY) {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_SetChannelSelection(dma, table_stream[id],
			table_channel[config->dma_slot]);

	if (config->source_burst_length == BURST_TRANS_LENGTH_1 &&
	    config->dest_burst_length == BURST_TRANS_LENGTH_1) {
		dma_stm32_config_direct_mode(dma, id, true);
	} else {
		dma_stm32_config_direct_mode(dma, id, false);
	}
#endif
	}

	count = config->head_block->block_size;

	LL_DMA_SetMemoryAddress(dma, table_stream[id], m_addr);
	LL_DMA_SetPeriphAddress(dma, table_stream[id], p_addr);
	LL_DMA_SetDataLength(dma, table_stream[id], count);

	LL_DMA_EnableIT_TC(dma, table_stream[id]);

	return ret;
}

int dma_stm32_disable_stream(struct device *dev, u32_t id)
{
	const struct dma_stm32_config *dma_stm32_config =
					dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)dma_stm32_config->base;
	int count = 0;

	for (;;) {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
		/* Stream already disabled */
		if (!LL_DMA_IsEnabledStream(dma, table_stream[id])) {
			return 0;
		}
		LL_DMA_DisableStream(dma, table_stream[id]);
#else
		if (!LL_DMA_IsEnabledChannel(dma, table_stream[id])) {
			return 0;
		}
		LL_DMA_DisableChannel(dma, table_stream[id]);
#endif
		/* After trying for 5 seconds, give up */
		if (count++ > (5 * 1000) / 50) {
			return -EBUSY;
		}
		k_sleep(1);
	}

	return 0;
}

static int dma_stm32_reload(struct device *dev, u32_t id,
			    u32_t src, u32_t dst, size_t size)
{
	const struct dma_stm32_config *config = dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_data *data = dev->driver_data;
	struct dma_stm32_stream *stream = &data->streams[id];

	if (id >= data->max_streams) {
		return -EINVAL;
	}

	switch (stream->direction) {
	case MEMORY_TO_PERIPHERAL:
		LL_DMA_SetMemoryAddress(dma, table_stream[id], src);
		LL_DMA_SetPeriphAddress(dma, table_stream[id], dst);
		break;
	case MEMORY_TO_MEMORY:
	case PERIPHERAL_TO_MEMORY:
		LL_DMA_SetPeriphAddress(dma, table_stream[id], src);
		LL_DMA_SetMemoryAddress(dma, table_stream[id], dst);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_start(struct device *dev, u32_t id)
{
	const struct dma_stm32_config *config = dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_data *data = dev->driver_data;

	/* Only M2P or M2M mode can be started manually. */
	if (id >= data->max_streams) {
		return -EINVAL;
	}

	dma_stm32_clear_stream_irq(dev, id);

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_EnableStream(dma, table_stream[id]);
#else
	LL_DMA_EnableChannel(dma, table_stream[id]);
#endif

	return 0;
}

static int dma_stm32_stop(struct device *dev, u32_t id)
{
	struct dma_stm32_data *data = dev->driver_data;
	struct dma_stm32_stream *stream = &data->streams[id];
	const struct dma_stm32_config *config =
				dev->config->config_info;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	if (id >= data->max_streams) {
		return -EINVAL;
	}

	LL_DMA_DisableIT_TC(dma, table_stream[id]);
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_DisableIT_FE(dma, table_stream[id]);
#endif
	dma_stm32_disable_stream(dev, id);
	dma_stm32_clear_stream_irq(dev, id);

	/* Finally, flag stream as free */
	stream->busy = false;

	return 0;
}

struct k_mem_block block;

static int dma_stm32_init(struct device *dev)
{
	struct dma_stm32_data *data = dev->driver_data;
	const struct dma_stm32_config *config = dev->config->config_info;
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);
	if (clock_control_on(clk,
		(clock_control_subsys_t *) &config->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	config->config_irq(dev);

	int size_stream =
		sizeof(struct dma_stm32_stream) * data->max_streams;
	data->streams = k_malloc(size_stream);
	if (!data->streams) {
		LOG_ERR("HEAP_MEM_POOL_SIZE is too small");
		return -ENOMEM;
	}
	memset(data->streams, 0, size_stream);

	for (int i = 0; i < data->max_streams; i++) {
		data->streams[i].busy = false;
	}

	return 0;
}

static const struct dma_driver_api dma_funcs = {
	.reload		 = dma_stm32_reload,
	.config		 = dma_stm32_config,
	.start		 = dma_stm32_start,
	.stop		 = dma_stm32_stop,
};

#define DMA_INIT(index)							\
static void dma_stm32_config_irq_##index(struct device *dev);		\
									\
const struct dma_stm32_config dma_stm32_config_##index = {		\
	.pclken = { .bus = DT_DMA_##index##_CLOCK_BUS,			\
		    .enr = DT_DMA_##index##_CLOCK_BITS },		\
	.config_irq = dma_stm32_config_irq_##index,			\
	.base = DT_DMA_##index##_BASE_ADDRESS,				\
	.support_m2m = DT_DMA_##index##_SUPPORT_M2M,			\
};									\
									\
static struct dma_stm32_data dma_stm32_data_##index = {			\
	.max_streams = 0,						\
	.streams = NULL,						\
};									\
									\
DEVICE_AND_API_INIT(dma_##index, DT_DMA_##index##_NAME, &dma_stm32_init,\
		    &dma_stm32_data_##index, &dma_stm32_config_##index,	\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
		    &dma_funcs)

#define irq_func(chan)                                                  \
static void dma_stm32_irq_##chan(void *arg)				\
{									\
	dma_stm32_irq_handler(arg, chan);				\
}

#define IRQ_INIT(dma, chan)                                             \
do {									\
	if (!irq_is_enabled(DT_DMA_##dma##_IRQ_##chan)) {		\
		irq_connect_dynamic(DT_DMA_##dma##_IRQ_##chan,		\
			DT_DMA_##dma##_IRQ_##chan##_PRI,		\
			dma_stm32_irq_handler, dev, 0);			\
		irq_enable(DT_DMA_##dma##_IRQ_##chan);			\
	}								\
	data->max_streams++;						\
} while (0)

#ifdef CONFIG_DMA_1
DMA_INIT(1);

static void dma_stm32_config_irq_1(struct device *dev)
{
	struct dma_stm32_data *data = dev->driver_data;

	IRQ_INIT(1, 0);
	IRQ_INIT(1, 1);
	IRQ_INIT(1, 2);
	IRQ_INIT(1, 3);
	IRQ_INIT(1, 4);
#ifdef DT_DMA_1_IRQ_5
	IRQ_INIT(1, 5);
	IRQ_INIT(1, 6);
#ifdef DT_DMA_1_IRQ_7
	IRQ_INIT(1, 7);
#endif
#endif
/* Either 5 or 7 or 8 channels for DMA1 across all stm32 series. */
}
#endif

#ifdef CONFIG_DMA_2
DMA_INIT(2);

static void dma_stm32_config_irq_2(struct device *dev)
{
	struct dma_stm32_data *data = dev->driver_data;

#ifdef DT_DMA_2_IRQ_0
	IRQ_INIT(2, 0);
	IRQ_INIT(2, 1);
	IRQ_INIT(2, 2);
	IRQ_INIT(2, 3);
	IRQ_INIT(2, 4);
#ifdef DT_DMA_2_IRQ_5
	IRQ_INIT(2, 5);
	IRQ_INIT(2, 6);
#ifdef DT_DMA_2_IRQ_7
	IRQ_INIT(2, 7);
#endif
#endif
#endif
/* Either 0 or 5 or 7 or 8 channels for DMA1 across all stm32 series. */
}
#endif
