/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi_dma, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/drivers/dma.h>
#include "spi_nxp_lpspi_priv.h"

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel; /* stores the channel for dma */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
};

struct spi_nxp_dma_data {
	volatile uint32_t status_flags;
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
	/* dummy value used for transferring NOP when tx buf is null */
	uint32_t dummy_buffer;
};

/* These flags are arbitrary */
#define LPSPI_DMA_ERROR_FLAG   BIT(0)
#define LPSPI_DMA_RX_DONE_FLAG BIT(1)
#define LPSPI_DMA_TX_DONE_FLAG BIT(2)
#define LPSPI_DMA_DONE_FLAG    (LPSPI_DMA_RX_DONE_FLAG | LPSPI_DMA_TX_DONE_FLAG)

/* This function is executed in the interrupt context */
static void spi_mcux_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	/* arg directly holds the spi device */
	const struct device *spi_dev = arg;
	struct spi_mcux_data *data = (struct spi_mcux_data *)spi_dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	char debug_char;

	if (status < 0) {
		goto error;
	}

	/* identify the origin of this callback */
	if (channel == dma_data->dma_tx.channel) {
		/* this part of the transfer ends */
		dma_data->status_flags |= LPSPI_DMA_TX_DONE_FLAG;
		debug_char = 'T';
	} else if (channel == dma_data->dma_rx.channel) {
		/* this part of the transfer ends */
		dma_data->status_flags |= LPSPI_DMA_RX_DONE_FLAG;
		debug_char = 'R';
	} else {
		goto error;
	}

	LOG_DBG("DMA %cX Block Complete", debug_char);

#if CONFIG_SPI_ASYNC
	if (data->ctx.asynchronous && (dma_data->status_flags & LPSPI_DMA_DONE_FLAG)) {
		/* Load dma blocks of equal length */
		size_t dma_size = spi_context_max_continuous_chunk(data->ctx);

		if (dma_size != 0) {
			return;
		}

		spi_context_update_tx(&data->ctx, 1, dma_size);
		spi_context_update_rx(&data->ctx, 1, dma_size);
	}
#endif

	goto done;
error:
	LOG_ERR("DMA callback error with channel %d.", channel);
	dma_data->status_flags |= LPSPI_DMA_ERROR_FLAG;
done:
	spi_context_complete(&data->ctx, spi_dev, 0);
}

static struct dma_block_config *spi_mcux_dma_common_load(struct spi_dma_stream *stream,
							 const struct device *dev,
							 const uint8_t *buf, size_t len)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct dma_block_config *blk_cfg = &stream->dma_blk_cfg;

	/* prepare the block for this TX DMA channel */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	blk_cfg->block_size = len;

	if (buf == NULL) {
		blk_cfg->source_address = (uint32_t)&dma_data->dummy_buffer;
		blk_cfg->dest_address = (uint32_t)&dma_data->dummy_buffer;
		/* pretend it is peripheral xfer so DMA just xfer to dummy buf */
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_PERIPHERAL;
	} else {
		blk_cfg->source_address = (uint32_t)buf;
		blk_cfg->dest_address = (uint32_t)buf;
	}

	/* Transfer 1 byte each DMA loop */
	stream->dma_cfg.source_burst_length = 1;
	stream->dma_cfg.user_data = (void *)dev;
	stream->dma_cfg.head_block = blk_cfg;

	return blk_cfg;
}

static int spi_mcux_dma_tx_load(const struct device *dev, const uint8_t *buf, size_t len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	/* remember active TX DMA channel (used in callback) */
	struct spi_dma_stream *stream = &dma_data->dma_tx;
	struct dma_block_config *blk_cfg = spi_mcux_dma_common_load(stream, dev, buf, len);

	if (buf != NULL) {
		/* tx direction has memory as source and periph as dest. */
		stream->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	}

	/* Dest is LPSPI tx fifo */
	blk_cfg->dest_address = LPSPI_GetTxRegisterAddress(base);

	/* give the client dev as arg, as the callback comes from the dma */
	/* pass our client origin to the dma: data->dma_tx.dma_channel */
	return dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
}

static int spi_mcux_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	/* retrieve active RX DMA channel (used in callback) */
	struct spi_dma_stream *stream = &dma_data->dma_rx;
	struct dma_block_config *blk_cfg = spi_mcux_dma_common_load(stream, dev, buf, len);

	if (buf != NULL) {
		/* rx direction has periph as source and mem as dest. */
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	}

	/* Source is LPSPI rx fifo */
	blk_cfg->source_address = LPSPI_GetRxRegisterAddress(base);

	/* pass our client origin to the dma: data->dma_rx.channel */
	return dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
}

static int wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	int ret;

	do {
		ret = spi_context_wait_for_completion(&data->ctx);
		if (ret) {
			LOG_DBG("Timed out waiting for SPI context to complete");
			return ret;
		} else if (dma_data->status_flags & LPSPI_DMA_ERROR_FLAG) {
			return -EIO;
		}
	} while (!((dma_data->status_flags & LPSPI_DMA_DONE_FLAG) == LPSPI_DMA_DONE_FLAG));

	LOG_DBG("DMA block completed");
	return 0;
}

static inline int spi_mcux_dma_rxtx_load(const struct device *dev, size_t *dma_size)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	/* Clear status flags */
	dma_data->status_flags = 0U;

	/* Load dma blocks of equal length */
	*dma_size = spi_context_max_continuous_chunk(ctx);

	ret = spi_mcux_dma_tx_load(dev, ctx->tx_buf, *dma_size);
	if (ret != 0) {
		return ret;
	}

	ret = spi_mcux_dma_rx_load(dev, ctx->rx_buf, *dma_size);
	if (ret != 0) {
		return ret;
	}

	/* Start DMA */
	ret = dma_start(dma_data->dma_tx.dma_dev, dma_data->dma_tx.channel);
	if (ret != 0) {
		return ret;
	}

	ret = dma_start(dma_data->dma_rx.dma_dev, dma_data->dma_rx.channel);
	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int transceive_dma_async(const struct device *dev, spi_callback_t cb, void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t dma_size;
	int ret;

	ctx->asynchronous = true;
	ctx->callback = cb;
	ctx->callback_data = userdata;

	ret = spi_mcux_dma_rxtx_load(dev, &dma_size);
	if (ret) {
		return ret;
	}

	/* Enable DMA Requests */
	LPSPI_EnableDMA(base, kLPSPI_TxDmaEnable | kLPSPI_RxDmaEnable);

	return 0;
}
#else
#define transceive_dma_async(...) 0
#endif /* CONFIG_SPI_ASYNC */

static int transceive_dma_sync(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t dma_size;
	int ret;

	spi_context_cs_control(ctx, true);

	/* Send each spi buf via DMA, updating context as DMA completes */
	while (ctx->rx_len > 0 || ctx->tx_len > 0) {
		/* Load dma block */
		ret = spi_mcux_dma_rxtx_load(dev, &dma_size);
		if (ret) {
			return ret;
		}

#ifdef CONFIG_SOC_SERIES_MCXN
		while (!(LPSPI_GetStatusFlags(base) & kLPSPI_TxDataRequestFlag)) {
			/* wait until previous tx finished */
		}
#endif

		/* Enable DMA Requests */
		LPSPI_EnableDMA(base, kLPSPI_TxDmaEnable | kLPSPI_RxDmaEnable);

		/* Wait for DMA to finish */
		ret = wait_dma_rx_tx_done(dev);
		if (ret) {
			return ret;
		}

#ifndef CONFIG_SOC_SERIES_MCXN
		while ((LPSPI_GetStatusFlags(base) & kLPSPI_ModuleBusyFlag)) {
			/* wait until module is idle */
		}
#endif

		/* Disable DMA */
		LPSPI_DisableDMA(base, kLPSPI_TxDmaEnable | kLPSPI_RxDmaEnable);

		/* Update SPI contexts with amount of data we just sent */
		spi_context_update_tx(ctx, 1, dma_size);
		spi_context_update_rx(ctx, 1, dma_size);
	}

	spi_context_cs_control(ctx, false);

	base->TCR &= ~LPSPI_TCR_CONT_MASK;

	return 0;
}

static int transceive_dma(const struct device *dev, const struct spi_config *spi_cfg,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			  bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	int ret;

	if (!asynchronous) {
		spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);
	}

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret && !asynchronous) {
		goto out;
	} else if (ret) {
		return ret;
	}

#ifdef CONFIG_SOC_SERIES_MCXN
	base->TCR |= LPSPI_TCR_CONT_MASK;
#endif

	/* DMA is fast enough watermarks are not required */
	LPSPI_SetFifoWatermarks(base, 0U, 0U);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	if (asynchronous) {
		ret = transceive_dma_async(dev, cb, userdata);
	} else {
		ret = transceive_dma_sync(dev);
	}

out:
	spi_context_release(&data->ctx, ret);
	return ret;
}

static int lpspi_dma_dev_ready(const struct device *dma_dev)
{
	if (!device_is_ready(dma_dev)) {
		LOG_ERR("%s device is not ready", dma_dev->name);
		return -ENODEV;
	}

	return 0;
}

static int lpspi_dma_devs_ready(struct spi_mcux_data *data)
{
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;

	return lpspi_dma_dev_ready(dma_data->dma_tx.dma_dev) |
	       lpspi_dma_dev_ready(dma_data->dma_rx.dma_dev);
}

static int spi_mcux_dma_init(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	err = lpspi_dma_devs_ready(data);
	if (err < 0) {
		return err;
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

static DEVICE_API(spi, spi_mcux_driver_api) = {
	.transceive = spi_nxp_dma_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nxp_dma_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mcux_release,
};

static void lpspi_isr(const struct device *dev)
{
	/* ISR not used for DMA based LPSPI driver */
}

#define SPI_DMA_CHANNELS(n)                                                                        \
	IF_ENABLED(                                                                                \
		DT_INST_DMAS_HAS_NAME(n, tx),                                                      \
		(.dma_tx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),            \
			    .channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, mux),                      \
			    .dma_cfg = {.channel_direction = MEMORY_TO_PERIPHERAL,                 \
					.dma_callback = spi_mcux_dma_callback,                     \
					.source_data_size = 1,                                     \
					.dest_data_size = 1,                                       \
					.block_count = 1,                                          \
					.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, tx, source)}},)) \
	IF_ENABLED(                                                                                \
		DT_INST_DMAS_HAS_NAME(n, rx),                                                      \
		(.dma_rx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),            \
			    .channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, mux),                      \
			    .dma_cfg = {.channel_direction = PERIPHERAL_TO_MEMORY,                 \
					.dma_callback = spi_mcux_dma_callback,                     \
					.source_data_size = 1,                                     \
					.dest_data_size = 1,                                       \
					.block_count = 1,                                          \
					.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, rx, source)}},))

#define LPSPI_DMA_INIT(n)                                                                          \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_MCUX_LPSPI_CONFIG_INIT(n)                                                              \
                                                                                                   \
	static struct spi_nxp_dma_data lpspi_dma_data##n = {SPI_DMA_CHANNELS(n)};                  \
                                                                                                   \
	static struct spi_mcux_data spi_mcux_data_##n = {.driver_data = &lpspi_dma_data##n,        \
							 SPI_NXP_LPSPI_COMMON_DATA_INIT(n)};       \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_dma_init, NULL, &spi_mcux_data_##n,                  \
				  &spi_mcux_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &spi_mcux_driver_api);

#define SPI_NXP_LPSPI_DMA_INIT(n) IF_ENABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_DMA_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_NXP_LPSPI_DMA_INIT)
