/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 qianfan Zhao.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sam);

#include "spi_context.h"
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define SAM_SPI_CHIP_SELECT_COUNT			4

/* Number of bytes in transfer before using DMA if available */
#define SAM_SPI_DMA_THRESHOLD				32

/* Device constant configuration parameters */
struct spi_sam_config {
	Spi *regs;
	uint32_t periph_id;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
	bool loopback;

#ifdef CONFIG_SPI_SAM_DMA
	const struct device *dma_dev;
	const uint32_t dma_tx_channel;
	const uint32_t dma_tx_perid;
	const uint32_t dma_rx_channel;
	const uint32_t dma_rx_perid;
#endif /* CONFIG_SPI_SAM_DMA */
};

/* Device run time data */
struct spi_sam_data {
	struct spi_context ctx;

#ifdef CONFIG_SPI_SAM_DMA
	uint32_t chunk_size;
#endif /* CONFIG_SPI_SAM_DMA */
};

static void spi_sam_write(const struct device *dev);

static int spi_slave_to_mr_pcs(int slave)
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

	return pcs[slave];
}

static int spi_sam_configure(const struct device *dev,
			     const struct spi_config *config)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	uint32_t spi_mr = 0U, spi_csr = 0U;
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

	if (config->slave > (SAM_SPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("Slave %d is greater than %d",
			config->slave, SAM_SPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	/* Set master mode, disable mode fault detection, set fixed peripheral
	 * select mode.
	 */
	spi_mr |= (SPI_MR_MSTR | SPI_MR_MODFDIS);
	spi_mr |= SPI_MR_PCS(spi_slave_to_mr_pcs(config->slave));

	if (cfg->loopback) {
		spi_mr |= SPI_MR_LLB;
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		spi_csr |= SPI_CSR_CPOL;
	}

	if ((config->operation & SPI_MODE_CPHA) == 0U) {
		spi_csr |= SPI_CSR_NCPHA;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	} else {
		spi_csr |= SPI_CSR_BITS(SPI_CSR_BITS_8_BIT);
	}

	/* Use the requested or next highest possible frequency */
	div = SOC_ATMEL_SAM_MCK_FREQ_HZ / config->frequency;
	div = CLAMP(div, 1, UINT8_MAX);
	spi_csr |= SPI_CSR_SCBR(div);

	regs->SPI_CR = SPI_CR_SPIDIS; /* Disable SPI */
	regs->SPI_CR = SPI_CR_SWRST; /* Reset SPI */
	regs->SPI_MR = spi_mr;
	regs->SPI_CSR[config->slave] = spi_csr;
	regs->SPI_CR = SPI_CR_SPIEN; /* Enable SPI */

	data->ctx.config = config;

	return 0;
}

static void spi_sam_next_transfer(const struct device *dev)
{
	struct spi_sam_data *data = dev->data;

	if (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx)) {
		spi_sam_write(dev);
	} else {
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
	}
}

#ifdef CONFIG_SPI_SAM_DMA

static uint8_t tx_dummy;
static uint8_t rx_dummy;

static void dma_callback(const struct device *dma_dev, void *user_data,
	uint32_t channel, int status)
{
	const struct device *spi_dev = (const struct device *)user_data;
	struct spi_sam_data *data = spi_dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status != 0) {
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, spi_dev, status);
	}

	spi_context_update_tx(&data->ctx, 1, data->chunk_size);
	spi_context_update_rx(&data->ctx, 1, data->chunk_size);

	spi_sam_next_transfer(spi_dev);
}

/* DMA transceive path */
static int spi_sam_dma_txrx(const struct device *dev)
{
	const struct spi_sam_config *drv_cfg = dev->config;
	struct spi_sam_data *drv_data = dev->data;
	Spi *regs = drv_cfg->regs;
	int res = 0;

	struct dma_config rx_dma_cfg = {
		.source_data_size = 1,
		.dest_data_size = 1,
		.block_count = 1,
		.dma_slot = drv_cfg->dma_rx_perid,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.complete_callback_en = true,
		.error_callback_en = true,
		.dma_callback = dma_callback,
		.user_data = (void *)&dev,
	};

	uint32_t dest_address, dest_addr_adjust;

	if (spi_context_rx_buf_on(&drv_data->ctx)) {
		dest_address = (uint32_t)drv_data->ctx.rx_buf;
		dest_addr_adjust = DMA_ADDR_ADJ_INCREMENT;
	} else {
		dest_address = (uint32_t)&rx_dummy;
		dest_addr_adjust = DMA_ADDR_ADJ_NO_CHANGE;
	}

	struct dma_block_config rx_block_cfg = {
		.dest_addr_adj = dest_addr_adjust,
		.block_size = drv_data->chunk_size,
		.source_address = (uint32_t)&regs->SPI_RDR,
		.dest_address = dest_address
	};

	rx_dma_cfg.head_block = &rx_block_cfg;

	struct dma_config tx_dma_cfg = {
		.source_data_size = 1,
		.dest_data_size = 1,
		.block_count = 1,
		.dma_slot = drv_cfg->dma_tx_perid,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.complete_callback_en = true,
		.error_callback_en = true,
		.dma_callback = dma_callback,
		.user_data = (void *)&dev,
	};

	uint32_t source_address, source_addr_adjust;

	if (spi_context_tx_buf_on(&drv_data->ctx)) {
		source_address = (uint32_t)drv_data->ctx.tx_buf;
		source_addr_adjust = DMA_ADDR_ADJ_INCREMENT;
	} else {
		source_address = (uint32_t)&tx_dummy;
		source_addr_adjust = DMA_ADDR_ADJ_NO_CHANGE;
	}

	struct dma_block_config tx_block_cfg = {
		.source_addr_adj = source_addr_adjust,
		.block_size = drv_data->chunk_size,
		.source_address = source_address,
		.dest_address = (uint32_t)&regs->SPI_TDR
	};

	tx_dma_cfg.head_block = &tx_block_cfg;

	res = dma_config(drv_cfg->dma_dev, drv_cfg->dma_rx_channel, &rx_dma_cfg);
	if (res != 0) {
		LOG_ERR("failed to configure SPI DMA RX");
		goto out;
	}

	res = dma_config(drv_cfg->dma_dev, drv_cfg->dma_tx_channel, &tx_dma_cfg);
	if (res != 0) {
		LOG_ERR("failed to configure SPI DMA TX");
		goto out;
	}

	/* Clocking begins on tx, so start rx first */
	res = dma_start(drv_cfg->dma_dev, drv_cfg->dma_rx_channel);
	if (res != 0) {
		LOG_ERR("failed to start SPI DMA RX");
		goto out;
	}

	res = dma_start(drv_cfg->dma_dev, drv_cfg->dma_tx_channel);
	if (res != 0) {
		LOG_ERR("failed to start SPI DMA TX");
		dma_stop(drv_cfg->dma_dev, drv_cfg->dma_rx_channel);
	}

out:
	return res;
}

#endif /* CONFIG_SPI_SAM_DMA */

static void spi_sam_write(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;

#ifdef CONFIG_SPI_SAM_DMA
	data->chunk_size = spi_context_max_continuous_chunk(&data->ctx);

	if (cfg->dma_dev != NULL && data->chunk_size >= SAM_SPI_DMA_THRESHOLD) {
		/* Disable read interrupt */
		regs->SPI_IDR = SPI_IDR_RDRF;

		spi_sam_dma_txrx(dev);
	} else
#endif
	{
		uint8_t tx;

		if (spi_context_tx_buf_on(&data->ctx)) {
			tx = *(uint8_t *)(data->ctx.tx_buf);
		} else {
			tx = 0U;
		}

		/* Enable read interrupt */
		regs->SPI_IER = SPI_IER_RDRF;

		regs->SPI_TDR = SPI_TDR_TD(tx);
		spi_context_update_tx(&data->ctx, 1, 1);
	}
}

static void spi_sam_read(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;
	Spi *regs = cfg->regs;
	uint8_t rx;

	rx = regs->SPI_RDR;
	if (spi_context_rx_buf_on(&data->ctx)) {
		*data->ctx.rx_buf = rx;
	}

	spi_context_update_rx(&data->ctx, 1, 1);
}

static int spi_sam_transceive(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs,
			      bool asynchronous,
			      spi_callback_t cb,
			      void *userdata)
{
	struct spi_sam_data *data = dev->data;
	int err;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	err = spi_sam_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_cs_control(&data->ctx, true);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_sam_write(dev);

	err = spi_context_wait_for_completion(&data->ctx);
done:
	spi_context_release(&data->ctx, err);

	return err;
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

static void spi_sam_isr(const struct device *dev)
{
	const struct spi_sam_config *cfg = dev->config;
	Spi *regs = cfg->regs;
	uint32_t status = regs->SPI_SR;

	if (status & SPI_SR_RDRF) {
		spi_sam_read(dev);
		spi_sam_next_transfer(dev);
	}
}

static int spi_sam_init(const struct device *dev)
{
	int err;
	const struct spi_sam_config *cfg = dev->config;
	struct spi_sam_data *data = dev->data;

	soc_pmc_peripheral_enable(cfg->periph_id);

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	cfg->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static const struct spi_driver_api spi_sam_driver_api = {
	.transceive = spi_sam_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sam_transceive_async,
#endif
	.release = spi_sam_release,
};

#define SPI_DMA_INIT(n)										\
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),				\
	.dma_tx_channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),				\
	.dma_tx_perid = DT_INST_DMAS_CELL_BY_NAME(n, tx, perid),				\
	.dma_rx_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),				\
	.dma_rx_perid = DT_INST_DMAS_CELL_BY_NAME(n, rx, perid),

#ifdef CONFIG_SPI_SAM_DMA
#define SPI_SAM_USE_DMA(n) DT_INST_DMAS_HAS_NAME(n, tx)
#else
#define SPI_SAM_USE_DMA(n) 0
#endif

#define SPI_SAM_DEFINE_CONFIG(n)								\
	static const struct spi_sam_config spi_sam_config_##n = {				\
		.regs = (Spi *)DT_INST_REG_ADDR(n),						\
		.periph_id = DT_INST_PROP(n, peripheral_id),					\
		.irq_config_func = &spi_##n##_sam_config_func,					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.loopback = DT_INST_PROP(n, loopback),						\
		COND_CODE_1(SPI_SAM_USE_DMA(n), (SPI_DMA_INIT(n)), ())				\
	}

#define SPI_SAM_DEVICE_INIT(n)									\
	static void spi_##n##_sam_config_func(const struct device *dev)				\
	{											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),					\
			    DT_INST_IRQ_BY_IDX(n, 0, priority),					\
			    spi_sam_isr,							\
			    DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));					\
	}											\
	PINCTRL_DT_INST_DEFINE(n);								\
	SPI_SAM_DEFINE_CONFIG(n);								\
	static struct spi_sam_data spi_sam_dev_data_##n = {					\
		SPI_CONTEXT_INIT_LOCK(spi_sam_dev_data_##n, ctx),				\
		SPI_CONTEXT_INIT_SYNC(spi_sam_dev_data_##n, ctx),				\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)				\
	};											\
	DEVICE_DT_INST_DEFINE(n, &spi_sam_init, NULL,						\
			      &spi_sam_dev_data_##n,						\
			      &spi_sam_config_##n, POST_KERNEL,					\
			      CONFIG_SPI_INIT_PRIORITY, &spi_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM_DEVICE_INIT)
