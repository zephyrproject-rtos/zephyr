/*
 * Copyright (c) 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_i2s

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <esp_clk_tree.h>
#include <hal/i2s_hal.h>

#if !SOC_GDMA_SUPPORTED
#include <soc/lldesc.h>
#include <esp_memory_utils.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif /* !SOC_GDMA_SUPPORTED */

#if SOC_GDMA_SUPPORTED && !DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_gdma)
#error "DMA peripheral is not enabled!"
#endif /* SOC_GDMA_SUPPORTED */

LOG_MODULE_REGISTER(i2s_esp32, CONFIG_I2S_LOG_LEVEL);

#define I2S_ESP32_CLK_SRC             I2S_CLK_SRC_DEFAULT
#define I2S_ESP32_DMA_BUFFER_MAX_SIZE 4092

#define I2S_ESP32_NUM_INST_OK DT_NUM_INST_STATUS_OKAY(espressif_esp32_i2s)
#define I2S_ESP32_IS_DIR_INST_EN(i, d) DT_INST_DMAS_HAS_NAME(i, d) || DT_INST_IRQ_HAS_NAME(i, d)
#define I2S_ESP32_IS_DIR_EN(d)  LISTIFY(I2S_ESP32_NUM_INST_OK, I2S_ESP32_IS_DIR_INST_EN, (||), d)

struct queue_item {
	void *buffer;
	size_t size;
};

struct i2s_esp32_stream;

struct i2s_esp32_stream_data {
	int32_t state;
	bool is_slave;
	struct i2s_config i2s_cfg;
	void *mem_block;
	size_t mem_block_len;
	bool last_block;
	bool stop_without_draining;
	struct k_msgq queue;
	struct intr_handle_data_t *irq_handle;
	bool dma_pending;
	uint8_t chunks_rem;
	uint8_t chunk_idx;
};

struct i2s_esp32_stream_conf {
	void (*queue_drop)(const struct i2s_esp32_stream *stream);
	int (*start_transfer)(const struct device *dev);
	void (*stop_transfer)(const struct device *dev);
	const struct device *dma_dev;
	uint32_t dma_channel;
#if SOC_GDMA_SUPPORTED
	void *dma_desc;
#else
	lldesc_t *dma_desc;
#endif
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct i2s_esp32_stream {
	struct i2s_esp32_stream_data *data;
	const struct i2s_esp32_stream_conf *conf;
};

struct i2s_esp32_cfg {
	const int unit;
	i2s_hal_context_t hal;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct i2s_esp32_stream rx;
	struct i2s_esp32_stream tx;
};

struct i2s_esp32_data {
	i2s_hal_clock_info_t clk_info;
};

uint32_t i2s_esp32_get_source_clk_freq(i2s_clock_src_t clk_src)
{
	uint32_t clk_freq = 0;

	esp_clk_tree_src_get_freq_hz(clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &clk_freq);
	return clk_freq;
}

static esp_err_t i2s_esp32_calculate_clock(const struct i2s_config *i2s_cfg, uint8_t channel_length,
					   i2s_hal_clock_info_t *i2s_hal_clock_info)
{
	uint16_t mclk_multiple = 256;

	if (i2s_cfg == NULL) {
		LOG_ERR("Input i2s_cfg is NULL");
		return ESP_ERR_INVALID_ARG;
	}

	if (i2s_hal_clock_info == NULL) {
		LOG_ERR("Input hal_clock_info is NULL");
		return ESP_ERR_INVALID_ARG;
	}

	if (i2s_cfg->word_size == 24) {
		mclk_multiple = 384;
	}

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		i2s_hal_clock_info->bclk_div = 8;
		i2s_hal_clock_info->bclk =
			i2s_cfg->frame_clk_freq * i2s_cfg->channels * channel_length;
		i2s_hal_clock_info->mclk = i2s_cfg->frame_clk_freq * i2s_hal_clock_info->bclk_div;
	} else {
		i2s_hal_clock_info->bclk =
			i2s_cfg->frame_clk_freq * i2s_cfg->channels * channel_length;
		i2s_hal_clock_info->mclk = i2s_cfg->frame_clk_freq * mclk_multiple;
		i2s_hal_clock_info->bclk_div = i2s_hal_clock_info->mclk / i2s_hal_clock_info->bclk;
	}

	i2s_hal_clock_info->sclk = i2s_esp32_get_source_clk_freq(I2S_ESP32_CLK_SRC);
	i2s_hal_clock_info->mclk_div = i2s_hal_clock_info->sclk / i2s_hal_clock_info->mclk;
	if (i2s_hal_clock_info->mclk_div == 0) {
		LOG_ERR("Sample rate is too large for the current clock source");
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

static void i2s_esp32_queue_drop(const struct i2s_esp32_stream *stream)
{
	struct queue_item item;

	while (k_msgq_get(&stream->data->queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->data->i2s_cfg.mem_slab, item.buffer);
	}
}

static int i2s_esp32_restart_dma(const struct device *dev, enum i2s_dir dir);
static int i2s_esp32_start_dma(const struct device *dev, enum i2s_dir dir);

#if I2S_ESP32_IS_DIR_EN(rx)

#if SOC_GDMA_SUPPORTED
static void i2s_esp32_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
#else
static void i2s_esp32_rx_callback(void *arg, int status)
#endif /* SOC_GDMA_SUPPORTED */
{
	const struct device *dev = (const struct device *)arg;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->rx;
	int err;

#if SOC_GDMA_SUPPORTED
	if (status < 0) {
#else
	if (status & I2S_LL_EVENT_RX_DSCR_ERR) {
#endif /* SOC_GDMA_SUPPORTED */
		stream->data->state = I2S_STATE_ERROR;
		LOG_ERR("RX status bad: %d", status);
		goto rx_disable;
	}

	if (stream->data->mem_block == NULL) {
		if (stream->data->state != I2S_STATE_READY) {
			stream->data->state = I2S_STATE_ERROR;
			LOG_ERR("RX mem_block NULL");
			goto rx_disable;
		} else {
			return;
		}
	}

#if SOC_GDMA_SUPPORTED
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
	uint16_t chunk_len;

	if (stream->data->chunks_rem) {
		uint32_t dst;

		stream->data->chunk_idx++;
		stream->data->chunks_rem--;
		if (stream->data->chunks_rem) {
			chunk_len = I2S_ESP32_DMA_BUFFER_MAX_SIZE;
		} else {
			chunk_len = stream->data->mem_block_len % I2S_ESP32_DMA_BUFFER_MAX_SIZE;
			if (chunk_len == 0) {
				chunk_len = I2S_ESP32_DMA_BUFFER_MAX_SIZE;
			}
		}

		dst = (uint32_t)stream->data->mem_block + (stream->data->chunk_idx *
							   I2S_ESP32_DMA_BUFFER_MAX_SIZE);
		err = dma_reload(stream->conf->dma_dev, stream->conf->dma_channel, (uint32_t)NULL,
				(uint32_t)dst, chunk_len);
		if (err < 0) {
			LOG_ERR("Failed to reload DMA channel: %"PRIu32, stream->conf->dma_channel);
			goto rx_disable;
		}

		i2s_ll_rx_set_eof_num(hal->dev, chunk_len);

		err = dma_start(stream->conf->dma_dev, stream->conf->dma_channel);
		if (err < 0) {
			LOG_ERR("Failed to start DMA channel: %"PRIu32, stream->conf->dma_channel);
			goto rx_disable;
		}

		stream->data->dma_pending = true;

		return;
	}
#endif /* SOC_GDMA_SUPPORTED */

	struct queue_item item = {
		.buffer = stream->data->mem_block,
		.size = stream->data->mem_block_len
	};

	err = k_msgq_put(&stream->data->queue, &item, K_NO_WAIT);
	if (err < 0) {
		stream->data->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	if (stream->data->state == I2S_STATE_STOPPING) {
		stream->data->state = I2S_STATE_READY;
		goto rx_disable;
	}

	err = k_mem_slab_alloc(stream->data->i2s_cfg.mem_slab, &stream->data->mem_block, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("RX failed to allocate memory from slab: %i:", err);
		stream->data->state = I2S_STATE_ERROR;
		goto rx_disable;
	}
	stream->data->mem_block_len = stream->data->i2s_cfg.block_size;

	err = i2s_esp32_restart_dma(dev, I2S_DIR_RX);
	if (err < 0) {
		stream->data->state = I2S_STATE_ERROR;
		LOG_ERR("Failed to restart RX transfer: %d", err);
		goto rx_disable;
	}

	return;

rx_disable:
	stream->conf->stop_transfer(dev);
}

#if !SOC_GDMA_SUPPORTED

static void IRAM_ATTR i2s_esp32_rx_handler(void *arg)
{
	if (arg == NULL) {
		return;
	}

	struct device *dev = (struct device *)arg;
	const struct i2s_esp32_cfg *const dev_cfg = dev->config;
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
	uint32_t status = i2s_hal_get_intr_status(hal);

	i2s_hal_clear_intr_status(hal, status);
	if (status & I2S_LL_EVENT_RX_EOF) {
		i2s_esp32_rx_callback((void *)arg, status);
	}
}

#endif /* !SOC_GDMA_SUPPORTED */

static int i2s_esp32_rx_start_transfer(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->rx;
	const i2s_hal_context_t *hal = &dev_cfg->hal;
	int err;

	err = k_mem_slab_alloc(stream->data->i2s_cfg.mem_slab, &stream->data->mem_block, K_NO_WAIT);
	if (err < 0) {
		return -ENOMEM;
	}
	stream->data->mem_block_len = stream->data->i2s_cfg.block_size;

	err = i2s_esp32_start_dma(dev, I2S_DIR_RX);
	if (err < 0) {
		LOG_ERR("Failed to start RX DMA transfer: %d", err);
		return -EIO;
	}

	i2s_hal_rx_start(hal);

#if !SOC_GDMA_SUPPORTED
	esp_intr_enable(stream->data->irq_handle);
#endif /* !SOC_GDMA_SUPPORTED */

	return 0;
}

static void i2s_esp32_rx_stop_transfer(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->rx;

#if SOC_GDMA_SUPPORTED
	dma_stop(stream->conf->dma_dev, stream->conf->dma_channel);
#else
	const i2s_hal_context_t *hal = &(dev_cfg->hal);

	esp_intr_disable(stream->data->irq_handle);
	i2s_hal_rx_stop_link(hal);
	i2s_hal_rx_disable_intr(hal);
	i2s_hal_rx_disable_dma(hal);
	i2s_hal_clear_intr_status(hal, I2S_INTR_MAX);
#endif /* SOC_GDMA_SUPPORTED */

	if (stream->data->mem_block != NULL) {
		k_mem_slab_free(stream->data->i2s_cfg.mem_slab, stream->data->mem_block);
		stream->data->mem_block = NULL;
		stream->data->mem_block_len = 0;
	}
}

#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)

#if SOC_GDMA_SUPPORTED
static void i2s_esp32_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
#else
static void i2s_esp32_tx_callback(void *arg, int status)
#endif /* SOC_GDMA_SUPPORTED */
{
	const struct device *dev = (const struct device *)arg;
	const struct i2s_esp32_cfg *const dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;
	struct queue_item item;
	void *mem_block_tmp;
	int err;

	if (!stream->data->dma_pending) {
		return;
	}

	stream->data->dma_pending = false;

#if SOC_GDMA_SUPPORTED
	if (status < 0) {
#else
	if (status & I2S_LL_EVENT_TX_DSCR_ERR) {
#endif /* SOC_GDMA_SUPPORTED */
		stream->data->state = I2S_STATE_ERROR;
		LOG_ERR("TX bad status: %d", status);
		goto tx_disable;
	}

	if (stream->data->mem_block == NULL) {
		if (stream->data->state != I2S_STATE_READY) {
			stream->data->state = I2S_STATE_ERROR;
			LOG_ERR("TX mem_block NULL");
			goto tx_disable;
		} else {
			return;
		}
	}

	if (stream->data->state == I2S_STATE_STOPPING) {
		if (k_msgq_num_used_get(&stream->data->queue) == 0) {
			stream->data->state = I2S_STATE_READY;
			goto tx_disable;
		} else if (stream->data->stop_without_draining == true) {
			stream->conf->queue_drop(stream);
			stream->data->state = I2S_STATE_READY;
			goto tx_disable;
		}
	}

	if (stream->data->last_block) {
		stream->data->state = I2S_STATE_READY;
		goto tx_disable;
	}

	err = k_msgq_get(&stream->data->queue, &item, K_NO_WAIT);
	if (err < 0) {
		stream->data->state = I2S_STATE_ERROR;
		LOG_ERR("TX queue empty: %d", err);
		goto tx_disable;
	}

	mem_block_tmp = stream->data->mem_block;

	stream->data->mem_block = item.buffer;
	stream->data->mem_block_len = item.size;

	err = i2s_esp32_restart_dma(dev, I2S_DIR_TX);
	if (err < 0) {
		stream->data->state = I2S_STATE_ERROR;
		LOG_ERR("Failed to restart TX transfer: %d", err);
		goto tx_disable;
	}

	k_mem_slab_free(stream->data->i2s_cfg.mem_slab, mem_block_tmp);

	return;

tx_disable:
	stream->conf->stop_transfer(dev);
}

#if !SOC_GDMA_SUPPORTED

static void IRAM_ATTR i2s_esp32_tx_handler(void *arg)
{
	if (arg == NULL) {
		return;
	}

	struct device *dev = (struct device *)arg;
	const struct i2s_esp32_cfg *const dev_cfg = dev->config;
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
	uint32_t status = i2s_hal_get_intr_status(hal);

	i2s_hal_clear_intr_status(hal, status);
	if (status & I2S_LL_EVENT_TX_EOF) {
		i2s_esp32_tx_callback((void *)arg, status);
	}
}

#endif /* !SOC_GDMA_SUPPORTED */

static int i2s_esp32_tx_start_transfer(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;
	const i2s_hal_context_t *hal = &dev_cfg->hal;
	struct queue_item item;
	int err;

	err = k_msgq_get(&stream->data->queue, &item, K_NO_WAIT);
	if (err < 0) {
		return -ENOMEM;
	}

	stream->data->mem_block = item.buffer;
	stream->data->mem_block_len = item.size;

	err = i2s_esp32_start_dma(dev, I2S_DIR_TX);
	if (err < 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", err);
		return -EIO;
	}

	i2s_hal_tx_start(hal);

#if !SOC_GDMA_SUPPORTED
	esp_intr_enable(stream->data->irq_handle);
#endif /* !SOC_GDMA_SUPPORTED */

	return 0;
}

static void i2s_esp32_tx_stop_transfer(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;

#if SOC_GDMA_SUPPORTED
	dma_stop(stream->conf->dma_dev, stream->conf->dma_channel);
#else
	const i2s_hal_context_t *hal = &(dev_cfg->hal);

	esp_intr_disable(stream->data->irq_handle);
	i2s_hal_tx_stop_link(hal);
	i2s_hal_tx_disable_intr(hal);
	i2s_hal_tx_disable_dma(hal);
	i2s_hal_clear_intr_status(hal, I2S_INTR_MAX);
#endif /* SOC_GDMA_SUPPORTED */

	if (stream->data->mem_block != NULL) {
		k_mem_slab_free(stream->data->i2s_cfg.mem_slab, stream->data->mem_block);
		stream->data->mem_block = NULL;
		stream->data->mem_block_len = 0;
	}
}

#endif /* I2S_ESP32_IS_DIR_EN(tx) */

int i2s_esp32_config_dma(const struct device *dev, enum i2s_dir dir,
			 const struct i2s_esp32_stream *stream)
{
	uint32_t mem_block = (uint32_t)stream->data->mem_block;
	uint32_t mem_block_size = stream->data->mem_block_len;

#if SOC_GDMA_SUPPORTED
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int err;

	dma_blk.block_size = mem_block_size;
	if (dir == I2S_DIR_RX) {
#if I2S_ESP32_IS_DIR_EN(rx)
		dma_blk.dest_address = mem_block;
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.dma_callback = i2s_esp32_rx_callback;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
	} else {
#if I2S_ESP32_IS_DIR_EN(tx)
		dma_blk.source_address = mem_block;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.dma_callback = i2s_esp32_tx_callback;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
	}
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot =
		dev_cfg->unit == 0 ? ESP_GDMA_TRIG_PERIPH_I2S0 : ESP_GDMA_TRIG_PERIPH_I2S1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;

	err = dma_config(stream->conf->dma_dev, stream->conf->dma_channel, &dma_cfg);
	if (err < 0) {
		LOG_ERR("Failed to configure DMA channel: %"PRIu32, stream->conf->dma_channel);
		return -EINVAL;
	}
#else
	lldesc_t *desc_iter = stream->conf->dma_desc;

	if (!mem_block) {
		LOG_ERR("At least one dma block is required");
		return -EINVAL;
	}

	if (!esp_ptr_dma_capable((void *)mem_block)) {
		LOG_ERR("Buffer is not in DMA capable memory: %p",
			(uint32_t *)mem_block);

		return -EINVAL;
	}

	for (int i = 0; i < CONFIG_I2S_ESP32_DMA_DESC_NUM_MAX; ++i) {
		uint32_t buffer_size;

		if (mem_block_size > I2S_ESP32_DMA_BUFFER_MAX_SIZE) {
			buffer_size = I2S_ESP32_DMA_BUFFER_MAX_SIZE;
		} else {
			buffer_size = mem_block_size;
		}

		memset(desc_iter, 0, sizeof(lldesc_t));
		desc_iter->owner = 1;
		desc_iter->sosf = 0;
		desc_iter->buf = (uint8_t *)mem_block;
		desc_iter->offset = 0;
		desc_iter->length = buffer_size;
		desc_iter->size = buffer_size;
		desc_iter->eof = 0;

		mem_block += buffer_size;
		mem_block_size -= buffer_size;

		if (mem_block_size > 0) {
			desc_iter->empty = (uint32_t)(desc_iter + 1);
			desc_iter += 1;
		} else {
			stream->data->dma_pending = true;
			desc_iter->empty = 0;
			if (dir == I2S_DIR_TX) {
				desc_iter->eof = 1;
			}
			break;
		}
	}

	if (desc_iter->empty)  {
		stream->data->dma_pending = false;
		LOG_ERR("Run out of descriptors. Increase CONFIG_I2S_ESP32_DMA_DESC_NUM_MAX");
		return -EINVAL;
	}
#endif /* SOC_GDMA_SUPPORTED */

	return 0;
}

static int i2s_esp32_start_dma(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
	const struct i2s_esp32_stream *stream = NULL;
	unsigned int key;
	int err = 0;

	if (dir == I2S_DIR_RX) {
		stream = &dev_cfg->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_cfg->tx;
	} else {
		LOG_ERR("Invalid DMA direction");
		return -EINVAL;
	}

	key = irq_lock();

	err = i2s_esp32_config_dma(dev, dir, stream);
	if (err < 0) {
		LOG_ERR("Dma configuration failed: %i", err);
		goto unlock;
	}

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX) {
		uint16_t chunk_len;

#if SOC_GDMA_SUPPORTED
		if (stream->data->mem_block_len < I2S_ESP32_DMA_BUFFER_MAX_SIZE) {
			chunk_len = stream->data->mem_block_len;
			stream->data->chunks_rem = 0;
		} else {
			chunk_len = I2S_ESP32_DMA_BUFFER_MAX_SIZE;
			stream->data->chunks_rem = ((stream->data->mem_block_len +
						     (I2S_ESP32_DMA_BUFFER_MAX_SIZE - 1)) /
						    I2S_ESP32_DMA_BUFFER_MAX_SIZE) - 1;
		}
		stream->data->chunk_idx = 0;
#else
		chunk_len = stream->data->mem_block_len;
#endif
		i2s_ll_rx_set_eof_num(hal->dev, chunk_len);
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if SOC_GDMA_SUPPORTED
	err = dma_start(stream->conf->dma_dev, stream->conf->dma_channel);
	if (err < 0) {
		LOG_ERR("Failed to start DMA channel: %"PRIu32, stream->conf->dma_channel);
		goto unlock;
	}
	stream->data->dma_pending = true;
#else
#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX) {
		i2s_hal_rx_enable_dma(hal);
		i2s_hal_rx_enable_intr(hal);
		i2s_hal_rx_start_link(hal, (uint32_t)&(stream->conf->dma_desc[0]));
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX) {
		i2s_hal_tx_enable_dma(hal);
		i2s_hal_tx_enable_intr(hal);
		i2s_hal_tx_start_link(hal, (uint32_t)&(stream->conf->dma_desc[0]));
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
#endif /* SOC_GDMA_SUPPORTED */

unlock:
	irq_unlock(key);
	return err;
}

static int i2s_esp32_restart_dma(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
	const struct i2s_esp32_stream *stream;
	int err = 0;

	if (dir == I2S_DIR_RX) {
		stream = &dev_cfg->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_cfg->tx;
	} else {
		LOG_ERR("Invalid DMA direction");
		return -EINVAL;
	}

#if SOC_GDMA_SUPPORTED
	void *src = NULL, *dst = NULL;

#if I2S_ESP32_IS_DIR_EN(rx)
	uint16_t chunk_len;

	if (dir == I2S_DIR_RX) {
		dst = stream->data->mem_block;

		if (stream->data->mem_block_len < I2S_ESP32_DMA_BUFFER_MAX_SIZE) {
			chunk_len = stream->data->mem_block_len;
			stream->data->chunks_rem = 0;
		} else {
			chunk_len = I2S_ESP32_DMA_BUFFER_MAX_SIZE;
			stream->data->chunks_rem = ((stream->data->mem_block_len +
						     (I2S_ESP32_DMA_BUFFER_MAX_SIZE - 1)) /
						    I2S_ESP32_DMA_BUFFER_MAX_SIZE) - 1;
		}
		stream->data->chunk_idx = 0;
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX) {
		src = stream->data->mem_block;
		chunk_len = stream->data->mem_block_len;
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

	err = dma_reload(stream->conf->dma_dev, stream->conf->dma_channel, (uint32_t)src,
			(uint32_t)dst, chunk_len);
	if (err < 0) {
		LOG_ERR("Failed to reload DMA channel: %"PRIu32, stream->conf->dma_channel);
		return -EIO;
	}

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX) {
		i2s_ll_rx_set_eof_num(hal->dev, chunk_len);
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

	err = dma_start(stream->conf->dma_dev, stream->conf->dma_channel);
	if (err < 0) {
		LOG_ERR("Failed to start DMA channel: %"PRIu32, stream->conf->dma_channel);
		return -EIO;
	}
#else
	err = i2s_esp32_config_dma(dev, dir, stream);
	if (err < 0) {
		LOG_ERR("Failed to configure DMA");
		return -EIO;
	}

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX) {
		i2s_ll_rx_set_eof_num(hal->dev, stream->data->mem_block_len);
		i2s_hal_rx_start_link(hal, (uint32_t)stream->conf->dma_desc);
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX) {
		i2s_hal_tx_start_link(hal, (uint32_t)stream->conf->dma_desc);
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
#endif /* SOC_GDMA_SUPPORTED */

	stream->data->dma_pending = true;

	return 0;
}

static int i2s_esp32_initialize(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct device *clk_dev = dev_cfg->clock_dev;
	const struct i2s_esp32_stream *stream;
	int err;

#if !SOC_GDMA_SUPPORTED
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
#endif /* !SOC_GDMA_SUPPORTED */

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(clk_dev, dev_cfg->clock_subsys);
	if (err != 0) {
		LOG_ERR("Clock control enabling failed: %d", err);
		return -EIO;
	}

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dev_cfg->rx.data && dev_cfg->rx.conf) {
		stream = &dev_cfg->rx;
#if SOC_GDMA_SUPPORTED
		if (stream->conf->dma_dev && !device_is_ready(stream->conf->dma_dev)) {
			LOG_ERR("%s device not ready", stream->conf->dma_dev->name);
			return -ENODEV;
		}
#else
		int irq_flags = ESP_PRIO_TO_FLAGS(stream->conf->irq_priority) |
				ESP_INT_FLAGS_CHECK(stream->conf->irq_flags) |
				ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED |
				ESP_INTR_FLAG_SHARED;

		err = esp_intr_alloc_intrstatus(stream->conf->irq_source, irq_flags,
						(uint32_t)i2s_ll_get_intr_status_reg(hal->dev),
						I2S_LL_RX_EVENT_MASK, i2s_esp32_rx_handler,
						(void *)dev, &(stream->data->irq_handle));
		if (err != 0) {
			LOG_ERR("Could not allocate rx interrupt (err %d)", err);
			return err;
		}
#endif /* SOC_GDMA_SUPPORTED */

		err = k_msgq_alloc_init(&stream->data->queue, sizeof(struct queue_item),
					CONFIG_I2S_ESP32_RX_BLOCK_COUNT);
		if (err < 0) {
			return err;
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dev_cfg->tx.data && dev_cfg->tx.conf) {
		stream = &dev_cfg->tx;
#if SOC_GDMA_SUPPORTED
		if (stream->conf->dma_dev && !device_is_ready(stream->conf->dma_dev)) {
			LOG_ERR("%s device not ready", stream->conf->dma_dev->name);
			return -ENODEV;
		}
#else
		int irq_flags = ESP_PRIO_TO_FLAGS(stream->conf->irq_priority) |
				ESP_INT_FLAGS_CHECK(stream->conf->irq_flags) |
				ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED |
				ESP_INTR_FLAG_SHARED;

		err = esp_intr_alloc_intrstatus(stream->conf->irq_source, irq_flags,
						(uint32_t)i2s_ll_get_intr_status_reg(hal->dev),
						I2S_LL_TX_EVENT_MASK, i2s_esp32_tx_handler,
						(void *)dev, &(stream->data->irq_handle));
		if (err != 0) {
			LOG_ERR("Could not allocate tx interrupt (err %d)", err);
			return err;
		}
#endif /* SOC_GDMA_SUPPORTED */

		err = k_msgq_alloc_init(&stream->data->queue, sizeof(struct queue_item),
					CONFIG_I2S_ESP32_TX_BLOCK_COUNT);
		if (err < 0) {
			return err;
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

#if !SOC_GDMA_SUPPORTED
	i2s_ll_clear_intr_status(hal->dev, I2S_INTR_MAX);
#endif /* !SOC_GDMA_SUPPORTED */

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

static int i2s_esp32_configure_dir(const struct device *dev, enum i2s_dir dir,
				   const struct i2s_esp32_stream *stream,
				   const struct i2s_config *i2s_cfg)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	uint8_t data_format;
	int err;

	switch (dir) {
#if I2S_ESP32_IS_DIR_EN(rx)
	case I2S_DIR_RX:
		if (stream->conf) {
#if SOC_GDMA_SUPPORTED
			if (stream->conf->dma_dev == NULL) {
				LOG_ERR("RX DMA controller not available");
#else
			if (stream->conf->irq_source == -1) {
				LOG_ERR("RX IRQ source not available");
#endif /* SOC_GDMA_SUPPORTED */
				return -EINVAL;
			}
		}
		break;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
#if I2S_ESP32_IS_DIR_EN(tx)
	case I2S_DIR_TX:
		if (stream->conf) {
#if SOC_GDMA_SUPPORTED
			if (stream->conf->dma_dev == NULL) {
				LOG_ERR("TX DMA controller not available");
#else
			if (stream->conf->irq_source == -1) {
				LOG_ERR("TX IRQ source not available");
#endif /* SOC_GDMA_SUPPORTED */
				return -EINVAL;
			}
		}
		break;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
	default:
		LOG_ERR("Invalid direction");
		return -EINVAL;
	}

	if (stream->data->state != I2S_STATE_NOT_READY && stream->data->state != I2S_STATE_READY) {
		LOG_ERR("Invalid state: %d", (int)stream->data->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		stream->conf->queue_drop(stream);
		memset(&stream->data->i2s_cfg, 0, sizeof(struct i2s_config));
		stream->data->is_slave = false;
		stream->data->state = I2S_STATE_NOT_READY;
		return 0;
	}

	data_format = i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK;

	if (data_format != I2S_FMT_DATA_FORMAT_I2S &&
	    data_format != I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED &&
	    data_format != I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
		LOG_ERR("Invalid data format: %u", (unsigned int)data_format);
		return -EINVAL;
	}

	if (i2s_cfg->word_size != 8 && i2s_cfg->word_size != 16 && i2s_cfg->word_size != 24 &&
	    i2s_cfg->word_size != 32) {
		LOG_ERR("Word size not supported: %d", (int)i2s_cfg->word_size);
		return -EINVAL;
	}

	if (i2s_cfg->channels != 2) {
		LOG_ERR("Currently only 2 channels are supported");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_LOOPBACK) {
		LOG_ERR("For internal loopback: I2S#_O_SD_GPIO = I2S#_I_SD_GPIO");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_PINGPONG) {
		LOG_ERR("Unsupported option: I2S_OPT_PINGPONG");
		return -EINVAL;
	}

	if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) != 0 &&
	    (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) != 0) {
		stream->data->is_slave = true;
	} else if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) == 0 &&
		   (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) == 0) {
		stream->data->is_slave = false;
	} else {
		LOG_ERR("I2S_OPT_FRAME_CLK and I2S_OPT_BIT_CLK options must both be"
			" MASTER or SLAVE");
		return -EINVAL;
	}

	i2s_hal_slot_config_t slot_cfg = {0};

	slot_cfg.data_bit_width = i2s_cfg->word_size;
	slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
	slot_cfg.slot_bit_width = i2s_cfg->word_size > 16 ? 32 : 16;
	if (data_format == I2S_FMT_DATA_FORMAT_I2S) {
		slot_cfg.std.ws_pol = i2s_cfg->format & I2S_FMT_FRAME_CLK_INV ? true : false;
		slot_cfg.std.bit_shift = true;
#if SOC_I2S_HW_VERSION_2
		slot_cfg.std.left_align = true;
#endif /* SOC_I2S_HW_VERSION_2 */
	} else {
		slot_cfg.std.ws_pol = i2s_cfg->format & I2S_FMT_FRAME_CLK_INV ? false : true;
		slot_cfg.std.bit_shift = false;
		if (data_format == I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) {
#if SOC_I2S_HW_VERSION_2
			slot_cfg.std.left_align = true;
		} else if (data_format == I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
			slot_cfg.std.left_align = false;
#endif /* SOC_I2S_HW_VERSION_2 */
		} else {
			LOG_ERR("Unsupported data format: %u", (unsigned int)data_format);
		}
	}
	slot_cfg.std.ws_width = slot_cfg.slot_bit_width;
	slot_cfg.std.slot_mask = I2S_STD_SLOT_BOTH;
#if SOC_I2S_HW_VERSION_1
	slot_cfg.std.msb_right = true;
#elif SOC_I2S_HW_VERSION_2
	slot_cfg.std.big_endian = false;
	slot_cfg.std.bit_order_lsb = i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB ? true : false;
#endif /* SOC_I2S_HW_VERSION_1 */

	i2s_hal_clock_info_t i2s_hal_clock_info;
	i2s_hal_context_t *hal = (i2s_hal_context_t *)&(dev_cfg->hal);

	err = i2s_esp32_calculate_clock(i2s_cfg, slot_cfg.slot_bit_width, &i2s_hal_clock_info);
	if (err != ESP_OK) {
		return -EINVAL;
	}

	if (dir == I2S_DIR_TX) {
#if I2S_ESP32_IS_DIR_EN(tx)
		i2s_hal_std_enable_tx_channel(hal);
		if (dev_cfg->rx.data != NULL && dev_cfg->rx.data->state != I2S_STATE_NOT_READY) {
			if (stream->data->is_slave && !dev_cfg->rx.data->is_slave) { /*full duplex*/
				i2s_ll_share_bck_ws(hal->dev, true);
			} else {
				i2s_ll_share_bck_ws(hal->dev, false);
			}
		} else {
			i2s_ll_share_bck_ws(hal->dev, false);
		}

		i2s_hal_std_set_tx_slot(hal, stream->data->is_slave, &slot_cfg);
		i2s_hal_set_tx_clock(hal, &i2s_hal_clock_info, I2S_ESP32_CLK_SRC);

#if SOC_I2S_HW_VERSION_2
		if (dev_cfg->rx.data != NULL && dev_cfg->rx.data->state != I2S_STATE_NOT_READY) {
			if (stream->data->is_slave && !dev_cfg->rx.data->is_slave) { /*full duplex*/
				i2s_ll_mclk_bind_to_rx_clk(hal->dev);
			}
		}
#endif /* SOC_I2S_HW_VERSION_2 */
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
	} else if (dir == I2S_DIR_RX) {
#if I2S_ESP32_IS_DIR_EN(rx)
		i2s_hal_std_enable_rx_channel(hal);
		if (dev_cfg->tx.data != NULL && dev_cfg->tx.data->state != I2S_STATE_NOT_READY) {
			if (stream->data->is_slave && !dev_cfg->tx.data->is_slave) { /*full duplex*/
				i2s_ll_share_bck_ws(hal->dev, true);
			} else {
				i2s_ll_share_bck_ws(hal->dev, false);
			}
		} else {
			i2s_ll_share_bck_ws(hal->dev, false);
		}

		i2s_hal_std_set_rx_slot(hal, stream->data->is_slave, &slot_cfg);
		i2s_hal_set_rx_clock(hal, &i2s_hal_clock_info, I2S_ESP32_CLK_SRC);

#if SOC_I2S_HW_VERSION_2
		if (dev_cfg->tx.data != NULL && dev_cfg->tx.data->state != I2S_STATE_NOT_READY) {
			if (stream->data->is_slave && !dev_cfg->tx.data->is_slave) { /*full duplex*/
				i2s_ll_mclk_bind_to_tx_clk(hal->dev);
			}
		}
#endif /* SOC_I2S_HW_VERSION_2 */
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
	}

	err = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Pins setup failed: %d", err);
		return -EIO;
	}

	memcpy(&stream->data->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->data->state = I2S_STATE_READY;

	return 0;
}

static int i2s_esp32_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	int err;

	switch (dir) {
	case I2S_DIR_RX:
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (stream) {
			err = i2s_esp32_configure_dir(dev, I2S_DIR_RX, stream, i2s_cfg);
		} else {
			LOG_ERR("I2S_DIR_RX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_RX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
		break;
	case I2S_DIR_TX:
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (stream) {
			err = i2s_esp32_configure_dir(dev, I2S_DIR_TX, stream, i2s_cfg);
		} else {
			LOG_ERR("I2S_DIR_TX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_TX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
		break;
	case I2S_DIR_BOTH:
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (stream) {
			err = i2s_esp32_configure_dir(dev, I2S_DIR_TX, stream, i2s_cfg);
		} else {
			LOG_ERR("I2S_DIR_TX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_TX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
		if (err < 0) {
			break;
		}
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (stream) {
			err = i2s_esp32_configure_dir(dev, I2S_DIR_RX, stream, i2s_cfg);
		} else {
			LOG_ERR("I2S_DIR_RX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_RX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
		break;
	default:
		LOG_ERR("Invalid direction: %d", (int)dir);
		return -EINVAL;
	}

	return err;
}

static const struct i2s_config *i2s_esp32_config_get(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;

	if (dir == I2S_DIR_RX) {
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (!stream) {
			LOG_ERR("I2S_DIR_RX not enabled");
			return NULL;
		}
#else
		LOG_ERR("I2S_DIR_RX not enabled");
		return NULL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
	} else if (dir == I2S_DIR_TX) {
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (!stream) {
			LOG_ERR("I2S_DIR_TX not enabled");
			return NULL;
		}
#else
		LOG_ERR("I2S_DIR_TX not enabled");
		return NULL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
	} else {
		LOG_ERR("Invalid direction: %d", (int)dir);
		return NULL;
	}

	if (stream->data->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->data->i2s_cfg;
}

static int i2s_esp32_trigger_stream(const struct device *dev, const struct i2s_esp32_stream *stream,
				    enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const i2s_hal_context_t *hal = &dev_cfg->hal;
	unsigned int key;
	int err;

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->data->state != I2S_STATE_READY) {
			LOG_ERR("START - Invalid state: %d", (int)stream->data->state);
			return -EIO;
		}

		key = irq_lock();

		if (dir == I2S_DIR_RX) {
			i2s_hal_rx_stop(hal);
			i2s_hal_rx_reset(hal);
#if !SOC_GDMA_SUPPORTED
			i2s_hal_rx_reset_dma(hal);
#endif /* !SOC_GDMA_SUPPORTED */
			i2s_hal_rx_reset_fifo(hal);
		} else if (dir == I2S_DIR_TX) {
			i2s_hal_tx_stop(hal);
			i2s_hal_tx_reset(hal);
#if !SOC_GDMA_SUPPORTED
			i2s_hal_tx_reset_dma(hal);
#endif /* !SOC_GDMA_SUPPORTED */
			i2s_hal_tx_reset_fifo(hal);
		}

		err = stream->conf->start_transfer(dev);
		if (err < 0) {
			LOG_ERR("START - Transfer start failed: %d", err);
			irq_unlock(key);
			return -EIO;
		}
		stream->data->last_block = false;
		stream->data->state = I2S_STATE_RUNNING;
		irq_unlock(key);
		break;

	case I2S_TRIGGER_STOP:
		key = irq_lock();
		if (stream->data->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_ERR("STOP - Invalid state: %d", (int)stream->data->state);
			return -EIO;
		}

		if (stream->data->dma_pending) {
			stream->data->stop_without_draining = true;
			stream->data->state = I2S_STATE_STOPPING;
		} else {
			stream->conf->stop_transfer(dev);
			stream->data->last_block = true;
			stream->data->state = I2S_STATE_READY;
		}

		irq_unlock(key);
		break;

	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		if (stream->data->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_ERR("DRAIN - Invalid state: %d", (int)stream->data->state);
			return -EIO;
		}

#if I2S_ESP32_IS_DIR_EN(tx)
		if (dir == I2S_DIR_TX) {
			if (k_msgq_num_used_get(&stream->data->queue) > 0 ||
			    stream->data->dma_pending) {
				stream->data->stop_without_draining = false;
				stream->data->state = I2S_STATE_STOPPING;
			} else {
				stream->conf->stop_transfer(dev);
				stream->data->state = I2S_STATE_READY;
			}
		}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

#if I2S_ESP32_IS_DIR_EN(rx)
		if (dir == I2S_DIR_RX) {
			if (stream->data->dma_pending) {
				stream->data->stop_without_draining = true;
				stream->data->state = I2S_STATE_STOPPING;
			} else {
				stream->conf->stop_transfer(dev);
				stream->data->last_block = true;
				stream->data->state = I2S_STATE_READY;
			}
		}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

		irq_unlock(key);
		break;

	case I2S_TRIGGER_DROP:
		if (stream->data->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - invalid state: %d", (int)stream->data->state);
			return -EIO;
		}
		stream->conf->stop_transfer(dev);
		stream->conf->queue_drop(stream);
		stream->data->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->data->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE - invalid state: %d", (int)stream->data->state);
			return -EIO;
		}
		stream->conf->queue_drop(stream);
		stream->data->state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("Unsupported trigger command: %d", (int)cmd);
		return -EINVAL;
	}

	return 0;
}

static int i2s_esp32_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	int err;

	switch (dir) {
	case I2S_DIR_RX:
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (stream) {
			err = i2s_esp32_trigger_stream(dev, stream, I2S_DIR_RX, cmd);
		} else {
			LOG_ERR("I2S_DIR_RX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_RX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
		break;
	case I2S_DIR_TX:
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (stream) {
			err = i2s_esp32_trigger_stream(dev, stream, I2S_DIR_TX, cmd);
		} else {
			LOG_ERR("I2S_DIR_TX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_TX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
		break;
	case I2S_DIR_BOTH:
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (stream) {
			err = i2s_esp32_trigger_stream(dev, stream, I2S_DIR_TX, cmd);
		} else {
			LOG_ERR("I2S_DIR_TX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_TX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
		if (err < 0) {
			break;
		}
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (stream) {
			err = i2s_esp32_trigger_stream(dev, stream, I2S_DIR_RX, cmd);
		} else {
			LOG_ERR("I2S_DIR_RX not enabled");
			err = -EINVAL;
		}
#else
		LOG_ERR("I2S_DIR_RX not enabled");
		err = -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
		break;
	default:
		LOG_ERR("Invalid direction: %d", (int)dir);
		err = -EINVAL;
	}

	return err;
}

static int i2s_esp32_read(const struct device *dev, void **mem_block, size_t *size)
{
#if I2S_ESP32_IS_DIR_EN(rx)
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->rx;
	struct queue_item item;
	int err;

	if (!stream) {
		*mem_block = NULL;
		*size = 0;

		LOG_ERR("I2S_DIR_RX not enabled");
		return -EINVAL;
	}

	if (stream->data->state == I2S_STATE_NOT_READY) {
		LOG_ERR("RX invalid state: %d", (int)stream->data->state);
		return -EIO;
	} else if (stream->data->state == I2S_STATE_ERROR &&
		   k_msgq_num_used_get(&stream->data->queue) == 0) {
		LOG_ERR("RX queue empty");
		return -EIO;
	}

	err = k_msgq_get(&stream->data->queue, &item,
			 K_MSEC(stream->data->i2s_cfg.timeout));
	if (err < 0) {
		LOG_ERR("RX queue empty");
		return err;
	}

	*mem_block = item.buffer;
	*size = item.size;

	return 0;
#else
	*mem_block = NULL;
	*size = 0;

	LOG_ERR("I2S_DIR_RX not enabled");
	return -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
}

static int i2s_esp32_write(const struct device *dev, void *mem_block, size_t size)
{
#if I2S_ESP32_IS_DIR_EN(tx)
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;
	int err;

	if (!stream) {
		LOG_ERR("I2S_DIR_TX not enabled");
		return -EINVAL;
	}

	if (stream->data->state != I2S_STATE_RUNNING &&
	    stream->data->state != I2S_STATE_READY) {
		LOG_ERR("TX Invalid state: %d", (int)stream->data->state);
		return -EIO;
	}

	if (size > stream->data->i2s_cfg.block_size) {
		LOG_ERR("Max write size is: %zu", stream->data->i2s_cfg.block_size);
		return -EINVAL;
	}

	struct queue_item item = {.buffer = mem_block, .size = size};

	err = k_msgq_put(&stream->data->queue, &item,
			 K_MSEC(stream->data->i2s_cfg.timeout));
	if (err < 0) {
		LOG_ERR("TX queue full");
		return err;
	}

	return 0;
#else
	LOG_ERR("I2S_DIR_TX not enabled");
	return -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
}

static DEVICE_API(i2s, i2s_esp32_driver_api) = {
	.configure = i2s_esp32_configure,
	.config_get = i2s_esp32_config_get,
	.trigger = i2s_esp32_trigger,
	.read = i2s_esp32_read,
	.write = i2s_esp32_write
};

#define I2S_ESP32_STREAM_DECLARE_DMA_DESC(index, dir)                                              \
	lldesc_t i2s_esp32_stream_##index##_##dir##_dma_desc[CONFIG_I2S_ESP32_DMA_DESC_NUM_MAX]

#define I2S_ESP32_STREAM_DECL_DMA_DESC(index, dir)                                                 \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                                              \
		    (I2S_ESP32_STREAM_DECLARE_DMA_DESC(index, dir)), ())

#define I2S_ESP32_STREAM_DECLARE(index, dir)                                                       \
	struct i2s_esp32_stream_data i2s_esp32_stream_##index##_##dir##_data = {                   \
		.state = I2S_STATE_NOT_READY,                                                      \
		.is_slave = false,                                                                 \
		.i2s_cfg = {0},                                                                    \
		.mem_block = NULL,                                                                 \
		.mem_block_len = 0,                                                                \
		.last_block = false,                                                               \
		.stop_without_draining = false,                                                    \
		.queue = {},                                                                       \
		.irq_handle = NULL,                                                                \
		.dma_pending = false                                                               \
	};                                                                                         \
                                                                                                   \
	const struct i2s_esp32_stream_conf i2s_esp32_stream_##index##_##dir##_conf = {             \
		.queue_drop = i2s_esp32_queue_drop,                                                \
		.start_transfer = i2s_esp32_##dir##_start_transfer,                                \
		.stop_transfer = i2s_esp32_##dir##_stop_transfer,                                  \
		.dma_dev = UTIL_AND(DT_INST_DMAS_HAS_NAME(index, dir),                             \
				    DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir))),         \
		.dma_channel = UTIL_AND(DT_INST_DMAS_HAS_NAME(index, dir),                         \
					DT_INST_DMAS_CELL_BY_NAME(index, dir, channel)),           \
		.dma_desc = UTIL_AND(DT_INST_IRQ_HAS_NAME(index, dir),                             \
					i2s_esp32_stream_##index##_##dir##_dma_desc),              \
		.irq_source = COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                        \
					  (DT_INST_IRQ_BY_NAME(index, dir, irq)), (-1)),           \
		.irq_priority = COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                      \
					    (DT_INST_IRQ_BY_NAME(index, dir, priority)), (-1)),    \
		.irq_flags = COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                         \
					 (DT_INST_IRQ_BY_NAME(index, dir, flags)), (-1))           \
	}

#define I2S_ESP32_STREAM_DECL(index, dir) \
	COND_CODE_1(UTIL_OR(DT_INST_DMAS_HAS_NAME(index, dir), DT_INST_IRQ_HAS_NAME(index, dir)),  \
		    (I2S_ESP32_STREAM_DECLARE(index, dir)), ())

#define I2S_ESP32_STREAM_INIT(index, dir)                                                          \
	.dir = {.conf = UTIL_AND(UTIL_OR(DT_INST_DMAS_HAS_NAME(index, dir),                        \
					 DT_INST_IRQ_HAS_NAME(index, dir)),                        \
				 &i2s_esp32_stream_##index##_##dir##_conf),                        \
		.data = UTIL_AND(UTIL_OR(DT_INST_DMAS_HAS_NAME(index, dir),                        \
					 DT_INST_IRQ_HAS_NAME(index, dir)),                        \
				 &i2s_esp32_stream_##index##_##dir##_data)}

#define I2S_ESP32_INIT(index)                                                                      \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	I2S_ESP32_STREAM_DECL_DMA_DESC(index, rx);                                                 \
	I2S_ESP32_STREAM_DECL(index, rx);                                                          \
                                                                                                   \
	I2S_ESP32_STREAM_DECL_DMA_DESC(index, tx);                                                 \
	I2S_ESP32_STREAM_DECL(index, tx);                                                          \
                                                                                                   \
	static struct i2s_esp32_data i2s_esp32_data_##index = {                                    \
		.clk_info = {0}};                                                                  \
                                                                                                   \
	static const struct i2s_esp32_cfg i2s_esp32_config_##index = {                             \
		.unit = DT_PROP(DT_DRV_INST(index), unit),                                         \
		.hal = {.dev = (i2s_dev_t *)DT_INST_REG_ADDR(index)},                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(index, offset),        \
		I2S_ESP32_STREAM_INIT(index, rx), I2S_ESP32_STREAM_INIT(index, tx)};               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_esp32_initialize, NULL, &i2s_esp32_data_##index,         \
			      &i2s_esp32_config_##index, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,    \
			      &i2s_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_ESP32_INIT)
