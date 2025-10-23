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

#if defined(CONFIG_SOC_SERIES_ESP32) && defined(CONFIG_ADC_ESP32_DMA) &&                           \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2s0))
#error "i2s0 must be disabled if ADC_ESP32_DMA is enabled for ESP32"
#endif

LOG_MODULE_REGISTER(i2s_esp32, CONFIG_I2S_LOG_LEVEL);

#define I2S_ESP32_CLK_SRC             I2S_CLK_SRC_DEFAULT
#define I2S_ESP32_DMA_BUFFER_MAX_SIZE 4092

#define I2S_ESP32_NUM_INST_OK          DT_NUM_INST_STATUS_OKAY(espressif_esp32_i2s)
#define I2S_ESP32_IS_DIR_INST_EN(i, d) DT_INST_DMAS_HAS_NAME(i, d) || DT_INST_IRQ_HAS_NAME(i, d)
#define I2S_ESP32_IS_DIR_EN(d)         (LISTIFY(I2S_ESP32_NUM_INST_OK, I2S_ESP32_IS_DIR_INST_EN,   \
						(||), d))

struct queue_item {
	void *buffer;
	size_t size;
};

struct i2s_esp32_stream_data {
	bool configured;
	bool transferring;
	struct i2s_config i2s_cfg;
	void *mem_block;
	size_t mem_block_len;
	struct k_msgq queue;
#if !SOC_GDMA_SUPPORTED
	struct intr_handle_data_t *irq_handle;
#endif
	bool dma_pending;
	uint8_t chunks_rem;
	uint8_t chunk_idx;
};

struct i2s_esp32_stream_conf {
#if SOC_GDMA_SUPPORTED
	const struct device *dma_dev;
	uint32_t dma_channel;
#else
	lldesc_t *dma_desc;
	int irq_source;
	int irq_priority;
	int irq_flags;
#endif
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
	int32_t state;
	enum i2s_dir active_dir;
	bool tx_stop_without_draining;
	i2s_hal_clock_info_t clk_info;
#if I2S_ESP32_IS_DIR_EN(tx)
	struct k_timer tx_deferred_transfer_timer;
	const struct device *dev;
#endif
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
		return ESP_ERR_INVALID_ARG;
	}

	if (i2s_hal_clock_info == NULL) {
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
		LOG_DBG("Sample rate is too large for the current clock source");
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

static void i2s_esp32_queue_drop(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	struct queue_item item;

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		stream = &dev_cfg->rx;

		while (k_msgq_get(&stream->data->queue, &item, K_NO_WAIT) == 0) {
			k_mem_slab_free(stream->data->i2s_cfg.mem_slab, item.buffer);
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		stream = &dev_cfg->tx;

		while (k_msgq_get(&stream->data->queue, &item, K_NO_WAIT) == 0) {
			k_mem_slab_free(stream->data->i2s_cfg.mem_slab, item.buffer);
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
}

static int i2s_esp32_restart_dma(const struct device *dev, enum i2s_dir dir);
static int i2s_esp32_start_dma(const struct device *dev, enum i2s_dir dir);

#if I2S_ESP32_IS_DIR_EN(rx)

static void i2s_esp32_rx_stop_transfer(const struct device *dev);

#if SOC_GDMA_SUPPORTED
static void i2s_esp32_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
#else
static void i2s_esp32_rx_callback(void *arg, int status)
#endif /* SOC_GDMA_SUPPORTED */
{
	const struct device *dev = (const struct device *)arg;
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->rx;
	int err;

	if (!stream->data->dma_pending) {
		return;
	}

	stream->data->dma_pending = false;

	if (stream->data->mem_block == NULL) {
		LOG_DBG("RX mem_block NULL");
		dev_data->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

#if SOC_GDMA_SUPPORTED
	if (status < 0) {
#else
	if (status & I2S_LL_EVENT_RX_DSCR_ERR) {
#endif /* SOC_GDMA_SUPPORTED */
		dev_data->state = I2S_STATE_ERROR;
		LOG_DBG("RX status bad: %d", status);
		goto rx_disable;
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
			LOG_DBG("Failed to reload DMA channel: %" PRIu32,
				stream->conf->dma_channel);
			goto rx_disable;
		}

		i2s_ll_rx_set_eof_num(hal->dev, chunk_len);

		err = dma_start(stream->conf->dma_dev, stream->conf->dma_channel);
		if (err < 0) {
			LOG_DBG("Failed to start DMA channel: %" PRIu32, stream->conf->dma_channel);
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
		LOG_DBG("RX queue full");
		dev_data->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	if (dev_data->state == I2S_STATE_STOPPING) {
		if (dev_data->active_dir == I2S_DIR_RX ||
		    (dev_data->active_dir == I2S_DIR_BOTH && !dev_cfg->tx.data->transferring)) {
			dev_data->state = I2S_STATE_READY;
			goto rx_disable;
		}
	}

	err = k_mem_slab_alloc(stream->data->i2s_cfg.mem_slab, &stream->data->mem_block, K_NO_WAIT);
	if (err < 0) {
		LOG_DBG("RX failed to allocate memory from slab: %i:", err);
		dev_data->state = I2S_STATE_ERROR;
		goto rx_disable;
	}
	stream->data->mem_block_len = stream->data->i2s_cfg.block_size;

	err = i2s_esp32_restart_dma(dev, I2S_DIR_RX);
	if (err < 0) {
		LOG_DBG("Failed to restart RX transfer: %d", err);
		k_mem_slab_free(stream->data->i2s_cfg.mem_slab, stream->data->mem_block);
		stream->data->mem_block = NULL;
		stream->data->mem_block_len = 0;
		dev_data->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	return;

rx_disable:
	i2s_esp32_rx_stop_transfer(dev);
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

	i2s_hal_rx_stop(hal);
	i2s_hal_rx_reset(hal);
#if !SOC_GDMA_SUPPORTED
	i2s_hal_rx_reset_dma(hal);
#endif /* !SOC_GDMA_SUPPORTED */
	i2s_hal_rx_reset_fifo(hal);

	err = i2s_esp32_start_dma(dev, I2S_DIR_RX);
	if (err < 0) {
		LOG_DBG("Failed to start RX DMA transfer: %d", err);
		k_mem_slab_free(stream->data->i2s_cfg.mem_slab, stream->data->mem_block);
		stream->data->mem_block = NULL;
		stream->data->mem_block_len = 0;
		return -EIO;
	}

	i2s_hal_rx_start(hal);

#if !SOC_GDMA_SUPPORTED
	esp_intr_enable(stream->data->irq_handle);
#endif /* !SOC_GDMA_SUPPORTED */

	stream->data->transferring = true;

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

	stream->data->mem_block = NULL;
	stream->data->mem_block_len = 0;

	stream->data->transferring = false;
}

#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)

static void i2s_esp32_tx_stop_transfer(const struct device *dev);

void i2s_esp32_tx_compl_transfer(struct k_timer *timer)
{
	struct i2s_esp32_data *dev_data =
		CONTAINER_OF(timer, struct i2s_esp32_data, tx_deferred_transfer_timer);
	const struct device *dev = dev_data->dev;
	const struct i2s_esp32_cfg *const dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;
	struct queue_item item;
	int err;

	if (dev_data->state == I2S_STATE_ERROR) {
		goto tx_disable;
	}

	if (dev_data->state == I2S_STATE_STOPPING) {
		if (k_msgq_num_used_get(&stream->data->queue) == 0 ||
		    dev_data->tx_stop_without_draining == true) {
			if (dev_data->active_dir == I2S_DIR_TX ||
			    (dev_data->active_dir == I2S_DIR_BOTH &&
			     !dev_cfg->rx.data->transferring)) {
				dev_data->state = I2S_STATE_READY;
			}
			goto tx_disable;
		}
	}

	err = k_msgq_get(&stream->data->queue, &item, K_NO_WAIT);
	if (err < 0) {
		dev_data->state = I2S_STATE_ERROR;
		LOG_DBG("TX queue empty: %d", err);
		goto tx_disable;
	}

	stream->data->mem_block = item.buffer;
	stream->data->mem_block_len = item.size;

	err = i2s_esp32_restart_dma(dev, I2S_DIR_TX);
	if (err < 0) {
		dev_data->state = I2S_STATE_ERROR;
		LOG_DBG("Failed to restart TX transfer: %d", err);
		goto tx_disable;
	}

	return;

tx_disable:
	i2s_esp32_tx_stop_transfer(dev);
}

#if SOC_GDMA_SUPPORTED
static void i2s_esp32_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
#else
static void i2s_esp32_tx_callback(void *arg, int status)
#endif /* SOC_GDMA_SUPPORTED */
{
	const struct device *dev = (const struct device *)arg;
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *const dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;

	if (dev_data->state == I2S_STATE_ERROR) {
		goto tx_disable;
	}

	if (!stream->data->dma_pending) {
		return;
	}

	stream->data->dma_pending = false;

	if (stream->data->mem_block == NULL) {
		LOG_DBG("TX mem_block NULL");
		dev_data->state = I2S_STATE_ERROR;
		goto tx_disable;
	}

	k_mem_slab_free(stream->data->i2s_cfg.mem_slab, stream->data->mem_block);

#if SOC_GDMA_SUPPORTED
	if (status < 0) {
#else
	if (status & I2S_LL_EVENT_TX_DSCR_ERR) {
#endif /* SOC_GDMA_SUPPORTED */
		dev_data->state = I2S_STATE_ERROR;
		LOG_DBG("TX bad status: %d", status);
		goto tx_disable;
	}

#if CONFIG_I2S_ESP32_ALLOWED_EMPTY_TX_QUEUE_DEFERRAL_TIME_MS
	if (k_msgq_num_used_get(&stream->data->queue) == 0) {
		k_timer_start(&dev_data->tx_deferred_transfer_timer,
			      K_MSEC(CONFIG_I2S_ESP32_ALLOWED_EMPTY_TX_QUEUE_DEFERRAL_TIME_MS),
			      K_NO_WAIT);
	} else {
#else
	{
#endif
		i2s_esp32_tx_compl_transfer(&dev_data->tx_deferred_transfer_timer);
	}

	return;

tx_disable:
	i2s_esp32_tx_stop_transfer(dev);
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

	i2s_hal_tx_stop(hal);
	i2s_hal_tx_reset(hal);
#if !SOC_GDMA_SUPPORTED
	i2s_hal_tx_reset_dma(hal);
#endif /* !SOC_GDMA_SUPPORTED */
	i2s_hal_tx_reset_fifo(hal);

	err = i2s_esp32_start_dma(dev, I2S_DIR_TX);
	if (err < 0) {
		LOG_DBG("Failed to start TX DMA transfer: %d", err);
		return -EIO;
	}

	i2s_hal_tx_start(hal);

#if !SOC_GDMA_SUPPORTED
	esp_intr_enable(stream->data->irq_handle);
#endif /* !SOC_GDMA_SUPPORTED */

	stream->data->transferring = true;

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

	stream->data->mem_block = NULL;
	stream->data->mem_block_len = 0;

	stream->data->transferring = false;
}

#endif /* I2S_ESP32_IS_DIR_EN(tx) */

static int i2s_esp32_start_transfer(const struct device *dev, enum i2s_dir dir)
{
	int err = 0;

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		err = i2s_esp32_rx_start_transfer(dev);
		if (err < 0) {
			LOG_DBG("RX failed to start transfer");
			goto start_transfer_end;
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		err = i2s_esp32_tx_start_transfer(dev);
		if (err < 0) {
			LOG_DBG("TX failed to start transfer");
			goto start_transfer_end;
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

start_transfer_end:
#if I2S_ESP32_IS_DIR_EN(rx) && I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_BOTH && err < 0) {
		const struct i2s_esp32_cfg *dev_cfg = dev->config;

		if (dev_cfg->rx.data->transferring && !dev_cfg->tx.data->transferring) {
			i2s_esp32_rx_stop_transfer(dev);
		}

		if (!dev_cfg->rx.data->transferring && dev_cfg->tx.data->transferring) {
			i2s_esp32_tx_stop_transfer(dev);
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) && I2S_ESP32_IS_DIR_EN(tx) */

	return err;
}

static void i2s_esp32_stop_transfer(const struct device *dev, enum i2s_dir dir)
{
#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_esp32_rx_stop_transfer(dev);
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_esp32_tx_stop_transfer(dev);
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
}

static bool i2s_esp32_try_stop_transfer(const struct device *dev, enum i2s_dir dir,
					enum i2s_trigger_cmd cmd)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	bool at_least_one_dir_with_pending_transfer = false;

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		stream = &dev_cfg->rx;
		if (stream->data->dma_pending) {
			at_least_one_dir_with_pending_transfer = true;
		} else {
			i2s_esp32_rx_stop_transfer(dev);
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		stream = &dev_cfg->tx;
		if ((cmd == I2S_TRIGGER_DRAIN && k_msgq_num_used_get(&stream->data->queue) > 0) ||
		    stream->data->dma_pending) {
			at_least_one_dir_with_pending_transfer = true;
		} else {
			i2s_esp32_tx_stop_transfer(dev);
		}
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

	return at_least_one_dir_with_pending_transfer;
}

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
		LOG_DBG("Failed to configure DMA channel: %" PRIu32, stream->conf->dma_channel);
		return -EINVAL;
	}
#else
	lldesc_t *desc_iter = stream->conf->dma_desc;

	if (!mem_block) {
		LOG_DBG("At least one dma block is required");
		return -EINVAL;
	}

	if (!esp_ptr_dma_capable((void *)mem_block)) {
		LOG_DBG("Buffer is not in DMA capable memory: %p", (uint32_t *)mem_block);

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
		LOG_DBG("Run out of descriptors. Increase CONFIG_I2S_ESP32_DMA_DESC_NUM_MAX");
		return -EINVAL;
	}
#endif /* SOC_GDMA_SUPPORTED */

	return 0;
}

static int i2s_esp32_start_dma(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = NULL;
	unsigned int key;
	int err = 0;

#if !SOC_GDMA_SUPPORTED || I2S_ESP32_IS_DIR_EN(rx)
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
#endif /* SOC_GDMA_SUPPORTED || I2S_ESP32_IS_DIR_EN(rx) */

	if (dir == I2S_DIR_RX) {
		stream = &dev_cfg->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_cfg->tx;
	} else {
		LOG_DBG("Invalid DMA direction");
		return -EINVAL;
	}

	key = irq_lock();

	err = i2s_esp32_config_dma(dev, dir, stream);
	if (err < 0) {
		LOG_DBG("Dma configuration failed: %i", err);
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
		LOG_DBG("Failed to start DMA channel: %" PRIu32, stream->conf->dma_channel);
		goto unlock;
	}
#else
#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX) {
		i2s_hal_rx_enable_dma(hal);
		i2s_hal_rx_enable_intr(hal);
		i2s_hal_rx_start_link(hal, (uint32_t)&(stream->conf->dma_desc[0]));
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX) {
		i2s_hal_tx_enable_dma(hal);
		i2s_hal_tx_enable_intr(hal);
		i2s_hal_tx_start_link(hal, (uint32_t)&(stream->conf->dma_desc[0]));
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
#endif /* SOC_GDMA_SUPPORTED */

	stream->data->dma_pending = true;

unlock:
	irq_unlock(key);
	return err;
}

static int i2s_esp32_restart_dma(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	int err = 0;

#if !SOC_GDMA_SUPPORTED || I2S_ESP32_IS_DIR_EN(rx)
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
#endif /* SOC_GDMA_SUPPORTED || I2S_ESP32_IS_DIR_EN(rx) */

	if (dir == I2S_DIR_RX) {
		stream = &dev_cfg->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_cfg->tx;
	} else {
		LOG_DBG("Invalid DMA direction");
		return -EINVAL;
	}

#if SOC_GDMA_SUPPORTED
	uint16_t chunk_len;
	void *src = NULL, *dst = NULL;

#if I2S_ESP32_IS_DIR_EN(rx)
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
		LOG_DBG("Failed to reload DMA channel: %" PRIu32, stream->conf->dma_channel);
		return -EIO;
	}

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX) {
		i2s_ll_rx_set_eof_num(hal->dev, chunk_len);
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

	err = dma_start(stream->conf->dma_dev, stream->conf->dma_channel);
	if (err < 0) {
		LOG_DBG("Failed to start DMA channel: %" PRIu32, stream->conf->dma_channel);
		return -EIO;
	}
#else
	err = i2s_esp32_config_dma(dev, dir, stream);
	if (err < 0) {
		LOG_DBG("Failed to configure DMA");
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
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct device *clk_dev = dev_cfg->clock_dev;
	const struct i2s_esp32_stream *stream;
	int err;

#if !SOC_GDMA_SUPPORTED
	const i2s_hal_context_t *hal = &(dev_cfg->hal);
#endif /* !SOC_GDMA_SUPPORTED */

	if (!device_is_ready(clk_dev)) {
		LOG_DBG("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(clk_dev, dev_cfg->clock_subsys);
	if (err != 0) {
		LOG_DBG("Clock control enabling failed: %d", err);
		return -EIO;
	}

	err = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_DBG("Pins setup failed: %d", err);
		return -EIO;
	}

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dev_cfg->rx.data && dev_cfg->rx.conf) {
		stream = &dev_cfg->rx;
#if SOC_GDMA_SUPPORTED
		if (stream->conf->dma_dev && !device_is_ready(stream->conf->dma_dev)) {
			LOG_DBG("%s device not ready", stream->conf->dma_dev->name);
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
			LOG_DBG("Could not allocate rx interrupt (err %d)", err);
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
	dev_data->dev = dev;
	k_timer_init(&dev_data->tx_deferred_transfer_timer, i2s_esp32_tx_compl_transfer, NULL);

	if (dev_cfg->tx.data && dev_cfg->tx.conf) {
		stream = &dev_cfg->tx;
#if SOC_GDMA_SUPPORTED
		if (stream->conf->dma_dev && !device_is_ready(stream->conf->dma_dev)) {
			LOG_DBG("%s device not ready", stream->conf->dma_dev->name);
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
			LOG_DBG("Could not allocate tx interrupt (err %d)", err);
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

	dev_data->state = I2S_STATE_NOT_READY;

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

static int i2s_esp32_config_check(const struct device *dev, enum i2s_dir dir,
				  const struct i2s_config *i2s_cfg)
{
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	uint8_t data_format;

	if (dir == I2S_DIR_BOTH &&
	    (!dev_cfg->rx.conf || !dev_cfg->rx.data || !dev_cfg->tx.conf || !dev_cfg->tx.data)) {
		LOG_DBG("I2S_DIR_BOTH not supported");
		return -ENOSYS;
	}

	if (dev_data->state != I2S_STATE_NOT_READY && dev_data->state != I2S_STATE_READY) {
		LOG_DBG("Invalid state: %d", (int)dev_data->state);
		return -EINVAL;
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (!stream->data || !stream->conf) {
			LOG_DBG("RX not enabled");
			return -EINVAL;
		}

#if SOC_GDMA_SUPPORTED
		if (stream->conf->dma_dev == NULL) {
			LOG_DBG("RX DMA controller not available");
#else
		if (stream->conf->irq_source == -1) {
			LOG_DBG("RX IRQ source not available");
#endif /* SOC_GDMA_SUPPORTED */
			return -EINVAL;
		}
#else
		LOG_DBG("RX not enabled");
		return -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (!stream->data || !stream->conf) {
			LOG_DBG("TX not enabled");
			return -EINVAL;
		}

#if SOC_GDMA_SUPPORTED
		if (stream->conf->dma_dev == NULL) {
			LOG_DBG("TX DMA controller not available");
#else
		if (stream->conf->irq_source == -1) {
			LOG_DBG("TX IRQ source not available");
#endif /* SOC_GDMA_SUPPORTED */
			return -EINVAL;
		}
#else
		LOG_DBG("TX not enabled");
		return -EINVAL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		return 0;
	}

	if (i2s_cfg->mem_slab == NULL) {
		LOG_DBG("Memory slab is NULL");
		return -EINVAL;
	}

	if (i2s_cfg->block_size == 0) {
		LOG_DBG("Block size is 0");
		return -EINVAL;
	}

	data_format = i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK;

	if (data_format != I2S_FMT_DATA_FORMAT_I2S &&
	    data_format != I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED &&
	    data_format != I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
		LOG_DBG("Invalid data format: %u", (unsigned int)data_format);
		return -EINVAL;
	}

	if (data_format == I2S_FMT_DATA_FORMAT_I2S && i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) {
		LOG_DBG("Invalid format: %u", (unsigned int)i2s_cfg->format);
		return -EINVAL;
	}

	if (i2s_cfg->word_size != 8 && i2s_cfg->word_size != 16 && i2s_cfg->word_size != 24 &&
	    i2s_cfg->word_size != 32) {
		LOG_DBG("Word size not supported: %d", (int)i2s_cfg->word_size);
		return -EINVAL;
	}

	if (i2s_cfg->channels != 2) {
		LOG_DBG("Currently only 2 channels are supported");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_LOOPBACK) {
		LOG_DBG("Unsupported option: I2S_OPT_LOOPBACK");
		LOG_DBG("To enable loopback, use the same SD for TX and RX pinctrl");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_PINGPONG) {
		LOG_DBG("Unsupported option: I2S_OPT_PINGPONG");
		return -EINVAL;
	}

	return 0;
}

static int i2s_esp32_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;
	i2s_hal_slot_config_t slot_cfg = {0};
	uint8_t data_format;
	bool is_slave;
	int err;

	err = i2s_esp32_config_check(dev, dir, i2s_cfg);
	if (err < 0) {
		return err;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		i2s_esp32_queue_drop(dev, dir);

#if I2S_ESP32_IS_DIR_EN(rx)
		if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
			stream = &dev_cfg->rx;
			memset(&stream->data->i2s_cfg, 0, sizeof(struct i2s_config));
			stream->data->configured = false;
		}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
		if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
			stream = &dev_cfg->tx;
			memset(&stream->data->i2s_cfg, 0, sizeof(struct i2s_config));
			stream->data->configured = false;
		}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

		dev_data->state = I2S_STATE_NOT_READY;

		return 0;
	}

	if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) != 0 &&
	    (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) != 0) {
		is_slave = true;
	} else if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) == 0 &&
		   (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) == 0) {
		is_slave = false;
	} else {
		LOG_DBG("I2S_OPT_FRAME_CLK and I2S_OPT_BIT_CLK options are incompatible");
		return -EINVAL;
	}

	data_format = i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK;

	slot_cfg.data_bit_width = i2s_cfg->word_size;
	slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
	slot_cfg.slot_bit_width = i2s_cfg->word_size > 16 ? 32 : 16;
	if (data_format == I2S_FMT_DATA_FORMAT_I2S) {
		slot_cfg.std.ws_pol = i2s_cfg->format & I2S_FMT_FRAME_CLK_INV ? true : false;
		slot_cfg.std.bit_shift = true;
#if SOC_I2S_HW_VERSION_2
		slot_cfg.std.left_align = true;
#endif /* SOC_I2S_HW_VERSION_2 */
	} else if (data_format == I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) {
		slot_cfg.std.ws_pol = i2s_cfg->format & I2S_FMT_FRAME_CLK_INV ? false : true;
		slot_cfg.std.bit_shift = false;
#if SOC_I2S_HW_VERSION_2
		slot_cfg.std.left_align = true;
	} else if (data_format == I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
		slot_cfg.std.ws_pol = i2s_cfg->format & I2S_FMT_FRAME_CLK_INV ? false : true;
		slot_cfg.std.bit_shift = false;
		slot_cfg.std.left_align = false;
#endif /* SOC_I2S_HW_VERSION_2 */
	} else {
		LOG_DBG("Unsupported data format: %u", (unsigned int)data_format);
		return -EINVAL;
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

#if I2S_ESP32_IS_DIR_EN(rx)
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		bool rx_is_slave;

		rx_is_slave = is_slave;
		if (dir == I2S_DIR_BOTH || (dev_cfg->tx.data && dev_cfg->tx.data->configured)) {
			rx_is_slave = true;
		}

		i2s_hal_std_set_rx_slot(hal, rx_is_slave, &slot_cfg);
		i2s_hal_set_rx_clock(hal, &i2s_hal_clock_info, I2S_ESP32_CLK_SRC);
		i2s_ll_rx_enable_std(hal->dev);

		stream = &dev_cfg->rx;
		memcpy(&stream->data->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));
		stream->data->configured = true;
	}
#endif /* I2S_ESP32_IS_DIR_EN(rx) */

#if I2S_ESP32_IS_DIR_EN(tx)
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_hal_std_set_tx_slot(hal, is_slave, &slot_cfg);
		i2s_hal_set_tx_clock(hal, &i2s_hal_clock_info, I2S_ESP32_CLK_SRC);
		i2s_ll_tx_enable_std(hal->dev);

		stream = &dev_cfg->tx;
		memcpy(&stream->data->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));
		stream->data->configured = true;
	}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */

	if (dev_cfg->rx.data && dev_cfg->rx.data->configured && dev_cfg->tx.data &&
	    dev_cfg->tx.data->configured) {
		i2s_ll_share_bck_ws(hal->dev, true);
	} else {
		i2s_ll_share_bck_ws(hal->dev, false);
	}

	dev_data->state = I2S_STATE_READY;

	return 0;
}

static const struct i2s_config *i2s_esp32_config_get(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream;

	if (dir == I2S_DIR_RX) {
#if I2S_ESP32_IS_DIR_EN(rx)
		stream = &dev_cfg->rx;
		if (!stream->data) {
			LOG_DBG("RX not enabled");
			return NULL;
		}
#else
		LOG_DBG("RX not enabled");
		return NULL;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
	} else if (dir == I2S_DIR_TX) {
#if I2S_ESP32_IS_DIR_EN(tx)
		stream = &dev_cfg->tx;
		if (!stream->data) {
			LOG_DBG("TX not enabled");
			return NULL;
		}
#else
		LOG_DBG("TX not enabled");
		return NULL;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
	} else {
		LOG_DBG("Invalid direction: %d", (int)dir);
		return NULL;
	}

	if (!stream->data->configured) {
		return NULL;
	}

	return &stream->data->i2s_cfg;
}

static int i2s_esp32_trigger_check(const struct device *dev, enum i2s_dir dir,
				   enum i2s_trigger_cmd cmd)
{
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	bool configured, cmd_allowed;
	enum i2s_state state;

	if (dir == I2S_DIR_BOTH) {
		if (dev_cfg->rx.conf && dev_cfg->rx.data && dev_cfg->tx.conf && dev_cfg->tx.data) {
			configured = dev_cfg->rx.data->configured && dev_cfg->tx.data->configured;
		} else {
			LOG_DBG("I2S_DIR_BOTH not supported");
			return -ENOSYS;
		}
	} else if (dir == I2S_DIR_RX) {
		configured = dev_cfg->rx.data->configured;
	} else if (dir == I2S_DIR_TX) {
		configured = dev_cfg->tx.data->configured;
	} else {
		LOG_DBG("Invalid dir: %d", dir);
		return -EINVAL;
	}

	if (!configured) {
		LOG_DBG("Device is not configured");
		return -EIO;
	}

	state = dev_data->state;
	switch (cmd) {
	case I2S_TRIGGER_START:
		cmd_allowed = (state == I2S_STATE_READY);
		break;
	case I2S_TRIGGER_STOP:
		__fallthrough;
	case I2S_TRIGGER_DRAIN:
		cmd_allowed = (state == I2S_STATE_RUNNING);
		break;
	case I2S_TRIGGER_DROP:
		cmd_allowed = (state != I2S_STATE_NOT_READY) && configured;
		break;
	case I2S_TRIGGER_PREPARE:
		cmd_allowed = (state == I2S_STATE_ERROR);
		break;
	default:
		LOG_DBG("Invalid trigger: %d", cmd);
		return -EINVAL;
	}

	if (!cmd_allowed) {
		switch (cmd) {
		case I2S_TRIGGER_START:
			LOG_DBG("START - Invalid state: %d", (int)state);
			break;
		case I2S_TRIGGER_STOP:
			LOG_DBG("STOP - Invalid state: %d", (int)state);
			break;
		case I2S_TRIGGER_DRAIN:
			LOG_DBG("DRAIN - Invalid state: %d", (int)state);
			break;
		case I2S_TRIGGER_DROP:
			LOG_DBG("DROP - invalid state: %d", (int)state);
			break;
		case I2S_TRIGGER_PREPARE:
			LOG_DBG("PREPARE - invalid state: %d", (int)state);
			break;
		default:
			LOG_DBG("Invalid trigger: %d", cmd);
		}

		return -EIO;
	}

	return 0;
}

static int i2s_esp32_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_esp32_data *dev_data = dev->data;
	bool at_least_one_dir_with_pending_transfer;
	unsigned int key;
	int err;

	err = i2s_esp32_trigger_check(dev, dir, cmd);
	if (err < 0) {
		return err;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		dev_data->active_dir = dir;
		dev_data->tx_stop_without_draining = true;
		err = i2s_esp32_start_transfer(dev, dir);
		if (err < 0) {
			LOG_DBG("START - Transfer start failed: %d", err);
			return -EIO;
		}
		dev_data->state = I2S_STATE_RUNNING;
		break;
	case I2S_TRIGGER_STOP:
		__fallthrough;
	case I2S_TRIGGER_DRAIN:
		if (dev_data->active_dir != dir) {
			LOG_DBG("Trigger dir (%d) different from active dir (%d)", dir,
				dev_data->active_dir);
			return -EINVAL;
		}

		key = irq_lock();
		at_least_one_dir_with_pending_transfer = i2s_esp32_try_stop_transfer(dev, dir, cmd);
		if (at_least_one_dir_with_pending_transfer) {
#if I2S_ESP32_IS_DIR_EN(tx)
			if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
				switch (cmd) {
				case I2S_TRIGGER_STOP:
					dev_data->tx_stop_without_draining = true;
					break;
				case I2S_TRIGGER_DRAIN:
					dev_data->tx_stop_without_draining = false;
				default:
				}
			}
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
			dev_data->state = I2S_STATE_STOPPING;
		} else {
			dev_data->state = I2S_STATE_READY;
		}
		irq_unlock(key);
		break;
	case I2S_TRIGGER_DROP:
		if (dev_data->state == I2S_STATE_RUNNING && dev_data->active_dir != dir) {
			LOG_DBG("Trigger dir (%d) different from active dir (%d)", dir,
				dev_data->active_dir);
			return -EINVAL;
		}

		key = irq_lock();
		i2s_esp32_stop_transfer(dev, dir);
		i2s_esp32_queue_drop(dev, dir);
		dev_data->state = I2S_STATE_READY;
		irq_unlock(key);
		break;
	case I2S_TRIGGER_PREPARE:
		i2s_esp32_queue_drop(dev, dir);
		dev_data->state = I2S_STATE_READY;
		break;
	default:
		LOG_DBG("Unsupported trigger command: %d", (int)cmd);
		return -EIO;
	}

	return 0;
}

static int i2s_esp32_read(const struct device *dev, void **mem_block, size_t *size)
{
#if I2S_ESP32_IS_DIR_EN(rx)
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->rx;
	enum i2s_state state = dev_data->state;
	struct queue_item item;
	int err;

	if (!stream->data || !stream->conf) {
		LOG_DBG("RX not enabled");
		return -EIO;
	}

	if (!stream->data->configured) {
		LOG_DBG("RX not configured");
		return -EIO;
	}

	if (state == I2S_STATE_NOT_READY) {
		LOG_DBG("Invalid state: %d", (int)state);
		return -EIO;
	}

	err = k_msgq_get(&stream->data->queue, &item,
			 (state == I2S_STATE_ERROR) ? K_NO_WAIT
						    : K_MSEC(stream->data->i2s_cfg.timeout));
	if (err == 0) {
		*mem_block = item.buffer;
		*size = item.size;
	} else {
		LOG_DBG("RX queue empty");

		if (err == -ENOMSG) {
			err = -EIO;
		}
	}

	return err;
#else
	LOG_DBG("RX not enabled");
	return -EIO;
#endif /* I2S_ESP32_IS_DIR_EN(rx) */
}

static int i2s_esp32_write(const struct device *dev, void *mem_block, size_t size)
{
#if I2S_ESP32_IS_DIR_EN(tx)
	struct i2s_esp32_data *dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	const struct i2s_esp32_stream *stream = &dev_cfg->tx;
	enum i2s_state state = dev_data->state;
	int err;

	if (!stream->data || !stream->conf) {
		LOG_DBG("TX not enabled");
		return -EIO;
	}

	if (!stream->data->configured) {
		LOG_DBG("TX not configured");
		return -EIO;
	}

	if (state != I2S_STATE_RUNNING && state != I2S_STATE_READY) {
		LOG_DBG("Invalid state: %d", (int)state);
		return -EIO;
	}

	if (size > stream->data->i2s_cfg.block_size) {
		LOG_DBG("Max write size is: %u", stream->data->i2s_cfg.block_size);
		return -EIO;
	}

	struct queue_item item = {.buffer = mem_block, .size = size};

	err = k_msgq_put(&stream->data->queue, &item,
			 K_MSEC(stream->data->i2s_cfg.timeout));
	if (err < 0) {
		LOG_DBG("TX queue full");
	}

	return err;
#else
	LOG_DBG("TX not enabled");
	return -EIO;
#endif /* I2S_ESP32_IS_DIR_EN(tx) */
}

static DEVICE_API(i2s, i2s_esp32_driver_api) = {
	.configure = i2s_esp32_configure,
	.config_get = i2s_esp32_config_get,
	.trigger = i2s_esp32_trigger,
	.read = i2s_esp32_read,
	.write = i2s_esp32_write
};

#if SOC_GDMA_SUPPORTED

#define I2S_ESP32_DT_INST_SANITY_CHECK(index)                                                      \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(index, dmas), "Missing property: dmas");                \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(index, dma_names), "Missing property: dma-names");

#define I2S_ESP32_STREAM_DECL_DESC(index, rx)

#define I2S_ESP32_STREAM_DECLARE_DATA(index, dir)

#define I2S_ESP32_STREAM_DECLARE_CONF(index, dir)                                                  \
	.dma_dev = UTIL_AND(DT_INST_DMAS_HAS_NAME(index, dir),                                     \
			    DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir))),                 \
	.dma_channel = UTIL_AND(DT_INST_DMAS_HAS_NAME(index, dir),                                 \
				DT_INST_DMAS_CELL_BY_NAME(index, dir, channel))

#else

#define I2S_ESP32_DT_INST_SANITY_CHECK(index)                                                      \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(index, dmas), "Unexpected property: dmas");            \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(index, dma_names), "Unexpected property: dma-names");  \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(index, interrupt_names),                                \
		     "Missing property: interrupt-names")

#define I2S_ESP32_STREAM_DECL_DESC(index, dir)                                                     \
	lldesc_t i2s_esp32_stream_##index##_##dir##_dma_desc[CONFIG_I2S_ESP32_DMA_DESC_NUM_MAX]

#define I2S_ESP32_STREAM_DECLARE_DATA(index, dir) .irq_handle = NULL

#define I2S_ESP32_STREAM_DECLARE_CONF(index, dir)                                                  \
	.irq_source = COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                                \
				  (DT_INST_IRQ_BY_NAME(index, dir, irq)), (-1)),                   \
	.irq_priority = COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                              \
				    (DT_INST_IRQ_BY_NAME(index, dir, priority)), (-1)),            \
	.irq_flags = COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir),                                 \
				 (DT_INST_IRQ_BY_NAME(index, dir, flags)), (-1)),                  \
	.dma_desc = UTIL_AND(DT_INST_IRQ_HAS_NAME(index, dir),                                     \
			     i2s_esp32_stream_##index##_##dir##_dma_desc)

#endif /* SOC_GDMA_SUPPORTED */

#define I2S_ESP32_STREAM_DECLARE(index, dir)                                                       \
	struct i2s_esp32_stream_data i2s_esp32_stream_##index##_##dir##_data = {                   \
		.configured = false,                                                               \
		.transferring = false,                                                             \
		.i2s_cfg = {0},                                                                    \
		.mem_block = NULL,                                                                 \
		.mem_block_len = 0,                                                                \
		.queue = {},                                                                       \
		.dma_pending = false,                                                              \
		I2S_ESP32_STREAM_DECLARE_DATA(index, dir)};                                        \
                                                                                                   \
	const struct i2s_esp32_stream_conf i2s_esp32_stream_##index##_##dir##_conf = {             \
		I2S_ESP32_STREAM_DECLARE_CONF(index, dir)};

#define I2S_ESP32_STREAM_COND_DECLARE(index, dir)                                                  \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(index, dir), (I2S_ESP32_STREAM_DECL_DESC(index, dir)),    \
		    ());   \
	COND_CODE_1(UTIL_OR(DT_INST_DMAS_HAS_NAME(index, dir), DT_INST_IRQ_HAS_NAME(index, dir)),  \
		    (I2S_ESP32_STREAM_DECLARE(index, dir)), ())

#define I2S_ESP32_STREAM_INIT(index, dir)                                                          \
	COND_CODE_1(UTIL_OR(DT_INST_DMAS_HAS_NAME(index, dir), DT_INST_IRQ_HAS_NAME(index, dir)),  \
		    (.dir = {.conf = &i2s_esp32_stream_##index##_##dir##_conf,                     \
			     .data = &i2s_esp32_stream_##index##_##dir##_data}),                   \
		    (.dir = {.conf = NULL, .data = NULL}))

#define I2S_ESP32_INIT(index)                                                                      \
	I2S_ESP32_DT_INST_SANITY_CHECK(index);                                                     \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	I2S_ESP32_STREAM_COND_DECLARE(index, rx);                                                  \
	I2S_ESP32_STREAM_COND_DECLARE(index, tx);                                                  \
                                                                                                   \
	static struct i2s_esp32_data i2s_esp32_data_##index = {                                    \
		.state = I2S_STATE_NOT_READY,                                                      \
		.tx_stop_without_draining = true,                                                  \
		.clk_info = {0},                                                                   \
	};                                                                                         \
                                                                                                   \
	static const struct i2s_esp32_cfg i2s_esp32_config_##index = {                             \
		.unit = DT_PROP(DT_DRV_INST(index), unit),                                         \
		.hal = {.dev = (i2s_dev_t *)DT_INST_REG_ADDR(index)},                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(index, offset),        \
		I2S_ESP32_STREAM_INIT(index, rx),                                                  \
		I2S_ESP32_STREAM_INIT(index, tx),                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_esp32_initialize, NULL, &i2s_esp32_data_##index,         \
			      &i2s_esp32_config_##index, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,    \
			      &i2s_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_ESP32_INIT)
