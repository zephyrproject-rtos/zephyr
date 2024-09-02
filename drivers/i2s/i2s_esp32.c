/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_i2s

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <esp_clk_tree.h>
#include <hal/i2s_hal.h>

LOG_MODULE_REGISTER(i2s_esp32, CONFIG_I2S_LOG_LEVEL);

#if !SOC_GDMA_SUPPORTED
#error "Only SoCs with GDMA peripheral are supported!"
#endif

#define I2S_ESP32_CLK_SRC I2S_CLK_SRC_DEFAULT

struct queue_item {
	void *buffer;
	size_t size;
};

struct i2s_esp32_stream {
	int32_t state;
	struct i2s_config i2s_cfg;
	bool is_slave;

	const struct device *dma_dev;
	uint32_t dma_channel;

	bool stop_without_draining;

	void *mem_block;
	size_t mem_block_len;
	bool last_block;

	struct k_msgq queue;
	void (*queue_drop)(struct i2s_esp32_stream *stream);

	int (*start_transfer)(const struct device *dev);
	void (*stop_transfer)(const struct device *dev);
};

struct i2s_esp32_cfg {
	const int unit;
	i2s_hal_context_t hal_cxt;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct i2s_esp32_data {
	i2s_hal_clock_info_t clk_info;
	struct i2s_esp32_stream rx;
	struct i2s_esp32_stream tx;
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

static void i2s_esp32_queue_drop(struct i2s_esp32_stream *stream)
{
	struct queue_item item;

	while (k_msgq_get(&stream->queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, item.buffer);
	}
}

static void i2s_esp32_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status);
static void i2s_esp32_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status);

static int i2s_esp32_restart_dma(const struct device *dev, enum i2s_dir i2s_dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream;
	void *src = NULL, *dst = NULL;
	int err;

	if (i2s_dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
		dst = stream->mem_block;
	} else if (i2s_dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
		src = stream->mem_block;
	} else {
		LOG_ERR("Invalid DMA direction");
		return -EINVAL;
	}

	err = dma_reload(stream->dma_dev, stream->dma_channel, (uint32_t)src, (uint32_t)dst,
			 stream->mem_block_len);
	if (err < 0) {
		LOG_ERR("Error reloading DMA channel[%d]: %d", (int)stream->dma_channel, err);
		return -EIO;
	}

	if (i2s_dir == I2S_DIR_RX) {
		i2s_ll_rx_set_eof_num(dev_cfg->hal_cxt.dev, stream->mem_block_len);
	}

	err = dma_start(stream->dma_dev, stream->dma_channel);
	if (err < 0) {
		LOG_ERR("Error starting DMA channel[%d]: %d", (int)stream->dma_channel, err);
		return -EIO;
	}

	return 0;
}

static int i2s_esp32_start_dma(const struct device *dev, enum i2s_dir i2s_dir)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream = NULL;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	unsigned int key;
	int err, ret = 0;

	if (i2s_dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (i2s_dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
	} else {
		LOG_ERR("Invalid DMA direction");
		return -EINVAL;
	}

	key = irq_lock();

	dma_blk.block_size = stream->mem_block_len;
	if (i2s_dir == I2S_DIR_RX) {
		dma_blk.dest_address = (uint32_t)stream->mem_block;
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.dma_callback = i2s_esp32_rx_callback;
	} else {
		dma_blk.source_address = (uint32_t)stream->mem_block;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.dma_callback = i2s_esp32_tx_callback;
	}
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot =
		dev_cfg->unit == 0 ? ESP_GDMA_TRIG_PERIPH_I2S0 : ESP_GDMA_TRIG_PERIPH_I2S1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;

	err = dma_config(stream->dma_dev, stream->dma_channel, &dma_cfg);
	if (err < 0) {
		LOG_ERR("Error configuring DMA channel[%d]: %d", (int)stream->dma_channel, err);
		ret = -EINVAL;
		goto unlock;
	}

	if (i2s_dir == I2S_DIR_RX) {
		i2s_ll_rx_set_eof_num(dev_cfg->hal_cxt.dev, stream->mem_block_len);
	}

	err = dma_start(stream->dma_dev, stream->dma_channel);
	if (err < 0) {
		LOG_ERR("Error starting DMA channel[%d]: %d", (int)stream->dma_channel, err);
		ret = -EIO;
		goto unlock;
	}

unlock:
	irq_unlock(key);
	return ret;
}

static int i2s_esp32_rx_start_transfer(const struct device *dev)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_stream *stream = &dev_data->rx;
	const i2s_hal_context_t *hal_cxt = &dev_cfg->hal_cxt;
	int err;

	err = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (err < 0) {
		return -ENOMEM;
	}
	stream->mem_block_len = stream->i2s_cfg.block_size;

	i2s_hal_rx_stop(hal_cxt);
	i2s_hal_rx_reset(hal_cxt);
	i2s_hal_rx_reset_fifo(hal_cxt);

	err = i2s_esp32_start_dma(dev, I2S_DIR_RX);
	if (err < 0) {
		LOG_ERR("Failed to start RX DMA transfer: %d", err);
		return -EIO;
	}

	i2s_hal_rx_start(hal_cxt);

	return 0;
}

static int i2s_esp32_tx_start_transfer(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream = &dev_data->tx;
	const i2s_hal_context_t *hal_cxt = &dev_cfg->hal_cxt;
	struct queue_item item;
	int err;

	err = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
	if (err < 0) {
		return -ENOMEM;
	}

	stream->mem_block = item.buffer;
	stream->mem_block_len = item.size;

	i2s_hal_tx_stop(hal_cxt);
	i2s_hal_tx_reset(hal_cxt);
	i2s_hal_tx_reset_fifo(hal_cxt);

	err = i2s_esp32_start_dma(dev, I2S_DIR_TX);
	if (err < 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", err);
		return -EIO;
	}

	i2s_hal_tx_start(hal_cxt);

	return 0;
}

static void i2s_esp32_rx_stop_transfer(const struct device *dev)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream = &dev_data->rx;

	dma_stop(stream->dma_dev, stream->dma_channel);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_len = 0;
	}
}

static void i2s_esp32_tx_stop_transfer(const struct device *dev)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream = &dev_data->tx;

	dma_stop(stream->dma_dev, stream->dma_channel);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_len = 0;
	}
}

static void i2s_esp32_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream = &dev_data->rx;
	int err;

	if (status < 0) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("RX status bad: %d", status);
		goto rx_disable;
	}

	if (stream->mem_block == NULL) {
		if (stream->state != I2S_STATE_READY) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("RX mem_block NULL");
			goto rx_disable;
		} else {
			return;
		}
	}

	struct queue_item item = {.buffer = stream->mem_block, .size = stream->mem_block_len};

	err = k_msgq_put(&stream->queue, &item, K_NO_WAIT);
	if (err < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		goto rx_disable;
	}

	err = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (err < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}
	stream->mem_block_len = stream->i2s_cfg.block_size;

	err = i2s_esp32_restart_dma(dev, I2S_DIR_RX);
	if (err < 0) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("Failed to restart RX transfer: %d", err);
		goto rx_disable;
	}

	return;

rx_disable:
	stream->stop_transfer(dev);
}

static void i2s_esp32_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream = &dev_data->tx;
	struct queue_item item;
	void *mem_block_tmp;
	int err;

	if (status < 0) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("TX bad status: %d", status);
		goto tx_disable;
	}

	if (stream->mem_block == NULL) {
		if (stream->state != I2S_STATE_READY) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("TX mem_block NULL");
			goto tx_disable;
		} else {
			return;
		}
	}

	if (stream->state == I2S_STATE_STOPPING) {
		if (k_msgq_num_used_get(&stream->queue) == 0) {
			stream->state = I2S_STATE_READY;
			goto tx_disable;
		} else if (stream->stop_without_draining == true) {
			stream->queue_drop(stream);
			stream->state = I2S_STATE_READY;
			goto tx_disable;
		}
		/*else: DRAIN trigger, so continue until queue is empty*/
	}

	if (stream->last_block) {
		stream->state = I2S_STATE_READY;
		goto tx_disable;
	}

	err = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
	if (err < 0) {
		stream->state = I2S_STATE_ERROR;
		goto tx_disable;
	}

	mem_block_tmp = stream->mem_block;

	stream->mem_block = item.buffer;
	stream->mem_block_len = item.size;

	err = i2s_esp32_restart_dma(dev, I2S_DIR_TX);
	if (err < 0) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("Failed to restart TX transfer: %d", err);
		goto tx_disable;
	}

	k_mem_slab_free(stream->i2s_cfg.mem_slab, mem_block_tmp);

	return;

tx_disable:
	stream->stop_transfer(dev);
}

static int i2s_esp32_initialize(const struct device *dev)
{
	const struct i2s_esp32_cfg *dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	const struct device *clk_dev = dev_cfg->clock_dev;
	int err;

	if (dev_data->tx.dma_dev && !device_is_ready(dev_data->tx.dma_dev)) {
		LOG_ERR("%s device not ready", dev_data->tx.dma_dev->name);
		return -ENODEV;
	}

	if (dev_data->rx.dma_dev && !device_is_ready(dev_data->rx.dma_dev)) {
		LOG_ERR("%s device not ready", dev_data->rx.dma_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(clk_dev, dev_cfg->clock_subsys);
	if (err != 0) {
		LOG_ERR("Clock control enabling failed: %d", err);
		return -EIO;
	}

	if (dev_data->tx.dma_dev) {
		err = k_msgq_alloc_init(&dev_data->tx.queue, sizeof(struct queue_item),
					CONFIG_I2S_ESP32_TX_BLOCK_COUNT);
		if (err < 0) {
			return err;
		}
	}

	if (dev_data->rx.dma_dev) {
		err = k_msgq_alloc_init(&dev_data->rx.queue, sizeof(struct queue_item),
					CONFIG_I2S_ESP32_RX_BLOCK_COUNT);
		if (err < 0) {
			return err;
		}
	}

	i2s_ll_enable_clock(dev_cfg->hal_cxt.dev);

	LOG_INF("%s initialized", dev->name);

	return 0;
}

static int i2s_esp32_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	const struct i2s_esp32_cfg *const dev_cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream;
	uint8_t data_format;

	switch (dir) {
	case I2S_DIR_RX:
		stream = &dev_data->rx;
		if (stream->dma_dev == NULL) {
			LOG_ERR("RX DMA controller not available");
			return -EINVAL;
		}
		break;
	case I2S_DIR_TX:
		stream = &dev_data->tx;
		if (stream->dma_dev == NULL) {
			LOG_ERR("TX DMA controller not available");
			return -EINVAL;
		}
		break;
	case I2S_DIR_BOTH:
		LOG_ERR("I2S_DIR_BOTH is not supported");
		return -ENOSYS;
	default:
		LOG_ERR("Invalid direction");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		LOG_ERR("Invalid state: %d", (int)stream->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		stream->queue_drop(stream);
		memset(&stream->i2s_cfg, 0, sizeof(struct i2s_config));
		stream->is_slave = false;
		stream->state = I2S_STATE_NOT_READY;
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
		stream->is_slave = true;
	} else if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) == 0 &&
		   (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) == 0) {
		stream->is_slave = false;
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
		slot_cfg.std.left_align = true;
	} else {
		slot_cfg.std.ws_pol = i2s_cfg->format & I2S_FMT_FRAME_CLK_INV ? false : true;
		slot_cfg.std.bit_shift = false;
		if (data_format == I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) {
			slot_cfg.std.left_align = true;
		} else if (data_format == I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
			slot_cfg.std.left_align = false;
		} else {
			LOG_ERR("Invalid data format: %u", (unsigned int)data_format);
		}
	}
	slot_cfg.std.ws_width = slot_cfg.slot_bit_width;
	slot_cfg.std.slot_mask = I2S_STD_SLOT_BOTH;
	slot_cfg.std.big_endian = false;
	slot_cfg.std.bit_order_lsb = i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB ? true : false;

	int err;
	i2s_hal_clock_info_t i2s_hal_clock_info;
	i2s_hal_context_t *hal_cxt = (i2s_hal_context_t *)&dev_cfg->hal_cxt;

	err = i2s_esp32_calculate_clock(i2s_cfg, slot_cfg.slot_bit_width, &i2s_hal_clock_info);
	if (err != ESP_OK) {
		return -EINVAL;
	}

	if (dir == I2S_DIR_TX) {
		if (dev_data->rx.state != I2S_STATE_NOT_READY) {
			if (stream->is_slave && !dev_data->rx.is_slave) { /*full duplex*/
				i2s_ll_share_bck_ws(hal_cxt->dev, true);
			} else {
				i2s_ll_share_bck_ws(hal_cxt->dev, false);
			}
		} else {
			i2s_ll_share_bck_ws(hal_cxt->dev, false);
		}

		i2s_hal_std_set_tx_slot(hal_cxt, stream->is_slave, &slot_cfg);

		i2s_hal_set_tx_clock(hal_cxt, &i2s_hal_clock_info, I2S_ESP32_CLK_SRC);

		err = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			LOG_ERR("Pins setup failed: %d", err);
			return -EIO;
		}

		if (dev_data->tx.state != I2S_STATE_NOT_READY) {
			if (stream->is_slave && dev_data->rx.is_slave) {
				i2s_ll_mclk_bind_to_tx_clk(hal_cxt->dev);
			}
		}

		i2s_hal_std_enable_tx_channel(hal_cxt);
	} else if (dir == I2S_DIR_RX) {
		if (dev_data->tx.state != I2S_STATE_NOT_READY) {
			if (stream->is_slave && !dev_data->tx.is_slave) { /*full duplex*/
				i2s_ll_share_bck_ws(hal_cxt->dev, true);
			} else {
				i2s_ll_share_bck_ws(hal_cxt->dev, false);
			}
		} else {
			i2s_ll_share_bck_ws(hal_cxt->dev, false);
		}

		i2s_hal_std_set_rx_slot(hal_cxt, stream->is_slave, &slot_cfg);

		i2s_hal_set_rx_clock(hal_cxt, &i2s_hal_clock_info, I2S_ESP32_CLK_SRC);

		err = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			LOG_ERR("Pins setup failed: %d", err);
			return -EIO;
		}

		if (dev_data->tx.state != I2S_STATE_NOT_READY) {
			if (!stream->is_slave && !dev_data->tx.is_slave) {
				i2s_ll_mclk_bind_to_rx_clk(hal_cxt->dev);
			}
		}

		i2s_hal_std_enable_rx_channel(hal_cxt);
	}
	memcpy(&stream->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->state = I2S_STATE_READY;

	return 0;
}

static const struct i2s_config *i2s_esp32_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_esp32_data *dev_data = dev->data;
	struct i2s_esp32_stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else {
		stream = &dev_data->tx;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->i2s_cfg;
}

static int i2s_esp32_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	struct i2s_esp32_stream *stream;
	struct dma_status dma_channel_status;
	unsigned int key;
	int err;

	switch (dir) {
	case I2S_DIR_RX:
		stream = &dev_data->rx;
		break;
	case I2S_DIR_TX:
		stream = &dev_data->tx;
		break;
	case I2S_DIR_BOTH:
		LOG_ERR("Unsupported direction: %d", (int)dir);
		return -ENOSYS;
	default:
		LOG_ERR("Invalid direction: %d", (int)dir);
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START - Invalid state: %d", (int)stream->state);
			return -EIO;
		}

		err = stream->start_transfer(dev);
		if (err < 0) {
			LOG_ERR("START - Transfer start failed: %d", err);
			return -EIO;
		}
		stream->last_block = false;
		stream->state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_STOP:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_ERR("STOP - Invalid state: %d", (int)stream->state);
			return -EIO;
		}

		err = dma_get_status(stream->dma_dev, stream->dma_channel, &dma_channel_status);
		if (err < 0) {
			irq_unlock(key);
			LOG_ERR("Unable to get DMA channel[%d] status: %d",
				(int)stream->dma_channel, err);
			return -EIO;
		}

		if (dma_channel_status.busy) {
			stream->stop_without_draining = true;
			stream->state = I2S_STATE_STOPPING;
		} else {
			stream->stop_transfer(dev);
			stream->last_block = true;
			stream->state = I2S_STATE_READY;
		}

		irq_unlock(key);
		break;

	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_ERR("DRAIN - Invalid state: %d", (int)stream->state);
			return -EIO;
		}

		err = dma_get_status(stream->dma_dev, stream->dma_channel, &dma_channel_status);
		if (err < 0) {
			irq_unlock(key);
			LOG_ERR("Unable to get DMA channel[%d] status: %d",
				(int)stream->dma_channel, err);
			return -EIO;
		}

		if (dir == I2S_DIR_TX) {
			if (k_msgq_num_used_get(&stream->queue) > 0 || dma_channel_status.busy) {
				stream->stop_without_draining = false;
				stream->state = I2S_STATE_STOPPING;
			} else {
				stream->stop_transfer(dev);
				stream->state = I2S_STATE_READY;
			}
		} else if (dir == I2S_DIR_RX) {
			if (dma_channel_status.busy) {
				stream->stop_without_draining = true;
				stream->state = I2S_STATE_STOPPING;
			} else {
				stream->stop_transfer(dev);
				stream->last_block = true;
				stream->state = I2S_STATE_READY;
			}
		} else {
			irq_unlock(key);
			LOG_ERR("Invalid direction: %d", (int)dir);
			return -EINVAL;
		}

		irq_unlock(key);
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - invalid state: %d", (int)stream->state);
			return -EIO;
		}
		stream->stop_transfer(dev);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE - invalid state: %d", (int)stream->state);
			return -EIO;
		}
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("Unsupported trigger command: %d", (int)cmd);
		return -EINVAL;
	}

	return 0;
}

static int i2s_esp32_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	struct queue_item item;
	int err;

	if (dev_data->rx.state == I2S_STATE_NOT_READY) {
		LOG_ERR("RX invalid state: %d", (int)dev_data->rx.state);
		return -EIO;
	} else if (dev_data->rx.state == I2S_STATE_ERROR &&
		   k_msgq_num_used_get(&dev_data->rx.queue) == 0) {
		LOG_ERR("RX queue empty");
		return -EIO;
	}

	err = k_msgq_get(&dev_data->rx.queue, &item, K_MSEC(dev_data->rx.i2s_cfg.timeout));
	if (err < 0) {
		LOG_ERR("RX queue empty");
		return err;
	}

	*mem_block = item.buffer;
	*size = item.size;

	return 0;
}

static int i2s_esp32_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	int err;

	if (dev_data->tx.state != I2S_STATE_RUNNING && dev_data->tx.state != I2S_STATE_READY) {
		LOG_ERR("TX Invalid state: %d", (int)dev_data->tx.state);
		return -EIO;
	}

	if (size > dev_data->tx.i2s_cfg.block_size) {
		LOG_ERR("Max write size is: %u", (unsigned int)dev_data->tx.i2s_cfg.block_size);
		return -EINVAL;
	}

	struct queue_item item = {.buffer = mem_block, .size = size};

	err = k_msgq_put(&dev_data->tx.queue, &item, K_MSEC(dev_data->tx.i2s_cfg.timeout));
	if (err < 0) {
		LOG_ERR("TX queue full");
		return err;
	}

	return 0;
}

static const struct i2s_driver_api i2s_esp32_driver_api = {
	.configure = i2s_esp32_configure,
	.config_get = i2s_esp32_config_get,
	.trigger = i2s_esp32_trigger,
	.read = i2s_esp32_read,
	.write = i2s_esp32_write,
};

#define I2S_ESP32_DMA_CHANNEL_INIT(index, dir)                                                     \
	.dir = {.state = I2S_STATE_NOT_READY,                                                      \
		.is_slave = false,                                                                 \
		.dma_dev = UTIL_AND(DT_INST_DMAS_HAS_NAME(index, dir),                             \
				    DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir))),         \
		.dma_channel = UTIL_AND(DT_INST_DMAS_HAS_NAME(index, dir),                         \
					DT_INST_DMAS_CELL_BY_NAME(index, dir, channel)),           \
		.mem_block = NULL,                                                                 \
		.mem_block_len = 0,                                                                \
		.start_transfer = i2s_esp32_##dir##_start_transfer,                                \
		.stop_transfer = i2s_esp32_##dir##_stop_transfer,                                  \
		.queue_drop = i2s_esp32_queue_drop,                                                \
		.last_block = false,                                                               \
		.stop_without_draining = false}

#define I2S_ESP32_INIT(index)                                                                      \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct i2s_esp32_cfg i2s_esp32_config_##index = {                             \
		.unit = DT_PROP(DT_DRV_INST(index), unit),                                         \
		.hal_cxt = {.dev = (i2s_dev_t *)DT_INST_REG_ADDR(index)},                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(index, offset)};       \
                                                                                                   \
	static struct i2s_esp32_data i2s_esp32_data_##index = {                                    \
		I2S_ESP32_DMA_CHANNEL_INIT(index, rx), I2S_ESP32_DMA_CHANNEL_INIT(index, tx)};     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_esp32_initialize, NULL, &i2s_esp32_data_##index,         \
			      &i2s_esp32_config_##index, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,    \
			      &i2s_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_ESP32_INIT)
