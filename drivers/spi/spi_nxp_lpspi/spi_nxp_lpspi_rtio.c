/*
 * Copyright 2024-2025 NXP
 * Copyright 2025 Croxel, Inc.
 * Copyright 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(spi_lpspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"
#ifdef CONFIG_SPI_NXP_LPSPI_RTIO_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>

#if defined(CONFIG_DCACHE) && !defined(CONFIG_RTIO_SQE_PLACEMENT_DTCM) &&                          \
	!defined(CONFIG_RTIO_SQE_PLACEMENT_NOCACHE)
#define RTIO_SQE_CACHED 1
#endif

#define Z_SRAM_NODE DT_CHOSEN(zephyr_sram)

#if DT_NODE_EXISTS(Z_SRAM_NODE) && DT_NODE_HAS_COMPAT(Z_SRAM_NODE, nxp_imx_dtcm)
#define RTIO_SRAM_IS_DTCM 1
#else
#define RTIO_SRAM_IS_DTCM 0
#endif

#if !RTIO_SRAM_IS_DTCM && defined(CONFIG_DCACHE) &&                                                \
	!defined(CONFIG_RTIO_BLOCK_POOL_PLACEMENT_DTCM) &&                                         \
	!defined(CONFIG_RTIO_BLOCK_POOL_PLACEMENT_NOCACHE)
#error "SPI RTIO DMA cannot use cached RTIO block pool memory (D-cache enabled). "
"Select RTIO_BLOCK_POOL_PLACEMENT_DTCM or RTIO_BLOCK_POOL_PLACEMENT_NOCACHE, "
	"or disable DMA/D-cache."
#endif

/* dummy memory used for transferring NOP when tx buf is null */
static uint32_t tx_nop_val; /* check compliance says no init to 0, but should be 0 in bss */
/* dummy memory for transferring to when RX buf is null */
static uint32_t dummy_buffer;

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg[CONFIG_DMA_TCD_QUEUE_SIZE];
	size_t blk_idx;
};
#endif

struct lpspi_driver_data {
	struct spi_rtio *rtio_ctx;
	struct {
		struct rtio_sqe *sqe;
		size_t words_clocked;
	} tx_curr;
	struct {
		size_t words_to_clock;
		size_t words_clocked_tx;
		size_t words_to_clock_rx;
	} total;
	struct {
		struct rtio_sqe *sqe;
		size_t words_clocked;
	} rx_curr;
	uint32_t word_size_bytes;
#ifdef CONFIG_SPI_NXP_LPSPI_RTIO_DMA
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
#endif
};

static inline size_t get_sqe_words_len(struct rtio_sqe *sqe)
{
	switch (sqe->op) {
	case RTIO_OP_RX:
		return sqe->rx.buf_len;
	case RTIO_OP_TX:
		return sqe->tx.buf_len;
	case RTIO_OP_TINY_TX:
		return sqe->tiny_tx.buf_len;
	case RTIO_OP_TXRX:
		return sqe->txrx.buf_len;
	default:
		return 0;
	}
}

static inline struct rtio_sqe *get_next_sqe(struct rtio_sqe *sqe)
{
	struct rtio_iodev_sqe *curr_iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);
	struct rtio_iodev_sqe *next_iodev_sqe = rtio_txn_next(curr_iodev_sqe);

	return &next_iodev_sqe->sqe;
}

static inline void set_sqe_words_to_clock(struct rtio_sqe *head, struct lpspi_driver_data *data)
{
	struct rtio_iodev_sqe *curr_iodev_sqe = CONTAINER_OF(head, struct rtio_iodev_sqe, sqe);
	data->total.words_to_clock = 0;
	data->total.words_to_clock_rx = 0;
	data->total.words_clocked_tx = 0;

	while (curr_iodev_sqe != NULL) {
		switch (curr_iodev_sqe->sqe.op) {
		case RTIO_OP_RX:
			data->total.words_to_clock += curr_iodev_sqe->sqe.rx.buf_len;
			data->total.words_to_clock_rx += curr_iodev_sqe->sqe.rx.buf_len;
			break;
		case RTIO_OP_TX:
			data->total.words_to_clock += curr_iodev_sqe->sqe.tx.buf_len;
			break;
		case RTIO_OP_TINY_TX:
			data->total.words_to_clock += curr_iodev_sqe->sqe.tiny_tx.buf_len;
			break;
		case RTIO_OP_TXRX:
			data->total.words_to_clock += curr_iodev_sqe->sqe.txrx.buf_len;
			data->total.words_to_clock_rx += curr_iodev_sqe->sqe.txrx.buf_len;
			break;
		default:
			break;
		}
		curr_iodev_sqe = rtio_txn_next(curr_iodev_sqe);
	}

	data->total.words_to_clock =
		DIV_ROUND_UP(data->total.words_to_clock, data->word_size_bytes);
	data->total.words_to_clock_rx =
		DIV_ROUND_UP(data->total.words_to_clock_rx, data->word_size_bytes);
}

static inline const uint8_t *get_sqe_tx_buf(struct rtio_sqe *sqe)
{
	switch (sqe->op) {
	case RTIO_OP_TX:
		return sqe->tx.buf;
	case RTIO_OP_TINY_TX:
		return sqe->tiny_tx.buf;
	case RTIO_OP_TXRX:
		return sqe->txrx.tx_buf;
	default:
		return NULL;
	}
}

static inline uint8_t *get_sqe_rx_buf(struct rtio_sqe *sqe)
{
	switch (sqe->op) {
	case RTIO_OP_RX:
		return sqe->rx.buf;
	case RTIO_OP_TXRX:
		return sqe->txrx.rx_buf;
	default:
		return NULL;
	}
}

static void lpspi_rtio_iodev_complete(const struct device *dev, int status);

static inline void lpspi_rtio_fetch_rx_fifo(LPSPI_Type *base, uint8_t *buf, size_t offset,
					    size_t fetch_len)
{
	uint8_t *p = buf + offset;

	for (size_t i = 0; i < fetch_len; i++) {
		*p++ = (uint8_t)base->RDR;
	}
}

static inline void lpspi_rtio_empty_rx_fifo_nop(LPSPI_Type *base, size_t fill_len)
{
	for (size_t i = 0; i < fill_len; i++) {
		(void)base->RDR; /* read-and-discard */
	}
}

static inline bool lpspi_rtio_next_rx_fetch(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	uint32_t fetch_len = rx_fifo_cur_len(base);

	if (fetch_len == 0) {
		return true;
	}

	int bytes_left = fetch_len;

	while (bytes_left > 0 && lpspi_data->rx_curr.sqe) {
		struct rtio_sqe *sqe = lpspi_data->rx_curr.sqe;
		size_t rx_sqe_len = get_sqe_words_len(sqe);
		int curr_len = MIN(rx_sqe_len - lpspi_data->rx_curr.words_clocked, bytes_left);
		uint8_t *buf = get_sqe_rx_buf(sqe);

		if (buf != NULL) {
			lpspi_rtio_fetch_rx_fifo(base, buf, lpspi_data->rx_curr.words_clocked,
						 curr_len);
			lpspi_data->total.words_to_clock_rx -= curr_len;
		} else {
			lpspi_rtio_empty_rx_fifo_nop(base, curr_len);
		}
		bytes_left -= curr_len;
		lpspi_data->rx_curr.words_clocked += curr_len;

		if (lpspi_data->rx_curr.words_clocked >= rx_sqe_len) {
			lpspi_data->rx_curr.sqe = get_next_sqe(sqe);
			lpspi_data->rx_curr.words_clocked = 0;
		}
	}
	if (bytes_left > 0) {
		LOG_WRN("rx returned with bytes_left: %d - fetch_len: %d", bytes_left, fetch_len);
	}

	return lpspi_data->total.words_to_clock_rx != 0;
}

static inline void lpspi_rtio_fill_tx_fifo(LPSPI_Type *base, const uint8_t *buf, size_t offset,
					   size_t fill_len)
{
	const uint8_t *p = buf + offset;

	for (size_t i = 0; i < fill_len; i++) {
		base->TDR = (uint32_t)*p++;
	}
}

static inline void lpspi_rtio_fill_tx_fifo_nop(LPSPI_Type *base, size_t fill_len)
{
	for (size_t i = 0; i < fill_len; i++) {
		base->TDR = 0;
	}
}

/* handles refilling the TX fifo from empty */
static inline bool lpspi_rtio_next_tx_fill(const struct device *dev)
{
	const struct lpspi_config *config = dev->config;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	int fifo_remaining_len = config->tx_fifo_size - tx_fifo_cur_len(base);
	int fill_len = MIN(lpspi_data->total.words_to_clock - lpspi_data->total.words_clocked_tx,
			   fifo_remaining_len);

	if (fill_len <= 0) {
		return false;
	}

	int bytes_left = fill_len;

	while (bytes_left > 0 && lpspi_data->tx_curr.sqe) {
		struct rtio_sqe *sqe = lpspi_data->tx_curr.sqe;
		int curr_len =
			MIN(get_sqe_words_len(sqe) - lpspi_data->tx_curr.words_clocked, bytes_left);
		const uint8_t *buf = get_sqe_tx_buf(sqe);

		if (buf != NULL) {
			lpspi_rtio_fill_tx_fifo(base, buf, lpspi_data->tx_curr.words_clocked,
						curr_len);
		} else {
			lpspi_rtio_fill_tx_fifo_nop(base, curr_len);
		}
		bytes_left -= curr_len;
		lpspi_data->tx_curr.words_clocked += curr_len;

		if (lpspi_data->tx_curr.words_clocked >= get_sqe_words_len(sqe)) {
			lpspi_data->tx_curr.sqe = get_next_sqe(sqe);
			lpspi_data->tx_curr.words_clocked = 0;
		}
	}
	lpspi_data->total.words_clocked_tx += fill_len - bytes_left;

	if (bytes_left > 0) {
		LOG_WRN("tx returned with bytes_left: %d - fifo_remaining: %d Curr Words clocked "
			"%d %d/%d",
			bytes_left, fifo_remaining_len, lpspi_data->tx_curr.words_clocked,
			lpspi_data->total.words_clocked_tx, lpspi_data->total.words_to_clock);
	}

	if (lpspi_data->total.words_clocked_tx >= lpspi_data->total.words_to_clock) {
		base->TCR &= ~(LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK);
	}

	return true;
}

#ifdef CONFIG_SPI_NXP_LPSPI_RTIO_DMA
static inline struct dma_block_config *lpspi_rtio_dma_load_blk_cfg(struct spi_dma_stream *stream,
								   size_t len)
{
	if (stream->blk_idx >= CONFIG_DMA_TCD_QUEUE_SIZE - 1) {
		return NULL;
	}

	struct dma_block_config *blk_cfg = &stream->blk_cfg[stream->blk_idx++];

	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	blk_cfg->block_size = len;
	blk_cfg->source_gather_en = 1;
	blk_cfg->dest_scatter_en = 1;

	return blk_cfg;
}

static int lpspi_rtio_dma_tx_load(const struct device *dev, struct rtio_sqe *sqe)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;

	struct dma_block_config *blk_cfg =
		lpspi_rtio_dma_load_blk_cfg(&lpspi_data->dma_tx, get_sqe_words_len(sqe));

	if (!blk_cfg) {
		return -ENOMEM;
	}

	lpspi_data->dma_tx.dma_cfg.head_block = blk_cfg;

	while (1) {
		const uint8_t *buf = get_sqe_tx_buf(sqe);

		if (buf == NULL) {
			blk_cfg->source_address = (uint32_t)&tx_nop_val;
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		} else {
			blk_cfg->source_address = (uint32_t)buf;
#if RTIO_SQE_CACHED
			if (sqe->op == RTIO_OP_TINY_TX) {
				sys_cache_data_flush_range((void *)sqe, sizeof(struct rtio_sqe));
			}
#endif
		}

		blk_cfg->dest_address = (uint32_t)(&base->TDR);
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		sqe = get_next_sqe(sqe);

		if (sqe) {
			struct dma_block_config *new_blk_cfg = lpspi_rtio_dma_load_blk_cfg(
				&lpspi_data->dma_tx, get_sqe_words_len(sqe));
			if (!new_blk_cfg) {
				return -ENOMEM;
			}
			blk_cfg->next_block = new_blk_cfg;
			blk_cfg = new_blk_cfg;
		} else {
			break;
		}
	};

	return dma_config(lpspi_data->dma_tx.dma_dev, lpspi_data->dma_tx.channel,
			  &lpspi_data->dma_tx.dma_cfg);
}

static int lpspi_rtio_dma_rx_load(const struct device *dev, struct rtio_sqe *sqe)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;

	struct dma_block_config *blk_cfg =
		lpspi_rtio_dma_load_blk_cfg(&lpspi_data->dma_rx, get_sqe_words_len(sqe));

	if (!blk_cfg) {
		return -ENOMEM;
	}

	lpspi_data->dma_rx.dma_cfg.head_block = blk_cfg;

	while (1) {
		uint8_t *buf = get_sqe_rx_buf(sqe);

		if (buf == NULL) {
			blk_cfg->dest_address = (uint32_t)&dummy_buffer;
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		} else {
			blk_cfg->dest_address = (uint32_t)buf;
		}

		blk_cfg->source_address = (uint32_t)(&base->RDR);
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		sqe = get_next_sqe(sqe);

		if (sqe) {
			struct dma_block_config *new_blk_cfg = lpspi_rtio_dma_load_blk_cfg(
				&lpspi_data->dma_rx, get_sqe_words_len(sqe));
			if (!new_blk_cfg) {
				return -ENOMEM;
			}
			blk_cfg->next_block = new_blk_cfg;
			blk_cfg = new_blk_cfg;
		} else {
			break;
		}
	};

	return dma_config(lpspi_data->dma_rx.dma_dev, lpspi_data->dma_rx.channel,
			  &lpspi_data->dma_rx.dma_cfg);
}

/* Return values:
 * positive value if a data chunk is loaded successfully and return the data chunk size loaded;
 * negative value if error happens and return the error code;
 * 0 if no data is loaded;
 */
static int lpspi_dma_start(const struct device *dev)
{
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_dma_stream *rx = &lpspi_data->dma_rx;
	struct spi_dma_stream *tx = &lpspi_data->dma_tx;
	size_t dma_size = lpspi_data->total.words_to_clock;
	int ret = 0;

	if (dma_size == 0) {
		/* In case both buffers are 0 length, we should not even be here
		 * and attempting to set up a DMA transfer like this will cause
		 * errors that lock up the system in some cases with eDMA.
		 */
		return 0;
	}

	ret = lpspi_rtio_dma_tx_load(dev, lpspi_data->tx_curr.sqe);
	if (ret != 0) {
		/* TX RTIO SQE chain requires more DMA TCDs than DMA_TCD_QUEUE_SIZE allows.
		 * Increase DMA_TCD_QUEUE_SIZE to prepare the TX DMA transaction.
		 */
		goto err_reset;
	}

	ret = lpspi_rtio_dma_rx_load(dev, lpspi_data->rx_curr.sqe);
	if (ret != 0) {
		/* RX RTIO SQE chain requires more DMA TCDs than DMA_TCD_QUEUE_SIZE allows.
		 * Increase DMA_TCD_QUEUE_SIZE to prepare the RX DMA transaction.
		 */
		goto err_reset;
	}

	ret = dma_start(rx->dma_dev, rx->channel);
	if (ret != 0) {
		goto err_reset;
	}

	ret = dma_start(tx->dma_dev, tx->channel);
	if (ret != 0) {
		dma_stop(rx->dma_dev, rx->channel);
		goto err_reset;
	}

	return dma_size;

err_reset:
	lpspi_data->dma_tx.blk_idx = 0;
	lpspi_data->dma_rx.blk_idx = 0;
	return ret;
}

static void lpspi_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	/* arg directly holds the spi device */
	const struct device *spi_dev = arg;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(spi_dev, reg_base);
	struct lpspi_data *data = (struct lpspi_data *)spi_dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
		lpspi_rtio_iodev_complete(spi_dev, -EIO);
		spi_context_cs_control(&data->ctx, false);
		return;
	}

	/*
	 * When TX DMA completes, it does NOT mean the SPI transfer is finished.
	 * It only indicates that all TX data has been written into the LPSPI FIFO.
	 * The hardware still needs time to shift out the data and generate clocks,
	 * after which the corresponding RX data will be received.
	 * Only then is the full transfer truly complete.
	 *
	 * At this point, we clear the Continuous Transfer bits to allow the PCS
	 * (chip select) line to be deasserted once the transfer fully completes.
	 */
	if (channel == lpspi_data->dma_tx.channel) {
		base->TCR &= ~(LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK);
	}

	/* RX DMA completion marks the end of the full SPI transfer;
	 * all data has been transmitted and corresponding RX data received.
	 */
	if (channel == lpspi_data->dma_rx.channel) {
		lpspi_rtio_iodev_complete(spi_dev, 0);
		lpspi_data->dma_rx.blk_idx = 0;
		lpspi_data->dma_tx.blk_idx = 0;
	}
}
#endif

static void lpspi_isr(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	uint32_t status_flags = base->SR;
	uint32_t irq_enable = base->IER;

	irq_enable &= ~(LPSPI_IER_RDIE_MASK | LPSPI_IER_TDIE_MASK);

	if (lpspi_data->total.words_to_clock_rx > 0) {
		if (lpspi_rtio_next_rx_fetch(dev)) {
			irq_enable |= LPSPI_IER_RDIE_MASK;
			if (lpspi_data->total.words_to_clock_rx < config->rx_fifo_size) {
				base->FCR =
					LPSPI_FCR_TXWATER(0) |
					LPSPI_FCR_RXWATER(lpspi_data->total.words_to_clock_rx - 1);
			}
		}
	}

	if (lpspi_data->total.words_to_clock != lpspi_data->total.words_clocked_tx) {
		if (lpspi_rtio_next_tx_fill(dev)) {
			irq_enable |= LPSPI_IER_TDIE_MASK;
		}
	}

	if (status_flags & LPSPI_SR_TCF_MASK) {
		irq_enable &= ~LPSPI_IER_TCIE_MASK;
	}

	if (status_flags & LPSPI_SR_REF_MASK) {
		base->IER = 0;
		lpspi_rtio_iodev_complete(dev, -EIO);
	} else {
		base->IER = irq_enable;

		if (irq_enable == 0) {
			/** Due to stalling behavior on older LPSPI, if we know we already wrote
			 * all the words into the fifo, then we need to end xfer manually by
			 * writing TCR in order to get last bit clocked out on bus. So all we need
			 * to do is touch the TCR by writing to fifo through TCR register and wait
			 * for final RX interrupt.
			 */
			base->TCR = base->TCR;

			/** We're done both TX and RX as they each clear their Interrupt
			 * enable bit once fully received. The transfer has completed.
			 */
			lpspi_rtio_iodev_complete(dev, 0);
		}
	}
}

static void lpspi_rtio_iodev_start(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_rtio *rtio_ctx = lpspi_data->rtio_ctx;
	struct rtio_sqe *sqe = &rtio_ctx->txn_head->sqe;
	struct spi_dt_spec *spi_dt_spec = sqe->iodev->data;
	struct spi_config *spi_cfg = &spi_dt_spec->config;
	int ret = 0;

	lpspi_data->word_size_bytes =
		DIV_ROUND_UP(SPI_WORD_SIZE_GET(spi_cfg->operation), BITS_PER_BYTE);
	if (lpspi_data->word_size_bytes != 1) {
		LOG_ERR("Driver only works with word size = 1 byte");
		ret = -EINVAL;
		goto lpspi_rtio_iodev_start_on_error;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		LOG_WRN("Target mode not supported for LPSPI RTIO");
		ret = -ENOTSUP;
		goto lpspi_rtio_iodev_start_on_error;
	}

	if (spi_cfg->operation & SPI_HOLD_ON_CS && !spi_cs_is_gpio(spi_cfg)) {
		ret = -ENOTSUP;
		goto lpspi_rtio_iodev_start_on_error;
	}

	ret = lpspi_configure(dev, spi_cfg);
	if (ret) {
		goto lpspi_rtio_iodev_start_on_error;
	}

	base->CR |= LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK;
	base->SR = LPSPI_INTERRUPT_BITS;

	set_sqe_words_to_clock(sqe, lpspi_data);

	if (lpspi_data->total.words_to_clock == 0) {
		ret = -EINVAL;
		goto lpspi_rtio_iodev_start_on_error;
	}

	lpspi_data->tx_curr.sqe = sqe;
	lpspi_data->tx_curr.words_clocked = 0;

	lpspi_data->rx_curr.sqe = sqe;
	lpspi_data->rx_curr.words_clocked = 0;

	base->TCR = (base->TCR & ~(LPSPI_TCR_PCS_MASK | LPSPI_TCR_RXMSK_MASK)) |
		    LPSPI_TCR_PCS(spi_cfg->slave) | LPSPI_TCR_CONT_MASK;
	spi_context_cs_control(&data->ctx, true);

	/* tcr is written to tx fifo */
	lpspi_wait_tx_fifo_empty(dev);

#ifdef CONFIG_SPI_NXP_LPSPI_RTIO_DMA
#ifdef CONFIG_SPI_NXP_LPSPI_RTIO_DMA_THRESHOLD
	if (lpspi_data->total.words_to_clock > CONFIG_SPI_NXP_LPSPI_RTIO_DMA_THRESHOLD) {
#else
	if (lpspi_data->total.words_to_clock > config->rx_fifo_size) {
#endif
		base->FCR = LPSPI_FCR_TXWATER(0) | LPSPI_FCR_RXWATER(0);
		base->IER = 0;
		base->CR = LPSPI_CR_MEN_MASK;

		/* Load dma block */
		ret = lpspi_dma_start(dev);
		if (ret <= 0) {
			LOG_ERR("DMA start failed: ret=%d.", ret);
			goto lpspi_rtio_iodev_start_on_error;
		}
		/* Enable DMA Requests */
		base->DER |= LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK;
		return;
	}
#endif

	if (lpspi_data->total.words_to_clock < config->rx_fifo_size) {
		base->FCR = LPSPI_FCR_TXWATER(0) |
			    LPSPI_FCR_RXWATER(lpspi_data->total.words_to_clock - 1);
	} else {
		base->FCR = LPSPI_FCR_TXWATER(0) | LPSPI_FCR_RXWATER(config->rx_fifo_size / 2);
	}

	base->CR = LPSPI_CR_MEN_MASK;

	/* start the transfer sequence which are handled by irqs */
	if (lpspi_rtio_next_tx_fill(dev) == false) {
		ret = -EINVAL;
		LOG_ERR("LPSPI FILL ERROR");
		goto lpspi_rtio_iodev_start_on_error;
	}

	if (lpspi_data->total.words_to_clock_rx > 0) {
		base->IER = LPSPI_IER_RDIE_MASK | LPSPI_IER_TCIE_MASK;
	} else {
		base->TCR |= LPSPI_TCR_RXMSK_MASK;
		base->IER = LPSPI_IER_TDIE_MASK | LPSPI_IER_TCIE_MASK;
	}
	return;

lpspi_rtio_iodev_start_on_error:
	lpspi_rtio_iodev_complete(dev, ret);
}

static void lpspi_rtio_iodev_complete(const struct device *dev, int status)
{
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_rtio *rtio_ctx = lpspi_data->rtio_ctx;
	struct spi_context *ctx = &data->ctx;

	NVIC_ClearPendingIRQ(config->irqn);

	if (!(ctx->config->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(&data->ctx, false);
	}

	if (spi_rtio_complete(rtio_ctx, status)) {
		lpspi_rtio_iodev_start(dev);
	}
}

static void lpspi_rtio_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct lpspi_data *data = (struct lpspi_data *)dev->data;
	struct lpspi_driver_data *drv_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_rtio *rtio_ctx = drv_data->rtio_ctx;

	if (spi_rtio_submit(rtio_ctx, iodev_sqe)) {
		lpspi_rtio_iodev_start(dev);
	}
}

static int transceive_rtio(const struct device *dev, const struct spi_config *spi_cfg,
			   const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct lpspi_data *data = (struct lpspi_data *)dev->data;
	struct lpspi_driver_data *drv_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_rtio *rtio_ctx = drv_data->rtio_ctx;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);
	ret = spi_rtio_transceive(rtio_ctx, spi_cfg, tx_bufs, rx_bufs);
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int lpspi_rtio_init(const struct device *dev)
{
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *drv_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_rtio *rtio_ctx = drv_data->rtio_ctx;
	int err = 0;

	err = spi_nxp_init_common(dev);
	if (err) {
		return err;
	}

	spi_rtio_init(rtio_ctx, dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int transceive_rtio_async(const struct device *dev, const struct spi_config *spi_cfg,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				 void *userdata)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(spi_cfg);
	ARG_UNUSED(tx_bufs);
	ARG_UNUSED(rx_bufs);
	ARG_UNUSED(cb);
	ARG_UNUSED(userdata);

	return -ENOTSUP;
}
#endif

static DEVICE_API(spi, lpspi_driver_api) = {
	.transceive = transceive_rtio,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = transceive_rtio_async,
#endif
	.iodev_submit = lpspi_rtio_submit,
	.release = spi_lpspi_release,
};

#define LPSPI_DMA_COMMON_CFG(n)                                                                    \
	.dma_callback = lpspi_dma_callback, .source_data_size = 1, .dest_data_size = 1,            \
	.block_count = 1

#if defined(CONFIG_SPI_NXP_LPSPI_RTIO_DMA)
#define SPI_DMA_CHANNELS(n)                                                                        \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(n, tx),				                   \
		(.dma_tx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),		   \
				.channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, mux),		   \
				.blk_idx = 0,							   \
				.dma_cfg = {.channel_direction = MEMORY_TO_PERIPHERAL,		   \
					.source_burst_length = 1,                                  \
					.user_data = (void *)DEVICE_DT_INST_GET(n),                \
				LPSPI_DMA_COMMON_CFG(n),					   \
				.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, tx, source)}},))          \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(n, rx),						   \
		(.dma_rx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),		   \
				.channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, mux),		   \
				.blk_idx = 0,							   \
				.dma_cfg = {.channel_direction = PERIPHERAL_TO_MEMORY,		   \
					.source_burst_length = 1,                                  \
					.user_data = (void *)DEVICE_DT_INST_GET(n),                \
				LPSPI_DMA_COMMON_CFG(n),					   \
				.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, rx, source)}},))
#else
#define SPI_DMA_CHANNELS(n) /* no DMA fields */
#endif

#define LPSPI_RTIO_INIT(n)                                                                         \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_LPSPI_CONFIG_INIT(n)                                                                   \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(n, tx_fifo_size) == DT_INST_PROP(n, rx_fifo_size),               \
		     "tx-fifo-size and rx-fifo-size must match for the RTIO SPI driver "           \
		     "to work. Please make them equal.");                                          \
                                                                                                   \
	SPI_RTIO_DEFINE(spi_nxp_rtio_##n, CONFIG_SPI_NXP_RTIO_SQ_SIZE,                             \
			CONFIG_SPI_NXP_RTIO_SQ_SIZE);                                              \
                                                                                                   \
	static struct lpspi_driver_data lpspi_##n##_driver_data = {.rtio_ctx = &spi_nxp_rtio_##n,  \
								   SPI_DMA_CHANNELS(n)};           \
                                                                                                   \
	static struct lpspi_data lpspi_data_##n = {                                                \
		SPI_NXP_LPSPI_COMMON_DATA_INIT(n).driver_data = &lpspi_##n##_driver_data,          \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, lpspi_rtio_init, NULL, &lpspi_data_##n, &lpspi_config_##n,    \
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &lpspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LPSPI_RTIO_INIT)
