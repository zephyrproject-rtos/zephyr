/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_gdma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_esp32_gdma, CONFIG_DMA_LOG_LEVEL);

#include <hal/gdma_hal.h>
#include <hal/gdma_ll.h>
#include <soc/gdma_channel.h>
#include <hal/dma_types.h>

#include <soc.h>
#include <esp_memory_utils.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/clock_control.h>
#ifndef CONFIG_SOC_SERIES_ESP32C3
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#endif

#ifdef CONFIG_SOC_SERIES_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

#define DMA_MAX_CHANNEL SOC_GDMA_PAIRS_PER_GROUP
#define ESP_DMA_M2M_ON  0
#define ESP_DMA_M2M_OFF 1

struct dma_esp32_data {
	gdma_hal_context_t hal;
};

enum dma_channel_dir {
	DMA_RX,
	DMA_TX,
	DMA_UNCONFIGURED
};

struct dma_esp32_channel {
	uint8_t dir;
	uint8_t channel_id;
	int host_id;
	int periph_id;
	dma_callback_t cb;
	void *user_data;
	dma_descriptor_t desc;
#if defined(CONFIG_SOC_SERIES_ESP32S3)
	struct intr_handle_data_t *intr_handle;
#endif
};

struct dma_esp32_config {
	int *irq_src;
	uint8_t irq_size;
	void **irq_handlers;
	uint8_t dma_channel_max;
	uint8_t sram_alignment;
	struct dma_esp32_channel dma_channel[DMA_MAX_CHANNEL * 2];
	void (*config_irq)(const struct device *dev);
	struct device *src_dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static void IRAM_ATTR dma_esp32_isr_handle_rx(const struct device *dev,
					      struct dma_esp32_channel *rx, uint32_t intr_status)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

	gdma_ll_rx_clear_interrupt_status(data->hal.dev, rx->channel_id, intr_status);

	if (intr_status & (GDMA_LL_EVENT_RX_SUC_EOF | GDMA_LL_EVENT_RX_DONE)) {
		intr_status &= ~(GDMA_LL_EVENT_RX_SUC_EOF | GDMA_LL_EVENT_RX_DONE);
	}

	if (rx->cb) {
		rx->cb(dev, rx->user_data, rx->channel_id*2, -intr_status);
	}
}

static void IRAM_ATTR dma_esp32_isr_handle_tx(const struct device *dev,
					      struct dma_esp32_channel *tx, uint32_t intr_status)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

	gdma_ll_tx_clear_interrupt_status(data->hal.dev, tx->channel_id, intr_status);

	intr_status &= ~(GDMA_LL_EVENT_TX_TOTAL_EOF | GDMA_LL_EVENT_TX_DONE | GDMA_LL_EVENT_TX_EOF);

	if (tx->cb) {
		tx->cb(dev, tx->user_data, tx->channel_id*2 + 1, -intr_status);
	}
}

static void IRAM_ATTR dma_esp32_isr_handle(const struct device *dev, uint8_t rx_id, uint8_t tx_id)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel_rx = &config->dma_channel[rx_id];
	struct dma_esp32_channel *dma_channel_tx = &config->dma_channel[tx_id];
	uint32_t intr_status = 0;

	intr_status = gdma_ll_rx_get_interrupt_status(data->hal.dev, dma_channel_rx->channel_id);
	if (intr_status) {
		dma_esp32_isr_handle_rx(dev, dma_channel_rx, intr_status);
	}

	intr_status = gdma_ll_tx_get_interrupt_status(data->hal.dev, dma_channel_tx->channel_id);
	if (intr_status) {
		dma_esp32_isr_handle_tx(dev, dma_channel_tx, intr_status);
	}
}

#if defined(CONFIG_SOC_SERIES_ESP32C3)
static int dma_esp32_enable_interrupt(const struct device *dev,
				      struct dma_esp32_channel *dma_channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;

	return esp_intr_enable(config->irq_src[dma_channel->channel_id]);
}

static int dma_esp32_disable_interrupt(const struct device *dev,
				       struct dma_esp32_channel *dma_channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;

	return esp_intr_disable(config->irq_src[dma_channel->channel_id]);
}
#else
static int dma_esp32_enable_interrupt(const struct device *dev,
				      struct dma_esp32_channel *dma_channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;

	return esp_intr_enable(dma_channel->intr_handle);
}

static int dma_esp32_disable_interrupt(const struct device *dev,
				       struct dma_esp32_channel *dma_channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;

	return esp_intr_disable(dma_channel->intr_handle);
}

#endif
static int dma_esp32_config_rx_descriptor(struct dma_esp32_channel *dma_channel,
					   struct dma_block_config *block)
{
	if (!esp_ptr_dma_capable((uint32_t *)block->dest_address)) {
		LOG_ERR("Rx buffer not in DMA capable memory: %p", (uint32_t *)block->dest_address);
		return -EINVAL;
	}

	memset(&dma_channel->desc, 0, sizeof(dma_channel->desc));
	dma_channel->desc.buffer = (void *)block->dest_address;
	dma_channel->desc.dw0.size = block->block_size;
	dma_channel->desc.dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;

	return 0;
}

static int dma_esp32_config_rx(const struct device *dev, struct dma_esp32_channel *dma_channel,
				struct dma_config *config_dma)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_block_config *block = config_dma->head_block;

	dma_channel->dir = DMA_RX;

	gdma_ll_rx_reset_channel(data->hal.dev, dma_channel->channel_id);

	gdma_ll_rx_connect_to_periph(
		data->hal.dev, dma_channel->channel_id,
		dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0 ? ESP_DMA_M2M_ON
								    : ESP_DMA_M2M_OFF,
		dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0 ? ESP_DMA_M2M_ON
								    : dma_channel->periph_id);

	if (config_dma->dest_burst_length) {
		/*
		 * RX channel burst mode depends on specific data alignment
		 */
		gdma_ll_rx_enable_data_burst(data->hal.dev, dma_channel->channel_id,
					     config->sram_alignment >= 4);
		gdma_ll_rx_enable_descriptor_burst(data->hal.dev, dma_channel->channel_id,
						   config->sram_alignment >= 4);
	}

	dma_channel->cb = config_dma->dma_callback;
	dma_channel->user_data = config_dma->user_data;

	gdma_ll_rx_clear_interrupt_status(data->hal.dev, dma_channel->channel_id, UINT32_MAX);
	gdma_ll_rx_enable_interrupt(data->hal.dev, dma_channel->channel_id, UINT32_MAX,
				    config_dma->dma_callback != NULL);

	return dma_esp32_config_rx_descriptor(dma_channel, config_dma->head_block);
}

static int dma_esp32_config_tx_descriptor(struct dma_esp32_channel *dma_channel,
					   struct dma_block_config *block)
{
	if (!esp_ptr_dma_capable((uint32_t *)block->source_address)) {
		LOG_ERR("Tx buffer not in DMA capable memory: %p",
			(uint32_t *)block->source_address);
		return -EINVAL;
	}

	memset(&dma_channel->desc, 0, sizeof(dma_channel->desc));
	dma_channel->desc.buffer = (void *)block->source_address;
	dma_channel->desc.dw0.size = block->block_size;
	dma_channel->desc.dw0.length = block->block_size;
	dma_channel->desc.dw0.suc_eof = 1;
	dma_channel->desc.dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;

	return 0;
}

static int dma_esp32_config_tx(const struct device *dev, struct dma_esp32_channel *dma_channel,
				struct dma_config *config_dma)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_block_config *block = config_dma->head_block;

	dma_channel->dir = DMA_TX;

	gdma_ll_tx_reset_channel(data->hal.dev, dma_channel->channel_id);

	gdma_ll_tx_connect_to_periph(
		data->hal.dev, dma_channel->channel_id,
		dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0 ? ESP_DMA_M2M_ON
								    : ESP_DMA_M2M_OFF,
		dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0 ? ESP_DMA_M2M_ON
								    : dma_channel->periph_id);

	/*
	 * TX channel can always enable burst mode, no matter data alignment
	 */
	if (config_dma->source_burst_length) {
		gdma_ll_tx_enable_data_burst(data->hal.dev, dma_channel->channel_id, true);
		gdma_ll_tx_enable_descriptor_burst(data->hal.dev, dma_channel->channel_id, true);
	}

	dma_channel->cb = config_dma->dma_callback;
	dma_channel->user_data = config_dma->user_data;

	gdma_ll_tx_clear_interrupt_status(data->hal.dev, dma_channel->channel_id, UINT32_MAX);

	gdma_ll_tx_enable_interrupt(data->hal.dev, dma_channel->channel_id, GDMA_LL_EVENT_TX_EOF,
				    config_dma->dma_callback != NULL);

	return dma_esp32_config_tx_descriptor(dma_channel, config_dma->head_block);
}

static int dma_esp32_config(const struct device *dev, uint32_t channel,
			     struct dma_config *config_dma)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];
	int ret = 0;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (!config_dma) {
		return -EINVAL;
	}

	if (config_dma->source_burst_length != config_dma->dest_burst_length) {
		LOG_ERR("Source and destination burst lengths must be equal");
		return -EINVAL;
	}

	dma_channel->periph_id = config_dma->channel_direction == MEMORY_TO_MEMORY
					 ? SOC_GDMA_TRIG_PERIPH_M2M0
					 : config_dma->dma_slot;

	dma_channel->channel_id = channel / 2;

	switch (config_dma->channel_direction) {
	case MEMORY_TO_MEMORY:
		/*
		 *	Create both Tx and Rx stream on the same channel_id
		 */
		struct dma_esp32_channel *dma_channel_rx =
			&config->dma_channel[dma_channel->channel_id * 2];
		struct dma_esp32_channel *dma_channel_tx =
			&config->dma_channel[(dma_channel->channel_id * 2) + 1];

		dma_channel_rx->channel_id = dma_channel->channel_id;
		dma_channel_tx->channel_id = dma_channel->channel_id;

		dma_channel_rx->periph_id = dma_channel->periph_id;
		dma_channel_tx->periph_id = dma_channel->periph_id;

		ret = dma_esp32_config_rx(dev, dma_channel_rx, config_dma);
		ret = dma_esp32_config_tx(dev, dma_channel_tx, config_dma);
		break;
	case PERIPHERAL_TO_MEMORY:
		ret = dma_esp32_config_rx(dev, dma_channel, config_dma);
		break;
	case MEMORY_TO_PERIPHERAL:
		ret = dma_esp32_config_tx(dev, dma_channel, config_dma);
		break;
	default:
		LOG_ERR("Invalid Channel direction");
		return -EINVAL;
	}

	return ret;
}

static int dma_esp32_start(const struct device *dev, uint32_t channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_esp32_enable_interrupt(dev, dma_channel)) {
		return -EINVAL;
	}

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		struct dma_esp32_channel *dma_channel_rx =
			&config->dma_channel[dma_channel->channel_id * 2];
		struct dma_esp32_channel *dma_channel_tx =
			&config->dma_channel[(dma_channel->channel_id * 2) + 1];

		gdma_ll_rx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
					 (int32_t)&dma_channel_rx->desc);
		gdma_ll_rx_start(data->hal.dev, dma_channel->channel_id);

		gdma_ll_tx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
					 (int32_t)&dma_channel_tx->desc);
		gdma_ll_tx_start(data->hal.dev, dma_channel->channel_id);
	} else {
		if (dma_channel->dir == DMA_RX) {
			gdma_ll_rx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
						 (int32_t)&dma_channel->desc);
			gdma_ll_rx_start(data->hal.dev, dma_channel->channel_id);
		} else if (dma_channel->dir == DMA_TX) {
			gdma_ll_tx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
						 (int32_t)&dma_channel->desc);
			gdma_ll_tx_start(data->hal.dev, dma_channel->channel_id);
		} else {
			LOG_ERR("Channel %d is not configured", channel);
			return -EINVAL;
		}
	}

	return 0;
}

static int dma_esp32_stop(const struct device *dev, uint32_t channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_esp32_disable_interrupt(dev, dma_channel)) {
		return -EINVAL;
	}

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		gdma_ll_rx_stop(data->hal.dev, dma_channel->channel_id);
		gdma_ll_tx_stop(data->hal.dev, dma_channel->channel_id);
	}

	if (dma_channel->dir == DMA_RX) {
		gdma_ll_rx_stop(data->hal.dev, dma_channel->channel_id);
	} else if (dma_channel->dir == DMA_TX) {
		gdma_ll_tx_stop(data->hal.dev, dma_channel->channel_id);
	}

	return 0;
}

static int dma_esp32_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *status)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (!status) {
		return -EINVAL;
	}

	if (dma_channel->dir == DMA_RX) {
		status->busy = !gdma_ll_rx_is_fsm_idle(data->hal.dev, dma_channel->channel_id);
		status->dir = PERIPHERAL_TO_MEMORY;
		status->read_position = dma_channel->desc.dw0.length;
	} else if (dma_channel->dir == DMA_TX) {
		status->busy = !gdma_ll_tx_is_fsm_idle(data->hal.dev, dma_channel->channel_id);
		status->dir = MEMORY_TO_PERIPHERAL;
		status->write_position = dma_channel->desc.dw0.length;
		status->total_copied = dma_channel->desc.dw0.length;
		status->pending_length = dma_channel->desc.dw0.size - dma_channel->desc.dw0.length;
	}

	return 0;
}

static int dma_esp32_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			    size_t size)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];
	struct dma_block_config block = {0};
	int err = 0;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_channel->dir == DMA_RX) {
		gdma_ll_rx_reset_channel(data->hal.dev, dma_channel->channel_id);
		block.block_size = size;
		block.dest_address = dst;
		err = dma_esp32_config_rx_descriptor(dma_channel, &block);
		if (err) {
			LOG_ERR("Error reloading RX channel (%d)", err);
			return err;
		}
	} else if (dma_channel->dir == DMA_TX) {
		gdma_ll_tx_reset_channel(data->hal.dev, dma_channel->channel_id);
		block.block_size = size;
		block.source_address = src;
		err = dma_esp32_config_tx_descriptor(dma_channel, &block);
		if (err) {
			LOG_ERR("Error reloading TX channel (%d)", err);
			return err;
		}
	}

	return 0;
}

#if defined(CONFIG_SOC_SERIES_ESP32C3)
static int dma_esp32_configure_irq(const struct device *dev)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;

	for (uint8_t i = 0; i < config->irq_size; i++) {
		int ret = esp_intr_alloc(config->irq_src[i],
					 0,
					 (ISR_HANDLER)config->irq_handlers[i],
					 (void *)dev,
					 NULL);
		if (ret != 0) {
			LOG_ERR("Could not allocate interrupt handler");
			return ret;
		}
	}

	return 0;
}
#else
static int dma_esp32_configure_irq(const struct device *dev)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *)dev->data;
	struct dma_esp32_channel *dma_channel;

	for (uint8_t i = 0; i < config->irq_size; i++) {
		dma_channel = &config->dma_channel[i];
		int ret = esp_intr_alloc(config->irq_src[i],
					 0,
					 (ISR_HANDLER)config->irq_handlers[i / 2],
					 (void *)dev,
					 &dma_channel->intr_handle);
		if (ret != 0) {
			LOG_ERR("Could not allocate interrupt handler");
			return ret;
		}
	}

	return 0;
}

#endif
static int dma_esp32_init(const struct device *dev)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *)dev->data;
	struct dma_esp32_channel *dma_channel;
	int ret = 0;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}

	ret = dma_esp32_configure_irq(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure IRQ (%d)", ret);
		return ret;
	}

	for (uint8_t i = 0; i < DMA_MAX_CHANNEL * 2; i++) {
		dma_channel = &config->dma_channel[i];
		dma_channel->cb = NULL;
		dma_channel->dir = DMA_UNCONFIGURED;
		dma_channel->periph_id = ESP_GDMA_TRIG_PERIPH_INVALID;
		memset(&dma_channel->desc, 0, sizeof(dma_descriptor_t));
	}

	gdma_hal_init(&data->hal, 0);
	gdma_ll_enable_clock(data->hal.dev, true);

	return 0;
}

static const struct dma_driver_api dma_esp32_api = {
	.config = dma_esp32_config,
	.start = dma_esp32_start,
	.stop = dma_esp32_stop,
	.get_status = dma_esp32_get_status,
	.reload = dma_esp32_reload,
};

#define DMA_ESP32_DEFINE_IRQ_HANDLER(channel)                                                      \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel(                     \
		const struct device *dev)                                                          \
	{                                                                                          \
		dma_esp32_isr_handle(dev, channel * 2, channel * 2 + 1);                           \
	}

#define ESP32_DMA_HANDLER(channel) dma_esp32_isr_##channel

DMA_ESP32_DEFINE_IRQ_HANDLER(0)
DMA_ESP32_DEFINE_IRQ_HANDLER(1)
DMA_ESP32_DEFINE_IRQ_HANDLER(2)
#if DMA_MAX_CHANNEL >= 5
DMA_ESP32_DEFINE_IRQ_HANDLER(3)
DMA_ESP32_DEFINE_IRQ_HANDLER(4)
#endif

static void *irq_handlers[] = {
	ESP32_DMA_HANDLER(0),
	ESP32_DMA_HANDLER(1),
	ESP32_DMA_HANDLER(2),
#if DMA_MAX_CHANNEL >= 5
	ESP32_DMA_HANDLER(3),
	ESP32_DMA_HANDLER(4),
#endif
	};

#define DMA_ESP32_INIT(idx)                                                                        \
	static int irq_numbers[] = DT_INST_PROP(idx, interrupts);                                  \
	static struct dma_esp32_config dma_config_##idx = {                                        \
		.irq_src = irq_numbers,                                                            \
		.irq_size = ARRAY_SIZE(irq_numbers),                                               \
		.irq_handlers = irq_handlers,                                                      \
		.dma_channel_max = DT_INST_PROP(idx, dma_channels),                                \
		.sram_alignment = DT_INST_PROP(idx, dma_buf_addr_alignment),                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys = (void *)DT_INST_CLOCKS_CELL(idx, offset),                          \
	};                                                                                         \
	static struct dma_esp32_data dma_data_##idx = {                                            \
		.hal =                                                                             \
			{                                                                          \
				.dev = (gdma_dev_t *)DT_INST_REG_ADDR(idx),                        \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &dma_esp32_init, NULL, &dma_data_##idx, &dma_config_##idx,      \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &dma_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_ESP32_INIT)
