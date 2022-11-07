/*
 * Copyright (c) 2018, NXP
 *
 * Forked off the spi_mcux_lpi2c driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_lpspi

#include <errno.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_lpspi.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_rv32m1_lpspi);

#include "spi_context.h"

#define CHIP_SELECT_COUNT	4
#define MAX_DATA_WIDTH		4096

struct spi_mcux_config {
	LPSPI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	clock_ip_name_t clock_ip_name;
	uint32_t clock_ip_src;
	void (*irq_config_func)(const struct device *dev);
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

struct spi_mcux_data {
	const struct device *dev;
	lpspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
};

static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	lpspi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return;
	}

	transfer.configFlags = kLPSPI_MasterPcsContinuous |
			       (ctx->config->slave << LPSPI_MASTER_PCS_SHIFT);

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		transfer.txData = NULL;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = NULL;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len == ctx->rx_len) {
		/* rx and tx are the same length */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len > ctx->rx_len) {
		/* Break up the tx into multiple transfers so we don't have to
		 * rx into a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	}

	if (!(ctx->tx_count <= 1 && ctx->rx_count <= 1)) {
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	}

	data->transfer_len = transfer.dataSize;
	status = LPSPI_MasterTransferNonBlocking(base, &data->handle,
						 &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start");
	}
}

static void spi_mcux_isr(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = config->base;

	LPSPI_MasterTransferHandleIRQ(base, &data->handle);
}

static void spi_mcux_master_transfer_callback(LPSPI_Type *base,
					      lpspi_master_handle_t *handle,
					      status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = config->base;
	lpspi_master_config_t master_config;
	uint32_t clock_freq;
	uint32_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	LPSPI_MasterGetDefaultConfig(&master_config);

	if (spi_cfg->slave > CHIP_SELECT_COUNT) {
		LOG_ERR("Slave %d is greater than %d",
			    spi_cfg->slave,
			    CHIP_SELECT_COUNT);
		return -EINVAL;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d",
			    word_size, MAX_DATA_WIDTH);
		return -EINVAL;
	}

	master_config.bitsPerFrame = word_size;

	master_config.cpol =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kLPSPI_ClockPolarityActiveLow
		: kLPSPI_ClockPolarityActiveHigh;

	master_config.cpha =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kLPSPI_ClockPhaseSecondEdge
		: kLPSPI_ClockPhaseFirstEdge;

	master_config.direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB)
		? kLPSPI_LsbFirst
		: kLPSPI_MsbFirst;

	master_config.baudRate = spi_cfg->frequency;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPSPI_MasterInit(base, &master_config, clock_freq);

	LPSPI_MasterTransferCreateHandle(base, &data->handle,
					 spi_mcux_master_transfer_callback,
					 data);

	LPSPI_SetDummyData(base, 0);

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

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

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

static int spi_mcux_release(const struct device *dev,
			    const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_init(const struct device *dev)
{
	int err;
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;

	CLOCK_SetIpSrc(config->clock_ip_name, config->clock_ip_src);

	config->irq_config_func(dev);

	data->dev = dev;

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_PINCTRL
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_mcux_driver_api = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
	.release = spi_mcux_release,
};

#ifdef CONFIG_PINCTRL
#define PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#else
#define PINCTRL_DEFINE(n)
#define PINCTRL_INIT(n)
#endif

#define SPI_RV32M1_INIT(n)						\
	PINCTRL_DEFINE(n)						\
									\
	static void spi_mcux_config_func_##n(const struct device *dev);	\
									\
	static const struct spi_mcux_config spi_mcux_config_##n = {	\
		.base = (LPSPI_Type *) DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
			DT_INST_CLOCKS_CELL(n, name),			\
		.irq_config_func = spi_mcux_config_func_##n,		\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
		.clock_ip_src  = kCLOCK_IpSrcFircAsync,			\
		PINCTRL_INIT(n)						\
	};								\
									\
	static struct spi_mcux_data spi_mcux_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &spi_mcux_init, NULL,			\
			    &spi_mcux_data_##n,				\
			    &spi_mcux_config_##n,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &spi_mcux_driver_api);			\
									\
	static void spi_mcux_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    0,						\
			    spi_mcux_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_RV32M1_INIT)
