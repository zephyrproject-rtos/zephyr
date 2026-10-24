/*
 * Copyright (c) 2024-2025 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 * Copyright (c) 2024-2026 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sai

#include <string.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/cache.h>

#include <stm32_bitops.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2s_stm32_sai, CONFIG_I2S_LOG_LEVEL);

enum mclk_divider {
	MCLK_NO_DIV,
	MCLK_DIV_256,
	MCLK_DIV_512
};

static const uint32_t sai_fifo_threshold[] = {
	SAI_FIFOTHRESHOLD_EMPTY,
	SAI_FIFOTHRESHOLD_1QF,
	SAI_FIFOTHRESHOLD_HF,
	SAI_FIFOTHRESHOLD_3QF,
	SAI_FIFOTHRESHOLD_FULL,
};

/* Maximum time to wait for the TX DMA to push data into the SAI FIFO */
#define SAI_FIFO_FILL_TIMEOUT_US 1000U

/* Maximum time for a sub-block to stop: one frame at the lowest supported rate */
#define SAI_DISABLE_TIMEOUT_US 1000U

/* SAI FIFO depth in words */
#define SAI_FIFO_DEPTH 8U

struct queue_item {
	void *buffer;
	size_t size;
};

struct stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;

	/* DMA size of a source block transfer in bytes according SAI data size. */
	uint8_t dma_src_size;

	struct i2s_config i2s_cfg;
	void *mem_block;
	size_t mem_block_len;

	bool master;
	bool last_block;

	int32_t state;
	struct k_msgq queue;

	int (*stream_start)(const struct device *dev, enum i2s_dir dir);
	void (*queue_drop)(const struct device *dev);
};

struct stm32_sai_sub_data {
	SAI_HandleTypeDef hsai;
	struct stream stream;
};

struct stm32_sai_sub_cfg {
	const struct pinctrl_dev_config *pincfg;
	bool mclk_enable;
	enum mclk_divider mclk_div;
	bool synchronous;
	enum i2s_dir dir;

	const struct device *controller;
};

struct stm32_sai_cfg {
	const struct stm32_pclken sai_ck;
	const struct stm32_pclken sai_ker_ck;
	const struct stm32_pclken sai_b_ker_ck; /* Dedicated to SAIn_B, if applicable */
	bool has_sai_ker_ck: 1;
	bool has_sai_b_ker_ck: 1;
};

static inline void sai_sub_disable(SAI_HandleTypeDef *hsai, i2s_opt_t options)
{
	if ((options & I2S_OPT_BIT_CLK_GATED) == 0) {
		LOG_DBG("SAI sub-block %p not disabled: bit clock gating disabled", hsai->Instance);
		return;
	}

	if (hsai->Init.Synchro == SAI_SYNCHRONOUS) {
		LOG_DBG("SAI sub-block %p not disabled: configured as synchronous peripheral",
			hsai->Instance);
		return;
	}

	__HAL_SAI_DISABLE(hsai);

	/*
	 * SAIEN clears at the end of the current frame. Wait for it, so that a following
	 * start does not see the sub-block as enabled and skip re-enabling it.
	 */
	if (!WAIT_FOR(stm32_reg_read_bits(&hsai->Instance->CR1, SAI_xCR1_SAIEN) == 0U,
		      SAI_DISABLE_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("SAI sub-block %p disable timeout", hsai->Instance);
		return;
	}

	LOG_DBG("SAI Disabled");
}

static void sai_sub_stop(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;

	stm32_reg_clear_bits(&hsai->Instance->CR1, SAI_xCR1_DMAEN);
	sai_sub_disable(hsai, sub_data->stream.i2s_cfg.options);
}

static void sai_sub_dma_addr_get(const struct device *dev, uint32_t *src, uint32_t *dst)
{
	const struct stm32_sai_sub_cfg *sub_cfg = dev->config;
	struct stm32_sai_sub_data *sub_data = dev->data;
	uint32_t dr = (uint32_t)&sub_data->hsai.Instance->DR;
	uint32_t mem = (uint32_t)sub_data->stream.mem_block;

	if (sub_cfg->dir == I2S_DIR_TX) {
		*src = mem;
		*dst = dr;
	} else {
		*src = dr;
		*dst = mem;
	}
}

static int sai_sub_dma_start(const struct device *dev)
{
	const struct stm32_sai_sub_cfg *sub_cfg = dev->config;
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	struct dma_block_config *blk_cfg = &stream->dma_blk_cfg;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	int ret;

	memset(blk_cfg, 0, sizeof(*blk_cfg));
	blk_cfg->block_size = stream->mem_block_len;
	sai_sub_dma_addr_get(dev, &blk_cfg->source_address, &blk_cfg->dest_address);

	if (sub_cfg->dir == I2S_DIR_TX) {
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	}

	stream->dma_cfg.head_block = blk_cfg;

	ret = dma_config(stream->dma_dev, stream->dma_channel, &stream->dma_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure DMA channel %u: %d", stream->dma_channel, ret);
		return ret;
	}

	ret = dma_start(stream->dma_dev, stream->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA channel %u: %d", stream->dma_channel, ret);
		return ret;
	}

	if ((sub_cfg->dir == I2S_DIR_RX) && !stream->master &&
	    (stm32_reg_read_bits(&hsai->Instance->CR1, SAI_xCR1_SAIEN) != 0U)) {
		/*
		 * An RX target (synchronous or not) can be left enabled between transfers.
		 * Restart it so that reception resumes on the first slot of a frame.
		 * It provides no clocks, so this does not affect any other sub-block.
		 */
		__HAL_SAI_DISABLE(hsai);
		if (!WAIT_FOR(stm32_reg_read_bits(&hsai->Instance->CR1, SAI_xCR1_SAIEN) == 0U,
			      SAI_DISABLE_TIMEOUT_US, k_busy_wait(1))) {
			LOG_ERR("RX disable timeout");
			(void)dma_stop(stream->dma_dev, stream->dma_channel);
			return -ETIMEDOUT;
		}
		stm32_reg_set_bits(&hsai->Instance->CR2, SAI_xCR2_FFLUSH);
	}

	if (sub_cfg->dir == I2S_DIR_RX) {
		/* Discard samples left in the FIFO since the previous transfer */
		for (uint32_t i = 0U; i < SAI_FIFO_DEPTH; i++) {
			if (stm32_reg_read_bits(&hsai->Instance->SR, SAI_xSR_FLVL) ==
			    SAI_FIFOSTATUS_EMPTY) {
				break;
			}
			(void)stm32_reg_read(&hsai->Instance->DR);
		}
	}

	stm32_reg_set_bits(&hsai->Instance->CR1, SAI_xCR1_DMAEN);

	/* Enable the SAI only once the TX FIFO holds data, to avoid an underrun */
	if ((sub_cfg->dir == I2S_DIR_TX) &&
	    !WAIT_FOR(stm32_reg_read_bits(&hsai->Instance->SR, SAI_xSR_FLVL) !=
			      SAI_FIFOSTATUS_EMPTY,
		      SAI_FIFO_FILL_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("TX FIFO fill timeout");
		stm32_reg_clear_bits(&hsai->Instance->CR1, SAI_xCR1_DMAEN);
		(void)dma_stop(stream->dma_dev, stream->dma_channel);
		return -ETIMEDOUT;
	}

	if (stm32_reg_read_bits(&hsai->Instance->CR1, SAI_xCR1_SAIEN) == 0U) {
		__HAL_SAI_ENABLE(hsai);
	}

	return 0;
}

static int sai_sub_dma_reload(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	uint32_t src, dst;
	int ret;

	sai_sub_dma_addr_get(dev, &src, &dst);

	ret = dma_reload(stream->dma_dev, stream->dma_channel, src, dst, stream->mem_block_len);
	if (ret < 0) {
		return ret;
	}

	return dma_start(stream->dma_dev, stream->dma_channel);
}

static void sai_sub_dma_stop(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	int ret;

	stm32_reg_clear_bits(&sub_data->hsai.Instance->CR1, SAI_xCR1_DMAEN);

	ret = dma_stop(stream->dma_dev, stream->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to stop DMA channel %u: %d", stream->dma_channel, ret);
	}

	/* Stop the bit clock as well when it is gated */
	sai_sub_disable(&sub_data->hsai, stream->i2s_cfg.options);
}

static void sai_sub_rx_callback(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	struct queue_item item = {
		.buffer = stream->mem_block,
		.size = stream->mem_block_len,
	};
	int ret;

	if (stream->state != I2S_STATE_RUNNING && stream->state != I2S_STATE_STOPPING) {
		sai_sub_stop(dev);
		return;
	}

	if (item.buffer == NULL) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("RX mem_block NULL");
		sai_sub_stop(dev);
		return;
	}

	stream->mem_block = NULL;

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		LOG_DBG("Stopping RX ...");
		sai_sub_stop(dev);
	} else {
		/* Restart the transfer first: the SAI FIFO only holds a few samples */
		ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret == 0) {
			stream->mem_block_len = stream->i2s_cfg.block_size;
			ret = sai_sub_dma_reload(dev);
		}

		if (ret < 0) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("RX restart failed: %d", ret);
			sai_sub_stop(dev);
		}
	}

	/*
	 * On SoCs with a data cache, the RX memory slab buffer must be placed
	 * in non-cacheable memory (__nocache with CONFIG_NOCACHE_MEMORY=y).
	 */
	ret = k_msgq_put(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, item.buffer);
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("RX queue full");
		sai_sub_stop(dev);
	}
}

static void sai_sub_tx_callback(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	void *done = stream->mem_block;
	struct queue_item item;
	int ret;

	stream->mem_block = NULL;

	if (stream->state != I2S_STATE_RUNNING && stream->state != I2S_STATE_STOPPING) {
		LOG_ERR("TX bad status: %d, Stopping...", stream->state);
		sai_sub_stop(dev);
		goto out;
	}

	if (stream->last_block) {
		LOG_DBG("TX Stopped ...");
		stream->state = I2S_STATE_READY;
		sai_sub_stop(dev);
		goto out;
	}

	/* No more data in the queue: reset I2S state */
	if (k_msgq_get(&stream->queue, &item, K_NO_WAIT) < 0) {
		LOG_DBG("Exit TX callback, no more data in the queue");
		stream->state = I2S_STATE_READY;
		sai_sub_stop(dev);
		goto out;
	}

	stream->mem_block = item.buffer;
	stream->mem_block_len = item.size;

	sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);

	ret = sai_sub_dma_reload(dev);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("TX DMA reload failed: %d", ret);
		sai_sub_stop(dev);
	}

out:
	/* Release the block that has just been transferred */
	if (done != NULL) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, done);
	}
}

static void dma_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	const struct device *dev = arg;
	const struct stm32_sai_sub_cfg *sub_cfg = dev->config;
	struct stm32_sai_sub_data *sub_data = dev->data;

	ARG_UNUSED(dma_dev);

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %u: %d", channel, status);
		sub_data->stream.state = I2S_STATE_ERROR;
		sai_sub_stop(dev);
		return;
	}

	/* Only the end of the block is of interest */
	if (status != DMA_STATUS_COMPLETE) {
		return;
	}

	if (sub_cfg->dir == I2S_DIR_TX) {
		sai_sub_tx_callback(dev);
	} else {
		sai_sub_rx_callback(dev);
	}
}

static int stm32_sai_clock_en(const struct device *dev)
{
	const struct stm32_sai_cfg *sai_cfg = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	/* Turn on SAI peripheral clock */
	ret = clock_control_on(clk, (clock_control_subsys_t)&sai_cfg->sai_ck);
	if (ret != 0) {
		return -EIO;
	}

	/* Configure shared SAIn_A & SAIn_B or dedicated SAIn_A kernel clock */
	if (sai_cfg->has_sai_ker_ck) {
		ret = clock_control_configure(clk, (clock_control_subsys_t)&sai_cfg->sai_ker_ck,
					      NULL);
		if (ret != 0) {
			return -EIO;
		}
	}

	/* Configure dedicated SAI B kernel clock */
	if (sai_cfg->has_sai_b_ker_ck) {
		ret = clock_control_configure(clk, (clock_control_subsys_t)&sai_cfg->sai_b_ker_ck,
					      NULL);
		if (ret != 0) {
			return -EIO;
		}
	}

	return 0;
}

static int sai_sub_init(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	const struct stm32_sai_sub_cfg *sub_cfg = dev->config;
	struct stream *stream = &sub_data->stream;
	int ret = 0;

	/* Configure DT provided pins */
	ret = pinctrl_apply_state(sub_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SAI Sub-Block pinctrl setup: <FAILED>, ret: %d", ret);
		return ret;
	}

	if (!device_is_ready(sub_data->stream.dma_dev)) {
		LOG_ERR("%s device not ready", sub_data->stream.dma_dev->name);
		return -ENODEV;
	}

	ret = k_msgq_alloc_init(&sub_data->stream.queue, sizeof(struct queue_item),
				CONFIG_I2S_STM32_SAI_BLOCK_COUNT);
	if (ret < 0) {
		LOG_ERR("k_msgq_alloc_init(): <FAILED>, ret: %d", ret);
		return ret;
	}

	sub_data->stream.dma_cfg.user_data = (void *)dev;

	/* State set to not ready until successfully configured */
	stream->state = I2S_STATE_NOT_READY;

	return 0;
}

#if defined(CONFIG_SOC_SERIES_STM32F4X)
static int stm32_sai_sub_f4_clk_src_conf(const struct device *dev)
{
	const struct stm32_sai_sub_cfg *sub_cfg = dev->config;
	const struct stm32_sai_cfg *sai_cfg = sub_cfg->controller->config;
	struct stm32_sai_sub_data *sub_data = dev->data;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	uint32_t clock_source = 0U;

	if (sai_cfg->has_sai_ker_ck) {
		clock_source = sai_cfg->sai_ker_ck.bus;
	}

	switch (clock_source) {
#if defined(STM32F413xx) || defined(STM32F423xx)
	case STM32_SRC_PLLI2S_POST_R:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
		break;

	case STM32_SRC_PLL_POST_R:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLR;
		break;

	case STM32_SRC_HSI:
		hsai->Init.ClockSource = SAI_CLKSOURCE_HS;
		break;
#else /* STM32F413xx || STM32F423xx */
	case STM32_SRC_PLLSAI_POST_Q:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
		break;

	case STM32_SRC_PLLI2S_POST_Q:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
		break;
#endif /* STM32F413xx || STM32F423xx */
	case 0U:
		/* No source clock defined. Do nothing. */
		break;

	default:
		LOG_ERR("Wrong source clock defined.");
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_SOC_SERIES_STM32F4X */

static int stm32_sai_sub_conf(const struct device *dev, enum i2s_dir dir,
				   const struct i2s_config *i2s_cfg)
{
	const struct stm32_sai_sub_cfg *const sub_cfg = dev->config;
	struct stm32_sai_sub_data *const sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	uint8_t protocol;
	uint8_t word_size;

	memcpy(&stream->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->master = true;
	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET) {
		stream->master = false;
	}

#if defined(CONFIG_SOC_SERIES_STM32F4X)
	int err;

	/* ON F4x, the HAL modifies the RCC to set the source clock, so it is necessary to define
	 * the ClockSource Init parameter.
	 */
	err = stm32_sai_sub_f4_clk_src_conf(dev);
	if (err != 0) {
		return err;
	}
#endif /* CONFIG_SOC_SERIES_STM32F4X */

	hsai->Init.Synchro = SAI_ASYNCHRONOUS;

	if (dir == I2S_DIR_RX) {
		if (sub_cfg->dir == I2S_DIR_TX) {
			LOG_ERR("Invalid direction, SAI configured as TX");
			return -EINVAL;
		}

		hsai->Init.AudioMode = SAI_MODEMASTER_RX;

		if (stream->master == false) {
			hsai->Init.AudioMode = SAI_MODESLAVE_RX;
			if (sub_cfg->synchronous) {
				hsai->Init.Synchro = SAI_SYNCHRONOUS;
				LOG_WRN("Synchronous RX peripheral mode requires an active "
					"controller with bit clock gating disabled");
			}
		}

	} else if (dir == I2S_DIR_TX) {
		if (sub_cfg->dir == I2S_DIR_RX) {
			LOG_ERR("Invalid direction, SAI configured as RX");
			return -EINVAL;
		}

		hsai->Init.AudioMode = SAI_MODEMASTER_TX;

		if (stream->master == false) {
			hsai->Init.AudioMode = SAI_MODESLAVE_TX;
			if (sub_cfg->synchronous) {
				hsai->Init.Synchro = SAI_SYNCHRONOUS;
				LOG_WRN("Synchronous TX peripheral mode requires an active "
					"controller with bit clock gating disabled");
			}
		}
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		LOG_ERR("Invalid state: %d", (int)stream->state);
		return -EINVAL;
	}

	/* MckOutput is not supported by all MCU series */
#if defined(SAI_MCK_OUTPUT_ENABLE)
	if (sub_cfg->mclk_enable && stream->master) {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
	} else {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
	}
#endif

	if (sub_cfg->mclk_div == (enum mclk_divider)MCLK_NO_DIV) {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;
	} else {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;

		/* MckOverSampling is not supported by all MCU series */
#if defined(SAI_MCK_OVERSAMPLING_DISABLE)
		if (sub_cfg->mclk_div == (enum mclk_divider)MCLK_DIV_256) {
			hsai->Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;
		} else {
			hsai->Init.MckOverSampling = SAI_MCK_OVERSAMPLING_ENABLE;
		}
#endif
	}

	/* AudioFrequency */
	switch (stream->i2s_cfg.frame_clk_freq) {
	case 192000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_192K;
		break;
	case 96000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_96K;
		break;
	case 48000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
		break;
	case 44100U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
		break;
	case 32000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_32K;
		break;
	case 22050U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_22K;
		break;
	case 16000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_16K;
		break;
	case 11025U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_11K;
		break;
	case 8000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_8K;
		break;
	default:
		LOG_ERR("Invalid frame_clk_freq %u", stream->i2s_cfg.frame_clk_freq);
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	/* WordSize */
	switch (stream->i2s_cfg.word_size) {
	case 16:
		word_size = SAI_PROTOCOL_DATASIZE_16BIT;
		stream->dma_src_size = 2;
		break;
	case 24:
		word_size = SAI_PROTOCOL_DATASIZE_24BIT;
		stream->dma_src_size = 4;
		break;
	case 32:
		word_size = SAI_PROTOCOL_DATASIZE_32BIT;
		stream->dma_src_size = 4;
		break;
	default:
		LOG_ERR("Invalid wordsize %u", stream->i2s_cfg.word_size);
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	/* DMA transfers match the SAI data register access width */
	stream->dma_cfg.source_data_size = stream->dma_src_size;
	stream->dma_cfg.dest_data_size = stream->dma_src_size;
	stream->dma_cfg.source_burst_length = stream->dma_src_size;
	stream->dma_cfg.dest_burst_length = stream->dma_src_size;

	/* MonoStereoMode */
	switch (stream->i2s_cfg.channels) {
	case 1:
		hsai->Init.MonoStereoMode = SAI_MONOMODE;
		LOG_DBG("SAI_MONOMODE");
		break;
	case 2:
		hsai->Init.MonoStereoMode = SAI_STEREOMODE;
		LOG_DBG("SAI_STEREOMODE");
		break;
	default:
		LOG_ERR("NOT VALID CHANNEL NUMBER %u", stream->i2s_cfg.channels);
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	if (stream->i2s_cfg.options & I2S_OPT_PINGPONG) {
		LOG_ERR("Ping-pong mode not supported");
		stream->state = I2S_STATE_NOT_READY;
		return -ENOTSUP;
	}

	if ((stream->i2s_cfg.format & I2S_FMT_DATA_ORDER_LSB) ||
	    (stream->i2s_cfg.format & I2S_FMT_BIT_CLK_INV) ||
	    (stream->i2s_cfg.format & I2S_FMT_FRAME_CLK_INV)) {
		LOG_ERR("Unsupported stream format");
		return -EINVAL;
	}

	switch (stream->i2s_cfg.format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		protocol = SAI_I2S_STANDARD;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		protocol = SAI_I2S_MSBJUSTIFIED;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		protocol = SAI_PCM_SHORT;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		protocol = SAI_PCM_LONG;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		protocol = SAI_I2S_LSBJUSTIFIED;
		break;
	default:
		LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	/* Initialize SAI peripheral */
	if (HAL_SAI_InitProtocol(hsai, protocol, word_size, 2) != HAL_OK) {
		LOG_ERR("HAL_SAI_InitProtocol: <FAILED>");
		return -EIO;
	}

	stream->state = I2S_STATE_READY;

	/*
	 * Enable immediately SAI peripheral only when the bit clock is not gated.
	 * It allows the synchronous sub-block to become operational immediately.
	 */
	if (((i2s_cfg->options & I2S_OPT_BIT_CLK_GATED) == 0) &&
	    hsai->Init.Synchro != SAI_SYNCHRONOUS) {

		__HAL_SAI_ENABLE(hsai);

		if (sub_cfg->dir == I2S_DIR_TX) {
			/* Prime the FIFO with a silent frame so the controller
			 * actually starts clocking out frames. A whole frame keeps
			 * the following data aligned on the first slot.
			 */
			for (uint8_t i = 0U; i < stream->i2s_cfg.channels; i++) {
				stm32_reg_write(&hsai->Instance->DR, 0U);
			}
		} else {
			/* Discard whatever's in DR to clear FIFO state
			 * before the real DMA-driven reads begin.
			 */
			(void)stm32_reg_read(&hsai->Instance->DR);
		}

		LOG_DBG("SAI Enabled");
	}

	return 0;
}

static int stm32_sai_sub_write(const struct device *dev, void *mem_block, size_t size)
{
	const struct stm32_sai_sub_cfg *const sub_cfg = dev->config;
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	int ret;

	if (sub_cfg->dir == I2S_DIR_RX) {
		LOG_ERR("Invalid operation, SAI configured as RX");
		return -EIO;
	}

	if (stream->state != I2S_STATE_RUNNING && stream->state != I2S_STATE_READY) {
		LOG_ERR("TX Invalid state: %d", stream->state);
		return -EIO;
	}

	if (size > stream->i2s_cfg.block_size) {
		LOG_ERR("Max write size is: %zu", stream->i2s_cfg.block_size);
		return -EINVAL;
	}

	struct queue_item item = {.buffer = mem_block, .size = size};

	ret = k_msgq_put(&stream->queue, &item, SYS_TIMEOUT_MS(stream->i2s_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("TX queue full");
		return ret;
	}

	return 0;
}

static int stm32_sai_sub_read(const struct device *dev, void **mem_block, size_t *size)
{
	const struct stm32_sai_sub_cfg *const sub_cfg = dev->config;
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct queue_item item;
	int ret;

	if (sub_cfg->dir == I2S_DIR_TX) {
		LOG_ERR("Invalid operation, SAI configured as TX");
		return -EIO;
	}

	if (sub_data->stream.state == I2S_STATE_NOT_READY ||
	    sub_data->stream.state == I2S_STATE_ERROR) {
		LOG_ERR("RX invalid state: %d", (int)sub_data->stream.state);
		return -EIO;
	}

	ret = k_msgq_get(&sub_data->stream.queue, &item,
			 SYS_TIMEOUT_MS(sub_data->stream.i2s_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("RX queue: %d", k_msgq_num_used_get(&sub_data->stream.queue));
		return ret;
	}

	*mem_block = item.buffer;
	*size = item.size;

	return 0;
}

static int stream_start(const struct device *dev, enum i2s_dir dir)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	struct queue_item item;
	int ret;

	if (dir == I2S_DIR_TX) {
		ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}

		stream->mem_block = item.buffer;
		stream->mem_block_len = item.size;

		sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);
	} else {
		ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}

		stream->mem_block_len = stream->i2s_cfg.block_size;
	}

	ret = sai_sub_dma_start(dev);
	if (ret < 0) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	return 0;
}

static void queue_drop(const struct device *dev)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	struct queue_item item;

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_len = 0;
	}

	while (k_msgq_get(&stream->queue, &item, K_NO_WAIT) == 0) {
		LOG_DBG("Dropping item from queue");
		k_mem_slab_free(stream->i2s_cfg.mem_slab, item.buffer);
	}
}

static int stm32_sai_sub_trigger(const struct device *dev, enum i2s_dir dir,
				 enum i2s_trigger_cmd cmd)
{
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_BOTH) {
		LOG_ERR("Unsupported direction: %d", (int)dir);
		return -ENOTSUP;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		LOG_DBG("I2S_TRIGGER_START");

		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d", stream->state);
			return -EIO;
		}

		ret = stream->stream_start(dev, dir);
		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			return ret;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;

		break;
	case I2S_TRIGGER_STOP:
		key = irq_lock();
		LOG_DBG("I2S_TRIGGER_STOP");

		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP - Invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->last_block = true;
		stream->state = I2S_STATE_STOPPING;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		LOG_DBG("I2S_TRIGGER_DRAIN");

		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN - Invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->state = I2S_STATE_STOPPING;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_DROP:
		LOG_DBG("I2S_TRIGGER_DROP");

		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - invalid state: %d", (int)stream->state);
			return -EIO;
		}

		/* Stop an ongoing transfer before releasing its buffer */
		sai_sub_dma_stop(dev);

		key = irq_lock();
		stream->queue_drop(dev);
		stream->state = I2S_STATE_READY;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_PREPARE:
		LOG_DBG("I2S_TRIGGER_PREPARE");

		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE - invalid state: %d", (int)stream->state);
			return -EIO;
		}

		/* A transfer may still be armed when the error was raised */
		sai_sub_dma_stop(dev);

		key = irq_lock();
		stream->queue_drop(dev);
		stream->state = I2S_STATE_READY;

		irq_unlock(key);
		break;
	default:
		LOG_ERR("Unsupported trigger command");
		return -EINVAL;
	}
	return 0;
}

static int sai_init(const struct device *dev)
{
	int ret = 0;

	/* Enable SAI clock */
	ret = stm32_sai_clock_en(dev);
	if (ret < 0) {
		LOG_ERR("SAI clock enable <FAILED>, ret: %d.", ret);
		return -EIO;
	}

	LOG_DBG("%s Init: <OK>", dev->name);

	return 0;
}

static const struct i2s_config *stm32_sai_sub_conf_get(const struct device *dev, enum i2s_dir dir)
{
	const struct stm32_sai_sub_cfg *const sub_cfg = dev->config;
	struct stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;

	if (sub_cfg->dir != dir) {
		LOG_WRN("Direction mismatch: requested %d, sub-block configured as %d", dir,
			sub_cfg->dir);
		return NULL;
	}

	if (stream != NULL && stream->state != I2S_STATE_NOT_READY) {
		return &stream->i2s_cfg;
	}

	return NULL;
}

static DEVICE_API(i2s, i2s_stm32_sai_api) = {
	.configure = stm32_sai_sub_conf,
	.trigger = stm32_sai_sub_trigger,
	.write = stm32_sai_sub_write,
	.read = stm32_sai_sub_read,
	.config_get = stm32_sai_sub_conf_get,
};

#define SAI_FIFO_THRESHOLD(node) sai_fifo_threshold[DT_ENUM_IDX(node, fifo_threshold)]

#define SAI_SUB_DMA_CHANNEL_INIT(node, src, dest)                                                  \
	.stream = {                                                                                \
		.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),                                      \
		.dma_channel = DT_DMAS_CELL_BY_IDX(node, 0, channel),                              \
		.dma_cfg = {                                                                       \
			.dma_slot = STM32_DT_DMA_SLOT_BY_IDX(node, 0),                             \
			.channel_direction = src##_TO_##dest,                                      \
			.block_count = 1,                                                          \
			.dma_callback = dma_callback,                                              \
			.channel_priority = STM32_DMA_CONFIG_PRIORITY(                             \
				STM32_DT_DMA_CHANNEL_CONFIG_BY_IDX(node, 0)),                      \
		},                                                                                 \
		.stream_start = stream_start,                                                      \
		.queue_drop = queue_drop,                                                          \
	}

#define SAI_SUB_INIT(node)                                                                         \
	PINCTRL_DT_DEFINE(node);                                                                   \
                                                                                                   \
	static struct stm32_sai_sub_data sub_data_##node = {                                       \
		.hsai = {                                                                          \
			.Instance = (SAI_Block_TypeDef *)DT_REG_ADDR(node),                        \
			.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE,                               \
			.Init.FIFOThreshold = SAI_FIFO_THRESHOLD(node),                            \
			.Init.SynchroExt = SAI_SYNCEXT_DISABLE,                                    \
			.Init.CompandingMode = SAI_NOCOMPANDING,                                   \
			.Init.TriState = SAI_OUTPUT_NOTRELEASED,                                   \
		},                                                                                 \
		COND_CODE_1(DT_DMAS_HAS_NAME(node, tx),                                            \
		    (SAI_SUB_DMA_CHANNEL_INIT(node, MEMORY, PERIPHERAL)),                          \
		    (SAI_SUB_DMA_CHANNEL_INIT(node, PERIPHERAL, MEMORY))),                         \
	};                                                                                         \
                                                                                                   \
	static const struct stm32_sai_sub_cfg sub_cfg_##node = {                                   \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(node),                                         \
		.mclk_enable = DT_PROP(node, mclk_enable),                                         \
		.mclk_div = DT_ENUM_IDX(node, mclk_divider),                                       \
		.synchronous = DT_PROP(node, synchronous),                                         \
		.controller = DEVICE_DT_GET(DT_PARENT(node)),                                      \
		.dir = COND_CODE_1(DT_DMAS_HAS_NAME(node, tx), (I2S_DIR_TX), (I2S_DIR_RX)),        \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, &sai_sub_init, NULL, &sub_data_##node, &sub_cfg_##node,             \
			 POST_KERNEL, CONFIG_I2S_INIT_PRIORITY, &i2s_stm32_sai_api);               \
	K_MSGQ_DEFINE_STATIC_TYPE(queue_##node, struct queue_item,                                 \
				  CONFIG_I2S_STM32_SAI_BLOCK_COUNT);

#define SAI_KER_CK_FIELD_INIT(inst, n)                                                             \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(inst, n),                                              \
		(.n = STM32_DT_INST_CLOCK_INFO_BY_NAME(inst, n), .has_##n = true,),                \
		(.has_##n = false,))

#define SAI_KER_CK_INIT(inst)                                                                      \
	SAI_KER_CK_FIELD_INIT(inst, sai_ker_ck)                                                    \
	SAI_KER_CK_FIELD_INIT(inst, sai_b_ker_ck)

/* Controller Node */
#define SAI_INIT(inst)                                                                             \
	static const struct stm32_sai_cfg sai_cfg_##inst = {                                       \
		.sai_ck = STM32_DT_INST_CLOCK_INFO_BY_NAME(inst, sai_ck),                          \
		SAI_KER_CK_INIT(inst)                                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &sai_init, NULL, NULL, &sai_cfg_##inst, POST_KERNEL,           \
			      CONFIG_I2S_INIT_PRIORITY, NULL);                                     \
                                                                                                   \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SAI_SUB_INIT)

DT_INST_FOREACH_STATUS_OKAY(SAI_INIT)
