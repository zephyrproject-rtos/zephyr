/*
 * Copyright (c) 2025 Croxel Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_i2s

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <wrap_max32_i2s.h>

LOG_MODULE_REGISTER(max32_i2s, CONFIG_I2S_LOG_LEVEL);

struct i2s_mem_block {
	void *block;
	size_t size;
};

struct i2s_max32_stream_data {
	volatile enum i2s_state state;
	volatile bool drain;
	struct i2s_mem_block cur_block;
	struct k_msgq *queue;
	struct i2s_config i2s_cfg;
};

struct i2s_max32_stream {
	struct i2s_max32_stream_data *data;
	struct {
		mxc_i2s_regs_t *reg;
		const struct device *dev;
	} i2s;
	struct {
		const struct device *dev;
		uint32_t channel;
		uint32_t slot;
	} dma;
};

struct i2s_max32_cfg {
	const struct i2s_max32_stream tx;
	const struct i2s_max32_stream rx;
	const struct pinctrl_dev_config *pcfg;
	const uint32_t i2s_clk_freq;
};

/*
 * following are the 5 helper apis to handle register configuration while
 * making other part of code readable
 */
static inline int mxc_i2s_init(mxc_i2s_req_t *req)
{
	/*
	 * MXC_I2S_Init internally required at least one data pointer set
	 * but we don't have any data to send or receive at this point.
	 * So we set a dummy buffer to satisfy the requirement.
	 * This buffer will not be used in the transaction.
	 */
	uint32_t buf0[1] = {0};

	req->rxData = buf0;
	req->length = ARRAY_SIZE(buf0);

	return MXC_I2S_Init(req);
}

static inline void mxc_i2s_enable_dma_tx(mxc_i2s_regs_t *i2s)
{
	i2s->dmach0 |= MXC_F_I2S_DMACH0_DMA_TX_EN;
	i2s->ctrl0ch0 |= MXC_F_I2S_CTRL0CH0_TX_EN;
}

static inline void mxc_i2s_enable_dma_rx(mxc_i2s_regs_t *i2s)
{
	i2s->dmach0 |= MXC_F_I2S_DMACH0_DMA_RX_EN;
	i2s->ctrl0ch0 |= MXC_F_I2S_CTRL0CH0_RX_EN;
}

static inline void mxc_i2s_set_dma_tx_threshold(mxc_i2s_regs_t *i2s, uint8_t threshold)
{
	i2s->dmach0 &= ~MXC_F_I2S_DMACH0_DMA_TX_THD_VAL;
	i2s->dmach0 |= (threshold << MXC_F_I2S_DMACH0_DMA_TX_THD_VAL_POS);
}

static inline void mxc_i2s_set_dma_rx_threshold(mxc_i2s_regs_t *i2s, uint8_t threshold)
{
	i2s->dmach0 &= ~MXC_F_I2S_DMACH0_DMA_RX_THD_VAL;
	i2s->dmach0 |= (threshold << MXC_F_I2S_DMACH0_DMA_RX_THD_VAL_POS);
}

static void i2s_max32_tx_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				      int status);

static void i2s_max32_rx_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				      int status);

static inline void free_mem_block(const struct i2s_max32_stream *stream)
{
	struct i2s_max32_stream_data *stream_data = stream->data;

	k_mem_slab_free(stream_data->i2s_cfg.mem_slab, stream_data->cur_block.block);
	stream_data->cur_block.block = NULL;
}

static inline void trigger_stream_stop(const struct i2s_max32_stream *stream, bool drain)
{
	struct i2s_max32_stream_data *stream_data = stream->data;

	LOG_DBG("Stopping stream %p, drain: %d", (void *)stream, drain);
	/* signal stopping to handle in the dma callback */
	stream_data->state = I2S_STATE_STOPPING;
	/* use to control drain/drop behaviour */
	stream_data->drain = drain;
}

static inline void clean_stream(const struct i2s_max32_stream *stream)
{
	struct i2s_max32_stream_data *stream_data = stream->data;
	struct i2s_mem_block mem_block;

	/* Clear transient block */
	if (stream->data->cur_block.block != NULL) {
		k_mem_slab_free(stream_data->i2s_cfg.mem_slab, stream_data->cur_block.block);
		stream->data->cur_block.block = NULL;
	}

	/* Clear the pending blocks from the queue */
	while (k_msgq_get(stream_data->queue, &mem_block, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream_data->i2s_cfg.mem_slab, mem_block.block);
	}

	/* mark as ready */
	stream_data->state = I2S_STATE_READY;
}

static int terminate_stream(const struct i2s_max32_stream *stream)
{
	int ret;
	struct i2s_max32_stream_data *stream_data = stream->data;

	if (stream_data->state != I2S_STATE_RUNNING) {
		LOG_ERR("Stream not running, state: %d", (int)stream_data->state);
		return -EIO;
	}
	stream_data->state = I2S_STATE_STOPPING;
	/* stop dma immediately */
	ret = dma_stop(stream->dma.dev, stream->dma.channel);
	if (ret < 0) {
		LOG_ERR("Failed to stop DMA channel[%d]: %d", (int)stream->dma.channel, ret);
		return ret;
	}
	/* Clear the queue */
	clean_stream(stream);
	return 0;
}

static inline int start_stream(const struct i2s_max32_stream *stream, enum i2s_dir dir)
{
	int ret;
	struct i2s_max32_stream_data *stream_data = stream->data;
	struct dma_block_config dma_block;
	/*
	 * For TX destination size and RX source size is always word,
	 * and thus burst length is always 4, refer to 14.6.4 of MAX32655 user manual
	 */
	struct dma_config dma_cfg = {
		.dma_slot = stream->dma.slot,
		.source_data_size = stream_data->i2s_cfg.word_size / 8,
		.source_burst_length = 4,
		.dest_data_size = stream_data->i2s_cfg.word_size / 8,
		.dest_burst_length = 4,
		.block_count = 1,
		.user_data = (void *)stream,
		.head_block = &dma_block,
	};

	if ((dir != I2S_DIR_RX) && (dir != I2S_DIR_TX)) {
		LOG_ERR("Invalid I2S direction: %d", (int)dir);
		return -EINVAL;
	}

	if (stream_data->cur_block.block != NULL) {
		LOG_ERR("Stream already running");
		return -EIO;
	}

	if (dir == I2S_DIR_RX) {
		ret = k_mem_slab_alloc(stream_data->i2s_cfg.mem_slab, &stream_data->cur_block.block,
				       K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Failed to allocate RX mem block: %d", ret);
			return -ENOMEM;
		}

		/* set block size manually */
		stream_data->cur_block.size = stream_data->i2s_cfg.mem_slab->info.block_size;
		/* Configure DMA Block */
		dma_block.block_size = stream_data->i2s_cfg.mem_slab->info.block_size,
		dma_block.source_address = (uint32_t)NULL;
		dma_block.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_block.dest_address = (uint32_t)stream_data->cur_block.block;
		dma_block.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		/* Configure DMA */
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.dma_callback = i2s_max32_rx_dma_callback;
		dma_cfg.channel_priority = 1;
	} else {
		ret = k_msgq_get(stream_data->queue, &stream_data->cur_block, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Failed to get item from TX queue: %d", ret);
			return ret;
		}
		/* Configure DMA Block */
		dma_block.block_size = stream_data->cur_block.size,
		dma_block.source_address = (uint32_t)stream_data->cur_block.block;
		dma_block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_block.dest_address = (uint32_t)NULL;
		dma_block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		/* Configure DMA */
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.dma_callback = i2s_max32_tx_dma_callback;
		dma_cfg.channel_priority = 0;
	}

	/* Configure DMA channel */
	ret = dma_config(stream->dma.dev, stream->dma.channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("DMA config failed with error: %d", ret);
		free_mem_block(stream);
		return ret;
	}

	/* TX/RX FIFO size is 8 word, thus using 4 words to trigger DMA transfer */
	if (dir == I2S_DIR_RX) {
		mxc_i2s_set_dma_rx_threshold(stream->i2s.reg, 4);
		mxc_i2s_enable_dma_rx(stream->i2s.reg);
	} else {
		mxc_i2s_set_dma_tx_threshold(stream->i2s.reg, 4);
		mxc_i2s_enable_dma_tx(stream->i2s.reg);
	}

	/* Start DMA transfer */
	ret = dma_start(stream->dma.dev, stream->dma.channel);
	if (ret < 0) {
		LOG_ERR("DMA start failed with error: %d", ret);
		free_mem_block(stream);
		return ret;
	}

	stream_data->state = I2S_STATE_RUNNING;
	return 0;
}

static inline int restart_stream(const struct i2s_max32_stream *stream, enum i2s_dir dir)
{
	int ret;
	struct i2s_max32_stream_data *stream_data = stream->data;

	if ((stream_data->state != I2S_STATE_RUNNING) &&
	    (stream_data->state != I2S_STATE_STOPPING)) {
		LOG_ERR("Stream not running");
		return -EIO;
	}

	if ((dir != I2S_DIR_TX) && (dir != I2S_DIR_RX)) {
		LOG_ERR("Invalid I2S direction: %d", (int)dir);
		return -ENOTSUP;
	}

	if (stream_data->cur_block.block != NULL) {
		LOG_ERR("Stream already running");
		return -EIO;
	}

	if (dir == I2S_DIR_RX) {
		ret = k_mem_slab_alloc(stream_data->i2s_cfg.mem_slab, &stream_data->cur_block.block,
				       K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Failed to allocate RX mem block: %d", ret);
			return -ENOMEM;
		}
	} else {
		ret = k_msgq_get(stream_data->queue, &stream_data->cur_block, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Failed to get item from TX queue: %d", ret);
			return ret;
		}
	}

	/*
	 * source address in case of RX and destination address in case of TX is ignored,
	 * thus it is safe to use same address irrespective of the direction
	 */
	ret = dma_reload(stream->dma.dev, stream->dma.channel,
			 (uint32_t)stream_data->cur_block.block,
			 (uint32_t)stream_data->cur_block.block, stream_data->i2s_cfg.block_size);
	if (ret < 0) {
		LOG_ERR("Error reloading DMA channel[%d]: %d", (int)stream->dma.channel, ret);
		free_mem_block(stream);
		return ret;
	}

	ret = dma_start(stream->dma.dev, stream->dma.channel);
	if (ret < 0) {
		LOG_ERR("Error starting DMA channel[%d]: %d", (int)stream->dma.channel, ret);
		free_mem_block(stream);
		return ret;
	}

	return 0;
}

static void i2s_max32_tx_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				      int status)
{
	int err;
	const struct i2s_max32_stream *stream = (const struct i2s_max32_stream *)arg;
	struct i2s_max32_stream_data *stream_data = stream->data;

	if (stream_data->cur_block.block == NULL) {
		LOG_ERR("TX DMA callback called with NULL block");
		stream_data->state = I2S_STATE_ERROR;
		return;
	}

	/* free the block we are working with irrespective of success/fail */
	free_mem_block(stream);

	/* check the status of the DMA transfer */
	if (status < 0) {
		LOG_ERR("TX DMA status bad: %d", status);
		stream_data->state = I2S_STATE_ERROR;
		return;
	}

	/*
	 * if a stop was requested without draining,
	 * or the queue has drained completely,
	 * then we can stop the stream
	 */
	if ((stream_data->state == I2S_STATE_STOPPING) &&
	    ((stream_data->drain == false) || (k_msgq_num_used_get(stream_data->queue) == 0))) {
		stream_data->state = I2S_STATE_READY;
		return;
	}

	err = restart_stream(stream, I2S_DIR_TX);
	if (err < 0) {
		LOG_ERR("Failed to restart TX transfer: %d", err);
		stream_data->state = I2S_STATE_ERROR;
	}
}

static void i2s_max32_rx_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				      int status)
{
	int err;
	const struct i2s_max32_stream *stream = (const struct i2s_max32_stream *)arg;
	struct i2s_max32_stream_data *stream_data = stream->data;

	if (stream_data->cur_block.block == NULL) {
		LOG_ERR("RX DMA callback called with NULL block");
		stream_data->state = I2S_STATE_ERROR;
		return;
	}

	if (status < 0) {
		LOG_ERR("RX DMA status bad: %d", status);
		stream_data->state = I2S_STATE_ERROR;
		return;
	}

	/* we have completely received the block, push to queue to let user handle and free it*/
	err = k_msgq_put(stream_data->queue, &stream_data->cur_block, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Failed to put item to RX queue: %d", err);
		free_mem_block(stream);
		stream_data->state = I2S_STATE_ERROR;
		return;
	}

	/* remove reference and let user free it when done */
	stream_data->cur_block.block = NULL;

	if (stream_data->state == I2S_STATE_STOPPING) {
		stream_data->state = I2S_STATE_READY;
		return;
	}

	err = restart_stream(stream, I2S_DIR_RX);
	if (err < 0) {
		LOG_ERR("Failed to restart RX transfer: %d", err);
		stream_data->state = I2S_STATE_ERROR;
	}
}

static int i2s_max32_trigger_single(const struct device *dev, enum i2s_dir dir,
				    enum i2s_trigger_cmd cmd, const struct i2s_max32_stream *stream)
{
	struct i2s_max32_stream_data *stream_data = stream->data;

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream_data->state != I2S_STATE_READY) {
			LOG_ERR("START - Invalid state: %d", (int)stream_data->state);
			return -EIO;
		}
		return start_stream(stream, dir);

	case I2S_TRIGGER_STOP:
		if (stream_data->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP - Invalid state: %d", (int)stream_data->state);
			return -EIO;
		}
		trigger_stream_stop(stream, false);
		return 0;
	case I2S_TRIGGER_DRAIN:
		if (stream_data->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN - Invalid state: %d", (int)stream_data->state);
			return -EIO;
		}
		trigger_stream_stop(stream, true);
		return 0;
	case I2S_TRIGGER_DROP:
		if (stream_data->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - Invalid state: %d", (int)stream_data->state);
			return -EIO;
		}
		return terminate_stream(stream);
	case I2S_TRIGGER_PREPARE:
		if (stream_data->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE - Invalid state: %d", (int)stream_data->state);
			return -EIO;
		}
		clean_stream(stream);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int i2s_max32_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	int ret;
	const struct i2s_max32_cfg *cfg = dev->config;

	LOG_DBG("trigger with dir=%d, cmd=%d", (int)dir, (int)cmd);

	switch (dir) {
	case I2S_DIR_TX:
		return i2s_max32_trigger_single(dev, dir, cmd, &cfg->tx);
	case I2S_DIR_RX:
		return i2s_max32_trigger_single(dev, dir, cmd, &cfg->rx);
	case I2S_DIR_BOTH:
		/*
		 * If user requests both TX and RX trigger, we trigger them one by one here.
		 * Failing to trigger any stream will result in an error.
		 * This could mean one has been trigger successfully and the other has failed.
		 * This is not an error condition, as the user can choose to trigger only one
		 * stream at a time.
		 */

		ret = i2s_max32_trigger_single(dev, I2S_DIR_TX, cmd, &cfg->tx);
		if (ret < 0) {
			return ret;
		}

		ret = i2s_max32_trigger_single(dev, I2S_DIR_RX, cmd, &cfg->rx);
		if (ret < 0) {
			return ret;
		}

		return 0;
	default:
		LOG_ERR("Invalid I2S direction: %d", (int)dir);
		return -EINVAL;
	}
}

static int i2s_cfg_to_max32_cfg(const struct i2s_config *i2s_cfg, mxc_i2s_req_t *max_cfg,
				uint32_t i2s_clk_freq)
{
	/* Input validation */
	__ASSERT(i2s_cfg != NULL, "i2s_cfg cannot be NULL");
	__ASSERT(max_cfg != NULL, "max_cfg cannot be NULL");

	/* Clear configuration struct */
	memset(max_cfg, 0, sizeof(mxc_i2s_req_t));

	/* Validate word size */
	switch (i2s_cfg->word_size) {
	case 8:
		max_cfg->wordSize = MXC_I2S_WSIZE_BYTE;
		max_cfg->sampleSize = MXC_I2S_SAMPLESIZE_EIGHT;
		break;
	case 16:
		max_cfg->wordSize = MXC_I2S_WSIZE_HALFWORD;
		max_cfg->sampleSize = MXC_I2S_SAMPLESIZE_SIXTEEN;
		break;
	case 32:
		max_cfg->wordSize = MXC_I2S_WSIZE_WORD;
		max_cfg->sampleSize = MXC_I2S_SAMPLESIZE_THIRTYTWO;
		break;
	default:
		LOG_ERR("Unsupported word size: %u", i2s_cfg->word_size);
		return -EINVAL;
	}

	/* Validate channels */
	if (i2s_cfg->channels == 2) {
		max_cfg->stereoMode = MXC_I2S_STEREO;
	} else if (i2s_cfg->channels == 1) {
		max_cfg->stereoMode = MXC_I2S_MONO_RIGHT_CH;
	} else {
		LOG_ERR("Unsupported number of channels: %u", i2s_cfg->channels);
		return -EINVAL;
	}

	/* Validate format */
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		max_cfg->justify = MXC_I2S_LSB_JUSTIFY;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		max_cfg->justify = MXC_I2S_MSB_JUSTIFY;
		break;
	default:
		LOG_ERR("Unsupported data format: 0x%02x", i2s_cfg->format);
		return -EINVAL;
	}

	/* Check unsupported format options */
	if (i2s_cfg->format &
	    (I2S_FMT_DATA_ORDER_LSB | I2S_FMT_BIT_CLK_INV | I2S_FMT_FRAME_CLK_INV)) {
		LOG_ERR("Unsupported format options: 0x%02x", i2s_cfg->format);
		return -EINVAL;
	}

	/* Set controller/target mode */
	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET) {
		max_cfg->channelMode = MXC_I2S_EXTERNAL_SCK_EXTERNAL_WS;
	} else {
		max_cfg->channelMode = MXC_I2S_INTERNAL_SCK_WS_0;
	}

	/* Check unsupported options */
	if (i2s_cfg->options & (I2S_OPT_LOOPBACK | I2S_OPT_PINGPONG)) {
		LOG_ERR("Unsupported options: 0x%02x", i2s_cfg->options);
		return -EINVAL;
	}

	/* Set standard values */
	max_cfg->bitOrder = MXC_I2S_LSB_FIRST;
	max_cfg->wsPolarity = MXC_I2S_POL_NORMAL;
	max_cfg->bitsWord = i2s_cfg->word_size;
	max_cfg->adjust = MXC_I2S_ADJUST_LEFT;

	/* Calculate clock divider for sample rate */
	max_cfg->clkdiv = Wrap_MXC_I2S_CalculateClockDiv(i2s_cfg->frame_clk_freq, max_cfg->wordSize,
							 i2s_clk_freq);
	if (max_cfg->clkdiv < 0) {
		LOG_ERR("Invalid frame clock frequency: %u", i2s_cfg->frame_clk_freq);
		return -EINVAL;
	}

	return 0;
}

static int i2s_max32_configure_single(const struct device *dev, enum i2s_dir dir,
				      const struct i2s_config *i2s_cfg,
				      const struct i2s_max32_stream *stream)
{
	int ret;
	struct i2s_max32_stream_data *stream_data = stream->data;
	const struct i2s_max32_cfg *config = dev->config;
	mxc_i2s_req_t mxc_cfg;

	if ((stream_data->state != I2S_STATE_NOT_READY) &&
	    (stream_data->state != I2S_STATE_READY)) {
		LOG_ERR("Invalid state: %d", (int)stream_data->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0) {
		terminate_stream(stream);
		return 0;
	}

	ret = i2s_cfg_to_max32_cfg(i2s_cfg, &mxc_cfg, config->i2s_clk_freq);
	if (ret < 0) {
		LOG_ERR("Failed to convert I2S config to MAX32 config");
		return ret;
	}

	ret = mxc_i2s_init(&mxc_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to initialize I2S: %d", ret);
		return -EINVAL;
	}

	stream_data->i2s_cfg = *i2s_cfg;
	stream_data->state = I2S_STATE_READY;
	return 0;
}

static int i2s_max32_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	int ret;
	const struct i2s_max32_cfg *cfg = dev->config;

	LOG_DBG("configure with dir=%d, word_size=%u, channels=%u, frame_clk_freq=%u", (int)dir,
		i2s_cfg->word_size, i2s_cfg->channels, i2s_cfg->frame_clk_freq);

	switch (dir) {
	case I2S_DIR_TX:
		return i2s_max32_configure_single(dev, dir, i2s_cfg, &cfg->tx);
	case I2S_DIR_RX:
		return i2s_max32_configure_single(dev, dir, i2s_cfg, &cfg->rx);
	case I2S_DIR_BOTH:
		/*
		 * If user requests both TX and RX configuration, we can configure both streams
		 * with the same configuration. This is useful for full-duplex operation.
		 * Failing to configure any stream will result in an error.
		 * This could mean one has been configured successfully and the other has failed.
		 * This is not an error condition, as the user can choose to configure only one
		 * stream at a time.
		 */
		ret = i2s_max32_configure_single(dev, I2S_DIR_TX, i2s_cfg, &cfg->tx);
		if (ret < 0) {
			return ret;
		}

		ret = i2s_max32_configure_single(dev, I2S_DIR_RX, i2s_cfg, &cfg->rx);
		if (ret < 0) {
			return ret;
		}
		return 0;
	default:
		LOG_ERR("Invalid I2S direction: %d", (int)dir);
		return -EINVAL;
	}
}

static int i2s_max32_read(const struct device *dev, void **mem_block, size_t *size)
{
	int err;
	const struct i2s_max32_cfg *cfg = dev->config;
	const struct i2s_max32_stream *stream = &cfg->rx;
	struct i2s_max32_stream_data *stream_data = stream->data;
	struct i2s_mem_block block;

	if (stream_data->state == I2S_STATE_NOT_READY) {
		LOG_ERR("RX invalid state: %d", (int)stream_data->state);
		return -EIO;
	} else if (stream_data->state == I2S_STATE_ERROR &&
		   k_msgq_num_used_get(stream_data->queue) == 0) {
		LOG_ERR("RX queue empty");
		return -EIO;
	}

	err = k_msgq_get(stream_data->queue, &block, K_MSEC(stream_data->i2s_cfg.timeout));
	if (err < 0) {
		LOG_ERR("RX queue empty");
		return err;
	}

	*mem_block = block.block;
	*size = block.size;

	return 0;
}

static int i2s_max32_write(const struct device *dev, void *mem_block, size_t size)
{
	int err;
	const struct i2s_max32_cfg *cfg = dev->config;
	const struct i2s_max32_stream *stream = &cfg->tx;
	struct i2s_max32_stream_data *stream_data = stream->data;
	struct i2s_mem_block block = {.block = mem_block, .size = size};

	if (stream_data->state != I2S_STATE_READY && stream_data->state != I2S_STATE_RUNNING) {
		LOG_ERR("TX Invalid state: %d", (int)stream_data->state);
		return -EIO;
	}

	if (size > stream_data->i2s_cfg.block_size) {
		LOG_ERR("Max write size is: %u", (unsigned int)stream_data->i2s_cfg.block_size);
		return -EINVAL;
	}

	err = k_msgq_put(stream_data->queue, &block, K_MSEC(stream_data->i2s_cfg.timeout));
	if (err < 0) {
		LOG_ERR("TX queue full");
		return err;
	}

	return 0;
}

static DEVICE_API(i2s, i2s_max32_driver_api) = {
	.read = i2s_max32_read,
	.write = i2s_max32_write,
	.configure = i2s_max32_configure,
	.trigger = i2s_max32_trigger,
};

static int i2s_max32_init(const struct device *dev)
{
	const struct i2s_max32_cfg *config = dev->config;
	int err;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	return 0;
}

#define MAX32_DT_INST_DMA_CTLR(n, name) DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))

#define MAX32_DT_INST_DMA_CELL(n, name, cell) DT_INST_DMAS_CELL_BY_NAME(n, name, cell)

#define I2S_MAX32_STREAM_DATA_CREATE_AND_INIT(n, dir)                                              \
	K_MSGQ_DEFINE(i2s_max32_##dir##_q_##n, sizeof(struct i2s_mem_block),                       \
		      CONFIG_I2S_MAX32_QUEUE_SIZE, 1);                                             \
	static struct i2s_max32_stream_data i2s_max32_##dir##_data_##n = {                         \
		.state = I2S_STATE_NOT_READY,                                                      \
		.drain = false,                                                                    \
		.queue = &i2s_max32_##dir##_q_##n,                                                 \
	};

#define I2S_MAX32_STREAM_INIT(n, dir)                                                              \
	{                                                                                          \
		.data = &i2s_max32_##dir##_data_##n,                                               \
		.i2s =                                                                             \
			{                                                                          \
				.reg = (mxc_i2s_regs_t *)DT_INST_REG_ADDR(n),                      \
				.dev = DEVICE_DT_INST_GET(n),                                      \
			},                                                                         \
		.dma =                                                                             \
			{                                                                          \
				.dev = MAX32_DT_INST_DMA_CTLR(n, dir),                             \
				.channel = MAX32_DT_INST_DMA_CELL(n, dir, channel),                \
				.slot = MAX32_DT_INST_DMA_CELL(n, dir, slot),                      \
			},                                                                         \
	}

#define I2S_MAX32_INIT(n)                                                                          \
	PINCTRL_DT_DEFINE(DT_DRV_INST(n));                                                         \
                                                                                                   \
	I2S_MAX32_STREAM_DATA_CREATE_AND_INIT(n, tx);                                              \
	I2S_MAX32_STREAM_DATA_CREATE_AND_INIT(n, rx);                                              \
                                                                                                   \
	static const struct i2s_max32_cfg i2s_max32_cfg_##n = {                                    \
		.tx = I2S_MAX32_STREAM_INIT(n, tx),                                                \
		.rx = I2S_MAX32_STREAM_INIT(n, rx),                                                \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(n)),                                 \
		.i2s_clk_freq = DT_INST_PROP(n, i2s_clk_frequency),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, i2s_max32_init, NULL, NULL, &i2s_max32_cfg_##n, POST_KERNEL,      \
			      CONFIG_I2S_INIT_PRIORITY, &i2s_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_MAX32_INIT)
