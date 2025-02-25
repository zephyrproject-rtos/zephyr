/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

struct lpspi_driver_data {
	size_t fill_len;
	uint8_t word_size_bytes;
};

static inline void lpspi_wait_tx_fifo_empty(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	while (LPSPI_GetTxFifoCount(base) != 0) {
	}
}

static inline uint8_t rx_fifo_cur_len(LPSPI_Type *base)
{
	return (base->FSR & LPSPI_FSR_RXCOUNT_MASK) >> LPSPI_FSR_RXCOUNT_SHIFT;
}

static inline uint8_t tx_fifo_cur_len(LPSPI_Type *base)
{
	return (base->FSR & LPSPI_FSR_TXCOUNT_MASK) >> LPSPI_FSR_TXCOUNT_SHIFT;
}


/* Reads a word from the RX fifo and handles writing it into the RX spi buf */
static inline void lpspi_rx_word_write_bytes(const struct device *dev, size_t offset)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t num_bytes = MIN(lpspi_data->word_size_bytes, ctx->rx_len);
	uint8_t *buf = ctx->rx_buf + offset;
	uint32_t word = LPSPI_ReadData(base);

	if (!spi_context_rx_buf_on(ctx) && spi_context_rx_on(ctx)) {
		/* receive no actual data if rx buf is NULL */
		return;
	}

	for (uint8_t i = 0; i < num_bytes; i++) {
		buf[i] = (uint8_t)(word >> (BITS_PER_BYTE * i));
	}
}

/* Reads a maximum number of words from RX fifo and writes them to the remainder of the RX buf */
static inline size_t lpspi_rx_buf_write_words(const struct device *dev, uint8_t max_read)
{
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	size_t buf_len = DIV_ROUND_UP(ctx->rx_len, lpspi_data->word_size_bytes);
	uint8_t words_read = 0;
	size_t offset = 0;

	while (buf_len-- > 0 && max_read-- > 0) {
		lpspi_rx_word_write_bytes(dev, offset);
		offset += lpspi_data->word_size_bytes;
		words_read++;
	}

	return words_read;
}

static inline void lpspi_handle_rx_irq(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t rx_fsr = rx_fifo_cur_len(base);
	uint8_t total_words_written = 0;
	uint8_t total_words_read = 0;
	uint8_t words_read;

	LPSPI_ClearStatusFlags(base, kLPSPI_RxDataReadyFlag);

	LOG_DBG("RX FIFO: %d, RX BUF: %p", rx_fsr, ctx->rx_buf);

	while ((rx_fsr = rx_fifo_cur_len(base)) > 0 && spi_context_rx_on(ctx)) {
		words_read = lpspi_rx_buf_write_words(dev, rx_fsr);
		total_words_read += words_read;
		total_words_written += (spi_context_rx_buf_on(ctx) ? words_read : 0);
		spi_context_update_rx(ctx, lpspi_data->word_size_bytes, words_read);
	}

	LOG_DBG("RX done %d words to spi buf", total_words_written);

	if (spi_context_rx_len_left(ctx) == 0) {
		LPSPI_DisableInterrupts(base, (uint32_t)kLPSPI_RxInterruptEnable);
		LPSPI_FlushFifo(base, false, true);
	}
}

static inline uint32_t lpspi_next_tx_word(const struct device *dev, int offset)
{
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	const uint8_t *byte = ctx->tx_buf + offset;
	uint32_t num_bytes = MIN(lpspi_data->word_size_bytes, ctx->tx_len);
	uint32_t next_word = 0;

	for (uint8_t i = 0; i < num_bytes; i++) {
		next_word |= *byte << (BITS_PER_BYTE * i);
	}

	return next_word;
}

static inline void lpspi_fill_tx_fifo(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	size_t bytes_in_xfer = lpspi_data->fill_len * lpspi_data->word_size_bytes;
	size_t offset;

	for (offset = 0; offset < bytes_in_xfer; offset += lpspi_data->word_size_bytes) {
		LPSPI_WriteData(base, lpspi_next_tx_word(dev, offset));
	}

	LOG_DBG("Filled TX FIFO to %d words (%d bytes)", lpspi_data->fill_len, offset);
}

static void lpspi_fill_tx_fifo_nop(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;

	for (int i = 0; i < lpspi_data->fill_len; i++) {
		LPSPI_WriteData(base, 0);
	}

	LOG_DBG("Filled TX fifo with %d NOPs", lpspi_data->fill_len);
}

static void lpspi_next_tx_fill(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	size_t max_chunk;

	/* Convert bytes to words for this xfer */
	max_chunk = DIV_ROUND_UP(ctx->tx_len, lpspi_data->word_size_bytes);
	max_chunk = MIN(max_chunk, config->tx_fifo_size);
	lpspi_data->fill_len = max_chunk;

	if (spi_context_tx_buf_on(ctx)) {
		lpspi_fill_tx_fifo(dev);
	} else {
		lpspi_fill_tx_fifo_nop(dev);
	}
}

static inline void lpspi_handle_tx_irq(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;

	spi_context_update_tx(ctx, lpspi_data->word_size_bytes, lpspi_data->fill_len);

	LPSPI_ClearStatusFlags(base, kLPSPI_TxDataRequestFlag);

	if (!spi_context_tx_on(ctx)) {
		LPSPI_DisableInterrupts(base, (uint32_t)kLPSPI_TxInterruptEnable);
		return;
	}

	lpspi_next_tx_fill(data->dev);
}

static void lpspi_isr(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct spi_mcux_config *config = dev->config;
	uint32_t status_flags = LPSPI_GetStatusFlags(base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;

	if (status_flags & kLPSPI_RxDataReadyFlag) {
		lpspi_handle_rx_irq(dev);
	}

	if (status_flags & kLPSPI_TxDataRequestFlag) {
		lpspi_handle_tx_irq(dev);
	}

	if (spi_context_tx_on(ctx)) {
		return;
	}

	if (spi_context_rx_len_left(ctx) == 1) {
		base->TCR &= ~LPSPI_TCR_CONT_MASK;
	} else if (spi_context_rx_on(ctx)) {
		size_t rx_fifo_len = rx_fifo_cur_len(base);
		size_t expected_rx_left = rx_fifo_len < ctx->rx_len ? ctx->rx_len - rx_fifo_len : 0;
		size_t max_fill = MIN(expected_rx_left, config->rx_fifo_size);
		size_t tx_current_fifo_len = tx_fifo_cur_len(base);

		lpspi_data->fill_len = tx_current_fifo_len < ctx->rx_len ?
					max_fill - tx_current_fifo_len : 0;

		lpspi_fill_tx_fifo_nop(dev);
	} else {
		spi_context_complete(ctx, dev, 0);
		NVIC_ClearPendingIRQ(config->irqn);
		base->TCR &= ~LPSPI_TCR_CONT_MASK;
		lpspi_wait_tx_fifo_empty(dev);
		spi_context_cs_control(ctx, false);
		spi_context_release(&data->ctx, 0);
	}
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	lpspi_data->word_size_bytes = SPI_WORD_SIZE_GET(spi_cfg->operation) / BITS_PER_BYTE;
	if (lpspi_data->word_size_bytes > 4) {
		LOG_ERR("Maximum 4 byte word size");
		ret = -EINVAL;
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, lpspi_data->word_size_bytes);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		return ret;
	}

	LPSPI_FlushFifo(base, true, true);
	LPSPI_ClearStatusFlags(base, (uint32_t)kLPSPI_AllStatusFlag);
	LPSPI_DisableInterrupts(base, (uint32_t)kLPSPI_AllInterruptEnable);

	LOG_DBG("Starting LPSPI transfer");
	spi_context_cs_control(&data->ctx, true);

	LPSPI_SetFifoWatermarks(base, 0, 0);
	LPSPI_Enable(base, true);

	/* keep the chip select asserted until the end of the zephyr xfer */
	base->TCR |= LPSPI_TCR_CONT_MASK;
	/* tcr is written to tx fifo */
	lpspi_wait_tx_fifo_empty(dev);

	/* start the transfer sequence which are handled by irqs */
	lpspi_next_tx_fill(dev);

	LPSPI_EnableInterrupts(base, (uint32_t)kLPSPI_TxInterruptEnable |
				     (uint32_t)kLPSPI_RxInterruptEnable);

	return spi_context_wait_for_completion(&data->ctx);
}

static int spi_mcux_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static DEVICE_API(spi, spi_mcux_driver_api) = {
	.transceive = spi_mcux_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mcux_release,
};

static int spi_mcux_init(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	err = spi_nxp_init_common(dev);
	if (err) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define LPSPI_INIT(n)                                                                              \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_MCUX_LPSPI_CONFIG_INIT(n)                                                              \
                                                                                                   \
	static struct lpspi_driver_data lpspi_##n##_driver_data;                                   \
                                                                                                   \
	static struct spi_mcux_data spi_mcux_data_##n = {                                          \
		SPI_NXP_LPSPI_COMMON_DATA_INIT(n)                                                  \
		.driver_data = &lpspi_##n##_driver_data,                                           \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_init, NULL, &spi_mcux_data_##n,                      \
				  &spi_mcux_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &spi_mcux_driver_api);

#define SPI_MCUX_LPSPI_INIT_IF_DMA(n) IF_DISABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_INIT(n)))

#define SPI_MCUX_LPSPI_INIT(n)                                                                     \
	COND_CODE_1(CONFIG_SPI_MCUX_LPSPI_DMA,				   \
						(SPI_MCUX_LPSPI_INIT_IF_DMA(n)), (LPSPI_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_LPSPI_INIT)
