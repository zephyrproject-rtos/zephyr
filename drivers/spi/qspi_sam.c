/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 qianfan Zhao.
 * Copyright (c) 2023 Gerson Fernando Budke.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_qspi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(qspi_sam);

#include "spi_context.h"
#include <errno.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#define SAM_QSPI_CHIP_SELECT_COUNT			4

/* Device constant configuration parameters */
struct qspi_sam_config {
	Qspi *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	const uint8_t width;
};

/* Device run time data */
struct qspi_sam_data {
	struct spi_context ctx;
	struct k_spinlock lock;
};

static inline k_spinlock_key_t spi_spin_lock(const struct device *dev)
{
	struct qspi_sam_data *data = dev->data;

	return k_spin_lock(&data->lock);
}

static inline void spi_spin_unlock(const struct device *dev, k_spinlock_key_t key)
{
	struct qspi_sam_data *data = dev->data;

	k_spin_unlock(&data->lock, key);
}

static int qspi_sam_configure(const struct device *dev,
			     const struct spi_config *config)
{
	const struct qspi_sam_config *cfg = dev->config;
	struct qspi_sam_data *data = dev->data;
	Qspi *regs = cfg->regs;
	uint32_t spi_mr = 0U, spi_csr = 0U;
	uint32_t mask;
	int div;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	if (config->slave > (SAM_QSPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("Slave %d is greater than %d",
			config->slave, SAM_QSPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	/* Set master mode, disable mode fault detection, set fixed peripheral
	 * select mode.
	 */
	/* Set SPI mode */
	spi_mr &= (~QSPI_MR_SMM_SPI);
	/* Disable local loopback */
	spi_mr &= (~QSPI_MR_LLB);
	/* Disable wait data read before transfer */
	spi_mr &= (~QSPI_MR_WDRBT);
	/* Set Chip Select Mode */
	/* Default: not reloaded */

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		spi_csr |= QSPI_SCR_CPOL;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		spi_csr |= QSPI_SCR_CPHA;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	} else {
		mask = regs->QSPI_MR & (~QSPI_MR_NBBITS_Msk);
		regs->QSPI_MR = mask | 8;
	}

	/* Use the requested or next highest possible frequency */
	div = SOC_ATMEL_SAM_MCK_FREQ_HZ / config->frequency;
	div = CLAMP(div, 1, UINT8_MAX);
	spi_csr |= QSPI_SCR_SCBR(div);

	regs->QSPI_CR = QSPI_CR_QSPIDIS; /* Disable SPI */
	regs->QSPI_MR = spi_mr;
	regs->QSPI_SCR = spi_csr;
	/* Set instruction length to quad */
	regs->QSPI_IFR = QSPI_IFR_WIDTH_QUAD_CMD;
	mask = regs->QSPI_IFR;
	mask |= QSPI_IFR_WIDTH(cfg->width);
	LOG_DBG("width: %d, ", QSPI_IFR_WIDTH(cfg->width));
	LOG_DBG("reg: %d\n", QSPI_IFR_WIDTH_QUAD_CMD);
	regs->QSPI_IFR = mask;

	regs->QSPI_CR = QSPI_CR_QSPIEN; /* Enable SPI */

	data->ctx.config = config;

	return 0;
}

/* Finish any ongoing writes and drop any remaining read data */
static void qspi_sam_finish(Qspi *regs)
{
	while ((regs->QSPI_SR & QSPI_SR_TXEMPTY) == 0) {
	}

	while (regs->QSPI_SR & QSPI_SR_RDRF) {
		(void)regs->QSPI_RDR;
	}
}

/* Fast path that transmits a buf */
static void qspi_sam_fast_tx(Qspi *regs, const uint8_t *tx_buf, const uint32_t tx_buf_len)
{
	const uint8_t *p = tx_buf;
	const uint8_t *pend = (uint8_t *)tx_buf + tx_buf_len;
	uint8_t ch;

	while (p != pend) {
		ch = *p++;

		while ((regs->QSPI_SR & QSPI_SR_TDRE) == 0) {
		}

		regs->QSPI_TDR = QSPI_TDR_TD(ch);
	}
}

/* Fast path that reads into a buf */
static void qspi_sam_fast_rx(Qspi *regs, uint8_t *rx_buf, const uint32_t rx_buf_len)
{
	uint8_t *rx = rx_buf;
	int len = rx_buf_len;

	if (len <= 0) {
		return;
	}

	/* Write the first byte */
	regs->QSPI_TDR = QSPI_TDR_TD(0);
	len--;

	while (len) {
		while ((regs->QSPI_SR & QSPI_SR_TDRE) == 0) {
		}

		/* Read byte N+0 from the receive register */
		while ((regs->QSPI_SR & QSPI_SR_RDRF) == 0) {
		}

		*rx = (uint8_t)regs->QSPI_RDR;
		rx++;

		/* Load byte N+1 into the transmit register */
		regs->QSPI_TDR = QSPI_TDR_TD(0);
		len--;
	}

	/* Read the final incoming byte */
	while ((regs->QSPI_SR & QSPI_SR_RDRF) == 0) {
	}

	*rx = (uint8_t)regs->QSPI_RDR;
}

/* Fast path that writes and reads bufs of the same length */
static void qspi_sam_fast_txrx(Qspi *regs,
			      const uint8_t *tx_buf,
			      const uint8_t *rx_buf,
			      const uint32_t len)
{
	const uint8_t *tx = tx_buf;
	const uint8_t *txend = tx_buf + len;
	uint8_t *rx = (uint8_t *)rx_buf;

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
	regs->QSPI_TDR = QSPI_TDR_TD(*tx++);

	while (tx != txend) {
		while ((regs->QSPI_SR & QSPI_SR_TDRE) == 0) {
		}

		/* Load byte N+1 into the transmit register.  TX is
		 * single buffered and we have at most one byte in
		 * flight so skip the DRE check.
		 */
		regs->QSPI_TDR = QSPI_TDR_TD(*tx++);

		/* Read byte N+0 from the receive register */
		while ((regs->QSPI_SR & QSPI_SR_RDRF) == 0) {
		}

		*rx++ = (uint8_t)regs->QSPI_RDR;
	}

	/* Read the final incoming byte */
	while ((regs->QSPI_SR & QSPI_SR_RDRF) == 0) {
	}

	*rx = (uint8_t)regs->QSPI_RDR;

}


static inline int qspi_sam_rx(const struct device *dev,
			      Qspi *regs,
			      uint8_t *rx_buf,
			      uint32_t rx_buf_len)
{
	k_spinlock_key_t key;

	key = spi_spin_lock(dev);
	qspi_sam_fast_rx(regs, rx_buf, rx_buf_len);
	qspi_sam_finish(regs);

	spi_spin_unlock(dev, key);
	return 0;
}

static inline int qspi_sam_tx(const struct device *dev,
			     Qspi *regs,
			     const uint8_t *tx_buf,
			     uint32_t tx_buf_len)
{
	k_spinlock_key_t key;

	key = spi_spin_lock(dev);
	qspi_sam_fast_tx(regs, tx_buf, tx_buf_len);
	qspi_sam_finish(regs);
	spi_spin_unlock(dev, key);
	return 0;
}


static inline int qspi_sam_txrx(const struct device *dev,
				Qspi *regs,
				const uint8_t *tx_buf,
				const uint8_t *rx_buf,
				uint32_t buf_len)
{
	k_spinlock_key_t key;

	key = spi_spin_lock(dev);
	qspi_sam_fast_txrx(regs, tx_buf, rx_buf, buf_len);
	qspi_sam_finish(regs);
	spi_spin_unlock(dev, key);
	return 0;
}

#ifndef CONFIG_SPI_RTIO

/* Fast path where every overlapping tx and rx buffer is the same length */
static void qspi_sam_fast_transceive(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct qspi_sam_config *cfg = dev->config;
	size_t tx_count = 0;
	size_t rx_count = 0;
	Qspi *regs = cfg->regs;
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
			qspi_sam_rx(dev, regs, rx->buf, rx->len);
		} else if (rx->buf == NULL) {
			qspi_sam_tx(dev, regs, tx->buf, tx->len);
		} else if (rx->len == tx->len) {
			qspi_sam_txrx(dev, regs, tx->buf, rx->buf, rx->len);
		} else {
			__ASSERT_NO_MSG("Invalid fast transceive configuration");
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	for (; tx_count != 0; tx_count--) {
		qspi_sam_tx(dev, regs, tx->buf, tx->len);
		tx++;
	}

	for (; rx_count != 0; rx_count--) {
		qspi_sam_rx(dev, regs, rx->buf, rx->len);
		rx++;
	}
}

static bool qspi_sam_transfer_ongoing(struct qspi_sam_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void qspi_sam_shift_master(Qspi *regs, struct qspi_sam_data *data)
{
	uint8_t tx;
	uint8_t rx;

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx = *(uint8_t *)(data->ctx.tx_buf);
	} else {
		tx = 0U;
	}

	while ((regs->QSPI_SR & QSPI_SR_TDRE) == 0) {
	}

	regs->QSPI_TDR = QSPI_TDR_TD(tx);
	spi_context_update_tx(&data->ctx, 1, 1);

	while ((regs->QSPI_SR & QSPI_SR_RDRF) == 0) {
	}

	rx = (uint8_t)regs->QSPI_RDR;

	if (spi_context_rx_buf_on(&data->ctx)) {
		*data->ctx.rx_buf = rx;
	}
	spi_context_update_rx(&data->ctx, 1, 1);
}

/* Returns true if the request is suitable for the fast
 * path. Specifically, the bufs are a sequence of:
 *
 * - Zero or more RX and TX buf pairs where each is the same length.
 * - Zero or more trailing RX only bufs
 * - Zero or more trailing TX only bufs
 */
static bool qspi_sam_is_regular(const struct spi_buf_set *tx_bufs,
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

#endif

static int qspi_sam_transceive(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	struct qspi_sam_data *data = dev->data;
	int err = 0;

	spi_context_cs_control(&data->ctx, true);
	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	const struct qspi_sam_config *cfg = dev->config;

	err = qspi_sam_configure(dev, config);
	if (err != 0) {
		goto done;
	}


	if (qspi_sam_is_regular(tx_bufs, rx_bufs)) {
		qspi_sam_fast_transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

		do {
			qspi_sam_shift_master(cfg->regs, data);
		} while (qspi_sam_transfer_ongoing(data));
	}

	spi_context_cs_control(&data->ctx, false);
done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int qspi_sam_transceive_sync(const struct device *dev,
				   const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
	return qspi_sam_transceive(dev, config, tx_bufs, rx_bufs);
}

static int qspi_sam_release(const struct device *dev,
			   const struct spi_config *config)
{
	struct qspi_sam_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int qspi_sam_init(const struct device *dev)
{
	int err;
	const struct qspi_sam_config *cfg = dev->config;
	struct qspi_sam_data *data = dev->data;

	/* Enable QSPI clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static DEVICE_API(spi, qspi_sam_driver_api) = {
	.transceive = qspi_sam_transceive_sync,
	.release = qspi_sam_release
};

PINCTRL_DT_INST_DEFINE(0);

static const struct qspi_sam_config qspi0_config = {
	.regs = (Qspi *)DT_INST_REG_ADDR(0),
	.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.width = DT_INST_PROP(0, lines)
};

static struct qspi_sam_data qspi0_data = {
		SPI_CONTEXT_INIT_LOCK(qspi0_data, ctx),
		SPI_CONTEXT_INIT_SYNC(qspi0_data, ctx),
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(0), ctx)
};

DEVICE_DT_INST_DEFINE(0, &qspi_sam_init, NULL,
		    &qspi0_data, &qspi0_config, POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY, &qspi_sam_driver_api);
