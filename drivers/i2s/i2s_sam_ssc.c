/*
 * Copyright (c) 2017 comsuisse AG
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
#include <sys/__assert.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/dma.h>
#include <drivers/i2s.h>
#include <soc.h>

#define LOG_DOMAIN dev_i2s_sam_ssc
#define LOG_LEVEL CONFIG_I2S_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#if __DCACHE_PRESENT == 1
#define DCACHE_INVALIDATE(addr, size) \
	SCB_InvalidateDCache_by_Addr((uint32_t *)addr, size)
#define DCACHE_CLEAN(addr, size) \
	SCB_CleanDCache_by_Addr((uint32_t *)addr, size)
#else
#define DCACHE_INVALIDATE(addr, size) {; }
#define DCACHE_CLEAN(addr, size) {; }
#endif

#define SAM_SSC_WORD_SIZE_BITS_MIN    2
#define SAM_SSC_WORD_SIZE_BITS_MAX   32
#define SAM_SSC_WORD_PER_FRAME_MIN    1
#define SAM_SSC_WORD_PER_FRAME_MAX   16

struct queue_item {
	void *mem_block;
	size_t size;
};

/* Minimal ring buffer implementation */
struct ring_buf {
	struct queue_item *buf;
	uint16_t len;
	uint16_t head;
	uint16_t tail;
};

/* Device constant configuration parameters */
struct i2s_sam_dev_cfg {
	Ssc *regs;
	void (*irq_config)(void);
	const struct soc_gpio_pin *pin_list;
	uint8_t pin_list_size;
	uint8_t periph_id;
	uint8_t irq_id;
};

struct stream {
	int32_t state;
	struct k_sem sem;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct i2s_config cfg;
	struct ring_buf mem_block_queue;
	void *mem_block;
	bool last_block;
	int (*stream_start)(struct stream *, Ssc *const,
			    const struct device *);
	void (*stream_disable)(struct stream *, Ssc *const,
			       const struct device *);
	void (*queue_drop)(struct stream *);
	int (*set_data_format)(const struct i2s_sam_dev_cfg *const,
			       struct i2s_config *);
};

/* Device run time data */
struct i2s_sam_dev_data {
	const struct device *dev_dma;
	struct stream rx;
	struct stream tx;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((const struct i2s_sam_dev_cfg *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct i2s_sam_dev_data *const)(dev)->data)

#define MODULO_INC(val, max) { val = (++val < max) ? val : 0; }

static const struct device *get_dev_from_dma_channel(uint32_t dma_channel);
static void dma_rx_callback(const struct device *, void *, uint32_t, int);
static void dma_tx_callback(const struct device *, void *, uint32_t, int);
static void rx_stream_disable(struct stream *, Ssc *const,
			      const struct device *);
static void tx_stream_disable(struct stream *, Ssc *const,
			      const struct device *);

/*
 * Get data from the queue
 */
static int queue_get(struct ring_buf *rb, void **mem_block, size_t *size)
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
	MODULO_INC(rb->tail, rb->len);

	irq_unlock(key);

	return 0;
}

/*
 * Put data in the queue
 */
static int queue_put(struct ring_buf *rb, void *mem_block, size_t size)
{
	uint16_t head_next;
	unsigned int key;

	key = irq_lock();

	head_next = rb->head;
	MODULO_INC(head_next, rb->len);

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

/* This function is executed in the interrupt context */
static void dma_rx_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	const struct device *dev = get_dev_from_dma_channel(channel);
	const struct i2s_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	Ssc *const ssc = dev_cfg->regs;
	struct stream *stream = &dev_data->rx;
	int ret;

	ARG_UNUSED(user_data);
	__ASSERT_NO_MSG(stream->mem_block != NULL);

	/* Stop reception if there was an error */
	if (stream->state == I2S_STATE_ERROR) {
		goto rx_disable;
	}

	/* Assure cache coherency after DMA write operation */
	DCACHE_INVALIDATE(stream->mem_block, stream->cfg.block_size);

	/* All block data received */
	ret = queue_put(&stream->mem_block_queue, stream->mem_block,
			stream->cfg.block_size);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}
	stream->mem_block = NULL;
	k_sem_give(&stream->sem);

	/* Stop reception if we were requested */
	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		goto rx_disable;
	}

	/* Prepare to receive the next data block */
	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block,
			       K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	ret = start_dma(dev_data->dev_dma, stream->dma_channel, &stream->dma_cfg,
			(void *)&(ssc->SSC_RHR), stream->mem_block,
			stream->cfg.block_size);
	if (ret < 0) {
		LOG_DBG("Failed to start RX DMA transfer: %d", ret);
		goto rx_disable;
	}

	return;

rx_disable:
	rx_stream_disable(stream, ssc, dev_data->dev_dma);
}

/* This function is executed in the interrupt context */
static void dma_tx_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	const struct device *dev = get_dev_from_dma_channel(channel);
	const struct i2s_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	Ssc *const ssc = dev_cfg->regs;
	struct stream *stream = &dev_data->tx;
	size_t mem_block_size;
	int ret;

	ARG_UNUSED(user_data);
	__ASSERT_NO_MSG(stream->mem_block != NULL);

	/* All block data sent */
	k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
	stream->mem_block = NULL;

	/* Stop transmission if there was an error */
	if (stream->state == I2S_STATE_ERROR) {
		LOG_DBG("TX error detected");
		goto tx_disable;
	}

	/* Stop transmission if we were requested */
	if (stream->last_block) {
		stream->state = I2S_STATE_READY;
		goto tx_disable;
	}

	/* Prepare to send the next data block */
	ret = queue_get(&stream->mem_block_queue, &stream->mem_block,
			&mem_block_size);
	if (ret < 0) {
		if (stream->state == I2S_STATE_STOPPING) {
			stream->state = I2S_STATE_READY;
		} else {
			stream->state = I2S_STATE_ERROR;
		}
		goto tx_disable;
	}
	k_sem_give(&stream->sem);

	/* Assure cache coherency before DMA read operation */
	DCACHE_CLEAN(stream->mem_block, mem_block_size);

	ret = start_dma(dev_data->dev_dma, stream->dma_channel, &stream->dma_cfg,
			stream->mem_block, (void *)&(ssc->SSC_THR),
			mem_block_size);
	if (ret < 0) {
		LOG_DBG("Failed to start TX DMA transfer: %d", ret);
		goto tx_disable;
	}

	return;

tx_disable:
	tx_stream_disable(stream, ssc, dev_data->dev_dma);
}

static int set_rx_data_format(const struct i2s_sam_dev_cfg *const dev_cfg,
			      struct i2s_config *i2s_cfg)
{
	Ssc *const ssc = dev_cfg->regs;
	const bool pin_rk_en = IS_ENABLED(CONFIG_I2S_SAM_SSC_0_PIN_RK_EN);
	const bool pin_rf_en = IS_ENABLED(CONFIG_I2S_SAM_SSC_0_PIN_RF_EN);
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
			   | (pin_rf_en ? SSC_RCMR_START_RF_FALLING : 0)
			   | SSC_RCMR_STTDLY(1);

		ssc_rfmr = (pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_NEGATIVE : SSC_RFMR_FSOS_NONE);
		break;

	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		ssc_rcmr = (pin_rf_en ? SSC_RCMR_START_RF_FALLING : 0)
			   | SSC_RCMR_STTDLY(0);

		ssc_rfmr = (pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_POSITIVE : SSC_RFMR_FSOS_NONE);
		break;

	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		fslen = num_words * word_size_bits / 2U - 1;

		ssc_rcmr = (pin_rf_en ? SSC_RCMR_START_RF_RISING : 0)
			   | SSC_RCMR_STTDLY(0);

		ssc_rfmr = (pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_POSITIVE : SSC_RFMR_FSOS_NONE);
		break;

	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		fslen = num_words * word_size_bits / 2U - 1;

		ssc_rcmr = SSC_RCMR_CKI
			   | (pin_rf_en ? SSC_RCMR_START_RF_RISING : 0)
			   | SSC_RCMR_STTDLY(0);

		ssc_rfmr = (pin_rf_en && frame_clk_master
			    ? SSC_RFMR_FSOS_POSITIVE : SSC_RFMR_FSOS_NONE);
		break;

	default:
		LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	if (pin_rk_en) {
		ssc_rcmr |= ((i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE)
			     ? SSC_RCMR_CKS_RK : SSC_RCMR_CKS_MCK)
			    | ((i2s_cfg->options & I2S_OPT_BIT_CLK_GATED)
			       ? SSC_RCMR_CKO_TRANSFER : SSC_RCMR_CKO_CONTINUOUS);
	} else {
		ssc_rcmr |= SSC_RCMR_CKS_TK
			    | SSC_RCMR_CKO_NONE;
	}
	/* SSC_RCMR.PERIOD bit filed does not support setting the
	 * frame period with one bit resolution. In case the required
	 * frame period is an odd number set it to be one bit longer.
	 */
	ssc_rcmr |= (pin_rf_en ? 0 : SSC_RCMR_START_TRANSMIT)
		    | SSC_RCMR_PERIOD((num_words * word_size_bits + 1) / 2U - 1);

	/* Receive Clock Mode Register */
	ssc->SSC_RCMR = ssc_rcmr;

	ssc_rfmr |= SSC_RFMR_DATLEN(word_size_bits - 1)
		    | ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB)
		       ? 0 : SSC_RFMR_MSBF)
		    | SSC_RFMR_DATNB(num_words - 1)
		    | SSC_RFMR_FSLEN(fslen)
		    | SSC_RFMR_FSLEN_EXT(fslen >> 4);

	/* Receive Frame Mode Register */
	ssc->SSC_RFMR = ssc_rfmr;

	return 0;
}

static int set_tx_data_format(const struct i2s_sam_dev_cfg *const dev_cfg,
			      struct i2s_config *i2s_cfg)
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
	ssc_tcmr |= ((i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE)
		     ? SSC_TCMR_CKS_TK : SSC_TCMR_CKS_MCK)
		    | ((i2s_cfg->options & I2S_OPT_BIT_CLK_GATED)
		       ? SSC_TCMR_CKO_TRANSFER : SSC_TCMR_CKO_CONTINUOUS)
		    | SSC_TCMR_PERIOD((num_words * word_size_bits + 1) / 2U - 1);

	/* Transmit Clock Mode Register */
	ssc->SSC_TCMR = ssc_tcmr;

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) {
		ssc_tfmr &= ~SSC_TFMR_FSOS_Msk;
		ssc_tfmr |= SSC_TFMR_FSOS_NONE;
	}

	ssc_tfmr |= SSC_TFMR_DATLEN(word_size_bits - 1)
		    | ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB)
		       ? 0 : SSC_TFMR_MSBF)
		    | SSC_TFMR_DATNB(num_words - 1)
		    | SSC_TFMR_FSLEN(fslen)
		    | SSC_TFMR_FSLEN_EXT(fslen >> 4);

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

static int bit_clock_set(Ssc *const ssc, uint32_t bit_clk_freq)
{
	uint32_t clk_div = SOC_ATMEL_SAM_MCK_FREQ_HZ / bit_clk_freq / 2U;

	if (clk_div == 0U || clk_div >= (1 << 12)) {
		LOG_ERR("Invalid bit clock frequency");
		return -EINVAL;
	}

	ssc->SSC_CMR = clk_div;

	LOG_DBG("freq = %d", bit_clk_freq);

	return 0;
}

static struct i2s_config *i2s_sam_config_get(const struct device *dev,
					     enum i2s_dir dir)
{
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
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
			     struct i2s_config *i2s_cfg)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	Ssc *const ssc = dev_cfg->regs;
	uint8_t num_words = i2s_cfg->channels;
	uint8_t word_size_bits = i2s_cfg->word_size;
	uint8_t word_size_bytes;
	uint32_t bit_clk_freq;
	struct stream *stream;
	int ret;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
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
	ret = bit_clock_set(ssc, bit_clk_freq);
	if (ret < 0) {
		return ret;
	}

	ret = stream->set_data_format(dev_cfg, i2s_cfg);
	if (ret < 0) {
		return ret;
	}

	word_size_bytes = get_word_size_bytes(word_size_bits);

	/* Set up DMA channel parameters */
	stream->dma_cfg.source_data_size = word_size_bytes;
	stream->dma_cfg.dest_data_size = word_size_bytes;

	if (i2s_cfg->options & I2S_OPT_LOOPBACK) {
		ssc->SSC_RFMR |= SSC_RFMR_LOOP;
	}

	stream->state = I2S_STATE_READY;

	return 0;
}

static int rx_stream_start(struct stream *stream, Ssc *const ssc,
			   const struct device *dev_dma)
{
	int ret;

	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block,
			       K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	/* Workaround for a hardware bug: DMA engine will read first data
	 * item even if SSC_SR.RXEN (Receive Enable) is not set. An extra read
	 * before enabling DMA engine sets hardware FSM in the correct state.
	 */
	(void)ssc->SSC_RHR;

	ret = start_dma(dev_dma, stream->dma_channel, &stream->dma_cfg,
			(void *)&(ssc->SSC_RHR), stream->mem_block,
			stream->cfg.block_size);
	if (ret < 0) {
		LOG_ERR("Failed to start RX DMA transfer: %d", ret);
		return ret;
	}

	/* Clear status register */
	(void)ssc->SSC_SR;

	ssc->SSC_IER = SSC_IER_OVRUN;

	ssc->SSC_CR = SSC_CR_RXEN;

	return 0;
}

static int tx_stream_start(struct stream *stream, Ssc *const ssc,
			   const struct device *dev_dma)
{
	size_t mem_block_size;
	int ret;

	ret = queue_get(&stream->mem_block_queue, &stream->mem_block,
			&mem_block_size);
	if (ret < 0) {
		return ret;
	}
	k_sem_give(&stream->sem);

	/* Workaround for a hardware bug: DMA engine will transfer first data
	 * item even if SSC_SR.TXEN (Transmit Enable) is not set. An extra write
	 * before enabling DMA engine sets hardware FSM in the correct state.
	 * This data item will not be output on I2S interface.
	 */
	ssc->SSC_THR = 0;

	/* Assure cache coherency before DMA read operation */
	DCACHE_CLEAN(stream->mem_block, mem_block_size);

	ret = start_dma(dev_dma, stream->dma_channel, &stream->dma_cfg,
			stream->mem_block, (void *)&(ssc->SSC_THR),
			mem_block_size);
	if (ret < 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", ret);
		return ret;
	}

	/* Clear status register */
	(void)ssc->SSC_SR;

	ssc->SSC_IER = SSC_IER_TXEMPTY;

	ssc->SSC_CR = SSC_CR_TXEN;

	return 0;
}

static void rx_stream_disable(struct stream *stream, Ssc *const ssc,
			      const struct device *dev_dma)
{
	ssc->SSC_CR = SSC_CR_RXDIS;
	ssc->SSC_IDR = SSC_IDR_OVRUN;
	dma_stop(dev_dma, stream->dma_channel);
	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
	}
}

static void tx_stream_disable(struct stream *stream, Ssc *const ssc,
			      const struct device *dev_dma)
{
	ssc->SSC_CR = SSC_CR_TXDIS;
	ssc->SSC_IDR = SSC_IDR_TXEMPTY;
	dma_stop(dev_dma, stream->dma_channel);
	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
	}
}

static void rx_queue_drop(struct stream *stream)
{
	size_t size;
	void *mem_block;

	while (queue_get(&stream->mem_block_queue, &mem_block, &size) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, &mem_block);
	}

	k_sem_reset(&stream->sem);
}

static void tx_queue_drop(struct stream *stream)
{
	size_t size;
	void *mem_block;
	unsigned int n = 0U;

	while (queue_get(&stream->mem_block_queue, &mem_block, &size) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, &mem_block);
		n++;
	}

	for (; n > 0; n--) {
		k_sem_give(&stream->sem);
	}
}

static int i2s_sam_trigger(const struct device *dev, enum i2s_dir dir,
			   enum i2s_trigger_cmd cmd)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	Ssc *const ssc = dev_cfg->regs;
	struct stream *stream;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
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

		ret = stream->stream_start(stream, ssc, dev_data->dev_dma);
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
		stream->stream_disable(stream, ssc, dev_data->dev_dma);
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
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	int ret;

	if (dev_data->rx.state == I2S_STATE_NOT_READY) {
		LOG_DBG("invalid state");
		return -EIO;
	}

	if (dev_data->rx.state != I2S_STATE_ERROR) {
		ret = k_sem_take(&dev_data->rx.sem,
				 SYS_TIMEOUT_MS(dev_data->rx.cfg.timeout));
		if (ret < 0) {
			return ret;
		}
	}

	/* Get data from the beginning of RX queue */
	ret = queue_get(&dev_data->rx.mem_block_queue, mem_block, size);
	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

static int i2s_sam_write(const struct device *dev, void *mem_block,
			 size_t size)
{
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	int ret;

	if (dev_data->tx.state != I2S_STATE_RUNNING &&
	    dev_data->tx.state != I2S_STATE_READY) {
		LOG_DBG("invalid state");
		return -EIO;
	}

	ret = k_sem_take(&dev_data->tx.sem,
			 SYS_TIMEOUT_MS(dev_data->tx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	/* Add data to the end of the TX queue */
	queue_put(&dev_data->tx.mem_block_queue, mem_block, size);

	return 0;
}

static void i2s_sam_isr(const struct device *dev)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
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
	/* Check for TX buffer underrun */
	if (isr_status & SSC_SR_TXEMPTY) {
		dev_data->tx.state = I2S_STATE_ERROR;
		/* Disable interrupt */
		ssc->SSC_IDR = SSC_IDR_TXEMPTY;
		LOG_DBG("TX buffer underrun error");
	}
}

static int i2s_sam_initialize(const struct device *dev)
{
	const struct i2s_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2s_sam_dev_data *const dev_data = DEV_DATA(dev);
	Ssc *const ssc = dev_cfg->regs;

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Initialize semaphores */
	k_sem_init(&dev_data->rx.sem, 0, CONFIG_I2S_SAM_SSC_RX_BLOCK_COUNT);
	k_sem_init(&dev_data->tx.sem, CONFIG_I2S_SAM_SSC_TX_BLOCK_COUNT,
		   CONFIG_I2S_SAM_SSC_TX_BLOCK_COUNT);

	dev_data->dev_dma = device_get_binding(DT_INST_DMAS_LABEL_BY_NAME(0, tx));
	if (!dev_data->dev_dma) {
		LOG_ERR("%s device not found", DT_INST_DMAS_LABEL_BY_NAME(0, tx));
		return -ENODEV;
	}

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(dev_cfg->pin_list, dev_cfg->pin_list_size);

	/* Enable module's clock */
	soc_pmc_peripheral_enable(dev_cfg->periph_id);

	/* Reset the module, disable receiver & transmitter */
	ssc->SSC_CR = SSC_CR_RXDIS | SSC_CR_TXDIS | SSC_CR_SWRST;

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct i2s_driver_api i2s_sam_driver_api = {
	.configure = i2s_sam_configure,
	.config_get = i2s_sam_config_get,
	.read = i2s_sam_read,
	.write = i2s_sam_write,
	.trigger = i2s_sam_trigger,
};

/* I2S0 */

DEVICE_DECLARE(i2s0_sam);

static const struct device *get_dev_from_dma_channel(uint32_t dma_channel)
{
	return &DEVICE_NAME_GET(i2s0_sam);
}

static void i2s0_sam_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), i2s_sam_isr,
		    DEVICE_GET(i2s0_sam), 0);
}

static const struct soc_gpio_pin i2s0_pins[] = ATMEL_SAM_DT_PINS(0);

static const struct i2s_sam_dev_cfg i2s0_sam_config = {
	.regs = (Ssc *)DT_INST_REG_ADDR(0),
	.irq_config = i2s0_sam_irq_config,
	.periph_id = DT_INST_PROP(0, peripheral_id),
	.irq_id = DT_INST_IRQN(0),
	.pin_list = i2s0_pins,
	.pin_list_size = ARRAY_SIZE(i2s0_pins),
};

struct queue_item rx_0_ring_buf[CONFIG_I2S_SAM_SSC_RX_BLOCK_COUNT + 1];
struct queue_item tx_0_ring_buf[CONFIG_I2S_SAM_SSC_TX_BLOCK_COUNT + 1];

static struct i2s_sam_dev_data i2s0_sam_data = {
	.rx = {
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(0, rx, channel),
		.dma_cfg = {
			.block_count = 1,
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(0, rx, perid),
			.channel_direction = PERIPHERAL_TO_MEMORY,
			.source_burst_length = 1,
			.dest_burst_length = 1,
			.dma_callback = dma_rx_callback,
		},
		.mem_block_queue.buf = rx_0_ring_buf,
		.mem_block_queue.len = ARRAY_SIZE(rx_0_ring_buf),
		.stream_start = rx_stream_start,
		.stream_disable = rx_stream_disable,
		.queue_drop = rx_queue_drop,
		.set_data_format = set_rx_data_format,
	},
	.tx = {
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(0, tx, channel),
		.dma_cfg = {
			.block_count = 1,
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(0, tx, perid),
			.channel_direction = MEMORY_TO_PERIPHERAL,
			.source_burst_length = 1,
			.dest_burst_length = 1,
			.dma_callback = dma_tx_callback,
		},
		.mem_block_queue.buf = tx_0_ring_buf,
		.mem_block_queue.len = ARRAY_SIZE(tx_0_ring_buf),
		.stream_start = tx_stream_start,
		.stream_disable = tx_stream_disable,
		.queue_drop = tx_queue_drop,
		.set_data_format = set_tx_data_format,
	},
};

DEVICE_AND_API_INIT(i2s0_sam, DT_INST_LABEL(0), &i2s_sam_initialize,
		    &i2s0_sam_data, &i2s0_sam_config, POST_KERNEL,
		    CONFIG_I2S_INIT_PRIORITY, &i2s_sam_driver_api);
