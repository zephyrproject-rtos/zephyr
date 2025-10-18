/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_i2s

#include <string.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include "clock_update.h"

#include "rsi_rom_clks.h"
#include "rsi_rom_ulpss_clk.h"
#include "rsi_power_save.h"
#include "rsi_pll.h"
#include "rsi_ulpss_clk.h"
#include "clock_update.h"

#define DMA_MAX_TRANSFER_COUNT 1024
#define I2S_SIWX91X_UNSUPPORTED_OPTIONS                                                            \
	(I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_LOOPBACK | I2S_OPT_PINGPONG |   \
	 I2S_OPT_BIT_CLK_GATED)

struct i2s_siwx91x_config {
	I2S0_Type *reg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys_peripheral;
	clock_control_subsys_t clock_subsys_static;
	const struct pinctrl_dev_config *pcfg;
	uint8_t channel_group;
};

struct i2s_siwx91x_queue_item {
	void *mem_block;
	size_t size;
};

struct i2s_siwx91x_ring_buffer {
	struct i2s_siwx91x_queue_item *buf;
	uint16_t len;
	uint16_t head;
	uint16_t tail;
};

struct i2s_siwx91x_stream {
	int32_t state;
	struct k_sem sem;
	const struct device *dma_dev;
	uint32_t dma_channel;
	bool last_block;
	struct i2s_config cfg;
	struct i2s_siwx91x_ring_buffer mem_block_queue;
	void *mem_block;
	bool reload_en;
	struct dma_block_config dma_descriptors[CONFIG_I2S_SILABS_SIWX91X_DMA_MAX_BLOCKS];
	int (*stream_start)(struct i2s_siwx91x_stream *stream, const struct device *dev);
	void (*queue_drop)(struct i2s_siwx91x_stream *stream);
};

struct i2s_siwx91x_data {
	struct i2s_siwx91x_stream rx;
	struct i2s_siwx91x_stream tx;
	uint8_t current_resolution;
};

static void i2s_siwx91x_dma_rx_callback(const struct device *dma_dev, void *user_data,
					uint32_t channel, int status);
static void i2s_siwx91x_dma_tx_callback(const struct device *dma_dev, void *user_data,
					uint32_t channel, int status);

static bool i2s_siwx91x_validate_word_size(uint8_t word_size)
{
	switch (word_size) {
	case 16:
	case 24:
	case 32:
		return true;
	default:
		return false;
	}
}

static bool i2s_siwx91x_validate_frequency(uint32_t sampling_freq)
{
	switch (sampling_freq) {
	case 8000:
	case 11025:
	case 16000:
	case 22050:
	case 24000:
	case 32000:
	case 44100:
	case 48000:
	case 88200:
	case 96000:
	case 192000:
		return true;
	default:
		return false;
	}
}

static int i2s_siwx91x_convert_to_resolution(uint8_t word_size)
{
	switch (word_size) {
	case 16:
		return 2;
	case 24:
		return 4;
	case 32:
		return 5;
	default:
		return -EINVAL;
	}
}

static int i2s_siwx91x_queue_put(struct i2s_siwx91x_ring_buffer *rb, void *mem_block, size_t size)
{
	uint16_t head_next;
	unsigned int key;

	key = irq_lock();

	head_next = rb->head;
	head_next = (head_next + 1) % rb->len;

	if (head_next == rb->tail) {
		/* Ring buffer is full */
		irq_unlock(key);
		return -ENOMEM;
	}

	rb->buf[rb->head].mem_block = mem_block;
	rb->buf[rb->head].size = size;
	rb->head = head_next;

	irq_unlock(key);

	return 0;
}

static int i2s_siwx91x_queue_get(struct i2s_siwx91x_ring_buffer *rb, void **mem_block, size_t *size)
{
	unsigned int key;

	key = irq_lock();

	if (rb->tail == rb->head) {
		/* Ring buffer is empty */
		irq_unlock(key);
		return -ENOMEM;
	}

	*mem_block = rb->buf[rb->tail].mem_block;
	*size = rb->buf[rb->tail].size;
	rb->tail = (rb->tail + 1) % rb->len;

	irq_unlock(key);

	return 0;
}

static int i2s_siwx91x_dma_config(const struct device *dev, struct i2s_siwx91x_stream *stream,
				  uint32_t block_count, bool is_tx, uint8_t xfer_size)
{
	struct dma_config cfg = {
		.channel_direction = is_tx ? MEMORY_TO_PERIPHERAL : PERIPHERAL_TO_MEMORY,
		.complete_callback_en = 0,
		.source_data_size = xfer_size,
		.dest_data_size = xfer_size,
		.source_burst_length = xfer_size,
		.dest_burst_length = xfer_size,
		.block_count = block_count,
		.head_block = stream->dma_descriptors,
		.dma_callback = is_tx ? &i2s_siwx91x_dma_tx_callback : &i2s_siwx91x_dma_rx_callback,
		.user_data = (void *)dev,
	};

	return dma_config(stream->dma_dev, stream->dma_channel, &cfg);
}

struct dma_block_config *i2s_siwx91x_fill_data_desc(const struct i2s_siwx91x_config *cfg,
						    struct dma_block_config *desc, void *buffer,
						    uint32_t size, bool is_tx, uint8_t xfer_size)
{
	uint32_t max_chunk_size = DMA_MAX_TRANSFER_COUNT * xfer_size;
	uint8_t *current_buffer = buffer;
	int num_descriptors = (size + max_chunk_size - 1) / max_chunk_size;
	int i;

	if (num_descriptors > CONFIG_I2S_SILABS_SIWX91X_DMA_MAX_BLOCKS) {
		return NULL;
	}

	for (i = 0; i < num_descriptors; i++) {
		if (is_tx) {
			desc[i].source_address = (uint32_t)current_buffer;
			desc[i].dest_address = (uint32_t)&cfg->reg->I2S_TXDMA;
			desc[i].dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			desc[i].source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			desc[i].dest_address = (uint32_t)current_buffer;
			desc[i].source_address = (uint32_t)&(cfg->reg->I2S_RXDMA);
			desc[i].source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			desc[i].dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		}

		desc[i].block_size = MIN(size, max_chunk_size);
		size -= max_chunk_size;
		current_buffer += max_chunk_size;
	}

	desc[i - 1].next_block = NULL;

	return &desc[i - 1];
}

static void i2s_siwx91x_reset_desc(struct i2s_siwx91x_stream *stream)
{
	int i;

	memset(stream->dma_descriptors, 0, sizeof(stream->dma_descriptors));

	for (i = 0; i < ARRAY_SIZE(stream->dma_descriptors) - 1; i++) {
		stream->dma_descriptors[i].next_block = &stream->dma_descriptors[i + 1];
	}
}

static int i2s_siwx91x_prepare_dma_channel(const struct device *i2s_dev, void *buffer,
					   uint32_t blk_size, uint32_t dma_channel, bool is_tx)
{
	const struct i2s_siwx91x_config *cfg = i2s_dev->config;
	struct i2s_siwx91x_data *data = i2s_dev->data;
	struct i2s_siwx91x_stream *stream;
	struct dma_block_config *desc;
	uint8_t xfer_size;
	int ret;

	if (is_tx) {
		stream = &data->tx;
	} else {
		stream = &data->rx;
	}

	if (stream->cfg.word_size != 24) {
		xfer_size = stream->cfg.word_size / 8;
	} else {
		/* 24-bit resolution also uses 32-bit (4 bytes) data size */
		xfer_size = 4;
	}

	i2s_siwx91x_reset_desc(stream);

	desc = i2s_siwx91x_fill_data_desc(cfg, stream->dma_descriptors, buffer, blk_size, is_tx,
					  xfer_size);
	if (!desc) {
		return -ENOMEM;
	}

	ret = i2s_siwx91x_dma_config(i2s_dev, stream,
				     ARRAY_INDEX(stream->dma_descriptors, desc) + 1,
				     is_tx, xfer_size);
	if (ret) {
		return ret;
	}

	if (ARRAY_INDEX(stream->dma_descriptors, desc) == 0) {
		/* Transfer size <= 1024*xfer_size */
		stream->reload_en = true;
	}

	return ret;
}

static int i2s_siwx91x_tx_stream_start(struct i2s_siwx91x_stream *stream, const struct device *dev)
{
	size_t mem_block_size;
	int ret;

	ret = i2s_siwx91x_queue_get(&stream->mem_block_queue, &stream->mem_block, &mem_block_size);
	if (ret < 0) {
		return ret;
	}

	k_sem_give(&stream->sem);

	ret = i2s_siwx91x_prepare_dma_channel(dev, stream->mem_block, mem_block_size,
					      stream->dma_channel, true);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(stream->dma_dev, stream->dma_channel);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int i2s_siwx91x_rx_stream_start(struct i2s_siwx91x_stream *stream, const struct device *dev)
{
	int ret;

	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	ret = i2s_siwx91x_prepare_dma_channel(dev, stream->mem_block, stream->cfg.block_size,
					      stream->dma_channel, false);

	if (ret < 0) {
		return ret;
	}

	ret = dma_start(stream->dma_dev, stream->dma_channel);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void i2s_siwx91x_stream_disable(struct i2s_siwx91x_stream *stream,
				       const struct device *dma_dev)
{
	dma_stop(dma_dev, stream->dma_channel);

	dma_release_channel(dma_dev, stream->dma_channel);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}
}

static void i2s_siwx91x_rx_queue_drop(struct i2s_siwx91x_stream *stream)
{
	size_t size;
	void *mem_block;

	while (i2s_siwx91x_queue_get(&stream->mem_block_queue, &mem_block, &size) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, mem_block);
	}

	k_sem_reset(&stream->sem);
}

static void i2s_siwx91x_tx_queue_drop(struct i2s_siwx91x_stream *stream)
{
	size_t size;
	void *mem_block;
	uint32_t n = 0U;

	while (i2s_siwx91x_queue_get(&stream->mem_block_queue, &mem_block, &size) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, mem_block);
		n++;
	}

	for (; n > 0; n--) {
		k_sem_give(&stream->sem);
	}
}

static void i2s_siwx91x_dma_rx_callback(const struct device *dma_dev, void *user_data,
					uint32_t channel, int status)
{
	const struct device *i2s_dev = user_data;
	const struct i2s_siwx91x_config *cfg = i2s_dev->config;
	struct i2s_siwx91x_data *data = i2s_dev->data;
	struct i2s_siwx91x_stream *stream = &data->rx;
	uint8_t data_size; /* data size in bytes */
	int ret;

	__ASSERT_NO_MSG(stream->mem_block != NULL);

	if (stream->state == I2S_STATE_ERROR) {
		goto rx_disable;
	}

	ret = i2s_siwx91x_queue_put(&stream->mem_block_queue, stream->mem_block,
				    stream->cfg.block_size);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	stream->mem_block = NULL;
	k_sem_give(&stream->sem);

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		goto rx_disable;
	}

	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	if (stream->cfg.word_size == 24) {
		/* 24-bit resolution also uses 32-bit (4 bytes) data size */
		data_size = 4;
	} else {
		data_size = stream->cfg.word_size / 8;
	}

	if ((stream->cfg.block_size <= DMA_MAX_TRANSFER_COUNT * data_size) && stream->reload_en) {
		ret = dma_reload(dma_dev, stream->dma_channel, (uint32_t)&cfg->reg->I2S_RXDMA,
				 (uint32_t)stream->mem_block, stream->cfg.block_size);
	} else {
		ret = i2s_siwx91x_prepare_dma_channel(i2s_dev, stream->mem_block,
						      stream->cfg.block_size, stream->dma_channel,
						      false);
		stream->reload_en = false;
	}

	if (ret < 0) {
		goto rx_disable;
	}

	ret = dma_start(dma_dev, stream->dma_channel);
	if (ret < 0) {
		goto rx_disable;
	}

	return;

rx_disable:
	i2s_siwx91x_stream_disable(stream, dma_dev);
	if (stream->state == I2S_STATE_READY && stream->last_block) {
		pm_device_runtime_put_async(i2s_dev, K_NO_WAIT);
	}
}

static void i2s_siwx91x_dma_tx_callback(const struct device *dma_dev, void *user_data,
					uint32_t channel, int status)
{
	const struct device *i2s_dev = user_data;
	const struct i2s_siwx91x_config *cfg = i2s_dev->config;
	struct i2s_siwx91x_data *data = i2s_dev->data;
	struct i2s_siwx91x_stream *stream = &data->tx;
	size_t mem_block_size;
	uint8_t data_size; /* data size in bytes */
	int ret;

	__ASSERT_NO_MSG(stream->mem_block != NULL);

	k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
	stream->mem_block = NULL;

	if (stream->state == I2S_STATE_ERROR) {
		goto tx_disable;
	}

	if (stream->last_block) {
		stream->state = I2S_STATE_READY;
		goto tx_disable;
	}

	ret = i2s_siwx91x_queue_get(&stream->mem_block_queue, &stream->mem_block, &mem_block_size);
	if (ret < 0) {
		if (stream->state == I2S_STATE_STOPPING) {
			stream->state = I2S_STATE_READY;
		} else {
			stream->state = I2S_STATE_ERROR;
		}
		goto tx_disable;
	}

	k_sem_give(&stream->sem);

	if (stream->cfg.word_size == 24) {
		data_size = 4;
	} else {
		data_size = stream->cfg.word_size / 8;
	}

	if ((mem_block_size <= DMA_MAX_TRANSFER_COUNT * data_size) && stream->reload_en) {
		ret = dma_reload(dma_dev, stream->dma_channel, (uint32_t)stream->mem_block,
				 (uint32_t)&cfg->reg->I2S_TXDMA, mem_block_size);
	} else {
		ret = i2s_siwx91x_prepare_dma_channel(i2s_dev, stream->mem_block, mem_block_size,
						      stream->dma_channel, true);
		stream->reload_en = false;
	}
	if (ret < 0) {
		goto tx_disable;
	}

	ret = dma_start(dma_dev, stream->dma_channel);
	if (ret < 0) {
		goto tx_disable;
	}

	return;

tx_disable:
	i2s_siwx91x_stream_disable(stream, dma_dev);
	if (stream->state == I2S_STATE_READY && stream->last_block) {
		pm_device_runtime_put_async(i2s_dev, K_NO_WAIT);
	}
}

static int i2s_siwx91x_param_config(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_siwx91x_config *cfg = dev->config;
	struct i2s_siwx91x_data *data = dev->data;
	struct i2s_siwx91x_stream *stream;
	uint32_t bit_freq;
	int resolution;
	int ret;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else {
		stream = &data->tx;
	}

	resolution = i2s_siwx91x_convert_to_resolution(stream->cfg.word_size);
	if (resolution < 0) {
		return -EINVAL;
	}

	if (resolution != data->current_resolution) {
		ret = clock_control_off(cfg->clock_dev, cfg->clock_subsys_static);
		if (ret) {
			return ret;
		}

		/* Configure primary mode and bit clock frequency */
		bit_freq = 2 * stream->cfg.frame_clk_freq * stream->cfg.word_size;

		ret = clock_control_set_rate(cfg->clock_dev, cfg->clock_subsys_peripheral,
					     &bit_freq);
		if (ret) {
			return ret;
		}

		cfg->reg->I2S_CCR_b.WSS = (resolution - 1) / 2;
		cfg->reg->I2S_CCR_b.SCLKG = resolution;
		data->current_resolution = resolution;
	}

	if (dir == I2S_DIR_RX) {
		cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_RCR_b.WLEN = resolution;
		cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_RFCR_b.RXCHDT = 1;
	} else {
		cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_TCR_b.WLEN = resolution;
		cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_TXFCR_b.TXCHET = 0;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys_static);
	if (ret) {
		return ret;
	}

	return 0;
}

static int i2s_siwx91x_dma_channel_alloc(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_siwx91x_data *data = dev->data;
	struct i2s_siwx91x_stream *stream;
	int channel_filter;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else {
		stream = &data->tx;
	}

	dma_release_channel(stream->dma_dev, stream->dma_channel);

	channel_filter = stream->dma_channel;
	stream->dma_channel = dma_request_channel(stream->dma_dev, &channel_filter);
	if (stream->dma_channel != channel_filter) {
		stream->dma_channel = channel_filter;
		return -EAGAIN;
	}

	return 0;
}

static void i2s_siwx91x_start_tx(const struct device *dev)
{
	const struct i2s_siwx91x_config *cfg = dev->config;

	cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_IMR &= ~F_TXFEM;
	cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_TER_b.TXCHEN = 1;
	cfg->reg->CHANNEL_CONFIG[1 - cfg->channel_group].I2S_TER_b.TXCHEN = 0;
}

static void i2s_siwx91x_start_rx(const struct device *dev)
{
	const struct i2s_siwx91x_config *cfg = dev->config;

	cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_RER_b.RXCHEN = 1;
	cfg->reg->CHANNEL_CONFIG[cfg->channel_group].I2S_IMR &= ~F_RXDAM;
	cfg->reg->CHANNEL_CONFIG[1 - cfg->channel_group].I2S_RER_b.RXCHEN = 0;
}

static int i2s_siwx91x_configure(const struct device *dev, enum i2s_dir dir,
				 const struct i2s_config *i2s_cfg)
{
	struct i2s_siwx91x_data *data = dev->data;
	struct i2s_siwx91x_stream *stream;

	if (dir != I2S_DIR_RX && dir != I2S_DIR_TX) {
		return -ENOTSUP;
	}

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else {
		stream = &data->tx;
	}

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		return -EINVAL;
	}

	if (!i2s_siwx91x_validate_word_size(i2s_cfg->word_size)) {
		return -EINVAL;
	}

	if (i2s_cfg->channels != 2) {
		return -EINVAL;
	}

	if ((i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) != I2S_FMT_DATA_FORMAT_I2S) {
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_SIWX91X_UNSUPPORTED_OPTIONS) {
		return -ENOTSUP;
	}

	if (!i2s_siwx91x_validate_frequency(i2s_cfg->frame_clk_freq)) {
		return -EINVAL;
	}

	if (i2s_cfg->word_size == 24) {
		if (i2s_cfg->block_size % 4 != 0) {
			return -EINVAL;
		}
	} else {
		if (i2s_cfg->block_size % 2 != 0) {
			return -EINVAL;
		}
	}

	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->state = I2S_STATE_READY;

	return 0;
}

static const struct i2s_config *i2s_siwx91x_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_siwx91x_data *data = dev->data;
	struct i2s_siwx91x_stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
	} else {
		return NULL;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->cfg;
}

static int i2s_siwx91x_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_siwx91x_data *data = dev->data;
	int ret;

	if (data->tx.state != I2S_STATE_RUNNING && data->tx.state != I2S_STATE_READY) {
		return -EIO;
	}

	ret = k_sem_take(&data->tx.sem, SYS_TIMEOUT_MS(data->tx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	/* Add data to the end of the TX queue */
	i2s_siwx91x_queue_put(&data->tx.mem_block_queue, mem_block, size);

	return 0;
}

static int i2s_siwx91x_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_siwx91x_data *data = dev->data;
	int ret;

	if (data->rx.state == I2S_STATE_NOT_READY) {
		return -EIO;
	}

	if (data->rx.state != I2S_STATE_ERROR) {
		ret = k_sem_take(&data->rx.sem, SYS_TIMEOUT_MS(data->rx.cfg.timeout));
		if (ret < 0) {
			return ret;
		}
	}

	/* Get data from the beginning of RX queue */
	ret = i2s_siwx91x_queue_get(&data->rx.mem_block_queue, mem_block, size);
	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

static int i2s_siwx91x_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	const struct i2s_siwx91x_config *cfg = dev->config;
	struct i2s_siwx91x_data *data = dev->data;
	struct i2s_siwx91x_stream *stream;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
	} else {
		return -ENOTSUP;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		ret = pm_device_runtime_get(dev);
		if (ret < 0) {
			return ret;
		}

		if (stream->state != I2S_STATE_READY) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			return -EIO;
		}

		__ASSERT_NO_MSG(stream->mem_block == NULL);

		ret = i2s_siwx91x_param_config(dev, dir);
		if (ret < 0) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			return ret;
		}

		ret = i2s_siwx91x_dma_channel_alloc(dev, dir);
		if (ret < 0) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			return ret;
		}

		if (dir == I2S_DIR_RX) {
			i2s_siwx91x_start_rx(dev);
		} else {
			i2s_siwx91x_start_tx(dev);
		}

		ret = stream->stream_start(stream, dev);
		if (ret < 0) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			return ret;
		}

		cfg->reg->I2S_CER_b.CLKEN = ENABLE;
		if (dir == I2S_DIR_TX) {
			cfg->reg->I2S_ITER_b.TXEN = ENABLE;
		} else {
			cfg->reg->I2S_IRER_b.RXEN = ENABLE;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;
		break;

	case I2S_TRIGGER_STOP:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			return -EIO;
		}

		stream->state = I2S_STATE_STOPPING;
		irq_unlock(key);
		stream->last_block = true;
		break;

	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			return -EIO;
		}

		stream->state = I2S_STATE_STOPPING;
		irq_unlock(key);
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			return -EIO;
		}

		i2s_siwx91x_stream_disable(stream, stream->dma_dev);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		pm_device_runtime_put_async(dev, K_NO_WAIT);
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			return -EIO;
		}

		stream->state = I2S_STATE_READY;
		stream->queue_drop(stream);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int i2s_siwx91x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2s_siwx91x_config *cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys_peripheral);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}

		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}

		cfg->reg->I2S_IER_b.IEN = 1;
		cfg->reg->I2S_IRER_b.RXEN = 0;
		cfg->reg->I2S_ITER_b.TXEN = 0;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = clock_control_off(cfg->clock_dev, cfg->clock_subsys_peripheral);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int i2s_siwx91x_init(const struct device *dev)
{
	struct i2s_siwx91x_data *data = dev->data;

	k_sem_init(&data->rx.sem, 0, CONFIG_I2S_SILABS_SIWX91X_RX_BLOCK_COUNT);
	k_sem_init(&data->tx.sem, CONFIG_I2S_SILABS_SIWX91X_TX_BLOCK_COUNT,
		   CONFIG_I2S_SILABS_SIWX91X_TX_BLOCK_COUNT);

	return pm_device_driver_init(dev, i2s_siwx91x_pm_action);
}

static DEVICE_API(i2s, i2s_siwx91x_driver_api) = {
	.configure = i2s_siwx91x_configure,
	.config_get = i2s_siwx91x_config_get,
	.read = i2s_siwx91x_read,
	.write = i2s_siwx91x_write,
	.trigger = i2s_siwx91x_trigger,
};

#define SIWX91X_I2S_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	struct i2s_siwx91x_queue_item                                                              \
		rx_ring_buf_##inst[CONFIG_I2S_SILABS_SIWX91X_RX_BLOCK_COUNT + 1];                  \
	struct i2s_siwx91x_queue_item                                                              \
		tx_ring_buf_##inst[CONFIG_I2S_SILABS_SIWX91X_TX_BLOCK_COUNT + 1];                  \
                                                                                                   \
	BUILD_ASSERT((DT_INST_PROP(inst, silabs_channel_group) <                                   \
		      DT_INST_PROP(inst, silabs_max_channel_count)),                               \
		     "Invalid channel group!");                                                    \
                                                                                                   \
	static struct i2s_siwx91x_data i2s_data_##inst = {                                         \
		.rx.dma_channel = DT_INST_DMAS_CELL_BY_NAME(inst, rx, channel),                    \
		.rx.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, rx)),                  \
		.rx.mem_block_queue.buf = rx_ring_buf_##inst,                                      \
		.rx.mem_block_queue.len = ARRAY_SIZE(rx_ring_buf_##inst),                          \
		.rx.stream_start = i2s_siwx91x_rx_stream_start,                                    \
		.rx.queue_drop = i2s_siwx91x_rx_queue_drop,                                        \
		.tx.dma_channel = DT_INST_DMAS_CELL_BY_NAME(0, tx, channel),                       \
		.tx.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, tx)),                  \
		.tx.mem_block_queue.buf = tx_ring_buf_##inst,                                      \
		.tx.mem_block_queue.len = ARRAY_SIZE(tx_ring_buf_##inst),                          \
		.tx.stream_start = i2s_siwx91x_tx_stream_start,                                    \
		.tx.queue_drop = i2s_siwx91x_tx_queue_drop,                                        \
	};                                                                                         \
	static const struct i2s_siwx91x_config i2s_config_##inst = {                               \
		.reg = (I2S0_Type *)DT_INST_REG_ADDR(inst),                                        \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys_peripheral =                                                         \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, clkid),        \
		.clock_subsys_static =                                                             \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 1, clkid),        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.channel_group = DT_INST_PROP(inst, silabs_channel_group),                         \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, i2s_siwx91x_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(inst, &i2s_siwx91x_init, PM_DEVICE_DT_INST_GET(inst),                \
			      &i2s_data_##inst, &i2s_config_##inst, POST_KERNEL,                   \
			      CONFIG_I2S_INIT_PRIORITY, &i2s_siwx91x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_I2S_INIT)
