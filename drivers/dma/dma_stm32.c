/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dma

/**
 * @brief Common part of DMA drivers for stm32.
 * @note  Functions named with stm32_dma_* are SoCs related functions
 *        implemented in dma_stm32_v*.c
 */

#include <soc.h>
#include <init.h>
#include <drivers/dma.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>

#include "dma_stm32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(dma_stm32, CONFIG_DMA_LOG_LEVEL);

static uint32_t table_m_size[] = {
	LL_DMA_MDATAALIGN_BYTE,
	LL_DMA_MDATAALIGN_HALFWORD,
	LL_DMA_MDATAALIGN_WORD,
};

static uint32_t table_p_size[] = {
	LL_DMA_PDATAALIGN_BYTE,
	LL_DMA_PDATAALIGN_HALFWORD,
	LL_DMA_PDATAALIGN_WORD,
};

static void dma_stm32_dump_stream_irq(struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	stm32_dma_dump_stream_irq(dma, id);
}

static void dma_stm32_clear_stream_irq(struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	func_ll_clear_tc[id](dma);
	func_ll_clear_ht[id](dma);
	stm32_dma_clear_stream_irq(dma, id);
}

static void dma_stm32_irq_handler(void *arg)
{
	struct device *dev = arg;
	struct dma_stm32_data *data = dev->data;
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_stream *stream;
	int id;

	for (id = 0; id < data->max_streams; id++) {
		if (func_ll_is_active_tc[id](dma)) {
			break;
		}
		if (stm32_dma_is_irq_happened(dma, id)) {
			break;
		}
	}

	if (id == data->max_streams) {
		LOG_ERR("Unknown interrupt happened.");
		return;
	}

	stream = &data->streams[id];

	if (!IS_ENABLED(CONFIG_DMAMUX_STM32)) {
		stream->busy = false;
	}

	/* the dma stream id is in range from STREAM_OFFSET..<dma-requests> */
	if (func_ll_is_active_tc[id](dma)) {
		func_ll_clear_tc[id](dma);

#ifdef CONFIG_DMAMUX_STM32
		stream->busy = false;
		/* the callback function expects the dmamux channel nb */
		stream->dma_callback(dev, stream->user_data,
				     stream->mux_channel, 0);
#else
		stream->dma_callback(dev, stream->user_data,
				     id + STREAM_OFFSET, 0);
#endif /* CONFIG_DMAMUX_STM32 */
	} else if (stm32_dma_is_unexpected_irq_happened(dma, id)) {
		LOG_ERR("Unexpected irq happened.");

#ifdef CONFIG_DMAMUX_STM32
		stream->dma_callback(dev, stream->user_data,
				     stream->mux_channel, -EIO);
#else
		stream->dma_callback(dev, stream->user_data,
				     id + STREAM_OFFSET, -EIO);
#endif /* CONFIG_DMAMUX_STM32 */
	} else {
		LOG_ERR("Transfer Error.");
		dma_stm32_dump_stream_irq(dev, id);
		dma_stm32_clear_stream_irq(dev, id);

#ifdef CONFIG_DMAMUX_STM32
		stream->dma_callback(dev, stream->user_data,
				     stream->mux_channel, -EIO);
#else
		stream->dma_callback(dev, stream->user_data,
				     id + STREAM_OFFSET, -EIO);
#endif /* CONFIG_DMAMUX_STM32 */
	}
}

static int dma_stm32_width_config(struct dma_config *config,
				    bool source_periph,
				    DMA_TypeDef *dma,
				    LL_DMA_InitTypeDef *DMA_InitStruct,
				    uint32_t id)
{
	uint32_t periph, memory;
	uint32_t m_size = 0, p_size = 0;

	if (source_periph) {
		periph = config->source_data_size;
		memory = config->dest_data_size;
	} else {
		periph = config->dest_data_size;
		memory = config->source_data_size;
	}
	int index = find_lsb_set(config->source_data_size) - 1;

	m_size = table_m_size[index];
	index = find_lsb_set(config->dest_data_size) - 1;
	p_size = table_p_size[index];

	DMA_InitStruct->PeriphOrM2MSrcDataSize = p_size;
	DMA_InitStruct->MemoryOrM2MDstDataSize = m_size;
	return 0;
}

static int dma_stm32_get_priority(uint8_t priority, uint32_t *ll_priority)
{
	switch (priority) {
	case 0x0:
		*ll_priority = LL_DMA_PRIORITY_LOW;
		break;
	case 0x1:
		*ll_priority = LL_DMA_PRIORITY_MEDIUM;
		break;
	case 0x2:
		*ll_priority = LL_DMA_PRIORITY_HIGH;
		break;
	case 0x3:
		*ll_priority = LL_DMA_PRIORITY_VERYHIGH;
		break;
	default:
		LOG_ERR("Priority error. %d", priority);
		return -EINVAL;
	}

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

static int dma_stm32_get_memory_increment(enum dma_addr_adj increment,
					  uint32_t *ll_increment)
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
					  uint32_t *ll_increment)
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

#ifdef CONFIG_DMAMUX_STM32
int dma_stm32_configure(struct device *dev, uint32_t id,
			       struct dma_config *config)
#else
static int dma_stm32_configure(struct device *dev, uint32_t id,
			       struct dma_config *config)
#endif /* CONFIG_DMAMUX_STM32 */
{
	struct dma_stm32_data *data = dev->data;
	struct dma_stm32_stream *stream = &data->streams[id - STREAM_OFFSET];
	const struct dma_stm32_config *dev_config =
					dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)dev_config->base;
	LL_DMA_InitTypeDef DMA_InitStruct;
	uint32_t msize;
	int ret;

	/* give channel from index 0 */
	id = id - STREAM_OFFSET;

	if (id >= data->max_streams) {
		LOG_ERR("cannot configure the dma stream %d.", id);
		return -EINVAL;
	}

	if (stream->busy) {
		LOG_ERR("dma stream %d is busy.", id);
		return -EBUSY;
	}

	stm32_dma_disable_stream(dma, id);
	dma_stm32_clear_stream_irq(dev, id);

	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		LOG_ERR("Data size too big: %d\n",
		       config->head_block->block_size);
		return -EINVAL;
	}

#ifdef CONFIG_DMA_STM32_V1
	if ((config->channel_direction == MEMORY_TO_MEMORY) &&
		(!dev_config->support_m2m)) {
		LOG_ERR("Memcopy not supported for device %s",
			dev->name);
		return -ENOTSUP;
	}
#endif /* CONFIG_DMA_STM32_V1 */

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

	/*
	 * STM32's circular mode will auto reset both source address
	 * counter and destination address counter.
	 */
	if (config->head_block->source_reload_en !=
		config->head_block->dest_reload_en) {
		LOG_ERR("source_reload_en and dest_reload_en must "
			"be the same.");
		return -EINVAL;
	}

	stream->busy		= true;
	stream->dma_callback	= config->dma_callback;
	stream->direction	= config->channel_direction;
	stream->user_data       = config->user_data;
	stream->src_size	= config->source_data_size;
	stream->dst_size	= config->dest_data_size;

	/* check dest or source memory address, warn if 0 */
	if ((config->head_block->source_address == 0)) {
		LOG_WRN("source_buffer address is null.");
	}

	if ((config->head_block->dest_address == 0)) {
		LOG_WRN("dest_buffer address is null.");
	}

	if (stream->direction == MEMORY_TO_PERIPHERAL) {
		DMA_InitStruct.MemoryOrM2MDstAddress =
					config->head_block->source_address;
		DMA_InitStruct.PeriphOrM2MSrcAddress =
					config->head_block->dest_address;
	} else {
		DMA_InitStruct.PeriphOrM2MSrcAddress =
					config->head_block->source_address;
		DMA_InitStruct.MemoryOrM2MDstAddress =
					config->head_block->dest_address;
	}

	uint16_t memory_addr_adj = 0, periph_addr_adj = 0;

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
	default:
		LOG_ERR("Channel direction error (%d).",
				config->channel_direction);
		return -EINVAL;
	}

	ret = dma_stm32_get_memory_increment(memory_addr_adj,
					&DMA_InitStruct.MemoryOrM2MDstIncMode);
	if (ret < 0) {
		return ret;
	}
	ret = dma_stm32_get_periph_increment(periph_addr_adj,
					&DMA_InitStruct.PeriphOrM2MSrcIncMode);
	if (ret < 0) {
		return ret;
	}

	if (config->head_block->source_reload_en) {
		DMA_InitStruct.Mode = LL_DMA_MODE_CIRCULAR;
	} else {
		DMA_InitStruct.Mode = LL_DMA_MODE_NORMAL;
	}

	stream->source_periph = stream->direction == MEMORY_TO_PERIPHERAL;
	ret = dma_stm32_width_config(config, stream->source_periph, dma,
				     &DMA_InitStruct, id);
	if (ret < 0) {
		return ret;
	}
	msize = DMA_InitStruct.MemoryOrM2MDstDataSize;

#if defined(CONFIG_DMA_STM32_V1)
	DMA_InitStruct.MemBurst = stm32_dma_get_mburst(config,
						       stream->source_periph);
	DMA_InitStruct.PeriphBurst = stm32_dma_get_pburst(config,
							stream->source_periph);

	if (config->channel_direction != MEMORY_TO_MEMORY) {
		if (config->dma_slot >= 8) {
			LOG_ERR("dma slot error.");
			return -EINVAL;
		}
	} else {
		if (config->dma_slot >= 8) {
			LOG_ERR("dma slot is too big, using 0 as default.");
			config->dma_slot = 0;
		}
	}
	DMA_InitStruct.Channel = table_ll_channel[config->dma_slot];

	DMA_InitStruct.FIFOThreshold = stm32_dma_get_fifo_threshold(
					config->head_block->fifo_mode_control);

	if (stm32_dma_check_fifo_mburst(&DMA_InitStruct)) {
		DMA_InitStruct.FIFOMode = LL_DMA_FIFOMODE_ENABLE;
	} else {
		DMA_InitStruct.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
	}
#endif
	if (stream->source_periph) {
		DMA_InitStruct.NbData = config->head_block->block_size /
					config->source_data_size;
	} else {
		DMA_InitStruct.NbData = config->head_block->block_size /
					config->dest_data_size;
	}

#if defined(CONFIG_DMA_STM32_V2) || defined(CONFIG_DMAMUX_STM32)
	/*
	 * the with dma V2 and dma mux,
	 * the request ID is stored in the dma_slot
	 */
	DMA_InitStruct.PeriphRequest = config->dma_slot;
#endif
	LL_DMA_Init(dma, table_ll_stream[id], &DMA_InitStruct);

	LL_DMA_EnableIT_TC(dma, table_ll_stream[id]);
	/* Half-Transfer irq is not handled */

#if defined(CONFIG_DMA_STM32_V1)
	if (DMA_InitStruct.FIFOMode == LL_DMA_FIFOMODE_ENABLE) {
		LL_DMA_EnableFifoMode(dma, table_ll_stream[id]);
		LL_DMA_EnableIT_FE(dma, table_ll_stream[id]);
	} else {
		LL_DMA_DisableFifoMode(dma, table_ll_stream[id]);
		LL_DMA_DisableIT_FE(dma, table_ll_stream[id]);
	}
#endif
	return ret;
}

static int dma_stm32_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	int count = 0;

	for (;;) {
		if (!stm32_dma_disable_stream(dma, id)) {
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

#ifdef CONFIG_DMAMUX_STM32
int dma_stm32_reload(struct device *dev, uint32_t id,
			    uint32_t src, uint32_t dst, size_t size)
#else
static int dma_stm32_reload(struct device *dev, uint32_t id,
			    uint32_t src, uint32_t dst, size_t size)
#endif /* CONFIG_DMAMUX_STM32 */
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_data *data = dev->data;
	struct dma_stm32_stream *stream = &data->streams[id - STREAM_OFFSET];

	/* give channel from index 0 */
	id = id - STREAM_OFFSET;

	if (id >= data->max_streams) {
		return -EINVAL;
	}

	stm32_dma_disable_stream(dma, id);

	switch (stream->direction) {
	case MEMORY_TO_PERIPHERAL:
		LL_DMA_SetMemoryAddress(dma, table_ll_stream[id], src);
		LL_DMA_SetPeriphAddress(dma, table_ll_stream[id], dst);
		break;
	case MEMORY_TO_MEMORY:
	case PERIPHERAL_TO_MEMORY:
		LL_DMA_SetPeriphAddress(dma, table_ll_stream[id], src);
		LL_DMA_SetMemoryAddress(dma, table_ll_stream[id], dst);
		break;
	default:
		return -EINVAL;
	}

	if (stream->source_periph) {
		LL_DMA_SetDataLength(dma, table_ll_stream[id],
				     size / stream->src_size);
	} else {
		LL_DMA_SetDataLength(dma, table_ll_stream[id],
				     size / stream->dst_size);
	}

	stm32_dma_enable_stream(dma, id);

	return 0;
}

#ifdef CONFIG_DMAMUX_STM32
int dma_stm32_start(struct device *dev, uint32_t id)
#else
static int dma_stm32_start(struct device *dev, uint32_t id)
#endif /* CONFIG_DMAMUX_STM32 */
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_data *data = dev->data;

	/* give channel from index 0 */
	id = id - STREAM_OFFSET;

	/* Only M2P or M2M mode can be started manually. */
	if (id >= data->max_streams) {
		return -EINVAL;
	}

	dma_stm32_clear_stream_irq(dev, id);

	stm32_dma_enable_stream(dma, id);

	return 0;
}

#ifdef CONFIG_DMAMUX_STM32
int dma_stm32_stop(struct device *dev, uint32_t id)
#else
static int dma_stm32_stop(struct device *dev, uint32_t id)
#endif /* CONFIG_DMAMUX_STM32 */
{
	struct dma_stm32_data *data = dev->data;
	struct dma_stm32_stream *stream = &data->streams[id - STREAM_OFFSET];
	const struct dma_stm32_config *config =
				dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	/* give channel from index 0 */
	id = id - STREAM_OFFSET;

	if (id >= data->max_streams) {
		return -EINVAL;
	}

#ifndef CONFIG_DMAMUX_STM32
	LL_DMA_DisableIT_TC(dma, table_ll_stream[id]);
#endif /* CONFIG_DMAMUX_STM32 */

#if defined(CONFIG_DMA_STM32_V1)
	stm32_dma_disable_fifo_irq(dma, id);
#endif
	dma_stm32_disable_stream(dma, id);
	dma_stm32_clear_stream_irq(dev, id);

	/* Finally, flag stream as free */
	stream->busy = false;

	return 0;
}

struct k_mem_block block;

static int dma_stm32_init(struct device *dev)
{
	struct dma_stm32_data *data = dev->data;
	const struct dma_stm32_config *config = dev->config;
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

#ifdef CONFIG_DMAMUX_STM32
	int offset = ((dev == device_get_binding((const char *)"DMA_1"))
			? 0 : data->max_streams);
#endif /* CONFIG_DMAMUX_STM32 */

	for (int i = 0; i < data->max_streams; i++) {
		data->streams[i].busy = false;
#ifdef CONFIG_DMAMUX_STM32
		/* each further stream->mux_channel is fixed here */
		data->streams[i].mux_channel = i + offset;
#endif /* CONFIG_DMAMUX_STM32 */
	}

	return 0;
}

static int dma_stm32_get_status(struct device *dev, uint32_t id,
				struct dma_status *stat)
{
	const struct dma_stm32_config *config = dev->config;
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);
	struct dma_stm32_data *data = dev->data;
	struct dma_stm32_stream *stream;

	/* give channel from index 0 */
	id = id - STREAM_OFFSET;
	if (id >= data->max_streams) {
		return -EINVAL;
	}

	stream = &data->streams[id];
	stat->pending_length = LL_DMA_GetDataLength(dma, table_ll_stream[id]);
	stat->dir = stream->direction;
	stat->busy = stream->busy;

	return 0;
}

static const struct dma_driver_api dma_funcs = {
	.reload		 = dma_stm32_reload,
	.config		 = dma_stm32_configure,
	.start		 = dma_stm32_start,
	.stop		 = dma_stm32_stop,
	.get_status	 = dma_stm32_get_status,
};

#define DMA_INIT(index)							\
static void dma_stm32_config_irq_##index(struct device *dev);		\
									\
const struct dma_stm32_config dma_stm32_config_##index = {		\
	.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),	\
		    .enr = DT_INST_CLOCKS_CELL(index, bits) },	\
	.config_irq = dma_stm32_config_irq_##index,			\
	.base = DT_INST_REG_ADDR(index),		\
	.support_m2m = DT_INST_PROP(index, st_mem2mem),	\
};									\
									\
static struct dma_stm32_data dma_stm32_data_##index = {			\
	.max_streams = 0,						\
	.streams = NULL,						\
};									\
									\
DEVICE_AND_API_INIT(dma_##index, DT_INST_LABEL(index),	\
		    &dma_stm32_init,					\
		    &dma_stm32_data_##index, &dma_stm32_config_##index,	\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
		    &dma_funcs)

#define irq_func(chan)                                                  \
static void dma_stm32_irq_##chan(void *arg)				\
{									\
	dma_stm32_irq_handler(arg, chan);				\
}

#define IRQ_INIT(dma, chan)                                             \
do {									\
	if (!irq_is_enabled(DT_INST_IRQ_BY_IDX(dma, chan, irq))) {	\
		irq_connect_dynamic(DT_INST_IRQ_BY_IDX(dma, chan, irq), \
			DT_INST_IRQ_BY_IDX(dma, chan, priority),        \
			dma_stm32_irq_handler, dev, 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(dma, chan, irq));	        \
	}								\
	data->max_streams++;						\
} while (0)

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)
DMA_INIT(0);

static void dma_stm32_config_irq_0(struct device *dev)
{
	struct dma_stm32_data *data = dev->data;

	IRQ_INIT(0, 0);
	IRQ_INIT(0, 1);
	IRQ_INIT(0, 2);
	IRQ_INIT(0, 3);
	IRQ_INIT(0, 4);
#if DT_INST_IRQ_HAS_IDX(0, 5)
	IRQ_INIT(0, 5);
#if DT_INST_IRQ_HAS_IDX(0, 6)
	IRQ_INIT(0, 6);
#if DT_INST_IRQ_HAS_IDX(0, 7)
	IRQ_INIT(0, 7);
#endif /* DT_INST_IRQ_HAS_IDX(0, 5) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 6) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 7) */
/* Either 5 or 6 or 7 or 8 channels for DMA across all stm32 series. */
}
#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay) */


#if DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay)
DMA_INIT(1);

static void dma_stm32_config_irq_1(struct device *dev)
{
	struct dma_stm32_data *data = dev->data;

	IRQ_INIT(1, 0);
	IRQ_INIT(1, 1);
	IRQ_INIT(1, 2);
	IRQ_INIT(1, 3);
	IRQ_INIT(1, 4);
#if DT_INST_IRQ_HAS_IDX(1, 5)
	IRQ_INIT(1, 5);
#if DT_INST_IRQ_HAS_IDX(1, 6)
	IRQ_INIT(1, 6);
#if DT_INST_IRQ_HAS_IDX(1, 7)
	IRQ_INIT(1, 7);
#endif /* DT_INST_IRQ_HAS_IDX(1, 5) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 6) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 7) */
/* Either 5 or 6 or 7 or 8 channels for DMA across all stm32 series. */
}
#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay) */
