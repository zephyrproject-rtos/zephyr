/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common part of DMA drivers for stm32.
 * @note  Functions named with stm32_dma_* are SoCs related functions
 *        implemented in dma_stm32_v*.c
 */

#include "dma_stm32.h"

#include <zephyr/init.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma/dma_stm32.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dma_stm32, CONFIG_DMA_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v1)
#define DT_DRV_COMPAT st_stm32_dma_v1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2)
#define DT_DRV_COMPAT st_stm32_dma_v2
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2bis)
#define DT_DRV_COMPAT st_stm32_dma_v2bis
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0))
#if DT_INST_IRQ_HAS_IDX(0, 7)
#define DMA_STM32_0_STREAM_COUNT 8
#elif DT_INST_IRQ_HAS_IDX(0, 6)
#define DMA_STM32_0_STREAM_COUNT 7
#elif DT_INST_IRQ_HAS_IDX(0, 5)
#define DMA_STM32_0_STREAM_COUNT 6
#elif DT_INST_IRQ_HAS_IDX(0, 4)
#define DMA_STM32_0_STREAM_COUNT 5
#else
#define DMA_STM32_0_STREAM_COUNT 3
#endif
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(1))
#if DT_INST_IRQ_HAS_IDX(1, 7)
#define DMA_STM32_1_STREAM_COUNT 8
#elif DT_INST_IRQ_HAS_IDX(1, 6)
#define DMA_STM32_1_STREAM_COUNT 7
#elif DT_INST_IRQ_HAS_IDX(1, 5)
#define DMA_STM32_1_STREAM_COUNT 6
#else
#define DMA_STM32_1_STREAM_COUNT 5
#endif
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(1)) */

static const uint32_t table_m_size[] = {
	LL_DMA_MDATAALIGN_BYTE,
	LL_DMA_MDATAALIGN_HALFWORD,
	LL_DMA_MDATAALIGN_WORD,
};

static const uint32_t table_p_size[] = {
	LL_DMA_PDATAALIGN_BYTE,
	LL_DMA_PDATAALIGN_HALFWORD,
	LL_DMA_PDATAALIGN_WORD,
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
#ifdef CONFIG_DMAMUX_STM32
	callback_arg = stream->mux_channel;
#else
	callback_arg = id + STM32_DMA_STREAM_OFFSET;
#endif /* CONFIG_DMAMUX_STM32 */
	if (!IS_ENABLED(CONFIG_DMAMUX_STM32)) {
		stream->busy = false;
	}

	/* The dma stream id is in range from STM32_DMA_STREAM_OFFSET..<dma-requests> */
	if (stm32_dma_is_ht_irq_active(dma, id)) {
		/* Let HAL DMA handle flags on its own */
		if (!stream->hal_override) {
			dma_stm32_clear_ht(dma, id);
		}
		stream->dma_callback(dev, stream->user_data, callback_arg, DMA_STATUS_BLOCK);
	} else if (stm32_dma_is_tc_irq_active(dma, id)) {
#ifdef CONFIG_DMAMUX_STM32
		/* Circular buffer never stops receiving as long as peripheral is enabled */
		if (!stream->cyclic) {
			stream->busy = false;
		}
#endif
		/* Let HAL DMA handle flags on its own */
		if (!stream->hal_override) {
			dma_stm32_clear_tc(dma, id);
		}
		stream->dma_callback(dev, stream->user_data, callback_arg, DMA_STATUS_COMPLETE);
	} else if (stm32_dma_is_unexpected_irq_happened(dma, id)) {
		LOG_ERR("Unexpected irq happened.");
		stream->dma_callback(dev, stream->user_data,
				     callback_arg, -EIO);
	} else {
		LOG_ERR("Transfer Error.");
		dma_stm32_dump_stream_irq(dev, id);
		dma_stm32_clear_stream_irq(dev, id);
		stream->dma_callback(dev, stream->user_data,
				     callback_arg, -EIO);
	}
}

#ifdef CONFIG_DMA_STM32_SHARED_IRQS

#define HANDLE_IRQS(index)						       \
	static const struct device *const dev_##index =			       \
		DEVICE_DT_INST_GET(index);				       \
	const struct dma_stm32_config *cfg_##index = dev_##index->config;      \
	DMA_TypeDef *dma_##index = (DMA_TypeDef *)(cfg_##index->base);	       \
									       \
	for (id = 0; id < cfg_##index->max_streams; ++id) {		       \
		if (stm32_dma_is_irq_active(dma_##index, id)) {		       \
			dma_stm32_irq_handler(dev_##index, id);		       \
		}							       \
	}

static void dma_stm32_shared_irq_handler(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t id = 0;

	DT_INST_FOREACH_STATUS_OKAY(HANDLE_IRQS)
}

#endif /* CONFIG_DMA_STM32_SHARED_IRQS */

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

DMA_STM32_EXPORT_API int dma_stm32_configure(const struct device *dev,
					     uint32_t id,
					     struct dma_config *config)
{
	const struct dma_stm32_config *dev_config = dev->config;
	struct dma_stm32_stream *stream =
				&dev_config->streams[id - STM32_DMA_STREAM_OFFSET];
	DMA_TypeDef *dma = (DMA_TypeDef *)dev_config->base;
	LL_DMA_InitTypeDef DMA_InitStruct;
	int ret;

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
		stream->cyclic = false;
		return 0;
	}

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

	/* Support only the same data width for source and dest */
	if ((config->dest_data_size != config->source_data_size)) {
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
	stream->cyclic		= config->head_block->source_reload_en;

	/* Check dest or source memory address, warn if 0 */
	if (config->head_block->source_address == 0) {
		LOG_WRN("source_buffer address is null.");
	}

	if (config->head_block->dest_address == 0) {
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

	LOG_DBG("Channel (%d) memory inc (%x).",
				id, DMA_InitStruct.MemoryOrM2MDstIncMode);

	ret = dma_stm32_get_periph_increment(periph_addr_adj,
					&DMA_InitStruct.PeriphOrM2MSrcIncMode);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Channel (%d) peripheral inc (%x).",
				id, DMA_InitStruct.PeriphOrM2MSrcIncMode);

	if (stream->cyclic) {
		DMA_InitStruct.Mode = LL_DMA_MODE_CIRCULAR;
	} else {
		DMA_InitStruct.Mode = LL_DMA_MODE_NORMAL;
	}

	stream->source_periph = (stream->direction == PERIPHERAL_TO_MEMORY);

	/* set the data width, when source_data_size equals dest_data_size */
	int index = find_lsb_set(config->source_data_size) - 1;
	DMA_InitStruct.PeriphOrM2MSrcDataSize = table_p_size[index];
	index = find_lsb_set(config->dest_data_size) - 1;
	DMA_InitStruct.MemoryOrM2MDstDataSize = table_m_size[index];

#if defined(CONFIG_DMA_STM32_V1)
	DMA_InitStruct.MemBurst = stm32_dma_get_mburst(config,
						       stream->source_periph);
	DMA_InitStruct.PeriphBurst = stm32_dma_get_pburst(config,
							stream->source_periph);

#if !defined(CONFIG_SOC_SERIES_STM32H7X) && !defined(CONFIG_SOC_SERIES_STM32MP1X)
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

	DMA_InitStruct.Channel = dma_stm32_slot_to_channel(config->dma_slot);
#endif

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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2) || DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dmamux)
	/* With dma V2 and dmamux,the request ID is stored in the dma_slot */
	DMA_InitStruct.PeriphRequest = config->dma_slot;
#endif
	LL_DMA_Init(dma, dma_stm32_id_to_stream(id), &DMA_InitStruct);

	LL_DMA_EnableIT_TC(dma, dma_stm32_id_to_stream(id));

	/* Enable Half-Transfer irq if circular mode is enabled */
	if (stream->cyclic) {
		LL_DMA_EnableIT_HT(dma, dma_stm32_id_to_stream(id));
	}

#if defined(CONFIG_DMA_STM32_V1)
	if (DMA_InitStruct.FIFOMode == LL_DMA_FIFOMODE_ENABLE) {
		LL_DMA_EnableFifoMode(dma, dma_stm32_id_to_stream(id));
		LL_DMA_EnableIT_FE(dma, dma_stm32_id_to_stream(id));
	} else {
		LL_DMA_DisableFifoMode(dma, dma_stm32_id_to_stream(id));
		LL_DMA_DisableIT_FE(dma, dma_stm32_id_to_stream(id));
	}
#endif
	return ret;
}

DMA_STM32_EXPORT_API int dma_stm32_reload(const struct device *dev, uint32_t id,
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

	switch (stream->direction) {
	case MEMORY_TO_PERIPHERAL:
		LL_DMA_SetMemoryAddress(dma, dma_stm32_id_to_stream(id), src);
		LL_DMA_SetPeriphAddress(dma, dma_stm32_id_to_stream(id), dst);
		break;
	case MEMORY_TO_MEMORY:
	case PERIPHERAL_TO_MEMORY:
		LL_DMA_SetPeriphAddress(dma, dma_stm32_id_to_stream(id), src);
		LL_DMA_SetMemoryAddress(dma, dma_stm32_id_to_stream(id), dst);
		break;
	default:
		return -EINVAL;
	}

	if (stream->source_periph) {
		LL_DMA_SetDataLength(dma, dma_stm32_id_to_stream(id),
				     size / stream->src_size);
	} else {
		LL_DMA_SetDataLength(dma, dma_stm32_id_to_stream(id),
				     size / stream->dst_size);
	}

	/* When reloading the dma, the stream is busy again before enabling */
	stream->busy = true;

	stm32_dma_enable_stream(dma, id);

	return 0;
}

DMA_STM32_EXPORT_API int dma_stm32_start(const struct device *dev, uint32_t id)
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

DMA_STM32_EXPORT_API int dma_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dma_stm32_config *config = dev->config;
	struct dma_stm32_stream *stream = &config->streams[id - STM32_DMA_STREAM_OFFSET];
	DMA_TypeDef *dma = (DMA_TypeDef *)(config->base);

	/* Give channel from index 0 */
	id = id - STM32_DMA_STREAM_OFFSET;

	if (id >= config->max_streams) {
		return -EINVAL;
	}

	/* Repeated stop : return now if channel is already stopped */
	if (!stm32_dma_is_enabled_stream(dma, id)) {
		return 0;
	}

#if !defined(CONFIG_DMAMUX_STM32) \
	|| defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32MP1X)
	LL_DMA_DisableIT_TC(dma, dma_stm32_id_to_stream(id));
#endif /* CONFIG_DMAMUX_STM32 */

#if defined(CONFIG_DMA_STM32_V1)
	stm32_dma_disable_fifo_irq(dma, id);
#endif

	dma_stm32_clear_stream_irq(dev, id);
	dma_stm32_disable_stream(dma, id);

	/* Finally, flag stream as free */
	stream->busy = false;

	return 0;
}

static int dma_stm32_init(const struct device *dev)
{
	const struct dma_stm32_config *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk,
		(clock_control_subsys_t) &config->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	config->config_irq(dev);

	for (uint32_t i = 0; i < config->max_streams; i++) {
		config->streams[i].busy = false;
#ifdef CONFIG_DMAMUX_STM32
		/* Each further stream->mux_channel is fixed here */
		config->streams[i].mux_channel = i + config->offset;
#endif /* CONFIG_DMAMUX_STM32 */
	}

	((struct dma_stm32_data *)dev->data)->dma_ctx.magic = 0;
	((struct dma_stm32_data *)dev->data)->dma_ctx.dma_channels = 0;
	((struct dma_stm32_data *)dev->data)->dma_ctx.atomic = 0;

	return 0;
}

DMA_STM32_EXPORT_API int dma_stm32_get_status(const struct device *dev,
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
	stat->pending_length = LL_DMA_GetDataLength(dma, dma_stm32_id_to_stream(id));
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

#define DMA_STM32_INIT_DEV(index)					\
static struct dma_stm32_stream						\
	dma_stm32_streams_##index[DMA_STM32_##index##_STREAM_COUNT];	\
									\
const struct dma_stm32_config dma_stm32_config_##index = {		\
	.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),		\
		    .enr = DT_INST_CLOCKS_CELL(index, bits) },		\
	.config_irq = dma_stm32_config_irq_##index,			\
	.base = DT_INST_REG_ADDR(index),				\
	IF_ENABLED(CONFIG_DMA_STM32_V1,					\
		(.support_m2m = DT_INST_PROP(index, st_mem2mem),))	\
	.max_streams = DMA_STM32_##index##_STREAM_COUNT,		\
	.streams = dma_stm32_streams_##index,				\
	IF_ENABLED(CONFIG_DMAMUX_STM32,					\
		(.offset = DT_INST_PROP(index, dma_offset),))		\
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
		    &dma_funcs)

#ifdef CONFIG_DMA_STM32_SHARED_IRQS

#define DMA_STM32_DEFINE_IRQ_HANDLER(dma, chan) /* nothing */

#define DMA_STM32_IRQ_CONNECT(dma, chan)				\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(dma, chan, irq),		\
			    DT_INST_IRQ_BY_IDX(dma, chan, priority),	\
			    dma_stm32_shared_irq_handler,		\
			    DEVICE_DT_INST_GET(dma), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(dma, chan, irq));		\
	} while (false)


#else /* CONFIG_DMA_STM32_SHARED_IRQS */

#define DMA_STM32_DEFINE_IRQ_HANDLER(dma, chan)				\
static void dma_stm32_irq_##dma##_##chan(const struct device *dev)	\
{									\
	dma_stm32_irq_handler(dev, chan);				\
}


#define DMA_STM32_IRQ_CONNECT(dma, chan)				\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(dma, chan, irq),		\
			    DT_INST_IRQ_BY_IDX(dma, chan, priority),	\
			    dma_stm32_irq_##dma##_##chan,		\
			    DEVICE_DT_INST_GET(dma), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(dma, chan, irq));		\
	} while (false)

#endif /* CONFIG_DMA_STM32_SHARED_IRQS */


#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0))

DMA_STM32_DEFINE_IRQ_HANDLER(0, 0);
DMA_STM32_DEFINE_IRQ_HANDLER(0, 1);
DMA_STM32_DEFINE_IRQ_HANDLER(0, 2);
#if DT_INST_IRQ_HAS_IDX(0, 3)
DMA_STM32_DEFINE_IRQ_HANDLER(0, 3);
DMA_STM32_DEFINE_IRQ_HANDLER(0, 4);
#if DT_INST_IRQ_HAS_IDX(0, 5)
DMA_STM32_DEFINE_IRQ_HANDLER(0, 5);
#if DT_INST_IRQ_HAS_IDX(0, 6)
DMA_STM32_DEFINE_IRQ_HANDLER(0, 6);
#if DT_INST_IRQ_HAS_IDX(0, 7)
DMA_STM32_DEFINE_IRQ_HANDLER(0, 7);
#endif /* DT_INST_IRQ_HAS_IDX(0, 3) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 5) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 6) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 7) */

static void dma_stm32_config_irq_0(const struct device *dev)
{
	ARG_UNUSED(dev);

	DMA_STM32_IRQ_CONNECT(0, 0);
	DMA_STM32_IRQ_CONNECT(0, 1);
#ifndef CONFIG_DMA_STM32_SHARED_IRQS
	DMA_STM32_IRQ_CONNECT(0, 2);
#endif /* CONFIG_DMA_STM32_SHARED_IRQS */
#if DT_INST_IRQ_HAS_IDX(0, 3)
	DMA_STM32_IRQ_CONNECT(0, 3);
#ifndef CONFIG_DMA_STM32_SHARED_IRQS
	DMA_STM32_IRQ_CONNECT(0, 4);
#if DT_INST_IRQ_HAS_IDX(0, 5)
	DMA_STM32_IRQ_CONNECT(0, 5);
#if DT_INST_IRQ_HAS_IDX(0, 6)
	DMA_STM32_IRQ_CONNECT(0, 6);
#if DT_INST_IRQ_HAS_IDX(0, 7)
	DMA_STM32_IRQ_CONNECT(0, 7);
#endif /* DT_INST_IRQ_HAS_IDX(0, 3) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 5) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 6) */
#endif /* DT_INST_IRQ_HAS_IDX(0, 7) */
#endif /* CONFIG_DMA_STM32_SHARED_IRQS */
/* Either 3 or 5 or 6 or 7 or 8 channels for DMA across all stm32 series. */
}

DMA_STM32_INIT_DEV(0);

#endif /* DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0)) */


#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(1))

DMA_STM32_DEFINE_IRQ_HANDLER(1, 0);
DMA_STM32_DEFINE_IRQ_HANDLER(1, 1);
DMA_STM32_DEFINE_IRQ_HANDLER(1, 2);
DMA_STM32_DEFINE_IRQ_HANDLER(1, 3);
#if DT_INST_IRQ_HAS_IDX(1, 4)
DMA_STM32_DEFINE_IRQ_HANDLER(1, 4);
#if DT_INST_IRQ_HAS_IDX(1, 5)
DMA_STM32_DEFINE_IRQ_HANDLER(1, 5);
#if DT_INST_IRQ_HAS_IDX(1, 6)
DMA_STM32_DEFINE_IRQ_HANDLER(1, 6);
#if DT_INST_IRQ_HAS_IDX(1, 7)
DMA_STM32_DEFINE_IRQ_HANDLER(1, 7);
#endif /* DT_INST_IRQ_HAS_IDX(1, 4) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 5) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 6) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 7) */

static void dma_stm32_config_irq_1(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifndef CONFIG_DMA_STM32_SHARED_IRQS
	DMA_STM32_IRQ_CONNECT(1, 0);
	DMA_STM32_IRQ_CONNECT(1, 1);
	DMA_STM32_IRQ_CONNECT(1, 2);
	DMA_STM32_IRQ_CONNECT(1, 3);
#if DT_INST_IRQ_HAS_IDX(1, 4)
	DMA_STM32_IRQ_CONNECT(1, 4);
#if DT_INST_IRQ_HAS_IDX(1, 5)
	DMA_STM32_IRQ_CONNECT(1, 5);
#if DT_INST_IRQ_HAS_IDX(1, 6)
	DMA_STM32_IRQ_CONNECT(1, 6);
#if DT_INST_IRQ_HAS_IDX(1, 7)
	DMA_STM32_IRQ_CONNECT(1, 7);
#endif /* DT_INST_IRQ_HAS_IDX(1, 4) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 5) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 6) */
#endif /* DT_INST_IRQ_HAS_IDX(1, 7) */
#endif /* CONFIG_DMA_STM32_SHARED_IRQS */
/*
 * Either 5 or 6 or 7 or 8 channels for DMA across all stm32 series.
 * STM32F0 and STM32G0: if dma2 exits, the channel interrupts overlap with dma1
 */
}

DMA_STM32_INIT_DEV(1);

#endif /* DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(1)) */
