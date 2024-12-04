/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 * Copyright (c) 2021, ATL Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cypress_psoc6_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_psoc6);

#include <errno.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <soc.h>

#include "spi_context.h"

#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_scb_spi.h"
#include "cy_sysint.h"

#define SPI_CHIP_SELECT_COUNT		4
#define SPI_MAX_DATA_WIDTH		16
#define SPI_PSOC6_CLK_DIV_NUMBER	1

struct spi_psoc6_config {
	CySCB_Type *base;
	uint32_t periph_id;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
};

struct spi_psoc6_transfer {
	uint8_t *txData;
	uint8_t *rxData;
	size_t dataSize;
};

struct spi_psoc6_data {
	struct spi_context ctx;
	struct cy_stc_scb_spi_config cfg;
	struct spi_psoc6_transfer xfer;
};

static void spi_psoc6_transfer_next_packet(const struct device *dev)
{
	const struct spi_psoc6_config *config = dev->config;
	struct spi_psoc6_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	struct spi_psoc6_transfer *xfer = &data->xfer;
	uint32_t count;

	LOG_DBG("TX L: %d, RX L: %d", ctx->tx_len, ctx->rx_len);

	if ((ctx->tx_len == 0U) && (ctx->rx_len == 0U)) {
		/* nothing left to rx or tx, we're done! */
		xfer->dataSize = 0U;

		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, 0U);
		return;
	}

	if (ctx->tx_len == 0U) {
		/* rx only, nothing to tx */
		xfer->txData = NULL;
		xfer->rxData = ctx->rx_buf;
		xfer->dataSize = ctx->rx_len;
	} else if (ctx->rx_len == 0U) {
		/* tx only, nothing to rx */
		xfer->txData = (uint8_t *) ctx->tx_buf;
		xfer->rxData = NULL;
		xfer->dataSize = ctx->tx_len;
	} else if (ctx->tx_len == ctx->rx_len) {
		/* rx and tx are the same length */
		xfer->txData = (uint8_t *) ctx->tx_buf;
		xfer->rxData = ctx->rx_buf;
		xfer->dataSize = ctx->tx_len;
	} else if (ctx->tx_len > ctx->rx_len) {
		/* Break up the tx into multiple transfers so we don't have to
		 * rx into a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		xfer->txData = (uint8_t *) ctx->tx_buf;
		xfer->rxData = ctx->rx_buf;
		xfer->dataSize = ctx->rx_len;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		xfer->txData = (uint8_t *) ctx->tx_buf;
		xfer->rxData = ctx->rx_buf;
		xfer->dataSize = ctx->tx_len;
	}

	if (xfer->txData != NULL) {
		if (Cy_SCB_SPI_WriteArray(config->base, xfer->txData,
					  xfer->dataSize) != xfer->dataSize) {
			goto err;
		}
	} else {
		/* Need fill TX fifo with garbage to perform read.
		 * This keeps logic simple and saves stack.
		 * Use 0 as dummy data.
		 */
		for (count = 0U; count < xfer->dataSize; count++) {
			if (Cy_SCB_SPI_Write(config->base, 0U) == 0U) {
				goto err;
			}
		}
	}

	LOG_DBG("TRX L: %d", xfer->dataSize);

	return;
err:
	/* no FIFO available to run the transfer */
	xfer->dataSize = 0U;

	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, -ENOMEM);
}

static void spi_psoc6_isr(const struct device *dev)
{
	const struct spi_psoc6_config *config = dev->config;
	struct spi_psoc6_data *data = dev->data;

	Cy_SCB_ClearMasterInterrupt(config->base,
				    CY_SCB_MASTER_INTR_SPI_DONE);

	/* extract data from RX FIFO */
	if (data->xfer.rxData != NULL) {
		Cy_SCB_SPI_ReadArray(config->base,
				     data->xfer.rxData,
				     data->xfer.dataSize);
	} else {
		Cy_SCB_ClearRxFifo(config->base);
	}

	/* Set next data block */
	spi_context_update_tx(&data->ctx, 1, data->xfer.dataSize);
	spi_context_update_rx(&data->ctx, 1, data->xfer.dataSize);

	/* Start next block
	 * Since 1 byte at TX FIFO will start transfer data, let's try
	 * minimize ISR recursion disabling all interrupt sources when add
	 * data on TX FIFO
	 */
	Cy_SCB_SetMasterInterruptMask(config->base, 0U);

	spi_psoc6_transfer_next_packet(dev);

	if (data->xfer.dataSize > 0U) {
		Cy_SCB_SetMasterInterruptMask(config->base,
					      CY_SCB_MASTER_INTR_SPI_DONE);
	}
}

static uint32_t spi_psoc6_get_freqdiv(uint32_t frequency)
{
	uint32_t oversample;
	uint32_t bus_freq = 100000000UL;
	/*
	 * TODO: Get PerBusSpeed when clocks are available to PSOC 6.
	 * Currently the bus freq is fixed to 50Mhz and max SPI clk can be
	 * 12.5MHz.
	 */

	for (oversample = 4; oversample < 16; oversample++) {
		if ((bus_freq / oversample) <= frequency) {
			break;
		}
	}

	/* Oversample [4, 16] */
	return oversample;
}

static void spi_psoc6_master_get_defaults(struct cy_stc_scb_spi_config *cfg)
{
	cfg->spiMode = CY_SCB_SPI_MASTER;
	cfg->subMode = CY_SCB_SPI_MOTOROLA;
	cfg->sclkMode = 0U;
	cfg->oversample = 0U;
	cfg->rxDataWidth = 0U;
	cfg->txDataWidth = 0U;
	cfg->enableMsbFirst = false;
	cfg->enableFreeRunSclk = false;
	cfg->enableInputFilter = false;
	cfg->enableMisoLateSample = false;
	cfg->enableTransferSeperation = false;
	cfg->ssPolarity = 0U;
	cfg->enableWakeFromSleep = false;
	cfg->rxFifoTriggerLevel = 0U;
	cfg->rxFifoIntEnableMask = 0U;
	cfg->txFifoTriggerLevel = 0U;
	cfg->txFifoIntEnableMask = 0U;
	cfg->masterSlaveIntEnableMask = 0U;
}

static int spi_psoc6_configure(const struct device *dev,
			       const struct spi_config *spi_cfg)
{
	struct spi_psoc6_data *data = dev->data;
	uint32_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d",
			word_size, SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		spi_psoc6_master_get_defaults(&data->cfg);

		if (spi_cfg->slave > SPI_CHIP_SELECT_COUNT) {
			LOG_ERR("Slave %d is greater than %d",
				spi_cfg->slave, SPI_CHIP_SELECT_COUNT);
			return -EINVAL;
		}

		data->cfg.rxDataWidth = data->cfg.txDataWidth = word_size;

		if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
			if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
				data->cfg.sclkMode = CY_SCB_SPI_CPHA1_CPOL1;
			} else {
				data->cfg.sclkMode = CY_SCB_SPI_CPHA1_CPOL0;
			}
		} else {
			if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
				data->cfg.sclkMode = CY_SCB_SPI_CPHA0_CPOL1;
			} else {
				data->cfg.sclkMode = CY_SCB_SPI_CPHA0_CPOL0;
			}
		}

		data->cfg.enableMsbFirst = !!!(spi_cfg->operation &
					       SPI_TRANSFER_LSB);
		data->cfg.oversample = spi_psoc6_get_freqdiv(spi_cfg->frequency);

		data->ctx.config = spi_cfg;
	} else {
		/* Slave mode is not implemented yet. */
		return -ENOTSUP;
	}

	return 0;
}

static void spi_psoc6_transceive_sync_loop(const struct device *dev)
{
	const struct spi_psoc6_config *config = dev->config;
	struct spi_psoc6_data *data = dev->data;

	while (data->xfer.dataSize > 0U) {
		while (!Cy_SCB_IsTxComplete(config->base)) {
			;
		}

		if (data->xfer.rxData != NULL) {
			Cy_SCB_SPI_ReadArray(config->base,
					     data->xfer.rxData,
					     data->xfer.dataSize);
		} else {
			Cy_SCB_ClearRxFifo(config->base);
		}

		spi_context_update_tx(&data->ctx, 1, data->xfer.dataSize);
		spi_context_update_rx(&data->ctx, 1, data->xfer.dataSize);

		spi_psoc6_transfer_next_packet(dev);
	}
}

static int spi_psoc6_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				bool asynchronous,
				spi_callback_t cb,
				void *userdata)
{
	const struct spi_psoc6_config *config = dev->config;
	struct spi_psoc6_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	LOG_DBG("\n\n");

	ret = spi_psoc6_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	Cy_SCB_SPI_Init(config->base, &data->cfg, NULL);
	Cy_SCB_SPI_SetActiveSlaveSelect(config->base, spi_cfg->slave);
	Cy_SCB_SPI_Enable(config->base);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	spi_psoc6_transfer_next_packet(dev);

	if (asynchronous) {
		Cy_SCB_SetMasterInterruptMask(config->base,
					      CY_SCB_MASTER_INTR_SPI_DONE);
	} else {
		spi_psoc6_transceive_sync_loop(dev);
	}

	ret = spi_context_wait_for_completion(&data->ctx);

	Cy_SCB_SPI_Disable(config->base, NULL);

out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_psoc6_transceive_sync(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	return spi_psoc6_transceive(dev, spi_cfg, tx_bufs,
				    rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_psoc6_transceive_async(const struct device *dev,
				      const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      spi_callback_t cb,
				      void *userdata)
{
	return spi_psoc6_transceive(dev, spi_cfg, tx_bufs,
				    rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_psoc6_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct spi_psoc6_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_psoc6_init(const struct device *dev)
{
	int err;
	const struct spi_psoc6_config *config = dev->config;
	struct spi_psoc6_data *data = dev->data;


	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	Cy_SysClk_PeriphAssignDivider(config->periph_id,
				      CY_SYSCLK_DIV_8_BIT,
				      SPI_PSOC6_CLK_DIV_NUMBER);
	Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_8_BIT,
				   SPI_PSOC6_CLK_DIV_NUMBER, 0U);
	Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_8_BIT,
				      SPI_PSOC6_CLK_DIV_NUMBER);

#ifdef CONFIG_SPI_ASYNC
	config->irq_config_func(dev);
#endif

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	return spi_psoc6_release(dev, NULL);
}

static DEVICE_API(spi, spi_psoc6_driver_api) = {
	.transceive = spi_psoc6_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_psoc6_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_psoc6_release,
};

#define SPI_PSOC6_DEVICE_INIT(n)					\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void spi_psoc6_spi##n##_irq_cfg(const struct device *port); \
	static const struct spi_psoc6_config spi_psoc6_config_##n = {	\
		.base = (CySCB_Type *)DT_INST_REG_ADDR(n),		\
		.periph_id = DT_INST_PROP(n, peripheral_id),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.irq_config_func = spi_psoc6_spi##n##_irq_cfg,		\
	};								\
	static struct spi_psoc6_data spi_psoc6_dev_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_psoc6_dev_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_psoc6_dev_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
	DEVICE_DT_INST_DEFINE(n, spi_psoc6_init, NULL,			\
			      &spi_psoc6_dev_data_##n,			\
			      &spi_psoc6_config_##n, POST_KERNEL,	\
			      CONFIG_SPI_INIT_PRIORITY,			\
			      &spi_psoc6_driver_api);			\
	static void spi_psoc6_spi##n##_irq_cfg(const struct device *port) \
	{								\
		CY_PSOC6_DT_INST_NVIC_INSTALL(n,			\
					      spi_psoc6_isr);		\
	};

DT_INST_FOREACH_STATUS_OKAY(SPI_PSOC6_DEVICE_INIT)
