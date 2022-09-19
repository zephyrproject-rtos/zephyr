/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_lpc_i2s

#include <string.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_i2s.h>
#include <fsl_dma.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

LOG_MODULE_REGISTER(i2s_mcux_flexcomm);

#define NUM_DMA_BLOCKS	2

/* Device constant configuration parameters */
struct i2s_mcux_config {
	I2S_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config)(const struct device *dev);
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

struct stream {
	int32_t state;
	const struct device *dev_dma;
	uint32_t channel; /* stores the channel for dma */
	struct i2s_config cfg;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block[NUM_DMA_BLOCKS];
	bool last_block;
	struct k_msgq in_queue;
	void *in_msgs[CONFIG_I2S_MCUX_FLEXCOMM_RX_BLOCK_COUNT];
	struct k_msgq out_queue;
	void *out_msgs[CONFIG_I2S_MCUX_FLEXCOMM_TX_BLOCK_COUNT];
};

struct i2s_mcux_data {
	struct stream rx;
	struct stream tx;
};

static int i2s_mcux_flexcomm_cfg_convert(uint32_t base_frequency,
					 enum i2s_dir dir,
					 const struct i2s_config *i2s_cfg,
					 i2s_config_t *fsl_cfg)
{
	if (dir == I2S_DIR_RX) {
		I2S_RxGetDefaultConfig(fsl_cfg);
	} else if (dir == I2S_DIR_TX) {
		I2S_TxGetDefaultConfig(fsl_cfg);
	}

	/* Support single channel pair */
	if (i2s_cfg->channels == 0 || i2s_cfg->channels > 2) {
		LOG_ERR("unsupported number of channels");
		return -EINVAL;
	}

	fsl_cfg->oneChannel = (i2s_cfg->channels == 1);

	fsl_cfg->dataLength = i2s_cfg->word_size;
	fsl_cfg->frameLength = i2s_cfg->channels * i2s_cfg->word_size;

	/* Set master/slave configuration */
	switch (i2s_cfg->options & (I2S_OPT_BIT_CLK_SLAVE |
				    I2S_OPT_FRAME_CLK_SLAVE)) {
	case I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER:
		fsl_cfg->masterSlave = kI2S_MasterSlaveNormalMaster;
		break;
	case I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE:
		fsl_cfg->masterSlave = kI2S_MasterSlaveNormalSlave;
		break;
	case I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_MASTER:
		/* Master using external CLK */
		fsl_cfg->masterSlave = kI2S_MasterSlaveExtSckMaster;
		break;
	case I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_SLAVE:
		/* WS synchronized master */
		fsl_cfg->masterSlave = kI2S_MasterSlaveWsSyncMaster;
		break;
	}

	/*
	 * Set format. Zephyr choose arbitrary subset of possible
	 * formats, the mapping below is not tested for anything
	 * but classic mode and is not guaranteed to be correct.
	 */
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		fsl_cfg->mode = kI2S_ModeI2sClassic;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		fsl_cfg->mode = kI2S_ModeDspWsShort;
		fsl_cfg->wsPol = true;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		fsl_cfg->mode = kI2S_ModeDspWsLong;
		fsl_cfg->wsPol = true;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		fsl_cfg->mode = kI2S_ModeDspWs50;
		fsl_cfg->wsPol = true;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		fsl_cfg->mode = kI2S_ModeDspWs50;
		fsl_cfg->wsPol = true;
		break;
	default:
		LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	if (fsl_cfg->masterSlave == kI2S_MasterSlaveNormalMaster ||
		fsl_cfg->masterSlave == kI2S_MasterSlaveWsSyncMaster) {
		fsl_cfg->divider = base_frequency /
				   i2s_cfg->frame_clk_freq /
				   fsl_cfg->frameLength;
	}

	/*
	 * Set frame and bit clock polarity according to
	 * inversion flags.
	 */
	switch (i2s_cfg->format & I2S_FMT_CLK_FORMAT_MASK) {
	case I2S_FMT_CLK_NF_NB:
		break;
	case I2S_FMT_CLK_NF_IB:
		fsl_cfg->sckPol = !fsl_cfg->sckPol;
		break;
	case I2S_FMT_CLK_IF_NB:
		fsl_cfg->wsPol = !fsl_cfg->wsPol;
		break;
	case I2S_FMT_CLK_IF_IB:
		fsl_cfg->sckPol = !fsl_cfg->sckPol;
		fsl_cfg->wsPol = !fsl_cfg->wsPol;
		break;
	default:
		LOG_ERR("Unsupported clocks polarity");
		return -EINVAL;
	}

	return 0;
}

static const struct i2s_config *i2s_mcux_config_get(const struct device *dev,
						    enum i2s_dir dir)
{
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else {
		stream = &dev_data->tx;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->cfg;
}

static int i2s_mcux_configure(const struct device *dev, enum i2s_dir dir,
			      const struct i2s_config *i2s_cfg)
{
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	I2S_Type *base = cfg->base;
	struct stream *stream;
	uint32_t base_frequency;
	i2s_config_t fsl_cfg;
	int result;
	uint8_t bits_per_word = 0;
	uint8_t bytes_per_word = 0;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY &&
	    stream->state != I2S_STATE_READY) {
		LOG_ERR("invalid state");
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	/*
	 * The memory block passed by the user to the i2s_write function is
	 * tightly packed next to each other.
	 * However for 8-bit word_size the I2S hardware expects the data
	 * to be in 2bytes which does not match what is passed by the user.
	 * This will be addressed in a separate PR once the zephyr API committee
	 * finalizes on an I2S API for the user to probe hardware variations.
	 */
	if (i2s_cfg->word_size <= 8) {
		return -ENOTSUP;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Figure out function base clock */
	if (clock_control_get_rate(cfg->clock_dev,
				   cfg->clock_subsys, &base_frequency)) {
		return -EINVAL;
	}

	/*
	 * Validate the configuration by converting it to SDK
	 * format.
	 */
	result = i2s_mcux_flexcomm_cfg_convert(base_frequency, dir, i2s_cfg,
					       &fsl_cfg);
	if (result != 0) {
		return result;
	}

	/* Apply the configuration */
	if (dir == I2S_DIR_RX) {
		I2S_RxInit(cfg->base, &fsl_cfg);
	} else {
		I2S_TxInit(cfg->base, &fsl_cfg);
	}

	/* Data length in bits */
	bits_per_word = (uint8_t)(((base->CFG1 & I2S_CFG1_DATALEN_MASK) >>
						I2S_CFG1_DATALEN_SHIFT) + 1U);

	/* Convert to bytes */
	bytes_per_word = (bits_per_word + 7U) / 8U;

	/* if one channel is disabled, bytes_per_word should be 4U, user should
	 * pay attention that when data length is shorter than 16,
	 * the data format: left data put in 0-15 bit and right data should put in 16-31
	 */
	if (((base->CFG1 & I2S_CFG1_ONECHANNEL_MASK) == 0U)) {
		bytes_per_word = 4U;
	}
	/* since DMA do not support 24bit transfer width, use 32bit instead */
	if (bytes_per_word == 3U) {
		bytes_per_word = 4U;
	}

	stream->dma_cfg.dest_data_size = bytes_per_word;

	/* Save configuration for get_config */
	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->state = I2S_STATE_READY;
	return 0;
}

static inline void i2s_purge_stream_buffers(struct stream *stream,
					    struct k_mem_slab *mem_slab)
{
	void *buffer;

	while (k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(mem_slab, &buffer);
	}
	while (k_msgq_get(&stream->out_queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(mem_slab, &buffer);
	}
}

static void i2s_mcux_tx_stream_disable(const struct device *dev, bool drop)
{
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->tx;
	I2S_Type *base = cfg->base;

	LOG_DBG("Stopping DMA channel %u for TX stream", stream->channel);
	dma_stop(stream->dev_dma, stream->channel);

	/* Clear TX error interrupt flag */
	base->FIFOSTAT = I2S_FIFOSTAT_TXERR(1U);
	I2S_DisableInterrupts(base, (uint32_t)kI2S_TxErrorFlag);

	if (base->CFG1 & I2S_CFG1_MAINENABLE_MASK) {
		/* Wait until all transmitted data get out of FIFO */
		while ((base->FIFOSTAT & I2S_FIFOSTAT_TXEMPTY_MASK) == 0U) {
		}
		/*
		 * The last piece of valid data can be still being transmitted from
		 * I2S at this moment
		 */
		/* Write additional data to FIFO */
		base->FIFOWR = 0U;
		while ((base->FIFOSTAT & I2S_FIFOSTAT_TXEMPTY_MASK) == 0U) {
		}

		/* At this moment the additional data is out of FIFO, we can stop I2S */
		/* Disable TX DMA */
		base->FIFOCFG &= (~I2S_FIFOCFG_DMATX_MASK);
		base->FIFOCFG |= I2S_FIFOCFG_EMPTYTX_MASK;

		I2S_Disable(base);
	}

	/* purge buffers queued in the stream */
	if (drop) {
		i2s_purge_stream_buffers(stream, stream->cfg.mem_slab);
	}
}

static void i2s_mcux_rx_stream_disable(const struct device *dev, bool drop)
{
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->rx;
	I2S_Type *base = cfg->base;

	LOG_DBG("Stopping DMA channel %u for RX stream", stream->channel);
	dma_stop(stream->dev_dma, stream->channel);

	/* Clear RX error interrupt flag */
	base->FIFOSTAT = I2S_FIFOSTAT_RXERR(1U);
	I2S_DisableInterrupts(base, (uint32_t)kI2S_RxErrorFlag);

	/* stop transfer */
	/* Disable Rx DMA */
	base->FIFOCFG &= (~I2S_FIFOCFG_DMARX_MASK);
	base->FIFOCFG |= I2S_FIFOCFG_EMPTYRX_MASK;

	I2S_Disable(base);

	/* purge buffers queued in the stream */
	if (drop) {
		i2s_purge_stream_buffers(stream, stream->cfg.mem_slab);
	}
}

static void i2s_mcux_config_dma_blocks(const struct device *dev,
					enum i2s_dir dir, uint32_t *buffer)
{
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	I2S_Type *base = cfg->base;
	struct dma_block_config *blk_cfg;
	struct stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else {
		stream = &dev_data->tx;
	}

	blk_cfg = &stream->dma_block[0];
	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	if (dir == I2S_DIR_RX) {
		blk_cfg->source_address = (uint32_t)&base->FIFORD;
		blk_cfg->dest_address = (uint32_t)buffer[0];
		blk_cfg->block_size = stream->cfg.block_size;
		blk_cfg->source_gather_en = 1;
		blk_cfg->next_block = &stream->dma_block[1];

		blk_cfg = &stream->dma_block[1];
		memset(blk_cfg, 0, sizeof(struct dma_block_config));

		blk_cfg->source_address = (uint32_t)&base->FIFORD;
		blk_cfg->dest_address = (uint32_t)buffer[1];
		blk_cfg->block_size = stream->cfg.block_size;

		stream->dma_cfg.block_count = NUM_DMA_BLOCKS;
	} else {
		blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
		blk_cfg->source_address = (uint32_t)buffer;
		blk_cfg->block_size = stream->cfg.block_size;
		stream->dma_cfg.block_count = 1;
	}

	stream->dma_cfg.head_block = &stream->dma_block[0];
	stream->dma_cfg.user_data = (void *)dev;

	dma_config(stream->dev_dma, stream->channel, &stream->dma_cfg);

	LOG_DBG("dma_slot is %d", stream->dma_cfg.dma_slot);
	LOG_DBG("channel_direction is %d", stream->dma_cfg.channel_direction);
	LOG_DBG("complete_callback_en is %d",
		stream->dma_cfg.complete_callback_en);
	LOG_DBG("error_callback_en is %d", stream->dma_cfg.error_callback_en);
	LOG_DBG("source_handshake is %d", stream->dma_cfg.source_handshake);
	LOG_DBG("dest_handshake is %d", stream->dma_cfg.dest_handshake);
	LOG_DBG("channel_priority is %d", stream->dma_cfg.channel_priority);
	LOG_DBG("source_chaining_en is %d", stream->dma_cfg.source_chaining_en);
	LOG_DBG("dest_chaining_en is %d", stream->dma_cfg.dest_chaining_en);
	LOG_DBG("linked_channel is %d", stream->dma_cfg.linked_channel);
	LOG_DBG("source_data_size is %d", stream->dma_cfg.source_data_size);
	LOG_DBG("dest_data_size is %d", stream->dma_cfg.dest_data_size);
	LOG_DBG("source_burst_length is %d", stream->dma_cfg.source_burst_length);
	LOG_DBG("dest_burst_length is %d", stream->dma_cfg.dest_burst_length);
	LOG_DBG("block_count is %d", stream->dma_cfg.block_count);
}

/* This function is executed in the interrupt context */
static void i2s_mcux_dma_tx_callback(const struct device *dma_dev, void *arg,
				uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)arg;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->tx;
	void *buffer;
	int ret;

	LOG_DBG("tx cb: %d", stream->state);
	ret = k_msgq_get(&stream->out_queue, &buffer, K_NO_WAIT);
	if (ret == 0) {
		/* transmission complete. free the buffer */
		k_mem_slab_free(stream->cfg.mem_slab, &buffer);
	} else {
		LOG_ERR("no buffer in output queue for channel %u", channel);
	}

	/* Received a STOP trigger, terminate TX immediately */
	if (stream->last_block) {
		stream->state = I2S_STATE_READY;
		i2s_mcux_tx_stream_disable(dev, false);
		LOG_DBG("TX STOPPED");
		return;
	}

	switch (stream->state) {
	case I2S_STATE_RUNNING:
	case I2S_STATE_STOPPING:
		/* get the next buffer from queue */
		ret = k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT);
		if (ret == 0) {
			/* config the DMA */
			i2s_mcux_config_dma_blocks(dev, I2S_DIR_TX, (uint32_t *)buffer);
			k_msgq_put(&stream->out_queue, &buffer, K_NO_WAIT);
			dma_start(stream->dev_dma, stream->channel);
		}

		if (ret || status) {
			/*
			 * DMA encountered an error (status != 0)
			 * or
			 * No buffers in input queue
			 */
			LOG_DBG("DMA status %08x channel %u k_msgq_get ret %d",
				status, channel, ret);
			if (stream->state == I2S_STATE_STOPPING) {
				stream->state = I2S_STATE_READY;
			} else {
				stream->state = I2S_STATE_ERROR;
			}
			i2s_mcux_tx_stream_disable(dev, false);
		}
		break;
	case I2S_STATE_ERROR:
		i2s_mcux_tx_stream_disable(dev, true);
		break;
	}
}

static void i2s_mcux_dma_rx_callback(const struct device *dma_dev, void *arg,
				uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)arg;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->rx;
	void *buffer;
	int ret;

	LOG_DBG("rx cb: %d", stream->state);

	if (status != 0) {
		stream->state = I2S_STATE_ERROR;
		i2s_mcux_rx_stream_disable(dev, false);
		return;
	}

	switch (stream->state) {
	case I2S_STATE_STOPPING:
	case I2S_STATE_RUNNING:
		/* retrieve buffer from input queue */
		ret = k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT);
		__ASSERT_NO_MSG(ret == 0);

		/* put buffer to output queue */
		ret = k_msgq_put(&stream->out_queue, &buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("buffer %p -> out_queue %p err %d", buffer,
				&stream->out_queue, ret);
			i2s_mcux_rx_stream_disable(dev, false);
			stream->state = I2S_STATE_ERROR;
		}
		if (stream->state == I2S_STATE_RUNNING) {
			/* allocate new buffer for next audio frame */
			ret = k_mem_slab_alloc(stream->cfg.mem_slab, &buffer, K_NO_WAIT);
			if (ret != 0) {
				LOG_ERR("buffer alloc from slab %p err %d",
					stream->cfg.mem_slab, ret);
				i2s_mcux_rx_stream_disable(dev, false);
				stream->state = I2S_STATE_ERROR;
			} else {
				const struct i2s_mcux_config *cfg = dev->config;
				I2S_Type *base = cfg->base;

				dma_reload(stream->dev_dma, stream->channel,
					   (uint32_t)&base->FIFORD, (uint32_t)buffer,
					   stream->cfg.block_size);
				/* put buffer in input queue */
				ret = k_msgq_put(&stream->in_queue, &buffer, K_NO_WAIT);
				if (ret != 0) {
					LOG_ERR("buffer %p -> in_queue %p err %d",
					buffer, &stream->in_queue, ret);
				}
				dma_start(stream->dev_dma, stream->channel);
			}
		} else {
			/* Received a STOP/DRAIN trigger */
			i2s_mcux_rx_stream_disable(dev, true);
			stream->state = I2S_STATE_READY;
		}
		break;
	case I2S_STATE_ERROR:
		i2s_mcux_rx_stream_disable(dev, true);
		break;
	}
}

static int i2s_mcux_tx_stream_start(const struct device *dev)
{
	int ret = 0;
	void *buffer;
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->tx;
	I2S_Type *base = cfg->base;

	/* retrieve buffer from input queue */
	ret = k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("No buffer in input queue to start transmission");
		return ret;
	}

	i2s_mcux_config_dma_blocks(dev, I2S_DIR_TX, (uint32_t *)buffer);

	/* put buffer in output queue */
	ret = k_msgq_put(&stream->out_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to put buffer in output queue");
		return ret;
	}

	/* Enable TX DMA */
	base->FIFOCFG |= I2S_FIFOCFG_DMATX_MASK;

	ret = dma_start(stream->dev_dma, stream->channel);
	if (ret < 0) {
		LOG_ERR("dma_start failed (%d)", ret);
		return ret;
	}

	I2S_Enable(base);
	I2S_EnableInterrupts(base, (uint32_t)kI2S_TxErrorFlag);

	return 0;
}

static int i2s_mcux_rx_stream_start(const struct device *dev)
{
	int ret = 0;
	void *buffer[NUM_DMA_BLOCKS];
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->rx;
	I2S_Type *base = cfg->base;
	uint8_t num_of_bufs;

	num_of_bufs = k_mem_slab_num_free_get(stream->cfg.mem_slab);

	/*
	 * Need at least two buffers on the RX memory slab for
	 * reliable DMA reception.
	 */
	if (num_of_bufs <= 1) {
		return -EINVAL;
	}

	for (int i = 0; i < NUM_DMA_BLOCKS; i++) {
		ret = k_mem_slab_alloc(stream->cfg.mem_slab, &buffer[i],
							K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("buffer alloc from mem_slab failed (%d)", ret);
			return ret;
		}
	}

	i2s_mcux_config_dma_blocks(dev, I2S_DIR_RX, (uint32_t *)buffer);

	/* put buffers in input queue */
	for (int i = 0; i < NUM_DMA_BLOCKS; i++) {
		ret = k_msgq_put(&stream->in_queue, &buffer[i], K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("failed to put buffer in input queue");
			return ret;
		}
	}

	/* Enable RX DMA */
	base->FIFOCFG |= I2S_FIFOCFG_DMARX_MASK;

	ret = dma_start(stream->dev_dma, stream->channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA Ch%d (%d)", stream->channel, ret);
		return ret;
	}

	I2S_Enable(base);
	I2S_EnableInterrupts(base, (uint32_t)kI2S_RxErrorFlag);

	return 0;
}

static int i2s_mcux_trigger(const struct device *dev, enum i2s_dir dir,
			     enum i2s_trigger_cmd cmd)
{
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream;
	unsigned int key;
	int ret = 0;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	key = irq_lock();

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d",
				    stream->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			ret = i2s_mcux_tx_stream_start(dev);
		} else {
			ret = i2s_mcux_rx_stream_start(dev);
		}

		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			break;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;
		break;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->state = I2S_STATE_STOPPING;
		stream->last_block = true;
		break;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->state = I2S_STATE_READY;
		if (dir == I2S_DIR_TX) {
			i2s_mcux_tx_stream_disable(dev, true);
		} else {
			i2s_mcux_rx_stream_disable(dev, true);
		}
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->state = I2S_STATE_READY;
		if (dir == I2S_DIR_TX) {
			i2s_mcux_tx_stream_disable(dev, true);
		} else {
			i2s_mcux_rx_stream_disable(dev, true);
		}
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		ret = -EINVAL;
	}

	irq_unlock(key);

	return ret;
}

static int i2s_mcux_read(const struct device *dev, void **mem_block,
			  size_t *size)
{
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->rx;
	void *buffer;
	int ret = 0;

	if (stream->state == I2S_STATE_NOT_READY) {
		LOG_ERR("invalid state %d", stream->state);
		return -EIO;
	}

	ret = k_msgq_get(&stream->out_queue, &buffer,
			 SYS_TIMEOUT_MS(stream->cfg.timeout));

	if (ret != 0) {
		if (stream->state == I2S_STATE_ERROR) {
			return -EIO;
		} else {
			return -EAGAIN;
		}
	}

	*mem_block = buffer;
	*size = stream->cfg.block_size;
	return 0;
}

static int i2s_mcux_write(const struct device *dev, void *mem_block,
			   size_t size)
{
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->tx;
	int ret;


	if (stream->state != I2S_STATE_RUNNING &&
		stream->state != I2S_STATE_READY) {
		LOG_ERR("invalid state (%d)", stream->state);
		return -EIO;
	}

	ret = k_msgq_put(&stream->in_queue, &mem_block,
			 SYS_TIMEOUT_MS(stream->cfg.timeout));

	if (ret) {
		LOG_ERR("k_msgq_put failed %d", ret);
		return ret;
	}

	return ret;
}

static const struct i2s_driver_api i2s_mcux_driver_api = {
	.configure = i2s_mcux_configure,
	.config_get = i2s_mcux_config_get,
	.read = i2s_mcux_read,
	.write = i2s_mcux_write,
	.trigger = i2s_mcux_trigger,
};

static void i2s_mcux_isr(const struct device *dev)
{
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *dev_data = dev->data;
	struct stream *stream = &dev_data->tx;
	I2S_Type *base = cfg->base;
	uint32_t intstat = base->FIFOINTSTAT;

	if ((intstat & I2S_FIFOINTSTAT_TXERR_MASK) != 0UL) {
		/* Clear TX error interrupt flag */
		base->FIFOSTAT = I2S_FIFOSTAT_TXERR(1U);
		stream = &dev_data->tx;
		stream->state = I2S_STATE_ERROR;
	}

	if ((intstat & I2S_FIFOINTSTAT_RXERR_MASK) != 0UL) {
		/* Clear RX error interrupt flag */
		base->FIFOSTAT = I2S_FIFOSTAT_RXERR(1U);
		stream = &dev_data->rx;
		stream->state = I2S_STATE_ERROR;
	}
}

static int i2s_mcux_init(const struct device *dev)
{
	const struct i2s_mcux_config *cfg = dev->config;
	struct i2s_mcux_data *const data = dev->data;
#ifdef CONFIG_PINCTRL
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}
#endif

	cfg->irq_config(dev);

	/* Initialize the buffer queues */
	k_msgq_init(&data->tx.in_queue, (char *)data->tx.in_msgs,
		    sizeof(void *), CONFIG_I2S_MCUX_FLEXCOMM_TX_BLOCK_COUNT);
	k_msgq_init(&data->rx.in_queue, (char *)data->rx.in_msgs,
		    sizeof(void *), CONFIG_I2S_MCUX_FLEXCOMM_RX_BLOCK_COUNT);
	k_msgq_init(&data->tx.out_queue, (char *)data->tx.out_msgs,
		    sizeof(void *), CONFIG_I2S_MCUX_FLEXCOMM_TX_BLOCK_COUNT);
	k_msgq_init(&data->rx.out_queue, (char *)data->rx.out_msgs,
		    sizeof(void *), CONFIG_I2S_MCUX_FLEXCOMM_RX_BLOCK_COUNT);

	if (data->tx.dev_dma != NULL) {
		if (!device_is_ready(data->tx.dev_dma)) {
			LOG_ERR("%s device not ready", data->tx.dev_dma->name);
			return -ENODEV;
		}
	}

	if (data->rx.dev_dma != NULL) {
		if (!device_is_ready(data->rx.dev_dma)) {
			LOG_ERR("%s device not ready", data->rx.dev_dma->name);
			return -ENODEV;
		}
	}

	data->tx.state = I2S_STATE_NOT_READY;
	data->rx.state = I2S_STATE_NOT_READY;

	LOG_INF("Device %s inited", dev->name);

	return 0;
}


#ifdef CONFIG_PINCTRL
#define I2S_MCUX_FLEXCOMM_PINCTRL_DEFINE(id) PINCTRL_DT_INST_DEFINE(id);
#define I2S_MCUX_FLEXCOMM_PINCTRL_INIT(id) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),
#else
#define I2S_MCUX_FLEXCOMM_PINCTRL_DEFINE(id)
#define I2S_MCUX_FLEXCOMM_PINCTRL_INIT(id)
#endif

#define I2S_DMA_CHANNELS(id)				\
	.tx = {						\
		.dev_dma = UTIL_AND(		\
			DT_DMAS_HAS_NAME(DT_NODELABEL(i2s##id), tx),	\
			DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, tx))), \
		.channel = UTIL_AND(		\
			DT_DMAS_HAS_NAME(DT_NODELABEL(i2s##id), tx),	\
			DT_INST_DMAS_CELL_BY_NAME(id, tx, channel)),	\
		.dma_cfg = {					\
			.channel_direction = MEMORY_TO_PERIPHERAL,	\
			.dma_callback = i2s_mcux_dma_tx_callback,	\
			.source_data_size = 1,				\
			.block_count = 1,		\
		}							\
	},								\
	.rx = {						\
		.dev_dma = UTIL_AND(		\
			DT_DMAS_HAS_NAME(DT_NODELABEL(i2s##id), rx),	\
			DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, rx))), \
		.channel = UTIL_AND(		\
			DT_DMAS_HAS_NAME(DT_NODELABEL(i2s##id), rx),	\
			DT_INST_DMAS_CELL_BY_NAME(id, rx, channel)),	\
		.dma_cfg = {				\
			.channel_direction = PERIPHERAL_TO_MEMORY,	\
			.dma_callback = i2s_mcux_dma_rx_callback,	\
			.source_data_size = 1,				\
			.block_count = 1,		\
		}							\
	}

#define I2S_MCUX_FLEXCOMM_DEVICE(id)					\
	I2S_MCUX_FLEXCOMM_PINCTRL_DEFINE(id)				\
	static void i2s_mcux_config_func_##id(const struct device *dev); \
	static const struct i2s_mcux_config i2s_mcux_config_##id = {	\
		.base =							\
		(I2S_Type *)DT_INST_REG_ADDR(id),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)), \
		.clock_subsys =				\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),\
		.irq_config = i2s_mcux_config_func_##id,		\
		I2S_MCUX_FLEXCOMM_PINCTRL_INIT(id)			\
	};								\
	static struct i2s_mcux_data i2s_mcux_data_##id = {		\
		I2S_DMA_CHANNELS(id)					\
	};								\
	DEVICE_DT_INST_DEFINE(id,					\
			    &i2s_mcux_init,			\
			    NULL,			\
			    &i2s_mcux_data_##id,			\
			    &i2s_mcux_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_I2S_INIT_PRIORITY,			\
			    &i2s_mcux_driver_api);			\
	static void i2s_mcux_config_func_##id(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(id),				\
			    DT_INST_IRQ(id, priority),			\
			    i2s_mcux_isr,						\
			    DEVICE_DT_INST_GET(id),	\
			    0);						\
		irq_enable(DT_INST_IRQN(id));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2S_MCUX_FLEXCOMM_DEVICE)
