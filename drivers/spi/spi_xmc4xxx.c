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

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>

#include <xmc_spi.h>
#include <xmc_usic.h>

#define USIC_IRQ_MIN  84
#define USIC_IRQ_MAX  101
#define IRQS_PER_USIC 6

#define SPI_XMC4XXX_DMA_ERROR_FLAG   BIT(0)
#define SPI_XMC4XXX_DMA_RX_DONE_FLAG BIT(1)
#define SPI_XMC4XXX_DMA_TX_DONE_FLAG BIT(2)


#ifdef CONFIG_SPI_XMC4XXX_DMA
static const uint8_t __aligned(4) tx_dummy_data;
#endif

struct spi_xmc4xxx_config {
	XMC_USIC_CH_t *spi;
	const struct pinctrl_dev_config *pcfg;
	uint8_t miso_src;
#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
	void (*irq_config_func)(const struct device *dev);
#endif
#if defined(CONFIG_SPI_XMC4XXX_DMA)
	uint8_t irq_num_tx;
	uint8_t irq_num_rx;
#endif
};

#ifdef CONFIG_SPI_XMC4XXX_DMA
struct spi_xmc4xxx_dma_stream {
	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};
#endif

struct spi_xmc4xxx_data {
	struct spi_context ctx;
#if defined(CONFIG_SPI_XMC4XXX_DMA)
	struct spi_xmc4xxx_dma_stream dma_rx;
	struct spi_xmc4xxx_dma_stream dma_tx;
	struct k_sem status_sem;
	uint8_t dma_status_flags;
	uint8_t dma_completion_flags;
	uint8_t service_request_tx;
	uint8_t service_request_rx;
#endif
};

#if defined(CONFIG_SPI_XMC4XXX_DMA)
static void spi_xmc4xxx_dma_callback(const struct device *dev_dma, void *arg, uint32_t dma_channel,
				     int status)
{
	struct spi_xmc4xxx_data *data = arg;

	if (status != 0) {
		LOG_ERR("DMA callback error on channel %d.", dma_channel);
		data->dma_status_flags |= SPI_XMC4XXX_DMA_ERROR_FLAG;
	} else {
		if (dev_dma == data->dma_tx.dev_dma && dma_channel == data->dma_tx.dma_channel) {
			data->dma_status_flags |= SPI_XMC4XXX_DMA_TX_DONE_FLAG;
		} else if (dev_dma == data->dma_rx.dev_dma &&
			   dma_channel == data->dma_rx.dma_channel) {
			data->dma_status_flags |= SPI_XMC4XXX_DMA_RX_DONE_FLAG;
		} else {
			LOG_ERR("DMA callback channel %d is not valid.", dma_channel);
			data->dma_status_flags |= SPI_XMC4XXX_DMA_ERROR_FLAG;
		}
	}
	k_sem_give(&data->status_sem);
}

#endif

static void spi_xmc4xxx_flush_rx(XMC_USIC_CH_t *spi)
{
	uint32_t recv_status;

	recv_status = XMC_USIC_CH_GetReceiveBufferStatus(spi);
	if (recv_status & USIC_CH_RBUFSR_RDV0_Msk) {
		XMC_SPI_CH_GetReceivedData(spi);
	}
	if (recv_status & USIC_CH_RBUFSR_RDV1_Msk) {
		XMC_SPI_CH_GetReceivedData(spi);
	}
}

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

	spi_context_update_tx(ctx, 1, 1);

	XMC_SPI_CH_Transmit(config->spi, tx_data, XMC_SPI_CH_MODE_STANDARD);

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
	XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_t clock_settings =
		XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_0_DELAY_ENABLED;

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
		LOG_DBG("SPI config on device %s failed", dev->name);
		spi_context_release(ctx, ret);
		return ret;
	}

	spi_xmc4xxx_flush_rx(config->spi);

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

#if defined(CONFIG_SPI_XMC4XXX_DMA)
static int spi_xmc4xxx_dma_rx_tx_done(struct spi_xmc4xxx_data *data)
{
	for (;;) {
		int ret;

		ret = k_sem_take(&data->status_sem, K_MSEC(CONFIG_SPI_XMC4XXX_DMA_TIMEOUT_MSEC));
		if (ret != 0) {
			LOG_ERR("Sem take error %d", ret);
			return ret;
		}
		if (data->dma_status_flags & SPI_XMC4XXX_DMA_ERROR_FLAG) {
			return -EIO;
		}
		if (data->dma_status_flags == data->dma_completion_flags) {
			return 0;
		}
	}
}

static int spi_xmc4xxx_transceive_dma(const struct device *dev, const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      bool asynchronous,
				      spi_callback_t cb, void *userdata)
{
	struct spi_xmc4xxx_data *data = dev->data;
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	struct spi_xmc4xxx_dma_stream *dma_tx = &data->dma_tx;
	struct spi_xmc4xxx_dma_stream *dma_rx = &data->dma_rx;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	if (asynchronous) {
		return -ENOTSUP;
	}

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	k_sem_reset(&data->status_sem);

	ret = spi_xmc4xxx_configure(dev, spi_cfg);
	if (ret) {
		LOG_ERR("SPI config on device %s failed", dev->name);
		spi_context_release(ctx, ret);
		return ret;
	}

	/* stop async isr from triggering */
	irq_disable(config->irq_num_rx);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	spi_context_cs_control(ctx, true);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		int dma_len;
		uint8_t dma_completion_flags = SPI_XMC4XXX_DMA_TX_DONE_FLAG;

		/* make sure the tx is not transmitting */
		while (XMC_USIC_CH_GetTransmitBufferStatus(config->spi) ==
			XMC_USIC_CH_TBUF_STATUS_BUSY) {
		};

		if (data->ctx.rx_len == 0) {
			dma_len = data->ctx.tx_len;
		} else if (data->ctx.tx_len == 0) {
			dma_len = data->ctx.rx_len;
		} else {
			dma_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
		}

		if (ctx->rx_buf) {

			spi_xmc4xxx_flush_rx(config->spi);

			dma_rx->blk_cfg.dest_address = (uint32_t)ctx->rx_buf;
			dma_rx->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
			dma_rx->blk_cfg.block_size = dma_len;
			dma_rx->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

			ret = dma_config(dma_rx->dev_dma, dma_rx->dma_channel, &dma_rx->dma_cfg);
			if (ret < 0) {
				break;
			}

			XMC_SPI_CH_EnableEvent(config->spi, XMC_SPI_CH_EVENT_STANDARD_RECEIVE |
							    XMC_SPI_CH_EVENT_ALTERNATIVE_RECEIVE);
			dma_completion_flags |= SPI_XMC4XXX_DMA_RX_DONE_FLAG;

			ret = dma_start(dma_rx->dev_dma, dma_rx->dma_channel);
			if (ret < 0) {
				break;
			}

		} else {
			XMC_SPI_CH_DisableEvent(config->spi,
						XMC_SPI_CH_EVENT_STANDARD_RECEIVE |
						XMC_SPI_CH_EVENT_ALTERNATIVE_RECEIVE);
		}

		if (ctx->tx_buf) {
			dma_tx->blk_cfg.source_address = (uint32_t)ctx->tx_buf;
			dma_tx->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			dma_tx->blk_cfg.source_address = (uint32_t)&tx_dummy_data;
			dma_tx->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}

		dma_tx->blk_cfg.block_size = dma_len;

		ret = dma_config(dma_tx->dev_dma, dma_tx->dma_channel, &dma_tx->dma_cfg);
		if (ret < 0) {
			break;
		}

		data->dma_status_flags = 0;
		data->dma_completion_flags = dma_completion_flags;

		XMC_SPI_CH_EnableEvent(config->spi, XMC_SPI_CH_EVENT_RECEIVE_START);
		XMC_USIC_CH_TriggerServiceRequest(config->spi, data->service_request_tx);

		ret = dma_start(dma_tx->dev_dma, dma_tx->dma_channel);
		if (ret < 0) {
			break;
		}

		ret = spi_xmc4xxx_dma_rx_tx_done(data);
		if (ret) {
			break;
		}

		spi_context_update_tx(ctx, 1, dma_len);
		spi_context_update_rx(ctx, 1, dma_len);
	}

	if (ret < 0) {
		dma_stop(dma_tx->dev_dma, dma_tx->dma_channel);
		dma_stop(dma_rx->dev_dma, dma_rx->dma_channel);
	}

	if (!(spi_cfg->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)
	irq_enable(config->irq_num_rx);
#endif
	spi_context_release(ctx, ret);

	return ret;
}
#endif

static int spi_xmc4xxx_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
#if defined(CONFIG_SPI_XMC4XXX_DMA)
	struct spi_xmc4xxx_data *data = dev->data;

	if (data->dma_tx.dev_dma != NULL && data->dma_rx.dev_dma != NULL) {
		return spi_xmc4xxx_transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL,
						  NULL);
	}
#endif
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

#if defined(CONFIG_SPI_XMC4XXX_DMA)
static void spi_xmc4xxx_configure_rx_service_requests(const struct device *dev)
{
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_xmc4xxx_data *data = dev->data;

	__ASSERT(config->irq_num_rx >= USIC_IRQ_MIN && config->irq_num_rx <= USIC_IRQ_MAX,
		 "Invalid irq number\n");

	data->service_request_rx = (config->irq_num_rx - USIC_IRQ_MIN) % IRQS_PER_USIC;

	XMC_SPI_CH_SelectInterruptNodePointer(config->spi,
					      XMC_SPI_CH_INTERRUPT_NODE_POINTER_RECEIVE,
					      data->service_request_rx);
	XMC_SPI_CH_SelectInterruptNodePointer(config->spi,
					      XMC_SPI_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE,
					      data->service_request_rx);
}

static void spi_xmc4xxx_configure_tx_service_requests(const struct device *dev)
{
	const struct spi_xmc4xxx_config *config = dev->config;
	struct spi_xmc4xxx_data *data = dev->data;

	__ASSERT(config->irq_num_tx >= USIC_IRQ_MIN && config->irq_num_tx <= USIC_IRQ_MAX,
		 "Invalid irq number\n");

	data->service_request_tx = (config->irq_num_tx - USIC_IRQ_MIN) % IRQS_PER_USIC;

	XMC_USIC_CH_SetInterruptNodePointer(config->spi,
					    XMC_USIC_CH_INTERRUPT_NODE_POINTER_TRANSMIT_BUFFER,
					    data->service_request_tx);
}
#endif

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

#if defined(CONFIG_SPI_XMC4XXX_DMA)
	spi_xmc4xxx_configure_tx_service_requests(dev);
	spi_xmc4xxx_configure_rx_service_requests(dev);

	if (data->dma_rx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_rx.dev_dma)) {
			return -ENODEV;
		}
		data->dma_rx.blk_cfg.source_address = (uint32_t)&config->spi->RBUF;
		data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
		data->dma_rx.dma_cfg.user_data = (void *)data;
	}

	if (data->dma_tx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_tx.dev_dma)) {
			return -ENODEV;
		}
		data->dma_tx.blk_cfg.dest_address =
			(uint32_t)&config->spi->TBUF[XMC_SPI_CH_MODE_STANDARD];
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
		data->dma_tx.dma_cfg.user_data = (void *)data;
	}
	k_sem_init(&data->status_sem, 0, 2);
#endif

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	XMC_SPI_CH_SetInputSource(config->spi, XMC_SPI_CH_INPUT_DIN0, config->miso_src);
	spi_context_cs_configure_all(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_xmc4xxx_driver_api) = {
	.transceive = spi_xmc4xxx_transceive_sync,
#if defined(CONFIG_SPI_ASYNC)
	.transceive_async = spi_xmc4xxx_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_xmc4xxx_release,
};

#if defined(CONFIG_SPI_XMC4XXX_DMA)
#define SPI_DMA_CHANNEL_INIT(index, dir, ch_dir, src_burst, dst_burst)                             \
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = {                                                                               \
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, config),                         \
		.channel_direction = ch_dir,                                                       \
		.channel_priority = DT_INST_DMAS_CELL_BY_NAME(index, dir, priority),               \
		.source_data_size = 1,                                                             \
		.dest_data_size = 1,                                                               \
		.source_burst_length = src_burst,                                                  \
		.dest_burst_length = dst_burst,                                                    \
		.block_count = 1,                                                                  \
		.dma_callback = spi_xmc4xxx_dma_callback,                                          \
		.complete_callback_en = true,                                                      \
	},

#define SPI_DMA_CHANNEL(index, dir, ch_dir, src_burst, dst_burst)                                  \
	.dma_##dir = {COND_CODE_1(                                                                 \
		DT_INST_DMAS_HAS_NAME(index, dir),                                                 \
		(SPI_DMA_CHANNEL_INIT(index, dir, ch_dir, src_burst, dst_burst)), (NULL))},
#else
#define SPI_DMA_CHANNEL(index, dir, ch_dir, src_burst, dst_burst)
#endif

#if defined(CONFIG_SPI_XMC4XXX_INTERRUPT)

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
												   \
		irq_enable(irq_num);                                                               \
	}

#define XMC4XXX_IRQ_HANDLER_STRUCT_INIT(index) .irq_config_func = spi_xmc4xxx_irq_setup_##index,

#else
#define XMC4XXX_IRQ_HANDLER_INIT(index)
#define XMC4XXX_IRQ_HANDLER_STRUCT_INIT(index)
#endif

#if defined(CONFIG_SPI_XMC4XXX_DMA)
#define XMC4XXX_IRQ_DMA_STRUCT_INIT(index)                                                         \
	.irq_num_rx = DT_INST_IRQ_BY_NAME(index, rx, irq),                                         \
	.irq_num_tx = DT_INST_IRQ_BY_NAME(index, tx, irq),
#else
#define XMC4XXX_IRQ_DMA_STRUCT_INIT(index)
#endif

#define XMC4XXX_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	XMC4XXX_IRQ_HANDLER_INIT(index)                                                            \
	static struct spi_xmc4xxx_data xmc4xxx_data_##index = {                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)                           \
			SPI_CONTEXT_INIT_LOCK(xmc4xxx_data_##index, ctx),                          \
		SPI_CONTEXT_INIT_SYNC(xmc4xxx_data_##index, ctx),                                  \
		SPI_DMA_CHANNEL(index, tx, MEMORY_TO_PERIPHERAL, 8, 1)                             \
		SPI_DMA_CHANNEL(index, rx, PERIPHERAL_TO_MEMORY, 1, 8)};                           \
                                                                                                   \
	static const struct spi_xmc4xxx_config xmc4xxx_config_##index = {                          \
		.spi = (XMC_USIC_CH_t *)DT_INST_REG_ADDR(index),                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.miso_src = DT_INST_ENUM_IDX(index, miso_src),                                     \
		XMC4XXX_IRQ_HANDLER_STRUCT_INIT(index)                                             \
		XMC4XXX_IRQ_DMA_STRUCT_INIT(index)};                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, spi_xmc4xxx_init, NULL, &xmc4xxx_data_##index,                \
			      &xmc4xxx_config_##index, POST_KERNEL,                                \
			      CONFIG_SPI_INIT_PRIORITY, &spi_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_INIT)
