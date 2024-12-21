/*
 * Copyright (c) 2024, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ecspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_ecspi, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <fsl_ecspi.h>

#include "spi_context.h"

#define SPI_MCUX_ECSPI_MAX_BURST 4096

struct spi_mcux_config {
	ECSPI_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct spi_mcux_data {
	ecspi_master_handle_t handle;
	struct spi_context ctx;

	uint16_t dfs;
	uint16_t word_size;

	uint32_t rx_data;
	uint32_t tx_data;
};

static inline uint16_t bytes_per_word(uint16_t bits_per_word)
{
	if (bits_per_word <= 8U) {
		return 1U;
	}
	if (bits_per_word <= 16U) {
		return 2U;
	}

	return 4U;
}

static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	ecspi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return;
	}

	transfer.channel = ctx->config->slave;

	if (spi_context_rx_buf_on(ctx)) {
		transfer.rxData = &data->rx_data;
	} else {
		transfer.rxData = NULL;
	}

	if (spi_context_tx_buf_on(ctx)) {
		switch (data->dfs) {
		case 1U:
			data->tx_data = UNALIGNED_GET((uint8_t *)ctx->tx_buf);
			break;
		case 2U:
			data->tx_data = UNALIGNED_GET((uint16_t *)ctx->tx_buf);
			break;
		case 4U:
			data->tx_data = UNALIGNED_GET((uint32_t *)ctx->tx_buf);
			break;
		}

		transfer.txData = &data->tx_data;
	} else {
		transfer.txData = NULL;
	}

	transfer.dataSize = data->dfs;

	status = ECSPI_MasterTransferNonBlocking(base, &data->handle, &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start");
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, -EIO);
	}
}

static void spi_mcux_isr(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;

	ECSPI_MasterTransferHandleIRQ(base, &data->handle);
}

static void spi_mcux_master_transfer_callback(ECSPI_Type *base, ecspi_master_handle_t *handle,
					      status_t status, void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct spi_mcux_data *data = dev->data;

	if (spi_context_rx_buf_on(&data->ctx)) {
		switch (data->dfs) {
		case 1:
			UNALIGNED_PUT(data->rx_data, (uint8_t *)data->ctx.rx_buf);
			break;
		case 2:
			UNALIGNED_PUT(data->rx_data, (uint16_t *)data->ctx.rx_buf);
			break;
		case 4:
			UNALIGNED_PUT(data->rx_data, (uint32_t *)data->ctx.rx_buf);
			break;
		}
	}

	spi_context_update_tx(&data->ctx, data->dfs, 1);
	spi_context_update_rx(&data->ctx, data->dfs, 1);

	spi_mcux_transfer_next_packet(dev);
}

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;
	ecspi_master_config_t master_config;
	uint32_t clock_freq;
	uint16_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("HW byte re-ordering not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->slave > kECSPI_Channel3) {
		LOG_ERR("Slave %d is greater than %d", spi_cfg->slave, kECSPI_Channel3);
		return -EINVAL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		LOG_ERR("Failed to get clock rate");
		return -EINVAL;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (0 == word_size || word_size > 32) {
		LOG_ERR("Invalid word size (0 < %d <= 32)", word_size);
		return -EINVAL;
	}

	ECSPI_MasterGetDefaultConfig(&master_config);

	master_config.channel = (ecspi_channel_source_t)spi_cfg->slave;
	master_config.channelConfig.polarity =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kECSPI_PolarityActiveLow
		: kECSPI_PolarityActiveHigh;
	master_config.channelConfig.phase =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kECSPI_ClockPhaseSecondEdge
		: kECSPI_ClockPhaseFirstEdge;
	master_config.baudRate_Bps = spi_cfg->frequency;
	master_config.burstLength = word_size;

	master_config.enableLoopback = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP);

	if (!spi_cs_is_gpio(spi_cfg)) {
		uint32_t clock_cycles =
			DIV_ROUND_UP(spi_cfg->cs.delay * USEC_PER_SEC, spi_cfg->frequency);

		if (clock_cycles > 63U) {
			LOG_ERR("CS delay is greater than 63 clock cycles (%u)", clock_cycles);
			return -EINVAL;
		}
		master_config.chipSelectDelay = (uint8_t)clock_cycles;
	}

	ECSPI_MasterInit(base, &master_config, clock_freq);
	ECSPI_MasterTransferCreateHandle(base, &data->handle,
					 spi_mcux_master_transfer_callback,
					 (void *)dev);

	data->word_size = word_size;
	data->dfs = bytes_per_word(word_size);
	data->ctx.config = spi_cfg;

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(&data->ctx, true);

	spi_mcux_transfer_next_packet(dev);
	ret = spi_context_wait_for_completion(&data->ctx);

out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_mcux_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mcux_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	ARG_UNUSED(spi_cfg);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_init(const struct device *dev)
{
	int ret;
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;

	config->irq_config_func(dev);

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_mcux_driver_api = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mcux_release,
};

#define SPI_MCUX_ECSPI_INIT(n)									\
	PINCTRL_DT_INST_DEFINE(n);								\
	static void spi_mcux_config_func_##n(const struct device *dev);				\
												\
	static const struct spi_mcux_config spi_mcux_config_##n = {				\
		.base = (ECSPI_Type *) DT_INST_REG_ADDR(n),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),		\
		.irq_config_func = spi_mcux_config_func_##n,					\
	};											\
												\
	static struct spi_mcux_data spi_mcux_data_##n = {					\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),					\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),					\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, spi_mcux_init, NULL,						\
			      &spi_mcux_data_##n, &spi_mcux_config_##n,				\
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,				\
			      &spi_mcux_driver_api);						\
												\
	static void spi_mcux_config_func_##n(const struct device *dev)				\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),				\
			    spi_mcux_isr, DEVICE_DT_INST_GET(n), 0);				\
												\
		irq_enable(DT_INST_IRQN(n));							\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_ECSPI_INIT)
