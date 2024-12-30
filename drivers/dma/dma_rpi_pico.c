/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#if defined(CONFIG_SOC_SERIES_RP2040)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2040.h>
#elif defined(CONFIG_SOC_SERIES_RP2350)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2350.h>
#endif

#include <hardware/dma.h>

#define DT_DRV_COMPAT raspberrypi_pico_dma

#define DMA_INT_ERROR_FLAGS                                                                        \
	(DMA_CH0_CTRL_TRIG_AHB_ERROR_BITS | DMA_CH0_CTRL_TRIG_READ_ERROR_BITS |                    \
	 DMA_CH0_CTRL_TRIG_WRITE_ERROR_BITS)

LOG_MODULE_REGISTER(dma_rpi_pico, CONFIG_DMA_LOG_LEVEL);

struct dma_rpi_pico_config {
	uint32_t reg;
	uint32_t channels;
	struct reset_dt_spec reset;
	void (*irq_configure)(void);
	uint32_t *irq0_channels;
	size_t irq0_channels_size;
};

struct dma_rpi_pico_channel {
	dma_callback_t callback;
	void *user_data;
	uint32_t direction;
	dma_channel_config config;
	void *source_address;
	void *dest_address;
	size_t block_size;
};

struct dma_rpi_pico_data {
	struct dma_context ctx;
	struct dma_rpi_pico_channel *channels;
};

/*
 * Register access functions
 */

static inline void rpi_pico_dma_channel_clear_error_flags(const struct device *dev,
							  uint32_t channel)
{
	const struct dma_rpi_pico_config *cfg = dev->config;

	((dma_hw_t *)cfg->reg)->ch[channel].al1_ctrl &= ~DMA_INT_ERROR_FLAGS;
}

static inline uint32_t rpi_pico_dma_channel_get_error_flags(const struct device *dev,
							    uint32_t channel)
{
	const struct dma_rpi_pico_config *cfg = dev->config;

	return ((dma_hw_t *)cfg->reg)->ch[channel].al1_ctrl & DMA_INT_ERROR_FLAGS;
}

static inline void rpi_pico_dma_channel_abort(const struct device *dev, uint32_t channel)
{
	const struct dma_rpi_pico_config *cfg = dev->config;

	((dma_hw_t *)cfg->reg)->abort = BIT(channel);
}

/*
 * Utility functions
 */

static inline uint32_t dma_rpi_pico_transfer_size(uint32_t width)
{
	switch (width) {
	case 4:
		return DMA_SIZE_32;
	case 2:
		return DMA_SIZE_16;
	default:
		return DMA_SIZE_8;
	}
}

static inline uint32_t dma_rpi_pico_channel_irq(const struct device *dev, uint32_t channel)
{
	const struct dma_rpi_pico_config *cfg = dev->config;

	for (size_t i = 0; i < cfg->irq0_channels_size; i++) {
		if (cfg->irq0_channels[i] == channel) {
			return 0;
		}
	}

	return 1;
}

/*
 * API functions
 */

static int dma_rpi_pico_config(const struct device *dev, uint32_t channel,
			       struct dma_config *dma_cfg)
{
	const struct dma_rpi_pico_config *cfg = dev->config;
	struct dma_rpi_pico_data *data = dev->data;

	if (channel >= cfg->channels) {
		LOG_ERR("channel must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels, channel);
		return -EINVAL;
	}

	if (dma_cfg->block_count != 1) {
		LOG_ERR("chained block transfer not supported.");
		return -ENOTSUP;
	}

	if (dma_cfg->channel_priority > 3) {
		LOG_ERR("channel_priority must be < 4 (%" PRIu32 ")", dma_cfg->channel_priority);
		return -EINVAL;
	}

	if (dma_cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("source_addr_adj not supported DMA_ADDR_ADJ_DECREMENT");
		return -ENOTSUP;
	}

	if (dma_cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("dest_addr_adj not supported DMA_ADDR_ADJ_DECREMENT");
		return -ENOTSUP;
	}

	if (dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_INCREMENT &&
	    dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
		LOG_ERR("invalid source_addr_adj %" PRIu16, dma_cfg->head_block->source_addr_adj);
		return -ENOTSUP;
	}
	if (dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_INCREMENT &&
	    dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
		LOG_ERR("invalid dest_addr_adj %" PRIu16, dma_cfg->head_block->dest_addr_adj);
		return -ENOTSUP;
	}

	if (dma_cfg->source_data_size != 1 && dma_cfg->source_data_size != 2 &&
	    dma_cfg->source_data_size != 4) {
		LOG_ERR("source_data_size must be 1, 2, or 4 (%" PRIu32 ")",
			dma_cfg->source_data_size);
		return -EINVAL;
	}

	if (dma_cfg->source_data_size != dma_cfg->dest_data_size) {
		return -EINVAL;
	}

	if (dma_cfg->dest_data_size != 1 && dma_cfg->dest_data_size != 2 &&
	    dma_cfg->dest_data_size != 4) {
		LOG_ERR("dest_data_size must be 1, 2, or 4 (%" PRIu32 ")", dma_cfg->dest_data_size);
		return -EINVAL;
	}

	if (dma_cfg->channel_direction > PERIPHERAL_TO_MEMORY) {
		LOG_ERR("channel_direction must be MEMORY_TO_MEMORY, "
			"MEMORY_TO_PERIPHERAL or PERIPHERAL_TO_MEMORY (%" PRIu32 ")",
			dma_cfg->channel_direction);
		return -ENOTSUP;
	}

	data->channels[channel].config = dma_channel_get_default_config(channel);

	data->channels[channel].source_address = (void *)dma_cfg->head_block->source_address;
	data->channels[channel].dest_address = (void *)dma_cfg->head_block->dest_address;
	data->channels[channel].block_size = dma_cfg->head_block->block_size;
	channel_config_set_read_increment(&data->channels[channel].config,
					  dma_cfg->head_block->source_addr_adj ==
						  DMA_ADDR_ADJ_INCREMENT);
	channel_config_set_write_increment(&data->channels[channel].config,
					   dma_cfg->head_block->dest_addr_adj ==
						   DMA_ADDR_ADJ_INCREMENT);
	channel_config_set_transfer_data_size(
		&data->channels[channel].config,
		dma_rpi_pico_transfer_size(dma_cfg->source_data_size));
	channel_config_set_dreq(&data->channels[channel].config,
				RPI_PICO_DMA_SLOT_TO_DREQ(dma_cfg->dma_slot));
	channel_config_set_high_priority(&data->channels[channel].config,
					 !!(dma_cfg->channel_priority));

	data->channels[channel].callback = dma_cfg->dma_callback;
	data->channels[channel].user_data = dma_cfg->user_data;
	data->channels[channel].direction = dma_cfg->channel_direction;

	return 0;
}

static int dma_rpi_pico_reload(const struct device *dev, uint32_t ch, uint32_t src, uint32_t dst,
			       size_t size)
{
	const struct dma_rpi_pico_config *cfg = dev->config;
	struct dma_rpi_pico_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("reload channel must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels, ch);
		return -EINVAL;
	}

	if (dma_channel_is_busy(ch)) {
		return -EBUSY;
	}

	data->channels[ch].source_address = (void *)src;
	data->channels[ch].dest_address = (void *)dst;
	data->channels[ch].block_size = size;
	dma_channel_configure(ch, &data->channels[ch].config, data->channels[ch].dest_address,
			      data->channels[ch].source_address, data->channels[ch].block_size,
			      true);

	return 0;
}

static int dma_rpi_pico_start(const struct device *dev, uint32_t ch)
{
	const struct dma_rpi_pico_config *cfg = dev->config;
	struct dma_rpi_pico_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("start channel must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels, ch);
		return -EINVAL;
	}

	dma_irqn_acknowledge_channel(dma_rpi_pico_channel_irq(dev, ch), ch);
	dma_irqn_set_channel_enabled(dma_rpi_pico_channel_irq(dev, ch), ch, true);

	dma_channel_configure(ch, &data->channels[ch].config, data->channels[ch].dest_address,
			      data->channels[ch].source_address, data->channels[ch].block_size,
			      true);

	return 0;
}

static int dma_rpi_pico_stop(const struct device *dev, uint32_t ch)
{
	const struct dma_rpi_pico_config *cfg = dev->config;

	if (ch >= cfg->channels) {
		LOG_ERR("stop channel must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels, ch);
		return -EINVAL;
	}

	dma_irqn_set_channel_enabled(dma_rpi_pico_channel_irq(dev, ch), ch, false);
	rpi_pico_dma_channel_clear_error_flags(dev, ch);

	/*
	 * Considering the possibility of being called in an interrupt context,
	 * it does not wait until the abort bit becomes clear.
	 * Ensure the busy status is canceled with dma_get_status
	 * before the next transfer starts.
	 */
	rpi_pico_dma_channel_abort(dev, ch);

	return 0;
}

static int dma_rpi_pico_get_status(const struct device *dev, uint32_t ch, struct dma_status *stat)
{
	const struct dma_rpi_pico_config *cfg = dev->config;
	struct dma_rpi_pico_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("channel must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels, ch);
		return -EINVAL;
	}

	stat->pending_length = 0;
	stat->dir = data->channels[ch].direction;
	stat->busy = dma_channel_is_busy(ch);

	return 0;
}

static bool dma_rpi_pico_api_chan_filter(const struct device *dev, int ch, void *filter_param)
{
	uint32_t filter;

	if (!filter_param) {
		return true;
	}

	filter = *((uint32_t *)filter_param);

	return (filter & BIT(ch));
}

static int dma_rpi_pico_init(const struct device *dev)
{
	const struct dma_rpi_pico_config *cfg = dev->config;

	(void)reset_line_toggle_dt(&cfg->reset);

	cfg->irq_configure();

	return 0;
}

static void dma_rpi_pico_isr(const struct device *dev)
{
	const struct dma_rpi_pico_config *cfg = dev->config;
	struct dma_rpi_pico_data *data = dev->data;
	int err = 0;

	for (uint32_t i = 0; i < cfg->channels; i++) {
		if (!dma_irqn_get_channel_status(dma_rpi_pico_channel_irq(dev, i), i)) {
			continue;
		}

		if (rpi_pico_dma_channel_get_error_flags(dev, i)) {
			err = -EIO;
		}

		dma_irqn_acknowledge_channel(dma_rpi_pico_channel_irq(dev, i), i);
		dma_irqn_set_channel_enabled(dma_rpi_pico_channel_irq(dev, i), i, false);
		rpi_pico_dma_channel_clear_error_flags(dev, i);

		if (data->channels[i].callback) {
			data->channels[i].callback(dev, data->channels[i].user_data, i, err);
		}
	}
}

static DEVICE_API(dma, dma_rpi_pico_driver_api) = {
	.config = dma_rpi_pico_config,
	.reload = dma_rpi_pico_reload,
	.start = dma_rpi_pico_start,
	.stop = dma_rpi_pico_stop,
	.get_status = dma_rpi_pico_get_status,
	.chan_filter = dma_rpi_pico_api_chan_filter,
};

#define IRQ_CONFIGURE(n, inst)                                                                     \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dma_rpi_pico_isr, DEVICE_DT_INST_GET(inst), 0);                                \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, IRQ_CONFIGURE, (), inst)

#define RPI_PICO_DMA_INIT(inst)                                                                    \
	static void dma_rpi_pico##inst##_irq_configure(void)                                       \
	{                                                                                          \
		CONFIGURE_ALL_IRQS(inst, DT_NUM_IRQS(DT_DRV_INST(inst)));                          \
	}                                                                                          \
	static uint32_t dma_rpi_pico##inst##_irq0_channels[] =                                     \
		DT_INST_PROP_OR(inst, irq0_channels, {0});                                         \
	static const struct dma_rpi_pico_config dma_rpi_pico##inst##_config = {                    \
		.reg = DT_INST_REG_ADDR(inst),                                                     \
		.channels = DT_INST_PROP(inst, dma_channels),                                      \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.irq_configure = dma_rpi_pico##inst##_irq_configure,                               \
		.irq0_channels = dma_rpi_pico##inst##_irq0_channels,                               \
		.irq0_channels_size = ARRAY_SIZE(dma_rpi_pico##inst##_irq0_channels),              \
	};                                                                                         \
	static struct dma_rpi_pico_channel                                                         \
		dma_rpi_pico##inst##_channels[DT_INST_PROP(inst, dma_channels)];                   \
	ATOMIC_DEFINE(dma_rpi_pico_atomic##inst, DT_INST_PROP(inst, dma_channels));                \
	static struct dma_rpi_pico_data dma_rpi_pico##inst##_data = {                              \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_rpi_pico_atomic##inst,                               \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
		.channels = dma_rpi_pico##inst##_channels,                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &dma_rpi_pico_init, NULL, &dma_rpi_pico##inst##_data,          \
			      &dma_rpi_pico##inst##_config, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, \
			      &dma_rpi_pico_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_PICO_DMA_INIT)
