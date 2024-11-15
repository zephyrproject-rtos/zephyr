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
#if defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32C6)
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif

#if defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32C6)
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

struct irq_config {
	uint8_t irq_source;
	uint8_t irq_priority;
	int irq_flags;
};

struct dma_esp32_channel {
	uint8_t dir;
	uint8_t channel_id;
	int host_id;
	int periph_id;
	dma_callback_t cb;
	void *user_data;
	dma_descriptor_t desc_list[CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM];
};

struct dma_esp32_config {
	struct irq_config *irq_config;
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
	uint32_t status;

	gdma_ll_rx_clear_interrupt_status(data->hal.dev, rx->channel_id, intr_status);

	if (intr_status == (GDMA_LL_EVENT_RX_SUC_EOF | GDMA_LL_EVENT_RX_DONE)) {
		status = DMA_STATUS_COMPLETE;
	} else if (intr_status == GDMA_LL_EVENT_RX_DONE) {
		status = DMA_STATUS_BLOCK;
#if defined(CONFIG_SOC_SERIES_ESP32S3)
	} else if (intr_status == GDMA_LL_EVENT_RX_WATER_MARK) {
		status = DMA_STATUS_BLOCK;
#endif
	} else {
		status = -intr_status;
	}

	if (rx->cb) {
		rx->cb(dev, rx->user_data, rx->channel_id * 2, status);
	}
}

static void IRAM_ATTR dma_esp32_isr_handle_tx(const struct device *dev,
					      struct dma_esp32_channel *tx, uint32_t intr_status)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

	gdma_ll_tx_clear_interrupt_status(data->hal.dev, tx->channel_id, intr_status);

	intr_status &= ~(GDMA_LL_EVENT_TX_TOTAL_EOF | GDMA_LL_EVENT_TX_DONE | GDMA_LL_EVENT_TX_EOF);

	if (tx->cb) {
		tx->cb(dev, tx->user_data, tx->channel_id * 2 + 1, -intr_status);
	}
}

#if !defined(CONFIG_SOC_SERIES_ESP32C6) && !defined(CONFIG_SOC_SERIES_ESP32S3)
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
#endif

static int dma_esp32_config_rx_descriptor(struct dma_esp32_channel *dma_channel,
						struct dma_block_config *block)
{
	if (!block) {
		LOG_ERR("At least one dma block is required");
		return -EINVAL;
	}

	if (!esp_ptr_dma_capable((uint32_t *)block->dest_address)
#if defined(CONFIG_ESP_SPIRAM)
	&& !esp_ptr_dma_ext_capable((uint32_t *)block->dest_address)
#endif
	) {
		LOG_ERR("Rx buffer not in DMA capable memory: %p", (uint32_t *)block->dest_address);
		return -EINVAL;
	}

	dma_descriptor_t *desc_iter = dma_channel->desc_list;

	for (int i = 0; i < CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM; ++i) {
		if (block->block_size > DMA_DESCRIPTOR_BUFFER_MAX_SIZE) {
			LOG_ERR("Size of block %d is too large", i);
			return -EINVAL;
		}
		memset(desc_iter, 0, sizeof(dma_descriptor_t));
		desc_iter->buffer = (void *)block->dest_address;
		desc_iter->dw0.size = block->block_size;
		desc_iter->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
		if (!block->next_block) {
			desc_iter->next = NULL;
			break;
		}
		desc_iter->next = desc_iter + 1;
		desc_iter += 1;
		block = block->next_block;
	}

	if (desc_iter->next) {
		memset(dma_channel->desc_list, 0, sizeof(dma_channel->desc_list));
		LOG_ERR("Too many dma blocks. Increase CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM");
		return -EINVAL;
	}

	return 0;
}

static int dma_esp32_config_rx(const struct device *dev, struct dma_esp32_channel *dma_channel,
				struct dma_config *config_dma)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

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
	if (!block) {
		LOG_ERR("At least one dma block is required");
		return -EINVAL;
	}

	if (!esp_ptr_dma_capable((uint32_t *)block->source_address)
#if defined(CONFIG_ESP_SPIRAM)
	&& !esp_ptr_dma_ext_capable((uint32_t *)block->source_address)
#endif
	) {
		LOG_ERR("Tx buffer not in DMA capable memory: %p",
			(uint32_t *)block->source_address);
		return -EINVAL;
	}

	dma_descriptor_t *desc_iter = dma_channel->desc_list;

	for (int i = 0; i < CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM; ++i) {
		if (block->block_size > DMA_DESCRIPTOR_BUFFER_MAX_SIZE) {
			LOG_ERR("Size of block %d is too large", i);
			return -EINVAL;
		}
		memset(desc_iter, 0, sizeof(dma_descriptor_t));
		desc_iter->buffer = (void *)block->source_address;
		desc_iter->dw0.size = block->block_size;
		desc_iter->dw0.length = block->block_size;
		desc_iter->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
		if (!block->next_block) {
			desc_iter->next = NULL;
			desc_iter->dw0.suc_eof = 1;
			break;
		}
		desc_iter->next = desc_iter + 1;
		desc_iter += 1;
		block = block->next_block;
	}

	if (desc_iter->next) {
		memset(dma_channel->desc_list, 0, sizeof(dma_channel->desc_list));
		LOG_ERR("Too many dma blocks. Increase CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM");
		return -EINVAL;
	}

	return 0;
}

static int dma_esp32_config_tx(const struct device *dev, struct dma_esp32_channel *dma_channel,
				struct dma_config *config_dma)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

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

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		struct dma_esp32_channel *dma_channel_rx =
			&config->dma_channel[dma_channel->channel_id * 2];
		struct dma_esp32_channel *dma_channel_tx =
			&config->dma_channel[(dma_channel->channel_id * 2) + 1];

		gdma_ll_rx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
					UINT32_MAX, true);
		gdma_ll_tx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
					GDMA_LL_EVENT_TX_EOF, true);

		gdma_ll_rx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
					 (int32_t)dma_channel_rx->desc_list);
		gdma_ll_rx_start(data->hal.dev, dma_channel->channel_id);

		gdma_ll_tx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
					 (int32_t)dma_channel_tx->desc_list);
		gdma_ll_tx_start(data->hal.dev, dma_channel->channel_id);
	} else {
		if (dma_channel->dir == DMA_RX) {
			gdma_ll_rx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
					UINT32_MAX, true);
			gdma_ll_rx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
						 (int32_t)dma_channel->desc_list);
			gdma_ll_rx_start(data->hal.dev, dma_channel->channel_id);
		} else if (dma_channel->dir == DMA_TX) {
			gdma_ll_tx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
					GDMA_LL_EVENT_TX_EOF, true);
			gdma_ll_tx_set_desc_addr(data->hal.dev, dma_channel->channel_id,
						 (int32_t)dma_channel->desc_list);
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

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		gdma_ll_rx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
				UINT32_MAX, false);
		gdma_ll_tx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
				GDMA_LL_EVENT_TX_EOF, false);
		gdma_ll_rx_stop(data->hal.dev, dma_channel->channel_id);
		gdma_ll_tx_stop(data->hal.dev, dma_channel->channel_id);
	}

	if (dma_channel->dir == DMA_RX) {
		gdma_ll_rx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
				UINT32_MAX, false);
		gdma_ll_rx_stop(data->hal.dev, dma_channel->channel_id);
	} else if (dma_channel->dir == DMA_TX) {
		gdma_ll_tx_enable_interrupt(data->hal.dev, dma_channel->channel_id,
				GDMA_LL_EVENT_TX_EOF, false);
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
	dma_descriptor_t *desc;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (!status) {
		return -EINVAL;
	}

	memset(status, 0, sizeof(struct dma_status));

	if (dma_channel->dir == DMA_RX) {
		status->busy = !gdma_ll_rx_is_fsm_idle(data->hal.dev, dma_channel->channel_id);
		status->dir = PERIPHERAL_TO_MEMORY;
		desc = (dma_descriptor_t *)gdma_ll_rx_get_current_desc_addr(
			data->hal.dev, dma_channel->channel_id);
		if (desc >= dma_channel->desc_list) {
			status->read_position = desc - dma_channel->desc_list;
			status->total_copied = desc->dw0.length
						+ dma_channel->desc_list[0].dw0.size
						* status->read_position;
		}
	} else if (dma_channel->dir == DMA_TX) {
		status->busy = !gdma_ll_tx_is_fsm_idle(data->hal.dev, dma_channel->channel_id);
		status->dir = MEMORY_TO_PERIPHERAL;
		desc = (dma_descriptor_t *)gdma_ll_tx_get_current_desc_addr(
			data->hal.dev, dma_channel->channel_id);
		if (desc >= dma_channel->desc_list) {
			status->write_position = desc - dma_channel->desc_list;
		}
	}

	return 0;
}

static int dma_esp32_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			    size_t size)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];
	dma_descriptor_t *desc_iter = dma_channel->desc_list;
	uint32_t buf;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_channel->dir == DMA_RX) {
		gdma_ll_rx_reset_channel(data->hal.dev, dma_channel->channel_id);
		buf = dst;
	} else if (dma_channel->dir == DMA_TX) {
		gdma_ll_tx_reset_channel(data->hal.dev, dma_channel->channel_id);
		buf = src;
	} else {
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(dma_channel->desc_list); ++i) {
		memset(desc_iter, 0, sizeof(dma_descriptor_t));
		desc_iter->buffer = (void *)(buf + DMA_DESCRIPTOR_BUFFER_MAX_SIZE * i);
		desc_iter->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
		if (size < DMA_DESCRIPTOR_BUFFER_MAX_SIZE) {
			desc_iter->dw0.size = size;
			if (dma_channel->dir == DMA_TX) {
				desc_iter->dw0.length = size;
				desc_iter->dw0.suc_eof = 1;
			}
			desc_iter->next = NULL;
			break;
		}
		desc_iter->dw0.size = DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
		if (dma_channel->dir == DMA_TX) {
			desc_iter->dw0.length = DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
		}
		size -= DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
		desc_iter->next = desc_iter + 1;
		desc_iter += 1;
	}

	if (desc_iter->next) {
		memset(desc_iter, 0, sizeof(dma_descriptor_t));
		LOG_ERR("Not enough DMA descriptors. Increase CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM");
		return -EINVAL;
	}

	return 0;
}

static int dma_esp32_configure_irq(const struct device *dev)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct irq_config *irq_cfg = (struct irq_config *)config->irq_config;

	for (uint8_t i = 0; i < config->irq_size; i++) {
		int ret = esp_intr_alloc(irq_cfg[i].irq_source,
			ESP_PRIO_TO_FLAGS(irq_cfg[i].irq_priority) |
				ESP_INT_FLAGS_CHECK(irq_cfg[i].irq_flags) | ESP_INTR_FLAG_IRAM,
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
		memset(dma_channel->desc_list, 0, sizeof(dma_channel->desc_list));
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

#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32S3)

#define DMA_ESP32_DEFINE_IRQ_HANDLER(channel)                                                      \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel##_rx(                \
		const struct device *dev)                                                          \
	{                                                                                          \
		struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;          \
		struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;           \
		uint32_t intr_status = gdma_ll_rx_get_interrupt_status(data->hal.dev, channel);    \
		if (intr_status) {                                                                 \
			dma_esp32_isr_handle_rx(dev, &config->dma_channel[channel * 2],            \
						intr_status);                                      \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel##_tx(                \
		const struct device *dev)                                                          \
	{                                                                                          \
		struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;          \
		struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;           \
		uint32_t intr_status = gdma_ll_tx_get_interrupt_status(data->hal.dev, channel);    \
		if (intr_status) {                                                                 \
			dma_esp32_isr_handle_tx(dev, &config->dma_channel[channel * 2 + 1],        \
						intr_status);                                      \
		}                                                                                  \
	}

#else

#define DMA_ESP32_DEFINE_IRQ_HANDLER(channel)                                                      \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel(                     \
		const struct device *dev)                                                          \
	{                                                                                          \
		dma_esp32_isr_handle(dev, channel * 2, channel * 2 + 1);                           \
	}

#endif

#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32S3)
#define ESP32_DMA_HANDLER(channel) dma_esp32_isr_##channel##_rx, dma_esp32_isr_##channel##_tx
#else
#define ESP32_DMA_HANDLER(channel) dma_esp32_isr_##channel
#endif

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

#define IRQ_NUM(idx)	DT_NUM_IRQS(DT_DRV_INST(idx))
#define IRQ_ENTRY(n, idx) {	\
	DT_INST_IRQ_BY_IDX(idx, n, irq),	\
	DT_INST_IRQ_BY_IDX(idx, n, priority),	\
	DT_INST_IRQ_BY_IDX(idx, n, flags)	},

#define DMA_ESP32_INIT(idx)                                                                        \
	static struct irq_config irq_config_##idx[] = {                                            \
		LISTIFY(IRQ_NUM(idx), IRQ_ENTRY, (), idx)                                          \
	};                                                                                         \
	static struct dma_esp32_config dma_config_##idx = {                                        \
		.irq_config = irq_config_##idx,                                                    \
		.irq_size = IRQ_NUM(idx),                                                          \
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
