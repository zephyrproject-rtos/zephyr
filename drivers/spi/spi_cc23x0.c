/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_cc23x0, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <driverlib/clkctl.h>
#include <driverlib/spi.h>

#include "spi_context.h"

/*
 * SPI bit rate = (SPI functional clock frequency) / ((SCR + 1) * 2)
 * Serial clock divider value (SCR) can be from 0 to 1023.
 */
#define SPI_CC23_MIN_FREQ	DIV_ROUND_UP(TI_CC23X0_DT_CPU_CLK_FREQ_HZ, 2048)
#define SPI_CC23_MAX_FREQ	(TI_CC23X0_DT_CPU_CLK_FREQ_HZ >> 1)

#define SPI_CC23_DATA_WIDTH	8
#define SPI_CC23_DFS		(SPI_CC23_DATA_WIDTH >> 3)

#define SPI_CC23_INT_MASK	(SPI_TXEMPTY | SPI_IDLE | SPI_RX)

struct spi_cc23x0_config {
	uint32_t base;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(void);
};

struct spi_cc23x0_data {
	struct spi_context ctx;
	size_t tx_len_left;
	uint32_t tx_count;
	uint32_t rx_count;
	bool xfer_done;
};

static void spi_cc23x0_isr(const struct device *dev)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t txd = 0, rxd;
	uint32_t status;

	status = SPIIntStatus(cfg->base, true);
	LOG_DBG("status = %08x", status);

	/*
	 * Disabling the interrupts in this ISR when SPI has completed
	 * the transfer triggers a subsequent spurious interrupt, with
	 * a null status. Let's ignore this event.
	 */
	if (!status) {
		return;
	}

	if (status & SPI_RX) {
		/* Rx FIFO contains 1 byte */
		LOG_DBG("SPI_RX");

		SPIClearInt(cfg->base, SPI_RX);

		SPIGetDataNonBlocking(cfg->base, &rxd);

		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = rxd;
			spi_context_update_rx(ctx, SPI_CC23_DFS, 1);
		}

		data->rx_count++;
	}

	if (status & SPI_IDLE) {
		/* The byte has been transferred and SPI has moved to idle mode */
		LOG_DBG("SPI_IDLE (tx_len_left = %d)", data->tx_len_left);

		SPIClearInt(cfg->base, SPI_IDLE);

		if (!data->tx_len_left) {
			LOG_DBG("xfer_done");
			data->xfer_done = true;
		}
	}

	/*
	 * Do not push a new Tx byte in the Tx FIFO while the current Rx byte (if any)
	 * has not been pulled from the Rx FIFO. In other words, Tx count and Rx count
	 * must be equal so a new Tx byte can be pushed.
	 */
	if (status & SPI_TXEMPTY && !data->xfer_done && data->tx_count == data->rx_count) {
		/*
		 * The previous byte in the Tx FIFO (if any) has been moved
		 * to the shift register
		 */
		LOG_DBG("SPI_TXEMPTY");

		SPIClearInt(cfg->base, SPI_TXEMPTY);

		if (spi_context_tx_buf_on(ctx)) {
			txd = *ctx->tx_buf;
			spi_context_update_tx(ctx, SPI_CC23_DFS, 1);
		}

		SPIPutDataNonBlocking(cfg->base, txd);

		data->tx_count++;
		data->tx_len_left--;
	}

	if (data->xfer_done) {
		LOG_DBG("complete");
		SPIDisableInt(cfg->base, SPI_CC23_INT_MASK);
		spi_context_complete(ctx, dev, 0);
	}
}

static int spi_cc23x0_configure(const struct device *dev,
				const struct spi_config *config)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;
	uint32_t protocol;

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex is not supported");
		return -ENOTSUP;
	}

	/* Peripheral mode has not been implemented */
	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Peripheral mode is not supported");
		return -ENOTSUP;
	}

	/* Word sizes other than 8 bits has not been implemented */
	if (SPI_WORD_SIZE_GET(config->operation) != SPI_CC23_DATA_WIDTH) {
		LOG_ERR("Word sizes other than %d bits are not supported", SPI_CC23_DATA_WIDTH);
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("Transfer LSB first mode is not supported");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Multiple lines are not supported");
		return -EINVAL;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH && !spi_cs_is_gpio(config)) {
		LOG_ERR("Active high CS requires emulation through a GPIO line");
		return -EINVAL;
	}

	if (config->frequency < SPI_CC23_MIN_FREQ) {
		LOG_ERR("Frequencies lower than %d Hz are not supported", SPI_CC23_MIN_FREQ);
		return -EINVAL;
	}

	if (config->frequency > SPI_CC23_MAX_FREQ) {
		LOG_ERR("Frequency greater than %d Hz are not supported", SPI_CC23_MAX_FREQ);
		return -EINVAL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
			protocol = SPI_FRF_MOTO_MODE_7;
		} else {
			protocol = SPI_FRF_MOTO_MODE_6;
		}
	} else {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
			protocol = SPI_FRF_MOTO_MODE_5;
		} else {
			protocol = SPI_FRF_MOTO_MODE_4;
		}
	}

	/* Enable clock */
	CLKCTLEnable(CLKCTL_BASE, CLKCTL_SPI0);

	/* Disable SPI before making configuration changes */
	SPIDisable(cfg->base);

	/* Configure SPI */
	SPIConfigSetExpClk(cfg->base, TI_CC23X0_DT_CPU_CLK_FREQ_HZ,
			   protocol, SPI_MODE_CONTROLLER,
			   config->frequency, SPI_CC23_DATA_WIDTH);

	data->ctx.config = config;

	/* Configure Rx FIFO level */
	HWREG(cfg->base + SPI_O_IFLS) = SPI_IFLS_RXSEL_LEVEL_1;

	/* Re-enable SPI after making configuration changes */
	SPIEnable(cfg->base);

	return 0;
}

static void spi_cc23x0_initialize_data(struct spi_cc23x0_data *data)
{
	data->tx_len_left = MAX(spi_context_total_tx_len(&data->ctx),
				spi_context_total_rx_len(&data->ctx));
	data->tx_count = 0;
	data->rx_count = 0;
	data->xfer_done = false;
}

static int spi_cc23x0_transceive(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_cc23x0_configure(dev, config);
	if (ret) {
		goto ctx_release;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, SPI_CC23_DFS);

	spi_cc23x0_initialize_data(data);

	spi_context_cs_control(ctx, true);

	SPIEnableInt(cfg->base, SPI_CC23_INT_MASK);

	ret = spi_context_wait_for_completion(ctx);
	if (ret) {
		SPIDisableInt(cfg->base, SPI_CC23_INT_MASK);
		LOG_ERR("SPI transfer failed (%d)", ret);
	} else {
		LOG_DBG("SPI transfer completed");
	}

	spi_context_cs_control(ctx, false);

ctx_release:
	spi_context_release(ctx, ret);
	return ret;
}

static int spi_cc23x0_release(const struct device *dev,
			      const struct spi_config *config)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	if (SPIBusy(cfg->base)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_cc23x0_driver_api = {
	.transceive = spi_cc23x0_transceive,
	.release = spi_cc23x0_release,
};

static int spi_cc23x0_init(const struct device *dev)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;
	int ret;

	cfg->irq_config_func();

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply SPI pinctrl state");
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SPI_CC23X0_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void spi_irq_config_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    spi_cc23x0_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	};								\
									\
	static const struct spi_cc23x0_config spi_cc23x0_config_##n = {	\
		.base = DT_INST_REG_ADDR(n),				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.irq_config_func = spi_irq_config_func_##n		\
	};								\
									\
	static struct spi_cc23x0_data spi_cc23x0_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_cc23x0_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_cc23x0_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      spi_cc23x0_init,				\
			      NULL,					\
			      &spi_cc23x0_data_##n,			\
			      &spi_cc23x0_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_SPI_INIT_PRIORITY,			\
			      &spi_cc23x0_driver_api)

DT_INST_FOREACH_STATUS_OKAY(SPI_CC23X0_INIT)
