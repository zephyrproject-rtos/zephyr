/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2S bus (SSP) driver for Intel CAVS.
 *
 * Limitations:
 * - DMA is used in simple single block transfer mode (with linked list
 *   enabled) and "interrupt on full transfer completion" mode.
 */

#include <errno.h>
#include <string.h>
#include <misc/__assert.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <dma.h>
#include <i2s.h>
#include <soc.h>
#include "i2s_cavs.h"

#define SYS_LOG_DOMAIN "dev/i2s_cavs"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2S_LEVEL
#include <logging/sys_log.h>

#ifdef CONFIG_DCACHE_WRITEBACK
#define DCACHE_INVALIDATE(addr, size) \
		{ dcache_invalidate_region(addr, size); }
#define DCACHE_CLEAN(addr, size) \
		{ dcache_writeback_region(addr, size); }
#else
#define DCACHE_INVALIDATE(addr, size) \
		do { } while (0)

#define DCACHE_CLEAN(addr, size) \
		do { } while (0)
#endif

#define CAVS_SSP_WORD_SIZE_BITS_MIN	4
#define CAVS_SSP_WORD_SIZE_BITS_MAX	32
#define CAVS_SSP_WORD_PER_FRAME_MIN	1
#define CAVS_SSP_WORD_PER_FRAME_MAX	8

struct queue_item {
	void *mem_block;
	size_t size;
};

/* Minimal ring buffer implementation:
 * buf - holds the pointer to Queue items
 * len - Max number of Queue items that can be referenced
 * head - current write index number
 * tail - current read index number
 */
struct ring_buf {
	struct queue_item *buf;
	u16_t len;
	u16_t head;
	u16_t tail;
};

/* This indicates the Tx/Rx stream. Most members of the stream are
 * self-explanatory except for sem.
 * sem - This is initialized to CONFIG_I2S_CAVS_TX_BLOCK_COUNT. If at all
 * mem_block_queue gets filled with the MAX blocks configured, this semaphore
 * ensures nothing gets written into the mem_block_queue before a slot gets
 * freed (which happens when a block gets read out).
 */
struct stream {
	s32_t state;
	struct k_sem sem;
	u32_t dma_channel;
	struct dma_config dma_cfg;
	struct i2s_config cfg;
	struct ring_buf mem_block_queue;
	void *mem_block;
	bool last_block;
	int (*stream_start)(struct stream *,
			volatile struct i2s_cavs_ssp *const, struct device *);
	void (*stream_disable)(struct stream *,
			volatile struct i2s_cavs_ssp *const, struct device *);
	void (*queue_drop)(struct stream *);
};

struct i2s_cavs_config {
	struct i2s_cavs_ssp *regs;
	struct i2s_cavs_mn_div *mn_regs;
	u32_t irq_id;
	void (*irq_config)(void);
};

/* Device run time data */
struct i2s_cavs_dev_data {
	struct device *dev_dma;
	struct stream tx;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct i2s_cavs_config *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct i2s_cavs_dev_data *const)(dev)->driver_data)

static struct device *get_dev_from_dma_channel(u32_t dma_channel);
static void dma_tx_callback(struct device *, u32_t, int);
static void tx_stream_disable(struct stream *,
		volatile struct i2s_cavs_ssp *const, struct device *);

static inline u16_t modulo_inc(u16_t val, u16_t max)
{
	val++;
	return (val < max) ? val : 0;
}

/*
 * Get data from the queue
 */
static int queue_get(struct ring_buf *rb, u8_t mode, void **mem_block,
			size_t *size)
{
	unsigned int key;

	key = irq_lock();

	/* In case of ping-pong mode, the buffers are not freed after
	 * reading. They are fixed in size. Another thread will populate
	 * the pong buffer while the ping buffer is being read out and
	 * vice versa. Hence, we just need to keep reading from buffer0
	 * (ping buffer) followed by buffer1 (pong buffer) and the same
	 * cycle continues.
	 *
	 * In case of non-ping-pong modes, each buffer is freed after it
	 * is read. The tail pointer will keep progressing depending upon
	 * the reads. The head pointer will move whenever there's a write.
	 * If tail equals head, it would mean we have read everything there
	 * is and the buffer is empty.
	 */
	if (rb->tail == rb->head) {
		if ((mode & I2S_OPT_PINGPONG) == I2S_OPT_PINGPONG) {
			/* Point back to the first element */
			rb->tail = 0;
		} else {
			/* Ring buffer is empty */
			irq_unlock(key);
			return -ENOMEM;
		}
	}

	*mem_block = rb->buf[rb->tail].mem_block;
	*size = rb->buf[rb->tail].size;
	rb->tail = modulo_inc(rb->tail, rb->len);

	irq_unlock(key);

	return 0;
}

/*
 * Put data in the queue
 */
static int queue_put(struct ring_buf *rb, u8_t mode, void *mem_block,
			size_t size)
{
	u16_t head_next;
	unsigned int key;

	key = irq_lock();

	head_next = rb->head;
	head_next = modulo_inc(head_next, rb->len);

	/* In case of ping-pong mode, the below comparison incorrectly
	 * leads to complications as the buffers are always predefined.
	 * Hence excluding ping-pong mode from this comparison.
	 */
	if ((mode & I2S_OPT_PINGPONG) != I2S_OPT_PINGPONG) {
		if (head_next == rb->tail) {
			/* Ring buffer is full */
			irq_unlock(key);
			return -ENOMEM;
		}
	}

	rb->buf[rb->head].mem_block = mem_block;
	rb->buf[rb->head].size = size;
	rb->head = head_next;

	irq_unlock(key);

	return 0;
}

static int start_dma(struct device *dev_dma, u32_t channel,
		     struct dma_config *cfg, void *src, void *dst,
		     u32_t blk_size)
{
	int ret;

	struct dma_block_config blk_cfg = {
		.block_size = blk_size,
		.source_address = (u32_t)src,
		.dest_address = (u32_t)dst,
	};

	cfg->head_block = &blk_cfg;

	ret = dma_config(dev_dma, channel, cfg);
	if (ret < 0) {
		SYS_LOG_ERR("dma_config failed: %d", ret);
		return ret;
	}

	ret = dma_start(dev_dma, channel);
	if (ret < 0) {
		SYS_LOG_ERR("dma_start failed: %d", ret);
	}

	return ret;
}

/* This function is executed in the interrupt context */
static void dma_tx_callback(struct device *dev_dma, u32_t channel, int status)
{
	struct device *dev = get_dev_from_dma_channel(channel);
	const struct i2s_cavs_config *const dev_cfg = DEV_CFG(dev);
	struct i2s_cavs_dev_data *const dev_data = DEV_DATA(dev);
	volatile struct i2s_cavs_ssp *const ssp = dev_cfg->regs;
	struct stream *strm = &dev_data->tx;
	size_t mem_block_size;
	int ret;

	__ASSERT_NO_MSG(strm->mem_block != NULL);

	if ((strm->cfg.options & I2S_OPT_PINGPONG) != I2S_OPT_PINGPONG) {
		/* All block data sent */
		k_mem_slab_free(strm->cfg.mem_slab, &strm->mem_block);
		strm->mem_block = NULL;
	}

	/* Stop transmission if there was an error */
	if (strm->state == I2S_STATE_ERROR) {
		SYS_LOG_DBG("TX error detected");
		goto tx_disable;
	}

	/* Stop transmission if we were requested */
	if (strm->last_block) {
		strm->state = I2S_STATE_READY;
		goto tx_disable;
	}

	/* Prepare to send the next data block */
	ret = queue_get(&strm->mem_block_queue, strm->cfg.options,
			&strm->mem_block, &mem_block_size);
	if (ret < 0) {
		if (strm->state == I2S_STATE_STOPPING) {
			strm->state = I2S_STATE_READY;
		} else {
			strm->state = I2S_STATE_ERROR;
		}
		goto tx_disable;
	}

	k_sem_give(&strm->sem);

	/* Assure cache coherency before DMA read operation */
	DCACHE_CLEAN(strm->mem_block, mem_block_size);

	ret = start_dma(dev_data->dev_dma, strm->dma_channel, &strm->dma_cfg,
			strm->mem_block, (void *)&(ssp->ssd),
			mem_block_size);
	if (ret < 0) {
		SYS_LOG_DBG("Failed to start TX DMA transfer: %d", ret);
		goto tx_disable;
	}
	return;

tx_disable:
	tx_stream_disable(strm, ssp, dev_data->dev_dma);
}

static int i2s_cavs_configure(struct device *dev, enum i2s_dir dir,
			     struct i2s_config *i2s_cfg)
{
	const struct i2s_cavs_config *const dev_cfg = DEV_CFG(dev);
	struct i2s_cavs_dev_data *const dev_data = DEV_DATA(dev);
	volatile struct i2s_cavs_ssp *const ssp = dev_cfg->regs;
	volatile struct i2s_cavs_mn_div *const mn_div = dev_cfg->mn_regs;
	u8_t num_words = i2s_cfg->channels;
	u8_t word_size_bits = i2s_cfg->word_size;
	u8_t word_size_bytes;
	u32_t bit_clk_freq, mclk;
	struct stream *strm;

	u32_t ssc0;
	u32_t ssc1;
	u32_t ssc2;
	u32_t ssc3;
	u32_t sspsp;
	u32_t sspsp2;
	u32_t sstsa;
	u32_t ssrsa;
	u32_t ssto;
	u32_t ssioc = 0;
	u32_t mdiv;
	u32_t i2s_m = 0;
	u32_t i2s_n = 0;
	u32_t frame_len = 0;
	bool inverted_frame = false;

	if (dir == I2S_DIR_TX) {
		strm = &dev_data->tx;
	} else {
		SYS_LOG_ERR("TX direction must be selected");
		return -EINVAL;
	}

	if (strm->state != I2S_STATE_NOT_READY &&
	    strm->state != I2S_STATE_READY) {
		SYS_LOG_ERR("invalid state");
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0) {
		strm->queue_drop(strm);
		(void)memset(&strm->cfg, 0, sizeof(struct i2s_config));
		strm->state = I2S_STATE_NOT_READY;
		return 0;
	}

	if (word_size_bits < CAVS_SSP_WORD_SIZE_BITS_MIN ||
	    word_size_bits > CAVS_SSP_WORD_SIZE_BITS_MAX) {
		SYS_LOG_ERR("Unsupported I2S word size");
		return -EINVAL;
	}

	if (num_words < CAVS_SSP_WORD_PER_FRAME_MIN ||
	    num_words > CAVS_SSP_WORD_PER_FRAME_MAX) {
		SYS_LOG_ERR("Unsupported words per frame number");
		return -EINVAL;
	}

	memcpy(&strm->cfg, i2s_cfg, sizeof(struct i2s_config));

	/* reset SSP settings */
	/* sscr0 dynamic settings are DSS, EDSS, SCR, FRDC, ECS */
	ssc0 = SSCR0_MOD | SSCR0_PSP | SSCR0_RIM | SSCR0_TIM;

	/* sscr1 dynamic settings are SFRMDIR, SCLKDIR, SCFR */
	ssc1 = SSCR1_TTE | SSCR1_TTELP | SSCR1_RWOT | SSCR1_TRAIL;

	/* sscr2 dynamic setting is LJDFD */
	ssc2 = SSCR2_SDFD | SSCR2_TURM1;

	/* sscr3 dynamic settings are TFT, RFT */
	ssc3 = 0;

	/* sspsp dynamic settings are SCMODE, SFRMP, DMYSTRT, SFRMWDTH */
	sspsp = 0;

	/* sspsp2 no dynamic setting */
	sspsp2 = 0x0;

	/* ssto no dynamic setting */
	ssto = 0x0;

	/* sstsa dynamic setting is TTSA, set according to num_words */
	sstsa = SSTSA_TXEN | BIT_MASK(num_words);

	/* ssrsa dynamic setting is RTSA, set according to num_words */
	ssrsa = SSRSA_RXEN | BIT_MASK(num_words);

	if (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		/* set BCLK mode as slave */
		ssc1 |= SSCR1_SCLKDIR;
	} else {
		/* enable BCLK output */
		ssioc = SSIOC_SCOE;
	}

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) {
		/* set WCLK mode as slave */
		ssc1 |= SSCR1_SFRMDIR;
	}

	ssioc |= SSIOC_SFCR;

	/* clock signal polarity */
	switch (i2s_cfg->format & I2S_FMT_CLK_FORMAT_MASK) {
	case I2S_FMT_CLK_NF_NB:
		break;

	case I2S_FMT_CLK_NF_IB:
		sspsp |= SSPSP_SCMODE(2);
		inverted_frame = true; /* handled later with format */
		break;

	case I2S_FMT_CLK_IF_NB:
		break;

	case I2S_FMT_CLK_IF_IB:
		sspsp |= SSPSP_SCMODE(2);
		inverted_frame = true; /* handled later with format */
		break;

	default:
		SYS_LOG_ERR("Unsupported Clock format");
		return -EINVAL;
	}

	mclk = soc_get_ref_clk_freq();
	bit_clk_freq = i2s_cfg->frame_clk_freq * word_size_bits * num_words;

	/* BCLK is generated from MCLK - must be divisible */
	if (mclk % bit_clk_freq) {
		SYS_LOG_INF("MCLK/BCLK is not an integer, using M/N divider");

		/*
		 * Simplification: Instead of calculating lowest values of
		 * M and N, just set M and N as BCLK and MCLK respectively
		 * in 0.1KHz units
		 * In addition, double M so that it can be later divided by 2
		 * to get an approximately 50% duty cycle clock
		 */
		i2s_m = (bit_clk_freq << 1) / 100;
		i2s_n = mclk / 100;

		/* set divider value of 1 which divides the clock by 2 */
		mdiv = 1;

		/* Select M/N divider as the clock source */
		ssc0 |= SSCR0_ECS;
	} else {
		mdiv = (mclk / bit_clk_freq) - 1;
	}

	/* divisor must be within SCR range */
	if (mdiv > (SSCR0_SCR_MASK >> 8)) {
		SYS_LOG_ERR("Divisor is not within SCR range");
		return -EINVAL;
	}

	/* set the SCR divisor */
	ssc0 |= SSCR0_SCR(mdiv);

	/* format */
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {

	case I2S_FMT_DATA_FORMAT_I2S:
		ssc0 |= SSCR0_FRDC(i2s_cfg->channels);

		/* set asserted frame length */
		frame_len = word_size_bits;

		/* handle frame polarity, I2S default is falling/active low */
		sspsp |= SSPSP_SFRMP(!inverted_frame);
		break;

	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		ssc0 |= SSCR0_FRDC(i2s_cfg->channels);

		/* LJDFD enable */
		ssc2 &= ~SSCR2_LJDFD;

		/* set asserted frame length */
		frame_len = word_size_bits;

		/* LEFT_J default is rising/active high, opposite of I2S */
		sspsp |= SSPSP_SFRMP(inverted_frame);
		break;

	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
	default:
		SYS_LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	sspsp |= SSPSP_SFRMWDTH(frame_len);

	if (word_size_bits > 16) {
		ssc0 |= (SSCR0_EDSS | SSCR0_DSIZE(word_size_bits - 16));
	} else {
		ssc0 |= SSCR0_DSIZE(word_size_bits);
	}

	ssp->ssc0 = ssc0;
	ssp->ssc1 = ssc1;
	ssp->ssc2 = ssc2;
	ssp->ssc3 = ssc3;
	ssp->sspsp2 = sspsp2;
	ssp->sspsp = sspsp;
	ssp->ssioc = ssioc;
	ssp->ssto = ssto;
	ssp->sstsa = sstsa;
	ssp->ssrsa = ssrsa;

	mn_div->mval = I2S_MNVAL(i2s_m);
	mn_div->nval = I2S_MNVAL(i2s_n);

	/* Set up DMA channel parameters */
	word_size_bytes = (word_size_bits + 7) / 8;
	strm->dma_cfg.source_data_size = word_size_bytes;
	strm->dma_cfg.dest_data_size = word_size_bytes;

	strm->state = I2S_STATE_READY;
	return 0;
}

static int tx_stream_start(struct stream *strm,
			   volatile struct i2s_cavs_ssp *const ssp,
			   struct device *dev_dma)
{
	size_t mem_block_size;
	int ret;

	ret = queue_get(&strm->mem_block_queue, strm->cfg.options,
			&strm->mem_block, &mem_block_size);
	if (ret < 0) {
		return ret;
	}
	k_sem_give(&strm->sem);

	/* Assure cache coherency before DMA read operation */
	DCACHE_CLEAN(strm->mem_block, mem_block_size);

	ret = start_dma(dev_dma, strm->dma_channel, &strm->dma_cfg,
			strm->mem_block, (void *)&(ssp->ssd),
			mem_block_size);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to start TX DMA transfer: %d", ret);
		return ret;
	}

	/* enable port */
	ssp->ssc0 |= SSCR0_SSE;

	/* Enable DMA service request handshake logic. Though DMA is
	 * already started, it won't work without the handshake logic.
	 */
	ssp->ssc1 |= SSCR1_TSRE;
	ssp->sstsa |= (0x1 << 8);

	return 0;
}

static void tx_stream_disable(struct stream *strm,
			      volatile struct i2s_cavs_ssp *const ssp,
			      struct device *dev_dma)
{
	/* Disable DMA service request handshake logic. Handshake is
	 * not required now since DMA is not in operation.
	 */
	ssp->ssc1 &= ~SSCR1_TSRE;
	ssp->sstsa &= ~(0x1 << 8);

	dma_stop(dev_dma, strm->dma_channel);

	if (((strm->cfg.options & I2S_OPT_PINGPONG) != I2S_OPT_PINGPONG) &&
	    (strm->mem_block != NULL)) {
		k_mem_slab_free(strm->cfg.mem_slab, &strm->mem_block);
		strm->mem_block = NULL;
	}
	strm->mem_block_queue.head = 0;
	strm->mem_block_queue.tail = 0;
}

static void tx_queue_drop(struct stream *strm)
{
	size_t size;
	void *mem_block;
	unsigned int n = 0;

	while (queue_get(&strm->mem_block_queue, strm->cfg.options,
			 &mem_block, &size) == 0) {
		if ((strm->cfg.options & I2S_OPT_PINGPONG)
				!= I2S_OPT_PINGPONG) {
			k_mem_slab_free(strm->cfg.mem_slab, &mem_block);
			n++;
		}
	}

	strm->mem_block_queue.head = 0;
	strm->mem_block_queue.tail = 0;

	for (; n > 0; n--) {
		k_sem_give(&strm->sem);
	}
}

static int i2s_cavs_trigger(struct device *dev, enum i2s_dir dir,
			   enum i2s_trigger_cmd cmd)
{
	const struct i2s_cavs_config *const dev_cfg = DEV_CFG(dev);
	struct i2s_cavs_dev_data *const dev_data = DEV_DATA(dev);
	volatile struct i2s_cavs_ssp *const ssp = dev_cfg->regs;
	struct stream *strm;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_TX) {
		strm = &dev_data->tx;
	} else {
		SYS_LOG_ERR("TX direction must be selected");
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (strm->state != I2S_STATE_READY) {
			SYS_LOG_DBG("START trigger: invalid state");
			return -EIO;
		}

		__ASSERT_NO_MSG(strm->mem_block == NULL);

		ret = strm->stream_start(strm, ssp, dev_data->dev_dma);
		if (ret < 0) {
			SYS_LOG_DBG("START trigger failed %d", ret);
			return ret;
		}

		strm->state = I2S_STATE_RUNNING;
		strm->last_block = false;
		break;

	case I2S_TRIGGER_STOP:
		key = irq_lock();
		if (strm->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			SYS_LOG_DBG("STOP trigger: invalid state");
			return -EIO;
		}
		strm->state = I2S_STATE_STOPPING;
		irq_unlock(key);
		strm->last_block = true;
		break;

	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		if (strm->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			SYS_LOG_DBG("DRAIN trigger: invalid state");
			return -EIO;
		}
		strm->state = I2S_STATE_STOPPING;
		irq_unlock(key);
		break;

	case I2S_TRIGGER_DROP:
		if (strm->state == I2S_STATE_NOT_READY) {
			SYS_LOG_DBG("DROP trigger: invalid state");
			return -EIO;
		}
		strm->stream_disable(strm, ssp, dev_data->dev_dma);
		strm->queue_drop(strm);
		strm->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (strm->state != I2S_STATE_ERROR) {
			SYS_LOG_DBG("PREPARE trigger: invalid state");
			return -EIO;
		}
		strm->state = I2S_STATE_READY;
		strm->queue_drop(strm);
		break;

	default:
		SYS_LOG_ERR("Unsupported trigger command");
		return -EINVAL;
	}

	return 0;
}

static int i2s_cavs_write(struct device *dev, void *mem_block, size_t size)
{
	struct i2s_cavs_dev_data *const dev_data = DEV_DATA(dev);
	struct stream *strm = &dev_data->tx;
	int ret;

	if (dev_data->tx.state != I2S_STATE_RUNNING &&
	    dev_data->tx.state != I2S_STATE_READY) {
		SYS_LOG_ERR("invalid state");
		return -EIO;
	}

	ret = k_sem_take(&dev_data->tx.sem, dev_data->tx.cfg.timeout);
	if (ret < 0) {
		SYS_LOG_ERR("Failure taking sem");
		return ret;
	}

	/* Add data to the end of the TX queue */
	queue_put(&dev_data->tx.mem_block_queue, strm->cfg.options,
			 mem_block, size);
	return 0;
}

/* clear IRQ sources atm */
static void i2s_cavs_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct i2s_cavs_config *const dev_cfg = DEV_CFG(dev);
	volatile struct i2s_cavs_ssp *const ssp = dev_cfg->regs;
	u32_t temp;

	/* clear IRQ */
	temp = ssp->sss;
	ssp->sss = temp;
}

static int i2s1_cavs_initialize(struct device *dev)
{
	const struct i2s_cavs_config *const dev_cfg = DEV_CFG(dev);
	struct i2s_cavs_dev_data *const dev_data = DEV_DATA(dev);

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Initialize semaphores */
	k_sem_init(&dev_data->tx.sem, CONFIG_I2S_CAVS_TX_BLOCK_COUNT,
		   CONFIG_I2S_CAVS_TX_BLOCK_COUNT);

	dev_data->dev_dma = device_get_binding(CONFIG_I2S_CAVS_DMA_NAME);
	if (!dev_data->dev_dma) {
		SYS_LOG_ERR("%s device not found", CONFIG_I2S_CAVS_DMA_NAME);
		return -ENODEV;
	}

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	SYS_LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct i2s_driver_api i2s_cavs_driver_api = {
	.configure = i2s_cavs_configure,
	.write = i2s_cavs_write,
	.trigger = i2s_cavs_trigger,
};

/* I2S1 */

static struct device DEVICE_NAME_GET(i2s1_cavs);

static struct device *get_dev_from_dma_channel(u32_t dma_channel)
{
	return &DEVICE_NAME_GET(i2s1_cavs);
}

struct queue_item i2s1_ring_buf[CONFIG_I2S_CAVS_TX_BLOCK_COUNT];

static void i2s1_irq_config(void)
{
	IRQ_CONNECT(I2S1_CAVS_IRQ, CONFIG_I2S_CAVS_1_IRQ_PRI, i2s_cavs_isr,
		    DEVICE_GET(i2s1_cavs), 0);
}

static const struct i2s_cavs_config i2s1_cavs_config = {
	.regs = (struct i2s_cavs_ssp *)SSP_BASE(1),
	.mn_regs = (struct i2s_cavs_mn_div *)SSP_MN_DIV_BASE(1),
	.irq_id = I2S1_CAVS_IRQ,
	.irq_config = i2s1_irq_config,
};

static struct i2s_cavs_dev_data i2s1_cavs_data = {
	.tx = {
		.dma_channel = CONFIG_I2S_CAVS_1_DMA_TX_CHANNEL,
		.dma_cfg = {
			.source_data_size = 1,
			.dest_data_size = 1,
			.source_burst_length = 1,
			.dest_burst_length = 1,
			.dma_callback = dma_tx_callback,
			.complete_callback_en = 0,
			.error_callback_en = 1,
			.block_count = 1,
			.channel_direction = MEMORY_TO_PERIPHERAL,
			.dma_slot = DMA_HANDSHAKE_SSP1_TX,
		},
		.mem_block_queue.buf = i2s1_ring_buf,
		.mem_block_queue.len = ARRAY_SIZE(i2s1_ring_buf),
		.stream_start = tx_stream_start,
		.stream_disable = tx_stream_disable,
		.queue_drop = tx_queue_drop,
	},
};

DEVICE_AND_API_INIT(i2s1_cavs, CONFIG_I2S_CAVS_1_NAME, &i2s1_cavs_initialize,
		    &i2s1_cavs_data, &i2s1_cavs_config, POST_KERNEL,
		    CONFIG_I2S_INIT_PRIORITY, &i2s_cavs_driver_api);
