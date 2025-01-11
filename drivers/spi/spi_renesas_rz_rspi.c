/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_rspi_spi

#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include "r_rspi.h"
#include "r_dmac_b.h"
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rz_spi);

#define LOG_DEV_ERR(dev, format, ...) LOG_ERR("%s:" #format, (dev)->name, ##__VA_ARGS__)

#include "spi_context.h"

struct spi_rz_rspi_config {
	const struct pinctrl_dev_config *pinctrl_dev;
	const spi_api_t *fsp_api;
};

struct spi_rz_rspi_data {
	struct spi_context ctx;
	uint8_t dfs;
	uint32_t data_len;
	spi_cfg_t *fsp_config;
	rspi_instance_ctrl_t *fsp_ctrl;
	rspi_extended_cfg_t fsp_extend_config;
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif /* CONFIG_SPI_RTIO */
};

volatile bool wait_int_complete = true;

#if defined(CONFIG_SPI_ASYNC)

void dmac_b_int_isr(void);
extern void rspi_tx_dmac_callback(rspi_instance_ctrl_t *p_ctrl);
extern void rspi_rx_dmac_callback(rspi_instance_ctrl_t *p_ctrl);

#else

void rspi_rxi_isr(void);
void rspi_txi_isr(void);

static void spi_rz_rspi_rxi_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	rspi_rxi_isr();
}

static void spi_rz_rspi_txi_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	rspi_txi_isr();
}

#endif /* CONFIG_SPI_ASYNC */

/* ISR called in the event that an error occurs (Ex: OVERFLOW) */
void rspi_eri_isr(void);
static void spi_rz_rspi_eri_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	rspi_eri_isr();
}

static void spi_callbacks(spi_callback_args_t *p_args)
{
	struct device *dev = (struct device *)p_args->p_context;
	struct spi_rz_rspi_data *data = dev->data;

	switch (p_args->event) {
	case SPI_EVENT_TRANSFER_COMPLETE:
		wait_int_complete = false;
#ifndef CONFIG_SPI_RTIO
		struct spi_context *spi_ctx = &data->ctx;

		spi_context_update_tx(spi_ctx, data->dfs, 1);
		spi_context_update_rx(spi_ctx, data->dfs, 1);
		if (!spi_context_rx_buf_on(spi_ctx) && !spi_context_tx_buf_on(spi_ctx)) {
			spi_context_complete(&data->ctx, dev, 0);
		}
#endif /* CONFIG_SPI_RTIO */
		break;
	case SPI_EVENT_ERR_MODE_FAULT:    /* Mode fault error */
	case SPI_EVENT_ERR_READ_OVERFLOW: /* Read overflow error */
	case SPI_EVENT_ERR_PARITY:        /* Parity error */
	case SPI_EVENT_ERR_OVERRUN:       /* Overrun error */
	case SPI_EVENT_ERR_FRAMING:       /* Framing error */
	case SPI_EVENT_ERR_MODE_UNDERRUN: /* Underrun error */
		spi_context_complete(&data->ctx, dev, -EIO);
		break;
	default:
		break;
	}
}

static int spi_rz_rspi_init(const struct device *dev)
{
	const struct spi_rz_rspi_config *config = dev->config;
	struct spi_rz_rspi_data *data = dev->data;
	uint32_t rate;
	int ret;

	ret = pinctrl_apply_state(config->pinctrl_dev, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("pinctrl_apply_state fail: %d", ret);
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("spi_context_cs_configure_all fail: %d", ret);
		return ret;
	}

	rate = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_P0CLK);
	if (rate < 0) {
		LOG_ERR("Failed to get clk, rate: %d", rate);
		return ret;
	}
#if CONFIG_SPI_RTIO
	spi_rtio_init(data->rtio_ctx, dev);
#endif /* CONFIG_SPI_RTIO */
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_rz_rspi_configure(const struct device *dev, const struct spi_config *config)
{
	struct spi_rz_rspi_data *data = dev->data;
	spi_bit_width_t spi_width;
	fsp_err_t err;

	if (spi_context_configured(&data->ctx, config)) {
		/* This configuration is already in use */
		return 0;
	}

	if (data->fsp_ctrl->open != 0) {
		R_RSPI_Close(data->fsp_ctrl);
	}

	if (config->operation & SPI_FRAME_FORMAT_TI) {
		LOG_ERR("TI frame not supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_DEV_ERR(dev, "Only single line mode is supported");
		return -ENOTSUP;
	}

	/* SPI mode */
	if (config->operation & SPI_OP_MODE_SLAVE) {
		data->fsp_config->operating_mode = SPI_MODE_SLAVE;
	} else {
		data->fsp_config->operating_mode = SPI_MODE_MASTER;
	}

	/* SPI POLARITY */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		data->fsp_config->clk_polarity = SPI_CLK_POLARITY_HIGH;
	} else {
		data->fsp_config->clk_polarity = SPI_CLK_POLARITY_LOW;
	}

	/* SPI PHASE */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		data->fsp_config->clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
	} else {
		data->fsp_config->clk_phase = SPI_CLK_PHASE_EDGE_ODD;
	}

	/* SPI bit order */
	if (config->operation & SPI_TRANSFER_LSB) {
		data->fsp_config->bit_order = SPI_BIT_ORDER_LSB_FIRST;
	} else {
		data->fsp_config->bit_order = SPI_BIT_ORDER_MSB_FIRST;
	}

	/* SPI bit width */
	spi_width = (spi_bit_width_t)(SPI_WORD_SIZE_GET(config->operation) - 1);
	if (spi_width > SPI_BIT_WIDTH_16_BITS) {
		data->dfs = 4;
		data->fsp_ctrl->bit_width = SPI_BIT_WIDTH_32_BITS;
	} else if (spi_width > SPI_BIT_WIDTH_8_BITS) {
		data->dfs = 2;
		data->fsp_ctrl->bit_width = SPI_BIT_WIDTH_16_BITS;
	} else {
		data->dfs = 1;
		data->fsp_ctrl->bit_width = SPI_BIT_WIDTH_8_BITS;
	}

	/* SPI slave select polarity */
	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		data->fsp_extend_config.ssl_polarity = RSPI_SSLP_HIGH;
	} else {
		data->fsp_extend_config.ssl_polarity = RSPI_SSLP_LOW;
	}

	/* Calculate bitrate */
	if (config->frequency > 0) {
		err = R_RSPI_CalculateBitrate(config->frequency, &data->fsp_extend_config.spck_div);
		if (err != FSP_SUCCESS) {
			LOG_DEV_ERR(dev, "rspi: bitrate calculate error: %d", err);
			return -ENOSYS;
		}
	}

	data->fsp_config->p_extend = &data->fsp_extend_config;
	/* Add callback, which will be called when transfer completed or error occur. */
	data->fsp_config->p_callback = spi_callbacks;
	/* Data is passed into spi_callbacks. */
	data->fsp_config->p_context = dev;
	/* Open module RSPI. */
	err = R_RSPI_Open(data->fsp_ctrl, data->fsp_config);
	if (err != FSP_SUCCESS) {
		LOG_ERR("R_RSPI_Open error: %d", err);
		return -EINVAL;
	}

	data->ctx.config = config;

	return 0;
}

static int transceiver(const struct device *dev, const struct spi_config *config,
		       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		       bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_rz_rspi_data *data = dev->data;
	struct spi_context *spi_ctx = &data->ctx;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_lock(spi_ctx, asynchronous, cb, userdata, config);

#if CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	ret = spi_rtio_transceive(rtio_ctx, config, tx_bufs, rx_bufs);
#else
	/* Configure module RSPI. */
	ret = spi_rz_rspi_configure(dev, config);
	if (ret) {
		goto out;
	}
	/* Setup tx buffer and rx buffer info. */
	spi_context_buffers_setup(spi_ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(spi_ctx, true);
	fsp_err_t err;
	/* This loop ensures that the buffer will be fully transmitted and received. */
	while (spi_context_rx_buf_on(spi_ctx) || spi_context_tx_buf_on(spi_ctx)) {
		if (!spi_context_tx_buf_on(spi_ctx)) { /* If there is only the rx buffer.*/
			err = R_RSPI_Read(data->fsp_ctrl, spi_ctx->rx_buf, 1,
					  data->fsp_ctrl->bit_width);
		} else if (!spi_context_rx_buf_on(spi_ctx)) { /* If there is only the tx buffer.*/
			err = R_RSPI_Write(data->fsp_ctrl, spi_ctx->tx_buf, 1,
					   data->fsp_ctrl->bit_width);
		} else {
			err = R_RSPI_WriteRead(data->fsp_ctrl, spi_ctx->tx_buf, spi_ctx->rx_buf, 1,
					       data->fsp_ctrl->bit_width);
		}

		/* Wait for transmision transfer. */
		while (wait_int_complete) {
		}

		wait_int_complete = true;
	}
	ret = spi_context_wait_for_completion(spi_ctx);

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(spi_ctx) && !ret) {
		ret = spi_ctx->recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

	spi_context_cs_control(spi_ctx, false);
out:
#endif /* CONFIG_SPI_RTIO */
	spi_context_release(spi_ctx, ret);

	return ret;
}

static int spi_rz_rspi_transceive(const struct device *dev, const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	return transceiver(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

static int spi_rz_rspi_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_rz_rspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_rz_rspi_transceive_async(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					void *userdata)
{
	return transceiver(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

#ifdef CONFIG_SPI_RTIO

static int spi_rz_rspi_iodev_transceiver(const struct device *dev, const uint8_t *tx_buf,
					 uint8_t *rx_buf, uint32_t buf_len)
{
	struct spi_rz_rspi_data *data = dev->data;
	fsp_err_t err;

	while (buf_len > 0) {
		if (tx_buf == NULL) {
			err = R_RSPI_Read(data->fsp_ctrl, rx_buf, 1, data->fsp_ctrl->bit_width);
			rx_buf++;
		} else if (rx_buf == NULL) {
			err = R_RSPI_Write(data->fsp_ctrl, tx_buf, 1, data->fsp_ctrl->bit_width);
			tx_buf++;
		} else {
			err = R_RSPI_WriteRead(data->fsp_ctrl, tx_buf, rx_buf, 1,
					       data->fsp_ctrl->bit_width);
			tx_buf++;
			rx_buf++;
		}

		if (err != FSP_SUCCESS) {
			return -EIO;
		}

		/* Wait for transmision transfer. */
		while (wait_int_complete) {
		}
		wait_int_complete = true;

		buf_len--;
	}

	return 0;
}

static inline void spi_rz_rspi_iodev_prepare_start(const struct device *dev)
{
	struct spi_rz_rspi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct spi_dt_spec *spi_dt_spec = rtio_ctx->txn_curr->sqe.iodev->data;
	struct spi_config *config = &spi_dt_spec->config;
	int err;

	err = spi_rz_rspi_configure(dev, config);
	if (err != 0) {
		LOG_ERR("RTIO config spi error: %d", err);
	}
	spi_context_cs_control(&data->ctx, true);
}

static void spi_rz_rspi_iodev_start(const struct device *dev)
{
	struct spi_rz_rspi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct rtio_sqe *sqe = &rtio_ctx->txn_curr->sqe;
	int ret = 0;

	while (rtio_ctx->txn_curr != NULL) {
		switch (sqe->op) {
		case RTIO_OP_RX:
			ret = spi_rz_rspi_iodev_transceiver(dev, NULL, sqe->rx.buf,
							    sqe->rx.buf_len);
			break;
		case RTIO_OP_TX:
			ret = spi_rz_rspi_iodev_transceiver(dev, sqe->tx.buf, NULL,
							    sqe->tx.buf_len);
			break;
		case RTIO_OP_TINY_TX:
			ret = spi_rz_rspi_iodev_transceiver(dev, sqe->tiny_tx.buf, NULL,
							    sqe->tiny_tx.buf_len);
			break;
		case RTIO_OP_TXRX:
			ret = spi_rz_rspi_iodev_transceiver(dev, sqe->txrx.tx_buf, sqe->txrx.rx_buf,
							    sqe->txrx.buf_len);
			break;
		default:
			LOG_ERR("Invalid op %d for sqe %p\n", sqe->op, (void *)sqe);
			ret = -EINVAL;
		}

		/* Check if the next SQE is part of the transaction */
		if (!ret && rtio_ctx->txn_curr->sqe.flags & RTIO_SQE_TRANSACTION) {
			/*  Get the next SQE */
			rtio_ctx->txn_curr = rtio_txn_next(rtio_ctx->txn_curr);
			sqe = &rtio_ctx->txn_curr->sqe;
		} else {
			spi_context_cs_control(&data->ctx, false);

			/*
			 * Submit the result of the operation to the completion queue
			 * This may start the next asynchronous request if one is available
			 */
			if (spi_rtio_complete(rtio_ctx, ret)) {
				spi_rz_rspi_iodev_prepare_start(dev);
				sqe = &rtio_ctx->txn_curr->sqe;
			}
		}
	}
	spi_context_complete(&data->ctx, dev, 0);
}

static void spi_rz_rspi_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_rz_rspi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	/* Submit sqe to the queue */
	if (spi_rtio_submit(rtio_ctx, iodev_sqe)) {
		spi_rz_rspi_iodev_prepare_start(dev);
		spi_rz_rspi_iodev_start(dev);
	}
}
#endif /* CONFIG_SPI_RTIO */

static DEVICE_API(spi, spi_rz_rspi_driver_api) = {
	.transceive = spi_rz_rspi_transceive,
	.release = spi_rz_rspi_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_rz_rspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rz_rspi_iodev_submit,
#endif /* CONFIG_SPI_RTIO */
};

#define RZ_DMA_CHANNEL_CONFIG(id, dir) DT_INST_DMAS_CELL_BY_NAME(id, dir, config)

#define RZ_DMA_MODE(config)           ((config) & 0x1)
#define RZ_DMA_SRC_DATA_SIZE(config)  ((config >> 1) & 0x7)
#define RZ_DMA_DEST_DATA_SIZE(config) ((config >> 4) & 0x7)
#define RZ_DMA_SRC_ADDR_MODE(config)  ((config >> 7) & 0x1)
#define RZ_DMA_DEST_ADDR_MODE(config) ((config >> 8) & 0x1)

#define RSPI_DMA_RZG_DEFINE(n, dir, TRIG, spi_channel)                                             \
	static dmac_b_instance_ctrl_t g_transfer##n##_##dir##_ctrl;                                \
	static void g_spi##n##_##dir##_transfer_callback(dmac_b_callback_args_t *p_args)           \
	{                                                                                          \
		FSP_PARAMETER_NOT_USED(p_args);                                                    \
		rspi_##dir##_dmac_callback(&g_spi##n##_ctrl);                                      \
	}                                                                                          \
	transfer_info_t g_transfer##n##_##dir##_info = {                                           \
		.dest_addr_mode = RZ_DMA_DEST_ADDR_MODE(RZ_DMA_CHANNEL_CONFIG(n, dir)),            \
		.src_addr_mode = RZ_DMA_SRC_ADDR_MODE(RZ_DMA_CHANNEL_CONFIG(n, dir)),              \
		.mode = RZ_DMA_MODE(RZ_DMA_CHANNEL_CONFIG(n, dir)),                                \
		.p_dest = (void *)NULL,                                                            \
		.p_src = (void const *)NULL,                                                       \
		.length = 0,                                                                       \
		.src_size = RZ_DMA_SRC_DATA_SIZE(RZ_DMA_CHANNEL_CONFIG(n, dir)),                   \
		.dest_size = RZ_DMA_DEST_DATA_SIZE(RZ_DMA_CHANNEL_CONFIG(n, dir)),                 \
		.p_next1_src = NULL,                                                               \
		.p_next1_dest = NULL,                                                              \
		.next1_length = 1,                                                                 \
	};                                                                                         \
	const dmac_b_extended_cfg_t g_transfer##n##_##dir##_extend = {                             \
		.unit = 0,                                                                         \
		.channel = DT_INST_DMAS_CELL_BY_NAME(n, dir, channel),                             \
		.dmac_int_irq = DT_IRQ_BY_NAME(                                                    \
			DT_INST_DMAS_CTLR_BY_NAME(n, dir),                                         \
			UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)), irq),            \
		.dmac_int_ipl = DT_IRQ_BY_NAME(                                                    \
			DT_INST_DMAS_CTLR_BY_NAME(n, dir),                                         \
			UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)), priority),       \
		.activation_source = UTIL_CAT(DMAC_TRIGGER_EVENT_RSPI_SP##TRIG, spi_channel),      \
		.ack_mode = DMAC_B_ACK_MODE_MASK_DACK_OUTPUT,                                      \
		.external_detection_mode = DMAC_B_EXTERNAL_DETECTION_NO_DETECTION,                 \
		.internal_detection_mode = DMAC_B_INTERNAL_DETECTION_NO_DETECTION,                 \
		.activation_request_source_select = DMAC_B_REQUEST_DIRECTION_SOURCE_MODULE,        \
		.dmac_mode = DMAC_B_MODE_SELECT_REGISTER,                                          \
		.continuous_setting = DMAC_B_CONTINUOUS_SETTING_TRANSFER_ONCE,                     \
		.transfer_interval = 0,                                                            \
		.channel_scheduling = DMAC_B_CHANNEL_SCHEDULING_FIXED,                             \
		.p_callback = g_spi##n##_##dir##_transfer_callback,                                \
		.p_context = NULL,                                                                 \
	};                                                                                         \
	const transfer_cfg_t g_transfer##n##_##dir##_cfg = {                                       \
		.p_info = &g_transfer##n##_##dir##_info,                                           \
		.p_extend = &g_transfer##n##_##dir##_extend,                                       \
	};                                                                                         \
	const transfer_instance_t g_transfer##n##_##dir = {                                        \
		.p_ctrl = &g_transfer##n##_##dir##_ctrl,                                           \
		.p_cfg = &g_transfer##n##_##dir##_cfg,                                             \
		.p_api = &g_transfer_on_dmac_b,                                                    \
	};

#if defined(CONFIG_SPI_ASYNC)

#define RZ_RSPI_IRQ_INIT(n)                                                                        \
	do {                                                                                       \
		IRQ_CONNECT(                                                                       \
			DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, rx),                           \
				       UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, rx, channel)),    \
				       irq),                                                       \
			DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, rx),                           \
				       UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, rx, channel)),    \
				       priority),                                                  \
			dmac_b_int_isr, DEVICE_DT_INST_GET(n), 0);                                 \
		IRQ_CONNECT(                                                                       \
			DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, tx),                           \
				       UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, tx, channel)),    \
				       irq),                                                       \
			DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, tx),                           \
				       UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, tx, channel)),    \
				       priority),                                                  \
			dmac_b_int_isr, DEVICE_DT_INST_GET(n), 0);                                 \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, error, irq),                                    \
			    DT_INST_IRQ_BY_NAME(n, error, priority), spi_rz_rspi_eri_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, error, irq));                                    \
	} while (0)

#else

#define RZ_RSPI_IRQ_INIT(n)                                                                        \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, rx, irq), DT_INST_IRQ_BY_NAME(n, rx, priority), \
			    spi_rz_rspi_rxi_isr, DEVICE_DT_INST_GET(n), 0);                        \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, tx, irq), DT_INST_IRQ_BY_NAME(n, tx, priority), \
			    spi_rz_rspi_txi_isr, DEVICE_DT_INST_GET(n), 0);                        \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, error, irq),                                    \
			    DT_INST_IRQ_BY_NAME(n, error, priority), spi_rz_rspi_eri_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, rx, irq));                                       \
		irq_enable(DT_INST_IRQ_BY_NAME(n, tx, irq));                                       \
		irq_enable(DT_INST_IRQ_BY_NAME(n, error, irq));                                    \
	} while (0)

#endif /* CONFIG_SPI_ASYNC */

#define SPI_RZ_RSPI_RTIO_DEFINE(n)                                                                 \
	SPI_RTIO_DEFINE(spi_rz_rspi_rtio_##n, CONFIG_SPI_RENESAS_RZ_RTIO_SQ_SIZE,                  \
			CONFIG_SPI_RENESAS_RZ_RTIO_CQ_SIZE)

#define SPI_RZ_RSPI_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	COND_CODE_1(CONFIG_SPI_RTIO, (SPI_RZ_RSPI_RTIO_DEFINE(n)),                                 \
			());                              \
	static rspi_instance_ctrl_t g_spi##n##_ctrl;                                               \
	static rspi_extended_cfg_t g_spi_##n##_cfg_extend = {                                      \
		.ssl_polarity = RSPI_SSLP_LOW,                                                     \
		.mosi_idle = RSPI_MOSI_IDLE_VALUE_FIXING_DISABLE,                                  \
		.spck_div =                                                                        \
			{                                                                          \
				.spbr = 4,                                                         \
				.brdv = 0,                                                         \
			},                                                                         \
		.spck_delay = RSPI_DELAY_COUNT_1,                                                  \
		.ssl_negation_delay = RSPI_DELAY_COUNT_1,                                          \
		.next_access_delay = RSPI_DELAY_COUNT_1,                                           \
		.ssl_level_keep = RSPI_SSL_LEVEL_KEEP_DISABLE,                                     \
		.rx_trigger_level = RSPI_RX_TRIGGER_24,                                            \
		.tx_trigger_level = RSPI_TX_TRIGGER_4,                                             \
	};                                                                                         \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(n, tx),                                                \
				    (RSPI_DMA_RZG_DEFINE(n, tx, TI, DT_INST_PROP(n, channel))))  \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(n, rx),                                                \
				    (RSPI_DMA_RZG_DEFINE(n, rx, RI, DT_INST_PROP(n, channel))))  \
	static spi_cfg_t g_spi_##n##_config = {                                                    \
		.channel = DT_INST_PROP(n, channel),                                               \
		.eri_irq = DT_INST_IRQ_BY_NAME(n, error, irq),                                     \
		.rxi_ipl = DT_INST_IRQ_BY_NAME(n, rx, priority),                                   \
		.txi_ipl = DT_INST_IRQ_BY_NAME(n, tx, priority),                                   \
		.eri_ipl = DT_INST_IRQ_BY_NAME(n, error, priority),                                \
		.operating_mode = SPI_MODE_MASTER,                                                 \
		.clk_phase = SPI_CLK_PHASE_EDGE_ODD,                                               \
		.clk_polarity = SPI_CLK_POLARITY_LOW,                                              \
		.mode_fault = SPI_MODE_FAULT_ERROR_ENABLE,                                         \
		.bit_order = SPI_BIT_ORDER_MSB_FIRST,                                              \
		.p_callback = NULL,                                                                \
		.p_context = NULL,                                                                 \
		.p_extend = &g_spi_##n##_cfg_extend,                                               \
		COND_CODE_1(CONFIG_SPI_ASYNC,                                                      \
			(                                                                          \
			.rxi_irq = FSP_INVALID_VECTOR,                                             \
			.txi_irq = FSP_INVALID_VECTOR,                                             \
			.p_transfer_tx = &g_transfer##n##_tx,                                      \
			.p_transfer_rx = &g_transfer##n##_rx,),                                    \
			(                                                                          \
			.rxi_irq = DT_INST_IRQ_BY_NAME(n, rx, irq),                                \
			.txi_irq = DT_INST_IRQ_BY_NAME(n, tx, irq),                                \
			.p_transfer_tx = NULL,                                                     \
			.p_transfer_rx = NULL,)) };       \
	static const struct spi_rz_rspi_config spi_rz_rspi_config_##n = {                          \
		.pinctrl_dev = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                  \
		.fsp_api = &g_spi_on_rspi,                                                         \
	};                                                                                         \
	static struct spi_rz_rspi_data spi_rz_rspi_data_##n = {                                    \
		SPI_CONTEXT_INIT_LOCK(spi_rz_rspi_data_##n, ctx),                                  \
		SPI_CONTEXT_INIT_SYNC(spi_rz_rspi_data_##n, ctx),                                  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx).fsp_ctrl = &g_spi##n##_ctrl,  \
		.fsp_config = &g_spi_##n##_config,                                                 \
		IF_ENABLED(CONFIG_SPI_RTIO,                                                        \
					 (.rtio_ctx = &spi_rz_rspi_rtio_##n,)) };                  \
	static int spi_rz_rspi_init_##n(const struct device *dev)                                  \
	{                                                                                          \
		int err = spi_rz_rspi_init(dev);                                                   \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		RZ_RSPI_IRQ_INIT(n);                                                               \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, &spi_rz_rspi_init_##n, NULL, &spi_rz_rspi_data_##n,               \
			      &spi_rz_rspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,      \
			      &spi_rz_rspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_RZ_RSPI_INIT)
