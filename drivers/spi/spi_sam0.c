/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_sam0);

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <soc.h>
#include <drivers/dma.h>

/* Device constant configuration parameters */
struct spi_sam0_config {
	SercomSpi *regs;
	u32_t pads;
	u32_t pm_apbcmask;
	u16_t gclk_clkctrl_id;
#ifdef CONFIG_SPI_ASYNC
	u8_t tx_dma_request;
	u8_t tx_dma_channel;
	u8_t rx_dma_request;
	u8_t rx_dma_channel;
#endif
};

/* Device run time data */
struct spi_sam0_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_ASYNC
	struct device *dma;
	u32_t dma_segment_len;
#endif
};

static void wait_synchronization(SercomSpi *regs)
{
#if defined(SERCOM_SPI_SYNCBUSY_MASK)
	/* SYNCBUSY is a register */
	while ((regs->SYNCBUSY.reg & SERCOM_SPI_SYNCBUSY_MASK) != 0) {
	}
#elif defined(SERCOM_SPI_STATUS_SYNCBUSY)
	/* SYNCBUSY is a bit */
	while ((regs->STATUS.reg & SERCOM_SPI_STATUS_SYNCBUSY) != 0) {
	}
#else
#error Unsupported device
#endif
}

static int spi_sam0_configure(struct device *dev,
			      const struct spi_config *config)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	SercomSpi *regs = cfg->regs;
	SERCOM_SPI_CTRLA_Type ctrla = {.reg = 0};
	SERCOM_SPI_CTRLB_Type ctrlb = {.reg = 0};
	int div;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	ctrla.bit.MODE = SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val;

	if ((config->operation & SPI_TRANSFER_LSB) != 0U) {
		ctrla.bit.DORD = 1;
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		ctrla.bit.CPOL = 1;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		ctrla.bit.CPHA = 1;
	}

	ctrla.reg |= cfg->pads;

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		/* Put MISO and MOSI on the same pad */
		ctrla.bit.DOPO = 0;
		ctrla.bit.DIPO = 0;
	}

	ctrla.bit.ENABLE = 1;
	ctrlb.bit.RXEN = 1;

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	}

	/* 8 bits per transfer */
	ctrlb.bit.CHSIZE = 0;

	/* Use the requested or next highest possible frequency */
	div = (SOC_ATMEL_SAM0_GCLK0_FREQ_HZ / config->frequency) / 2U - 1;
	div = MAX(0, MIN(UINT8_MAX, div));

	/* Update the configuration only if it has changed */
	if (regs->CTRLA.reg != ctrla.reg || regs->CTRLB.reg != ctrlb.reg ||
	    regs->BAUD.reg != div) {
		regs->CTRLA.bit.ENABLE = 0;
		wait_synchronization(regs);

		regs->CTRLB = ctrlb;
		wait_synchronization(regs);
		regs->BAUD.reg = div;
		wait_synchronization(regs);
		regs->CTRLA = ctrla;
		wait_synchronization(regs);
	}

	data->ctx.config = config;
	spi_context_cs_configure(&data->ctx);

	return 0;
}

static bool spi_sam0_transfer_ongoing(struct spi_sam0_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void spi_sam0_shift_master(SercomSpi *regs, struct spi_sam0_data *data)
{
	u8_t tx;
	u8_t rx;

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx = *(u8_t *)(data->ctx.tx_buf);
	} else {
		tx = 0U;
	}

	while (!regs->INTFLAG.bit.DRE) {
	}

	regs->DATA.reg = tx;
	spi_context_update_tx(&data->ctx, 1, 1);

	while (!regs->INTFLAG.bit.RXC) {
	}

	rx = regs->DATA.reg;

	if (spi_context_rx_buf_on(&data->ctx)) {
		*data->ctx.rx_buf = rx;
	}
	spi_context_update_rx(&data->ctx, 1, 1);
}

/* Finish any ongoing writes and drop any remaining read data */
static void spi_sam0_finish(SercomSpi *regs)
{
	while (!regs->INTFLAG.bit.TXC) {
	}

	while (regs->INTFLAG.bit.RXC) {
		(void)regs->DATA.reg;
	}
}

/* Fast path that transmits a buf */
static void spi_sam0_fast_tx(SercomSpi *regs, const struct spi_buf *tx_buf)
{
	const u8_t *p = tx_buf->buf;
	const u8_t *pend = (u8_t *)tx_buf->buf + tx_buf->len;
	u8_t ch;

	while (p != pend) {
		ch = *p++;

		while (!regs->INTFLAG.bit.DRE) {
		}

		regs->DATA.reg = ch;
	}

	spi_sam0_finish(regs);
}

/* Fast path that reads into a buf */
static void spi_sam0_fast_rx(SercomSpi *regs, const struct spi_buf *rx_buf)
{
	u8_t *rx = rx_buf->buf;
	int len = rx_buf->len;

	if (len <= 0) {
		return;
	}

	/* See the comment in spi_sam0_fast_txrx re: interleaving. */

	/* Write the first byte */
	regs->DATA.reg = 0;
	len--;

	while (len) {
		/* Load byte N+1 into the transmit register */
		regs->DATA.reg = 0;
		len--;

		/* Read byte N+0 from the receive register */
		while (!regs->INTFLAG.bit.RXC) {
		}

		*rx++ = regs->DATA.reg;
	}

	/* Read the final incoming byte */
	while (!regs->INTFLAG.bit.RXC) {
	}

	*rx = regs->DATA.reg;

	spi_sam0_finish(regs);
}

/* Fast path that writes and reads bufs of the same length */
static void spi_sam0_fast_txrx(SercomSpi *regs,
			       const struct spi_buf *tx_buf,
			       const struct spi_buf *rx_buf)
{
	const u8_t *tx = tx_buf->buf;
	const u8_t *txend = (u8_t *)tx_buf->buf + tx_buf->len;
	u8_t *rx = rx_buf->buf;
	size_t len = rx_buf->len;

	if (len == 0) {
		return;
	}

	/*
	 * The code below interleaves the transmit writes with the
	 * receive reads to keep the bus fully utilised.  The code is
	 * equivalent to:
	 *
	 * Transmit byte 0
	 * Loop:
	 * - Transmit byte n+1
	 * - Receive byte n
	 * Receive the final byte
	 */

	/* Write the first byte */
	regs->DATA.reg = *tx++;

	while (tx != txend) {
		/* Load byte N+1 into the transmit register.  TX is
		 * single buffered and we have at most one byte in
		 * flight so skip the DRE check.
		 */
		regs->DATA.reg = *tx++;

		/* Read byte N+0 from the receive register */
		while (!regs->INTFLAG.bit.RXC) {
		}

		*rx++ = regs->DATA.reg;
	}

	/* Read the final incoming byte */
	while (!regs->INTFLAG.bit.RXC) {
	}

	*rx = regs->DATA.reg;

	spi_sam0_finish(regs);
}

/* Fast path where every overlapping tx and rx buffer is the same length */
static void spi_sam0_fast_transceive(struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	size_t tx_count = 0;
	size_t rx_count = 0;
	SercomSpi *regs = cfg->regs;
	const struct spi_buf *tx = NULL;
	const struct spi_buf *rx = NULL;

	if (tx_bufs) {
		tx = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs) {
		rx = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	} else {
		rx = NULL;
	}

	while (tx_count != 0 && rx_count != 0) {
		if (tx->buf == NULL) {
			spi_sam0_fast_rx(regs, rx);
		} else if (rx->buf == NULL) {
			spi_sam0_fast_tx(regs, tx);
		} else {
			spi_sam0_fast_txrx(regs, tx, rx);
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	for (; tx_count != 0; tx_count--) {
		spi_sam0_fast_tx(regs, tx++);
	}

	for (; rx_count != 0; rx_count--) {
		spi_sam0_fast_rx(regs, rx++);
	}
}

/* Returns true if the request is suitable for the fast
 * path. Specifically, the bufs are a sequence of:
 *
 * - Zero or more RX and TX buf pairs where each is the same length.
 * - Zero or more trailing RX only bufs
 * - Zero or more trailing TX only bufs
 */
static bool spi_sam0_is_regular(const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	const struct spi_buf *tx = NULL;
	const struct spi_buf *rx = NULL;
	size_t tx_count = 0;
	size_t rx_count = 0;

	if (tx_bufs) {
		tx = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs) {
		rx = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	}

	if (!tx || !rx) {
		return false;
	}

	while (tx_count != 0 && rx_count != 0) {
		if (tx->len != rx->len) {
			return false;
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	return true;
}

static int spi_sam0_transceive(struct device *dev,
			       const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	SercomSpi *regs = cfg->regs;
	int err;

	spi_context_lock(&data->ctx, false, NULL);

	err = spi_sam0_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_cs_control(&data->ctx, true);

	/* This driver special cases the common send only, receive
	 * only, and transmit then receive operations.	This special
	 * casing is 4x faster than the spi_context() routines
	 * and allows the transmit and receive to be interleaved.
	 */
	if (spi_sam0_is_regular(tx_bufs, rx_bufs)) {
		spi_sam0_fast_transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

		do {
			spi_sam0_shift_master(regs, data);
		} while (spi_sam0_transfer_ongoing(data));
	}

	spi_context_cs_control(&data->ctx, false);

done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int spi_sam0_transceive_sync(struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_sam0_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC

static void spi_sam0_dma_rx_done(void *arg, u32_t id, int error_code);

static int spi_sam0_dma_rx_load(struct device *dev, u8_t *buf,
				size_t len)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	SercomSpi *regs = cfg->regs;
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_blk = { 0 };
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.callback_arg = dev;
	dma_cfg.dma_callback = spi_sam0_dma_rx_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->rx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.dest_address = (u32_t)buf;
	} else {
		static u8_t dummy;

		dma_blk.dest_address = (u32_t)&dummy;
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.source_address = (u32_t)(&(regs->DATA.reg));
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(data->dma, cfg->rx_dma_channel,
			    &dma_cfg);
	if (retval != 0) {
		return retval;
	}

	return dma_start(data->dma, cfg->rx_dma_channel);
}

static int spi_sam0_dma_tx_load(struct device *dev, const u8_t *buf,
				size_t len)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	SercomSpi *regs = cfg->regs;
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_blk = { 0 };
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->tx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.source_address = (u32_t)buf;
	} else {
		static const u8_t dummy;

		dma_blk.source_address = (u32_t)&dummy;
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.dest_address = (u32_t)(&(regs->DATA.reg));
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(data->dma, cfg->tx_dma_channel,
			    &dma_cfg);

	if (retval != 0) {
		return retval;
	}

	return dma_start(data->dma, cfg->tx_dma_channel);
}

static bool spi_sam0_dma_advance_segment(struct device *dev)
{
	struct spi_sam0_data *data = dev->driver_data;
	u32_t segment_len;

	/* Pick the shorter buffer of ones that have an actual length */
	if (data->ctx.rx_len != 0) {
		segment_len = data->ctx.rx_len;
		if (data->ctx.tx_len != 0) {
			segment_len = MIN(segment_len, data->ctx.tx_len);
		}
	} else {
		segment_len = data->ctx.tx_len;
	}

	if (segment_len == 0) {
		return false;
	}

	segment_len = MIN(segment_len, 65535);

	data->dma_segment_len = segment_len;
	return true;
}

static int spi_sam0_dma_advance_buffers(struct device *dev)
{
	struct spi_sam0_data *data = dev->driver_data;
	int retval;

	if (data->dma_segment_len == 0) {
		return -EINVAL;
	}

	/* Load receive first, so it can accept transmit data */
	if (data->ctx.rx_len) {
		retval = spi_sam0_dma_rx_load(dev, data->ctx.rx_buf,
					      data->dma_segment_len);
	} else {
		retval = spi_sam0_dma_rx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	/* Now load the transmit, which starts the actual bus clocking */
	if (data->ctx.tx_len) {
		retval = spi_sam0_dma_tx_load(dev, data->ctx.tx_buf,
					      data->dma_segment_len);
	} else {
		retval = spi_sam0_dma_tx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	return 0;
}

static void spi_sam0_dma_rx_done(void *arg, u32_t id, int error_code)
{
	struct device *dev = arg;
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	int retval;

	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	spi_context_update_tx(&data->ctx, 1, data->dma_segment_len);
	spi_context_update_rx(&data->ctx, 1, data->dma_segment_len);

	if (!spi_sam0_dma_advance_segment(dev)) {
		/* Done */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, 0);
		return;
	}

	retval = spi_sam0_dma_advance_buffers(dev);
	if (retval != 0) {
		dma_stop(data->dma, cfg->tx_dma_channel);
		dma_stop(data->dma, cfg->rx_dma_channel);
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, retval);
		return;
	}
}


static int spi_sam0_transceive_async(struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	int retval;

	if (!data->dma) {
		return -ENOTSUP;
	}

	/*
	 * Transmit clocks the output and we use receive to determine when
	 * the transmit is done, so we always need both
	 */
	if (cfg->tx_dma_channel == 0xFF || cfg->rx_dma_channel == 0xFF) {
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, true, async);

	retval = spi_sam0_configure(dev, config);
	if (retval != 0) {
		goto err_unlock;
	}

	spi_context_cs_control(&data->ctx, true);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_sam0_dma_advance_segment(dev);
	retval = spi_sam0_dma_advance_buffers(dev);
	if (retval != 0) {
		goto err_cs;
	}

	return 0;

err_cs:
	dma_stop(data->dma, cfg->tx_dma_channel);
	dma_stop(data->dma, cfg->rx_dma_channel);

	spi_context_cs_control(&data->ctx, false);

err_unlock:
	spi_context_release(&data->ctx, retval);
	return retval;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_sam0_release(struct device *dev,
			    const struct spi_config *config)
{
	struct spi_sam0_data *data = dev->driver_data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_sam0_init(struct device *dev)
{
	const struct spi_sam0_config *cfg = dev->config->config_info;
	struct spi_sam0_data *data = dev->driver_data;
	SercomSpi *regs = cfg->regs;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	/* Enable SERCOM clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;

	/* Disable all SPI interrupts */
	regs->INTENCLR.reg = SERCOM_SPI_INTENCLR_MASK;
	wait_synchronization(regs);

#ifdef CONFIG_SPI_ASYNC

	data->dma = device_get_binding(CONFIG_DMA_0_NAME);

#endif

	spi_context_unlock_unconditionally(&data->ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static const struct spi_driver_api spi_sam0_driver_api = {
	.transceive = spi_sam0_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sam0_transceive_async,
#endif
	.release = spi_sam0_release,
};

#if CONFIG_SPI_ASYNC
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_0_TXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_0_TXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_0_RXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_0_RXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_1_TXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_1_TXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_1_RXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_1_RXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_2_TXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_2_TXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_2_RXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_2_RXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_3_TXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_3_TXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_3_RXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_3_RXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_4_TXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_4_TXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_4_RXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_4_RXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_5_TXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_5_TXDMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_SPI_SERCOM_5_RXDMA
#define DT_ATMEL_SAM0_SPI_SERCOM_5_RXDMA 0xFF
#endif

#define SPI_SAM0_DMA_CHANNELS(n)                                             \
	.tx_dma_request = SERCOM##n##_DMAC_ID_TX,                            \
	.tx_dma_channel = DT_ATMEL_SAM0_SPI_SERCOM_##n##_TXDMA,              \
	.rx_dma_request = SERCOM##n##_DMAC_ID_RX,                            \
	.rx_dma_channel = DT_ATMEL_SAM0_SPI_SERCOM_##n##_RXDMA
#else
#define SPI_SAM0_DMA_CHANNELS(n)
#endif

#define SPI_SAM0_SERCOM_PADS(n) \
	SERCOM_SPI_CTRLA_DIPO(DT_ATMEL_SAM0_SPI_SERCOM_##n##_DIPO) | \
	SERCOM_SPI_CTRLA_DOPO(DT_ATMEL_SAM0_SPI_SERCOM_##n##_DOPO)

#define SPI_SAM0_DEFINE_CONFIG(n)                                            \
	static const struct spi_sam0_config spi_sam0_config_##n = {          \
		.regs = (SercomSpi *)DT_ATMEL_SAM0_SPI_SERCOM_##n##_BASE_ADDRESS,\
		.pm_apbcmask = PM_APBCMASK_SERCOM##n,                        \
		.gclk_clkctrl_id = GCLK_CLKCTRL_ID_SERCOM##n##_CORE,         \
		.pads = SPI_SAM0_SERCOM_PADS(n),                             \
		SPI_SAM0_DMA_CHANNELS(n)                                     \
	}

#define SPI_SAM0_DEVICE_INIT(n)                                              \
	SPI_SAM0_DEFINE_CONFIG(n);                                           \
	static struct spi_sam0_data spi_sam0_dev_data_##n = {                \
		SPI_CONTEXT_INIT_LOCK(spi_sam0_dev_data_##n, ctx),           \
		SPI_CONTEXT_INIT_SYNC(spi_sam0_dev_data_##n, ctx),           \
	};                                                                   \
	DEVICE_AND_API_INIT(spi_sam0_##n, \
			    DT_ATMEL_SAM0_SPI_SERCOM_##n##_LABEL,            \
			    &spi_sam0_init, &spi_sam0_dev_data_##n,          \
			    &spi_sam0_config_##n, POST_KERNEL,               \
			    CONFIG_SPI_INIT_PRIORITY, &spi_sam0_driver_api)

#if DT_ATMEL_SAM0_SPI_SERCOM_0_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(0);
#endif

#if DT_ATMEL_SAM0_SPI_SERCOM_1_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(1);
#endif

#if DT_ATMEL_SAM0_SPI_SERCOM_2_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(2);
#endif

#if DT_ATMEL_SAM0_SPI_SERCOM_3_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(3);
#endif

#if DT_ATMEL_SAM0_SPI_SERCOM_4_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(4);
#endif

#if DT_ATMEL_SAM0_SPI_SERCOM_5_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(5);
#endif
