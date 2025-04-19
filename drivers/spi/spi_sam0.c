/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT atmel_sam0_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sam0);

/* clang-format off */

#include "spi_context.h"
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#include "spi_sam0.h"

#define CTRLA_MODE_SPI_MASTER 0x3

/* Device constant configuration parameters */
struct spi_sam0_config {
	uintptr_t regs;
	uint32_t pads;
	const struct pinctrl_dev_config *pcfg;

	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint32_t gclk_gen;
	uint16_t gclk_id;

#ifdef CONFIG_SPI_ASYNC
	const struct device *dma_dev;
	uint8_t tx_dma_request;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_request;
	uint8_t rx_dma_channel;
#endif
};

/* Device run time data */
struct spi_sam0_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_ASYNC
	const struct device *dev;
	uint32_t dma_segment_len;
#endif
};

static void wait_synchronization(uintptr_t regs)
{
#ifndef CONFIG_SOC_SERIES_SAMD20
	/* SYNCBUSY is a register */
	while ((sys_read32(regs + SYNCBUSY_OFFSET) & SYNCBUSY_MASK) != 0) {
	}
#else
	/* SYNCBUSY is a bit */
	while (sys_test_bit(regs + STATUS_OFFSET, STATUS_SYNCBUSY_BIT)) {
	}
#endif
}

static int spi_sam0_configure(const struct device *dev,
			      const struct spi_config *config)
{
	const struct spi_sam0_config *cfg = dev->config;
	struct spi_sam0_data *data = dev->data;
	uintptr_t regs = cfg->regs;
	uint32_t ctrla = 0;
	uint32_t ctrlb = 0;
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

	ctrla = CTRLA_MODE(CTRLA_MODE_SPI_MASTER);

	if ((config->operation & SPI_TRANSFER_LSB) != 0U) {
		WRITE_BIT(ctrla, CTRLA_DORD_BIT, 1);
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		WRITE_BIT(ctrla, CTRLA_CPOL_BIT, 1);
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		WRITE_BIT(ctrla, CTRLA_CPHA_BIT, 1);
	}

	ctrla |= cfg->pads;

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		/* Put MISO and MOSI on the same pad */
		ctrla &= ~(CTRLA_DOPO_MASK | CTRLA_DIPO_MASK);
	}

	WRITE_BIT(ctrla, CTRLA_ENABLE_BIT, 1);
	WRITE_BIT(ctrlb, CTRLB_RXEN_BIT, 1);

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	}

	/* 8 bits per transfer */
	ctrlb &= ~CTRLB_CHSIZE_MASK;

	/* Use the requested or next highest possible frequency */
	div = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / config->frequency) / 2U - 1;
	div = CLAMP(div, 0, UINT8_MAX);

	/* Update the configuration only if it has changed */
	if (sys_read32(regs + CTRLA_OFFSET) != ctrla ||
			sys_read32(regs + CTRLB_OFFSET) != ctrlb ||
			sys_read8(regs + BAUD_OFFSET) != div) {
		sys_clear_bit(regs + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
		wait_synchronization(regs);

		sys_write32(ctrlb, regs + CTRLB_OFFSET);
		wait_synchronization(regs);
		sys_write8(div, regs + BAUD_OFFSET);
		wait_synchronization(regs);
		sys_write32(ctrla, regs + CTRLA_OFFSET);
		wait_synchronization(regs);
	}

	data->ctx.config = config;

	return 0;
}

static bool spi_sam0_transfer_ongoing(struct spi_sam0_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void spi_sam0_shift_master(uintptr_t regs, struct spi_sam0_data *data)
{
	uint8_t tx;
	uint8_t rx;

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx = *(uint8_t *)(data->ctx.tx_buf);
	} else {
		tx = 0U;
	}

	while (!(sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_DRE)) {
	}

	sys_write8(tx, regs + DATA_OFFSET);
	spi_context_update_tx(&data->ctx, 1, 1);

	while (!(sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC)) {
	}

	rx = sys_read8(regs + DATA_OFFSET);

	if (spi_context_rx_buf_on(&data->ctx)) {
		*data->ctx.rx_buf = rx;
	}
	spi_context_update_rx(&data->ctx, 1, 1);
}

/* Finish any ongoing writes and drop any remaining read data */
static void spi_sam0_finish(uintptr_t regs)
{
	while (!(sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_TXC)) {
	}

	while (sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC) {
		(void)sys_read8(regs + DATA_OFFSET);
	}
}

/* Fast path that transmits a buf */
static void spi_sam0_fast_tx(uintptr_t regs, const struct spi_buf *tx_buf)
{
	const uint8_t *p = tx_buf->buf;
	const uint8_t *pend = (uint8_t *)tx_buf->buf + tx_buf->len;
	uint8_t ch;

	while (p != pend) {
		ch = *p++;

		while (!(sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_DRE)) {
		}

		sys_write8(ch, regs + DATA_OFFSET);
	}

	spi_sam0_finish(regs);
}

/* Fast path that reads into a buf */
static void spi_sam0_fast_rx(uintptr_t regs, const struct spi_buf *rx_buf)
{
	uint8_t *rx = rx_buf->buf;
	int len = rx_buf->len;

	if (len <= 0) {
		return;
	}

	while (len) {
		/* Send the next byte */
		sys_write8(0, regs + DATA_OFFSET);
		len--;

		/* Wait for completion, and read */
		while (!(sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC)) {
		}
		*rx++ = sys_read8(regs + DATA_OFFSET);
	}

	spi_sam0_finish(regs);
}

/* Fast path that writes and reads bufs of the same length */
static void spi_sam0_fast_txrx(uintptr_t regs,
			       const struct spi_buf *tx_buf,
			       const struct spi_buf *rx_buf)
{
	const uint8_t *tx = tx_buf->buf;
	const uint8_t *txend = (uint8_t *)tx_buf->buf + tx_buf->len;
	uint8_t *rx = rx_buf->buf;
	size_t len = rx_buf->len;

	if (len == 0) {
		return;
	}

	while (tx != txend) {
		/* Send the next byte */
		sys_write8(*tx++, regs + DATA_OFFSET);

		/* Wait for completion, and read */
		while (!(sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC)) {
		}
		*rx++ = sys_read8(regs + DATA_OFFSET);
	}

	spi_sam0_finish(regs);
}

/* Fast path where every overlapping tx and rx buffer is the same length */
static void spi_sam0_fast_transceive(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam0_config *cfg = dev->config;
	size_t tx_count = 0;
	size_t rx_count = 0;
	uintptr_t regs = cfg->regs;
	const struct spi_buf *tx = NULL;
	const struct spi_buf *rx = NULL;

	if (tx_bufs) {
		tx = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs) {
		rx = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	} else {
		rx = NULL;
	}

	while (tx_count != 0 && rx_count != 0) {
		if (tx->buf == NULL) {
			spi_sam0_fast_rx(regs, rx);
		} else if (rx->buf == NULL) {
			spi_sam0_fast_tx(regs, tx);
		} else {
			spi_sam0_fast_txrx(regs, tx, rx);
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	for (; tx_count != 0; tx_count--) {
		spi_sam0_fast_tx(regs, tx++);
	}

	for (; rx_count != 0; rx_count--) {
		spi_sam0_fast_rx(regs, rx++);
	}
}

/* Returns true if the request is suitable for the fast
 * path. Specifically, the bufs are a sequence of:
 *
 * - Zero or more RX and TX buf pairs where each is the same length.
 * - Zero or more trailing RX only bufs
 * - Zero or more trailing TX only bufs
 */
static bool spi_sam0_is_regular(const struct spi_buf_set *tx_bufs,
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

static int spi_sam0_transceive(const struct device *dev,
			       const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam0_config *cfg = dev->config;
	struct spi_sam0_data *data = dev->data;
	uintptr_t regs = cfg->regs;
	int err;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	err = spi_sam0_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_cs_control(&data->ctx, true);

	/* This driver special cases the common send only, receive
	 * only, and transmit then receive operations.	This special
	 * casing is 4x faster than the spi_context() routines
	 * and allows the transmit and receive to be interleaved.
	 */
	if (spi_sam0_is_regular(tx_bufs, rx_bufs)) {
		spi_sam0_fast_transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

		do {
			spi_sam0_shift_master(regs, data);
		} while (spi_sam0_transfer_ongoing(data));
	}

	spi_context_cs_control(&data->ctx, false);

done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int spi_sam0_transceive_sync(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_sam0_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC

static void spi_sam0_dma_rx_done(const struct device *dma_dev, void *arg,
				 uint32_t id, int error_code);

static int spi_sam0_dma_rx_load(const struct device *dev, uint8_t *buf,
				size_t len)
{
	const struct spi_sam0_config *cfg = dev->config;
	struct spi_sam0_data *data = dev->data;
	uintptr_t regs = cfg->regs;
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_blk = { 0 };
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.user_data = data;
	dma_cfg.dma_callback = spi_sam0_dma_rx_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->rx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.dest_address = (uint32_t)buf;
	} else {
		static uint8_t dummy;

		dma_blk.dest_address = (uint32_t)&dummy;
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.source_address = (uint32_t)(regs + DATA_OFFSET);
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->dma_dev, cfg->rx_dma_channel,
			    &dma_cfg);
	if (retval != 0) {
		return retval;
	}

	return dma_start(cfg->dma_dev, cfg->rx_dma_channel);
}

static int spi_sam0_dma_tx_load(const struct device *dev, const uint8_t *buf,
				size_t len)
{
	const struct spi_sam0_config *cfg = dev->config;
	uintptr_t regs = cfg->regs;
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_blk = { 0 };
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->tx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.source_address = (uint32_t)buf;
	} else {
		static const uint8_t dummy;

		dma_blk.source_address = (uint32_t)&dummy;
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.dest_address = (uint32_t)(regs + DATA_OFFSET);
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->dma_dev, cfg->tx_dma_channel,
			    &dma_cfg);

	if (retval != 0) {
		return retval;
	}

	return dma_start(cfg->dma_dev, cfg->tx_dma_channel);
}

static bool spi_sam0_dma_advance_segment(const struct device *dev)
{
	struct spi_sam0_data *data = dev->data;
	uint32_t segment_len;

	/* Pick the shorter buffer of ones that have an actual length */
	if (data->ctx.rx_len != 0) {
		segment_len = data->ctx.rx_len;
		if (data->ctx.tx_len != 0) {
			segment_len = MIN(segment_len, data->ctx.tx_len);
		}
	} else {
		segment_len = data->ctx.tx_len;
	}

	if (segment_len == 0) {
		return false;
	}

	segment_len = MIN(segment_len, 65535);

	data->dma_segment_len = segment_len;
	return true;
}

static int spi_sam0_dma_advance_buffers(const struct device *dev)
{
	struct spi_sam0_data *data = dev->data;
	int retval;

	if (data->dma_segment_len == 0) {
		return -EINVAL;
	}

	/* Load receive first, so it can accept transmit data */
	if (data->ctx.rx_len) {
		retval = spi_sam0_dma_rx_load(dev, data->ctx.rx_buf,
					      data->dma_segment_len);
	} else {
		retval = spi_sam0_dma_rx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	/* Now load the transmit, which starts the actual bus clocking */
	if (data->ctx.tx_len) {
		retval = spi_sam0_dma_tx_load(dev, data->ctx.tx_buf,
					      data->dma_segment_len);
	} else {
		retval = spi_sam0_dma_tx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	return 0;
}

static void spi_sam0_dma_rx_done(const struct device *dma_dev, void *arg,
				 uint32_t id, int error_code)
{
	struct spi_sam0_data *data = arg;
	const struct device *dev = data->dev;
	const struct spi_sam0_config *cfg = dev->config;
	int retval;

	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	spi_context_update_tx(&data->ctx, 1, data->dma_segment_len);
	spi_context_update_rx(&data->ctx, 1, data->dma_segment_len);

	if (!spi_sam0_dma_advance_segment(dev)) {
		/* Done */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return;
	}

	retval = spi_sam0_dma_advance_buffers(dev);
	if (retval != 0) {
		dma_stop(cfg->dma_dev, cfg->tx_dma_channel);
		dma_stop(cfg->dma_dev, cfg->rx_dma_channel);
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, retval);
		return;
	}
}


static int spi_sam0_transceive_async(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	const struct spi_sam0_config *cfg = dev->config;
	struct spi_sam0_data *data = dev->data;
	int retval;

	/*
	 * Transmit clocks the output and we use receive to determine when
	 * the transmit is done, so we always need both
	 */
	if (cfg->tx_dma_channel == 0xFF || cfg->rx_dma_channel == 0xFF) {
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, true, cb, userdata, config);

	retval = spi_sam0_configure(dev, config);
	if (retval != 0) {
		goto err_unlock;
	}

	spi_context_cs_control(&data->ctx, true);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_sam0_dma_advance_segment(dev);
	retval = spi_sam0_dma_advance_buffers(dev);
	if (retval != 0) {
		goto err_cs;
	}

	return 0;

err_cs:
	dma_stop(cfg->dma_dev, cfg->tx_dma_channel);
	dma_stop(cfg->dma_dev, cfg->rx_dma_channel);

	spi_context_cs_control(&data->ctx, false);

err_unlock:
	spi_context_release(&data->ctx, retval);
	return retval;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_sam0_release(const struct device *dev,
			    const struct spi_config *config)
{
	struct spi_sam0_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_sam0_init(const struct device *dev)
{
	int err;
	const struct spi_sam0_config *cfg = dev->config;
	const uintptr_t gclk = DT_REG_ADDR(DT_INST(0, atmel_sam0_gclk));
	struct spi_sam0_data *data = dev->data;
	uintptr_t regs = cfg->regs;

	*cfg->mclk |= cfg->mclk_mask;

#if !defined(CONFIG_SOC_SERIES_SAMD20) && !defined(CONFIG_SOC_SERIES_SAMD21) &&                    \
	!defined(CONFIG_SOC_SERIES_SAMR21)

	sys_write32(PCHCTRL_CHEN | PCHCTRL_GEN(cfg->gclk_gen),
		    gclk + PCHCTRL_OFFSET + (4 * cfg->gclk_id));
#else
	sys_write16(CLKCTRL_CLKEN | CLKCTRL_GEN(cfg->gclk_gen) | CLKCTRL_ID(cfg->gclk_id),
		    gclk + CLKCTRL_OFFSET);
#endif

	/* Disable all SPI interrupts */
	sys_write8(INTENCLR_MASK, regs + INTENCLR_OFFSET);
	wait_synchronization(regs);

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_SPI_ASYNC
	if (!device_is_ready(cfg->dma_dev)) {
		return -ENODEV;
	}
	data->dev = dev;
#endif

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

static DEVICE_API(spi, spi_sam0_driver_api) = {
	.transceive = spi_sam0_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sam0_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_sam0_release,
};

#if CONFIG_SPI_ASYNC
#define SPI_SAM0_DMA_CHANNELS(n)					\
	.dma_dev = DEVICE_DT_GET(ATMEL_SAM0_DT_INST_DMA_CTLR(n, tx)),	\
	.tx_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, tx),	\
	.tx_dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, tx),	\
	.rx_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, rx),	\
	.rx_dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, rx),
#else
#define SPI_SAM0_DMA_CHANNELS(n)
#endif

#define SPI_SAM0_SERCOM_PADS(n) \
	CTRLA_DIPO(DT_INST_PROP(n, dipo)) | \
	CTRLA_DOPO(DT_INST_PROP(n, dopo))

#ifndef ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET
#define ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, cell)		\
	(volatile uint32_t *)						\
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, cell)) +	\
	 DT_INST_CLOCKS_CELL_BY_NAME(n, cell, offset))
#endif

#ifndef ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET
#define ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, pm)))
#endif

#ifndef ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK
#define ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, cell)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, cell))),	\
		(BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, pm, cell))))
#endif

#define SPI_SAM0_DEFINE_CONFIG(n)					\
static const struct spi_sam0_config spi_sam0_config_##n = {		\
	.regs = DT_INST_REG_ADDR(n),					\
	.gclk_gen = DT_PHA_BY_NAME(DT_DRV_INST(n), atmel_assigned_clocks, gclk, gen),	\
	.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, id),		\
	.mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n),		\
	.mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, bit),	\
	.pads = SPI_SAM0_SERCOM_PADS(n),				\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	SPI_SAM0_DMA_CHANNELS(n)					\
}

#define SPI_SAM0_DEVICE_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	SPI_SAM0_DEFINE_CONFIG(n);					\
	static struct spi_sam0_data spi_sam0_dev_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_sam0_dev_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_sam0_dev_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
	SPI_DEVICE_DT_INST_DEFINE(n, spi_sam0_init, NULL,		\
			    &spi_sam0_dev_data_##n,			\
			    &spi_sam0_config_##n, POST_KERNEL,		\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &spi_sam0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM0_DEVICE_INIT)

/* clang-format on */
