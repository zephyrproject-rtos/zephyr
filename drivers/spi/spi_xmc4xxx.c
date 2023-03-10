/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_xmc4xxx);

#include "spi_context.h"

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>

#include <xmc_spi.h>
#include <xmc_usic.h>

struct spi_xmc4xxx_config {
	XMC_USIC_CH_t *spi;
	const struct pinctrl_dev_config *pcfg;
	uint8_t miso_src;
#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct spi_xmc4xxx_data {
	struct spi_context ctx;
};

static void spi_xmc4xxx_shift_frames(const struct device *dev)
{
	struct spi_xmc4xxx_data *data = dev->data;
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint8_t tx_data = 0;
	uint8_t rx_data;
	uint32_t status;

	if (spi_context_tx_buf_on(ctx)) {
		tx_data = ctx->tx_buf[0];
	}

	XMC_SPI_CH_ClearStatusFlag(config->spi,
				   XMC_SPI_CH_STATUS_FLAG_TRANSMIT_SHIFT_INDICATION |
				   XMC_SPI_CH_STATUS_FLAG_RECEIVE_INDICATION |
				   XMC_SPI_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION);

	XMC_SPI_CH_Transmit(config->spi, tx_data, XMC_SPI_CH_MODE_STANDARD);

	spi_context_update_tx(ctx, 1, 1);

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
	return;
#endif

	/* Wait to finish transmitting */
	while (1) {
		status = XMC_SPI_CH_GetStatusFlag(config->spi);
		if (status & XMC_SPI_CH_STATUS_FLAG_TRANSMIT_SHIFT_INDICATION) {
			break;
		}
	}

	/* Wait to finish receiving */
	while (1) {
		status = XMC_SPI_CH_GetStatusFlag(config->spi);
		if (status & (XMC_SPI_CH_STATUS_FLAG_RECEIVE_INDICATION |
			      XMC_SPI_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION)) {
			break;
		}
	}

	rx_data = XMC_SPI_CH_GetReceivedData(config->spi);

	if (spi_context_rx_buf_on(ctx)) {
		*ctx->rx_buf = rx_data;
	}
	spi_context_update_rx(ctx, 1, 1);
}

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
static void spi_xmc4xxx_isr(const struct device *dev)
{
	struct spi_xmc4xxx_data *data = dev->data;
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint8_t rx_data;

	rx_data = XMC_SPI_CH_GetReceivedData(config->spi);

	if (spi_context_rx_buf_on(ctx)) {
		*ctx->rx_buf = rx_data;
	}
	spi_context_update_rx(ctx, 1, 1);

	if (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		spi_xmc4xxx_shift_frames(dev);
		return;
	}

	if (!(ctx->config->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}

	spi_context_complete(ctx, dev, 0);
}
#endif

#define LOOPBACK_SRC 6
static int spi_xmc4xxx_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	int ret;
	struct spi_xmc4xxx_data *data = dev->data;
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint16_t settings = spi_cfg->operation;
	bool CPOL = SPI_MODE_GET(settings) & SPI_MODE_CPOL;
	bool CPHA = SPI_MODE_GET(settings) & SPI_MODE_CPHA;
	XMC_SPI_CH_CONFIG_t usic_cfg = {.baudrate = spi_cfg->frequency};
	XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_t clock_settings;

	if (spi_context_configured(ctx, spi_cfg)) {
		return 0;
	}

	ctx->config = spi_cfg;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		LOG_ERR("Only 8 bit word size is supported");
		return -ENOTSUP;
	}

	ret = XMC_SPI_CH_Stop(config->spi);
	if (ret != XMC_SPI_CH_STATUS_OK) {
		return -EBUSY;
	}
	XMC_SPI_CH_Init(config->spi, &usic_cfg);
	XMC_SPI_CH_Start(config->spi);

	if (SPI_MODE_GET(settings) & SPI_MODE_LOOP) {
		XMC_SPI_CH_SetInputSource(config->spi, XMC_SPI_CH_INPUT_DIN0, LOOPBACK_SRC);
	} else {
		XMC_SPI_CH_SetInputSource(config->spi, XMC_SPI_CH_INPUT_DIN0, config->miso_src);
	}

	if (!CPOL && !CPHA) {
		clock_settings = XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_0_DELAY_ENABLED;
	} else if (!CPOL && CPHA) {
		clock_settings = XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_0_DELAY_DISABLED;
	} else if (CPOL && !CPHA) {
		clock_settings = XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_1_DELAY_ENABLED;
	} else if (CPOL && CPHA) {
		clock_settings = XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_1_DELAY_DISABLED;
	}
	XMC_SPI_CH_ConfigureShiftClockOutput(config->spi, clock_settings,
					     XMC_SPI_CH_BRG_SHIFT_CLOCK_OUTPUT_SCLK);

	if (settings & SPI_TRANSFER_LSB) {
		XMC_SPI_CH_SetBitOrderLsbFirst(config->spi);
	} else {
		XMC_SPI_CH_SetBitOrderMsbFirst(config->spi);
	}

	XMC_SPI_CH_SetWordLength(config->spi, 8);

	return 0;
}

static int spi_xmc4xxx_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs,
				  bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_xmc4xxx_data *data = dev->data;
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t recv_status;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_XMC4XXX_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_xmc4xxx_configure(dev, spi_cfg);
	if (ret) {
		LOG_DBG("SPI config on device %s failed\n", dev->name);
		spi_context_release(ctx, ret);
		return ret;
	}

	/* flush out any received bytes */
	recv_status = XMC_USIC_CH_GetReceiveBufferStatus(config->spi);
	if (recv_status & USIC_CH_RBUFSR_RDV0_Msk) {
		XMC_SPI_CH_GetReceivedData(config->spi);
	}
	if (recv_status & USIC_CH_RBUFSR_RDV1_Msk) {
		XMC_SPI_CH_GetReceivedData(config->spi);
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(ctx, true);

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
	XMC_SPI_CH_EnableEvent(config->spi, XMC_SPI_CH_EVENT_STANDARD_RECEIVE |
					    XMC_SPI_CH_EVENT_ALTERNATIVE_RECEIVE);
	spi_xmc4xxx_shift_frames(dev);
	ret = spi_context_wait_for_completion(ctx);
	/* cs released in isr */
#else
	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		spi_xmc4xxx_shift_frames(dev);
	}

	if (!(spi_cfg->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}
#endif

	spi_context_release(ctx, ret);

	return ret;
}

#if defined(CONFIG_SPI_ASYNC)
static int spi_xmc4xxx_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs,
					spi_callback_t cb,
					void *userdata)
{
	return spi_xmc4xxx_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int spi_xmc4xxx_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	return spi_xmc4xxx_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

static int spi_xmc4xxx_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_xmc4xxx_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static int spi_xmc4xxx_init(const struct device *dev)
{
	struct spi_xmc4xxx_data *data = dev->data;
	const struct spi_xmc4xxx_config *config = dev->config;
	int ret;

	XMC_USIC_CH_Enable(config->spi);

	spi_context_unlock_unconditionally(&data->ctx);

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
	config->irq_config_func(dev);
#endif

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	XMC_SPI_CH_SetInputSource(config->spi, XMC_SPI_CH_INPUT_DIN0, config->miso_src);
	spi_context_cs_configure_all(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_xmc4xxx_driver_api = {
	.transceive = spi_xmc4xxx_transceive_sync,
#if defined(CONFIG_SPI_ASYNC)
	.transceive_async = spi_xmc4xxx_transceive_async,
#endif
	.release = spi_xmc4xxx_release,
};

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)

#define USIC_IRQ_MIN  84
#define IRQS_PER_USIC 6

#define XMC4XXX_IRQ_HANDLER_INIT(index)                                                            \
	static void spi_xmc4xxx_irq_setup_##index(const struct device *dev)                        \
	{                                                                                          \
		const struct spi_xmc4xxx_config *config = dev->config;                             \
		uint8_t service_request;                                                           \
		uint8_t irq_num;                                                                   \
												   \
		irq_num = DT_INST_IRQ_BY_NAME(index, rx, irq);                                     \
		service_request = (irq_num - USIC_IRQ_MIN) % IRQS_PER_USIC;                        \
												   \
		XMC_SPI_CH_SelectInterruptNodePointer(                                             \
			config->spi, XMC_SPI_CH_INTERRUPT_NODE_POINTER_RECEIVE, service_request);  \
		XMC_SPI_CH_SelectInterruptNodePointer(                                             \
			config->spi, XMC_SPI_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE,          \
			service_request);                                                          \
												   \
		XMC_SPI_CH_EnableEvent(config->spi, XMC_SPI_CH_EVENT_STANDARD_RECEIVE |            \
						    XMC_SPI_CH_EVENT_ALTERNATIVE_RECEIVE);         \
												   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rx, irq),                                   \
			    DT_INST_IRQ_BY_NAME(index, rx, priority), spi_xmc4xxx_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rx, irq));                                   \
	}

#define XMC4XXX_IRQ_HANDLER_STRUCT_INIT(index) .irq_config_func = spi_xmc4xxx_irq_setup_##index,

#else
#define XMC4XXX_IRQ_HANDLER_INIT(index)
#define XMC4XXX_IRQ_HANDLER_STRUCT_INIT(index)
#endif

#define XMC4XXX_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	XMC4XXX_IRQ_HANDLER_INIT(index)                                                            \
	static struct spi_xmc4xxx_data xmc4xxx_data_##index = {                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)                           \
			SPI_CONTEXT_INIT_LOCK(xmc4xxx_data_##index, ctx),                          \
		SPI_CONTEXT_INIT_SYNC(xmc4xxx_data_##index, ctx),                                  \
	};                                                                                         \
                                                                                                   \
	static const struct spi_xmc4xxx_config xmc4xxx_config_##index = {                          \
		.spi = (XMC_USIC_CH_t *)DT_INST_REG_ADDR(index),                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.miso_src = DT_INST_ENUM_IDX(index, miso_src),                                     \
		XMC4XXX_IRQ_HANDLER_STRUCT_INIT(index)};                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &spi_xmc4xxx_init, NULL, &xmc4xxx_data_##index,               \
			      &xmc4xxx_config_##index, POST_KERNEL,                                \
			      CONFIG_SPI_INIT_PRIORITY, &spi_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_INIT)
