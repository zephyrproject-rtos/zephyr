/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

static uint8_t lpspi_rx_word_write_bytes(const struct device *dev, uint32_t word, size_t offset)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t num_bytes = MIN(data->word_size_bytes, ctx->rx_len);
	uint8_t *buf = ctx->rx_buf + offset;

	if (!spi_context_rx_buf_on(ctx) && spi_context_rx_on(ctx)) {
		/* receive no actual data if rx buf is NULL */
		return num_bytes;
	}

	for (uint8_t i = 0; i < num_bytes; i++) {
		buf[i] = (uint8_t)(word >> (BITS_PER_BYTE * i));
	}

	return num_bytes;
}

static size_t lpspi_rx_buf_write_words(const struct device *dev, uint8_t fifo_count)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t buf_len = ctx->rx_len / data->word_size_bytes;
	uint8_t words_read = 0;
	uint32_t next_word;
	size_t offset = 0;

	while (buf_len-- > 0 && fifo_count-- > 0) {
		next_word = LPSPI_ReadData(base);
		lpspi_rx_word_write_bytes(dev, next_word, offset);
		offset += data->word_size_bytes;
		words_read++;
	}

	return words_read;
}

static void lpspi_handle_rx_irq(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t rx_fsr = (base->FSR & LPSPI_FSR_RXCOUNT_MASK) >> LPSPI_FSR_RXCOUNT_SHIFT;
	uint8_t total_words_read = 0;
	uint8_t words_read;

	while (total_words_read < rx_fsr && spi_context_rx_on(ctx)) {
		words_read = lpspi_rx_buf_write_words(dev, rx_fsr);
		total_words_read += words_read;
		spi_context_update_rx(ctx, data->word_size_bytes, words_read);
	}

	if (!spi_context_rx_on(ctx)) {
		LPSPI_DisableInterrupts(base, (uint32_t)kLPSPI_RxInterruptEnable);
		LOG_DBG("LPSPI transfer over");
		spi_context_complete(ctx, dev, 0);
	}

	LPSPI_ClearStatusFlags(base, kLPSPI_RxDataReadyFlag);

	LOG_DBG("Receieved %d SPI words", total_words_read);
}

static uint32_t lpspi_next_tx_word(const struct device *dev, int offset)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint8_t *byte = ctx->tx_buf + offset;
	uint32_t num_bytes = MIN(data->word_size_bytes, ctx->tx_len);
	uint32_t next_word = 0;

	for (uint8_t i = 0; i < num_bytes; i++) {
		next_word |= *byte << (BITS_PER_BYTE * i);
	}

	return next_word;
}

static void lpspi_fill_tx_fifo(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	size_t bytes_in_xfer = data->transfer_len * data->word_size_bytes;
	size_t offset;

	for (offset = 0; offset < bytes_in_xfer; offset += data->word_size_bytes) {
		LPSPI_WriteData(base, lpspi_next_tx_word(dev, offset));
	}

	LOG_DBG("Filled TX fifo with %d words (%d bytes)", data->transfer_len, offset);
}

static void lpspi_fill_tx_fifo_nop(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;

	for (int i = 0; i < data->transfer_len; i++) {
		LPSPI_WriteData(base, 0);
	}

	LOG_DBG("Filled TX fifo with %d NOPs", data->transfer_len);
}

static void lpspi_next_tx_fill(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t max_chunk = spi_context_max_continuous_chunk(ctx);

	/* Convert bytes to words for this xfer */
	max_chunk /= data->word_size_bytes;
	max_chunk = MIN(max_chunk, config->tx_fifo_size);
	data->transfer_len = max_chunk;

	if (spi_context_tx_buf_on(ctx)) {
		lpspi_fill_tx_fifo(dev);
	} else {
		lpspi_fill_tx_fifo_nop(dev);
	}

	/* now that fifo has data, we enable the interrupt to happen when it empties */
	LPSPI_EnableInterrupts(base, (uint32_t)kLPSPI_TxInterruptEnable);
}

static void lpspi_handle_tx_irq(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_update_tx(ctx, data->word_size_bytes, data->transfer_len);

	/* Since the watermark is 0, need to disable the interrupt until we fill the fifo */
	LPSPI_DisableInterrupts(base, (uint32_t)kLPSPI_TxInterruptEnable);
	LPSPI_ClearStatusFlags(base, kLPSPI_TxDataRequestFlag);

	/* Having no buffer length left indicates transfer is done, if there
	 * was RX to do left, the TX buf would be null but
	 * ctx still tracks length of dummy data
	 */
	if (!spi_context_tx_on(ctx)) {
		/* Disable chip select */
		base->TCR = 0;
		spi_context_cs_control(ctx, false);

		data->transfer_len = 0;
		return;
	}

	lpspi_next_tx_fill(data->dev);
}

static void lpspi_isr(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t status_flags = LPSPI_GetStatusFlags(base);

	/* handle RX first to avoid fifo overflow */
	if (status_flags & kLPSPI_RxDataReadyFlag) {
		lpspi_handle_rx_irq(dev);
	}

	if (status_flags & kLPSPI_TxDataRequestFlag) {
		lpspi_handle_tx_irq(dev);
	}
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	data->word_size_bytes = SPI_WORD_SIZE_GET(spi_cfg->operation) / BITS_PER_BYTE;
	if (data->word_size_bytes > 4) {
		LOG_ERR("Maximum 4 byte word size");
		ret = -EINVAL;
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->word_size_bytes);

	LPSPI_FlushFifo(base, true, true);
	LPSPI_ClearStatusFlags(base, (uint32_t)kLPSPI_AllStatusFlag);
	LPSPI_DisableInterrupts(base, (uint32_t)kLPSPI_AllInterruptEnable);

	LOG_DBG("Starting LPSPI transfer");
	spi_context_cs_control(&data->ctx, true);

	LPSPI_SetFifoWatermarks(base, 0, 0);
	LPSPI_EnableInterrupts(base, (uint32_t)kLPSPI_RxInterruptEnable);
	LPSPI_Enable(base, true);

	/* keep the chip select asserted until the end of the zephyr xfer */
	base->TCR |= LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK;

	/* start the transfer sequence with first tx fill,
	 * once that finishes, irq will start the next and so on
	 */
	lpspi_next_tx_fill(dev);

	ret = spi_context_wait_for_completion(&data->ctx);
out:
	spi_context_release(&data->ctx, ret);

	return ret;
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
	static struct spi_mcux_data spi_mcux_data_##n = {SPI_NXP_LPSPI_COMMON_DATA_INIT(n)};       \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_init, NULL, &spi_mcux_data_##n,                      \
				  &spi_mcux_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &spi_mcux_driver_api);

#define SPI_MCUX_LPSPI_INIT_IF_DMA(n) IF_DISABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_INIT(n)))

#define SPI_MCUX_LPSPI_INIT(n)                                                                     \
	COND_CODE_1(CONFIG_SPI_MCUX_LPSPI_DMA,				   \
						(SPI_MCUX_LPSPI_INIT_IF_DMA(n)), (LPSPI_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_LPSPI_INIT)
