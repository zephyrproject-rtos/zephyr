/*
 * Copyright 2018, 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(spi_lpspi, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/drivers/dma.h>
#include "spi_nxp_lpspi_priv.h"

/* These states indicate what's the status of RX and TX, also synchronization
 * status of DMA size of the next DMA transfer.
 */
typedef enum {
	LPSPI_TRANSFER_STATE_NULL,
	LPSPI_TRANSFER_STATE_ONGOING,
	LPSPI_TRANSFER_STATE_NEXT_DMA_SIZE_UPDATED,
	LPSPI_TRANSFER_STATE_TX_DONE,
	LPSPI_TRANSFER_STATE_RX_DONE,
	LPSPI_TRANSFER_STATE_RX_TX_DONE,
	LPSPI_TRANSFER_STATE_INVALID = 0xFFFFFFFFUL,
} lpspi_transfer_state_t;

/* dummy memory used for transferring NOP when tx buf is null */
static uint32_t tx_nop_val; /* check compliance says no init to 0, but should be 0 in bss */
/* dummy memory for transferring to when RX buf is null */
static uint32_t dummy_buffer;

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
};

struct spi_nxp_dma_data {
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;

	lpspi_transfer_state_t state;
	/* This DMA size is used in callback function for RX and TX context update.
	 * because of old LPSPI IP limitation, RX complete depend on next TX DMA transfer start,
	 * so TX and RX not always start at the same time while we can only calculate DMA transfer
	 * size once and update the buffer pointers at the same time.
	 */
	size_t synchronize_dma_size;
};

/*
 * Issue a TCR (Transmit Command Register) command to properly end RX DMA transfers
 * on certain LPSPI versions. The behavior depends on:
 *
 * 1. LPSPI Hardware Version:
 *    - Version 1 (RT1170, RT10xx, Kinetis K series): TCR issue always required
 *    - Version 2 (RT1180, MCXN, RT700, K32W, S32K3xx, MCXL10): TCR issue not needed
 *
 * 2. SPI_HOLD_ON_CS Configuration:
 *    - If enabled: Keeps chip select (PCS) asserted between transfers
 *    - If disabled: Deasserts PCS after each transfer
 *
 * This function checks the LPSPI version and SPI_HOLD_ON_CS setting to determine
 * if a TCR command is needed. For version 1, TCR is always issued. For version 2,
 * TCR is only issued if SPI_HOLD_ON_CS is not set. Therefore, SPI_HOLD_ON_CS is not
 * supported for version 1.
 *
 * The LPSPI version can be read from the VERID register, which is typically the
 * first register in the memory map.
 */
static void spi_mcux_issue_TCR(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct spi_config *spi_cfg = DEV_DATA(dev)->ctx.config;
	uint8_t major_ver = (base->VERID & LPSPI_VERID_MAJOR_MASK) >> LPSPI_VERID_MAJOR_SHIFT;

	/* On old LPSPI versions, we always have to issue TCR, or transaction will never end.
	 * On a newer LPSPI version, only issue TCR when hold on CS feature is disabled.
	 */
	if (major_ver < 2 || !(spi_cfg->operation & SPI_HOLD_ON_CS)) {
		base->TCR &= ~LPSPI_TCR_CONTC_MASK;
	}
}

static struct dma_block_config *lpspi_dma_common_load(struct spi_dma_stream *stream,
						      const struct device *dev, const uint8_t *buf,
						      size_t len)
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

/* Return values:
 * positive value if a data chunk is loaded successfully and return the data chunk size loaded;
 * negative value if error happens and return the error code;
 * 0 if no data is loaded;
 */
static int lpspi_dma_rxtx_load(const struct device *dev)
{
	struct lpspi_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *rx = &dma_data->dma_rx;
	struct spi_dma_stream *tx = &dma_data->dma_tx;
	struct spi_context *ctx = &data->ctx;
	size_t dma_size = spi_context_max_continuous_chunk(ctx);
	int ret = 0;

	if (dma_size == 0) {
		/* In case both buffers are 0 length, we should not even be here
		 * and attempting to set up a DMA transfer like this will cause
		 * errors that lock up the system in some cases with eDMA.
		 */
		return 0;
	}

	ret = lpspi_dma_tx_load(dev, ctx->tx_buf, dma_size);
	if (ret != 0) {
		return ret;
	}

	ret = lpspi_dma_rx_load(dev, ctx->rx_buf, dma_size);
	if (ret != 0) {
		return ret;
	}

	ret = dma_start(rx->dma_dev, rx->channel);
	if (ret != 0) {
		return ret;
	}

	ret = dma_start(tx->dma_dev, tx->channel);
	if (ret != 0) {
		return ret;
	}

	return dma_size;
}

static void lpspi_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	/* arg directly holds the spi device */
	const struct device *spi_dev = arg;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(spi_dev, reg_base);
	struct lpspi_data *data = (struct lpspi_data *)spi_dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_dma_stream *rx = &dma_data->dma_rx;
	struct spi_dma_stream *tx = &dma_data->dma_tx;
	struct spi_context *ctx = &data->ctx;
	char debug_char = (channel == dma_data->dma_tx.channel) ? 'T' : 'R';
	int ret = 0;

	if (status < 0) {
		ret = status;
		goto error;
	}

	if (channel != dma_data->dma_tx.channel && channel != dma_data->dma_rx.channel) {
		ret = -EIO;
		goto error;
	}

	switch (dma_data->state) {
	case LPSPI_TRANSFER_STATE_ONGOING:
		spi_context_update_tx(ctx, 1, tx->dma_blk_cfg.block_size);
		spi_context_update_rx(ctx, 1, rx->dma_blk_cfg.block_size);
		/* Calculate next DMA transfer size */
		dma_data->synchronize_dma_size = spi_context_max_continuous_chunk(ctx);
		LOG_DBG("tx len:%d rx len:%d next dma size:%d",	ctx->tx_len, ctx->rx_len,
			dma_data->synchronize_dma_size);
		if (dma_data->synchronize_dma_size > 0)	{
			ret = (channel == dma_data->dma_tx.channel)
				      ? lpspi_dma_tx_load(spi_dev, ctx->tx_buf,
							  dma_data->synchronize_dma_size)
				      : lpspi_dma_rx_load(spi_dev, ctx->rx_buf,
							  dma_data->synchronize_dma_size);

			if (ret != 0) {
				goto error;
			}

			ret = dma_start(dev, channel);
			if (ret != 0) {
				goto error;
			}
			dma_data->state = LPSPI_TRANSFER_STATE_NEXT_DMA_SIZE_UPDATED;
		} else {
			ret = dma_stop(dev, channel);
			if (ret != 0) {
				goto error;
			}
			/* This is the end of the transfer. */
			if (channel == dma_data->dma_tx.channel) {
				spi_mcux_issue_TCR(spi_dev);
				dma_data->state = LPSPI_TRANSFER_STATE_TX_DONE;
				base->DER &= ~LPSPI_DER_TDDE_MASK;
			} else {
				dma_data->state = LPSPI_TRANSFER_STATE_RX_DONE;
				base->DER &= ~LPSPI_DER_RDDE_MASK;
			}
		}
		break;
	case LPSPI_TRANSFER_STATE_NEXT_DMA_SIZE_UPDATED:
		ret = (channel == dma_data->dma_tx.channel)
			      ? lpspi_dma_tx_load(spi_dev, ctx->tx_buf,
						  dma_data->synchronize_dma_size)
			      : lpspi_dma_rx_load(spi_dev, ctx->rx_buf,
						  dma_data->synchronize_dma_size);
		dma_data->synchronize_dma_size = 0;

		if (ret != 0) {
			goto error;
		}

		ret = dma_start(dev, channel);
		if (ret != 0) {
			goto error;
		}
		dma_data->state = LPSPI_TRANSFER_STATE_ONGOING;
		break;

	case LPSPI_TRANSFER_STATE_TX_DONE:
	case LPSPI_TRANSFER_STATE_RX_DONE:
		dma_data->state = LPSPI_TRANSFER_STATE_RX_TX_DONE;
		/* TX and RX both done here. */
		spi_context_complete(ctx, spi_dev, 0);
		spi_context_cs_control(ctx, false);
		break;

	default:
		LOG_ERR("unknown spi stransfer state:%d", dma_data->state);
		ret = -EIO;
		goto error;
	}

	LOG_DBG("DMA %cX Block Complete", debug_char);
	return;
error:
	LOG_ERR("DMA callback error with channel %d.", channel);
	spi_context_complete(ctx, spi_dev, ret);
	spi_context_cs_control(ctx, false);
}

static int transceive_dma(const struct device *dev, const struct spi_config *spi_cfg,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			  bool asynchronous, spi_callback_t cb, void *userdata)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct lpspi_data *data = dev->data;
	struct spi_nxp_dma_data *dma_data = (struct spi_nxp_dma_data *)data->driver_data;
	struct spi_context *ctx = &data->ctx;
	uint8_t major_ver = (base->VERID & LPSPI_VERID_MAJOR_MASK) >> LPSPI_VERID_MAJOR_SHIFT;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	ret = lpspi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	/* Check CS hold on feature for DMA mode, it is not supported on some platform. */
	if ((spi_cfg->operation & SPI_HOLD_ON_CS) && major_ver < 2) {
		LOG_ERR("SPI CS hold on feature is not supported on this platform.");
		return -ENOTSUP;
	}

	/* Always use continuous mode to satisfy SPI API requirements. */
	base->TCR |= LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK;

	/* Please set both watermarks as 0 because there are some synchronize requirements
	 * between RX and TX on RT platform. TX and RX DMA callback must be called in interleaved
	 * mode, a none-zero TX watermark may break this.
	 */
	base->FCR = LPSPI_FCR_TXWATER(0) | LPSPI_FCR_RXWATER(0);
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* Set next dma size is invalid. */
	dma_data->synchronize_dma_size = 0;
	dma_data->state = LPSPI_TRANSFER_STATE_NULL;

	/* Load dma block */
	ret = lpspi_dma_rxtx_load(dev);
	if (ret <= 0) {
		goto out;
	}

	dma_data->state = LPSPI_TRANSFER_STATE_ONGOING;
	/* Set CS line just before DMA transfer. */
	spi_context_cs_control(ctx, true);
	/* Enable DMA Requests */
	base->DER |= LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK;

	ret = spi_context_wait_for_completion(ctx);
	if (ret) {
		spi_context_cs_control(ctx, false);
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

static int lpspi_dma_init(const struct device *dev)
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
	.dma_callback = lpspi_dma_callback,	\
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
	SPI_DEVICE_DT_INST_DEFINE(n, lpspi_dma_init, NULL, &lpspi_data_##n,                  \
				  &lpspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &lpspi_dma_driver_api);

#define SPI_NXP_LPSPI_DMA_INIT(n) IF_ENABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_DMA_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_NXP_LPSPI_DMA_INIT)
