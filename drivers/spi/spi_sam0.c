/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <spi.h>
#include <soc.h>
#include <board.h>

/* Device constant configuration parameters */
struct spi_sam0_config {
	SercomSpi *regs;
	u32_t pads;
	u32_t pm_apbcmask;
	u16_t gclk_clkctrl_id;
};

/* Device run time data */
struct spi_sam0_data {
	struct spi_context ctx;
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
	SercomSpi *regs = cfg->regs;
	SERCOM_SPI_CTRLA_Type ctrla = {.reg = 0};
	SERCOM_SPI_CTRLB_Type ctrlb = {.reg = 0};
	int div;

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	ctrla.bit.MODE = SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val;

	if ((config->operation & SPI_TRANSFER_LSB) != 0) {
		ctrla.bit.DORD = 1;
	}

	if ((config->operation & SPI_MODE_CPOL) != 0) {
		ctrla.bit.CPOL = 1;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0) {
		ctrla.bit.CPHA = 1;
	}

	ctrla.reg |= cfg->pads;

	if ((config->operation & SPI_MODE_LOOP) != 0) {
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

	/* Use the requested or next higest possible frequency */
	div = (SOC_ATMEL_SAM0_GCLK0_FREQ_HZ / config->frequency) / 2 - 1;
	div = max(0, min(UINT8_MAX, div));

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
		tx = 0;
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
	const u8_t *pend = tx_buf->buf + tx_buf->len;
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
	const u8_t *txend = tx_buf->buf + tx_buf->len;
	u8_t *rx = rx_buf->buf;
	size_t len = rx_buf->len;

	if (len <= 0) {
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
	const struct spi_buf *tx;
	const struct spi_buf *rx;

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

	data->ctx.config = config;
	spi_context_cs_configure(&data->ctx);
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

#ifdef CONFIG_SPI_ASYNC
static int spi_sam0_transceive_async(struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	return -ENOTSUP;
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

	spi_context_unlock_unconditionally(&data->ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static const struct spi_driver_api spi_sam0_driver_api = {
	.transceive = spi_sam0_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sam0_transceive_async,
#endif
	.release = spi_sam0_release,
};

#define SPI_SAM0_DEFINE_CONFIG(n)                                            \
	static const struct spi_sam0_config spi_sam0_config_##n = {          \
		.regs = (SercomSpi *)CONFIG_SPI_SAM0_SERCOM##n##_BASE_ADDRESS, \
		.pm_apbcmask = PM_APBCMASK_SERCOM##n,                        \
		.gclk_clkctrl_id = GCLK_CLKCTRL_ID_SERCOM##n##_CORE,         \
		.pads = CONFIG_SPI_SAM0_SERCOM##n##_PADS                     \
	}

#define SPI_SAM0_DEVICE_INIT(n)                                              \
	SPI_SAM0_DEFINE_CONFIG(n);                                           \
	static struct spi_sam0_data spi_sam0_dev_data_##n = {                \
		SPI_CONTEXT_INIT_LOCK(spi_sam0_dev_data_##n, ctx),           \
		SPI_CONTEXT_INIT_SYNC(spi_sam0_dev_data_##n, ctx),           \
	};                                                                   \
	DEVICE_AND_API_INIT(spi_sam0_##n, \
			    CONFIG_SPI_SAM0_SERCOM##n##_LABEL,               \
			    &spi_sam0_init, &spi_sam0_dev_data_##n,          \
			    &spi_sam0_config_##n, POST_KERNEL,               \
			    CONFIG_SPI_INIT_PRIORITY, &spi_sam0_driver_api)

#if CONFIG_SPI_SAM0_SERCOM0_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(0);
#endif

#if CONFIG_SPI_SAM0_SERCOM1_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(1);
#endif

#if CONFIG_SPI_SAM0_SERCOM2_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(2);
#endif

#if CONFIG_SPI_SAM0_SERCOM3_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(3);
#endif

#if CONFIG_SPI_SAM0_SERCOM4_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(4);
#endif

#if CONFIG_SPI_SAM0_SERCOM5_BASE_ADDRESS
SPI_SAM0_DEVICE_INIT(5);
#endif
