/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_lpspi_dma, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/drivers/dma.h>
#include "spi_nxp_lpspi_priv.h"

/* dummy memory used for transferring NOP when tx buf is null */
static uint32_t tx_nop_val; /* check compliance says no init to 0, but should be 0 in bss */
/* dummy memory for transferring to when RX buf is null */
static uint32_t dummy_buffer;

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
	bool chunk_done;
};

struct spi_nxp_dma_data {
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
};

static struct dma_block_config *lpspi_dma_common_load(struct spi_dma_stream *stream,
							 const struct device *dev,
							 const uint8_t *buf, size_t len)
{
	struct dma_block_config *blk_cfg = &stream->dma_blk_cfg;

	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	blk_cfg->block_size = len;

	stream->dma_cfg.source_burst_length = 1;
	stream->dma_cfg.user_data = (void *)dev;
	stream->dma_cfg.head_block = blk_cfg;

	return blk_cfg;
}

static int lpspi_dma_tx_load(const struct device *dev, const uint8_t *buf, size_t len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *stream = &dma_data->dma_tx;
	struct dma_block_config *blk_cfg = lpspi_dma_common_load(stream, dev, buf, len);

	if (buf == NULL) {
		/* pretend that nop value comes from peripheral so dma doesn't move source */
		blk_cfg->source_address = (uint32_t)&tx_nop_val;
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_PERIPHERAL;
	} else {
		blk_cfg->source_address = (uint32_t)buf;
		stream->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	}

	blk_cfg->dest_address = (uint32_t) &(base->TDR);

	return dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
}

static int lpspi_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *stream = &dma_data->dma_rx;
	struct dma_block_config *blk_cfg = lpspi_dma_common_load(stream, dev, buf, len);

	if (buf == NULL) {
		/* pretend it is peripheral xfer so DMA just xfer to dummy buf */
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_PERIPHERAL;
		blk_cfg->dest_address = (uint32_t)&dummy_buffer;
	} else {
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		blk_cfg->dest_address = (uint32_t)buf;
	}

	blk_cfg->source_address = (uint32_t) &(base->RDR);

	return dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
}

static inline int lpspi_dma_rxtx_load(const struct device *dev)
{
	struct lpspi_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *rx = &dma_data->dma_rx;
	struct spi_dma_stream *tx = &dma_data->dma_tx;
	struct spi_context *ctx = &data->ctx;
	size_t next_chunk_size = spi_context_max_continuous_chunk(ctx);
	int ret = 0;

	if (next_chunk_size == 0) {
		/* In case both buffers are 0 length, we should not even be here
		 * and attempting to set up a DMA transfer like this will cause
		 * errors that lock up the system in some cases with eDMA.
		 */
		return -ENODATA;
	}

	ret = lpspi_dma_tx_load(dev, ctx->tx_buf, next_chunk_size);
	if (ret != 0) {
		return ret;
	}

	ret = lpspi_dma_rx_load(dev, ctx->rx_buf, next_chunk_size);
	if (ret != 0) {
		return ret;
	}

	ret = dma_start(rx->dma_dev, rx->channel);
	if (ret != 0) {
		return ret;
	}

	return dma_start(tx->dma_dev, tx->channel);
}

static int lpspi_dma_next_fill(const struct device *dev)
{
	struct lpspi_data *data = (struct lpspi_data *)dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *rx = &dma_data->dma_rx;
	struct spi_dma_stream *tx = &dma_data->dma_tx;

	rx->chunk_done = false;
	tx->chunk_done = false;

	return lpspi_dma_rxtx_load(dev);
}

static void spi_mcux_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	const struct device *spi_dev = arg;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(spi_dev, reg_base);
	struct lpspi_data *data = (struct lpspi_data *)spi_dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *rx = &dma_data->dma_rx;
	struct spi_dma_stream *tx = &dma_data->dma_tx;
	struct spi_context *ctx = &data->ctx;
	char debug_char;

	if (status < 0) {
		goto error;
	} else {
		/* don't care about positive values, normalize to "okay" = 0 */
		status = 0;
	}

	if (channel == rx->channel) {
		spi_context_update_rx(ctx, 1, rx->dma_blk_cfg.block_size);
		debug_char = 'R';
		rx->chunk_done = true;
	} else if (channel == tx->channel) {
		spi_context_update_tx(ctx, 1, tx->dma_blk_cfg.block_size);
		debug_char = 'T';
		tx->chunk_done = true;
	} else {
		/* invalid channel */
		status = -EIO;
		goto error;
	}

	LOG_DBG("DMA %cX Block Complete", debug_char);

	/* wait for the other channel to finish if needed */
	if (!rx->chunk_done || !tx->chunk_done) {
		return;
	}


	while ((IS_ENABLED(CONFIG_SOC_FAMILY_NXP_IMXRT) ||
		IS_ENABLED(CONFIG_SOC_FAMILY_KINETIS)) &&
		(base->SR & LPSPI_SR_MBF_MASK)) {
		/* wait until module is idle */
	}

	if (spi_context_max_continuous_chunk(ctx) == 0) {
		goto done;
	}

	status = lpspi_dma_next_fill(spi_dev);
	if (status) {
		goto error;
	}

	return;
error:
	LOG_ERR("DMA callback error with channel %d err %d.", channel, status);
done:
	base->DER &= ~(LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK);
	base->TCR &= ~LPSPI_TCR_CONT_MASK;
	lpspi_wait_tx_fifo_empty(spi_dev);
	spi_context_cs_control(ctx, false);
	base->CR |= LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK;
	spi_context_complete(ctx, spi_dev, status);
	spi_context_release(ctx, status);
}

static int transceive_dma(const struct device *dev, const struct spi_config *spi_cfg,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			  bool asynchronous, spi_callback_t cb, void *userdata)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	ret = lpspi_dma_next_fill(dev);
	if (ret == -ENODATA) {
		/* No transfer to do? So just exit */
		ret = 0;
		goto out;
	} else if (ret) {
		goto out;
	}

	if (!(IS_ENABLED(CONFIG_SOC_FAMILY_NXP_IMXRT) || IS_ENABLED(CONFIG_SOC_FAMILY_KINETIS))) {
		base->TCR |= LPSPI_TCR_CONT_MASK;
	}

	spi_context_cs_control(ctx, true);

	base->CR |= LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK;

	base->DER |= LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK;

	ret = spi_context_wait_for_completion(ctx);
	if (ret >= 0) {
		return ret;
	}
out:
	spi_context_release(ctx, ret);
	return ret;
}

static int lpspi_dma_dev_ready(const struct device *dma_dev)
{
	if (!device_is_ready(dma_dev)) {
		LOG_ERR("%s device is not ready", dma_dev->name);
		return false;
	}

	return true;
}

static int spi_mcux_dma_init(const struct device *dev)
{
	struct lpspi_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	int err = 0;

	if (!lpspi_dma_dev_ready(dma_data->dma_tx.dma_dev) ||
	    !lpspi_dma_dev_ready(dma_data->dma_rx.dma_dev)) {
		return -ENODEV;
	}

	err = spi_nxp_init_common(dev);
	if (err) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_nxp_dma_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	return transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_nxp_dma_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					void *userdata)
{
	return transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static DEVICE_API(spi, lpspi_dma_driver_api) = {
	.transceive = spi_nxp_dma_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nxp_dma_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_lpspi_release,
};

static void lpspi_isr(const struct device *dev)
{
	/* ISR not used for DMA based LPSPI driver */
}

#define LPSPI_DMA_COMMON_CFG(n)			\
	.dma_callback = spi_mcux_dma_callback,	\
	.source_data_size = 1,			\
	.dest_data_size = 1,			\
	.block_count = 1

#define SPI_DMA_CHANNELS(n)                                                                        \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(n, tx),                                                   \
		(.dma_tx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),            \
			    .channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, mux),                      \
			    .dma_cfg = {.channel_direction = MEMORY_TO_PERIPHERAL,                 \
			    LPSPI_DMA_COMMON_CFG(n),						   \
			    .dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, tx, source)}},))		   \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(n, rx),                                                   \
		(.dma_rx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),            \
			    .channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, mux),                      \
			    .dma_cfg = {.channel_direction = PERIPHERAL_TO_MEMORY,                 \
			    LPSPI_DMA_COMMON_CFG(n),						   \
			    .dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, rx, source)}},))

#define LPSPI_DMA_INIT(n)                                                                          \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_LPSPI_CONFIG_INIT(n)                                                              \
                                                                                                   \
	static struct spi_nxp_dma_data lpspi_dma_data##n = {SPI_DMA_CHANNELS(n)};                  \
                                                                                                   \
	static struct lpspi_data lpspi_data_##n = {.driver_data = &lpspi_dma_data##n,        \
							 SPI_NXP_LPSPI_COMMON_DATA_INIT(n)};       \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_dma_init, NULL, &lpspi_data_##n,                  \
				  &lpspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &lpspi_dma_driver_api);

#define SPI_NXP_LPSPI_DMA_INIT(n) IF_ENABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_DMA_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_NXP_LPSPI_DMA_INIT)
