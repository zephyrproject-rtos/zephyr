/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 qianfan Zhao.
 * Copyright (c) 2023 Gerson Fernando Budke.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sam);

#include "spi_context.h"
#include <errno.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#define SAM_SPI_CHIP_SELECT_COUNT			4

/* Number of bytes in transfer before using DMA if available */
#define SAM_SPI_DMA_THRESHOLD                           32

/* Device constant configuration parameters */
struct spi_sam_config {
	Spi *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	bool loopback;

#ifdef CONFIG_SPI_SAM_DMA
	const struct device *dma_dev;
	const uint32_t dma_tx_channel;
	const uint32_t dma_tx_perid;
	const uint32_t dma_rx_channel;
	const uint32_t dma_rx_perid;
#endif /* CONFIG_SPI_SAM_DMA */
};

/* Device run time data */
struct spi_sam_data {
	struct spi_context ctx;
	struct k_spinlock lock;

#ifdef CONFIG_SPI_RTIO
	struct rtio *r; /* context for thread calls */
	struct rtio_iodev iodev;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct spi_dt_spec dt_spec;
#endif

#ifdef CONFIG_SPI_SAM_DMA
	struct k_sem dma_sem;
#endif /* CONFIG_SPI_SAM_DMA */
};

static inline k_spinlock_key_t spi_spin_lock(const struct device *dev)
{
	struct spi_sam_data *data = dev->data;

	return k_spin_lock(&data->lock);
}

static inline void spi_spin_unlock(const struct device *dev, k_spinlock_key_t key)
{
	struct spi_sam_data *data = dev->data;

	k_spin_unlock(&data->lock, key);
}

static int spi_slave_to_mr_pcs(int slave)
{
	int pcs[SAM_SPI_CHIP_SELECT_COUNT] = {0x0, 0x1, 0x3, 0x7};

	/* SPI worked in fixed peripheral mode(SPI_MR.PS = 0) and disabled chip
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
	uint16_t spi_csr_idx = spi_cs_is_gpio(config) ? 0 : config->slave;
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

	if (config->slave > (SAM_SPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("Slave %d is greater than %d",
			config->slave, SAM_SPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	/* Set master mode, disable mode fault detection, set fixed peripheral
	 * select mode.
	 */
	spi_mr |= (SPI_MR_MSTR | SPI_MR_MODFDIS);
	spi_mr |= SPI_MR_PCS(spi_slave_to_mr_pcs(spi_csr_idx));

	if (cfg->loopback) {
		spi_mr |= SPI_MR_LLB;
	}

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
	div = CLAMP(div, 1, UINT8_MAX);
	spi_csr |= SPI_CSR_SCBR(div);

	regs->SPI_CR = SPI_CR_SPIDIS; /* Disable SPI */
	regs->SPI_MR = spi_mr;
	regs->SPI_CSR[spi_csr_idx] = spi_csr;
	regs->SPI_CR = SPI_CR_SPIEN; /* Enable SPI */

	data->ctx.config = config;

	return 0;
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
static void spi_sam_fast_tx(Spi *regs, const uint8_t *tx_buf, const uint32_t tx_buf_len)
{
	const uint8_t *p = tx_buf;
	const uint8_t *pend = (uint8_t *)tx_buf + tx_buf_len;
	uint8_t ch;

	while (p != pend) {
		ch = *p++;

		while ((regs->SPI_SR & SPI_SR_TDRE) == 0) {
		}

		regs->SPI_TDR = SPI_TDR_TD(ch);
	}
}

/* Fast path that reads into a buf */
static void spi_sam_fast_rx(Spi *regs, uint8_t *rx_buf, const uint32_t rx_buf_len)
{
	uint8_t *rx = rx_buf;
	int len = rx_buf_len;

	if (len <= 0) {
		return;
	}

	/* Write the first byte */
	regs->SPI_TDR = SPI_TDR_TD(0);
	len--;

	while (len) {
		while ((regs->SPI_SR & SPI_SR_TDRE) == 0) {
		}

		/* Read byte N+0 from the receive register */
		while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
		}

		*rx = (uint8_t)regs->SPI_RDR;
		rx++;

		/* Load byte N+1 into the transmit register */
		regs->SPI_TDR = SPI_TDR_TD(0);
		len--;
	}

	/* Read the final incoming byte */
	while ((regs->SPI_SR & SPI_SR_RDRF) == 0) {
	}

	*rx = (uint8_t)regs->SPI_RDR;
}

/* Fast path that writes and reads bufs of the same length */
static void spi_sam_fast_txrx(Spi *regs,
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

}


#ifdef CONFIG_SPI_SAM_DMA

static __aligned(4) uint32_t tx_dummy;
static __aligned(4) uint32_t rx_dummy;

#ifdef CONFIG_SPI_RTIO
static void spi_sam_iodev_complete(const struct device *dev, int status);
#endif

static void dma_callback(const struct device *dma_dev, void *user_data,
	uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	const struct device *dev = user_data;
	struct spi_sam_data *drv_data = dev->data;

#ifdef CONFIG_SPI_RTIO
	if (drv_data->iodev_sqe != NULL) {
		spi_sam_iodev_complete(dev, status);
		return;
	}
#endif
	k_sem_give(&drv_data->dma_sem);
}


/* DMA transceive path */
static int spi_sam_dma_txrx(const struct device *dev,
			    Spi *regs,
			    const uint8_t *tx_buf,
			    const uint8_t *rx_buf,
			    const uint32_t len)
{
	const struct spi_sam_config *drv_cfg = dev->config;
	struct spi_sam_data *drv_data = dev->data;
#ifdef CONFIG_SPI_RTIO
	bool blocking = drv_data->iodev_sqe == NULL;
#else
	bool blocking = true;
#endif

	int res = 0;

	__ASSERT_NO_MSG(rx_buf != NULL || tx_buf != NULL);

	struct dma_config rx_dma_cfg = {
		.source_data_size = 1,
		.dest_data_size = 1,
		.block_count = 1,
		.dma_slot = drv_cfg->dma_rx_perid,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.complete_callback_en = true,
		.error_callback_en = true,
		.dma_callback = NULL,
		.user_data = (void *)dev,
	};

	uint32_t dest_address, dest_addr_adjust;

	if (rx_buf != NULL) {
		dest_address = (uint32_t)rx_buf;
		dest_addr_adjust = DMA_ADDR_ADJ_INCREMENT;
	} else {
		dest_address = (uint32_t)&rx_dummy;
		dest_addr_adjust = DMA_ADDR_ADJ_NO_CHANGE;
	}

	struct dma_block_config rx_block_cfg = {
		.dest_addr_adj = dest_addr_adjust,
		.block_size = len,
		.source_address = (uint32_t)&regs->SPI_RDR,
		.dest_address = dest_address
	};

	rx_dma_cfg.head_block = &rx_block_cfg;

	struct dma_config tx_dma_cfg = {
		.source_data_size = 1,
		.dest_data_size = 1,
		.block_count = 1,
		.dma_slot = drv_cfg->dma_tx_perid,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.complete_callback_en = true,
		.error_callback_en = true,
		.dma_callback = dma_callback,
		.user_data = (void *)dev,
	};

	uint32_t source_address, source_addr_adjust;

	if (tx_buf != NULL) {
		source_address = (uint32_t)tx_buf;
		source_addr_adjust = DMA_ADDR_ADJ_INCREMENT;
	} else {
		source_address = (uint32_t)&tx_dummy;
		source_addr_adjust = DMA_ADDR_ADJ_NO_CHANGE;
	}

	struct dma_block_config tx_block_cfg = {
		.source_addr_adj = source_addr_adjust,
		.block_size = len,
		.source_address = source_address,
		.dest_address = (uint32_t)&regs->SPI_TDR
	};

	tx_dma_cfg.head_block = &tx_block_cfg;

	res = dma_config(drv_cfg->dma_dev, drv_cfg->dma_rx_channel, &rx_dma_cfg);
	if (res != 0) {
		LOG_ERR("failed to configure SPI DMA RX");
		goto out;
	}

	res = dma_config(drv_cfg->dma_dev, drv_cfg->dma_tx_channel, &tx_dma_cfg);
	if (res != 0) {
		LOG_ERR("failed to configure SPI DMA TX");
		goto out;
	}

	/* Clocking begins on tx, so start rx first */
	res = dma_start(drv_cfg->dma_dev, drv_cfg->dma_rx_channel);
	if (res != 0) {
		LOG_ERR("failed to start SPI DMA RX");
		goto out;
	}

	res = dma_start(drv_cfg->dma_dev, drv_cfg->dma_tx_channel);
	if (res != 0) {
		LOG_ERR("failed to start SPI DMA TX");
		dma_stop(drv_cfg->dma_dev, drv_cfg->dma_rx_channel);
	}

	/* Move up a level or wrap in branch when blocking */
	if (blocking) {
		k_sem_take(&drv_data->dma_sem, K_FOREVER);
		spi_sam_finish(regs);
	} else {
		res = -EWOULDBLOCK;
	}

out:
	return res;
}

#endif /* CONFIG_SPI_SAM_DMA */


static inline int spi_sam_rx(const struct device *dev,
			      Spi *regs,
			      uint8_t *rx_buf,
			      uint32_t rx_buf_len)
{
	k_spinlock_key_t key;

#ifdef CONFIG_SPI_SAM_DMA
	const struct spi_sam_config *cfg = dev->config;

	if (rx_buf_len < SAM_SPI_DMA_THRESHOLD || cfg->dma_dev == NULL) {
		key = spi_spin_lock(dev);
		spi_sam_fast_rx(regs, rx_buf, rx_buf_len);
	} else {
		return spi_sam_dma_txrx(dev, regs, NULL, rx_buf, rx_buf_len);
	}
#else
	key = spi_spin_lock(dev);
	spi_sam_fast_rx(regs, rx_buf, rx_buf_len);
#endif
	spi_sam_finish(regs);

	spi_spin_unlock(dev, key);
	return 0;
}

static inline int spi_sam_tx(const struct device *dev,
			     Spi *regs,
			     const uint8_t *tx_buf,
			     uint32_t tx_buf_len)
{
	k_spinlock_key_t key;

#ifdef CONFIG_SPI_SAM_DMA
	const struct spi_sam_config *cfg = dev->config;

	if (tx_buf_len < SAM_SPI_DMA_THRESHOLD || cfg->dma_dev == NULL) {
		key = spi_spin_lock(dev);
		spi_sam_fast_tx(regs, tx_buf, tx_buf_len);
	} else {
		return spi_sam_dma_txrx(dev, regs, tx_buf, NULL, tx_buf_len);
	}
#else
	key = spi_spin_lock(dev);
	spi_sam_fast_tx(regs, tx_buf, tx_buf_len);
#endif
	spi_sam_finish(regs);
	spi_spin_unlock(dev, key);
	return 0;
}


static inline int spi_sam_txrx(const struct device *dev,
				Spi *regs,
				const uint8_t *tx_buf,
				const uint8_t *rx_buf,
				uint32_t buf_len)
{
	k_spinlock_key_t key;

#ifdef CONFIG_SPI_SAM_DMA
	const struct spi_sam_config *cfg = dev->config;

	if (buf_len < SAM_SPI_DMA_THRESHOLD || cfg->dma_dev == NULL) {
		key = spi_spin_lock(dev);
		spi_sam_fast_txrx(regs, tx_buf, rx_buf, buf_len);
	} else {
		return spi_sam_dma_txrx(dev, regs, tx_buf, rx_buf, buf_len);
	}
#else
	key = spi_spin_lock(dev);
	spi_sam_fast_txrx(regs, tx_buf, rx_buf, buf_len);
#endif
	spi_sam_finish(regs);
	spi_spin_unlock(dev, key);
	return 0;
}

#ifndef CONFIG_SPI_RTIO

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
			spi_sam_rx(dev, regs, rx->buf, rx->len);
		} else if (rx->buf == NULL) {
			spi_sam_tx(dev, regs, tx->buf, tx->len);
		} else if (rx->len == tx->len) {
			spi_sam_txrx(dev, regs, tx->buf, rx->buf, rx->len);
		} else {
			__ASSERT_NO_MSG("Invalid fast transceive configuration");
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	for (; tx_count != 0; tx_count--) {
		spi_sam_tx(dev, regs, tx->buf, tx->len);
		tx++;
	}

	for (; rx_count != 0; rx_count--) {
		spi_sam_rx(dev, regs, rx->buf, rx->len);
		rx++;
	}
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

#else

static void spi_sam_iodev_complete(const struct device *dev, int status);
static void spi_sam_iodev_next(const struct device *dev, bool completion);

static void spi_sam_iodev_start(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	struct rtio_sqe *sqe = &data->txn_curr->sqe;
	int ret = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
		ret = spi_sam_rx(dev, cfg->regs, sqe->buf, sqe->buf_len);
		break;
	case RTIO_OP_TX:
		ret = spi_sam_tx(dev, cfg->regs, sqe->buf, sqe->buf_len);
		break;
	case RTIO_OP_TINY_TX:
		ret = spi_sam_tx(dev, cfg->regs, sqe->tiny_buf, sqe->tiny_buf_len);
		break;
	case RTIO_OP_TXRX:
		ret = spi_sam_txrx(dev, cfg->regs, sqe->tx_buf, sqe->rx_buf, sqe->txrx_buf_len);
		break;
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		struct rtio_iodev_sqe *txn_head = data->txn_head;

		spi_sam_iodev_next(dev, true);
		rtio_iodev_sqe_err(txn_head, -EINVAL);
		ret = 0;
	}
	if (ret == 0) {
		spi_sam_iodev_complete(dev, 0);
	}
}

static void spi_sam_iodev_next(const struct device *dev, bool completion)
{
	struct spi_sam_data *data = dev->data;

	k_spinlock_key_t key  = spi_spin_lock(dev);

	if (!completion && data->txn_curr != NULL) {
		spi_spin_unlock(dev, key);
		return;
	}

	struct rtio_mpsc_node *next = rtio_mpsc_pop(&data->iodev.iodev_sq);

	if (next != NULL) {
		struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

		data->txn_head = next_sqe;
		data->txn_curr = next_sqe;
	} else {
		data->txn_head = NULL;
		data->txn_curr = NULL;
	}

	spi_spin_unlock(dev, key);

	if (data->txn_curr != NULL) {
		struct spi_dt_spec *spi_dt_spec = data->txn_curr->sqe.iodev->data;
		struct spi_config *spi_cfg = &spi_dt_spec->config;

		spi_sam_configure(dev, spi_cfg);
		spi_context_cs_control(&data->ctx, true);
		spi_sam_iodev_start(dev);
	}
}

static void spi_sam_iodev_complete(const struct device *dev, int status)
{
	struct spi_sam_data *data = dev->data;

	if (data->txn_curr->sqe.flags & RTIO_SQE_TRANSACTION) {
		data->txn_curr = rtio_txn_next(data->txn_curr);
		spi_sam_iodev_start(dev);
	} else {
		struct rtio_iodev_sqe *txn_head = data->txn_head;

		spi_context_cs_control(&data->ctx, false);
		spi_sam_iodev_next(dev, true);
		rtio_iodev_sqe_ok(txn_head, status);
	}
}

static void spi_sam_iodev_submit(const struct device *dev,
				 struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_sam_data *data = dev->data;

	rtio_mpsc_push(&data->iodev.iodev_sq, &iodev_sqe->q);
	spi_sam_iodev_next(dev, false);
}
#endif

static int spi_sam_transceive(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	struct spi_sam_data *data = dev->data;
	int err = 0;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

#if CONFIG_SPI_RTIO
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	struct spi_dt_spec *dt_spec = &data->dt_spec;

	dt_spec->config = *config;

	int ret = spi_rtio_copy(data->r, &data->iodev, tx_bufs, rx_bufs, &sqe);
	if (ret < 0) {
		err = ret;
		goto done;
	}

	/* Submit request and wait */
	rtio_submit(data->r, ret);

	while (ret > 0) {
		cqe = rtio_cqe_consume(data->r);

		if (cqe->result < 0) {
			err = cqe->result;
		}

		rtio_cqe_release(data->r, cqe);

		ret--;
	}
#else
	const struct spi_sam_config *cfg = dev->config;

	err = spi_sam_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_cs_control(&data->ctx, true);

	if (spi_sam_is_regular(tx_bufs, rx_bufs)) {
		spi_sam_fast_transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

		do {
			spi_sam_shift_master(cfg->regs, data);
		} while (spi_sam_transfer_ongoing(data));
	}

	spi_context_cs_control(&data->ctx, false);
#endif
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
				    spi_callback_t cb,
				    void *userdata)
{
	/* TODO: implement async transceive */
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
	int err;
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;

	/* Enable SPI clock in PMC */
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

#ifdef CONFIG_SPI_SAM_DMA
	k_sem_init(&data->dma_sem, 0, K_SEM_MAX_LIMIT);
#endif

#ifdef CONFIG_SPI_RTIO
	data->dt_spec.bus = dev;
	data->iodev.api = &spi_iodev_api;
	data->iodev.data = &data->dt_spec;
	rtio_mpsc_init(&data->iodev.iodev_sq);
#endif

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
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_sam_iodev_submit,
#endif
	.release = spi_sam_release,
};

#define SPI_DMA_INIT(n)										\
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),				\
	.dma_tx_channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),				\
	.dma_tx_perid = DT_INST_DMAS_CELL_BY_NAME(n, tx, perid),				\
	.dma_rx_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),				\
	.dma_rx_perid = DT_INST_DMAS_CELL_BY_NAME(n, rx, perid),

#ifdef CONFIG_SPI_SAM_DMA
#define SPI_SAM_USE_DMA(n) DT_INST_DMAS_HAS_NAME(n, tx)
#else
#define SPI_SAM_USE_DMA(n) 0
#endif

#define SPI_SAM_DEFINE_CONFIG(n)								\
	static const struct spi_sam_config spi_sam_config_##n = {				\
		.regs = (Spi *)DT_INST_REG_ADDR(n),						\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.loopback = DT_INST_PROP(n, loopback),						\
		COND_CODE_1(SPI_SAM_USE_DMA(n), (SPI_DMA_INIT(n)), ())				\
	}

#define SPI_SAM_RTIO_DEFINE(n) RTIO_DEFINE(spi_sam_rtio_##n, CONFIG_SPI_SAM_RTIO_SQ_SIZE,	\
					   CONFIG_SPI_SAM_RTIO_SQ_SIZE)

#define SPI_SAM_DEVICE_INIT(n)									\
	PINCTRL_DT_INST_DEFINE(n);								\
	SPI_SAM_DEFINE_CONFIG(n);								\
	COND_CODE_1(CONFIG_SPI_RTIO, (SPI_SAM_RTIO_DEFINE(n)), ());				\
	static struct spi_sam_data spi_sam_dev_data_##n = {					\
		SPI_CONTEXT_INIT_LOCK(spi_sam_dev_data_##n, ctx),				\
		SPI_CONTEXT_INIT_SYNC(spi_sam_dev_data_##n, ctx),				\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)				\
		IF_ENABLED(CONFIG_SPI_RTIO, (.r = &spi_sam_rtio_##n))				\
	};											\
	DEVICE_DT_INST_DEFINE(n, &spi_sam_init, NULL,						\
			    &spi_sam_dev_data_##n,						\
			    &spi_sam_config_##n, POST_KERNEL,					\
			    CONFIG_SPI_INIT_PRIORITY, &spi_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM_DEVICE_INIT)
