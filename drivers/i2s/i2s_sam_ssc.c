/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_ssc

/** @file
 * @brief I2S bus (SSC) driver for Atmel SAM MCU family.
 *
 * Limitations:
 * - TX and RX path share a common bit clock divider and as a result they cannot
 *   be configured independently. If RX and TX path are set to different bit
 *   clock frequencies the latter setting will quietly override the former.
 *   We should return an error in such a case.
 * - DMA is used in simple single block transfer mode and as such is not able
 *   to handle high speed data. To support higher transfer speeds the DMA
 *   linked list mode should be used.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#ifdef CONFIG_I2S_SAM_SSC_DMA
#include <zephyr/drivers/dma.h>
#endif /* CONFIG_I2S_SAM_SSC_DMA */
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <soc.h>

#define LOG_DOMAIN dev_i2s_sam_ssc
#define LOG_LEVEL CONFIG_I2S_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#define SAM_SSC_WORD_SIZE_BITS_MIN    2
#define SAM_SSC_WORD_SIZE_BITS_MAX   32
#define SAM_SSC_WORD_PER_FRAME_MIN    1
#define SAM_SSC_WORD_PER_FRAME_MAX   16

/* Device constant configuration parameters */
struct i2s_sam_dev_cfg {
#ifdef CONFIG_I2S_SAM_SSC_DMA
	const struct device *dev_dma;
#endif /* CONFIG_I2S_SAM_SSC_DMA */
	Ssc *regs;
	void (*irq_config)(void);
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	bool pin_rk_en;
	bool pin_rf_en;
};

struct stream {
	enum i2s_state state;
#ifdef CONFIG_I2S_SAM_SSC_DMA
	uint32_t dma_channel;
	uint8_t dma_perid;
#endif /* CONFIG_I2S_SAM_SSC_DMA */
	uint8_t word_size_bytes;
	bool last_block;
	struct i2s_config cfg;
	struct k_msgq *mem_block_queue;
	void *mem_block;
	size_t mem_block_offset;
	int (*stream_start)(const struct device *dev, struct stream *str);
	void (*stream_disable)(const struct device *dev, struct stream *str);
	void (*queue_drop)(struct stream *str);
	int (*set_data_format)(const struct i2s_sam_dev_cfg *dev_cfg,
			       const struct i2s_config *i2s_cfg);
};

/* Device run time data */
struct i2s_sam_dev_data {
	struct stream rx;
	struct stream tx;
};

static void rx_callback(const struct device *dev, int status, size_t buf_size);
static void tx_callback(const struct device *dev, int status, size_t buf_size);
static void rx_stream_disable(const struct device *dev, struct stream *str);
static void tx_stream_disable(const struct device *dev, struct stream *str);

#ifdef CONFIG_I2S_SAM_SSC_DMA

static int reload_dma(const struct device *dev_dma, uint32_t channel,
		      void *src, void *dst, size_t size)
{
	int ret;

	ret = dma_reload(dev_dma, channel, (uint32_t)src, (uint32_t)dst, size);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(dev_dma, channel);

	return ret;
}

static int start_dma(const struct device *dev_dma, uint32_t channel,
		     struct dma_config *cfg, void *src, void *dst,
		     uint32_t blk_size)
{
	struct dma_block_config blk_cfg;
	int ret;

	(void)memset(&blk_cfg, 0, sizeof(blk_cfg));
	blk_cfg.block_size = blk_size;
	blk_cfg.source_address = (uint32_t)src;
	blk_cfg.dest_address = (uint32_t)dst;

	cfg->head_block = &blk_cfg;

	ret = dma_config(dev_dma, channel, cfg);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(dev_dma, channel);

	return ret;
}

static void dma_rx_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct i2s_sam_dev_data *const dev_data = dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	rx_callback(dev, status, dev_data->rx.cfg.block_size);
}

static void dma_tx_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct i2s_sam_dev_data *const dev_data = dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	tx_callback(dev, status, dev_data->tx.cfg.block_size);
}

#endif /* CONFIG_I2S_SAM_SSC_DMA */

/* This function is executed in the interrupt context */
static void rx_callback(const struct device *dev, int status, size_t buf_size)
{
#ifdef CONFIG_I2S_SAM_SSC_DMA
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
#endif /* CONFIG_I2S_SAM_SSC_DMA */
	struct i2s_sam_dev_data *const dev_data = dev->data;
	struct stream *stream = &dev_data->rx;
	int ret;

	__ASSERT_NO_MSG(stream->mem_block != NULL);

	/* Stop reception if there was an error */
	if (stream->state == I2S_STATE_ERROR || status < 0) {
		goto rx_disable;
	}

	stream->mem_block_offset += buf_size;
	if (stream->mem_block_offset >= stream->cfg.block_size) {
		/* All block data received */
		ret = k_msgq_put(stream->mem_block_queue, &stream->mem_block, K_NO_WAIT);
		if (ret < 0) {
			stream->state = I2S_STATE_ERROR;
			goto rx_disable;
		}
		stream->mem_block = NULL;
		stream->mem_block_offset = 0;

		/* Stop reception if we were requested */
		if (stream->state == I2S_STATE_STOPPING) {
			stream->state = I2S_STATE_READY;
			goto rx_disable;
		}

		/* Prepare to receive the next data block */
		ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret < 0) {
			stream->state = I2S_STATE_ERROR;
			goto rx_disable;
		}
	}

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma) {
		Ssc *const ssc = dev_cfg->regs;

		/* Assure cache coherency before DMA write operation */
		sys_cache_data_invd_range(stream->mem_block, stream->cfg.block_size);

		ret = reload_dma(dev_cfg->dev_dma, stream->dma_channel,
				(void *)&(ssc->SSC_RHR), stream->mem_block,
				stream->cfg.block_size);
		if (ret < 0) {
			LOG_DBG("Failed to reload RX DMA transfer: %d", ret);
			goto rx_disable;
		}
	}
#endif /* CONFIG_I2S_SAM_SSC_DMA */

	return;

rx_disable:
	rx_stream_disable(dev, stream);
}

/* This function is executed in the interrupt context */
static void tx_callback(const struct device *dev, int status, size_t buf_size)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	struct i2s_sam_dev_data *const dev_data = dev->data;
	Ssc *const ssc = dev_cfg->regs;
	struct stream *stream = &dev_data->tx;
	int ret;

	__ASSERT_NO_MSG(stream->mem_block != NULL);

	stream->mem_block_offset += buf_size;
	if (stream->mem_block_offset >= stream->cfg.block_size) {
		/* All block data sent */
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_offset = 0;

		/* Stop transmission if there was an error */
		if (stream->state == I2S_STATE_ERROR || status < 0) {
			LOG_DBG("TX error detected");
			goto tx_disable;
		}

		/* Stop transmission if we were requested */
		if (stream->last_block) {
			stream->state = I2S_STATE_READY;
			goto tx_disable;
		}

		/* Prepare to send the next data block */
		ret = k_msgq_get(stream->mem_block_queue, &stream->mem_block, K_NO_WAIT);
		if (ret < 0) {
			if (stream->state == I2S_STATE_STOPPING) {
				stream->state = I2S_STATE_READY;
			} else {
				stream->state = I2S_STATE_ERROR;
			}
			goto tx_disable;
		}
	}

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma) {
		/* Assure cache coherency before DMA read operation */
		sys_cache_data_flush_range(stream->mem_block, stream->cfg.block_size);

		ret = reload_dma(dev_cfg->dev_dma, stream->dma_channel,
				stream->mem_block, (void *)&(ssc->SSC_THR),
				stream->cfg.block_size);
		if (ret < 0) {
			LOG_DBG("Failed to reload TX DMA transfer: %d", ret);
			goto tx_disable;
		}
	} else
#endif
	{
		mem_addr_t addr = (mem_addr_t)stream->mem_block;
		addr += stream->mem_block_offset;

		switch (stream->word_size_bytes) {
		case 1:
			ssc->SSC_THR = ((uint8_t *)addr)[0];
			break;
		case 2:
			ssc->SSC_THR = ((uint16_t *)addr)[0];
			break;
		case 4:
			ssc->SSC_THR = ((uint32_t *)addr)[0];
			break;
		default:
			goto tx_disable;
		}
	}

	return;

tx_disable:
	tx_stream_disable(dev, stream);
}

static int set_rx_data_format(const struct i2s_sam_dev_cfg *const dev_cfg,
			      const struct i2s_config *i2s_cfg)
{
	Ssc *const ssc = dev_cfg->regs;
	uint8_t word_size_bits = i2s_cfg->word_size;
	uint8_t num_words = i2s_cfg->channels;
	uint8_t fslen = 0U;
	uint32_t ssc_rcmr = 0U;
	uint32_t ssc_rfmr = 0U;
	bool frame_clk_master = !(i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE);

	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {

	case I2S_FMT_DATA_FORMAT_I2S:
		num_words = 2U;
		fslen = word_size_bits - 1;

		ssc_rcmr = SSC_RCMR_CKI
			   | (dev_cfg->pin_rf_en ? SSC_RCMR_START_RF_FALLING : 0)
			   | SSC_RCMR_STTDLY(1);

		ssc_rfmr = (dev_cfg->pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_NEGATIVE : SSC_RFMR_FSOS_NONE);
		break;

	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		ssc_rcmr = (dev_cfg->pin_rf_en ? SSC_RCMR_START_RF_FALLING : 0)
			   | SSC_RCMR_STTDLY(0);

		ssc_rfmr = (dev_cfg->pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_POSITIVE : SSC_RFMR_FSOS_NONE);
		break;

	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		fslen = num_words * word_size_bits / 2U - 1;

		ssc_rcmr = (dev_cfg->pin_rf_en ? SSC_RCMR_START_RF_RISING : 0)
			   | SSC_RCMR_STTDLY(0);

		ssc_rfmr = (dev_cfg->pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_POSITIVE : SSC_RFMR_FSOS_NONE);
		break;

	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		fslen = num_words * word_size_bits / 2U - 1;

		ssc_rcmr = SSC_RCMR_CKI
			   | (dev_cfg->pin_rf_en ? SSC_RCMR_START_RF_RISING : 0)
			   | SSC_RCMR_STTDLY(0);

		ssc_rfmr = (dev_cfg->pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_POSITIVE : SSC_RFMR_FSOS_NONE);
		break;

	default:
		LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	if (dev_cfg->pin_rk_en) {
		if (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
			ssc_rcmr |= SSC_RCMR_CKS_RK
				 | SSC_RCMR_CKO_NONE;
		} else {
			ssc_rcmr |= SSC_RCMR_CKS_MCK
				 | ((i2s_cfg->options & I2S_OPT_BIT_CLK_GATED)
				    ? SSC_RCMR_CKO_TRANSFER : SSC_RCMR_CKO_CONTINUOUS);
		}
	} else {
		ssc_rcmr |= SSC_RCMR_CKS_TK
			 | SSC_RCMR_CKO_NONE;
	}
	/* SSC_RCMR.PERIOD bit filed does not support setting the
	 * frame period with one bit resolution. In case the required
	 * frame period is an odd number set it to be one bit longer.
	 */

	if (!(i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE)) {
		ssc_rcmr |= (dev_cfg->pin_rf_en ? 0 : SSC_RCMR_START_TRANSMIT)
			 | SSC_RCMR_PERIOD((num_words * word_size_bits + 1) / 2U - 1);
	}

	/* Receive Clock Mode Register */
	ssc->SSC_RCMR = ssc_rcmr;

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) {
		ssc_rfmr &= ~SSC_RFMR_FSOS_Msk;
		ssc_rfmr |= SSC_RFMR_FSOS_NONE;
	} else {
		ssc_rfmr |= SSC_RFMR_DATNB(num_words - 1)
			 | SSC_RFMR_FSLEN(fslen)
			 | SSC_RFMR_FSLEN_EXT(fslen >> 4);
	}

	ssc_rfmr |= SSC_RFMR_DATLEN(word_size_bits - 1)
		 | ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB)
		    ? 0 : SSC_RFMR_MSBF);

	/* Receive Frame Mode Register */
	ssc->SSC_RFMR = ssc_rfmr;

	return 0;
}

static int set_tx_data_format(const struct i2s_sam_dev_cfg *const dev_cfg,
			      const struct i2s_config *i2s_cfg)
{
	Ssc *const ssc = dev_cfg->regs;
	uint8_t word_size_bits = i2s_cfg->word_size;
	uint8_t num_words = i2s_cfg->channels;
	uint8_t fslen = 0U;
	uint32_t ssc_tcmr = 0U;
	uint32_t ssc_tfmr = 0U;

	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {

	case I2S_FMT_DATA_FORMAT_I2S:
		num_words = 2U;
		fslen = word_size_bits - 1;

		ssc_tcmr = SSC_TCMR_START_TF_FALLING
			   | SSC_TCMR_STTDLY(1);

		ssc_tfmr = SSC_TFMR_FSOS_NEGATIVE;
		break;

	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		ssc_tcmr = SSC_TCMR_CKI
			   | SSC_TCMR_START_TF_FALLING
			   | SSC_TCMR_STTDLY(0);

		ssc_tfmr = SSC_TFMR_FSOS_POSITIVE;
		break;

	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		fslen = num_words * word_size_bits / 2U - 1;

		ssc_tcmr = SSC_TCMR_CKI
			   | SSC_TCMR_START_TF_RISING
			   | SSC_TCMR_STTDLY(0);

		ssc_tfmr = SSC_TFMR_FSOS_POSITIVE;
		break;

	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		fslen = num_words * word_size_bits / 2U - 1;

		ssc_tcmr = SSC_TCMR_START_TF_RISING
			   | SSC_TCMR_STTDLY(0);

		ssc_tfmr = SSC_TFMR_FSOS_POSITIVE;
		break;

	default:
		LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	/* SSC_TCMR.PERIOD bit filed does not support setting the
	 * frame period with one bit resolution. In case the required
	 * frame period is an odd number set it to be one bit longer.
	 */
	if (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		ssc_tcmr |= SSC_TCMR_CKS_TK
			 | SSC_TCMR_CKO_NONE;
	} else {
		ssc_tcmr |= SSC_TCMR_CKS_MCK
			 | ((i2s_cfg->options & I2S_OPT_BIT_CLK_GATED)
			     ? SSC_TCMR_CKO_TRANSFER : SSC_TCMR_CKO_CONTINUOUS)
			 | SSC_TCMR_PERIOD((num_words * word_size_bits + 1) / 2U - 1);
	}

	/* Transmit Clock Mode Register */
	ssc->SSC_TCMR = ssc_tcmr;

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) {
		ssc_tfmr &= ~SSC_TFMR_FSOS_Msk;
		ssc_tfmr |= SSC_TFMR_FSOS_NONE;
	} else {
		ssc_tfmr |= SSC_TFMR_DATNB(num_words - 1)
			 | SSC_TFMR_FSLEN(fslen)
			 | SSC_TFMR_FSLEN_EXT(fslen >> 4);
	}

	ssc_tfmr |= SSC_TFMR_DATLEN(word_size_bits - 1)
		 | ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB)
		    ? 0 : SSC_TFMR_MSBF);

	/* Transmit Frame Mode Register */
	ssc->SSC_TFMR = ssc_tfmr;

	return 0;
}

/* Calculate number of bytes required to store a word of bit_size length */
static uint8_t get_word_size_bytes(uint8_t bit_size)
{
	uint8_t byte_size_min = (bit_size + 7) / 8U;
	uint8_t byte_size;

	byte_size = (byte_size_min == 3U) ? 4 : byte_size_min;

	return byte_size;
}

static int bit_clock_set(const struct device *dev, uint32_t bit_clk_freq)
{
	const struct i2s_sam_dev_cfg *dev_cfg = dev->config;
	Ssc *ssc = dev_cfg->regs;
	uint32_t clk_div, rate;
	int ret;

	ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				     (clock_control_subsys_t)&dev_cfg->clock_cfg, &rate);
	if (ret < 0) {
		LOG_ERR("Failed to get peripheral clock rate (%d)", ret);
		return ret;
	}

	clk_div = rate / bit_clk_freq / 2U;

	if (clk_div == 0U || clk_div >= (1 << 12)) {
		LOG_ERR("Invalid bit clock frequency %d", bit_clk_freq);
		return -EINVAL;
	}

	ssc->SSC_CMR = clk_div;

	LOG_DBG("freq = %d", bit_clk_freq);

	return 0;
}

static const struct i2s_config *i2s_sam_config_get(const struct device *dev,
						   enum i2s_dir dir)
{
	struct i2s_sam_dev_data *const dev_data = dev->data;
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

static int i2s_sam_configure(const struct device *dev, enum i2s_dir dir,
			     const struct i2s_config *i2s_cfg)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	struct i2s_sam_dev_data *const dev_data = dev->data;
	Ssc *const ssc = dev_cfg->regs;
	uint8_t num_words = i2s_cfg->channels;
	uint8_t word_size_bits = i2s_cfg->word_size;
	uint32_t bit_clk_freq;
	struct stream *stream;
	int ret;

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
		stream->queue_drop(stream);
		(void)memset(&stream->cfg, 0, sizeof(struct i2s_config));
		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	if (i2s_cfg->format & I2S_FMT_FRAME_CLK_INV) {
		LOG_ERR("Frame clock inversion is not implemented");
		LOG_ERR("Please submit a patch");
		return -EINVAL;
	}

	if (i2s_cfg->format & I2S_FMT_BIT_CLK_INV) {
		LOG_ERR("Bit clock inversion is not implemented");
		LOG_ERR("Please submit a patch");
		return -EINVAL;
	}

	if (word_size_bits < SAM_SSC_WORD_SIZE_BITS_MIN ||
	    word_size_bits > SAM_SSC_WORD_SIZE_BITS_MAX) {
		LOG_ERR("Unsupported I2S word size");
		return -EINVAL;
	}

	if (num_words < SAM_SSC_WORD_PER_FRAME_MIN ||
	    num_words > SAM_SSC_WORD_PER_FRAME_MAX) {
		LOG_ERR("Unsupported words per frame number");
		return -EINVAL;
	}

	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	bit_clk_freq = i2s_cfg->frame_clk_freq * word_size_bits * num_words;
	ret = bit_clock_set(dev, bit_clk_freq);
	if (ret < 0) {
		return ret;
	}

	ret = stream->set_data_format(dev_cfg, i2s_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Set up DMA channel parameters */
	stream->word_size_bytes = get_word_size_bytes(word_size_bits);

	if (i2s_cfg->options & I2S_OPT_LOOPBACK) {
		ssc->SSC_RFMR |= SSC_RFMR_LOOP;
	}

	stream->state = I2S_STATE_READY;

	return 0;
}

static int rx_stream_start(const struct device *dev, struct stream *stream)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	Ssc *const ssc = dev_cfg->regs;
	int ret;
	uint32_t ier_flags = SSC_IER_OVRUN;

	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block,
			       K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma) {
		/* Workaround for a hardware bug: DMA engine will read first data
		 * item even if SSC_SR.RXEN (Receive Enable) is not set. An extra read
		 * before enabling DMA engine sets hardware FSM in the correct state.
		 */
		(void)ssc->SSC_RHR;

		struct dma_config dma_cfg = {
			.source_data_size = stream->word_size_bytes,
			.dest_data_size = stream->word_size_bytes,
			.block_count = 1,
			.dma_slot = stream->dma_perid,
			.channel_direction = PERIPHERAL_TO_MEMORY,
			.source_burst_length = 1,
			.dest_burst_length = 1,
			.dma_callback = dma_rx_callback,
			.user_data = (void *)dev,
		};

		ret = start_dma(dev_cfg->dev_dma, stream->dma_channel, &dma_cfg,
				(void *)&(ssc->SSC_RHR), stream->mem_block,
				stream->cfg.block_size);
		if (ret < 0) {
			LOG_ERR("Failed to start RX DMA transfer: %d", ret);
			return ret;
		}
	} else
#endif /* CONFIG_I2S_SAM_SSC_DMA */
	{
		ier_flags |= SSC_IER_RXRDY;
	}

	/* Clear status register */
	(void)ssc->SSC_SR;

	ssc->SSC_IER = ier_flags;

	ssc->SSC_CR = SSC_CR_RXEN;

	return 0;
}

static int tx_stream_start(const struct device *dev, struct stream *stream)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	Ssc *const ssc = dev_cfg->regs;
	int ret;

	ret = k_msgq_get(stream->mem_block_queue, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}
	stream->mem_block_offset = 0;

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma) {
		/* Workaround for a hardware bug: DMA engine will transfer first data
		 * item even if SSC_SR.TXEN (Transmit Enable) is not set. An extra write
		 * before enabling DMA engine sets hardware FSM in the correct state.
		 * This data item will not be output on I2S interface.
		 */
		ssc->SSC_THR = 0;

		struct dma_config dma_cfg = {
			.source_data_size = stream->word_size_bytes,
			.dest_data_size = stream->word_size_bytes,
			.block_count = 1,
			.dma_slot = stream->dma_perid,
			.channel_direction = MEMORY_TO_PERIPHERAL,
			.source_burst_length = 1,
			.dest_burst_length = 1,
			.dma_callback = dma_tx_callback,
			.user_data = (void *)dev,
		};

		/* Assure cache coherency before DMA read operation */
		sys_cache_data_flush_range(stream->mem_block, stream->cfg.block_size);

		ret = start_dma(dev_cfg->dev_dma, stream->dma_channel, &dma_cfg,
				stream->mem_block, (void *)&(ssc->SSC_THR),
				stream->cfg.block_size);
		if (ret < 0) {
			LOG_ERR("Failed to start TX DMA transfer: %d", ret);
			return ret;
		}
	} else
#endif /* CONFIG_I2S_SAM_SSC_DMA */
	{
		switch (stream->word_size_bytes) {
		case 1:
			ssc->SSC_THR = ((uint8_t *)stream->mem_block)[0];
			break;
		case 2:
			ssc->SSC_THR = ((uint16_t *)stream->mem_block)[0];
			break;
		case 4:
			ssc->SSC_THR = ((uint32_t *)stream->mem_block)[0];
			break;
		default:
			return -EINVAL;
		}
	}

	/* Clear status register */
	(void)ssc->SSC_SR;

	ssc->SSC_IER = SSC_IER_TXEMPTY;

	ssc->SSC_CR = SSC_CR_TXEN;

	return 0;
}

static void rx_stream_disable(const struct device *dev, struct stream *stream)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	Ssc *const ssc = dev_cfg->regs;

	ssc->SSC_CR = SSC_CR_RXDIS;
	ssc->SSC_IDR = SSC_IDR_OVRUN | SSC_IDR_RXRDY;

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma) {
		dma_stop(dev_cfg->dev_dma, stream->dma_channel);
	}
#endif /* CONFIG_I2S_SAM_SSC_DMA */

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_offset = 0;
	}
}

static void tx_stream_disable(const struct device *dev, struct stream *stream)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	Ssc *const ssc = dev_cfg->regs;

	ssc->SSC_CR = SSC_CR_TXDIS;
	ssc->SSC_IDR = SSC_IDR_TXEMPTY;

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma) {
		dma_stop(dev_cfg->dev_dma, stream->dma_channel);
	}
#endif /* CONFIG_I2S_SAM_SSC_DMA */

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_offset = 0;
	}
}

static void stream_queue_drop(struct stream *stream)
{
	void *mem_block;

	while (k_msgq_get(stream->mem_block_queue, &mem_block, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, &mem_block);
	}
}

static int i2s_sam_trigger(const struct device *dev, enum i2s_dir dir,
			   enum i2s_trigger_cmd cmd)
{
	struct i2s_sam_dev_data *const dev_data = dev->data;
	struct stream *stream;
	unsigned int key;
	int ret;

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

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_DBG("START trigger: invalid state");
			return -EIO;
		}

		__ASSERT_NO_MSG(stream->mem_block == NULL);

		ret = stream->stream_start(dev, stream);
		if (ret < 0) {
			LOG_DBG("START trigger failed %d", ret);
			return ret;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;
		break;

	case I2S_TRIGGER_STOP:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_DBG("STOP trigger: invalid state");
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
			LOG_DBG("DRAIN trigger: invalid state");
			return -EIO;
		}
		stream->state = I2S_STATE_STOPPING;
		irq_unlock(key);
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_DBG("DROP trigger: invalid state");
			return -EIO;
		}
		stream->stream_disable(dev, stream);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_DBG("PREPARE trigger: invalid state");
			return -EIO;
		}
		stream->state = I2S_STATE_READY;
		stream->queue_drop(stream);
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		return -EINVAL;
	}

	return 0;
}

static int i2s_sam_read(const struct device *dev, void **mem_block,
			size_t *size)
{
	struct i2s_sam_dev_data *const dev_data = dev->data;
	int ret;

	if (dev_data->rx.state == I2S_STATE_NOT_READY) {
		LOG_DBG("invalid state");
		return -EIO;
	}

	ret = k_msgq_get(dev_data->rx.mem_block_queue,
			 mem_block,
			 (dev_data->rx.state == I2S_STATE_ERROR)
				? K_NO_WAIT
				: SYS_TIMEOUT_MS(dev_data->rx.cfg.timeout));
	if (ret == -ENOMSG) {
		return -EIO;
	}

	if (ret == 0) {
		*size = dev_data->rx.cfg.block_size;
	}

	return ret;
}

static int i2s_sam_write(const struct device *dev, void *mem_block,
			 size_t size)
{
	struct i2s_sam_dev_data *const dev_data = dev->data;

	if (dev_data->tx.state != I2S_STATE_RUNNING &&
	    dev_data->tx.state != I2S_STATE_READY) {
		LOG_DBG("invalid state");
		return -EIO;
	}

	if (size != dev_data->tx.cfg.block_size) {
		LOG_ERR("This device can only write blocks of %u bytes",
			dev_data->tx.cfg.block_size);
		return -EIO;
	}

	return k_msgq_put(dev_data->tx.mem_block_queue, &mem_block,
			  SYS_TIMEOUT_MS(dev_data->tx.cfg.timeout));
}

static void i2s_sam_isr(const struct device *dev)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	struct i2s_sam_dev_data *const dev_data = dev->data;
	Ssc *const ssc = dev_cfg->regs;
	uint32_t isr_status;

	/* Retrieve interrupt status */
	isr_status = ssc->SSC_SR & ssc->SSC_IMR;

	/* Check for RX buffer overrun */
	if (isr_status & SSC_SR_OVRUN) {
		dev_data->rx.state = I2S_STATE_ERROR;
		/* Disable interrupt */
		ssc->SSC_IDR = SSC_IDR_OVRUN;
		LOG_DBG("RX buffer overrun error");
	}
	if (isr_status & SSC_SR_TXEMPTY) {
#ifdef CONFIG_I2S_SAM_SSC_DMA
		/* Check for TX buffer underrun */
		if (dev_cfg->dev_dma) {
			dev_data->tx.state = I2S_STATE_ERROR;
			/* Disable interrupt */
			ssc->SSC_IDR = SSC_IDR_TXEMPTY;
			LOG_DBG("TX buffer underrun error");
		} else
#endif /* CONFIG_I2S_SAM_SSC_DMA */
		{
			tx_callback(dev, 0, dev_data->tx.word_size_bytes);
		}
	}
	if (isr_status & SSC_SR_RXRDY) {
		struct stream *stream = &dev_data->rx;
		mem_addr_t addr = (mem_addr_t)stream->mem_block;
		uint32_t val = ssc->SSC_RHR;

		addr += stream->mem_block_offset;

		switch (stream->word_size_bytes) {
		case 1:
			((uint8_t *)addr)[0] = val & 0xff;
			break;
		case 2:
			((uint16_t *)addr)[0] = val & 0xffff;
			break;
		case 4:
			((uint32_t *)addr)[0] = val;
			break;
		default:
			rx_stream_disable(dev, stream);
			return;
		}

		rx_callback(dev, 0, stream->word_size_bytes);
	}
}

static int i2s_sam_init(const struct device *dev)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = dev->config;
	Ssc *const ssc = dev_cfg->regs;
	int ret;

#ifdef CONFIG_I2S_SAM_SSC_DMA
	if (dev_cfg->dev_dma != NULL && !device_is_ready(dev_cfg->dev_dma)) {
		LOG_ERR("%s device not ready", dev_cfg->dev_dma->name);
		return -ENODEV;
	}
#endif /* CONFIG_I2S_SAM_SSC_DMA */

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Enable SSC clock in PMC */
	ret = clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&dev_cfg->clock_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to enable SSC clock (%d)", ret);
		return ret;
	}

	/* Reset the module, disable receiver & transmitter */
	ssc->SSC_CR = SSC_CR_RXDIS | SSC_CR_TXDIS | SSC_CR_SWRST;

	/* Enable module's IRQ */
	dev_cfg->irq_config();

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct i2s_driver_api i2s_sam_driver_api = {
	.configure = i2s_sam_configure,
	.config_get = i2s_sam_config_get,
	.read = i2s_sam_read,
	.write = i2s_sam_write,
	.trigger = i2s_sam_trigger,
};

#define I2S_SAM_DEV_DMA_DEFINE(inst)							\
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, tx)),

#define I2S_SAM_STREAM_DMA_DEFINE(inst, name)						\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(inst, name, channel),			\
	.dma_perid = DT_INST_DMAS_CELL_BY_NAME(inst, name, perid),

#ifdef CONFIG_I2S_SAM_SSC_DMA
#define I2S_SAM_USE_DMA(inst) DT_INST_DMAS_HAS_NAME(inst, tx)
#else
#define I2S_SAM_USE_DMA(inst) 0
#endif

#define I2S_SAM_DEFINE(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
	static void i2s_sam_irq_config_##inst(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(inst),						\
			    DT_INST_IRQ(inst, priority),				\
			    i2s_sam_isr,						\
			    DEVICE_DT_INST_GET(inst), 0);				\
		irq_enable(DT_INST_IRQN(inst));						\
	}										\
	static const struct i2s_sam_dev_cfg i2s_sam_config_##inst = {			\
		COND_CODE_1(I2S_SAM_USE_DMA(inst), (I2S_SAM_DEV_DMA_DEFINE(inst)), ())	\
		.regs = (Ssc *)DT_INST_REG_ADDR(inst),					\
		.irq_config = i2s_sam_irq_config_##inst,				\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.pin_rk_en = DT_INST_PROP(inst, rk_enabled),				\
		.pin_rf_en = DT_INST_PROP(inst, rf_enabled),				\
	};										\
	K_MSGQ_DEFINE(rx_msgs_##inst, sizeof(void *),					\
		      CONFIG_I2S_SAM_SSC_RX_BLOCK_COUNT, 4);				\
	K_MSGQ_DEFINE(tx_msgs_##inst, sizeof(void *),					\
		      CONFIG_I2S_SAM_SSC_TX_BLOCK_COUNT, 4);				\
	static struct i2s_sam_dev_data i2s_sam_data_##inst = {				\
		.rx = {									\
			COND_CODE_1(I2S_SAM_USE_DMA(inst),				\
				(I2S_SAM_STREAM_DMA_DEFINE(inst, rx)), ())		\
			.mem_block_queue = &rx_msgs_##inst,				\
			.stream_start = rx_stream_start,				\
			.stream_disable = rx_stream_disable,				\
			.queue_drop = stream_queue_drop,				\
			.set_data_format = set_rx_data_format,				\
		},									\
		.tx = {									\
			COND_CODE_1(I2S_SAM_USE_DMA(inst),				\
				(I2S_SAM_STREAM_DMA_DEFINE(inst, tx)), ())		\
			.mem_block_queue = &tx_msgs_##inst,				\
			.stream_start = tx_stream_start,				\
			.stream_disable = tx_stream_disable,				\
			.queue_drop = stream_queue_drop,				\
			.set_data_format = set_tx_data_format,				\
		},									\
	};										\
	DEVICE_DT_INST_DEFINE(inst, i2s_sam_init, NULL,					\
			      &i2s_sam_data_##inst, &i2s_sam_config_##inst,		\
			      POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,			\
			      &i2s_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_SAM_DEFINE)
