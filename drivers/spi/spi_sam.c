/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 qianfan Zhao.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_sam);

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <soc.h>

#define SAM_SPI_CHIP_SELECT_COUNT			4

/* Device constant configuration parameters */
struct spi_sam_config {
	Spi *regs;
	uint32_t periph_id;
	uint32_t num_pins;
	struct soc_gpio_pin pins[];
};

/* Device run time data */
struct spi_sam_data {
	struct spi_context ctx;
};

static int spi_slave_to_mr_pcs(int slave)
{
	int pcs[SAM_SPI_CHIP_SELECT_COUNT] = {0x0, 0x1, 0x3, 0x7};

	/* SPI worked in fixed perieral mode(SPI_MR.PS = 0) and disabled chip
	 * select decode(SPI_MR.PCSDEC = 0), based on Atmel | SMART ARM-based
	 * Flash MCU DATASHEET 40.8.2 SPI Mode Register:
	 * PCS = xxx0    NPCS[3:0] = 1110
	 * PCS = xx01    NPCS[3:0] = 1101
	 * PCS = x011    NPCS[3:0] = 1011
	 * PCS = 0111    NPCS[3:0] = 0111
	 */

	return pcs[slave];
}

static int spi_sam_configure(const struct device *dev,
			     const struct spi_config *config)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	uint32_t spi_mr = 0U, spi_csr = 0U;
	int div;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	if (config->slave > (SAM_SPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("Slave %d is greater than %d",
			config->slave, SAM_SPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	/* Set master mode, disable mode fault detection, set fixed peripheral
	 * select mode.
	 */
	spi_mr |= (SPI_MR_MSTR | SPI_MR_MODFDIS);
	spi_mr |= SPI_MR_PCS(spi_slave_to_mr_pcs(config->slave));

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		spi_csr |= SPI_CSR_CPOL;
	}

	if ((config->operation & SPI_MODE_CPHA) == 0U) {
		spi_csr |= SPI_CSR_NCPHA;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	} else {
		spi_csr |= SPI_CSR_BITS(SPI_CSR_BITS_8_BIT);
	}

	/* Use the requested or next highest possible frequency */
	div = SOC_ATMEL_SAM_MCK_FREQ_HZ / config->frequency;
	div = MAX(1, MIN(UINT8_MAX, div));
	spi_csr |= SPI_CSR_SCBR(div);

	regs->SPI_CR = SPI_CR_SPIDIS; /* Disable SPI */
	regs->SPI_MR = spi_mr;
	regs->SPI_CSR[config->slave] = spi_csr;
	regs->SPI_CR = SPI_CR_SPIEN; /* Enable SPI */

	data->ctx.config = config;
	spi_context_cs_configure(&data->ctx);

	return 0;
}

static bool spi_sam_transfer_ongoing(struct spi_sam_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void spi_sam_shift_master(Spi *regs, struct spi_sam_data *data)
{
	uint8_t tx;
	uint8_t rx;

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx = *(uint8_t *)(data->ctx.tx_buf);
	} else {
		tx = 0U;
	}

	while ((regs->SPI_SR & SPI_SR_TDRE) == 0) {
	}

	regs->SPI_TDR = SPI_TDR_TD(tx);
	spi_context_update_tx(&data->ctx, 1, 1);

	while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
	}

	rx = (uint8_t)regs->SPI_RDR;

	if (spi_context_rx_buf_on(&data->ctx)) {
		*data->ctx.rx_buf = rx;
	}
	spi_context_update_rx(&data->ctx, 1, 1);
}

/* Finish any ongoing writes and drop any remaining read data */
static void spi_sam_finish(Spi *regs)
{
	while ((regs->SPI_SR & SPI_SR_TXEMPTY) == 0) {
	}

	while (regs->SPI_SR & SPI_SR_RDRF) {
		(void)regs->SPI_RDR;
	}
}

/* Fast path that transmits a buf */
static void spi_sam_fast_tx(Spi *regs, const struct spi_buf *tx_buf)
{
	const uint8_t *p = tx_buf->buf;
	const uint8_t *pend = (uint8_t *)tx_buf->buf + tx_buf->len;
	uint8_t ch;

	while (p != pend) {
		ch = *p++;

		while ((regs->SPI_SR & SPI_SR_TDRE) == 0) {
		}

		regs->SPI_TDR = SPI_TDR_TD(ch);
	}

	spi_sam_finish(regs);
}

/* Fast path that reads into a buf */
static void spi_sam_fast_rx(Spi *regs, const struct spi_buf *rx_buf)
{
	uint8_t *rx = rx_buf->buf;
	int len = rx_buf->len;

	if (len <= 0) {
		return;
	}

	/* See the comment in spi_sam_fast_txrx re: interleaving. */

	/* Write the first byte */
	regs->SPI_TDR = SPI_TDR_TD(0);
	len--;

	while (len) {
		while ((regs->SPI_SR & SPI_SR_TDRE) == 0) {
		}

		/* Load byte N+1 into the transmit register */
		regs->SPI_TDR = SPI_TDR_TD(0);
		len--;

		/* Read byte N+0 from the receive register */
		while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
		}

		*rx++ = (uint8_t)regs->SPI_RDR;
	}

	/* Read the final incoming byte */
	while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
	}

	*rx = (uint8_t)regs->SPI_RDR;

	spi_sam_finish(regs);
}

/* Fast path that writes and reads bufs of the same length */
static void spi_sam_fast_txrx(Spi *regs,
			      const struct spi_buf *tx_buf,
			      const struct spi_buf *rx_buf)
{
	const uint8_t *tx = tx_buf->buf;
	const uint8_t *txend = (uint8_t *)tx_buf->buf + tx_buf->len;
	uint8_t *rx = rx_buf->buf;
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
	regs->SPI_TDR = SPI_TDR_TD(*tx++);

	while (tx != txend) {
		while ((regs->SPI_SR & SPI_SR_TDRE) == 0) {
		}

		/* Load byte N+1 into the transmit register.  TX is
		 * single buffered and we have at most one byte in
		 * flight so skip the DRE check.
		 */
		regs->SPI_TDR = SPI_TDR_TD(*tx++);

		/* Read byte N+0 from the receive register */
		while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
		}

		*rx++ = (uint8_t)regs->SPI_RDR;
	}

	/* Read the final incoming byte */
	while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
	}

	*rx = (uint8_t)regs->SPI_RDR;

	spi_sam_finish(regs);
}

/* Fast path where every overlapping tx and rx buffer is the same length */
static void spi_sam_fast_transceive(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam_config *cfg = dev->config;
	size_t tx_count = 0;
	size_t rx_count = 0;
	Spi *regs = cfg->regs;
	const struct spi_buf *tx = NULL;
	const struct spi_buf *rx = NULL;

	if (tx_bufs) {
		tx = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs) {
		rx = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	}

	while (tx_count != 0 && rx_count != 0) {
		if (tx->buf == NULL) {
			spi_sam_fast_rx(regs, rx);
		} else if (rx->buf == NULL) {
			spi_sam_fast_tx(regs, tx);
		} else {
			spi_sam_fast_txrx(regs, tx, rx);
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	for (; tx_count != 0; tx_count--) {
		spi_sam_fast_tx(regs, tx++);
	}

	for (; rx_count != 0; rx_count--) {
		spi_sam_fast_rx(regs, rx++);
	}
}

/* Returns true if the request is suitable for the fast
 * path. Specifically, the bufs are a sequence of:
 *
 * - Zero or more RX and TX buf pairs where each is the same length.
 * - Zero or more trailing RX only bufs
 * - Zero or more trailing TX only bufs
 */
static bool spi_sam_is_regular(const struct spi_buf_set *tx_bufs,
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
		return true;
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

static int spi_sam_transceive(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	int err;

	spi_context_lock(&data->ctx, false, NULL);

	err = spi_sam_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_cs_control(&data->ctx, true);

	/* This driver special cases the common send only, receive
	 * only, and transmit then receive operations.	This special
	 * casing is 4x faster than the spi_context() routines
	 * and allows the transmit and receive to be interleaved.
	 */
	if (spi_sam_is_regular(tx_bufs, rx_bufs)) {
		spi_sam_fast_transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

		do {
			spi_sam_shift_master(regs, data);
		} while (spi_sam_transfer_ongoing(data));
	}

	spi_context_cs_control(&data->ctx, false);

done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int spi_sam_transceive_sync(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_sam_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_sam_transceive_async(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	/* TODO: implement asyc transceive */
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_sam_release(const struct device *dev,
			   const struct spi_config *config)
{
	struct spi_sam_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_sam_init(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;

	soc_pmc_peripheral_enable(cfg->periph_id);

	soc_gpio_list_configure(cfg->pins, cfg->num_pins);

	spi_context_unlock_unconditionally(&data->ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static const struct spi_driver_api spi_sam_driver_api = {
	.transceive = spi_sam_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sam_transceive_async,
#endif
	.release = spi_sam_release,
};

#define SPI_SAM_DEFINE_CONFIG(n)					\
	static const struct spi_sam_config spi_sam_config_##n = {	\
		.regs = (Spi *)DT_INST_REG_ADDR(n),			\
		.periph_id = DT_INST_PROP(n, peripheral_id),		\
		.num_pins = ATMEL_SAM_DT_NUM_PINS(n),			\
		.pins = ATMEL_SAM_DT_PINS(n),				\
	}

#define SPI_SAM_DEVICE_INIT(n)						\
	SPI_SAM_DEFINE_CONFIG(n);					\
	static struct spi_sam_data spi_sam_dev_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_sam_dev_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_sam_dev_data_##n, ctx),	\
	};								\
	DEVICE_AND_API_INIT(spi_sam_##n,				\
			    DT_INST_LABEL(n),				\
			    &spi_sam_init, &spi_sam_dev_data_##n,	\
			    &spi_sam_config_##n, POST_KERNEL,		\
			    CONFIG_SPI_INIT_PRIORITY, &spi_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM_DEVICE_INIT)
