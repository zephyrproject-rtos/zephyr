/*
 * Copyright 2021,2023-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2S bus (SAI) driver for NXP i.MX RT series.
 */

#define DT_DRV_COMPAT nxp_mcux_i2s

#include <errno.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#ifdef CONFIG_CLOCK_CONTROL_MCUX_CCM
#include <zephyr/dt-bindings/clock/imx_ccm.h>
#endif

#include <zephyr/sys/barrier.h>
#include <soc.h>

#include <fsl_sai.h>
#include <fsl_edma.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dev_i2s_mcux, CONFIG_I2S_LOG_LEVEL);

#define NUM_DMA_BLOCKS_RX_PREP 3
#if defined(CONFIG_DMA_MCUX_EDMA)
BUILD_ASSERT(NUM_DMA_BLOCKS_RX_PREP >= 3,
	     "eDMA avoids TCD coherency issue if NUM_DMA_BLOCKS_RX_PREP >= 3");
#endif /* CONFIG_DMA_MCUX_EDMA */

#define MAX_TX_DMA_BLOCKS CONFIG_DMA_TCD_QUEUE_SIZE
BUILD_ASSERT(MAX_TX_DMA_BLOCKS > NUM_DMA_BLOCKS_RX_PREP,
	     "NUM_DMA_BLOCKS_RX_PREP must be < CONFIG_DMA_TCD_QUEUE_SIZE");

/*
 * The TX out_queue tracks the blocks loaded in the DMA, so it never holds
 * more than MAX_TX_DMA_BLOCKS entries. Size it for that, independently of
 * CONFIG_I2S_TX_BLOCK_COUNT, so that every block handed to the DMA can be
 * tracked and freed.
 */
#define TX_OUT_QUEUE_LEN MAX_TX_DMA_BLOCKS

/* The RX in_queue tracks the NUM_DMA_BLOCKS_RX_PREP blocks owned by the DMA */
BUILD_ASSERT(CONFIG_I2S_RX_BLOCK_COUNT >= NUM_DMA_BLOCKS_RX_PREP,
	     "CONFIG_I2S_RX_BLOCK_COUNT must be >= NUM_DMA_BLOCKS_RX_PREP");

#define SAI_WORD_SIZE_BITS_MIN 8
#define SAI_WORD_SIZE_BITS_MAX 32

#define SAI_WORD_PER_FRAME_MIN 0
#define SAI_WORD_PER_FRAME_MAX 32

/*
 * SAI driver uses source_gather_en/dest_scatter_en feature of DMA, and relies
 * on DMA driver managing circular list of DMA blocks.  Like eDMA driver links
 * Transfer Control Descriptors (TCDs) in list, and manages the tcdpool.
 * Calling dma_reload() adds new DMA block to DMA channel already configured,
 * into the DMA driver's circular list of blocks.

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
struct i2s_q_entry {
	void *mem_block;
	size_t size;
};

struct stream {
	enum i2s_state state;
	uint32_t dma_channel;
	uint32_t start_channel;
	void (*irq_call_back)(void);
	struct i2s_config cfg;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block;
	uint8_t free_tx_dma_blocks;
	bool last_block;
	struct k_msgq in_queue;
	struct k_msgq out_queue;
};

struct i2s_mcux_config {
	DEVICE_MMIO_NAMED_ROM(sai_mmio);
	uint32_t clk_src;
	uint32_t clk_pre_div;
	uint32_t clk_src_div;
	uint32_t pll_src;
	uint32_t pll_lp;
	uint32_t pll_pd;
	uint32_t pll_num;
	uint32_t pll_den;
	uint32_t mclk_control_base;
	uint32_t mclk_pin_mask;
	uint32_t mclk_pin_offset;
	bool mclk_output;
	uint32_t tx_channel;
	clock_control_subsys_t clk_sub_sys;
	const struct device *ccm_dev;
	const struct pinctrl_dev_config *pinctrl;
	struct reset_dt_spec reset;
	void (*irq_connect)(const struct device *dev);
	sai_sync_mode_t rx_sync_mode;
	sai_sync_mode_t tx_sync_mode;
	sai_bclk_source_t bclk_source;
};

/* Device run time data */
struct i2s_dev_data {
	DEVICE_MMIO_NAMED_RAM(sai_mmio);
	const struct device *dev_dma;
	struct stream tx;
	struct i2s_q_entry tx_in_msgs[CONFIG_I2S_TX_BLOCK_COUNT];
	struct i2s_q_entry tx_out_msgs[TX_OUT_QUEUE_LEN];
	struct stream rx;
	struct i2s_q_entry rx_in_msgs[CONFIG_I2S_RX_BLOCK_COUNT];
	struct i2s_q_entry rx_out_msgs[CONFIG_I2S_RX_BLOCK_COUNT];
};

#define DEV_CFG(_dev)  ((const struct i2s_mcux_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct i2s_dev_data *)(_dev)->data)

static I2S_Type *get_base(const struct device *dev)
{
	return (I2S_Type *)DEVICE_MMIO_NAMED_GET(dev, sai_mmio);
}

/*
 * Upper bound, in microseconds, for the TX FIFO to drain or the receiver to
 * finish its frame once the stream is being disabled. At least one FIFO word
 * is shifted per frame, so allow twice the FIFO depth in frames plus 1 ms.
 * Bounding these waits keeps a stopped external clock from hanging the CPU.
 */
static uint32_t i2s_stream_wind_down_us(const struct device *dev, const struct stream *strm)
{
	uint32_t fs = strm->cfg.frame_clk_freq;

	if (fs == 0U) {
		return 1000U;
	}

	return ((2U * (uint32_t)FSL_FEATURE_SAI_FIFO_COUNTn(get_base(dev)) * USEC_PER_SEC) / fs) +
	       1000U;
}

/* Write-1-to-clear status flags, the same bits in TCSR and RCSR */
#define I2S_CSR_W1C_MASK (I2S_TCSR_WSF_MASK | I2S_TCSR_SEF_MASK | I2S_TCSR_FEF_MASK)

/*
 * TX and RX share their bit and frame clocks when one of them is synchronous
 * to the other. The side that provides the clocks must stay enabled while the
 * other side needs them. TX keeps them in I2S_STATE_ERROR too, until PREPARE
 * or DROP; RX is already disabled in that state.
 */
static bool i2s_tx_needs_clocks(const struct i2s_dev_data *dev_data)
{
	return (dev_data->tx.state == I2S_STATE_RUNNING) ||
	       (dev_data->tx.state == I2S_STATE_STOPPING) ||
	       (dev_data->tx.state == I2S_STATE_ERROR);
}

static bool i2s_rx_needs_clocks(const struct i2s_dev_data *dev_data)
{
	return (dev_data->rx.state == I2S_STATE_RUNNING) ||
	       (dev_data->rx.state == I2S_STATE_STOPPING);
}

/* RX is clocked by TX (nxp,rx-sync-mode) */
static bool i2s_rx_sync_to_tx(I2S_Type *base)
{
	return ((base->RCR2 & I2S_RCR2_SYNC_MASK) >> I2S_RCR2_SYNC_SHIFT) == 1U;
}

/* TX is clocked by RX (nxp,tx-sync-mode) */
static bool i2s_tx_sync_to_rx(I2S_Type *base)
{
	return ((base->TCR2 & I2S_TCR2_SYNC_MASK) >> I2S_TCR2_SYNC_SHIFT) == 1U;
}

/*
 * Clear TE and BCE, wait for the transmitter to finish its frame, then reset
 * the TX FIFO and clear the error flags. BCE is set together with TE but is
 * not cleared with it: the bit clock keeps running until BCE is cleared too.
 * SAI_TxEnable() is not used because it keeps TE set whenever RX is
 * synchronous to TX, even when RX is idle, and never clears BCE.
 */
static void i2s_tx_transmitter_off(const struct device *dev)
{
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;

	base->TCSR = (base->TCSR & ~I2S_CSR_W1C_MASK) & ~(I2S_TCSR_TE_MASK | I2S_TCSR_BCE_MASK);

	if (!WAIT_FOR((base->TCSR & (I2S_TCSR_TE_MASK | I2S_TCSR_BCE_MASK)) == 0UL,
		      i2s_stream_wind_down_us(dev, &dev_data->tx), k_busy_wait(1))) {
		LOG_WRN("TX did not stop");
		return;
	}

	base->TCSR |= (I2S_TCSR_FR_MASK | I2S_TCSR_SR_MASK);
	base->TCSR &= ~I2S_TCSR_SR_MASK;
}

/* Same as i2s_tx_transmitter_off() for RE, RCSR[BCE] and the RX FIFO */
static void i2s_rx_receiver_off(const struct device *dev)
{
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;

	base->RCSR = (base->RCSR & ~I2S_CSR_W1C_MASK) & ~(I2S_RCSR_RE_MASK | I2S_RCSR_BCE_MASK);

	if (!WAIT_FOR((base->RCSR & (I2S_RCSR_RE_MASK | I2S_RCSR_BCE_MASK)) == 0UL,
		      i2s_stream_wind_down_us(dev, &dev_data->rx), k_busy_wait(1))) {
		LOG_WRN("RX did not stop");
		return;
	}

	base->RCSR |= (I2S_RCSR_FR_MASK | I2S_RCSR_SR_MASK);
	base->RCSR &= ~I2S_RCSR_SR_MASK;
}

/*
 * Wait for the next frame start on a side that is still enabled. WDFL is left
 * at 0, so the word start flag (WSF, the same bit in TCSR and RCSR) sets at the
 * first word of each frame. A channel enabled right after that has almost a
 * whole frame before it takes effect at the frame end: enough for the DMA to
 * fill the TX FIFO, and no data word of the running frame is involved.
 */
static void i2s_wait_frame_start(volatile uint32_t *csr, uint32_t timeout_us)
{
	*csr = (*csr & ~I2S_CSR_W1C_MASK) | I2S_TCSR_WSF_MASK;

	if (!WAIT_FOR((*csr & I2S_TCSR_WSF_MASK) != 0UL, timeout_us, k_busy_wait(1))) {
		LOG_WRN("no frame start");
	}
}

/*
 * The channel FIFO reset (TCR3[CFR], RCR3[CFR]) may be used on a disabled
 * channel while the transmitter or receiver stays enabled. Only instances with
 * more than one channel have it; the others have the global FIFO reset
 * (TCSR[FR], RCSR[FR]) only, which needs the side disabled.
 */
static bool i2s_has_channel_fifo_reset(I2S_Type *base)
{
#if defined(I2S_TCR3_CFR_MASK) && defined(I2S_RCR3_CFR_MASK)
	return (int)FSL_FEATURE_SAI_CHANNEL_COUNTn(base) > 1;
#else
	ARG_UNUSED(base);
	return false;
#endif
}

/* A channel FIFO is empty when its read and write pointers are equal */
static bool i2s_channel_fifo_empty(I2S_Type *base, enum i2s_dir dir, uint32_t channel)
{
#if defined(I2S_TFR_RFP_MASK) && defined(I2S_TFR_WFP_MASK) && defined(I2S_RFR_RFP_MASK) &&         \
	defined(I2S_RFR_WFP_MASK)
	uint32_t fr;

	if (dir == I2S_DIR_TX) {
		fr = base->TFR[channel];
		return ((fr & I2S_TFR_RFP_MASK) >> I2S_TFR_RFP_SHIFT) ==
		       ((fr & I2S_TFR_WFP_MASK) >> I2S_TFR_WFP_SHIFT);
	}

	fr = base->RFR[channel];
	return ((fr & I2S_RFR_RFP_MASK) >> I2S_RFR_RFP_SHIFT) ==
	       ((fr & I2S_RFR_WFP_MASK) >> I2S_RFR_WFP_SHIFT);
#else
	ARG_UNUSED(base);
	ARG_UNUSED(dir);
	ARG_UNUSED(channel);
	return true;
#endif
}

/*
 * Empty the FIFO of a stream that stops while its side stays enabled for the
 * clocks of the opposite stream, so that its next run starts with its own
 * data. The global FIFO reset needs the side disabled.
 *
 * The channel is masked already. A mask change takes effect at the end of a
 * frame, so wait for two frame starts: the second one follows a whole frame
 * with the channel masked, and no word moves into or out of its FIFO any
 * more. Then reset the channel FIFO. Without a channel FIFO reset, RX reads
 * out the words left, as a read of RDR on a masked channel still advances the
 * read pointer, and TX has sent its words out before the channel was masked.
 */
static void i2s_channel_fifo_flush(const struct device *dev, enum i2s_dir dir)
{
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	struct stream *strm = (dir == I2S_DIR_TX) ? &dev_data->tx : &dev_data->rx;
	volatile uint32_t *csr = (dir == I2S_DIR_TX) ? &base->TCSR : &base->RCSR;
	uint32_t timeout_us = i2s_stream_wind_down_us(dev, strm);
	uint32_t channel = strm->start_channel;

	i2s_wait_frame_start(csr, timeout_us);
	i2s_wait_frame_start(csr, timeout_us);

#if defined(I2S_TCR3_CFR_MASK) && defined(I2S_RCR3_CFR_MASK)
	if (i2s_has_channel_fifo_reset(base)) {
		if (dir == I2S_DIR_TX) {
			base->TCR3 |= I2S_TCR3_CFR(dev_cfg->tx_channel);
		} else {
			base->RCR3 |= I2S_RCR3_CFR(dev_cfg->tx_channel);
		}
	}
#else
	ARG_UNUSED(dev_cfg);
#endif

	if (dir == I2S_DIR_RX) {
		for (uint32_t i = 0U; (i < (uint32_t)FSL_FEATURE_SAI_FIFO_COUNTn(base)) &&
				      !i2s_channel_fifo_empty(base, dir, channel);
		     i++) {
			(void)base->RDR[channel];
		}
	}

	if (!i2s_channel_fifo_empty(base, dir, channel)) {
		LOG_WRN("%s FIFO not empty", (dir == I2S_DIR_TX) ? "TX" : "RX");
	}
}

static void i2s_purge_stream_buffers(struct stream *strm, struct k_mem_slab *mem_slab, bool in_drop,
				     bool out_drop)
{
	struct i2s_q_entry q_entry;

	/*
	 * mem_slab may be NULL when cleaning up NOT_READY or failed configuration
	 * paths. Drain queues but do not free buffers in that case.
	 */
	if (mem_slab == NULL) {
		if (in_drop) {
			while (k_msgq_get(&strm->in_queue, &q_entry, K_NO_WAIT) == 0) {
				/* drain only */
			}
		}
		if (out_drop) {
			while (k_msgq_get(&strm->out_queue, &q_entry, K_NO_WAIT) == 0) {
				/* drain only */
			}
		}
		return;
	}

	if (in_drop) {
		while (k_msgq_get(&strm->in_queue, &q_entry, K_NO_WAIT) == 0) {
			k_mem_slab_free(mem_slab, q_entry.mem_block);
		}
	}

	if (out_drop) {
		while (k_msgq_get(&strm->out_queue, &q_entry, K_NO_WAIT) == 0) {
			k_mem_slab_free(mem_slab, q_entry.mem_block);
		}
	}
}

static void i2s_tx_stream_disable(const struct device *dev, bool drop)
{
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->tx;
	const struct device *dev_dma = dev_data->dev_dma;
	/* the transmitter stays enabled for the clocks of a running RX */
	bool keep_te = i2s_rx_sync_to_tx(base) && i2s_rx_needs_clocks(dev_data);
	bool te_on = (base->TCSR & I2S_TCSR_TE_MASK) != 0UL;

	LOG_DBG("Stopping DMA channel %u for TX stream", strm->dma_channel);

	/* Mask the FIFO error interrupt, TX start enables it again */
	SAI_TxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable);

	/* Disable FIFO DMA request */
	SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, false);

	dma_stop(dev_dma, strm->dma_channel);

	/*
	 * On STOP and DRAIN, let the words in the FIFO go out before the
	 * channel is masked. On DROP they are discarded: by the FIFO reset when
	 * the transmitter goes off, or by the channel FIFO reset when it stays
	 * enabled for RX. Without a channel FIFO reset a masked channel keeps
	 * its words, so they are sent out then too.
	 */
	if (te_on && (!drop || (keep_te && !i2s_has_channel_fifo_reset(base)))) {
		if (!WAIT_FOR((base->TCSR & I2S_TCSR_FWF_MASK) != 0UL,
			      i2s_stream_wind_down_us(dev, strm), k_busy_wait(1))) {
			LOG_WRN("TX FIFO did not drain");
		}
	}

	/* Disable the channel FIFO */
	SAI_TxSetChannelFIFOMask(base, 0);

	/*
	 * Disable the transmitter, unless it provides the clocks of an RX
	 * stream that still needs them. The RX disable turns it off then; flush
	 * the channel FIFO for now.
	 */
	if (!keep_te) {
		i2s_tx_transmitter_off(dev);
	} else if (te_on) {
		i2s_channel_fifo_flush(dev, I2S_DIR_TX);
	}

	/*
	 * When TX is synchronous to RX, an idle RX may have kept its receiver
	 * enabled for this stream. Turn it off now that TX is done with it.
	 */
	if (i2s_tx_sync_to_rx(base) && !i2s_rx_needs_clocks(dev_data) &&
	    ((base->RCSR & I2S_RCSR_RE_MASK) != 0UL)) {
		i2s_rx_receiver_off(dev);
	}

	/* purge buffers queued in the stream */
	i2s_purge_stream_buffers(strm, dev_data->tx.cfg.mem_slab, drop, drop);
	if (drop) {
		strm->free_tx_dma_blocks = MAX_TX_DMA_BLOCKS;
		strm->last_block = false;
	}
}

static void i2s_rx_stream_disable(const struct device *dev, bool in_drop, bool out_drop)
{
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->rx;
	const struct device *dev_dma = dev_data->dev_dma;

	LOG_DBG("Stopping RX stream & DMA channel %u", strm->dma_channel);

	/* Mask the FIFO error interrupt, RX start enables it again */
	SAI_RxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable);

	dma_stop(dev_dma, strm->dma_channel);

	/* Disable the channel FIFO */
	SAI_RxSetChannelFIFOMask(base, 0);

	/* Disable DMA enable bit */
	SAI_RxEnableDMA(base, kSAI_FIFORequestDMAEnable, false);

	/*
	 * Disable the receiver, unless it provides the clocks of a TX stream
	 * that still needs them. The TX disable turns it off then; flush the
	 * channel FIFO for now.
	 */
	if (!i2s_tx_sync_to_rx(base) || !i2s_tx_needs_clocks(dev_data)) {
		i2s_rx_receiver_off(dev);
	} else if ((base->RCSR & I2S_RCSR_RE_MASK) != 0UL) {
		i2s_channel_fifo_flush(dev, I2S_DIR_RX);
	}

	/*
	 * When RX is synchronous to TX, SAI_RxEnable() started the transmitter
	 * for the RX clocks, and an idle TX may have kept it enabled for this
	 * stream. Turn it off now that RX is done with it.
	 */
	if (i2s_rx_sync_to_tx(base) && !i2s_tx_needs_clocks(dev_data) &&
	    ((base->TCSR & I2S_TCSR_TE_MASK) != 0UL)) {
		i2s_tx_transmitter_off(dev);
	}

	/* purge buffers queued in the stream */
	i2s_purge_stream_buffers(strm, dev_data->rx.cfg.mem_slab, in_drop, out_drop);
}

static int i2s_tx_reload_multiple_dma_blocks(const struct device *dev, uint8_t *blocks_queued)
{
	struct i2s_dev_data *dev_data = dev->data;
	I2S_Type *base = get_base(dev);
	struct stream *strm = &dev_data->tx;
	struct i2s_q_entry q_entry = {0};
	int ret = 0;
	unsigned int key;

	*blocks_queued = 0;

	key = irq_lock();

	/* queue additional blocks to DMA if in_queue and DMA has free blocks */
	while (strm->free_tx_dma_blocks) {
		/*
		 * Peek at the next entry and take it off in_queue only once the
		 * DMA accepted it. A block the DMA rejects stays on in_queue, so
		 * a later reload retries it and PREPARE/DROP can still free it.
		 */
		if (k_msgq_peek(&strm->in_queue, &q_entry) != 0) {
			/* in_queue is empty, no more blocks to send to DMA */
			break;
		}

		/* reload the DMA */
		ret = dma_reload(dev_data->dev_dma, strm->dma_channel, (uintptr_t)q_entry.mem_block,
				 (uintptr_t)&base->TDR[strm->start_channel], q_entry.size);
		if (ret != 0) {
			LOG_ERR("dma_reload() failed with error 0x%x", ret);
			break;
		}

		(void)k_msgq_get(&strm->in_queue, &q_entry, K_NO_WAIT);
		(strm->free_tx_dma_blocks)--;

		/* out_queue holds TX_OUT_QUEUE_LEN entries, one per DMA block */
		ret = k_msgq_put(&strm->out_queue, &q_entry, K_NO_WAIT);
		__ASSERT(ret == 0, "TX out_queue full");

		(*blocks_queued)++;
	}

	irq_unlock(key);
	return ret;
}

/*
 * Move an active TX stream to I2S_STATE_ERROR. Used for every TX error: an
 * underrun detected by the TX DMA callback or by the SAI FIFO error
 * interrupt, a DMA error and a failed DMA reload. Per the I2S API,
 * i2s_write() then fails with -EIO until a PREPARE or DROP trigger.
 *
 * Stop feeding the FIFO but keep the transmitter enabled, so the bit and
 * frame clocks keep running for an RX stream (or an external device)
 * clocked from this TX. Mask the FIFO error interrupt, which would otherwise
 * fire on every frame while the FIFO is empty. The queued buffers are kept;
 * the PREPARE/DROP trigger performs the full disable.
 *
 * Called in interrupt context.
 */
static void i2s_tx_stream_set_error(const struct device *dev)
{
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->tx;
	I2S_Type *base = get_base(dev);

	strm->state = I2S_STATE_ERROR;
	SAI_TxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable);
	SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, false);
	dma_stop(dev_data->dev_dma, strm->dma_channel);
}

/*
 * Move an active RX stream to I2S_STATE_ERROR. Used for every RX error: an
 * overrun detected by the SAI FIFO error interrupt or by the RX DMA callback
 * (no room left in out_queue or in the memory slab), a DMA error and a
 * failed DMA reload.
 *
 * Stop reception and release the blocks still owned by the DMA, but keep the
 * blocks already received so that i2s_read() can still return them, as the
 * I2S API requires for the ERROR state. PREPARE/DROP releases the rest.
 *
 * Called in interrupt context.
 */
static void i2s_rx_stream_set_error(const struct device *dev)
{
	struct i2s_dev_data *dev_data = dev->data;

	dev_data->rx.state = I2S_STATE_ERROR;
	i2s_rx_stream_disable(dev, true, false);
}

/* This function is executed in the interrupt context */
static void i2s_dma_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				int status)
{
	const struct device *dev = (struct device *)arg;
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->tx;
	uint8_t blocks_queued;
	struct i2s_q_entry q_entry = {0};
	int ret;

	LOG_DBG("tx cb");

	if (strm->state != I2S_STATE_RUNNING && strm->state != I2S_STATE_STOPPING) {
		/*
		 * A completion that was pending when the stream was stopped, by a
		 * trigger or by an error. Leave the queues and the transmitter
		 * alone: START resumes from them, PREPARE/DROP releases them.
		 */
		return;
	}

	if (status < 0) {
		LOG_ERR("TX DMA error %d", status);
		goto error;
	}

	ret = k_msgq_get(&strm->out_queue, &q_entry, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("no buf in out_queue for channel %u", channel);
		goto error;
	}

	/* transmission complete. free the buffer */
	k_mem_slab_free(strm->cfg.mem_slab, q_entry.mem_block);
	(strm->free_tx_dma_blocks)++;

	if (strm->free_tx_dma_blocks > MAX_TX_DMA_BLOCKS) {
		LOG_ERR("free_tx_dma_blocks exceeded maximum, now %d", strm->free_tx_dma_blocks);
		goto error;
	}

	/*
	 * Received a STOP trigger, terminate TX at the end of this block. The
	 * blocks still queued are kept for the next START.
	 */
	if (strm->last_block) {
		strm->state = I2S_STATE_READY;
		LOG_DBG("TX STOPPED last_block set");
		goto disabled_exit;
	}

	ret = i2s_tx_reload_multiple_dma_blocks(dev, &blocks_queued);
	if (ret) {
		goto error;
	}

	if (blocks_queued || (strm->free_tx_dma_blocks < MAX_TX_DMA_BLOCKS)) {
		return;
	}

	/* all DMA blocks are free but no blocks were queued */
	if (strm->state == I2S_STATE_STOPPING) {
		/* TX queue has drained */
		strm->state = I2S_STATE_READY;
		LOG_DBG("TX stream has stopped");
		goto disabled_exit;
	}

	/*
	 * RUNNING with every DMA block consumed and nothing queued: this is a
	 * TX underrun.
	 */
	LOG_ERR("TX underrun");

error:
	i2s_tx_stream_set_error(dev);
	return;

disabled_exit:
	i2s_tx_stream_disable(dev, false);
}

static void i2s_dma_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				int status)
{
	struct device *dev = (struct device *)arg;
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->rx;
	struct i2s_q_entry q_entry;
	int ret;

	LOG_DBG("RX cb");

	if (strm->state != I2S_STATE_STOPPING && strm->state != I2S_STATE_RUNNING) {
		/*
		 * A completion that was pending when the stream was stopped, by a
		 * trigger or by an error. The stream is already disabled.
		 */
		return;
	}

	if (status < 0) {
		LOG_ERR("RX DMA error %d", status);
		goto error;
	}

	/* retrieve entry from input queue */
	ret = k_msgq_get(&strm->in_queue, &q_entry, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("RX in_queue empty for channel %u", channel);
		goto error;
	}

	/* put entry to output queue */
	ret = k_msgq_put(&strm->out_queue, &q_entry, K_NO_WAIT);
	if (ret != 0) {
		/*
		 * RX overrun: the application does not read fast enough. Drop
		 * the block just received, the queued ones stay readable.
		 */
		LOG_ERR("RX overrun, out_queue full");
		k_mem_slab_free(strm->cfg.mem_slab, q_entry.mem_block);
		goto error;
	}

	if (strm->state == I2S_STATE_STOPPING) {
		i2s_rx_stream_disable(dev, true, false);
		/* Received a STOP/DRAIN trigger */
		strm->state = I2S_STATE_READY;
		return;
	}

	/* Now the only possible case is the running state */

	/* allocate new buffer for next audio frame */
	ret = k_mem_slab_alloc(strm->cfg.mem_slab, &q_entry.mem_block, K_NO_WAIT);
	if (ret != 0) {
		/* RX overrun: the application holds every block of the slab */
		LOG_ERR("buffer alloc from slab %p err %d", strm->cfg.mem_slab, ret);
		goto error;
	}
	q_entry.size = strm->cfg.block_size;

	uint32_t data_path = strm->start_channel;

	ret = dma_reload(dev_data->dev_dma, strm->dma_channel,
			 (uintptr_t)&base->RDR[data_path], (uintptr_t)q_entry.mem_block,
			 q_entry.size);
	if (ret != 0) {
		LOG_ERR("dma_reload() failed with error 0x%x", ret);
		k_mem_slab_free(strm->cfg.mem_slab, q_entry.mem_block);
		goto error;
	}

	/* in_queue holds the NUM_DMA_BLOCKS_RX_PREP blocks owned by the DMA */
	ret = k_msgq_put(&strm->in_queue, &q_entry, K_NO_WAIT);
	__ASSERT(ret == 0, "RX in_queue full");

	return;

error:
	i2s_rx_stream_set_error(dev);
}

static void enable_mclk_direction(const struct device *dev, bool dir)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	uint32_t control_base = dev_cfg->mclk_control_base;
	uint32_t offset = dev_cfg->mclk_pin_offset;
	uint32_t mask = dev_cfg->mclk_pin_mask;
	uint32_t *base = (uint32_t *)(uintptr_t)(control_base + offset);

	if (control_base == 0 && offset == 0 && mask == 0) {
		return;
	}

	if (dir) {
		*base |= mask;
	} else {
		*base &= ~mask;
	}
}

static void get_mclk_rate(const struct device *dev, uint32_t *mclk)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	const struct device *ccm_dev = dev_cfg->ccm_dev;
	clock_control_subsys_t clk_sub_sys = dev_cfg->clk_sub_sys;
	uint32_t rate = 0;

	if (device_is_ready(ccm_dev)) {
		clock_control_get_rate(ccm_dev, clk_sub_sys, &rate);
	} else {
		LOG_ERR("CCM driver is not installed");
		*mclk = rate;
		return;
	}
	*mclk = rate;
}

#ifndef CONFIG_I2S_HAS_PLL_SETTING
static void set_mclk_rate(const struct device *dev, uint32_t rate)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	const struct device *ccm_dev = dev_cfg->ccm_dev;
	clock_control_subsys_t clk_sub_sys = dev_cfg->clk_sub_sys;

	if (device_is_ready(ccm_dev)) {
		clock_control_set_rate(ccm_dev, clk_sub_sys,
					(clock_control_subsys_rate_t)(uintptr_t)rate);
		clock_control_on(ccm_dev, clk_sub_sys);
	} else {
		LOG_ERR("CCM driver is not installed");
		return;
	}
}
#endif

static int i2s_mcux_config(const struct device *dev, enum i2s_dir dir,
			   const struct i2s_config *i2s_cfg)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;
	enum i2s_state *tx_state = &(dev_data->tx.state);
	enum i2s_state *rx_state = &(dev_data->rx.state);
	uint8_t word_size_bits = i2s_cfg->word_size;
	uint8_t word_size_bytes = (word_size_bits == 24U) ? 4U : (word_size_bits / 8U);
	uint8_t num_words = i2s_cfg->channels;
	sai_transceiver_t config;
	struct stream *strm;
	int ret = -EINVAL;
	uint32_t mclk;

	if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	}

	/* only the stream being configured matters here, the other direction
	 * may well be running
	 */
	strm = (dir == I2S_DIR_TX) ? &dev_data->tx : &dev_data->rx;

	if ((strm->state != I2S_STATE_NOT_READY) && (strm->state != I2S_STATE_READY)) {
		LOG_ERR("invalid state %u", strm->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		/* deconfigure the stream: release any queued buffers and
		 * return the interface to the NOT_READY state
		 */
		i2s_purge_stream_buffers(strm, strm->cfg.mem_slab, true, true);
		strm->state = I2S_STATE_NOT_READY;
		return 0;
	}

	/*
	 * Do not reprogram a side that provides the clocks of the opposite
	 * stream while that stream still needs them.
	 */
	if (((dir == I2S_DIR_TX) && i2s_rx_sync_to_tx(base) && i2s_rx_needs_clocks(dev_data)) ||
	    ((dir == I2S_DIR_RX) && i2s_tx_sync_to_rx(base) && i2s_tx_needs_clocks(dev_data))) {
		LOG_ERR("cannot configure: the opposite stream uses its clocks");
		return -EINVAL;
	}

	if (word_size_bits < SAI_WORD_SIZE_BITS_MIN || word_size_bits > SAI_WORD_SIZE_BITS_MAX) {
		LOG_ERR("Unsupported I2S word size %u", word_size_bits);
		goto invalid_config;
	}

	if (num_words < SAI_WORD_PER_FRAME_MIN || num_words > SAI_WORD_PER_FRAME_MAX) {
		LOG_ERR("Unsupported words length %u", num_words);
		goto invalid_config;
	}

	if ((i2s_cfg->options & I2S_OPT_PINGPONG) == I2S_OPT_PINGPONG) {
		LOG_ERR("Ping-pong mode not supported");
		ret = -ENOTSUP;
		goto invalid_config;
	}

	/*
	 * Validate the format before any clock or pin setting is touched.
	 * Bits of i2s_cfg->format that are not decoded below are ignored.
	 */
	if ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		LOG_ERR("LSB first data order not supported");
		goto invalid_config;
	}

	if (((i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) == I2S_FMT_DATA_FORMAT_I2S) &&
	    (num_words > 2U)) {
		LOG_ERR("I2S format supports at most 2 channels");
		goto invalid_config;
	}

	memset(&config, 0, sizeof(config));

	if (dev_cfg->mclk_control_base) {
		enable_mclk_direction(dev, dev_cfg->mclk_output);
	}

#ifndef CONFIG_I2S_HAS_PLL_SETTING
	/*
	 * Re-tune the SAI root clock to a value that matches the requested
	 * sample rate's clock family. The default value programmed at driver
	 * initialization (12.288MHz, 48kHz-family) cannot generate a valid
	 * 44.1kHz LRCK via any integer bit-clock divider, so leaving it in
	 * place for a 44.1kHz stream produces silence at the codec (the SAI
	 * rounds LRCK to 48kHz and the codec's SYSCLK:LRCK ratio falls out
	 * of its supported range). Pick the closest MCLK that is an integer
	 * multiple of 256 * frame_clk_freq; the platform's CCM driver is
	 * responsible for reconfiguring the underlying audio PLL family.
	 *
	 * Both directions share this root clock, so only touch it when the
	 * rate actually has to change, and refuse the change while the
	 * opposite direction is configured: its bit clock divider was
	 * derived from the current MCLK and would silently shift.
	 */
	{
		enum i2s_state peer_state = (dir == I2S_DIR_TX) ? *rx_state : *tx_state;
		uint32_t desired_mclk;
		uint32_t current_mclk;

		if ((i2s_cfg->frame_clk_freq % 11025U) == 0U) {
			desired_mclk = 11289600U;   /* 44.1kHz family */
		} else {
			desired_mclk = 12288000U;   /* 48kHz family (default) */
		}

		get_mclk_rate(dev, &current_mclk);

		if (current_mclk != desired_mclk) {
			if (peer_state != I2S_STATE_NOT_READY) {
				LOG_ERR("cannot retune root clock to %uHz: opposite "
					"direction is already configured", desired_mclk);
				goto invalid_config;
			}
			set_mclk_rate(dev, desired_mclk);
		}
	}
#endif

	get_mclk_rate(dev, &mclk);
	LOG_DBG("mclk is %d", mclk);

	/*
	 * additional settings for bclk
	 * read the SDK header file for more details
	 */
	config.bitClock.bclkInputDelay = false;

	/* frame sync default configurations */
#if defined(FSL_FEATURE_SAI_HAS_ON_DEMAND_MODE) && FSL_FEATURE_SAI_HAS_ON_DEMAND_MODE
	config.frameSync.frameSyncGenerateOnDemand = false;
#endif

	/* serial data default configurations */
#if defined(FSL_FEATURE_SAI_HAS_CHANNEL_MODE) && FSL_FEATURE_SAI_HAS_CHANNEL_MODE
	config.serialData.dataMode = kSAI_DataPinStateOutputZero;
#endif

	config.frameSync.frameSyncPolarity = kSAI_PolarityActiveLow;
#if defined(FSL_FEATURE_SAI_HAS_BIT_CLOCK_SWAP) && FSL_FEATURE_SAI_HAS_BIT_CLOCK_SWAP
	config.bitClock.bclkSrcSwap = true;
#endif
	/* format */
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		SAI_GetClassicI2SConfig(&config, word_size_bits, kSAI_Stereo, dev_cfg->tx_channel);
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		SAI_GetLeftJustifiedConfig(&config, word_size_bits, kSAI_Stereo,
					   dev_cfg->tx_channel);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		SAI_GetDSPConfig(&config, kSAI_FrameSyncLenOneBitClk, word_size_bits, kSAI_Stereo,
				 dev_cfg->tx_channel);
		/* We need to set the data word count manually, since the HAL
		 * function does not
		 */
		config.serialData.dataWordNum = num_words;
		config.frameSync.frameSyncEarly = true;
		config.bitClock.bclkPolarity = kSAI_SampleOnFallingEdge;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		SAI_GetTDMConfig(&config, kSAI_FrameSyncLenPerWordWidth, word_size_bits, num_words,
				 dev_cfg->tx_channel);
		config.bitClock.bclkPolarity = kSAI_SampleOnFallingEdge;
		break;
	default:
		LOG_ERR("Unsupported I2S data format");
		ret = -EINVAL;
		goto invalid_config;
	}

	/* The SAI_Get*Config() helpers above reset the bit clock source to
	 * the master clock divider; honor the devicetree selection instead.
	 */
	config.bitClock.bclkSource = dev_cfg->bclk_source;

	/* sync mode configurations */
	if (dir == I2S_DIR_TX) {
		config.syncMode = dev_cfg->tx_sync_mode;
	} else if (dir == I2S_DIR_RX) {
		config.syncMode = dev_cfg->rx_sync_mode;
	}

	bool frame_clk_slave = i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET;
	bool bit_clk_slave = i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET;

	if (frame_clk_slave && bit_clk_slave) {
		config.masterSlave = kSAI_Slave;
	} else if (frame_clk_slave && !bit_clk_slave) {
		config.masterSlave = kSAI_Bclk_Master_FrameSync_Slave;
	} else if (!frame_clk_slave && bit_clk_slave) {
		config.masterSlave = kSAI_Bclk_Slave_FrameSync_Master;
	} else {
		config.masterSlave = kSAI_Master;
	}

	/* clock signal polarity */
	switch (i2s_cfg->format & I2S_FMT_CLK_FORMAT_MASK) {
	case I2S_FMT_CLK_NF_NB:
		/* No action required, leave the configuration untouched */
		break;

	case I2S_FMT_CLK_NF_IB:
		/* Swap bclk polarity */
		config.bitClock.bclkPolarity =
			(config.bitClock.bclkPolarity == kSAI_SampleOnFallingEdge)
				? kSAI_SampleOnRisingEdge
				: kSAI_SampleOnFallingEdge;
		break;
	case I2S_FMT_CLK_IF_NB:
		/* Swap frame sync polarity */
		config.frameSync.frameSyncPolarity =
			(config.frameSync.frameSyncPolarity == kSAI_PolarityActiveHigh)
				? kSAI_PolarityActiveLow
				: kSAI_PolarityActiveHigh;
		break;
	case I2S_FMT_CLK_IF_IB:
		/* Swap frame sync and bclk polarity */
		config.frameSync.frameSyncPolarity =
			(config.frameSync.frameSyncPolarity == kSAI_PolarityActiveHigh)
				? kSAI_PolarityActiveLow
				: kSAI_PolarityActiveHigh;
		config.bitClock.bclkPolarity =
			(config.bitClock.bclkPolarity == kSAI_SampleOnFallingEdge)
				? kSAI_SampleOnRisingEdge
				: kSAI_SampleOnFallingEdge;
		break;
	}

	/* PCM short format always requires that WS be one BCLK cycle */
	if ((i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) != I2S_FMT_DATA_FORMAT_PCM_SHORT) {
		config.frameSync.frameSyncWidth = (uint8_t)word_size_bits;
	}

	if (dir == I2S_DIR_TX) {
		memcpy(&dev_data->tx.cfg, i2s_cfg, sizeof(struct i2s_config));
		LOG_DBG("tx slab free_list = %p", (void *)i2s_cfg->mem_slab->free_list);
		LOG_DBG("tx slab num_blocks = %d", (uint32_t)i2s_cfg->mem_slab->info.num_blocks);
		LOG_DBG("tx slab block_size = %d", (uint32_t)i2s_cfg->mem_slab->info.block_size);
		LOG_DBG("tx slab buffer = %p", (void *)i2s_cfg->mem_slab->buffer);

		config.fifo.fifoWatermark = (uint32_t)FSL_FEATURE_SAI_FIFO_COUNTn(base) - 1;
#if defined(FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE) && FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE
		config.fifo.fifoCombine = kSAI_FifoCombineModeEnabledOnWrite;
#endif
		/* set bit clock divider */
		SAI_TxSetConfig(base, &config);
		dev_data->tx.start_channel = config.startChannel;
		/* Disable the channel FIFO */
		SAI_TxSetChannelFIFOMask(base, 0);
		SAI_TxSetBitClockRate(base, mclk, i2s_cfg->frame_clk_freq, word_size_bits,
				      i2s_cfg->channels);
		LOG_DBG("tx start_channel = %d", dev_data->tx.start_channel);
		/*set up dma settings*/
		dev_data->tx.dma_cfg.source_data_size = word_size_bytes;
		dev_data->tx.dma_cfg.dest_data_size = word_size_bytes;
		dev_data->tx.dma_cfg.source_burst_length = word_size_bytes;
		dev_data->tx.dma_cfg.dest_burst_length = word_size_bytes;
		dev_data->tx.dma_cfg.user_data = (void *)dev;
		dev_data->tx.state = I2S_STATE_READY;
	} else {
		/* For RX, DMA reads from FIFO whenever data present */
		config.fifo.fifoWatermark = 0;
#if defined(FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE) && FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE
		config.fifo.fifoCombine = kSAI_RXFifoCombineModeEnabledOnRead;
#endif

		memcpy(&dev_data->rx.cfg, i2s_cfg, sizeof(struct i2s_config));
		LOG_DBG("rx slab free_list = %p", (void *)i2s_cfg->mem_slab->free_list);
		LOG_DBG("rx slab num_blocks = %d", (uint32_t)i2s_cfg->mem_slab->info.num_blocks);
		LOG_DBG("rx slab block_size = %d", (uint32_t)i2s_cfg->mem_slab->info.block_size);
		LOG_DBG("rx slab buffer = %p", (void *)i2s_cfg->mem_slab->buffer);

		/* set bit clock divider */
		SAI_RxSetConfig(base, &config);
		dev_data->rx.start_channel = config.startChannel;
		/* Disable the channel FIFO */
		SAI_RxSetChannelFIFOMask(base, 0);
		SAI_RxSetBitClockRate(base, mclk, i2s_cfg->frame_clk_freq, word_size_bits,
				      i2s_cfg->channels);
		LOG_DBG("rx start_channel = %d", dev_data->rx.start_channel);
		/*set up dma settings*/
		dev_data->rx.dma_cfg.source_data_size = word_size_bytes;
		dev_data->rx.dma_cfg.dest_data_size = word_size_bytes;
		dev_data->rx.dma_cfg.source_burst_length = word_size_bytes;
		dev_data->rx.dma_cfg.dest_burst_length = word_size_bytes;
		dev_data->rx.dma_cfg.user_data = (void *)dev;
		dev_data->rx.state = I2S_STATE_READY;
	}

	return 0;

invalid_config:
	if (dir == I2S_DIR_TX) {
		*tx_state = I2S_STATE_NOT_READY;
	} else if (dir == I2S_DIR_RX) {
		*rx_state = I2S_STATE_NOT_READY;
	}
	return ret;
}

const struct i2s_config *i2s_mcux_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = (dir == I2S_DIR_RX) ? &dev_data->rx : &dev_data->tx;

	/* the i2s API expects NULL for a stream that is not configured, so
	 * that the leftover configuration is not mistaken for a live one
	 */
	if (strm->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &strm->cfg;
}

static int i2s_tx_stream_start(const struct device *dev)
{
	int ret = 0;
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->tx;
	const struct device *dev_dma = dev_data->dev_dma;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = get_base(dev);
	struct i2s_q_entry pending[TX_OUT_QUEUE_LEN];
	size_t pending_cnt = 0;
	struct dma_block_config *blk_cfg = &strm->dma_block;
	uint32_t data_path = strm->start_channel;
	uint8_t blocks_queued;
	bool te_on;

	/* 1) If STOP left pending buffers in out_queue, collect them */
	while (k_msgq_get(&strm->out_queue, &pending[pending_cnt], K_NO_WAIT) == 0) {
		pending_cnt++;
		if (pending_cnt >= ARRAY_SIZE(pending)) {
			break;
		}
	}

	/* 2) If no pending buffers, use normal start from in_queue */
	if (pending_cnt == 0) {
		ret = k_msgq_get(&strm->in_queue, &pending[pending_cnt], K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("No entry in input queue to start");
			return -EIO;
		}
		pending_cnt++;
	}

	/*
	 * 3) Put the pending buffers back on out_queue, in the order the TX DMA
	 * callback frees them. Do this before any call that can fail, so that
	 * the START error path (i2s_tx_stream_disable() with drop) returns them
	 * to the slab instead of leaking them.
	 */
	for (size_t i = 0; i < pending_cnt; i++) {
		ret = k_msgq_put(&strm->out_queue, &pending[i], K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("failed to rebuild out_queue (%d)", ret);
			return ret;
		}
	}

	/* 4) Reset DMA bookkeeping */
	strm->free_tx_dma_blocks = MAX_TX_DMA_BLOCKS;

	/* 5) Configure DMA with first pending buffer */
	memset(blk_cfg, 0, sizeof(*blk_cfg));
	blk_cfg->dest_address = (uintptr_t)&base->TDR[data_path];
	blk_cfg->source_address = (uintptr_t)pending[0].mem_block;
	blk_cfg->block_size = pending[0].size;
	blk_cfg->dest_scatter_en = 1;

	strm->dma_cfg.block_count = 1;
	strm->dma_cfg.head_block = blk_cfg;
	strm->dma_cfg.user_data = (void *)dev;

	strm->free_tx_dma_blocks--;
	ret = dma_config(dev_dma, strm->dma_channel, &strm->dma_cfg);
	if (ret != 0) {
		LOG_ERR("dma_config() failed with error 0x%x", ret);
		return ret;
	}

	/* 6) Reload remaining pending buffers (pending[1..]) into DMA first */
	for (size_t i = 1; i < pending_cnt && strm->free_tx_dma_blocks; i++) {
		ret = dma_reload(dev_dma, strm->dma_channel, (uintptr_t)pending[i].mem_block,
				 (uintptr_t)&base->TDR[data_path], pending[i].size);
		if (ret != 0) {
			LOG_ERR("dma_reload(pending) failed (0x%x)", ret);
			return ret;
		}
		strm->free_tx_dma_blocks--;
	}

	/* 7) Then queue new blocks from in_queue as usual */
	ret = i2s_tx_reload_multiple_dma_blocks(dev, &blocks_queued);
	if (ret) {
		LOG_ERR("i2s_tx_reload_multiple_dma_blocks() failed (%d)", ret);
		return ret;
	}

	LOG_DBG("Starting DMA Ch%u", strm->dma_channel);
	ret = dma_start(dev_dma, strm->dma_channel);
	if (ret < 0) {
		LOG_ERR("dma_start failed (%d)", ret);
		return ret;
	}

	/*
	 * Reset the FIFO pointers, which may hold stale words from a previous
	 * run of the stream. The FIFO reset is only allowed with the
	 * transmitter disabled. When it stays enabled for the clocks of a
	 * synchronous RX, the channel FIFO was flushed when the stream stopped.
	 */
	te_on = (base->TCSR & I2S_TCSR_TE_MASK) != 0UL;
	if (!te_on) {
		base->TCSR = (base->TCSR & ~I2S_CSR_W1C_MASK) | I2S_TCSR_FR_MASK;
	}

	/*
	 * The FIFO error interrupt is enabled only while the stream runs. An
	 * idle transmitter, for example one kept enabled to clock a
	 * synchronous receiver, underruns on every frame. Clear any stale
	 * FIFO error flag before enabling it for the new run.
	 */
	SAI_TxClearStatusFlags(base, kSAI_FIFOErrorFlag);
	SAI_TxEnableInterrupts(base, kSAI_FIFOErrorInterruptEnable);

	/* Enable DMA enable bit */
	SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, true);

	/*
	 * Enable the channel FIFO. On a running transmitter this takes effect
	 * at the end of the current frame, so do it at a frame start.
	 */
	if (te_on) {
		i2s_wait_frame_start(&base->TCSR, i2s_stream_wind_down_us(dev, strm));
	}
	SAI_TxSetChannelFIFOMask(base, dev_cfg->tx_channel);

	/* Enable SAI Tx clock */
	SAI_TxEnable(base, true);

	return 0;
}

static int i2s_rx_stream_start(const struct device *dev)
{
	int ret = 0;
	struct i2s_q_entry q_entry = {0};
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->rx;
	const struct device *dev_dma = dev_data->dev_dma;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base = get_base(dev);
	uint32_t num_of_bufs;
	bool re_on;

	num_of_bufs = k_mem_slab_num_free_get(strm->cfg.mem_slab);

	/*
	 * Need at least NUM_DMA_BLOCKS_RX_PREP buffers on the RX memory slab
	 * for reliable DMA reception.
	 */
	if (num_of_bufs < NUM_DMA_BLOCKS_RX_PREP) {
		return -ENOMEM;
	}

	/* allocate 1st receive entry from SLAB */
	ret = k_mem_slab_alloc(strm->cfg.mem_slab, &q_entry.mem_block, K_NO_WAIT);
	if (ret != 0) {
		LOG_DBG("entry alloc from mem_slab failed (%d)", ret);
		return -ENOMEM;
	}
	q_entry.size = strm->cfg.block_size;

	/* Configure DMA block */
	struct dma_block_config *blk_cfg = &strm->dma_block;

	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	uint32_t data_path = strm->start_channel;

	blk_cfg->dest_address = (uintptr_t)q_entry.mem_block;
	blk_cfg->source_address = (uintptr_t)&base->RDR[data_path];
	blk_cfg->block_size = q_entry.size;

	blk_cfg->source_gather_en = 1;

	strm->dma_cfg.block_count = 1;
	strm->dma_cfg.head_block = &strm->dma_block;
	strm->dma_cfg.user_data = (void *)dev;

	/*
	 * Track every allocated block on in_queue before any call that can
	 * fail, so that the START error path (i2s_rx_stream_disable() with
	 * drop) returns it to the slab. in_queue holds at least
	 * NUM_DMA_BLOCKS_RX_PREP entries, so the put cannot fail.
	 */
	ret = k_msgq_put(&strm->in_queue, &q_entry, K_NO_WAIT);
	__ASSERT(ret == 0, "RX in_queue full");

	ret = dma_config(dev_dma, strm->dma_channel, &strm->dma_cfg);
	if (ret != 0) {
		LOG_ERR("dma_config() failed with error 0x%x", ret);
		return ret;
	}

	/* prep DMA for each of remaining (NUM_DMA_BLOCKS_RX_PREP-1) buffers */
	for (int i = 0; i < NUM_DMA_BLOCKS_RX_PREP - 1; i++) {

		/* allocate receive entry from SLAB */
		ret = k_mem_slab_alloc(strm->cfg.mem_slab, &q_entry.mem_block, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("entry alloc from mem_slab failed (%d)", ret);
			return -ENOMEM;
		}
		q_entry.size = blk_cfg->block_size;

		ret = dma_reload(dev_dma, strm->dma_channel, (uintptr_t)&base->RDR[data_path],
				 (uintptr_t)q_entry.mem_block, q_entry.size);
		if (ret != 0) {
			LOG_ERR("dma_reload() failed with error 0x%x", ret);
			k_mem_slab_free(strm->cfg.mem_slab, q_entry.mem_block);
			return ret;
		}

		ret = k_msgq_put(&strm->in_queue, &q_entry, K_NO_WAIT);
		__ASSERT(ret == 0, "RX in_queue full");
	}

	LOG_DBG("Starting DMA Ch%u", strm->dma_channel);
	ret = dma_start(dev_dma, strm->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA Ch%d (%d)", strm->dma_channel, ret);
		return ret;
	}

	/*
	 * Reset the FIFO pointers, which may hold stale words from a previous
	 * run of the stream. The FIFO reset is only allowed with the receiver
	 * disabled. When it stays enabled for the clocks of a synchronous TX,
	 * the channel FIFO was flushed when the stream stopped.
	 */
	re_on = (base->RCSR & I2S_RCSR_RE_MASK) != 0UL;
	if (!re_on) {
		base->RCSR = (base->RCSR & ~I2S_CSR_W1C_MASK) | I2S_RCSR_FR_MASK;
	}

	/*
	 * The FIFO error interrupt is enabled only while the stream runs.
	 * Clear any stale FIFO error flag before enabling it for the new run.
	 */
	SAI_RxClearStatusFlags(base, kSAI_FIFOErrorFlag);
	SAI_RxEnableInterrupts(base, kSAI_FIFOErrorInterruptEnable);

	/* Enable DMA enable bit */
	SAI_RxEnableDMA(base, kSAI_FIFORequestDMAEnable, true);

	/*
	 * Enable the channel FIFO. On a running receiver this takes effect at
	 * the end of the current frame, so do it at a frame start.
	 */
	if (re_on) {
		i2s_wait_frame_start(&base->RCSR, i2s_stream_wind_down_us(dev, strm));
	}
	SAI_RxSetChannelFIFOMask(base, dev_cfg->tx_channel);

	/* Enable SAI Rx clock */
	SAI_RxEnable(base, true);

	return 0;
}

static int i2s_mcux_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_dev_data *dev_data = dev->data;
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
			LOG_ERR("START trigger: invalid state %u", strm->state);
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
			/* Ensure no partial DMA or buffers survive */
			if (dir == I2S_DIR_TX) {
				i2s_tx_stream_disable(dev, true);
			} else {
				i2s_rx_stream_disable(dev, true, true);
			}
			/* -ENOMEM is a documented trigger result, report it as is */
			ret = (ret == -ENOMEM) ? -ENOMEM : -EIO;
			break;
		}

		strm->state = I2S_STATE_RUNNING;
		strm->last_block = false;
		break;

	case I2S_TRIGGER_DROP:
		if (strm->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state %d", strm->state);
			ret = -EIO;
			break;
		}

		strm->state = I2S_STATE_READY;
		if (dir == I2S_DIR_TX) {
			i2s_tx_stream_disable(dev, true);
		} else {
			i2s_rx_stream_disable(dev, true, true);
		}
		break;

	case I2S_TRIGGER_STOP:
		if (strm->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP trigger: invalid state %d", strm->state);
			ret = -EIO;
			break;
		}

		strm->state = I2S_STATE_STOPPING;
		strm->last_block = true;
		if ((dir == I2S_DIR_TX) && (strm->free_tx_dma_blocks == MAX_TX_DMA_BLOCKS)) {
			/*
			 * No block in flight, so no DMA callback will complete
			 * the STOP. Stop now, but keep the queued blocks: a
			 * subsequent START resumes from them.
			 */
			strm->state = I2S_STATE_READY;
			i2s_tx_stream_disable(dev, false);
		}
		break;

	case I2S_TRIGGER_DRAIN:
		if (strm->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN/STOP trigger: invalid state %d", strm->state);
			ret = -EIO;
			break;
		}

		strm->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_PREPARE:
		if (strm->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state %d", strm->state);
			ret = -EIO;
			break;
		}
		strm->state = I2S_STATE_READY;
		if (dir == I2S_DIR_TX) {
			i2s_tx_stream_disable(dev, true);
		} else {
			i2s_rx_stream_disable(dev, true, true);
		}
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		ret = -EINVAL;
	}

	irq_unlock(key);
	return ret;
}

static int i2s_mcux_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->rx;
	struct i2s_q_entry q_entry = {0};
	int status;

	LOG_DBG("i2s_mcux_read");
	if (strm->state == I2S_STATE_NOT_READY) {
		LOG_ERR("invalid state %d", strm->state);
		return -EIO;
	}

	status = k_msgq_get(&strm->out_queue, &q_entry, SYS_TIMEOUT_MS(strm->cfg.timeout));
	if (status != 0) {
		/*
		 * Map k_msgq_get() results to I2S API:
		 *  -EBUSY : returned immediately without waiting (K_NO_WAIT/timeout==0)
		 *  -EAGAIN: waited and timed out
		 *  -EIO   : in ERROR state and no more blocks are available
		 */
		if (status == -ENOMSG) {
			/* No message and did not wait */
			return (strm->state == I2S_STATE_ERROR) ? -EIO : -EBUSY;
		}

		if (status == -EAGAIN) {
			/* Timed out while waiting */
			return (strm->state == I2S_STATE_ERROR) ? -EIO : -EAGAIN;
		}

		/* Propagate unexpected errors */
		return status;
	}

	*mem_block = q_entry.mem_block;
	*size = q_entry.size;
	return 0;
}

static int i2s_mcux_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_dev_data *dev_data = dev->data;
	struct stream *strm = &dev_data->tx;
	struct i2s_q_entry q_entry = {.mem_block = mem_block, .size = size};
	unsigned int key;
	int ret;

	LOG_DBG("i2s_mcux_write");
	if (strm->state != I2S_STATE_RUNNING && strm->state != I2S_STATE_READY) {
		LOG_ERR("invalid state (%d)", strm->state);
		return -EIO;
	}

	ret = k_msgq_put(&strm->in_queue, &q_entry, SYS_TIMEOUT_MS(strm->cfg.timeout));
	if (ret) {
		LOG_DBG("k_msgq_put returned code %d", ret);
		return ret;
	}

	/*
	 * The driver owns the block from here on, so the write succeeded,
	 * whatever happens below.
	 *
	 * While the stream runs and the DMA has a free TCD slot, hand the new
	 * block to the DMA now instead of waiting for the next TX callback.
	 * Otherwise the DMA can finish its last queued block before the
	 * callback reloads it, the FIFO underruns and the stream moves to
	 * I2S_STATE_ERROR although the application wrote data in time.
	 *
	 * Check the state again with interrupts locked: an underrun flagged by
	 * the DMA callback or the FIFO error interrupt since the check above
	 * stopped the DMA channel, which must not be reloaded. A block the DMA
	 * rejects stays on in_queue for the next callback or PREPARE/DROP.
	 */
	key = irq_lock();
	if (strm->state == I2S_STATE_RUNNING && strm->free_tx_dma_blocks > 0) {
		uint8_t blocks_queued = 0;

		(void)i2s_tx_reload_multiple_dma_blocks(dev, &blocks_queued);
	}
	irq_unlock(key);

	return 0;
}

static void sai_driver_irq(const struct device *dev)
{
	I2S_Type *base = get_base(dev);
	struct i2s_dev_data *dev_data = dev->data;

	if ((base->TCSR & I2S_TCSR_FEF_MASK) == I2S_TCSR_FEF_MASK) {
		struct stream *strm = &dev_data->tx;

		/*
		 * Handle the underrun exactly like the TX DMA callback does, so
		 * that the outcome does not depend on which of the two detects
		 * it first. Only PREPARE/DROP performs the full disable.
		 */
		if (strm->state == I2S_STATE_RUNNING) {
			i2s_tx_stream_set_error(dev);
		}

		/* Clear FIFO error flag to continue transfer */
		SAI_TxClearStatusFlags(base, I2S_TCSR_FEF_MASK);
		/* Reset FIFO for safety */
		SAI_TxSoftwareReset(base, kSAI_ResetTypeFIFO);

		LOG_DBG("sai tx error occurred");
	}

	if ((base->RCSR & I2S_RCSR_FEF_MASK) == I2S_RCSR_FEF_MASK) {
		struct stream *strm = &dev_data->rx;

		/*
		 * Handle the overrun like the RX DMA callback does. Only
		 * PREPARE/DROP releases the blocks already received.
		 */
		if (strm->state == I2S_STATE_RUNNING || strm->state == I2S_STATE_STOPPING) {
			i2s_rx_stream_set_error(dev);
		}

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
	I2S_Type *base = get_base(dev);

	if ((base->TCSR & I2S_TCSR_FEF_MASK) == I2S_TCSR_FEF_MASK) {
		sai_driver_irq(dev);
	}

	if ((base->RCSR & I2S_RCSR_FEF_MASK) == I2S_RCSR_FEF_MASK) {
		sai_driver_irq(dev);
	}
	/*
	 * Add for ARM errata 838869, affects Cortex-M4,
	 * Cortex-M4F Store immediate overlapping exception return operation
	 *  might vector to incorrect interrupt
	 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	barrier_dsync_fence_full();
#endif
}

static void audio_clock_settings(const struct device *dev)
{
#ifdef CONFIG_I2S_HAS_PLL_SETTING
	clock_audio_pll_config_t audioPllConfig;
	const struct i2s_mcux_config *dev_cfg = dev->config;
	uint32_t clock_name = (uint32_t)dev_cfg->clk_sub_sys;

	/*Clock setting for SAI*/
	imxrt_audio_codec_pll_init(clock_name, dev_cfg->clk_src, dev_cfg->clk_pre_div,
				   dev_cfg->clk_src_div);

#if defined(CONFIG_SOC_SERIES_IMXRT11XX) || defined(CONFIG_SOC_SERIES_IMXRT118X)
	audioPllConfig.loopDivider = dev_cfg->pll_lp;
	audioPllConfig.postDivider = dev_cfg->pll_pd;
	audioPllConfig.numerator = dev_cfg->pll_num;
	audioPllConfig.denominator = dev_cfg->pll_den;
	audioPllConfig.ssEnable = false;
#elif defined CONFIG_SOC_SERIES_IMXRT10XX
	audioPllConfig.src = dev_cfg->pll_src;
	audioPllConfig.loopDivider = dev_cfg->pll_lp;
	audioPllConfig.postDivider = dev_cfg->pll_pd;
	audioPllConfig.numerator = dev_cfg->pll_num;
	audioPllConfig.denominator = dev_cfg->pll_den;
#else
#error Initialize SOC Series-specific clock_audio_pll_config_t
#endif /* CONFIG_SOC_SERIES */

	CLOCK_InitAudioPll(&audioPllConfig);
#else
	set_mclk_rate(dev, 12288000);
#endif
}

static int i2s_mcux_initialize(const struct device *dev)
{
	const struct i2s_mcux_config *dev_cfg = dev->config;
	I2S_Type *base;
	struct i2s_dev_data *dev_data = dev->data;
	uint32_t mclk;
	int err;

	if (!dev_data->dev_dma) {
		LOG_ERR("DMA device not found");
		return -ENODEV;
	}

	DEVICE_MMIO_NAMED_MAP(dev, sai_mmio, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	base = get_base(dev);

	/* Initialize the entry queues */
	k_msgq_init(&dev_data->tx.in_queue, (char *)dev_data->tx_in_msgs,
		    sizeof(struct i2s_q_entry), CONFIG_I2S_TX_BLOCK_COUNT);
	k_msgq_init(&dev_data->rx.in_queue, (char *)dev_data->rx_in_msgs,
		    sizeof(struct i2s_q_entry), CONFIG_I2S_RX_BLOCK_COUNT);
	k_msgq_init(&dev_data->tx.out_queue, (char *)dev_data->tx_out_msgs,
		    sizeof(struct i2s_q_entry), TX_OUT_QUEUE_LEN);
	k_msgq_init(&dev_data->rx.out_queue, (char *)dev_data->rx_out_msgs,
		    sizeof(struct i2s_q_entry), CONFIG_I2S_RX_BLOCK_COUNT);

	/* register ISR */
	dev_cfg->irq_connect(dev);

	if (dev_cfg->reset.dev != NULL) {
		if (!device_is_ready(dev_cfg->reset.dev)) {
			LOG_ERR("reset controller not ready");
			return -ENODEV;
		}

		err = reset_line_deassert_dt(&dev_cfg->reset);
		if (err != 0) {
			LOG_ERR("Failed to deassert reset line (%d)", err);
			return err;
		}
	}

	/* pinctrl */
	err = pinctrl_apply_state(dev_cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("mclk pinctrl setup failed (%d)", err);
		return err;
	}

	/*clock configuration*/
	audio_clock_settings(dev);

	if (dev_cfg->mclk_control_base) {
		enable_mclk_direction(dev, dev_cfg->mclk_output);
	}

	SAI_Init(base);

	dev_data->tx.state = I2S_STATE_NOT_READY;
	dev_data->rx.state = I2S_STATE_NOT_READY;

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) ||                             \
	(defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
	sai_master_clock_t mclkConfig = {
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
		.mclkOutputEnable = true,
#if !(defined(FSL_FEATURE_SAI_HAS_NO_MCR_MICS) && (FSL_FEATURE_SAI_HAS_NO_MCR_MICS))
		.mclkSource = kSAI_MclkSourceSysclk,
#endif
#endif
	};
#endif

	get_mclk_rate(dev, &mclk);
/* master clock configurations */
#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) ||                             \
	(defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
#if ((defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)) || \
	(defined(FSL_FEATURE_SAI_HAS_MCR_MCLK_POST_DIV) && (FSL_FEATURE_SAI_HAS_MCR_MCLK_POST_DIV)))
	mclkConfig.mclkHz = mclk;
	mclkConfig.mclkSourceClkHz = mclk;
#endif
	SAI_SetMasterClockConfig(base, &mclkConfig);
#endif

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(i2s, i2s_mcux_driver_api) = {
	.configure = i2s_mcux_config,
	.read = i2s_mcux_read,
	.write = i2s_mcux_write,
	.config_get = i2s_mcux_config_get,
	.trigger = i2s_mcux_trigger,
};

#define I2S_MCUX_PINMUX_INIT(i2s_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(i2s_id), pinmuxes),                               \
		(.mclk_control_base = DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(i2s_id), pinmuxes)),      \
		.mclk_pin_mask = DT_PHA_BY_IDX(DT_DRV_INST(i2s_id), pinmuxes, 0, mask),            \
		.mclk_pin_offset = DT_PHA_BY_IDX(DT_DRV_INST(i2s_id), pinmuxes, 0, offset),),      \
		(.mclk_control_base = 0,                                                           \
		.mclk_pin_mask = 0,                                                                \
		.mclk_pin_offset = 0,))

#define I2S_MCUX_INIT(i2s_id)                                                                      \
	static void i2s_irq_connect_##i2s_id(const struct device *dev);                            \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(i2s_id);                                                            \
                                                                                                   \
	static const struct i2s_mcux_config i2s_##i2s_id##_config = {                              \
		DEVICE_MMIO_NAMED_ROM_INIT(sai_mmio, DT_DRV_INST(i2s_id)),                         \
		.clk_src = DT_INST_PROP_OR(i2s_id, clock_mux, 0),                                  \
		.clk_pre_div = DT_INST_PROP_OR(i2s_id, pre_div, 0),                                \
		.clk_src_div = DT_INST_PROP_OR(i2s_id, podf, 0),                                   \
		.pll_src = DT_PHA_BY_NAME_OR(DT_DRV_INST(i2s_id), pll_clocks, src, value, 0),      \
		.pll_lp = DT_PHA_BY_NAME_OR(DT_DRV_INST(i2s_id), pll_clocks, lp, value, 0),        \
		.pll_pd = DT_PHA_BY_NAME_OR(DT_DRV_INST(i2s_id), pll_clocks, pd, value, 0),        \
		.pll_num = DT_PHA_BY_NAME_OR(DT_DRV_INST(i2s_id), pll_clocks, num, value, 0),      \
		.pll_den = DT_PHA_BY_NAME_OR(DT_DRV_INST(i2s_id), pll_clocks, den, value, 0),      \
		I2S_MCUX_PINMUX_INIT(i2s_id)                                                       \
		.mclk_output = DT_INST_PROP_OR(i2s_id, mclk_output, 0),                            \
		.clk_sub_sys =                                                                     \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(i2s_id, 0, name),       \
		.ccm_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(i2s_id)),                             \
		.reset = RESET_DT_SPEC_INST_GET_OR(i2s_id, {0}),                                   \
		.irq_connect = i2s_irq_connect_##i2s_id,                                           \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(i2s_id),                                 \
		.tx_sync_mode =                                                                    \
			DT_INST_PROP(i2s_id, nxp_tx_sync_mode) ? kSAI_ModeSync : kSAI_ModeAsync,   \
		.rx_sync_mode =                                                                    \
			DT_INST_PROP(i2s_id, nxp_rx_sync_mode) ? kSAI_ModeSync : kSAI_ModeAsync,   \
		.tx_channel = DT_INST_PROP(i2s_id, nxp_tx_channel),                                \
		.bclk_source = DT_INST_PROP(i2s_id, nxp_bclk_source),                              \
	};                                                                                         \
                                                                                                   \
	static struct i2s_dev_data i2s_##i2s_id##_data = {                                         \
		.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(i2s_id, rx)),                   \
		.tx =                                                                              \
			{                                                                          \
				.dma_channel = DT_INST_PROP(i2s_id, nxp_tx_dma_channel),           \
				.dma_cfg =                                                         \
					{                                                          \
						.source_burst_length = CONFIG_I2S_EDMA_BURST_SIZE, \
						.dest_burst_length = CONFIG_I2S_EDMA_BURST_SIZE,   \
						.dma_callback = i2s_dma_tx_callback,               \
						.complete_callback_en = 1,                         \
						.error_callback_dis = 1,                           \
						.block_count = 1,                                  \
						.head_block = &i2s_##i2s_id##_data.tx.dma_block,   \
						.channel_direction = MEMORY_TO_PERIPHERAL,         \
						.dma_slot = DT_INST_DMAS_CELL_BY_NAME(i2s_id, tx,  \
										      source),     \
						.cyclic = 1,                                       \
					},                                                         \
			},                                                                         \
		.rx =                                                                              \
			{                                                                          \
				.dma_channel = DT_INST_PROP(i2s_id, nxp_rx_dma_channel),           \
				.dma_cfg =                                                         \
					{                                                          \
						.source_burst_length = CONFIG_I2S_EDMA_BURST_SIZE, \
						.dest_burst_length = CONFIG_I2S_EDMA_BURST_SIZE,   \
						.dma_callback = i2s_dma_rx_callback,               \
						.complete_callback_en = 1,                         \
						.error_callback_dis = 1,                           \
						.block_count = 1,                                  \
						.head_block = &i2s_##i2s_id##_data.rx.dma_block,   \
						.channel_direction = PERIPHERAL_TO_MEMORY,         \
						.dma_slot = DT_INST_DMAS_CELL_BY_NAME(i2s_id, rx,  \
										      source),     \
						.cyclic = 1,                                       \
					},                                                         \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i2s_id, &i2s_mcux_initialize, NULL, &i2s_##i2s_id##_data,            \
			      &i2s_##i2s_id##_config, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,       \
			      &i2s_mcux_driver_api);                                               \
                                                                                                   \
	static void i2s_irq_connect_##i2s_id(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(i2s_id, 0, irq),                                    \
			    DT_INST_IRQ_BY_IDX(i2s_id, 0, priority), i2s_mcux_isr,                 \
			    DEVICE_DT_INST_GET(i2s_id), 0);                                        \
		irq_enable(DT_INST_IRQN(i2s_id));                                                  \
	}

DT_INST_FOREACH_STATUS_OKAY(I2S_MCUX_INIT)
