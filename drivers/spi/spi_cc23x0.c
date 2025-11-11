/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_cc23x0, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <driverlib/clkctl.h>
#include <driverlib/spi.h>

#include <inc/hw_memmap.h>

#include "spi_context.h"

/*
 * SPI bit rate = (SPI functional clock frequency) / ((SCR + 1) * 2)
 * Serial clock divider value (SCR) can be from 0 to 1023.
 */
#define SPI_CC23_MIN_FREQ	DIV_ROUND_UP(TI_CC23X0_DT_CPU_CLK_FREQ_HZ, 2048)
#define SPI_CC23_MAX_FREQ	(TI_CC23X0_DT_CPU_CLK_FREQ_HZ >> 1)

#define SPI_CC23_DATA_WIDTH	8
#define SPI_CC23_DFS		(SPI_CC23_DATA_WIDTH >> 3)

#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
#define SPI_CC23_REG_GET(base, offset) ((base) + (offset))
#define SPI_CC23_INT_MASK	SPI_DMA_DONE_RX
#else
#define SPI_CC23_INT_MASK	(SPI_TXEMPTY | SPI_IDLE | SPI_RX)
#endif

struct spi_cc23x0_config {
	uint32_t base;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(void);
#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
	const struct device *dma_dev;
	uint8_t dma_channel_tx;
	uint8_t dma_trigsrc_tx;
	uint8_t dma_channel_rx;
	uint8_t dma_trigsrc_rx;
#endif
};

struct spi_cc23x0_data {
	struct spi_context ctx;
	size_t tx_len_left;
#ifndef CONFIG_SPI_CC23X0_DMA_DRIVEN
	uint32_t tx_count;
	uint32_t rx_count;
	bool xfer_done;
#endif
};

static void spi_cc23x0_isr(const struct device *dev)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t status;
#ifndef CONFIG_SPI_CC23X0_DMA_DRIVEN
	uint32_t txd = 0, rxd;
#endif

	status = SPIIntStatus(cfg->base, true);
	LOG_DBG("status = %08x", status);

#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
	if (status & SPI_DMA_DONE_RX) {
		SPIClearInt(cfg->base, SPI_DMA_DONE_RX);
		spi_context_complete(ctx, dev, 0);
	}
#else
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
#endif
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

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		HWREG(cfg->base + SPI_O_CTL1) |= SPI_CTL1_LBM;
	} else {
		HWREG(cfg->base + SPI_O_CTL1) &= ~SPI_CTL1_LBM;
	}

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
#ifndef CONFIG_SPI_CC23X0_DMA_DRIVEN
	data->tx_count = 0;
	data->rx_count = 0;
	data->xfer_done = false;
#endif
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
#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
	struct dma_block_config block_cfg_tx = {
		.dest_address = SPI_CC23_REG_GET(cfg->base, SPI_O_TXDATA),
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.block_size = 1,
	};

	struct dma_config dma_cfg_tx = {
		.dma_slot = cfg->dma_trigsrc_tx,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.block_count = 1,
		.head_block = &block_cfg_tx,
		.source_data_size = SPI_CC23_DFS,
		.dest_data_size = SPI_CC23_DFS,
		.source_burst_length = SPI_CC23_DFS,
		.dma_callback = NULL,
		.user_data = NULL,
	};

	struct dma_block_config block_cfg_rx = {
		.source_address = SPI_CC23_REG_GET(cfg->base, SPI_O_RXDATA),
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.block_size = 1,
	};

	struct dma_config dma_cfg_rx = {
		.dma_slot = cfg->dma_trigsrc_rx,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.block_count = 1,
		.head_block = &block_cfg_rx,
		.source_data_size = SPI_CC23_DFS,
		.dest_data_size = SPI_CC23_DFS,
		.source_burst_length = SPI_CC23_DFS,
		.dma_callback = NULL,
		.user_data = NULL,
	};
#endif

	pm_policy_device_power_lock_get(dev);

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_cc23x0_configure(dev, config);
	if (ret) {
		goto ctx_release;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, SPI_CC23_DFS);

#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
	if (spi_context_total_tx_len(ctx) != spi_context_total_rx_len(ctx)) {
		LOG_ERR("In DMA mode, RX and TX buffer lengths must be the same");
		ret = -EINVAL;
		goto ctx_release;
	}
#endif

	spi_cc23x0_initialize_data(data);

	spi_context_cs_control(ctx, true);

	SPIEnableInt(cfg->base, SPI_CC23_INT_MASK);

#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
	block_cfg_tx.source_address = (uint32_t)ctx->tx_buf;
	block_cfg_tx.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	block_cfg_tx.block_size = SPI_CC23_DFS * data->tx_len_left;

	block_cfg_rx.dest_address = (uint32_t)ctx->rx_buf;
	block_cfg_rx.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	block_cfg_rx.block_size = SPI_CC23_DFS * data->tx_len_left;

	ret = pm_device_runtime_get(cfg->dma_dev);
	if (ret) {
		LOG_ERR("Failed to resume DMA (%d)", ret);
		goto int_disable;
	}

	ret = dma_config(cfg->dma_dev, cfg->dma_channel_tx, &dma_cfg_tx);
	if (ret) {
		LOG_ERR("Failed to configure DMA TX channel (%d)", ret);
		goto dma_suspend;
	}

	ret = dma_config(cfg->dma_dev, cfg->dma_channel_rx, &dma_cfg_rx);
	if (ret) {
		LOG_ERR("Failed to configure DMA RX channel (%d)", ret);
		goto dma_suspend;
	}

	/* Disable DMA triggers */
	SPIDisableDMA(cfg->base, SPI_DMA_TX | SPI_DMA_RX);

	/* Start DMA channels */
	dma_start(cfg->dma_dev, cfg->dma_channel_rx);
	dma_start(cfg->dma_dev, cfg->dma_channel_tx);

	/* Enable DMA triggers to start transfer */
	SPIEnableDMA(cfg->base, SPI_DMA_TX | SPI_DMA_RX);

	ret = spi_context_wait_for_completion(&data->ctx);
	if (ret) {
		LOG_ERR("SPI transfer failed (%d)", ret);
		goto dma_suspend;
	}

	spi_context_update_tx(ctx, SPI_CC23_DFS, data->tx_len_left);
	spi_context_update_rx(ctx, SPI_CC23_DFS, data->tx_len_left);

	LOG_DBG("SPI transfer completed");

dma_suspend:
	ret = pm_device_runtime_put(cfg->dma_dev);
	if (ret) {
		LOG_ERR("Failed to suspend DMA (%d)", ret);
	}
int_disable:
	SPIDisableInt(cfg->base, SPI_CC23_INT_MASK);
#else
	ret = spi_context_wait_for_completion(ctx);
	if (ret) {
		SPIDisableInt(cfg->base, SPI_CC23_INT_MASK);
		LOG_ERR("SPI transfer failed (%d)", ret);
	} else {
		LOG_DBG("SPI transfer completed");
	}
#endif

	spi_context_cs_control(ctx, false);

ctx_release:
	spi_context_release(ctx, ret);
	pm_policy_device_power_lock_put(dev);
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

static DEVICE_API(spi, spi_cc23x0_driver_api) = {
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

#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA not ready");
		return -ENODEV;
	}
#endif

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int spi_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct spi_cc23x0_config *cfg = dev->config;
	struct spi_cc23x0_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		SPIDisable(cfg->base);
		CLKCTLDisable(CLKCTL_BASE, CLKCTL_SPI0);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		/* Force SPI to be reconfigured at next transfer */
		data->ctx.config = NULL;
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_SPI_CC23X0_DMA_DRIVEN
#define SPI_CC23X0_DMA_INIT(n)						\
	.dma_dev = DEVICE_DT_GET(TI_CC23X0_DT_INST_DMA_CTLR(n, tx)),	\
	.dma_channel_tx = TI_CC23X0_DT_INST_DMA_CHANNEL(n, tx),		\
	.dma_trigsrc_tx = TI_CC23X0_DT_INST_DMA_TRIGSRC(n, tx),		\
	.dma_channel_rx = TI_CC23X0_DT_INST_DMA_CHANNEL(n, rx),		\
	.dma_trigsrc_rx = TI_CC23X0_DT_INST_DMA_TRIGSRC(n, rx),
#else
#define SPI_CC23X0_DMA_INIT(n)
#endif

#define SPI_CC23X0_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	PM_DEVICE_DT_INST_DEFINE(n, spi_cc23x0_pm_action);		\
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
		.irq_config_func = spi_irq_config_func_##n,		\
		SPI_CC23X0_DMA_INIT(n)					\
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
			      PM_DEVICE_DT_INST_GET(n),			\
			      &spi_cc23x0_data_##n,			\
			      &spi_cc23x0_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_SPI_INIT_PRIORITY,			\
			      &spi_cc23x0_driver_api)

DT_INST_FOREACH_STATUS_OKAY(SPI_CC23X0_INIT)
