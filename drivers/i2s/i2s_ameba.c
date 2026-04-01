/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_i2s

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>
#include "ameba_audio_clock.h"
#include <string.h>

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include "i2s_ameba.h"

#include "dma_ameba_gdma.h"
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_ameba);

#define NUM_DMA_BLOCKS_RX_PREP 2
#define MAX_TX_DMA_BLOCKS      1
#define I2S_AMEBA_DEBUG       0

static const struct i2s_config *i2s_ameba_config_get(const struct device *dev, enum i2s_dir dir);
static void i2s_ameba_tx_sport_fifo_empty_irq(const struct device *dev, bool enable);

static inline void i2s_purge_stream_buffers(struct i2s_ameba_stream *stream,
					    struct k_mem_slab *mem_slab,
					    bool in_drop, bool out_drop)
{
	void *buffer;

	if (in_drop) {
		while (k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT) == 0) {
			k_mem_slab_free(mem_slab, buffer);
		}
	}

	if (out_drop) {
		while (k_msgq_get(&stream->out_queue, &buffer, K_NO_WAIT) == 0) {
			k_mem_slab_free(mem_slab, buffer);
		}
	}
}

uint32_t i2s_ameba_tx_fifo_empty_handler(struct device *dev)
{
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	AUDIO_SPORT_TypeDef *SPORTx = cfg->i2s;

	if (data->fifo_num == 0 || data->fifo_num == 1) {
		if (SPORTx->SP_FIFO_CTRL & SP_BIT_TX_FIFO_EMPTY_INTR_0) {
			/* Clear TX FIFO0 irq. */
			SPORTx->SP_INT_CTRL |= SP_INTR_CLR_0(2);
			/* FIFO0 empty. */
			goto sport_end;
		}
	} else if (data->fifo_num == 2 || data->fifo_num == 3) {
		if ((SPORTx->SP_FIFO_CTRL & SP_BIT_TX_FIFO_EMPTY_INTR_0) &&
		    ((SPORTx->SP_FIFO_CTRL & SP_BIT_TX_FIFO_EMPTY_INTR_1))) {
			/* Clear TX FIFO0 and FIFO1 irq. */
			SPORTx->SP_INT_CTRL |= SP_INTR_CLR_0(2);
			SPORTx->SP_INT_CTRL |= SP_INTR_CLR_1(2);
			/* FIFO0 and FIFO1 are both empty. */
			goto sport_end;
		} /* else: wait for another irq end. */
	}
	return 0;

sport_end:
	i2s_ameba_tx_sport_fifo_empty_irq(dev, 0);
	if (stream->state == I2S_STATE_STOPPING) {
		LOG_DBG("I2S TX FIFO is empty. Deinit AUDIO SPORT.");
		stream->state = I2S_STATE_READY;
		AUDIO_SP_Deinit(cfg->index, SP_DIR_TX);
	} else {
		/* If state == RUNNING, it means i2s TX underrun. */
		LOG_ERR("I2S FIFO empty with an error state %d.", stream->state);
		stream->state = I2S_STATE_ERROR;
	}
	return 0;
}

static void i2s_ameba_tx_sport_fifo_empty_irq(const struct device *dev, bool enable)
{
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_ameba_data *data = dev->data;
	AUDIO_SPORT_TypeDef *SPORTx = cfg->i2s;

	if (data->fifo_num == 2 || data->fifo_num == 3) {
		if (enable) {
			SPORTx->SP_INT_CTRL |= SP_INT_ENABLE_DSP_1(BIT(4));
		} else {
			SPORTx->SP_INT_CTRL &= ~SP_INT_ENABLE_DSP_1(BIT(4));
		}
	}

	if (enable) {
		irq_connect_dynamic(cfg->irq, 0, (void *)i2s_ameba_tx_fifo_empty_handler,
				    (void *)dev, 0);
		SPORTx->SP_INT_CTRL |= SP_INT_ENABLE_DSP_0(BIT(4));
		irq_enable(cfg->irq);
	} else {
		SPORTx->SP_INT_CTRL &= ~SP_INT_ENABLE_DSP_0(BIT(4));
		irq_disable(cfg->irq);
		irq_connect_dynamic(cfg->irq, 0, NULL, NULL, 0);
	}
}

static void i2s_tx_stream_disable(const struct device *dev, bool drop)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	const struct i2s_ameba_cfg *cfg = dev->config;

	if (dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel) < 0) {
		LOG_ERR("Stop tx dma failed !!");
	}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	if ((data->fifo_num == 2 || data->fifo_num == 3) &&
	    (dma_stop(data->dma_tx_ext.dma_dev, data->dma_tx_ext.dma_channel) < 0)) {
		LOG_ERR("Stop tx ext dma failed !!");
	}
#endif

	AUDIO_SP_DmaCmd(cfg->index, DISABLE);

	if (drop) {
		AUDIO_SP_Deinit(cfg->index, SP_DIR_TX);
		/* purge buffers queued in the stream */
		i2s_purge_stream_buffers(stream, data->tx.cfg.mem_slab, true, true);
		stream->state = I2S_STATE_READY;
	} else {
		LOG_DBG("DMA DONE. Wait SPORT FIFO-EMPTY.");
		i2s_ameba_tx_sport_fifo_empty_irq(dev, 1);
	}
}

static void i2s_rx_stream_disable(const struct device *dev, bool in_drop, bool out_drop)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->rx;
	const struct i2s_ameba_cfg *cfg = dev->config;

	if (dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel) < 0) {
		LOG_ERR("Stop rx dma failed !!");
	}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	if ((data->fifo_num == 2 || data->fifo_num == 3) &&
	    (dma_stop(data->dma_rx_ext.dma_dev, data->dma_rx_ext.dma_channel) < 0)) {
		LOG_ERR("Stop rx ext dma failed !!");
	}
#endif

	AUDIO_SP_DmaCmd(cfg->index, DISABLE);
	AUDIO_SP_Deinit(cfg->index, SP_DIR_RX);

	/* purge buffers queued in the stream */
	if (in_drop || out_drop) {
		i2s_purge_stream_buffers(stream, data->rx.cfg.mem_slab, in_drop, out_drop);
	}
}

static int i2s_ameba_tx_dequeue_next_buffer(const struct device *dev, uint8_t *blocks_queued)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	void *buffer = NULL;
	int ret = -EINVAL;
	unsigned int key;

	*blocks_queued = 0;

	key = irq_lock();

	/* queue additional blocks to DMA if in_queue and DMA has free blocks */
	while (stream->free_tx_dma_blocks) {
		/* get the next buffer from queue */
		ret = k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT);
		if (ret) {
			/* in_queue is empty, no more blocks to send to DMA */
			break;
		}

		(stream->free_tx_dma_blocks)--;

		ret = k_msgq_put(&stream->out_queue, &buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("buffer %p -> out %p err %d", buffer, &stream->out_queue, ret);
			break;
		}

		(*blocks_queued)++;
	}

	irq_unlock(key);

	if ((ret == 0) && (*blocks_queued)) {
		ret = (int)buffer;
	} else {
		ret = -EIO;
	}
	return ret;
}

static void i2s_ameba_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			 int status)
{
	struct device *dev = user_data;
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	const struct i2s_ameba_cfg *cfg = dev->config;
	void *buffer = NULL;
	int ret;
	uint8_t blocks_queued;

	if (status < 0) {
		LOG_ERR("DMA tx is in error state.");
		return;
	}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	int length = stream->cfg.block_size;
	int upper_block_size = (data->fifo_num == 2) ? (length * 2 / 3) : (length / 2);
	int bottom_block_size = (data->fifo_num == 2) ? (length / 3) : (length / 2);

	data->dma_sw_irq |= BIT(channel);
	if (IS_REORDER_CH(data->reorder_mode) &&
	    !((data->dma_sw_irq & BIT(data->dma_tx.dma_channel)) &&
	      (data->dma_sw_irq) & BIT(data->dma_tx_ext.dma_channel))) {
		LOG_DBG("TX wait for the pair dma done. (sw irq: %08x)", data->dma_sw_irq);
		return;
	}
	data->dma_sw_irq &= ~(BIT(data->dma_tx.dma_channel) | BIT(data->dma_tx_ext.dma_channel));
#endif

	ret = k_msgq_get(&stream->out_queue, &buffer, K_NO_WAIT);
	if (ret == 0) {
		/* transmission complete. free the buffer */
		k_mem_slab_free(stream->cfg.mem_slab, buffer);
		(stream->free_tx_dma_blocks)++;
	} else {
		LOG_ERR("no buf in out_queue");
		i2s_tx_stream_disable(dev, false);
		return;
	}

	if (stream->free_tx_dma_blocks > MAX_TX_DMA_BLOCKS) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("free_tx_dma_blocks exceeded maximum, now %d", stream->free_tx_dma_blocks);
		i2s_tx_stream_disable(dev, false);
		return;
	}

	/* Received a STOP trigger, terminate TX immediately */
	if (stream->last_block) {
		LOG_DBG("TX STOPPED last_block set");
		i2s_tx_stream_disable(dev, false);
		return;
	}

	if (ret) {
		/* k_msgq_get() returned error, and was not last_block */
		i2s_tx_stream_disable(dev, false);
		return;
	}

	switch (stream->state) {
	case I2S_STATE_STOPPING:
	case I2S_STATE_RUNNING:
		ret = i2s_ameba_tx_dequeue_next_buffer(dev, &blocks_queued);
		if (ret < 0) {
			i2s_tx_stream_disable(dev, false);
			return;
		}
		buffer = (void *)ret;
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
		if (IS_REORDER_CH(data->reorder_mode)) {
			sys_cache_data_flush_range(buffer, stream->cfg.block_size);
			dma_reload(data->dma_tx.dma_dev, data->dma_tx.dma_channel, (uint32_t)buffer,
				   (uint32_t)&cfg->i2s->SP_TX_FIFO_0_WR_ADDR, upper_block_size);
			/* dma_start */
			dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
			dma_reload(data->dma_tx_ext.dma_dev, data->dma_tx_ext.dma_channel,
				   (uint32_t)buffer + upper_block_size,
				   (uint32_t)&cfg->i2s->SP_TX_FIFO_1_WR_ADDR, bottom_block_size);
			/* dma_start */
			dma_start(data->dma_tx_ext.dma_dev, data->dma_tx_ext.dma_channel);
		} else {
			sys_cache_data_flush_range(buffer, stream->cfg.block_size);
			dma_reload(data->dma_tx.dma_dev, data->dma_tx.dma_channel,
				   (uint32_t)buffer,
				   (uint32_t)&cfg->i2s->SP_TX_FIFO_0_WR_ADDR,
				   stream->cfg.block_size);
			/* dma_start */
			dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
		}
#else
		sys_cache_data_flush_range(buffer, stream->cfg.block_size);
		dma_reload(data->dma_tx.dma_dev, data->dma_tx.dma_channel, (uint32_t)buffer,
				(uint32_t)&cfg->i2s->SP_TX_FIFO_0_WR_ADDR, stream->cfg.block_size);
		/* dma_start */
		dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
#endif

		if (blocks_queued || (stream->free_tx_dma_blocks < MAX_TX_DMA_BLOCKS)) {
		} else {
			i2s_tx_stream_disable(dev, false);
		}
		return;
	case I2S_STATE_ERROR:
		LOG_ERR("Error state in tx callback.");
	default:
		i2s_tx_stream_disable(dev, true);
		return;
	}
}

static void i2s_ameba_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			 int status)
{
	struct device *dev = user_data;
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->rx;
	const struct i2s_ameba_cfg *cfg = dev->config;
	void *buffer;
	int ret = 0;

	if (status < 0) {
		LOG_ERR("DMA rx is in error state.");
		return;
	}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	int length = stream->cfg.block_size;
	int upper_block_size = (data->fifo_num == 2) ? (length * 2 / 3) : (length / 2);
	int bottom_block_size = (data->fifo_num == 2) ? (length / 3) : (length / 2);

	data->dma_sw_irq |= BIT(channel);

	if (IS_REORDER_CH(data->reorder_mode) &&
	    !((data->dma_sw_irq & BIT(data->dma_rx.dma_channel)) &&
	      (data->dma_sw_irq) & BIT(data->dma_rx_ext.dma_channel))) {
		LOG_DBG("RX wait for the pair dma done. (sw irq: %08x)", data->dma_sw_irq);
		return;
	}
	data->dma_sw_irq &= ~(BIT(data->dma_rx.dma_channel) | BIT(data->dma_rx_ext.dma_channel));
#endif

	switch (stream->state) {
	case I2S_STATE_STOPPING:
	case I2S_STATE_RUNNING:
		/* retrieve buffer from input queue */
		ret = k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT);
		__ASSERT_NO_MSG(ret == 0);

		sys_cache_data_invd_range(buffer, stream->cfg.block_size);
		/* put buffer to output queue */
		ret = k_msgq_put(&stream->out_queue, &buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("buffer %p -> out_queue %p err %d", buffer, &stream->out_queue,
				ret);
			i2s_rx_stream_disable(dev, false, false);
			stream->state = I2S_STATE_ERROR;
			return;
		}
		if (stream->state == I2S_STATE_RUNNING) {
			/* allocate new buffer for next audio frame */
			ret = k_mem_slab_alloc(stream->cfg.mem_slab, &buffer, K_NO_WAIT);
			if (ret != 0) {
				LOG_ERR("buffer alloc from slab %p err %d", stream->cfg.mem_slab,
					ret);
				i2s_rx_stream_disable(dev, false, false);
				stream->state = I2S_STATE_ERROR;
			} else {
				/* reload DMA */
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
				if (IS_REORDER_CH(data->reorder_mode)) {
					sys_cache_data_flush_range(buffer, stream->cfg.block_size);
					dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
						   (uint32_t)&cfg->i2s->SP_RX_FIFO_0_RD_ADDR,
						   (uint32_t)buffer, upper_block_size);
					dma_reload(data->dma_rx_ext.dma_dev,
						   data->dma_rx_ext.dma_channel,
						   (uint32_t)&cfg->i2s->SP_RX_FIFO_1_RD_ADDR,
						   (uint32_t)buffer + upper_block_size,
						   bottom_block_size);
				} else {
					sys_cache_data_flush_range(buffer, stream->cfg.block_size);
					dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
						   (uint32_t)&cfg->i2s->SP_RX_FIFO_0_RD_ADDR,
						   (uint32_t)buffer, stream->cfg.block_size);
				}
#else
				sys_cache_data_flush_range(buffer, stream->cfg.block_size);
				dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
						(uint32_t)&cfg->i2s->SP_RX_FIFO_0_RD_ADDR,
						(uint32_t)buffer, stream->cfg.block_size);
#endif
				/* put buffer in input queue */
				ret = k_msgq_put(&stream->in_queue, &buffer, K_NO_WAIT);
				if (ret != 0) {
					LOG_ERR("%p -> in_queue %p err %d", buffer,
						&stream->in_queue, ret);
					i2s_rx_stream_disable(dev, false, false);
					stream->state = I2S_STATE_ERROR;
					return;
				}

				/* dma_start */
				dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
				if (IS_REORDER_CH(data->reorder_mode)) {
					dma_start(data->dma_rx_ext.dma_dev,
						  data->dma_rx_ext.dma_channel);
				}
#endif
			}
		} else {
			i2s_rx_stream_disable(dev, true, false);
			/* Received a STOP/DRAIN trigger */
			stream->state = I2S_STATE_READY;
		}
		break;
	case I2S_STATE_ERROR:
		LOG_ERR("Error state in rx callback.");
		i2s_rx_stream_disable(dev, true, true);
		break;
	}
}

static int i2s_ameba_enable_clock(const struct device *dev)
{
	const struct i2s_ameba_cfg *cfg = dev->config;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enables I2S peripheral */
	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys)) {
		LOG_ERR("Could not enable I2S clock");
		return -EIO;
	}

	return 0;
}

static int i2s_ameba_start_state_check(struct i2s_ameba_stream *stream)
{
	int retry = 3;

	while (stream->state == I2S_STATE_STOPPING) {
		k_sleep(K_MSEC(1));
		LOG_WRN("Last SPORT is STOPPING, will wait a while...");
		if (retry-- < 0) {
			break;
		}
	}

	if ((stream->state != I2S_STATE_READY) && (stream->state != I2S_STATE_NOT_READY)) {
		LOG_ERR("Stream state (%d) is error for configure.", stream->state);
		return -EIO;
	}
	return 0;
}

static int i2s_ameba_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	struct i2s_ameba_data *data = dev->data;
	const struct i2s_ameba_cfg *const cfg = dev->config;
	uint8_t sp_dir;
	struct i2s_ameba_stream *stream;
	int ret = 0;

	ret = i2s_ameba_start_state_check(&data->tx);
	if (ret < 0) {
		LOG_ERR("I2S TX configure error: wrong state");
		return -EINVAL;
	}

	ret = i2s_ameba_start_state_check(&data->rx);
	if (ret < 0) {
		LOG_ERR("I2S RX configure error: wrong state");
		return -EINVAL;
	}

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
		sp_dir = SP_DIR_RX;
		AUDIO_SP_Deinit(cfg->index, SP_DIR_RX);
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
		sp_dir = SP_DIR_TX;
		AUDIO_SP_Deinit(cfg->index, SP_DIR_TX);
	} else {
		LOG_ERR("Unsupported I2S direction");
		data->tx.state = I2S_STATE_NOT_READY;
		data->rx.state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	if (i2s_cfg->frame_clk_freq == 0U) {
		LOG_WRN("Invalid frame_clk_freq %u", i2s_cfg->frame_clk_freq);
		if (dir == I2S_DIR_TX) {
			data->tx.state = I2S_STATE_NOT_READY;
		} else {
			data->rx.state = I2S_STATE_NOT_READY;
		}
		return 0;
	}

	SP_InitTypeDef SP_InitStruct;
	AUDIO_ClockParams Clock_Params;
	AUDIO_InitParams Init_Params;
	bool slave;

	/*
	 * Channel slot length is always 32-bit; 16/24-bit words are packed
	 * into 32-bit slots by the hardware.
	 */
	uint16_t chn_len1 = SP_CL_32;
	uint16_t sp_txcl = SP_TXCL_32;
	uint16_t xtal_enable;

	if (cfg->MultiIO) {
		data->tdmmode = 0;
	} else {
		/*
		 * The tdm mode is normally decided by channels.
		 * For advanced setting, tdm can be different from channels,
		 * but need more codes to pad or deal with free channels!
		 */
		data->tdmmode = (i2s_cfg->channels - 1) / 2;
	}
	Init_Params.chn_len = chn_len1;
	Init_Params.chn_cnt = (data->tdmmode + 1U) * 2U;
	Init_Params.sr = i2s_cfg->frame_clk_freq;

	Init_Params.codec_multiplier_with_rate = cfg->mclk_multiple;
	Init_Params.sport_mclk_fixed_max = cfg->mclk_fixed_max;

	if (cfg->clock_mode == I2S_CLOCK_XTAL40M) {
		xtal_enable = 1;
	} else {
		xtal_enable = 0;
	}

	/* get MCLK div */
	Audio_Clock_Choose(xtal_enable, &Init_Params, &Clock_Params);

	AUDIO_SP_StructInit(&SP_InitStruct);

	/*
	 * set I2S Data Format
	 * 16-bit data extended on 32-bit channel length excluded
	 */
	if (i2s_cfg->word_size == 16U) {
		SP_InitStruct.SP_SelWordLen = 0;
	} else if (i2s_cfg->word_size == 24U) {
		SP_InitStruct.SP_SelWordLen = 2U;
	} else if (i2s_cfg->word_size == 32U) {
		SP_InitStruct.SP_SelWordLen = 4U;
	} else {
		LOG_ERR("invalid word size");
		return -EINVAL;
	}

	if ((i2s_cfg->options & I2S_OPT_PINGPONG) == I2S_OPT_PINGPONG) {
		LOG_ERR("Ping-pong mode not supported");
		return -ENOTSUP;
	}

	/* set I2S Standard */
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		if ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) || (i2s_cfg->channels == 3U)) {
			return -EINVAL;
		}
		SP_InitStruct.SP_SelDataFormat = i2s_cfg->format;
		break;

	case I2S_FMT_DATA_FORMAT_PCM_SHORT: /* PCM_A */
		SP_InitStruct.SP_SelDataFormat = i2s_cfg->format + 1U;
		break;

	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		LOG_ERR("Unsupported I2S data format: I2S_FMT_DATA_FORMAT_PCM_LONG");
		return -EINVAL;

	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		SP_InitStruct.SP_SelDataFormat = i2s_cfg->format - 2U;
		break;

	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		LOG_ERR("Unsupported I2S data format: I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED");
		return -EINVAL;

	default:
		LOG_ERR("Unsupported I2S data format: others");
		return -EINVAL;
	}

	if (i2s_cfg->channels == 1U) {
		SP_InitStruct.SP_SelI2SMonoStereo = SP_CH_MONO;
	} else {
		SP_InitStruct.SP_SelI2SMonoStereo = SP_CH_STEREO;
	}

	/*
	 * channel = 1, use fifo-0.
	 * channel = 2, use fifo-0 for left and right.
	 * channel = 4, use fifo-0 and fifo-1.
	 * channel = 6, use fifo-0, fifo-1, and fifo-2.
	 * channel = 8, use fifo-0, fifo-1, fifo-2 and fifo-3.
	 */
	if ((i2s_cfg->channels % 2 == 1) && (i2s_cfg->channels != 1)) {
		return -EINVAL;
	}
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	if (i2s_cfg->channels > 8) {
		LOG_ERR("Do not support channels above 8.");
		return -EINVAL;
	}
#else
	if (i2s_cfg->channels > 4) {
		LOG_ERR("See macro: CONFIG_I2S_AMEBA_CHANNEL_EXT");
		return -EINVAL;
	}
#endif
	data->fifo_num = (i2s_cfg->channels - 1) / 2;
	data->reorder_mode = 0;
	if (i2s_cfg->word_size == 24U) {
		LOG_WRN("ATTENTION: i2s_ameba only support I2S_8_24 mode!! "
			"Data inside buffer must been padded by upper layer.");
		data->reorder_mode |= BUFFER_REORDER_8_24;
	}
	data->reorder_mode |= BUFFER_REORDER_CH(data->fifo_num);

	SP_InitStruct.SP_SelTDM = data->tdmmode;
	SP_InitStruct.SP_SelFIFO = data->fifo_num;
	SP_InitStruct.SP_SetMultiIO = cfg->MultiIO;
	SP_InitStruct.SP_SR = i2s_cfg->frame_clk_freq;
	SP_InitStruct.SP_SelChLen = sp_txcl;
	SP_InitStruct.SP_SelClk = cfg->clock_mode;

	/* set MCLK */
	AUDIO_SP_SetMclkDiv(cfg->index, Clock_Params.MCLK_NI, Clock_Params.MCLK_MI);

	if ((i2s_cfg->options & I2S_OPT_LOOPBACK) == I2S_OPT_LOOPBACK) {
		AUDIO_SP_SetSelfLPBK(cfg->index);
	}

	AUDIO_SP_Init(cfg->index, sp_dir, &SP_InitStruct);

	if (dir == I2S_DIR_TX) {
		data->tx.state = I2S_STATE_READY;
	} else {
		data->rx.state = I2S_STATE_READY;
	}

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		slave = true; /* slave */
	} else {
		slave = false; /* master */
	}

	if ((i2s_cfg->options & I2S_OPT_LOOPBACK) == I2S_OPT_LOOPBACK) {
		/* Single i2s-loopback cannot be slave-mode (always need clock). */
		slave = false;
	}

	AUDIO_SP_SetMasterSlave(cfg->index, slave); /* master:0 slave:1 */

	if (cfg->MultiIO == 1) {
		if (dir == I2S_DIR_RX) {
			switch (i2s_cfg->channels) {
			case 1:
			case 2: {
				AUDIO_SP_SetPinMux(cfg->index, DIN0_FUNC);
				break;
			}
			case 3:
			case 4:
				AUDIO_SP_SetPinMux(cfg->index, DIN0_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DIN1_FUNC);
				break;
			case 5:
			case 6:
				AUDIO_SP_SetPinMux(cfg->index, DIN0_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DIN1_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DIN2_FUNC);
				break;
			case 7:
			case 8:
				AUDIO_SP_SetPinMux(cfg->index, DIN0_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DIN1_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DIN2_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DIN3_FUNC);
				break;
			default:
				LOG_ERR("Unsupported I2S channel");
				return -EINVAL;
			}
		} else {
			switch (i2s_cfg->channels) {
			case 1:
			case 2:
				AUDIO_SP_SetPinMux(cfg->index, DOUT0_FUNC);
				break;
			case 3:
			case 4:
				AUDIO_SP_SetPinMux(cfg->index, DOUT0_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DOUT1_FUNC);
				break;
			case 5:
			case 6:
				AUDIO_SP_SetPinMux(cfg->index, DOUT0_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DOUT1_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DOUT2_FUNC);
				break;
			case 7:
			case 8:
				AUDIO_SP_SetPinMux(cfg->index, DOUT0_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DOUT1_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DOUT2_FUNC);
				AUDIO_SP_SetPinMux(cfg->index, DOUT3_FUNC);
			default:
				LOG_ERR("Unsupported I2S channel");
				return -EINVAL;
			}
		}
	} else { /* cfg->MultiIO == 0 */
		if (dir == I2S_DIR_RX) {
			AUDIO_SP_SetPinMux(cfg->index, DIN0_FUNC);
		} else {
			AUDIO_SP_SetPinMux(cfg->index, DOUT0_FUNC);
		}
	}

	/* Clear Software buffers for next transform. */
	i2s_purge_stream_buffers(&data->rx, data->rx.cfg.mem_slab, 1, 1);
	i2s_purge_stream_buffers(&data->tx, data->tx.cfg.mem_slab, 1, 1);

	return 0;
}

static const struct i2s_config *i2s_ameba_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else {
		stream = &data->tx;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->cfg;
}

static int i2s_tx_dma_config(const struct device *dev, uint8_t *pdata, uint32_t length)
{
	struct i2s_ameba_data *data = dev->data;
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_dma_stream *i2s_dma = NULL;

	i2s_dma = &data->dma_tx;
	uint32_t handshake_index = i2s_dma->dma_cfg.dma_slot;

	memset(&i2s_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&i2s_dma->blk_cfg, 0, sizeof(struct dma_block_config));
	i2s_dma->dma_cfg.head_block = &i2s_dma->blk_cfg;

	i2s_dma->blk_cfg.dest_address = (uint32_t)&cfg->i2s->SP_TX_FIFO_0_WR_ADDR;
	i2s_dma->dma_cfg.dma_slot = handshake_index;
	i2s_dma->dma_cfg.dma_callback = i2s_ameba_dma_tx_cb;

	i2s_dma->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	i2s_dma->dma_cfg.complete_callback_en = 1;
	i2s_dma->dma_cfg.error_callback_dis = 1;
	i2s_dma->dma_cfg.source_handshake = 1;
	i2s_dma->dma_cfg.dest_handshake = 0;
	i2s_dma->dma_cfg.channel_priority = 1;
	i2s_dma->dma_cfg.cyclic = 0;

	/* Cofigure GDMA transfer */
	/* 24bits or 16bits mode */
	if (((length & 0x03) == 0) && (((uint32_t)(pdata) & 0x03) == 0)) {
		/* 4-bytes aligned, move 4 bytes each transfer */
		i2s_dma->dma_cfg.source_burst_length = 4;
		i2s_dma->dma_cfg.source_data_size = 4;
		i2s_dma->blk_cfg.block_size = length;
	} else if (((length & 0x01) == 0) && (((uint32_t)(pdata) & 0x01) == 0)) {
		/* 2-bytes aligned, move 2 bytes each transfer */
		i2s_dma->dma_cfg.source_burst_length = 8;
		i2s_dma->dma_cfg.source_data_size = 2;
		i2s_dma->blk_cfg.block_size = length;
	} else {
		LOG_ERR("Alignment Err.");
		return -EINVAL;
	}

	i2s_dma->dma_cfg.block_count = 1;
	i2s_dma->blk_cfg.source_address = (uint32_t)pdata;
	i2s_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	i2s_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	i2s_dma->blk_cfg.source_reload_en = 0;
	i2s_dma->blk_cfg.dest_reload_en = 0;
	i2s_dma->blk_cfg.flow_control_mode = 0;

	i2s_dma->dma_cfg.dest_data_size = 4;
	i2s_dma->dma_cfg.dest_burst_length = 4;
	i2s_dma->dma_cfg.user_data = (void *)dev;

	return dma_config(i2s_dma->dma_dev, i2s_dma->dma_channel, &i2s_dma->dma_cfg);
}

static int i2s_rx_dma_config(const struct device *dev, uint8_t *pdata, uint32_t length)
{
	struct i2s_ameba_data *data = dev->data;
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_dma_stream *i2s_dma = NULL;

	i2s_dma = &data->dma_rx;
	uint32_t handshake_index = i2s_dma->dma_cfg.dma_slot;

	memset(&i2s_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&i2s_dma->blk_cfg, 0, sizeof(struct dma_block_config));
	i2s_dma->dma_cfg.head_block = &i2s_dma->blk_cfg;

	i2s_dma->blk_cfg.source_address = (uint32_t)&cfg->i2s->SP_RX_FIFO_0_RD_ADDR;
	i2s_dma->dma_cfg.dma_slot = handshake_index;
	i2s_dma->dma_cfg.dma_callback = i2s_ameba_dma_rx_cb;

	i2s_dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	i2s_dma->dma_cfg.complete_callback_en = 1;
	i2s_dma->dma_cfg.error_callback_dis = 1;
	i2s_dma->dma_cfg.source_handshake = 0;
	i2s_dma->dma_cfg.dest_handshake = 1;
	i2s_dma->dma_cfg.channel_priority = 1;
	i2s_dma->dma_cfg.cyclic = 0;

	i2s_dma->dma_cfg.source_burst_length = 8;
	i2s_dma->dma_cfg.source_data_size = 4;
	i2s_dma->blk_cfg.block_size = length;

	i2s_dma->dma_cfg.block_count = 1;
	i2s_dma->blk_cfg.dest_address = (uint32_t)pdata;
	i2s_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	i2s_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	i2s_dma->blk_cfg.source_reload_en = 0;
	i2s_dma->blk_cfg.dest_reload_en = 0;
	i2s_dma->blk_cfg.flow_control_mode = 0;

	i2s_dma->dma_cfg.dest_data_size = 4;
	i2s_dma->dma_cfg.dest_burst_length = 8;
	i2s_dma->dma_cfg.user_data = (void *)dev;

	return dma_config(i2s_dma->dma_dev, i2s_dma->dma_channel, &i2s_dma->dma_cfg);
}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
static int i2s_tx_dma_pair_config(const struct device *dev, uint8_t *pdata, uint32_t length)
{
	struct i2s_ameba_data *data = dev->data;
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_dma_stream *i2s_dma = NULL;
	struct i2s_dma_stream *i2s_dma_ext = NULL;
	int ret = 0;

	i2s_dma = &data->dma_tx;
	uint32_t handshake_index = i2s_dma->dma_cfg.dma_slot;

	memset(&i2s_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&i2s_dma->blk_cfg, 0, sizeof(struct dma_block_config));
	i2s_dma->dma_cfg.head_block = &i2s_dma->blk_cfg;

	i2s_dma->blk_cfg.dest_address = (uint32_t)&cfg->i2s->SP_TX_FIFO_0_WR_ADDR;
	i2s_dma->dma_cfg.dma_slot = handshake_index;
	i2s_dma->dma_cfg.dma_callback = i2s_ameba_dma_tx_cb;

	i2s_dma->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	i2s_dma->dma_cfg.complete_callback_en = 1;
	i2s_dma->dma_cfg.error_callback_dis = 1;
	i2s_dma->dma_cfg.source_handshake = 1;
	i2s_dma->dma_cfg.dest_handshake = 0;
	i2s_dma->dma_cfg.channel_priority = 1;
	i2s_dma->dma_cfg.cyclic = 0;

	/* Cofigure GDMA transfer */
	i2s_dma->dma_cfg.source_burst_length = 4;
	i2s_dma->dma_cfg.source_data_size = 1;
	i2s_dma->blk_cfg.block_size = (data->fifo_num == 2) ? (length * 2 / 3) : (length / 2);

	i2s_dma->dma_cfg.block_count = 1;
	i2s_dma->blk_cfg.source_address = (uint32_t)pdata;
	i2s_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	i2s_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	i2s_dma->blk_cfg.source_reload_en = 0;
	i2s_dma->blk_cfg.dest_reload_en = 0;
	i2s_dma->blk_cfg.flow_control_mode = 0;

	i2s_dma->dma_cfg.dest_data_size = 4;
	i2s_dma->dma_cfg.dest_burst_length = 4;
	i2s_dma->dma_cfg.user_data = (void *)dev;

	i2s_dma_ext = &data->dma_tx_ext;
	uint32_t handshake_index_ext = i2s_dma_ext->dma_cfg.dma_slot;

	memcpy(&i2s_dma_ext->dma_cfg, &i2s_dma->dma_cfg, sizeof(struct dma_config));
	i2s_dma_ext->dma_cfg.head_block = &i2s_dma_ext->blk_cfg;
	memcpy(&i2s_dma_ext->blk_cfg, &i2s_dma->blk_cfg, sizeof(struct dma_block_config));
	i2s_dma_ext->blk_cfg.dest_address = (uint32_t)&cfg->i2s->SP_TX_FIFO_1_WR_ADDR;
	i2s_dma_ext->dma_cfg.dma_slot = handshake_index_ext;
	i2s_dma_ext->blk_cfg.block_size = (data->fifo_num == 2) ? (length / 3) : (length / 2);
	i2s_dma_ext->blk_cfg.source_address = (uint32_t)pdata + i2s_dma->blk_cfg.block_size;

	ret = dma_config(i2s_dma->dma_dev, i2s_dma->dma_channel, &i2s_dma->dma_cfg);
	if (ret < 0) {
		LOG_ERR("Config TX DMA failed!");
		return ret;
	}
	ret = dma_config(i2s_dma_ext->dma_dev, i2s_dma_ext->dma_channel, &i2s_dma_ext->dma_cfg);
	if (ret < 0) {
		LOG_ERR("Config TX DMA EXT failed!");
		return ret;
	}
	return ret;
}

static int i2s_rx_dma_pair_config(const struct device *dev, uint8_t *pdata, uint32_t length)
{
	struct i2s_ameba_data *data = dev->data;
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_dma_stream *i2s_dma = NULL;
	struct i2s_dma_stream *i2s_dma_ext = NULL;
	int ret = 0;

	i2s_dma = &data->dma_rx;
	uint32_t handshake_index = i2s_dma->dma_cfg.dma_slot;

	memset(&i2s_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&i2s_dma->blk_cfg, 0, sizeof(struct dma_block_config));
	i2s_dma->dma_cfg.head_block = &i2s_dma->blk_cfg;

	i2s_dma->blk_cfg.source_address = (uint32_t)&cfg->i2s->SP_RX_FIFO_0_RD_ADDR;
	i2s_dma->dma_cfg.dma_slot = handshake_index;
	i2s_dma->dma_cfg.dma_callback = i2s_ameba_dma_rx_cb;

	i2s_dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	i2s_dma->dma_cfg.complete_callback_en = 1;
	i2s_dma->dma_cfg.error_callback_dis = 1;
	i2s_dma->dma_cfg.source_handshake = 0;
	i2s_dma->dma_cfg.dest_handshake = 1;
	i2s_dma->dma_cfg.channel_priority = 1;
	i2s_dma->dma_cfg.cyclic = 0;

	i2s_dma->dma_cfg.source_burst_length = 8;
	i2s_dma->dma_cfg.source_data_size = 4;
	i2s_dma->blk_cfg.block_size = (data->fifo_num == 2) ? (length * 2 / 3) : (length / 2);

	i2s_dma->dma_cfg.block_count = 1;
	i2s_dma->blk_cfg.dest_address = (uint32_t)pdata;
	i2s_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	i2s_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	i2s_dma->blk_cfg.source_reload_en = 0;
	i2s_dma->blk_cfg.dest_reload_en = 0;
	i2s_dma->blk_cfg.flow_control_mode = 0;

	i2s_dma->dma_cfg.dest_data_size = 4;
	i2s_dma->dma_cfg.dest_burst_length = 8;
	i2s_dma->dma_cfg.user_data = (void *)dev;

	i2s_dma_ext = &data->dma_rx_ext;
	memcpy(&i2s_dma_ext->dma_cfg, &i2s_dma->dma_cfg, sizeof(struct dma_config));
	i2s_dma_ext->dma_cfg.head_block = &i2s_dma_ext->blk_cfg;
	memcpy(&i2s_dma_ext->blk_cfg, &i2s_dma->blk_cfg, sizeof(struct dma_block_config));
	i2s_dma_ext->blk_cfg.source_address = (uint32_t)&cfg->i2s->SP_RX_FIFO_1_RD_ADDR;
	i2s_dma_ext->dma_cfg.dma_slot = data->dma_rx.dma_cfg.dma_slot1;
	i2s_dma_ext->blk_cfg.block_size = (data->fifo_num == 2) ? (length / 3) : (length / 2);
	i2s_dma_ext->blk_cfg.dest_address = (uint32_t)pdata + i2s_dma->blk_cfg.block_size;

	ret = dma_config(i2s_dma->dma_dev, i2s_dma->dma_channel, &i2s_dma->dma_cfg);
	if (ret < 0) {
		LOG_ERR("Config RX DMA failed!");
		return ret;
	}
	ret = dma_config(i2s_dma_ext->dma_dev, i2s_dma_ext->dma_channel, &i2s_dma_ext->dma_cfg);
	if (ret < 0) {
		LOG_ERR("Config RX DMA EXT failed!");
		return ret;
	}
	return ret;
}
#endif

static int i2s_tx_stream_start(const struct device *dev)
{
	int ret = 0;
	void *buffer;
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	const struct i2s_ameba_cfg *cfg = dev->config;

	/* retrieve buffer from input queue */
	ret = k_msgq_get(&stream->in_queue, &buffer, K_NO_WAIT);

	if (ret != 0) {
		LOG_ERR("No buffer in input queue to start");
		return -EIO;
	}

	/* Driver keeps track of how many DMA blocks can be loaded to the DMA */
	stream->free_tx_dma_blocks = MAX_TX_DMA_BLOCKS;

	(stream->free_tx_dma_blocks)--;

	/* put buffer in output queue */
	ret = k_msgq_put(&stream->out_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to put buffer in output queue");
		return ret;
	}

	if (IS_REORDER_NULL(data->reorder_mode)) {
		if (i2s_tx_dma_config(dev, (uint8_t *)buffer,
				      (uint32_t)stream->cfg.block_size) < 0) {
			LOG_ERR("i2s tx dma config failed.");
			/* Cannot TX. Recover free_tx_dma_blocks. Call prepare will clear up
			 * allocated buffers.
			 */
			stream->free_tx_dma_blocks = MAX_TX_DMA_BLOCKS;
			return -EIO;
		}
		sys_cache_data_flush_range(buffer, stream->cfg.block_size);
		dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	}
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	else if (IS_REORDER_CH(data->reorder_mode)) {
		if (i2s_tx_dma_pair_config(dev, (uint8_t *)buffer,
					   (uint32_t)stream->cfg.block_size) < 0) {
			LOG_ERR("i2s tx dma pair config failed.");
			stream->free_tx_dma_blocks = MAX_TX_DMA_BLOCKS;
			return -EIO;
		}
		sys_cache_data_flush_range(buffer, stream->cfg.block_size);
		dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
		dma_start(data->dma_tx_ext.dma_dev, data->dma_tx_ext.dma_channel);
	}
#endif
	else {
		return -EIO;
	}

	/* Enable Tx */
	AUDIO_SP_TXStart(cfg->index, ENABLE);

	return 0;
}

static int i2s_rx_stream_start(const struct device *dev)
{
	int ret = 0, i = 0;
	void *buffer;
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->rx;
	const struct i2s_ameba_cfg *cfg = dev->config;
	uint8_t num_of_bufs;
	const struct i2s_config *stream_cfg_rx = i2s_ameba_config_get(dev, I2S_DIR_RX);
	int sample_time = 0;

	num_of_bufs = k_mem_slab_num_free_get(stream->cfg.mem_slab);

	/*
	 * Need at least NUM_DMA_BLOCKS_RX_PREP buffers on the RX memory slab
	 * for reliable DMA reception.
	 */
	if (num_of_bufs < NUM_DMA_BLOCKS_RX_PREP || !IS_SP_SEL_RX_FIFO(data->fifo_num)) {
		return -EINVAL;
	}

	/* allocate 1st receive buffer from SLAB */
	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("buffer alloc from mem_slab failed (%d)", ret);
		return ret;
	}

	/* put buffer in input queue */
	ret = k_msgq_put(&stream->in_queue, &buffer, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to put buffer in input queue, ret %d", ret);
		k_mem_slab_free(stream->cfg.mem_slab, buffer);
		return ret;
	}

	for (i = 0; i <= data->fifo_num; i++) {
		/* Clear hardware RX FIFO. */
		AUDIO_SP_RXSetFifo(cfg->index, i, 0);
		AUDIO_SP_RXSetFifo(cfg->index, i, 1);
	}

	if (IS_REORDER_NULL(data->reorder_mode)) {
		if (i2s_rx_dma_config(dev, (uint8_t *)buffer,
				      (uint32_t)stream->cfg.block_size) < 0) {
			LOG_ERR("i2s rx dma config failed.");
			/* Cannot rx. Call prepare will clear up allocated buffers.*/
			return -EIO;
		}
		sys_cache_data_flush_range(buffer, stream->cfg.block_size);
		dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
	}
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	else if (IS_REORDER_CH(data->reorder_mode)) {
		if (i2s_rx_dma_pair_config(dev, (uint8_t *)buffer,
					   (uint32_t)stream->cfg.block_size) < 0) {
			LOG_ERR("i2s rx dma pair config failed.");
			return -EIO;
		}
		sys_cache_data_flush_range(buffer, stream->cfg.block_size);
		dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		dma_start(data->dma_rx_ext.dma_dev, data->dma_rx_ext.dma_channel);
	}
#endif
	else {
		return -EIO;
	}

	if (((stream_cfg_rx->options & I2S_OPT_LOOPBACK) == I2S_OPT_LOOPBACK) &&
	    (data->tx.state == I2S_STATE_RUNNING)) {
		/* Critically wait LRCLK to ignore hw-first sample(0) in loopback mode.
		 * TX start first, then start rx. Wait for a tx sample LRCLK.
		 * If RX first then TX, refer to CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET.
		 * Attention: Do not print any logs in this delay region.
		 */
		sample_time = 1000000 / stream_cfg_rx->frame_clk_freq;
		k_busy_wait(sample_time);
	}

	/* Enable Rx */
	AUDIO_SP_RXStart(cfg->index, ENABLE);
	return 0;
}

static int i2s_ameba_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_ameba_data *data = dev->data;
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_ameba_stream *stream;
	unsigned int key;
	int ret = 0;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
	} else {
		LOG_ERR("Unsupported I2S direction");
		return -ENOSYS;
	}

	key = irq_lock();
	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_DBG("START trigger: invalid state %u", stream->state);
			ret = -EIO;
			break;
		}

		/* SPORT DMA handshake */
		AUDIO_SP_DmaCmd(cfg->index, ENABLE);

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
		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_DBG("DROP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		if (stream->state == I2S_STATE_RUNNING) {
			/* Stop as the I2S_TRIGGER_STOP. */
			stream->state = I2S_STATE_STOPPING;
			stream->last_block = true;
		} else if (stream->state == I2S_STATE_STOPPING) {
			/* Do nothing. */
		} else {
			/* Stop and drop immediately. */
			stream->state = I2S_STATE_READY;
			if (dir == I2S_DIR_TX) {
				i2s_tx_stream_disable(dev, true);
			} else {
				i2s_rx_stream_disable(dev, true, true);
			}
		}

		irq_unlock(key);
		while (stream->state == I2S_STATE_STOPPING) {
			k_sleep(K_MSEC(1));
		}
		if (stream->state != I2S_STATE_READY) {
			return -EIO;
		}

		if (dir == I2S_DIR_TX) {
			i2s_purge_stream_buffers(stream, data->tx.cfg.mem_slab, true, true);
		} else {
			i2s_purge_stream_buffers(stream, data->rx.cfg.mem_slab, true, true);
		}
		return ret;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_DBG("STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->state = I2S_STATE_STOPPING;
		stream->last_block = true;
		break;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_DBG("DRAIN/STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_DBG("PREPARE trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		AUDIO_SP_Reset(cfg->index);
		stream->state = I2S_STATE_READY;

		if (dir == I2S_DIR_TX) {
			i2s_tx_stream_disable(dev, true);
		} else {
			i2s_rx_stream_disable(dev, true, true);
		}
		break;
	default:
		LOG_ERR("Unsupported trigger cmd");
		ret = -EINVAL;
		break;
	}

	irq_unlock(key);
	return ret;
}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
static void i2s_ameba_reorder_rx_mem_block_by_ch(const struct device *dev, void *buffer)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->rx;
	const struct i2s_config *stream_cfg = i2s_ameba_config_get(dev, I2S_DIR_RX);
	uint8_t *ptemp_block = NULL;
	uint8_t *pdata = (uint8_t *)buffer, *ptemp = NULL;
	int temp_total = 0;
	int upper_bytes = 0, bottom_bytes = 0;
	int upper_unit = 0, bottom_unit = 0;
	int size = stream->cfg.block_size;

	if (data->fifo_num == 2) {
		upper_bytes = size * 2 / 3;
		bottom_bytes = size / 3;
		if (IS_REORDER_CH_8_24(data->reorder_mode)) {
			upper_unit = 16;
			bottom_unit = 8;
		} else {
			upper_unit = stream_cfg->word_size / 2;
			bottom_unit = stream_cfg->word_size / 4;
		}
	} else if (data->fifo_num == 3) {
		upper_bytes = size / 2;
		bottom_bytes = size / 2;
		if (IS_REORDER_CH_8_24(data->reorder_mode)) {
			upper_unit = 16;
			bottom_unit = 16;
		} else {
			upper_unit = stream_cfg->word_size / 2;
			bottom_unit = stream_cfg->word_size / 2;
		}
	}

	BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE > 0,
		     "CONFIG_HEAP_MEM_POOL_SIZE must be configured in .conf");
	ptemp_block = k_malloc(upper_bytes);
	if (ptemp_block == NULL) {
		LOG_ERR("Failed to allocate memory");
		return;
	}
	ptemp = ptemp_block;
	memcpy(ptemp, pdata, upper_bytes);

	ptemp = (uint8_t *)buffer + upper_bytes;
	temp_total = bottom_unit;
	while (temp_total < bottom_bytes) {
		pdata += upper_unit;
#if I2S_AMEBA_DEBUG
		assert_param((pdata >= (uint8_t *)buffer) &&
			     (pdata < ((uint8_t *)buffer + upper_bytes + bottom_bytes)));
		assert_param((ptemp >= (uint8_t *)buffer) &&
			     (ptemp < ((uint8_t *)buffer + upper_bytes + bottom_bytes)));
#endif
		memcpy(pdata, ptemp, bottom_unit);
		pdata += bottom_unit;
		ptemp += bottom_unit;
		temp_total += bottom_unit;
	}

	temp_total = 0;
	pdata = (uint8_t *)buffer;
	ptemp = ptemp_block;
	while (temp_total < upper_bytes) {
#if I2S_AMEBA_DEBUG
		assert_param((pdata >= (uint8_t *)buffer) &&
			     (pdata < ((uint8_t *)buffer + upper_bytes + bottom_bytes)));
		assert_param((ptemp >= ptemp_block) && (ptemp < ptemp_block + upper_bytes));
#endif
		memcpy(pdata, ptemp, upper_unit);
		pdata += (upper_unit + bottom_unit);
		ptemp += upper_unit;
		temp_total += upper_unit;
	}

	k_free(ptemp_block);
}

static void i2s_ameba_reorder_tx_mem_block_by_ch(const struct device *dev, void *mem_block,
						 size_t size)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	const struct i2s_config *stream_cfg = i2s_ameba_config_get(dev, I2S_DIR_TX);
	uint8_t *ptemp_block = NULL;
	uint8_t *pdata = (uint8_t *)mem_block, *ptemp = NULL;
	int temp_total = 0;
	int upper_bytes = 0, bottom_bytes = 0;
	int upper_unit = 0, bottom_unit = 0;

	if (size != stream->cfg.block_size) {
		LOG_ERR("Write Invalid size! size: %d, block: %d.", size, stream->cfg.block_size);
	}

	if (data->fifo_num == 2) {
		upper_bytes = size * 2 / 3;
		bottom_bytes = size / 3;
		if (IS_REORDER_CH_8_24(data->reorder_mode)) {
			upper_unit = 16;
			bottom_unit = 8;
		} else {
			upper_unit = stream_cfg->word_size / 2;
			bottom_unit = stream_cfg->word_size / 4;
		}
	} else if (data->fifo_num == 3) {
		upper_bytes = size / 2;
		bottom_bytes = size / 2;
		if (IS_REORDER_CH_8_24(data->reorder_mode)) {
			upper_unit = 16;
			bottom_unit = 16;
		} else {
			upper_unit = stream_cfg->word_size / 2;
			bottom_unit = stream_cfg->word_size / 2;
		}
	}

	LOG_DBG("upper_bytes = %d, bottom_bytes = %d, upper_unit = %d, bottom_unit = %d",
		upper_bytes, bottom_bytes, upper_unit, bottom_unit);

	BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE > 0,
		     "CONFIG_HEAP_MEM_POOL_SIZE must be configured in .conf");
	ptemp_block = k_malloc(upper_bytes);
	if (ptemp_block == NULL) {
		LOG_ERR("Failed to allocate memory");
		return;
	}
	ptemp = ptemp_block;

	/* bottom_bytes / bottom_unit times (e.g. block_size=4096, uint16_t, ch=8 -> 256 times). */
	while (temp_total < upper_bytes) {
#if I2S_AMEBA_DEBUG
		assert_param((pdata >= (uint8_t *)mem_block) &&
			     (pdata < ((uint8_t *)mem_block + upper_bytes + bottom_bytes)));
		assert_param((ptemp >= ptemp_block) && (ptemp < ptemp_block + upper_bytes));
#endif
		memcpy(ptemp, pdata, upper_unit);
		ptemp += upper_unit;
		pdata += (upper_unit + bottom_unit);
		temp_total += upper_unit;
	}

	pdata = (uint8_t *)mem_block + upper_bytes + bottom_bytes;
	ptemp = (uint8_t *)mem_block + upper_bytes + bottom_bytes;
	temp_total = bottom_unit;
	pdata -= (bottom_unit + upper_unit);
	ptemp -= bottom_unit;
	/* upper_bytes / upper_unit times (e.g. block_size=4096, uint16_t, ch=8 -> 256 times). */
	while (temp_total < bottom_bytes) {
		pdata -= bottom_unit;
		ptemp -= bottom_unit;
#if I2S_AMEBA_DEBUG
		assert_param((pdata >= (uint8_t *)mem_block) &&
			     (pdata < ((uint8_t *)mem_block + upper_bytes + bottom_bytes)));
		assert_param((ptemp >= (uint8_t *)mem_block) &&
			     (ptemp < ((uint8_t *)mem_block + upper_bytes + bottom_bytes)));
#endif
		memcpy(ptemp, pdata, bottom_unit);
		pdata -= upper_unit;
		temp_total += bottom_unit;
	}

	pdata = (uint8_t *)mem_block;
	ptemp = ptemp_block;
	memcpy(pdata, ptemp, upper_bytes);

	k_free(ptemp_block);
}
#endif

static int i2s_ameba_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->rx;

	void *buffer;
	int status;

	if (stream->state == I2S_STATE_NOT_READY) {
		LOG_ERR("invalid state %d", stream->state);
		return -EIO;
	}

	status = k_msgq_get(&stream->out_queue, &buffer, SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (status != 0) {
		if (stream->state == I2S_STATE_ERROR) {
			return -EIO;

		} else {
			LOG_DBG("need retry");
			return -EAGAIN;
		}
	}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	if (IS_REORDER_CH(data->reorder_mode)) {
		i2s_ameba_reorder_rx_mem_block_by_ch(dev, buffer);
	}
#endif

	*mem_block = buffer;
	*size = stream->cfg.block_size;
	return 0;
}

static int i2s_ameba_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_ameba_data *data = dev->data;
	struct i2s_ameba_stream *stream = &data->tx;
	int ret = 0;

	if (stream->state != I2S_STATE_READY && stream->state != I2S_STATE_RUNNING) {
		LOG_ERR("invalid state (%d)", stream->state);
		return -EIO;
	}

	if (size % 32) {
		LOG_ERR("size error for dma cache aligned.");
	}

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	if (IS_REORDER_CH(data->reorder_mode)) {
		i2s_ameba_reorder_tx_mem_block_by_ch(dev, mem_block, size);
	}
#endif

	ret = k_msgq_put(&stream->in_queue, &mem_block, SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (ret) {
		LOG_DBG("k_msgq_put returned code %d", ret);
		return ret;
	}

	return ret;
}

static DEVICE_API(i2s, i2s_ameba_driver_api) = {
	.configure = i2s_ameba_configure,
	.config_get = i2s_ameba_config_get,
	.read = i2s_ameba_read,
	.write = i2s_ameba_write,
	.trigger = i2s_ameba_trigger,
};

static int i2s_ameba_initialize(const struct device *dev)
{
	const struct i2s_ameba_cfg *cfg = dev->config;
	struct i2s_ameba_data *data = dev->data;
	int ret;

	/* Enable I2S clock propagation */
	ret = i2s_ameba_enable_clock(dev);
	if (ret < 0) {
		LOG_ERR("clock enabling failed: %d", ret);
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2S pinctrl setup failed (%d)", ret);
		return ret;
	}

	switch (cfg->clock_mode) {
	case PLL_CLOCK_45P1584M:
		RCC_PeriphClockSource_SPORT(cfg->i2s, CKSL_I2S_CPUPLL);
		PLL_I2S_CLK_DIV(cfg->index, ENABLE, 1U);
		break;

	case PLL_CLOCK_98P304M:
		RCC_PeriphClockSource_SPORT(cfg->i2s, CKSL_I2S_CPUPLL);
		PLL_I2S_CLK_DIV(cfg->index, ENABLE, 0U);
		break;

	case I2S_CLOCK_XTAL40M:
		RCC_PeriphClockSource_SPORT(cfg->i2s, CKSL_I2S_XTAL40M);
		break;

	default:
		LOG_ERR("invalid clock");
		return -EINVAL;
	}

	/* Initialize the buffer queues */
	k_msgq_init(&data->tx.in_queue, (char *)data->tx_in_msgs, sizeof(void *),
		    CONFIG_I2S_AMEBA_TX_BLOCK_COUNT);
	k_msgq_init(&data->rx.out_queue, (char *)data->rx_out_msgs, sizeof(void *),
		    CONFIG_I2S_AMEBA_RX_BLOCK_COUNT);
	/* Always only 1 on rx.in_queue for dma. Always only 1 on tx.out_queue for dma. */
	k_msgq_init(&data->rx.in_queue, (char *)data->rx_in_msgs, sizeof(void *), 1);
	k_msgq_init(&data->tx.out_queue, (char *)data->tx_out_msgs, sizeof(void *), 1);

	AUDIO_SP_Reset(cfg->index);
	return 0;
}

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define I2S_DMA_CHANNEL_INIT(index, dir)                                                           \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = AMEBA_DMA_CONFIG(index, dir, 1, NULL),

#define I2S_DMA_CHANNEL(index, dir)                                                                \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                               \
				  (I2S_DMA_CHANNEL_INIT(index, dir)), (NULL))},

#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
#define I2S_DATA_INIT(n)                                                                           \
	I2S_DMA_CHANNEL(n, rx)                                                                     \
	I2S_DMA_CHANNEL(n, tx) I2S_DMA_CHANNEL(n, rx_ext) I2S_DMA_CHANNEL(n, tx_ext)
#else
#define I2S_DATA_INIT(n) I2S_DMA_CHANNEL(n, rx) I2S_DMA_CHANNEL(n, tx)
#endif

#define I2S_AMEBA_INIT(n)                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct i2s_ameba_cfg i2s_ameba_config_##n = {                                 \
		.i2s = (AUDIO_SPORT_TypeDef *)DT_INST_REG_ADDR(n),                                 \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.index = (uint8_t)((DT_INST_REG_ADDR(n) - SPORT0_REG_BASE) / 0x1000U),             \
		.mclk_multiple = DT_INST_PROP(n, mclk_multiple),                                   \
		.mclk_fixed_max = DT_INST_PROP(n, mclk_fixed_max),                                 \
		.MultiIO = DT_INST_PROP(n, multiio),                                               \
		.clock_mode = DT_INST_PROP(n, clock_mode),                                         \
		.pll_tune = DT_INST_PROP(n, pll_tune),                                             \
		.irq = DT_INST_IRQN(n),                                                            \
                                                                                                   \
	};                                                                                         \
                                                                                                   \
	static struct i2s_ameba_data i2s_ameba_data_##n = {I2S_DATA_INIT(n)};                      \
	DEVICE_DT_INST_DEFINE(n, &i2s_ameba_initialize, NULL, &i2s_ameba_data_##n,                 \
			      &i2s_ameba_config_##n, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,        \
			      &i2s_ameba_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_AMEBA_INIT)
