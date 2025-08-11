/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(spi_lpspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

struct lpspi_driver_data {
	size_t total_words_to_clock;
	size_t words_clocked;
	uint8_t word_size_bytes;
	uint8_t lpspi_op_mode;
};

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
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t num_bytes = lpspi_data->word_size_bytes;
	uint8_t *buf = ctx->rx_buf + offset;
	uint32_t word = base->RDR;

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
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	size_t buf_len = ctx->rx_len;
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
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t total_words_written = 0;
	uint8_t total_words_read = 0;
	uint8_t words_read;
	uint8_t rx_fsr;

	base->SR = LPSPI_SR_RDF_MASK;

	LOG_DBG("RX FIFO: %d, RX BUF: %p", rx_fsr, ctx->rx_buf);

	while ((rx_fsr = rx_fifo_cur_len(base)) > 0 && spi_context_rx_on(ctx)) {
		words_read = lpspi_rx_buf_write_words(dev, rx_fsr);
		total_words_read += words_read;
		total_words_written += (spi_context_rx_buf_on(ctx) ? words_read : 0);
		spi_context_update_rx(ctx, lpspi_data->word_size_bytes, words_read);
	}

	LOG_DBG("RX done %d words to spi buf", total_words_written);
}

/* constructs the next word from the buffer */
static inline uint32_t lpspi_next_tx_word(const struct device *dev, const uint8_t *buf,
					  int offset, size_t max_bytes)
{
	const uint8_t *byte = buf + offset;
	uint32_t next_word = 0;

	for (uint8_t i = 0; i < max_bytes; i++) {
		next_word |= byte[i] << (BITS_PER_BYTE * i);
	}

	return next_word;
}

/* fills the TX fifo with specified amount of data from the specified buffer */
static inline void lpspi_fill_tx_fifo(const struct device *dev, const uint8_t *buf,
				      size_t buf_len, size_t fill_len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	uint8_t word_size = lpspi_data->word_size_bytes;
	size_t offset = 0;
	uint32_t next_word;
	uint32_t next_word_bytes;

	for (int word_count = 0; word_count < fill_len; word_count++) {
		next_word_bytes = MIN(word_size, buf_len);
		next_word = lpspi_next_tx_word(dev, buf, offset, next_word_bytes);
		base->TDR = next_word;
		offset += word_size;
		buf_len -= word_size;
	}

	lpspi_data->words_clocked += fill_len;
	LOG_DBG("Filled TX FIFO to %d words (%d bytes)", fill_len, offset);
}

/* just fills TX fifo with the specified amount of NOPS */
static void lpspi_fill_tx_fifo_nop(const struct device *dev, size_t fill_len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;

	for (int i = 0; i < fill_len; i++) {
		base->TDR = 0;
	}

	lpspi_data->words_clocked += fill_len;
	LOG_DBG("Filled TX fifo with %d NOPs", fill_len);
}

/* handles refilling the TX fifo from empty */
static void lpspi_next_tx_fill(const struct device *dev)
{
	const struct lpspi_config *config = dev->config;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t left_in_fifo = tx_fifo_cur_len(base);
	size_t fill_len = MIN(ctx->tx_len, config->tx_fifo_size - left_in_fifo);
	size_t actual_filled = 0;

	const struct spi_buf *current_buf = ctx->current_tx;
	const uint8_t *cur_buf_pos = ctx->tx_buf;
	size_t cur_buf_len_left = ctx->tx_len;
	size_t bufs_left = ctx->tx_count;

	while (fill_len > 0) {
		size_t next_buf_fill = MIN(cur_buf_len_left, fill_len);

		if (cur_buf_pos == NULL) {
			lpspi_fill_tx_fifo_nop(dev, next_buf_fill);
		} else {
			lpspi_fill_tx_fifo(dev, cur_buf_pos,
						current_buf->len, next_buf_fill);
		}

		fill_len -= next_buf_fill;
		cur_buf_pos += next_buf_fill;

		/* in the case where we just filled as much as we could from the current buffer,
		 * this logic while wrong should have no effect, since fill_len will be 0,
		 * so I choose not to make the code extra complex
		 */
		bufs_left--;
		if (bufs_left > 0) {
			current_buf += 1;
			cur_buf_len_left = current_buf->len;
			cur_buf_pos = current_buf->buf;
		} else {
			fill_len = 0;
		}

		actual_filled += next_buf_fill;
	}

	spi_context_update_tx(ctx, lpspi_data->word_size_bytes, actual_filled);
}

static inline void lpspi_handle_tx_irq(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	uint8_t op_mode = lpspi_data->lpspi_op_mode;
	struct spi_context *ctx = &data->ctx;
	uint32_t status_flags = base->SR;

	base->SR = LPSPI_SR_TDF_MASK;

	if (op_mode == SPI_OP_MODE_SLAVE && (status_flags & LPSPI_SR_TEF_MASK)) {
		/* handling err051588 */
		base->SR = LPSPI_SR_TEF_MASK;
		/* workaround is to reset the transmit fifo before writing any new data */
		base->CR |= LPSPI_CR_RTF_MASK;
	}

	/* If we receive a TX interrupt but no more data is available,
	 * we can be sure that all data has been written to the fifo.
	 * Disable the interrupt to signal that we are done.
	 */
	if (!spi_context_tx_on(ctx)) {
		base->IER &= ~LPSPI_IER_TDIE_MASK;
		return;
	}

	lpspi_next_tx_fill(dev);
}

static inline void lpspi_end_xfer(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_complete(ctx, dev, 0);
	NVIC_ClearPendingIRQ(config->irqn);
	if (!(ctx->config->operation & SPI_HOLD_ON_CS)) {
		base->TCR &= ~(LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK);
		/* don't need to wait for TCR since we are at end of xfer + in IRQ context */
	}
	spi_context_cs_control(ctx, false);
	spi_context_release(&data->ctx, 0);
}

static void lpspi_isr(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	uint8_t word_size_bytes = lpspi_data->word_size_bytes;
	struct spi_context *ctx = &data->ctx;
	uint32_t status_flags = base->SR;

	if (status_flags & LPSPI_SR_RDF_MASK && base->IER & LPSPI_IER_RDIE_MASK) {
		lpspi_handle_rx_irq(dev);
	}

	if (status_flags & LPSPI_SR_TDF_MASK && base->IER & LPSPI_IER_TDIE_MASK) {
		lpspi_handle_tx_irq(dev);
	}

	if (spi_context_rx_len_left(ctx, word_size_bytes) == 0) {
		base->IER &= ~LPSPI_IER_RDIE_MASK;
		base->CR |= LPSPI_CR_RRF_MASK; /* flush rx fifo */
	}

	if (spi_context_tx_on(ctx)) {
		return;
	}

	/* Both receive and transmit parts disable their interrupt once finished. */
	if (base->IER == 0) {
		lpspi_end_xfer(dev);
		return;
	}

	/* only explanation at this point is one of two things:
	 * 1) that rx is bigger than tx and we need to clock nops
	 * 2) this is a version of LPSPI which will not clock the last bit of the transfer
	 *    in continuous mode until the TCR is written to end the XFER
	 */

	if (lpspi_data->words_clocked >= lpspi_data->total_words_to_clock) {
		/* Due to stalling behavior on older LPSPI, if we know we already wrote all the
		 * words into the fifo, then we need to end xfer manually by writing TCR
		 * in order to get last bit clocked out on bus. So all we need to do is touch the
		 * TCR by writing to fifo through TCR register and wait for final RX interrupt.
		 */
		base->TCR = base->TCR;
		return;
	}

	/* At this point we know only case is that we need to put NOPs in the TX fifo
	 * in order to get the rest of the RX
	 */

	size_t rx_fifo_len = rx_fifo_cur_len(base);
	size_t tx_fifo_len = tx_fifo_cur_len(base);
	size_t words_really_left = lpspi_data->total_words_to_clock - lpspi_data->words_clocked;

	/*
	 * Goal here is to provide the number of TX NOPS to match the amount of RX
	 * we have left to receive, so that we provide the correct number of
	 * clocks to the bus, since the clocks only happen when TX data is being sent.
	 *
	 * The expected RX left is essentially what is left in the spi transfer
	 * minus what we have just got in the fifo, but prevent underflow of this
	 * subtraction since it is unsigned.
	 */
	size_t expected_rx_left = rx_fifo_len < words_really_left ?
					words_really_left - rx_fifo_len : 0;

	/*
	 * We know the expected amount of RX we have left but only fill up to the
	 * max of the RX fifo size so that we don't have some overflow of the FIFO later.
	 * Similarly, we shouldn't overfill the TX fifo with too many NOPs.
	 */
	size_t tx_fifo_space_left = config->tx_fifo_size - tx_fifo_len;
	size_t rx_fifo_space_left = config->rx_fifo_size - rx_fifo_len;
	size_t max_fifo_fill = MIN(tx_fifo_space_left, rx_fifo_space_left);
	size_t max_fill = MIN(max_fifo_fill, expected_rx_left);

	lpspi_fill_tx_fifo_nop(dev, max_fill);
}

static void lpspi_master_setup_native_cs(const struct device *dev, const struct spi_config *spi_cfg)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	/* keep the chip select asserted until the end of the zephyr xfer by using
	 * continunous transfer mode. If SPI_HOLD_ON_CS is requested, we need
	 * to also set CONTC in order to continue the previous command to keep CS
	 * asserted.
	 */
	if (spi_cfg->operation & SPI_HOLD_ON_CS || base->TCR & LPSPI_TCR_CONTC_MASK) {
		base->TCR |= LPSPI_TCR_CONTC_MASK | LPSPI_TCR_CONT_MASK;
	} else {
		base->TCR |= LPSPI_TCR_CONT_MASK;
	}

	/* tcr is written to tx fifo */
	lpspi_wait_tx_fifo_empty(dev);
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t op_mode = SPI_OP_MODE_GET(spi_cfg->operation);
	int ret = 0;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	lpspi_data->word_size_bytes =
		DIV_ROUND_UP(SPI_WORD_SIZE_GET(spi_cfg->operation), BITS_PER_BYTE);
	if (lpspi_data->word_size_bytes > 4) {
		LOG_ERR("Maximum 4 byte word size");
		ret = -EINVAL;
		goto error;
	}

	if (op_mode == SPI_OP_MODE_SLAVE && !(spi_cfg->operation & SPI_MODE_CPHA)) {
		LOG_ERR("CPHA=0 not supported with LPSPI peripheral mode");
		ret = -ENOTSUP;
		goto error;
	}

	if (data->major_version < 2 && spi_cfg->operation & SPI_HOLD_ON_CS &&
	    !spi_cs_is_gpio(spi_cfg)) {
		/* on this version of LPSPI, due to errata in design
		 * CS must be deasserted in order to clock all words,
		 * so HOLD_ON_CS flag cannot be supported.
		 */
		return -EINVAL;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, lpspi_data->word_size_bytes);
	lpspi_data->lpspi_op_mode = op_mode;

	ret = lpspi_configure(dev, spi_cfg);
	if (ret) {
		goto error;
	}

	base->CR |= LPSPI_CR_RRF_MASK;
	base->IER = 0;
	base->SR |= LPSPI_INTERRUPT_BITS;

	size_t max_side_clocks = MAX(spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));

	lpspi_data->total_words_to_clock =
				DIV_ROUND_UP(max_side_clocks, lpspi_data->word_size_bytes);
	lpspi_data->words_clocked = 0;

	LOG_DBG("Starting LPSPI transfer");
	spi_context_cs_control(ctx, true);

	if (op_mode == SPI_OP_MODE_MASTER) {
		/* set watermarks to 0 so get tx interrupt when fifo empty
		 * and rx interrupt when any data received
		 */
		base->FCR = 0;
	} else {
		 /* set watermarks so that we are as responsive to master as possible and don't
		  * miss any communication. This means RX interrupt at 0 so that if we ever
		  * get any data, we get interrupt and handle immediately, and TX interrupt
		  * to one less than the max of the fifo (-2 of size) so that we have as much
		  * data ready to send to master as possible at any time
		  */
		base->FCR = LPSPI_FCR_TXWATER(config->tx_fifo_size - 1);
		base->CFGR1 |= LPSPI_CFGR1_AUTOPCS_MASK;
	}

	base->CR |= LPSPI_CR_MEN_MASK;

	if (op_mode == SPI_OP_MODE_MASTER) {
		lpspi_master_setup_native_cs(dev, spi_cfg);
	}

	/* start the transfer sequence which are handled by irqs */
	lpspi_next_tx_fill(dev);

	base->IER |= LPSPI_IER_TDIE_MASK | LPSPI_IER_RDIE_MASK;

	ret = spi_context_wait_for_completion(ctx);
	if (ret >= 0) {
		return ret;
	}

error:
	spi_context_release(ctx, ret);
	return ret;
}

static int lpspi_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int lpspi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static DEVICE_API(spi, lpspi_driver_api) = {
	.transceive = lpspi_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = lpspi_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_lpspi_release,
};

static int lpspi_init(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	int err = 0;

	err = spi_nxp_init_common(dev);
	if (err) {
		return err;
	}

	/* Starting config should be master with active low CS, to make sure
	 * the CS lines are configured properly at init for the most common use
	 * cases. This can be changed later on transceive call if user specifies
	 * different spi configuration.
	 */
	base->CFGR1 |= LPSPI_CFGR1_MASTER_MASK;
	base->CFGR1 &= ~LPSPI_CFGR1_PCSPOL_MASK;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define LPSPI_INIT(n)                                                                              \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_LPSPI_CONFIG_INIT(n)                                                              \
                                                                                                   \
	static struct lpspi_driver_data lpspi_##n##_driver_data;                                   \
                                                                                                   \
	static struct lpspi_data lpspi_data_##n = {                                             \
		SPI_NXP_LPSPI_COMMON_DATA_INIT(n)                                                  \
		.driver_data = &lpspi_##n##_driver_data,                                           \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, lpspi_init, NULL, &lpspi_data_##n,                            \
				  &lpspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &lpspi_driver_api);

#define SPI_LPSPI_INIT_IF_DMA(n) IF_DISABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_INIT(n)))

#define SPI_LPSPI_INIT(n)                                                                     \
	COND_CODE_1(CONFIG_SPI_NXP_LPSPI_DMA,				   \
						(SPI_LPSPI_INIT_IF_DMA(n)), (LPSPI_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_LPSPI_INIT)
