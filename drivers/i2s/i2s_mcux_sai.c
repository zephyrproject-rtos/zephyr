/*
 * Copyright (c) 2021 NXP Semiconductor INC.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2S bus (SAI) driver for NXP i.MX RT series.
 */

#include <errno.h>
#include <string.h>
#include <sys/__assert.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/dma.h>

#include <drivers/i2s.h>
#include <drivers/pinmux.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/imx_ccm.h>
#include <soc.h>

#include "i2s_mcux_sai.h"

#define LOG_DOMAIN dev_i2s_mcux
#define LOG_LEVEL CONFIG_I2S_LOG_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(LOG_DOMAIN);

#define DT_DRV_COMPAT nxp_mcux_i2s

/*
 * This indicates the Tx/Rx stream.
 *
 * in_queue and out_queue are used as follows
 *   transmit stream:
 *   application provided buffer is queued to in_queue until loaded to DMA.
 *   when DMA channel is idle, buffer is retrieved from in_queue and loaded
 *   to DMA and queued to out_queue. when DMA completes, buffer is retrieved
 *   from out_queue and freed.
 *
 *   receive stream:
 *   driver allocates buffer from slab and loads DMA buffer is queued to
 *   in_queue when DMA completes, buffer is retrieved from in_queue
 *   and queued to out_queue when application reads, buffer is read
 *   (may optionally block) from out_queue and presented to application.
 */
struct stream {
	int32_t state;
	uint32_t dma_channel;
	uint32_t start_channel;
	void (*irq_call_back)(void);
	struct i2s_config cfg;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block;
	struct k_msgq in_queue;
	void *in_msgs[CONFIG_I2S_RX_BLOCK_COUNT];
	struct k_msgq out_queue;
	void *out_msgs[CONFIG_I2S_TX_BLOCK_COUNT];
	uint32_t stream_starving;
};

struct i2s_mcux_config {
	I2S_Type *base;
	uint32_t clk_src;
	uint32_t clk_pre_div;
	uint32_t clk_src_div;
	uint32_t clk_src;
	uint32_t pll_src;
	uint32_t pll_lp;
	uint32_t pll_pd;
	uint32_t pll_num;
	uint32_t pll_den;
	uint32_t mclk_pin_mask;
	uint32_t mclk_pin_offset;
	uint32_t tx_channel;
	clock_control_subsys_t clk_sub_sys;
	const struct device *ccm_dev;
	const struct device *pinmux_dev;
	void (*irq_connect)(const struct device *dev);
	bool rx_sync_mode;
	bool tx_sync_mode;
};

/* Device run time data */
struct i2s_dev_data {
	const struct device *dev_dma;
	struct stream tx;
	struct stream rx;
};


static void i2s_dma_tx_callback(const struct device *, void *,
				uint32_t, int);
static void i2s_tx_stream_disable(const struct device *);
static void i2s_rx_stream_disable(const struct device *);

static inline void i2s_purge_stream_buffers(struct stream *strm,
					    struct k_mem_slab *mem_slab)
{
	void *buffer;

	while (k_msgq_get(&strm->in_queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(mem_slab, &buffer);
	}
	while (k_msgq_get(&strm->out_queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(mem_slab, &buffer);
	}
}

static void i2s_tx_stream_disable(const struct device *dev)
{
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->tx;
	const struct device *dev_dma = dev_data->dev_dma;
	const struct i2s_mcux_config *dev_cfg = dev->config;

	LOG_DBG("Stopping DMA channel %u for TX stream", strm->dma_channel);
	dma_stop(dev_dma, strm->dma_channel);

	/* Disable the channel FIFO */
	dev_cfg->base->TCR3 &= ~I2S_TCR3_TCE_MASK;

	/* purge buffers queued in the stream */
	i2s_purge_stream_buffers(strm, dev_data->tx.cfg.mem_slab);

	/* Disable DMA enable bit */
	SAI_TxEnableDMA(dev_cfg->base, kSAI_FIFORequestDMAEnable,
			false);

	/* Disable Tx */
	SAI_TxEnable(dev_cfg->base, false);

	strm->stream_starving = false;

	/* If Tx is disabled, reset the FIFO pointer, clear error flags */
	if ((dev_cfg->base->TCSR & I2S_TCSR_TE_MASK) == 0UL) {
		dev_cfg->base->TCSR |=
			(I2S_TCSR_FR_MASK | I2S_TCSR_SR_MASK);
		dev_cfg->base->TCSR &= ~I2S_TCSR_SR_MASK;
	}
}

static void i2s_rx_stream_disable(const struct device *dev)
{
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->rx;
	const struct device *dev_dma = dev_data->dev_dma;
	const struct i2s_mcux_config *dev_cfg = dev->config;

	LOG_DBG("Stopping RX stream & DMA channel %u", strm->dma_channel);
	dma_stop(dev_dma, strm->dma_channel);

	/* Disable the channel FIFO */
	dev_cfg->base->RCR3 &= ~I2S_RCR3_RCE_MASK;

	/* Disable DMA enable bit */
	SAI_RxEnableDMA(dev_cfg->base, kSAI_FIFORequestDMAEnable,
			false);

	/* Disable Rx */
	SAI_RxEnable(dev_cfg->base, false);

	/* purge buffers queued in the stream */
	i2s_purge_stream_buffers(strm, dev_data->rx.cfg.mem_slab);

	strm->stream_starving = false;

	/* If Rx is disabled, reset the FIFO pointer
	 * and clear error flags
	 */
	if ((dev_cfg->base->RCSR & I2S_RCSR_RE_MASK) == 0UL) {
		dev_cfg->base->RCSR |=
			(I2S_RCSR_FR_MASK | I2S_RCSR_SR_MASK);
		dev_cfg->base->RCSR &= ~I2S_RCSR_SR_MASK;
	}
}

/* This function is executed in the interrupt context */
static void i2s_dma_tx_callback(const struct device *dma_dev,
				void *arg, uint32_t channel, int status)
{
	const struct device *dev = (struct device *)arg;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->tx;
	void *buffer = NULL;
	int ret;

	LOG_DBG("tx cb");

	ret = k_msgq_get(&strm->out_queue, &buffer, K_NO_WAIT);
	if (ret == 0) {
		/* transmission complete. free the buffer */
		k_mem_slab_free(strm->cfg.mem_slab, &buffer);
	} else {
		LOG_ERR("no buf in out queue for channel %u", channel);
	}

	switch (strm->state) {
	case I2S_STATE_RUNNING:
		/* get the next buffer from queue */
		if (strm->in_queue.used_msgs == 0) {
			LOG_DBG("tx no more in queue data");
			i2s_tx_stream_disable(dev);
			/* stream is starving */
			strm->stream_starving = true;
			return;
		}
		ret = k_msgq_get(&strm->in_queue, &buffer, K_NO_WAIT);
		if (ret == 0) {
			/* config the DMA */
			uint32_t data_path = dev_data->tx.start_channel;

			strm->dma_block.block_size = strm->cfg.block_size;
			strm->dma_block.dest_address =
				(uint32_t)&base->TDR[data_path];
			strm->dma_block.source_address = (uint32_t)buffer;
			dma_config(dev_data->dev_dma, strm->dma_channel,
				   &strm->dma_cfg);
			ret = k_msgq_put(&strm->out_queue,
					 &buffer, K_NO_WAIT);
			if (ret != 0) {
				LOG_ERR("buffer %p -> out %p err %d",
					buffer, &strm->out_queue, ret);
			}
			dma_start(dev_data->dev_dma, strm->dma_channel);
		}

		if (ret || status) {
		/*
		 * DMA encountered an error (status != 0)
		 * or No bufers in input queue
		 */
			LOG_ERR("DMA status %08x chan %u get ret %d",
				status, channel, ret);
			strm->state = I2S_STATE_READY;
			i2s_tx_stream_disable(dev);
		}

		break;

	case I2S_STATE_STOPPING:
		i2s_tx_stream_disable(dev);
		strm->state = I2S_STATE_READY;
		break;
	}
}

static void i2s_dma_rx_callback(const struct device *dma_dev,
				void *arg, uint32_t channel, int status)
{
	struct device *dev = (struct device *)arg;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->rx;
	void *buffer;
	int ret;

	LOG_DBG("RX cb");
	switch (strm->state) {
	case I2S_STATE_STOPPING:
	case I2S_STATE_RUNNING:
		/* retrieve buffer from input queue */
		ret = k_msgq_get(&strm->in_queue, &buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("get buffer from in_queue %p failed (%d)",
				&strm->in_queue, ret);
		}
		/* put buffer to output queue */
		ret = k_msgq_put(&strm->out_queue, &buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("buffer %p -> out_queue %p err %d",
				buffer,
				&strm->out_queue, ret);
		}
		if (strm->state == I2S_STATE_RUNNING) {
			/* allocate new buffer for next audio frame */
			ret = k_mem_slab_alloc(strm->cfg.mem_slab,
					       &buffer, K_NO_WAIT);
			if (ret != 0) {
				LOG_ERR("buffer alloc from slab %p err %d",
					strm->cfg.mem_slab, ret);
				i2s_rx_stream_disable(dev);
				strm->state = I2S_STATE_ERROR;
			} else {
				uint32_t data_path = strm->start_channel;
				/* put buffer in input queue */
				ret = k_msgq_put(&strm->in_queue,
						 &buffer, K_NO_WAIT);
				if (ret != 0) {
					LOG_ERR("%p -> in_queue %p err %d",
						buffer, &strm->in_queue,
						ret);
				}

				strm->dma_block.block_size =
					strm->cfg.block_size;
				strm->dma_block.dest_address =
					(uint32_t)buffer;
				strm->dma_block.source_address = (uint32_t)
					&base->RDR[data_path];
				dma_config(dev_data->dev_dma,
					   strm->dma_channel,
					   &strm->dma_cfg);
				/* disable rx interrupt and wait for next read*/
				SAI_RxEnableDMA(dev_cfg->base,
					kSAI_FIFORequestDMAEnable, false);
				dma_start(dev_data->dev_dma,
					strm->dma_channel);
			}
		} else {
			/* Received a STOP trigger */
			strm->state = I2S_STATE_READY;
		}
		break;
	case I2S_STATE_ERROR:
		i2s_rx_stream_disable(dev);
		break;
	}
}

static void enable_mclk_direction(const struct device *dev, bool dir)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	const struct device *iomuxgpr_dev = dev_cfg->pinmux_dev;
	uint32_t offset = dev_cfg->mclk_pin_offset;
	uint32_t mask = dev_cfg->mclk_pin_mask;
	uint32_t value = 0;

	if (iomuxgpr_dev == NULL) {
		LOG_ERR("fail to find pinmux controller\n");
		return;
	}

	pinmux_pin_get(iomuxgpr_dev, offset, &value);
	if (dir) {
		/* enable MCLK output */
		value |= mask;
	} else {
		/* disable MCLK output */
		value &= ~mask;
	}
	pinmux_pin_set(iomuxgpr_dev, offset, value);
}

static void get_mclk_rate(const struct device *dev, uint32_t *mclk)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	const struct device *ccm_dev = dev_cfg->ccm_dev;
	clock_control_subsys_t clk_sub_sys = dev_cfg->clk_sub_sys;
	uint32_t rate = 0;

	if (ccm_dev != NULL) {
		clock_control_get_rate(ccm_dev, clk_sub_sys, &rate);
	} else {
		LOG_ERR("CCM driver is not installed");
		*mclk = rate;
		return;
	}
	*mclk = rate;
}

static int i2s_mcux_config(const struct device *dev, enum i2s_dir dir,
			   const struct i2s_config *i2s_cfg)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	sai_transceiver_t config;
	uint32_t mclk;
	/*num_words is frame size*/
	uint8_t num_words = i2s_cfg->channels;
	uint8_t word_size_bits = i2s_cfg->word_size;

	if ((dev_data->tx.state != I2S_STATE_NOT_READY) &&
	    (dev_data->tx.state != I2S_STATE_READY) &&
	    (dev_data->rx.state != I2S_STATE_NOT_READY) &&
	    (dev_data->rx.state != I2S_STATE_READY)) {
		LOG_ERR("invalid state tx(%u) rx(%u)",
			dev_data->tx.state,
			dev_data->rx.state);
		if (dir == I2S_DIR_TX) {
			dev_data->tx.state = I2S_STATE_NOT_READY;
		} else {
			dev_data->rx.state = I2S_STATE_NOT_READY;
		}
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		LOG_ERR("Invalid frame_clk_freq %u",
			i2s_cfg->frame_clk_freq);
		if (dir == I2S_DIR_TX) {
			dev_data->tx.state = I2S_STATE_NOT_READY;
		} else {
			dev_data->rx.state = I2S_STATE_NOT_READY;
		}
		return 0;
	}

	if (word_size_bits < SAI_WORD_SIZE_BITS_MIN ||
	    word_size_bits > SAI_WORD_SIZE_BITS_MAX) {
		LOG_ERR("Unsupported I2S word size %u", word_size_bits);
		if (dir == I2S_DIR_TX) {
			dev_data->tx.state = I2S_STATE_NOT_READY;
		} else {
			dev_data->rx.state = I2S_STATE_NOT_READY;
		}
		return -EINVAL;
	}

	if (num_words < SAI_WORD_PER_FRAME_MIN ||
	    num_words > SAI_WORD_PER_FRAME_MAX) {
		LOG_ERR("Unsupported words length %u", num_words);
		if (dir == I2S_DIR_TX) {
			dev_data->tx.state = I2S_STATE_NOT_READY;
		} else {
			dev_data->rx.state = I2S_STATE_NOT_READY;
		}
		return -EINVAL;
	}

	if ((i2s_cfg->options & I2S_OPT_PINGPONG) == I2S_OPT_PINGPONG) {
		LOG_ERR("Ping-pong mode not supported");
		if (dir == I2S_DIR_TX) {
			dev_data->tx.state = I2S_STATE_NOT_READY;
		} else {
			dev_data->rx.state = I2S_STATE_NOT_READY;
		}
		return -ENOTSUP;
	}

	memset(&config, 0, sizeof(config));

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) {
		if (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
			config.masterSlave = kSAI_Slave;
		} else {
			config.masterSlave =
				kSAI_Bclk_Master_FrameSync_Slave;
		}
	} else {
		if (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
			config.masterSlave =
				kSAI_Bclk_Slave_FrameSync_Master;
		} else {
			config.masterSlave = kSAI_Master;
		}
	}

	enable_mclk_direction(dev,
			      i2s_cfg->options|I2S_OPT_BIT_CLK_MASTER);
	get_mclk_rate(dev, &mclk);
	LOG_DBG("mclk is %d", mclk);

	/* sync mode configurations */
	if (dir == I2S_DIR_TX) {
		/* TX */
		if (dev_cfg->tx_sync_mode) {
			config.syncMode = kSAI_ModeSync;
		} else {
			config.syncMode = kSAI_ModeAsync;
		}
	} else {
		/* RX */
		if (dev_cfg->rx_sync_mode) {
			config.syncMode = kSAI_ModeSync;
		} else {
			config.syncMode = kSAI_ModeAsync;
		}
	}

	config.frameSync.frameSyncPolarity = kSAI_PolarityActiveLow;
	config.bitClock.bclkSrcSwap = false;
	/* clock signal polarity */
	switch (i2s_cfg->format & I2S_FMT_CLK_FORMAT_MASK) {
	case I2S_FMT_CLK_NF_NB:
		config.frameSync.frameSyncPolarity =
			kSAI_PolarityActiveLow;
		config.bitClock.bclkSrcSwap = false;
		break;

	case I2S_FMT_CLK_NF_IB:
		config.frameSync.frameSyncPolarity =
			kSAI_PolarityActiveLow;
		config.bitClock.bclkSrcSwap = true;
		break;

	case I2S_FMT_CLK_IF_NB:
		config.frameSync.frameSyncPolarity =
			kSAI_PolarityActiveHigh;
		config.bitClock.bclkSrcSwap = false;
		break;

	case I2S_FMT_CLK_IF_IB:
		config.frameSync.frameSyncPolarity =
			kSAI_PolarityActiveHigh;
		config.bitClock.bclkSrcSwap = true;
		break;
	}
	/* bit clock source is MCLK */
	config.bitClock.bclkSource = kSAI_BclkSourceMclkDiv;
	/*
	 * additional settings for bclk
	 * read the SDK header file for more details
	 */
	config.bitClock.bclkInputDelay = false;

	/* frame sync default configurations */
#if defined(FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND) &&		\
	FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND
	config.frameSync.frameSyncGenerateOnDemand = false;
#endif

	config.frameSync.frameSyncWidth = (uint8_t)word_size_bits;

	/* serial data default configurations */
#if defined(FSL_FEATURE_SAI_HAS_CHANNEL_MODE) &&			\
	FSL_FEATURE_SAI_HAS_CHANNEL_MODE
	config.serialData.dataMode = kSAI_DataPinStateOutputZero;
#endif

	/* format */
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		SAI_GetClassicI2SConfig(&config, word_size_bits,
					kSAI_Stereo,
					dev_cfg->tx_channel);
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		SAI_GetLeftJustifiedConfig(&config, word_size_bits,
					   kSAI_Stereo,
					   dev_cfg->tx_channel);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		SAI_GetDSPConfig(&config, kSAI_FrameSyncLenOneBitClk,
				 word_size_bits, kSAI_Stereo,
				 dev_cfg->tx_channel);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		SAI_GetTDMConfig(&config, kSAI_FrameSyncLenPerWordWidth,
				 word_size_bits, num_words,
				 dev_cfg->tx_channel);
		break;
	default:
		LOG_ERR("Unsupported I2S data format");
		if (dir == I2S_DIR_TX) {
			dev_data->tx.state = I2S_STATE_NOT_READY;
		} else {
			dev_data->rx.state = I2S_STATE_NOT_READY;
		}
		return -EINVAL;
	}

	if (dir == I2S_DIR_TX) {
		memcpy(&dev_data->tx.cfg, i2s_cfg, sizeof(struct i2s_config));
		LOG_DBG("tx slab free_list = 0x%x",
			(uint32_t)i2s_cfg->mem_slab->free_list);
		LOG_DBG("tx slab num_blocks = %d",
			(uint32_t)i2s_cfg->mem_slab->num_blocks);
		LOG_DBG("tx slab block_size = %d",
			(uint32_t)i2s_cfg->mem_slab->block_size);
		LOG_DBG("tx slab buffer = 0x%x",
			(uint32_t)i2s_cfg->mem_slab->buffer);

		/* set bit clock divider */
		SAI_TxSetConfig(base, &config);
		dev_data->tx.start_channel = config.startChannel;
		/* Disable the channel FIFO */
		base->TCR3 &= ~I2S_TCR3_TCE_MASK;
		SAI_TxSetBitClockRate(base, mclk,
				      i2s_cfg->frame_clk_freq,
				      word_size_bits,
				      i2s_cfg->channels);
		LOG_DBG("tx start_channel = %d", dev_data->tx.start_channel);
		/*set up dma settings*/
		dev_data->tx.dma_cfg.source_data_size = 1;
		dev_data->tx.dma_cfg.dest_data_size = 1;
		dev_data->tx.dma_cfg.source_burst_length =
			i2s_cfg->word_size / 8;
		dev_data->tx.dma_cfg.dest_burst_length =
			i2s_cfg->word_size / 8;
		dev_data->tx.dma_cfg.user_data = (void *)dev;
		dev_data->tx.state = I2S_STATE_READY;
	} else {
		memcpy(&dev_data->rx.cfg, i2s_cfg, sizeof(struct i2s_config));
		LOG_DBG("rx slab free_list = 0x%x",
			(uint32_t)i2s_cfg->mem_slab->free_list);
		LOG_DBG("rx slab num_blocks = %d",
			(uint32_t)i2s_cfg->mem_slab->num_blocks);
		LOG_DBG("rx slab block_size = %d",
			(uint32_t)i2s_cfg->mem_slab->block_size);
		LOG_DBG("rx slab buffer = 0x%x",
			(uint32_t)i2s_cfg->mem_slab->buffer);

		/* set bit clock divider */
		SAI_RxSetConfig(base, &config);
		dev_data->rx.start_channel = config.startChannel;
		SAI_RxSetBitClockRate(base, mclk,
				      i2s_cfg->frame_clk_freq,
				      word_size_bits,
				      i2s_cfg->channels);
		LOG_DBG("rx start_channel = %d", dev_data->rx.start_channel);
		/*set up dma settings*/
		dev_data->rx.dma_cfg.source_data_size = 1;
		dev_data->rx.dma_cfg.dest_data_size = 1;
		dev_data->rx.dma_cfg.source_burst_length =
			i2s_cfg->word_size / 8;
		dev_data->rx.dma_cfg.dest_burst_length =
			i2s_cfg->word_size / 8;
		dev_data->rx.dma_cfg.user_data = (void *)dev;
		dev_data->rx.state = I2S_STATE_READY;
	}

	return 0;
}

const struct i2s_config *i2s_mcux_config_get(const struct device *dev,
					     enum i2s_dir dir)
{
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;

	if (dir == I2S_DIR_RX)
		return &dev_data->rx.cfg;

	return &dev_data->tx.cfg;
}

static int i2s_tx_stream_start(const struct device *dev)
{
	int ret = 0;
	void *buffer;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->tx;
	uint32_t data_path = strm->start_channel;
	const struct device *dev_dma = dev_data->dev_dma;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;

	/* retrieve buffer from input queue */
	ret = k_msgq_get(&strm->in_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("No buffer in input queue to start");
		return -EIO;
	}

	LOG_DBG("tx stream start");

	strm->dma_block.block_size = strm->cfg.block_size;
	strm->dma_block.dest_address =
		(uint32_t)&base->RDR[data_path];
	strm->dma_block.source_address = (uint32_t)buffer;

	dma_config(dev_dma, strm->dma_channel, &strm->dma_cfg);

	/* put buffer in output queue */
	ret = k_msgq_put(&strm->out_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to put buffer in output queue");
		return ret;
	}

	ret = dma_start(dev_dma, strm->dma_channel);
	if (ret < 0) {
		LOG_ERR("dma_start failed (%d)", ret);
		return ret;
	}

	/* Enable DMA enable bit */
	SAI_TxEnableDMA(dev_cfg->base, kSAI_FIFORequestDMAEnable, true);

	/* Enable SAI Tx clock */
	SAI_TxEnable(dev_cfg->base, true);

	/* Enable the channel FIFO */
	dev_cfg->base->TCR3 |= I2S_TCR3_TCE(1UL << strm->start_channel);

	strm->stream_starving = false;

	return 0;
}

static int i2s_rx_stream_start(const struct device *dev)
{
	int ret = 0;
	void *buffer;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->rx;
	const struct device *dev_dma = dev_data->dev_dma;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;
	uint32_t data_path = strm->start_channel;

	/* allocate receive buffer from SLAB */
	ret = k_mem_slab_alloc(strm->cfg.mem_slab, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_DBG("buffer alloc from mem_slab failed (%d)", ret);
		return ret;
	}

	strm->dma_block.block_size = strm->cfg.block_size;
	strm->dma_block.dest_address = (uint32_t)buffer;
	strm->dma_block.source_address =
		(uint32_t)&base->RDR[data_path];

	dma_config(dev_dma, strm->dma_channel, &strm->dma_cfg);
	/* put buffer in input queue */
	ret = k_msgq_put(&strm->in_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to put buffer in output queue");
		return ret;
	}

	LOG_DBG("Starting DMA Ch%u", strm->dma_channel);
	ret = dma_start(dev_dma, strm->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA Ch%d (%d)", strm->dma_channel,
			ret);
		return ret;
	}

	/* Enable DMA enable bit */
	SAI_RxEnableDMA(dev_cfg->base, kSAI_FIFORequestDMAEnable, true);

	/* Enable the channel FIFO */
	dev_cfg->base->RCR3 |= I2S_RCR3_RCE(1UL << strm->start_channel);

	/* Enable SAI Rx clock */
	SAI_RxEnable(dev_cfg->base, true);

	strm->stream_starving = false;

	return 0;
}

static int i2s_mcux_trigger(const struct device *dev, enum i2s_dir dir,
			    enum i2s_trigger_cmd cmd)
{
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm;
	unsigned int key;
	int ret = 0;

	if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	}

	strm = (dir == I2S_DIR_TX) ? &dev_data->tx : &dev_data->rx;

	key = irq_lock();
	switch (cmd) {
	case I2S_TRIGGER_START:
		if (strm->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %u",
				strm->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			ret = i2s_tx_stream_start(dev);
		} else {
			ret = i2s_rx_stream_start(dev);
		}

		if (ret < 0) {
			LOG_DBG("START trigger failed %d", ret);
			ret = -EIO;
			break;
		}

		strm->state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_DROP:
		if (strm->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state %d",
				strm->state);
			ret = -EIO;
			break;
		}
		if (strm->state == I2S_STATE_READY) {
			/* nothing to do in this state*/
			break;
		}
		if (dir == I2S_DIR_TX) {
			i2s_tx_stream_disable(dev);
		} else {
			i2s_rx_stream_disable(dev);
		}
		strm->state = I2S_STATE_READY;
		break;
	case I2S_TRIGGER_DRAIN:
	case I2S_TRIGGER_STOP:
		if (strm->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN/STOP trigger: invalid state %d",
				strm->state);
			ret = -EIO;
			break;
		}
		if (dir == I2S_DIR_TX && strm->in_queue.used_msgs == 0) {
			/* if the transmission is done, then go ready*/
			strm->state = I2S_STATE_READY;
		} else {
			strm->state = I2S_STATE_STOPPING;
		}
		break;

	case I2S_TRIGGER_PREPARE:
		if (strm->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state %d",
				strm->state);
			ret = -EIO;
			break;
		}
		strm->state = I2S_STATE_READY;
		if (dir == I2S_DIR_TX) {
			i2s_tx_stream_disable(dev);
		} else {
			i2s_rx_stream_disable(dev);
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
	const struct i2s_mcux_config *dev_cfg = dev->config;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->rx;
	void *buffer;
	int ret = 0;

	LOG_DBG("i2s_mcux_read");
	if (strm->state == I2S_STATE_NOT_READY) {
		LOG_ERR("invalid state %d", strm->state);
		return -EIO;
	}

	ret = k_msgq_get(&strm->out_queue, &buffer,
			 SYS_TIMEOUT_MS(strm->cfg.timeout));
	if (ret != 0) {
		LOG_DBG("need retry");
		return -EAGAIN;
	}

	switch (strm->state) {
	case I2S_STATE_STOPPING:
		i2s_rx_stream_disable(dev);
		strm->state = I2S_STATE_READY;
		break;
	case I2S_STATE_RUNNING:
		SAI_RxEnableDMA(dev_cfg->base,
				kSAI_FIFORequestDMAEnable, true);
		break;
	default:
	case I2S_STATE_READY:
		break;
	}
	*mem_block = buffer;
	*size = strm->cfg.block_size;
	return 0;
}

static int i2s_mcux_write(const struct device *dev, void *mem_block,
			  size_t size)
{
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	struct stream *strm = &dev_data->tx;
	int ret;

	LOG_DBG("i2s_mcux_write");
	if (strm->state != I2S_STATE_RUNNING &&
	    strm->state != I2S_STATE_READY) {
		LOG_ERR("invalid state (%d)", strm->state);
		return -EIO;
	}

	ret = k_msgq_put(&strm->in_queue, &mem_block,
			 SYS_TIMEOUT_MS(strm->cfg.timeout));
	if (ret) {
		LOG_ERR("k_msgq_put failed %d", ret);
		return ret;
	}

	if (strm->state == I2S_STATE_RUNNING && strm->stream_starving) {
		/* in running state if the stream is off turn it on*/
		ret = i2s_tx_stream_start(dev);
		strm->stream_starving = false;
	}

	return ret;
}

static void sai_driver_irq(const struct device *dev)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;

	if ((base->TCSR & I2S_TCSR_FEF_MASK) == I2S_TCSR_FEF_MASK) {
		/* Clear FIFO error flag to continue transfer */
		SAI_TxClearStatusFlags(base, I2S_TCSR_FEF_MASK);

		/* Reset FIFO for safety */
		SAI_TxSoftwareReset(base, kSAI_ResetTypeFIFO);

		LOG_DBG("sai tx error occurred");
	}

	if ((base->RCSR & I2S_RCSR_FEF_MASK) == I2S_RCSR_FEF_MASK) {
		/* Clear FIFO error flag to continue transfer */
		SAI_RxClearStatusFlags(base, I2S_RCSR_FEF_MASK);

		/* Reset FIFO for safety */
		SAI_RxSoftwareReset(base, kSAI_ResetTypeFIFO);

		LOG_DBG("sai rx error occurred");
	}
}

/* clear IRQ sources atm */
static void i2s_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;

	if ((base->RCSR & I2S_TCSR_FEF_MASK) == I2S_TCSR_FEF_MASK) {
		sai_driver_irq(dev);
	}

	if ((base->TCSR & I2S_RCSR_FEF_MASK) == I2S_RCSR_FEF_MASK) {
		sai_driver_irq(dev);
	}
	/*
	 * Add for ARM errata 838869, affects Cortex-M4,
	 * Cortex-M4F Store immediate overlapping exception return operation
	 *  might vector to incorrect interrupt
	 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	__DSB();
#endif
}

static void audio_clock_settings(const struct device *dev)
{
	clock_audio_pll_config_t audioPllConfig;
	const struct i2s_mcux_config *dev_cfg = dev->config;

	/*Clock setting for SAI*/
#if CONFIG_CLOCK_CONTROL_MCUX_CCM_REV2
	switch (dev_cfg->clk_sub_sys) {
	case IMX_CCM_SAI1_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai1, dev_cfg->clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai1, dev_cfg->clk_src_div);
		break;
	case IMX_CCM_SAI2_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai2, dev_cfg->clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai2, dev_cfg->clk_src_div);
		break;
	case IMX_CCM_SAI3_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai3, dev_cfg->clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai3, dev_cfg->clk_src_div);
		break;
	case IMX_CCM_SAI4_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai4, dev_cfg->clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai4, dev_cfg->clk_src_div);
		break;
	default:
		LOG_ERR("wrong clock system configured");
		return;
	}
#endif

#if CONFIG_CLOCK_CONTROL_MCUX_CCM
	switch (dev_cfg->clk_sub_sys) {
	case IMX_CCM_SAI1_CLK:
		CLOCK_SetMux(kCLOCK_Sai1Mux, dev_cfg->clk_src);
		CLOCK_SetDiv(kCLOCK_Sai1PreDiv, dev_cfg->clk_pre_div);
		CLOCK_SetDiv(kCLOCK_Sai1Div, dev_cfg->clk_src_div);
		break;
	case IMX_CCM_SAI2_CLK:
		CLOCK_SetMux(kCLOCK_Sai2Mux, dev_cfg->clk_src);
		CLOCK_SetDiv(kCLOCK_Sai2PreDiv, dev_cfg->clk_pre_div);
		CLOCK_SetDiv(kCLOCK_Sai2Div, dev_cfg->clk_src_div);
		break;
	case IMX_CCM_SAI3_CLK:
		CLOCK_SetMux(kCLOCK_Sai2Mux, dev_cfg->clk_src);
		CLOCK_SetDiv(kCLOCK_Sai2PreDiv, dev_cfg->clk_pre_div);
		CLOCK_SetDiv(kCLOCK_Sai2Div, dev_cfg->clk_src_div);
		break;
	default:
		LOG_ERR("wrong clock system configured");
		return;
	}
#endif

	audioPllConfig.loopDivider = dev_cfg->pll_lp;
	audioPllConfig.postDivider = dev_cfg->pll_pd;
	audioPllConfig.numerator = dev_cfg->pll_num;
	audioPllConfig.denominator = dev_cfg->pll_den;
	audioPllConfig.src = dev_cfg->pll_src;

	CLOCK_InitAudioPll(&audioPllConfig);
}

static int i2s_mcux_initialize(const struct device *dev)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = (I2S_Type *)dev_cfg->base;
	struct i2s_dev_data *dev_data = (struct i2s_dev_data *)dev->data;
	uint32_t mclk;

	if (!dev_data->dev_dma) {
		LOG_ERR("DMA device not found");
		return -ENODEV;
	}

	/* Initialize the buffer queues */
	k_msgq_init(&dev_data->tx.in_queue, (char *)dev_data->tx.in_msgs,
		    sizeof(void *), CONFIG_I2S_TX_BLOCK_COUNT);
	k_msgq_init(&dev_data->rx.in_queue, (char *)dev_data->rx.in_msgs,
		    sizeof(void *), CONFIG_I2S_RX_BLOCK_COUNT);
	k_msgq_init(&dev_data->tx.out_queue, (char *)dev_data->tx.out_msgs,
		    sizeof(void *), CONFIG_I2S_TX_BLOCK_COUNT);
	k_msgq_init(&dev_data->rx.out_queue, (char *)dev_data->rx.out_msgs,
		    sizeof(void *), CONFIG_I2S_RX_BLOCK_COUNT);

	/* register ISR */
	dev_cfg->irq_connect(dev);

	/*clock configuration*/
	audio_clock_settings(dev);

	SAI_Init(base);

	dev_data->tx.state = I2S_STATE_NOT_READY;
	dev_data->rx.state = I2S_STATE_NOT_READY;

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) ||	\
	(defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) &&		\
	 (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
	sai_master_clock_t mclkConfig = {
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
		.mclkOutputEnable = true,
#if !(defined(FSL_FEATURE_SAI_HAS_NO_MCR_MICS) &&			\
	(FSL_FEATURE_SAI_HAS_NO_MCR_MICS))
		.mclkSource = kSAI_MclkSourceSysclk,
#endif
#endif
	};
#endif

	get_mclk_rate(dev, &mclk);
/* master clock configurations */
#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) ||	\
	(defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) &&		\
	 (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) &&			\
	(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
	mclkConfig.mclkHz = mclk;
	mclkConfig.mclkSourceClkHz = mclk;
#endif
	SAI_SetMasterClockConfig(base, &mclkConfig);
#endif

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct i2s_driver_api i2s_mcux_driver_api = {
	.configure = i2s_mcux_config,
	.read = i2s_mcux_read,
	.write = i2s_mcux_write,
	.config_get = i2s_mcux_config_get,
	.trigger = i2s_mcux_trigger,
};

#define I2S_MCUX_INIT(i2s_id)						\
	static void i2s_irq_connect_##i2s_id(const struct device *dev); \
									\
	static const struct i2s_mcux_config i2s_##i2s_id##_config = {	\
		.base = (I2S_Type *)DT_INST_REG_ADDR(i2s_id),		\
		.clk_src =						\
			DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(i2s_id),	\
				0, bits),				\
		.clk_pre_div = DT_INST_PROP(i2s_id, pre_div);		\
		.clk_src_div = DT_INST_PROP(i2s_id, podf);		\
		.pll_src =						\
			DT_PHA_BY_NAME(DT_DRV_INST(i2s_id),		\
				pll_clocks, src, value),		\
		.pll_lp =						\
			DT_PHA_BY_NAME(DT_DRV_INST(i2s_id),		\
				pll_clocks, lp, value),			\
		.pll_pd =						\
			DT_PHA_BY_NAME(DT_DRV_INST(i2s_id),		\
				pll_clocks, pd, value),			\
		.pll_num =						\
			DT_PHA_BY_NAME(DT_DRV_INST(i2s_id),		\
				pll_clocks, num, value),		\
		.pll_den =						\
			DT_PHA_BY_NAME(DT_DRV_INST(i2s_id),		\
				pll_clocks, den, value),		\
		.mclk_pin_mask =					\
			DT_PHA_BY_IDX(DT_DRV_INST(i2s_id),		\
				pinmuxes, 0, function),			\
		.mclk_pin_offset =					\
			DT_PHA_BY_IDX(DT_DRV_INST(i2s_id),		\
				pinmuxs, 0, pin),			\
		.clk_sub_sys =	(clock_control_subsys_t)		\
			DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(i2s_id),	\
				0, name),				\
		.ccm_dev = DEVICE_DT_GET(				\
				DT_CLOCKS_CTLR(DT_DRV_INST(i2s_id))),	\
		.pinmux_dev = DEVICE_DT_GET(				\
				DT_PHANDLE_BY_IDX(DT_DRV_INST(i2s_id),	\
				pinmuxes, 0)),				\
		.irq_connect = i2s_irq_connect_##i2s_id,		\
		.tx_sync_mode =						\
			   DT_INST_PROP(i2s_id, nxp_tx_sync_mode),	\
		.rx_sync_mode =						\
			   DT_INST_PROP(i2s_id, nxp_rx_sync_mode),	\
		.tx_channel = DT_INST_PROP(i2s_id, nxp_tx_channel),	\
	};								\
									\
	static struct i2s_dev_data i2s_##i2s_id##_data = {		\
		.dev_dma = DEVICE_DT_GET(				\
				DT_INST_DMAS_CTLR_BY_NAME(i2s_id, rx)), \
		.tx = {							\
			.dma_channel =					\
			  DT_INST_PROP(i2s_id, nxp_tx_dma_channel),	\
			.dma_cfg = {					\
			  .source_burst_length =			\
				CONFIG_I2S_EDMA_BURST_SIZE,		\
			  .dest_burst_length =				\
				CONFIG_I2S_EDMA_BURST_SIZE,		\
			  .dma_callback = i2s_dma_tx_callback,		\
			  .complete_callback_en = 1,			\
			  .error_callback_en = 1,			\
			  .block_count = 1,				\
			  .head_block =					\
				&i2s_##i2s_id##_data.tx.dma_block,	\
			  .channel_direction = MEMORY_TO_PERIPHERAL,	\
			  .dma_slot =					\
			    DT_INST_DMAS_CELL_BY_NAME(i2s_id,		\
						      tx, source),	\
			},						\
		},							\
		.rx = {							\
			.dma_channel =					\
			  DT_INST_PROP(i2s_id, nxp_rx_dma_channel),	\
			.dma_cfg = {					\
			  .source_burst_length =			\
				CONFIG_I2S_EDMA_BURST_SIZE,		\
			  .dest_burst_length =				\
				CONFIG_I2S_EDMA_BURST_SIZE,		\
			  .dma_callback = i2s_dma_rx_callback,		\
			  .complete_callback_en = 1,			\
			  .error_callback_en = 1,			\
			  .block_count = 1,				\
			  .head_block =					\
				&i2s_##i2s_id##_data.rx.dma_block,	\
			  .channel_direction = PERIPHERAL_TO_MEMORY,	\
			  .dma_slot =					\
			    DT_INST_DMAS_CELL_BY_NAME(i2s_id,		\
						      rx, source),	\
			},						\
		},							\
	};								\
									\
	DEVICE_DT_INST_DEFINE(i2s_id, &i2s_mcux_initialize, NULL,	\
		    &i2s_##i2s_id##_data, &i2s_##i2s_id##_config,	\
		    POST_KERNEL,					\
		    CONFIG_I2S_INIT_PRIORITY, &i2s_mcux_driver_api);	\
									\
	static void i2s_irq_connect_##i2s_id(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(i2s_id, 0, irq),		\
			DT_INST_IRQ_BY_IDX(i2s_id, 0, priority),	\
			i2s_mcux_isr,					\
			DEVICE_DT_INST_GET(i2s_id), 0);			\
		irq_enable(DT_INST_IRQN(i2s_id));			\
	}

DT_INST_FOREACH_STATUS_OKAY(I2S_MCUX_INIT)
