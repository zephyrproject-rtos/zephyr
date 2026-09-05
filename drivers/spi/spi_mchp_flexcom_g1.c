/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 qianfan Zhao.
 * Copyright (c) 2023 Gerson Fernando Budke.
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_flexcom_g1_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mchp_flexcom_g1, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"
#include <zephyr/cache.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#define SAM_SPI_CHIP_SELECT_COUNT	4
#define SAM_SPI_MAX_DATA_BITS		16

#define DFS(op)		DIV_ROUND_UP(SPI_WORD_SIZE_GET(op), 8)
#define DUMMY_DATA	0

/* Device constant configuration parameters */
struct spi_sam_config {
	Spi *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	bool loopback;
	uint32_t fifo_size;

#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
	const struct device *dma_dev;
	const uint32_t dma_tx_channel;
	const uint32_t dma_tx_perid;
	const uint32_t dma_rx_channel;
	const uint32_t dma_rx_perid;
#endif

	void (*irq_config_func)(const struct device *dev);
};

/* Device run time data */
struct spi_sam_data {
	struct spi_context ctx;
	uint8_t dfs;
#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
	uint8_t dma_tx_buf[CONFIG_SPI_MCHP_FLEXCOM_DMA_BUFFER_SIZE]
		__aligned(CONFIG_DCACHE_LINE_SIZE);
	uint8_t dma_rx_buf[CONFIG_SPI_MCHP_FLEXCOM_DMA_BUFFER_SIZE]
		__aligned(CONFIG_DCACHE_LINE_SIZE);
	uint32_t dma_buf_size;
	uint32_t dma_xfer_len;
	bool dma_tx_nop;
#endif
};

static void spi_sam_xfer_next(const struct device *dev);

static int spi_peripheral_to_mr_pcs(int peripheral)
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

	return pcs[peripheral];
}

static int spi_sam_configure(const struct device *dev,
			     const struct spi_config *config)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	struct spi_context *ctx = &data->ctx;
	uint32_t spi_mr = 0U, spi_csr = 0U;
	uint32_t rate, bits;
	uint16_t spi_csr_idx = spi_cs_is_gpio(config) ? 0 : config->peripheral;
	int clk_div;
	int ret;

	ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				     (clock_control_subsys_t)&cfg->clock_cfg,
				     &rate);
	if (ret != 0) {
		return ret;
	}

	if (spi_context_configured(ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER) {
		/* Peripheral mode is not implemented. */
		return -ENOTSUP;
	}

	if (spi_csr_idx > (SAM_SPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("Peripheral %d is greater than %d",
			spi_csr_idx, SAM_SPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	/* Set controller mode, disable mode fault detection, set fixed peripheral
	 * select mode.
	 */
	spi_mr |= (SPI_MR_MSTR_Msk | SPI_MR_MODFDIS_Msk);
	spi_mr |= SPI_MR_PCS(spi_peripheral_to_mr_pcs(spi_csr_idx));

	if (cfg->loopback) {
		spi_mr |= SPI_MR_LLB_Msk;
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		spi_csr |= SPI_CSR_CPOL_Msk;
	}

	if ((config->operation & SPI_MODE_CPHA) == 0U) {
		spi_csr |= SPI_CSR_NCPHA_Msk;
	}

	bits = SPI_WORD_SIZE_GET(config->operation);
	if ((bits < 8U) || (bits > SAM_SPI_MAX_DATA_BITS)) {
		LOG_ERR("Data frame bits %d not supported", bits);
		return -ENOTSUP;
	}

	spi_csr |= SPI_CSR_BITS(bits - 8);
	spi_csr |= SPI_CSR_CSAAT_Msk;

	/* Use the requested or next highest possible frequency */
	clk_div = rate / config->frequency;
	clk_div = CLAMP(clk_div, 1, UINT8_MAX);
	spi_csr |= SPI_CSR_SCBR(clk_div);

	regs->SPI_CR = SPI_CR_SPIDIS_Msk; /* Disable SPI */
	regs->SPI_CR = SPI_CR_SWRST_Msk;  /* Reset SPI */
	regs->SPI_CR = SPI_CR_FIFOEN_Msk;
	regs->SPI_MR = spi_mr;
	regs->SPI_CSR[spi_csr_idx] = spi_csr;
	regs->SPI_CR = SPI_CR_SPIEN_Msk; /* Enable SPI */

	if (!(regs->SPI_SR & SPI_SR_SPIENS_Msk)) {
		LOG_ERR("Failed to enable SPI");
		return -EIO;
	}

	ctx->config = config;
	data->dfs = DFS(config->operation);

	return 0;
}

static void spi_sam_complete(const struct device *dev, int status)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	struct spi_context *ctx = &data->ctx;

	/* De-asserted NPCS line */
	regs->SPI_CR = SPI_CR_LASTXFER_Msk;
	/* Disable all enabled interrupts */
	regs->SPI_IDR = regs->SPI_IMR;

	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, status);
}

#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
static bool tx_bufs_is_nop(const struct spi_buf_set *tx_bufs)
{
	if (tx_bufs == NULL ||
	    tx_bufs->buffers == NULL ||
	    tx_bufs->count == 0U) {
		return true;
	}

	for (size_t i = 0U; i < tx_bufs->count; i++) {
		const struct spi_buf *buf = &tx_bufs->buffers[i];

		if (buf->buf != NULL && buf->len != 0U) {
			return false;
		}
	}

	return true;
}

static bool dma_check_nop_tx(const struct device *dev,
			     const struct spi_buf_set *tx_bufs)
{
	struct spi_sam_data *data = dev->data;

	if (tx_bufs_is_nop(tx_bufs)) {
		memset(data->dma_tx_buf, DUMMY_DATA, sizeof(long long));

		sys_cache_data_flush_range(data->dma_tx_buf, sizeof(long long));
		__DSB();

		return true;
	}

	return false;
}

static void dma_callback(const struct device *dma_dev,
			 void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	const struct device *dev = user_data;
	struct spi_sam_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (status != 0) {
		LOG_ERR("DMA callback return error, status=%08x", status);
		spi_sam_complete(dev, status);

		return;
	}

	if (spi_context_rx_on(ctx)) {
		uint32_t xfer_len = data->dma_xfer_len;
		uint8_t *rx_buf = data->dma_rx_buf;

		sys_cache_data_invd_range(rx_buf, xfer_len * data->dfs);
		__DSB();

		while (spi_context_rx_on(ctx) && (xfer_len > 0U)) {
			uint32_t rx_len = MIN(xfer_len, ctx->rx_len);

			if (spi_context_rx_buf_on(ctx)) {
				memcpy(ctx->rx_buf, rx_buf, rx_len * data->dfs);
			}

			spi_context_update_rx(ctx, data->dfs, rx_len);
			rx_buf += rx_len * data->dfs;
			xfer_len -= rx_len;
		}
	}

	spi_sam_xfer_next(dev);
}

static void spi_sam_xfer_dma(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	struct spi_context *ctx = &data->ctx;
	uint32_t xfer_len, len;
	int ret;

	data->dma_xfer_len = MIN(spi_context_longest_current_buf(ctx),
				 data->dma_buf_size / data->dfs);
	xfer_len = data->dma_xfer_len;
	len = data->dma_xfer_len * data->dfs;

	struct dma_block_config tx_block_cfg = {
		.source_address = (uintptr_t)data->dma_tx_buf,
		.dest_address = (uintptr_t)&regs->SPI_TDR,
		.block_size = len,
		.source_addr_adj = (data->dma_tx_nop || !spi_context_tx_on(ctx)) ?
				   DMA_ADDR_ADJ_NO_CHANGE:
				   DMA_ADDR_ADJ_INCREMENT,
	};

	struct dma_config tx_dma_cfg = {
		.dma_slot = cfg->dma_tx_perid,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.complete_callback_en = false,
		.source_data_size = data->dfs,
		.dest_data_size = data->dfs,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.block_count = 1,
		.head_block = &tx_block_cfg,
		.user_data = NULL,
		.dma_callback = NULL,
	};

	struct dma_block_config rx_block_cfg = {
		.source_address = (uintptr_t)&regs->SPI_RDR,
		.dest_address = (uintptr_t)data->dma_rx_buf,
		.block_size = len,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
	};

	struct dma_config rx_dma_cfg = {
		.dma_slot = cfg->dma_rx_perid,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.complete_callback_en = true,
		.source_data_size = data->dfs,
		.dest_data_size = data->dfs,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.block_count = 1,
		.head_block = &rx_block_cfg,
		.user_data = (void *)dev,
		.dma_callback = dma_callback,
	};

	if (data->dma_tx_nop) {
		/* Dump TX buffers because this is a NOP TX */
		while (spi_context_tx_on(ctx) && (xfer_len > 0U)) {
			uint32_t tx_len = MIN(xfer_len, ctx->tx_len);

			spi_context_update_tx(ctx, data->dfs, tx_len);
			xfer_len -= tx_len;
		}
	} else if (spi_context_tx_on(ctx)) {
		uint8_t *tx_buf = data->dma_tx_buf;

		while (xfer_len > 0U) {
			if (spi_context_tx_on(ctx)) {
				uint32_t tx_len = MIN(xfer_len, ctx->tx_len);

				if (spi_context_tx_buf_on(ctx)) {
					memcpy(tx_buf, ctx->tx_buf, tx_len * data->dfs);
				} else {
					memset(tx_buf, DUMMY_DATA, tx_len * data->dfs);
				}

				spi_context_update_tx(ctx, data->dfs, tx_len);
				tx_buf += tx_len * data->dfs;
				xfer_len -= tx_len;
			} else {
				memset(tx_buf, DUMMY_DATA, xfer_len * data->dfs);
				xfer_len = 0;
			}
		}

		sys_cache_data_flush_range(data->dma_tx_buf, len);
		__DSB();
	} else {
		/*
		 * FIXME
		 * Use data->dfs as size doesn't work
		 * Why sizeof(long long) must be used when fixed address is enabled?
		 */
		memset(data->dma_tx_buf, DUMMY_DATA, sizeof(long long));

		sys_cache_data_flush_range(data->dma_tx_buf, sizeof(long long));
		__DSB();
	}

	/* Enable relevant interrupts */
	regs->SPI_IER = SPI_IER_OVRES_Msk;

	ret = dma_config(cfg->dma_dev, cfg->dma_rx_channel, &rx_dma_cfg);
	if (ret != 0) {
		LOG_ERR("failed to configure SPI DMA RX");
		goto err;
	}

	ret = dma_config(cfg->dma_dev, cfg->dma_tx_channel, &tx_dma_cfg);
	if (ret != 0) {
		LOG_ERR("failed to configure SPI DMA TX");
		goto err;
	}

	/* Clocking begins on tx, so start rx first */
	ret = dma_start(cfg->dma_dev, cfg->dma_rx_channel);
	if (ret != 0) {
		LOG_ERR("failed to start SPI DMA RX");
		goto err;
	}

	ret = dma_start(cfg->dma_dev, cfg->dma_tx_channel);
	if (ret != 0) {
		LOG_ERR("failed to start SPI DMA TX");
		dma_stop(cfg->dma_dev, cfg->dma_rx_channel);
	}

	return;
err:
	spi_sam_complete(dev, ret);
}
#endif

static void spi_sam_pump_fifo(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	struct spi_context *ctx = &data->ctx;
	uint32_t xfer_len = FIELD_GET(SPI_FLR_RXFL_Msk, regs->SPI_FLR);

	while (spi_context_rx_on(ctx) && (xfer_len != 0U)) {
		if (spi_context_rx_buf_on(ctx)) {
			if (data->dfs > 1U) {
				*(uint16_t *)ctx->rx_buf = (uint16_t)regs->SPI_RDR;
			} else {
				*ctx->rx_buf = (uint8_t)regs->SPI_RDR;
			}
		} else {
			(void)regs->SPI_RDR;
		}

		spi_context_update_rx(ctx, data->dfs, 1);
		xfer_len--;
	}

	if (xfer_len != 0U) {
		/* Flush RX FIFO */
		regs->SPI_CR = SPI_CR_RXFCLR_Msk;
		while (regs->SPI_FLR & SPI_FLR_RXFL_Msk);
	}
}

static void spi_sam_xfer_fifo(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	struct spi_context *ctx = &data->ctx;
	uint32_t xfer_len = MIN(spi_context_longest_current_buf(ctx),
				cfg->fifo_size);
	uint32_t tdr = 0, td_idx = 0;
	uint16_t td;

	if (regs->SPI_FLR != 0U) {
		LOG_ERR("FIFO is not empty! FLR=0x%08x\n", regs->SPI_FLR);

		/* Flush TX and RX FIFOs */
		regs->SPI_CR = SPI_CR_RXFCLR_Msk | SPI_CR_TXFCLR_Msk;
		while (regs->SPI_FLR);
	}

	regs->SPI_FMR = (regs->SPI_FMR & ~SPI_FMR_RXFTHRES_Msk) |
			SPI_FMR_RXFTHRES(xfer_len);
	(void)regs->SPI_SR;

	while (xfer_len > 0U) {
		if (spi_context_tx_on(ctx)) {
			if (spi_context_tx_buf_on(ctx)) {
				if (data->dfs > 1U) {
					td = *(uint16_t *)(ctx->tx_buf);
				} else {
					td = *ctx->tx_buf;
				}
			} else {
				td = DUMMY_DATA;
			}

			spi_context_update_tx(ctx, data->dfs, 1);
		} else {
			td = DUMMY_DATA;
		}

		tdr |= td << (td_idx++ * 16);
		if (td_idx >= 2U) {
			regs->SPI_TDR = tdr;
			tdr = 0;
			td_idx = 0;
		}

		xfer_len--;
	}

	if (td_idx >= 1U) {
		sys_write16(td, (uintptr_t)&regs->SPI_TDR);
	}

	regs->SPI_IER = SPI_IER_RXFTHF_Msk | SPI_IER_OVRES_Msk;
}

static void spi_sam_xfer_next(const struct device *dev)
{
#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
	const struct spi_sam_config *cfg = dev->config;
#endif
	struct spi_sam_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if ((!spi_context_tx_on(ctx)) &&
	    (!spi_context_rx_on(ctx))) {
		spi_sam_complete(dev, 0);
		return;
	}

#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
	if (spi_context_longest_current_buf(ctx) > cfg->fifo_size) {
		spi_sam_xfer_dma(dev);
	} else
#endif
	{
		spi_sam_xfer_fifo(dev);
	}
}

static void spi_sam_isr(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	Spi *regs = cfg->regs;
	uint32_t status, pending;

	status = regs->SPI_SR;
	pending = status & regs->SPI_IMR;
	regs->SPI_IDR = regs->SPI_IMR;

	if (pending & SPI_SR_OVRES_Msk) {
		spi_sam_complete(dev, -EIO);
		return;
	}

	if (pending & SPI_IER_RXFTHF_Msk) {
		spi_sam_pump_fifo(dev);
		spi_sam_xfer_next(dev);
	}
}

static int spi_sam_transceive(const struct device *dev,
			      const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs,
			      bool async, spi_callback_t cb, void *userdata)
{
	struct spi_sam_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	spi_context_lock(ctx, async, cb, userdata, spi_cfg);

	ret = spi_sam_configure(dev, spi_cfg);
	if (ret != 0) {
		goto done;
	}

#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
	data->dma_tx_nop = dma_check_nop_tx(dev, tx_bufs);
#endif

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(ctx, true);

	spi_sam_xfer_next(dev);

	ret = spi_context_wait_for_completion(ctx);
done:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_sam_transceive_sync(const struct device *dev,
				   const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
	return spi_sam_transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_sam_transceive_async(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    spi_callback_t cb,
				    void *userdata)
{
	return spi_sam_transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
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
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	struct spi_context *ctx = &data->ctx;
	int ret;

#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
	if (cfg->dma_dev == NULL) {
		LOG_WRN("No DMA channel found, FIFO mode will be used");
	} else {
		if (device_is_ready(cfg->dma_dev) != true) {
			LOG_ERR("DMA device is not ready!");
			return -ENODEV;
		}
	}

	data->dma_buf_size = ROUND_DOWN(CONFIG_SPI_MCHP_FLEXCOM_DMA_BUFFER_SIZE,
					sys_cache_data_line_size_get());

	sys_cache_data_invd_range(data->dma_rx_buf, data->dma_buf_size);
	__DSB();
#endif

	/* Enable SPI clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Reset Flexcom SPI controller */
	regs->SPI_CR = SPI_CR_SWRST_Msk;

	ret = spi_context_cs_configure_all(ctx);
	if (ret < 0) {
		return ret;
	}

	cfg->irq_config_func(dev);

	spi_context_unlock_unconditionally(ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static DEVICE_API(spi, spi_sam_driver_api) = {
	.transceive = spi_sam_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sam_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_sam_release,
};

#define SPI_SAM_DMA_INIT(n)							\
	.dma_dev = DEVICE_DT_GET_OR_NULL(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),	\
	.dma_tx_channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),		\
	.dma_tx_perid = DT_INST_DMAS_CELL_BY_NAME(n, tx, perid),		\
	.dma_rx_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),		\
	.dma_rx_perid = DT_INST_DMAS_CELL_BY_NAME(n, rx, perid),

#ifdef CONFIG_SPI_MCHP_FLEXCOM_DMA_DRIVEN
#define SPI_USE_DMA(n) DT_INST_DMAS_HAS_NAME(n, tx)
#else
#define SPI_USE_DMA(n) 0
#endif

#define SPI_SAM_DEFINE_CONFIG(n)						\
	static const struct spi_sam_config spi_sam_config_##n = {		\
		.regs      = (Spi *)DT_INST_REG_ADDR(n),			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),			\
		.pcfg      = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.loopback  = DT_INST_PROP(n, loopback),				\
		.fifo_size = DT_INST_PROP(n, fifo_size),			\
		COND_CODE_1(SPI_USE_DMA(n), (SPI_SAM_DMA_INIT(n)), ())		\
		.irq_config_func = &spi_sam_irq_config_func_##n,		\
	}

#define SPI_SAM_DEVICE_INIT(n)							\
	static void spi_sam_irq_config_func_##n(const struct device *dev);	\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
	SPI_SAM_DEFINE_CONFIG(n);						\
										\
	static struct spi_sam_data spi_sam_dev_data_##n = {			\
		SPI_CONTEXT_INIT_LOCK(spi_sam_dev_data_##n, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_sam_dev_data_##n, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)		\
	};									\
										\
	SPI_DEVICE_DT_INST_DEFINE(n, &spi_sam_init, NULL,			\
				  &spi_sam_dev_data_##n,			\
				  &spi_sam_config_##n, POST_KERNEL,		\
				  CONFIG_SPI_INIT_PRIORITY,			\
				  &spi_sam_driver_api);				\
										\
	static void spi_sam_irq_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    spi_sam_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM_DEVICE_INIT)
