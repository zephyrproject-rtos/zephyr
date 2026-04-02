/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gspi

#include <string.h>
#include <errno.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "clock_update.h"

LOG_MODULE_REGISTER(spi_siwx91x_gspi, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

#define GSPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE 4096
#define SPI_HIGH_BURST_FREQ_THRESHOLD_HZ      10000000

/* Warning for unsupported configurations */
#if defined(CONFIG_SPI_ASYNC) && !defined(CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA)
#warning "Silabs GSPI SPI driver ASYNC without DMA is not supported"
#endif

/* Structure for DMA configuration */
struct gspi_siwx91x_dma_channel {
	const struct device *dma_dev;
	uint8_t dma_slot;
	int chan_nb;
#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
	struct dma_block_config dma_descriptors[CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA_MAX_BLOCKS];
#endif
};

struct gspi_siwx91x_config {
	GSPI0_Type *reg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	uint8_t mosi_overrun __aligned(32);
};

struct gspi_siwx91x_data {
	struct spi_context ctx;
	struct gspi_siwx91x_dma_channel dma_rx;
	struct gspi_siwx91x_dma_channel dma_tx;
	bool use_tx_cb;
};

#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
/* Placeholder buffer for unused RX data */
static volatile uint8_t empty_buffer __aligned(4);
#endif

static bool spi_siwx91x_is_dma_enabled_instance(const struct device *dev)
{
#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
	struct gspi_siwx91x_data *data = dev->data;

	/* Ensure both TX and RX DMA devices are either present or absent */
	__ASSERT_NO_MSG(!!data->dma_tx.dma_dev == !!data->dma_rx.dma_dev);

	return data->dma_rx.dma_dev != NULL;
#else
	return false;
#endif
}

static uint32_t gspi_siwx91x_get_divider(uint32_t clock_hz, uint32_t requested_hz)
{
	uint32_t divider = DIV_ROUND_UP(clock_hz, 2 * requested_hz);
	uint32_t actual_freq = clock_hz / (2U * divider);

	if (requested_hz != actual_freq) {
		LOG_INF("Requested %u Hz, programmed %u Hz (divider=%u)",
			requested_hz, actual_freq, divider);
	}

	return divider;
}

static int gspi_siwx91x_config(const struct device *dev, const struct spi_config *spi_cfg,
			       spi_callback_t cb, void *userdata)
{
	__maybe_unused struct gspi_siwx91x_data *data = dev->data;
	const struct gspi_siwx91x_config *cfg = dev->config;
	uint32_t clock_rate;
	int ret;
	__maybe_unused int channel_filter;

	/* Validate unsupported configurations */
	if (spi_cfg->operation & (SPI_HALF_DUPLEX | SPI_CS_ACTIVE_HIGH | SPI_TRANSFER_LSB |
				  SPI_OP_MODE_SLAVE | SPI_MODE_LOOP)) {
		LOG_ERR("Unsupported configuration 0x%X!", spi_cfg->operation);
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8 &&
	    SPI_WORD_SIZE_GET(spi_cfg->operation) != 16) {
		LOG_ERR("Word size incorrect %d!", SPI_WORD_SIZE_GET(spi_cfg->operation));
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode!");
		return -ENOTSUP;
	}

	if (!!(spi_cfg->operation & SPI_MODE_CPOL) != !!(spi_cfg->operation & SPI_MODE_CPHA)) {
		LOG_ERR("Only SPI mode 0 and 3 supported!");
		return -ENOTSUP;
	}

	/* Configure clock divider based on the requested bit rate */
	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &clock_rate);
	if (ret) {
		return ret;
	}
	cfg->reg->GSPI_CLK_DIV = gspi_siwx91x_get_divider(clock_rate, spi_cfg->frequency);

	/* Configure SPI clock mode */
	if ((spi_cfg->operation & (SPI_MODE_CPOL | SPI_MODE_CPHA)) == 0) {
		cfg->reg->GSPI_BUS_MODE_b.GSPI_CLK_MODE_CSN0 = 0;
	} else {
		cfg->reg->GSPI_BUS_MODE_b.GSPI_CLK_MODE_CSN0 = 1;
	}

	/*  Update the number of Data Bits */
	cfg->reg->GSPI_WRITE_DATA2 = SPI_WORD_SIZE_GET(spi_cfg->operation);

	/* Swap the write and read data inside the GSPI controller it-self */
	cfg->reg->GSPI_CONFIG2_b.GSPI_RD_DATA_SWAP_MNL_CSN0 = 1;
	cfg->reg->GSPI_CONFIG2_b.GSPI_WR_DATA_SWAP_MNL_CSN0 = 1;

	/* Enable full-duplex mode and manual read/write */
	cfg->reg->GSPI_CONFIG1_b.SPI_FULL_DUPLEX_EN = 1;
	cfg->reg->GSPI_CONFIG1_b.GSPI_MANUAL_WR = 1;
	cfg->reg->GSPI_CONFIG1_b.GSPI_MANUAL_RD = 1;
	cfg->reg->GSPI_WRITE_DATA2_b.USE_PREV_LENGTH = 1;

	/* Configure FIFO thresholds */
	cfg->reg->GSPI_FIFO_THRLD = 0;
#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
	if (spi_siwx91x_is_dma_enabled_instance(dev)) {
		if (!device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}

		/* Release and reconfigure DMA channels */
		dma_release_channel(data->dma_rx.dma_dev, data->dma_rx.chan_nb);
		dma_release_channel(data->dma_tx.dma_dev, data->dma_tx.chan_nb);

		/* Configure RX DMA channel */
		channel_filter = data->dma_rx.chan_nb;
		data->dma_rx.chan_nb = dma_request_channel(data->dma_rx.dma_dev, &channel_filter);
		if (data->dma_rx.chan_nb != channel_filter) {
			data->dma_rx.chan_nb = channel_filter;
			return -EAGAIN;
		}

		/* Configure TX DMA channel */
		channel_filter = data->dma_tx.chan_nb;
		data->dma_tx.chan_nb = dma_request_channel(data->dma_tx.dma_dev, &channel_filter);
		if (data->dma_tx.chan_nb != channel_filter) {
			data->dma_tx.chan_nb = channel_filter;
			return -EAGAIN;
		}
	}

#ifdef CONFIG_SPI_ASYNC
	data->ctx.callback = cb;
	data->ctx.callback_data = userdata;
#endif
#endif
	data->ctx.config = spi_cfg;

	return 0;
}

#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
static void gspi_siwx91x_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				      int status)
{
	const struct device *spi_dev = (const struct device *)user_data;
	struct gspi_siwx91x_data *data = spi_dev->data;
	struct spi_context *instance_ctx = &data->ctx;

	ARG_UNUSED(dev);

	if (status >= 0 && status != DMA_STATUS_COMPLETE) {
		return;
	}

	if (status < 0) {
		dma_stop(data->dma_tx.dma_dev, data->dma_tx.chan_nb);
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.chan_nb);
	}

	spi_context_cs_control(instance_ctx, false);
	spi_context_complete(instance_ctx, spi_dev, status);
	pm_device_runtime_put_async(spi_dev, K_NO_WAIT);
}

static int gspi_siwx91x_dma_config(const struct device *dev,
				   struct gspi_siwx91x_dma_channel *channel, uint32_t block_count,
				   bool is_tx, uint8_t dfs, uint8_t burst_size)
{
	struct gspi_siwx91x_data *data = dev->data;
	struct dma_config cfg = {
		.channel_direction = is_tx ? MEMORY_TO_PERIPHERAL : PERIPHERAL_TO_MEMORY,
		.channel_priority = 1,
		.complete_callback_en = 0,
		.source_data_size = dfs,
		.dest_data_size = dfs,
		.source_burst_length = burst_size,
		.dest_burst_length = burst_size,
		.block_count = block_count,
		.head_block = channel->dma_descriptors,
		.dma_slot = channel->dma_slot,
		.dma_callback = NULL,
		.user_data = (void *)dev,
	};

	/* We normally rely on the Rx DMA callback because, due to a gpDMA issue,
	 * the last byte of a transfer is missed when completion is inferred from
	 * the Tx DMA. This problem is not visible in the Zephyr test case because
	 * the final byte there is '\0'.
	 *
	 * However, there is another gpDMA bug where the Rx DMA completes early if
	 * the Rx buffer is NULL. In that specific case, we must instead rely on
	 * the Tx DMA callback to signal completion.
	 *
	 * Despite this conditional handling, the logic works correctly on
	 * non-buggy DMA engines (for example, uDMA).
	 */
	if (data->use_tx_cb == is_tx) {
		cfg.dma_callback = &gspi_siwx91x_dma_callback;
	}

	return dma_config(channel->dma_dev, channel->chan_nb, &cfg);
}

static uint32_t gspi_siwx91x_fill_desc(const struct gspi_siwx91x_config *cfg,
				       struct dma_block_config *new_blk_cfg, uint8_t *buffer,
				       size_t requested_transaction_size, bool is_tx)
{

	/* Set-up source and destination address with increment behavior */
	if (is_tx) {
		new_blk_cfg->dest_address = (uint32_t)&cfg->reg->GSPI_WRITE_FIFO;
		new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->source_address = (uint32_t)buffer;
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means sending dummy byte */
			new_blk_cfg->source_address = (uint32_t)&(cfg->mosi_overrun);
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	} else {
		new_blk_cfg->source_address = (uint32_t)&cfg->reg->GSPI_READ_FIFO;
		new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->dest_address = (uint32_t)buffer;
			new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means rx to null byte */
			new_blk_cfg->dest_address = (uint32_t)&empty_buffer;
			new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	/* The underlying DMA can sent a bit less than 4k of data depending of the data size of and
	 * the burst length. We avoid complex computation, 32 bytes fits all the cases.
	 */
	new_blk_cfg->block_size = MIN(requested_transaction_size,
				      GSPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE - 32);
	return new_blk_cfg->block_size;
}

struct dma_block_config *gspi_siwx91x_fill_data_desc(const struct device *spi_dev,
						     struct dma_block_config *desc,
						     const struct spi_buf buffers[],
						     int buffer_count, size_t transaction_len,
						     bool is_tx)
{
	__ASSERT(transaction_len > 0, "Not supported");

	const struct gspi_siwx91x_config *cfg = spi_dev->config;
	struct gspi_siwx91x_data *data = spi_dev->data;
	size_t offset = 0;
	int i = 0;
	uint8_t *buffer = NULL;

	while (i != buffer_count) {
		if (!buffers[i].len) {
			i++;
			continue;
		}

		if (!desc) {
			return NULL;
		}

		/* Calculate the buffer pointer with the current offset */
		buffer = buffers[i].buf ? (uint8_t *)buffers[i].buf + offset : NULL;
		/* Fill the descriptor with the buffer data and update the offset */
		offset += gspi_siwx91x_fill_desc(cfg, desc, buffer, buffers[i].len - offset, is_tx);
		/* If the end of the current buffer is reached, move to the next buffer */
		if (offset == buffers[i].len) {
			transaction_len -= offset;
			offset = 0;
			i++;
		}

		if (transaction_len) {
			desc = desc->next_block;
		}
	}

	if (transaction_len == 0 && !is_tx && buffer == NULL) {
		/* Last Rx buffer is NULL */
		data->use_tx_cb = true;
	}

	/* Process any remaining transaction length with NULL buffer data */
	while (transaction_len) {
		if (!desc) {
			return NULL;
		}

		transaction_len -= gspi_siwx91x_fill_desc(cfg, desc, NULL, transaction_len, is_tx);
		if (transaction_len) {
			desc = desc->next_block;
		}
	}

	/* Mark the end of the descriptor chain */
	desc->next_block = NULL;
	return desc;
}

static void gspi_siwx91x_reset_desc(struct gspi_siwx91x_dma_channel *channel)
{
	int i;

	memset(channel->dma_descriptors, 0, sizeof(channel->dma_descriptors));

	for (i = 0; i < ARRAY_SIZE(channel->dma_descriptors) - 1; i++) {
		channel->dma_descriptors[i].next_block = &channel->dma_descriptors[i + 1];
	}
}

static int gspi_siwx91x_prepare_dma_channel(const struct device *spi_dev,
					    const struct spi_buf *buffer, size_t buffer_count,
					    struct gspi_siwx91x_dma_channel *channel,
					    size_t padded_transaction_size, bool is_tx,
					    uint8_t burst_size)
{
	struct gspi_siwx91x_data *data = spi_dev->data;
	const uint8_t dfs = SPI_WORD_SIZE_GET(data->ctx.config->operation) / 8;
	struct dma_block_config *desc;
	int ret = 0;

	gspi_siwx91x_reset_desc(channel);

	desc = gspi_siwx91x_fill_data_desc(spi_dev, channel->dma_descriptors, buffer, buffer_count,
					   padded_transaction_size, is_tx);
	if (!desc) {
		return -ENOMEM;
	}

	ret = gspi_siwx91x_dma_config(spi_dev, channel,
				      ARRAY_INDEX(channel->dma_descriptors, desc) + 1, is_tx, dfs,
				      burst_size);
	return ret;
}

static int gspi_siwx91x_prepare_dma_transaction(const struct device *dev,
						size_t padded_transaction_size, uint8_t burst_size)
{
	int ret;
	struct gspi_siwx91x_data *data = dev->data;

	if (padded_transaction_size == 0) {
		return 0;
	}

	data->use_tx_cb = false;

	ret = gspi_siwx91x_prepare_dma_channel(dev, data->ctx.current_rx, data->ctx.rx_count,
					       &data->dma_rx, padded_transaction_size, false,
					       burst_size);

	if (ret) {
		return ret;
	}

	ret = gspi_siwx91x_prepare_dma_channel(dev, data->ctx.current_tx, data->ctx.tx_count,
					       &data->dma_tx, padded_transaction_size, true,
					       burst_size);

	return ret;
}

static size_t gspi_siwx91x_longest_transfer_size(struct spi_context *instance_ctx)
{
	uint32_t tx_transfer_size = spi_context_total_tx_len(instance_ctx);
	uint32_t rx_transfer_size = spi_context_total_rx_len(instance_ctx);

	return MAX(tx_transfer_size, rx_transfer_size);
}

static int gspi_siwx91x_burst_size_buf(const struct spi_buf *dma_spi_buf)
{
	int burst_len = 4;

	if (!dma_spi_buf->buf || !dma_spi_buf->len) {
		return burst_len;
	}

	burst_len = MIN(burst_len, BIT(find_lsb_set((uint32_t)dma_spi_buf->buf) - 1));
	burst_len = MIN(burst_len, BIT(find_lsb_set(dma_spi_buf->len) - 1));

	return burst_len;
}

static int gspi_siwx91x_burst_size(struct spi_context *ctx)
{
	int burst_len = 4;

	for (int i = 0; i < ctx->tx_count; i++) {
		burst_len = MIN(burst_len, gspi_siwx91x_burst_size_buf(ctx->current_tx + i));
	}

	for (int i = 0; i < ctx->rx_count; i++) {
		burst_len = MIN(burst_len, gspi_siwx91x_burst_size_buf(ctx->current_rx + i));
	}

	return burst_len;
}

static void gspi_siwx91x_gspi_fifo_reset_sync(uint32_t frequency)
{
	int loops = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / frequency;

	/* GSPI FIFO reset requires the RESET bits to be held high
	 * for at least one GSPI bus clock cycle.
	 * Since there is no explicit hardware status indicating
	 * completion of the FIFO reset, insert a short, frequency-
	 * dependent delay to guarantee the minimum reset pulse width.
	 */
	while (loops-- > 0) {
		arch_nop();
	}
}
#endif /* CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA */

static int gspi_siwx91x_transceive_dma(const struct device *dev, const struct spi_config *config)
{
#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
	const struct gspi_siwx91x_config *cfg = dev->config;
	struct gspi_siwx91x_data *data = dev->data;
	const struct device *dma_dev = data->dma_rx.dma_dev;
	struct spi_context *ctx = &data->ctx;
	size_t padded_transaction_size = gspi_siwx91x_longest_transfer_size(ctx);
	uint8_t burst_size = 1;
	int ret = 0;

	if (padded_transaction_size == 0) {
		return -EINVAL;
	}

	if (config->frequency >= SPI_HIGH_BURST_FREQ_THRESHOLD_HZ &&
	    data->dma_rx.dma_slot != 0xFF && data->dma_tx.dma_slot != 0xFF) {
		/* NOTE: This condition ensures that high burst rates are only used with GPDMA.
		 *
		 * GPDMA (General-Purpose DMA) supports higher burst rates and operates at higher
		 * frequencies, unlike UDMA, which does not handle such speeds reliably.
		 * Therefore, the DMA slots are validated to ensure that the active DMA channels
		 * belong to GPDMA before enabling higher burst rates.
		 *
		 * Currently, DMA flow control (DMA_FLOW_CTRL) is not functioning correctly for
		 * memory-to-peripheral and peripheral-to-memory transfers. As a result, at lower
		 * SPI clock frequencies, GPDMA may read/write FIFOs at a much higher rate than the
		 * SPI peripheral, causing synchronization issues. However, at higher SPI clock
		 * frequencies (≥ 10 MHz), this timing mismatch is negligible, and higher burst
		 * rates operate as expected.
		 *
		 * In summary, high burst rates are safely enabled only for SPI transfers running
		 * at or above 10 MHz when using GPDMA.
		 */
		burst_size = gspi_siwx91x_burst_size(ctx);
	}

	cfg->reg->GSPI_FIFO_THRLD_b.RFIFO_RESET = 1;
	cfg->reg->GSPI_FIFO_THRLD_b.WFIFO_RESET = 1;
	/* Hold FIFO reset asserted for at least one GSPI clock cycle */
	gspi_siwx91x_gspi_fifo_reset_sync(config->frequency);
	cfg->reg->GSPI_FIFO_THRLD = 0;
	cfg->reg->GSPI_FIFO_THRLD_b.FIFO_AEMPTY_THRLD = burst_size - 1;
	cfg->reg->GSPI_FIFO_THRLD_b.FIFO_AFULL_THRLD = burst_size - 1;

	ret = gspi_siwx91x_prepare_dma_transaction(dev, padded_transaction_size, burst_size);
	if (ret) {
		return ret;
	}

	spi_context_cs_control(ctx, true);

	ret = dma_start(dma_dev, data->dma_rx.chan_nb);
	if (ret) {
		return ret;
	}

	ret = dma_start(dma_dev, data->dma_tx.chan_nb);
	if (ret) {
		return ret;
	}

	/* Note: spi_context_wait_for_completion() does not block if ctx->asynchronous is set */
	ret = spi_context_wait_for_completion(&data->ctx);
	if (ret < 0) {
		goto force_transaction_close;
	}

	/* Successful transaction. DMA transfer done interrupt ended the transaction. */
	return 0;

force_transaction_close:
	dma_stop(data->dma_rx.dma_dev, data->dma_rx.chan_nb);
	dma_stop(data->dma_tx.dma_dev, data->dma_tx.chan_nb);
	spi_context_cs_control(ctx, false);
	return ret;
#else
	return -ENOTSUP;
#endif
}

static inline uint16_t gspi_siwx91x_next_tx(struct gspi_siwx91x_data *data, const uint8_t dfs)
{
	uint16_t tx_frame = 0;

	if (spi_context_tx_buf_on(&data->ctx)) {
		if (dfs == 1) {
			tx_frame = *((uint8_t *)(data->ctx.tx_buf));
		} else {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
		}
	}

	return tx_frame;
}

static void gspi_siwx91x_send(const struct gspi_siwx91x_config *cfg, uint32_t data)
{
	cfg->reg->GSPI_WRITE_FIFO[0] = data;
}

static uint32_t gspi_siwx91x_receive(const struct gspi_siwx91x_config *cfg)
{
	return cfg->reg->GSPI_READ_FIFO[0];
}

static int gspi_siwx91x_shift_frames(const struct device *dev)
{
	const struct gspi_siwx91x_config *cfg = dev->config;
	struct gspi_siwx91x_data *data = dev->data;
	const uint8_t dfs = SPI_WORD_SIZE_GET(data->ctx.config->operation) / 8;
	uint16_t tx_frame;
	uint16_t rx_frame;

	tx_frame = gspi_siwx91x_next_tx(data, dfs);
	gspi_siwx91x_send(cfg, tx_frame);

	while (cfg->reg->GSPI_STATUS_b.GSPI_BUSY) {
	}

	spi_context_update_tx(&data->ctx, dfs, 1);

	rx_frame = (uint16_t)gspi_siwx91x_receive(cfg);

	if (spi_context_rx_buf_on(&data->ctx)) {
		if (dfs == 1) {
			/* Store the received frame as a byte */
			UNALIGNED_PUT((uint8_t)rx_frame, (uint8_t *)data->ctx.rx_buf);
		} else {
			/* Store the received frame as a word */
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
		}
	}

	spi_context_update_rx(&data->ctx, dfs, 1);

	return 0;
}

static bool gspi_siwx91x_transfer_ongoing(struct gspi_siwx91x_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int gspi_siwx91x_transceive_polling_sync(const struct device *dev, struct spi_context *ctx)
{
	struct gspi_siwx91x_data *data = dev->data;
	int ret = 0;

	spi_context_cs_control(&data->ctx, true);

	while (!ret && gspi_siwx91x_transfer_ongoing(data)) {
		ret = gspi_siwx91x_shift_frames(dev);
	}

	spi_context_cs_control(&data->ctx, false);
	spi_context_complete(&data->ctx, dev, 0);
	return 0;
}

static int gspi_siwx91x_transceive(const struct device *dev, const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs, bool asynchronous,
				   spi_callback_t cb, void *userdata)
{
	struct gspi_siwx91x_data *data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	if (!spi_siwx91x_is_dma_enabled_instance(dev) && asynchronous) {
		ret = -ENOTSUP;
		goto pm;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);
	if (!spi_context_configured(&data->ctx, config)) {
		ret = gspi_siwx91x_config(dev, config, cb, userdata);
		if (ret) {
			goto context;
		}
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs,
				  SPI_WORD_SIZE_GET(config->operation) / 8);

	if (spi_siwx91x_is_dma_enabled_instance(dev)) {
		ret = gspi_siwx91x_transceive_dma(dev, config);
	} else {
		ret = gspi_siwx91x_transceive_polling_sync(dev, &data->ctx);
	}

	if (spi_siwx91x_is_dma_enabled_instance(dev) && !ret) {
		/* pm_device_runtime_put(dev) is called from the dma callback */
		spi_context_release(&data->ctx, ret);
		return ret;
	}

context:
	spi_context_release(&data->ctx, ret);
pm:
	pm_device_runtime_put(dev);
	return ret;

}

static int gspi_siwx91x_transceive_sync(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	return gspi_siwx91x_transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int gspi_siwx91x_transceive_async(const struct device *dev, const struct spi_config *config,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					 void *userdata)
{
	return gspi_siwx91x_transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int gspi_siwx91x_release(const struct device *dev, const struct spi_config *config)
{
	struct gspi_siwx91x_data *data = dev->data;

	if (spi_context_configured(&data->ctx, config)) {
		spi_context_unlock_unconditionally(&data->ctx);
	}

	return 0;
}

static int gspi_siwx91x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct gspi_siwx91x_config *cfg = dev->config;
	struct gspi_siwx91x_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}

		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}

		ret = spi_context_cs_configure_all(&data->ctx);
		if (ret) {
			return ret;
		}

		spi_context_unlock_unconditionally(&data->ctx);

		cfg->reg->GSPI_BUS_MODE_b.SPI_HIGH_PERFORMANCE_EN = 1;
		cfg->reg->GSPI_CONFIG1_b.GSPI_MANUAL_CSN = 0;
		data->ctx.config = NULL;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = clock_control_off(cfg->clock_dev, cfg->clock_subsys);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gspi_siwx91x_init(const struct device *dev)
{
	return pm_device_driver_init(dev, gspi_siwx91x_pm_action);
}

static DEVICE_API(spi, gspi_siwx91x_driver_api) = {
	.transceive = gspi_siwx91x_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = gspi_siwx91x_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = gspi_siwx91x_release,
};

#ifdef CONFIG_SPI_SILABS_SIWX91X_GSPI_DMA
#define SPI_SILABS_SIWX91X_GSPI_DMA_CHANNEL_INIT(index, dir)                                       \
	.dma_##dir = {                                                                             \
		.chan_nb = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                         \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                   \
		.dma_slot = DT_DMAS_CELL_BY_NAME_OR(DT_DRV_INST(index), dir, slot, 0xFF),          \
	},
#define SPI_SILABS_SIWX91X_GSPI_DMA_CHANNEL(index, dir)                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, dmas),                                            \
		    (SPI_SILABS_SIWX91X_GSPI_DMA_CHANNEL_INIT(index, dir)), ())
#else
#define SPI_SILABS_SIWX91X_GSPI_DMA_CHANNEL(index, dir)
#endif

#define SIWX91X_GSPI_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct gspi_siwx91x_data gspi_data_##inst = {                                       \
		SPI_CONTEXT_INIT_LOCK(gspi_data_##inst, ctx),                                      \
		SPI_CONTEXT_INIT_SYNC(gspi_data_##inst, ctx),                                      \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)                            \
		SPI_SILABS_SIWX91X_GSPI_DMA_CHANNEL(inst, rx)                                      \
		SPI_SILABS_SIWX91X_GSPI_DMA_CHANNEL(inst, tx)                                      \
	};                                                                                         \
	static const struct gspi_siwx91x_config gspi_config_##inst = {                             \
		.reg = (GSPI0_Type *)DT_INST_REG_ADDR(inst),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.mosi_overrun = (uint8_t)SPI_MOSI_OVERRUN_DT(inst),                                \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, gspi_siwx91x_pm_action);                                    \
	DEVICE_DT_INST_DEFINE(inst, &gspi_siwx91x_init, PM_DEVICE_DT_INST_GET(inst),               \
			      &gspi_data_##inst, &gspi_config_##inst, POST_KERNEL,                 \
			      CONFIG_SPI_INIT_PRIORITY, &gspi_siwx91x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_GSPI_INIT)
