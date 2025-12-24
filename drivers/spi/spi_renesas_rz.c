/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_spi

#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include "r_spi.h"
#ifdef CONFIG_SPI_RENESAS_RZ_DMA
#include "r_dmac.h"
#endif
#ifdef CONFIG_SPI_RTIO
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/rtio/rtio.h>
#endif
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rz_spi);

#define LOG_DEV_ERR(dev, format, ...) LOG_ERR("%s:" #format, (dev)->name, ##__VA_ARGS__)

#include "spi_context.h"

#define SPI_RZ_SPSRC_CLR        0xFD80
#define SPI_RZ_TRANSMIT_RECEIVE 0x0
#define SPI_RZ_TX_ONLY          0x1

struct spi_rz_config {
	const struct pinctrl_dev_config *pinctrl_dev;
	const spi_api_t *fsp_api;
	spi_clock_source_t clock_source;
};

struct spi_rz_data {
	struct spi_context ctx;
	uint8_t dfs;
	uint32_t data_len;
	spi_cfg_t *fsp_config;
	spi_instance_ctrl_t *fsp_ctrl;
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif /* CONFIG_SPI_RTIO */
};

#ifdef CONFIG_SPI_RENESAS_RZ_INTERRUPT
void spi_rxi_isr(void);
void spi_txi_isr(void);
void spi_tei_isr(void);
void spi_eri_isr(void);
#endif /* CONFIG_SPI_RENESAS_RZ_INTERRUPT */

#ifdef CONFIG_SPI_RENESAS_RZ_DMA
#define RZ_MASTER_MPU_STADD_DISABLE_RW_PROTECTION  (0x00000000)
#define RZ_MASTER_MPU_ENDADD_DISABLE_RW_PROTECTION (0x00000C00)

void dmac_int_isr(void);
void spi_tx_dmac_callback(spi_instance_ctrl_t *p_ctrl);
void spi_rx_dmac_callback(spi_instance_ctrl_t *p_ctrl);
#endif /* CONFIG_SPI_RENESAS_RZ_DMA */

#ifdef CONFIG_SPI_RTIO
static void spi_rz_iodev_complete(const struct device *dev, int status);
#else
static bool spi_rz_transfer_ongoing(struct spi_rz_data *data)
{
#if defined(CONFIG_SPI_RENESAS_RZ_INTERRUPT)
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
#else
	if (spi_context_total_tx_len(&data->ctx) == spi_context_total_rx_len(&data->ctx)) {
		return (spi_context_tx_on(&data->ctx) && spi_context_rx_on(&data->ctx));
	} else {
		return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
	}
#endif /* CONFIG_SPI_RENESAS_RZ_INTERRUPT */
}
#endif /* CONFIG_SPI_RTIO */

static void spi_callback(spi_callback_args_t *p_args)
{
	struct device *dev = (struct device *)p_args->p_context;
	struct spi_rz_data *data = dev->data;

	switch (p_args->event) {
	case SPI_EVENT_TRANSFER_COMPLETE:
		spi_context_cs_control(&data->ctx, false);
#ifdef CONFIG_SPI_RTIO
		struct spi_rtio *rtio_ctx = data->rtio_ctx;

		if (rtio_ctx->txn_head != NULL) {
			spi_rz_iodev_complete(dev, 0);
		}
#endif
		spi_context_complete(&data->ctx, dev, 0);
		break;
	case SPI_EVENT_ERR_MODE_FAULT:    /* Mode fault error */
	case SPI_EVENT_ERR_READ_OVERFLOW: /* Read overflow error */
	case SPI_EVENT_ERR_PARITY:        /* Parity error */
	case SPI_EVENT_ERR_OVERRUN:       /* Overrun error */
	case SPI_EVENT_ERR_FRAMING:       /* Framing error */
	case SPI_EVENT_ERR_MODE_UNDERRUN: /* Underrun error */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, -EIO);
		break;
	default:
		break;
	}
}

static int spi_rz_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_rz_data *data = dev->data;
	const struct spi_rz_config *config = dev->config;
	spi_extended_cfg_t *spi_extend = (spi_extended_cfg_t *)data->fsp_config->p_extend;
	fsp_err_t err;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (data->fsp_ctrl->open != 0) {
		config->fsp_api->close(data->fsp_ctrl);
	}

	if (spi_cfg->operation & SPI_FRAME_FORMAT_TI) {
		LOG_ERR("TI frame not supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_DEV_ERR(dev, "Only single line mode is supported");
		return -ENOTSUP;
	}

	/* SPI mode */
	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		data->fsp_config->operating_mode = SPI_MODE_SLAVE;
	} else {
		data->fsp_config->operating_mode = SPI_MODE_MASTER;
	}

	/* SPI POLARITY */
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		data->fsp_config->clk_polarity = SPI_CLK_POLARITY_HIGH;
	} else {
		data->fsp_config->clk_polarity = SPI_CLK_POLARITY_LOW;
	}

	/* SPI PHASE */
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		data->fsp_config->clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
	} else {
		data->fsp_config->clk_phase = SPI_CLK_PHASE_EDGE_ODD;
	}

	/* SPI bit order */
	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		data->fsp_config->bit_order = SPI_BIT_ORDER_LSB_FIRST;
	} else {
		data->fsp_config->bit_order = SPI_BIT_ORDER_MSB_FIRST;
	}

	data->dfs = ((SPI_WORD_SIZE_GET(spi_cfg->operation) - 1) / 8) + 1;
	data->fsp_ctrl->bit_width = (spi_bit_width_t)(SPI_WORD_SIZE_GET(spi_cfg->operation) - 1);
	if ((data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_32_BITS) ||
	    (data->fsp_ctrl->bit_width < SPI_BIT_WIDTH_4_BITS)) {
		LOG_ERR("Unsupported SPI word size: %u", SPI_WORD_SIZE_GET(spi_cfg->operation));
		return -ENOTSUP;
	}

	/* SPI slave select polarity */
	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		spi_extend->ssl_polarity = SPI_SSLP_HIGH;
	} else {
		spi_extend->ssl_polarity = SPI_SSLP_LOW;
	}

	/* Calculate bitrate */
	if ((spi_cfg->frequency > 0) && (!(spi_cfg->operation & SPI_OP_MODE_SLAVE))) {
		err = R_SPI_CalculateBitrate(spi_cfg->frequency, config->clock_source,
					     &spi_extend->spck_div);

		if (err != FSP_SUCCESS) {
			LOG_DEV_ERR(dev, "spi: bitrate calculate error: %d", err);
			return -ENOSYS;
		}
	}

	spi_extend->spi_comm = SPI_COMMUNICATION_FULL_DUPLEX;

	if (spi_cs_is_gpio(spi_cfg) || !IS_ENABLED(CONFIG_SPI_USE_HW_SS)) {
		if ((spi_cfg->operation & SPI_OP_MODE_SLAVE) &&
		    (data->fsp_config->clk_phase == SPI_CLK_PHASE_EDGE_ODD)) {
			LOG_DEV_ERR(dev, "The CPHA bit must be set to 1 slave mode");
			return -EIO;
		}
		spi_extend->spi_clksyn = SPI_SSL_MODE_CLK_SYN;
	} else {
		spi_extend->spi_clksyn = SPI_SSL_MODE_SPI;
		switch (spi_cfg->slave) {
		case 0:
			spi_extend->ssl_select = SPI_SSL_SELECT_SSL0;
			break;
		case 1:
			spi_extend->ssl_select = SPI_SSL_SELECT_SSL1;
			break;
		case 2:
			spi_extend->ssl_select = SPI_SSL_SELECT_SSL2;
			break;
		case 3:
			spi_extend->ssl_select = SPI_SSL_SELECT_SSL3;
			break;
		default:
			LOG_ERR("Invalid SSL");
			return -EINVAL;
		}
	}

	spi_extend->receive_fifo_threshold = 0;
	spi_extend->transmit_fifo_threshold = 0;

	/* Open module r_spi. */
	err = config->fsp_api->open(data->fsp_ctrl, data->fsp_config);
	if (err != FSP_SUCCESS) {
		LOG_ERR("R_SPI_Open error: %d", err);
		return -EIO;
	}

	data->ctx.config = spi_cfg;

	return 0;
}

#if !defined(CONFIG_SPI_RENESAS_RZ_INTERRUPT) && !defined(CONFIG_SPI_RENESAS_RZ_DMA)
static int spi_rz_spi_transceive_data(struct spi_rz_data *data)
{
	uint32_t tx;
	uint32_t rx;

	if (spi_context_tx_buf_on(&data->ctx)) {
		if (data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_16_BITS) {
			tx = *(uint32_t *)(data->ctx.tx_buf);
		} else if (data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_8_BITS) {
			tx = *(uint16_t *)(data->ctx.tx_buf);
		} else {
			tx = *(uint8_t *)(data->ctx.tx_buf);
		}
	} else {
		tx = 0U;
	}

	while (!data->fsp_ctrl->p_regs->SPSR_b.SPTEF) {
	}

	/* TX transfer */
	if (data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_16_BITS) {
		data->fsp_ctrl->p_regs->SPDR = (uint32_t)tx;
	} else if (data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_8_BITS) {
		data->fsp_ctrl->p_regs->SPDR = (uint16_t)tx;
	} else {
		data->fsp_ctrl->p_regs->SPDR = (uint8_t)tx;
	}

	data->fsp_ctrl->p_regs->SPSRC_b.SPTEFC = 1; /* Clear SPTEF flag */
	spi_context_update_tx(&data->ctx, data->dfs, 1);

	if (data->fsp_ctrl->p_regs->SPCR_b.TXMD == 0x0) {
		while (!data->fsp_ctrl->p_regs->SPSR_b.SPRF) {
		}

		if (SPI_BIT_WIDTH_16_BITS < data->fsp_ctrl->bit_width) {
			rx = (uint32_t)data->fsp_ctrl->p_regs->SPDR;
		} else if (SPI_BIT_WIDTH_8_BITS >= data->fsp_ctrl->bit_width) {
			rx = (uint8_t)data->fsp_ctrl->p_regs->SPDR;
		} else {
			rx = (uint16_t)data->fsp_ctrl->p_regs->SPDR;
		}

		/* RX transfer */
		if (spi_context_rx_buf_on(&data->ctx)) {

			/* Read data from Data Register */
			if (data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_16_BITS) {
				UNALIGNED_PUT(rx, (uint32_t *)data->ctx.rx_buf);
			} else if (data->fsp_ctrl->bit_width > SPI_BIT_WIDTH_8_BITS) {
				UNALIGNED_PUT(rx, (uint16_t *)data->ctx.rx_buf);
			} else {
				UNALIGNED_PUT(rx, (uint8_t *)data->ctx.rx_buf);
			}
		}
		spi_context_update_rx(&data->ctx, data->dfs, 1);
		data->fsp_ctrl->p_regs->SPSRC_b.SPRFC = 1; /* Clear SPRF flag */
	}
	return 0;
}
#endif /* !CONFIG_SPI_RENESAS_RZ_INTERRUPT && !CONFIG_SPI_RENESAS_RZ_DMA */

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_rz_data *data = dev->data;
	struct spi_context *spi_ctx = &data->ctx;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_RENESAS_RZ_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif
	spi_context_lock(spi_ctx, asynchronous, cb, userdata, spi_cfg);
	/* Configure module SPI. */
	ret = spi_rz_configure(dev, spi_cfg);
	if (ret) {
		spi_context_release(spi_ctx, ret);
		return -EIO;
	}
#ifndef CONFIG_SPI_RTIO
	/* Setup tx buffer and rx buffer info. */
	spi_context_buffers_setup(spi_ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(spi_ctx, true);
#if defined(CONFIG_SPI_RENESAS_RZ_INTERRUPT) || defined(CONFIG_SPI_RENESAS_RZ_DMA)
	const struct spi_rz_config *config = dev->config;

	if (!spi_context_total_tx_len(&data->ctx) && !spi_context_total_rx_len(&data->ctx)) {
		goto end_transceive;
	}
	if (data->ctx.rx_len == 0) {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? spi_context_total_tx_len(&data->ctx)
					 : data->ctx.tx_len;
	} else if (data->ctx.tx_len == 0) {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? spi_context_total_rx_len(&data->ctx)
					 : data->ctx.rx_len;
	} else {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? MAX(spi_context_total_tx_len(&data->ctx),
					       spi_context_total_rx_len(&data->ctx))
					 : MIN(data->ctx.tx_len, data->ctx.rx_len);
	}

	if (data->ctx.tx_buf == NULL) { /* If there is only the rx buffer */
		ret = config->fsp_api->read(data->fsp_ctrl, data->ctx.rx_buf, data->data_len,
					    data->fsp_ctrl->bit_width);
	} else if (data->ctx.rx_buf == NULL) { /* If there is only the tx buffer */
		ret = config->fsp_api->write(data->fsp_ctrl, data->ctx.tx_buf, data->data_len,
					     data->fsp_ctrl->bit_width);
	} else {
		ret = config->fsp_api->writeRead(data->fsp_ctrl, data->ctx.tx_buf, data->ctx.rx_buf,
						 data->data_len, data->fsp_ctrl->bit_width);
	}
	if (ret) {
		LOG_ERR("Async transmit fail: %d", ret);
		spi_context_cs_control(spi_ctx, false);
		spi_context_release(spi_ctx, ret);
		return -EIO;
	}
	ret = spi_context_wait_for_completion(spi_ctx);
end_transceive:
#else
	/* Enable the SPI transfer */
	data->fsp_ctrl->p_regs->SPCR_b.TXMD = SPI_RZ_TRANSMIT_RECEIVE;
	if (!spi_context_rx_on(&data->ctx)) {
		data->fsp_ctrl->p_regs->SPCR_b.TXMD = SPI_RZ_TX_ONLY; /* tx only */
	}

	/* Configure data length based on the selected bit width . */
	uint32_t spcmd0 = data->fsp_ctrl->p_regs->SPCMD[0];

	spcmd0 &= (uint32_t)~R_SPI0_SPCMD_SPB_Msk;
	spcmd0 |= (uint32_t)(data->fsp_ctrl->bit_width) << R_SPI0_SPCMD_SPB_Pos;
	data->fsp_ctrl->p_regs->SPCMD[0] = spcmd0;

	/* FIFO clear */
	data->fsp_ctrl->p_regs->SPFCR_b.SPFRST = 1;
	/* Enable the SPI Transfer */
	data->fsp_ctrl->p_regs->SPCR_b.SPE = 1;

	do {
		spi_rz_spi_transceive_data(data);
	} while (spi_rz_transfer_ongoing(data));

	/* Wait for transmit complete */
	while (data->fsp_ctrl->p_regs->SPSR_b.IDLNF) {
	}

	/* Disable the SPI Transfer */
	data->fsp_ctrl->p_regs->SPCR_b.SPE = 0x0;

#endif /* CONFIG_SPI_RENESAS_RZ_INTERRUPT || CONFIG_SPI_RENESAS_RZ_DMA */

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(spi_ctx) && !ret) {
		ret = spi_ctx->recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

	spi_context_cs_control(spi_ctx, false);

#else
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	ret = spi_rtio_transceive(rtio_ctx, spi_cfg, tx_bufs, rx_bufs);

#endif /* CONFIG_SPI_RTIO */
	spi_context_release(spi_ctx, ret);

	return ret;
}

static int spi_rz_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

static int spi_rz_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_rz_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_rz_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				   void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

#ifdef CONFIG_SPI_RTIO

static inline void spi_rz_iodev_prepare_start(const struct device *dev)
{
	struct spi_rz_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct spi_dt_spec *spi_dt_spec = rtio_ctx->txn_curr->sqe.iodev->data;
	struct spi_config *spi_config = &spi_dt_spec->config;
	int err;

	err = spi_rz_configure(dev, spi_config);
	if (err != 0) {
		LOG_ERR("RTIO config spi error: %d", err);
		spi_rz_iodev_complete(dev, err);
		return;
	}
	spi_context_cs_control(&data->ctx, true);
}

static void spi_rz_iodev_start(const struct device *dev)
{
	struct spi_rz_data *data = dev->data;
	const struct spi_rz_config *config = dev->config;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct rtio_sqe *sqe = &rtio_ctx->txn_curr->sqe;
	int ret = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
		data->data_len = sqe->rx.buf_len / data->dfs;
		ret = config->fsp_api->read(data->fsp_ctrl, sqe->rx.buf, data->data_len,
					    data->fsp_ctrl->bit_width);
		break;
	case RTIO_OP_TX:
		data->data_len = sqe->tx.buf_len / data->dfs;
		ret = config->fsp_api->write(data->fsp_ctrl, sqe->tx.buf, data->data_len,
					     data->fsp_ctrl->bit_width);
		break;
	case RTIO_OP_TINY_TX:
		data->data_len = sqe->tiny_tx.buf_len / data->dfs;
		ret = config->fsp_api->write(data->fsp_ctrl, sqe->tiny_tx.buf, data->data_len,
					     data->fsp_ctrl->bit_width);
		break;
	case RTIO_OP_TXRX:
		data->data_len = sqe->txrx.buf_len / data->dfs;
		ret = config->fsp_api->writeRead(data->fsp_ctrl, sqe->txrx.tx_buf, sqe->txrx.rx_buf,
						 data->data_len, data->fsp_ctrl->bit_width);
		break;
	default:
		spi_rz_iodev_complete(dev, -EINVAL);
		break;
	}

	if (ret != 0) {
		spi_rz_iodev_complete(dev, ret);
	}
}

static void spi_rz_iodev_complete(const struct device *dev, int status)
{
	struct spi_rz_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (!status && rtio_ctx->txn_curr->sqe.flags & RTIO_SQE_TRANSACTION) {
		rtio_ctx->txn_curr = rtio_txn_next(rtio_ctx->txn_curr);
		spi_rz_iodev_start(dev);
	} else {
		spi_context_cs_control(&data->ctx, false);

		/*
		 * Submit the result of the operation to the completion queue
		 * This may start the next asynchronous request if one is available
		 */
		if (spi_rtio_complete(rtio_ctx, status)) {
			spi_rz_iodev_prepare_start(dev);
			spi_rz_iodev_start(dev);
		}
	}
}

static void spi_rz_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_rz_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	/* Submit sqe to the queue */
	if (spi_rtio_submit(rtio_ctx, iodev_sqe)) {
		spi_rz_iodev_prepare_start(dev);
		spi_rz_iodev_start(dev);
	}
}

#endif /* CONFIG_SPI_RTIO */

static DEVICE_API(spi, spi_rz_driver_api) = {
	.transceive = spi_rz_transceive_sync,
	.release = spi_rz_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_rz_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rz_iodev_submit,
#endif /* CONFIG_SPI_RTIO */
};

#ifdef CONFIG_SPI_RENESAS_RZ_INTERRUPT
static void spi_rz_rxi_isr(const struct device *dev) __attribute__((unused));
static void spi_rz_txi_isr(const struct device *dev) __attribute__((unused));

static void spi_rz_rxi_isr(const struct device *dev)
{
#ifndef CONFIG_SPI_SLAVE
	ARG_UNUSED(dev);
	spi_rxi_isr();
#else
	struct spi_rz_data *data = dev->data;

	spi_rxi_isr();

	if (spi_context_is_slave(&data->ctx) && data->fsp_ctrl->rx_count == data->fsp_ctrl->count) {
		if (data->ctx.rx_buf != NULL && data->ctx.tx_buf != NULL) {
			data->ctx.recv_frames = MIN(spi_context_total_tx_len(&data->ctx),
						    spi_context_total_rx_len(&data->ctx));
		} else if (data->ctx.tx_buf == NULL) {
			data->ctx.recv_frames = data->data_len;
		}
		R_BSP_IrqDisable(data->fsp_config->tei_irq);

		/* Writing 0 to SPE generatates a TXI IRQ. Disable the TXI IRQ.
		 * (See Section 38.2.1 SPI Control Register in the RA6T2 manual R01UH0886EJ0100).
		 */
		R_BSP_IrqDisable(data->fsp_config->txi_irq);

		/* Disable the SPI Transfer. */
		data->fsp_ctrl->p_regs->SPCR_b.SPE = 0;

		/* Re-enable the TXI IRQ and clear the pending IRQ. */
		R_BSP_IrqEnable(data->fsp_config->txi_irq);

		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
	}

#endif
}

static void spi_rz_txi_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	spi_txi_isr();
}

static void spi_rz_tei_isr(const struct device *dev)
{
#ifndef CONFIG_SPI_RTIO
	struct spi_rz_data *data = dev->data;
	const struct spi_rz_config *config = dev->config;
	int ret = 0;

	if (data->fsp_ctrl->rx_count == data->fsp_ctrl->count) {
#ifdef CONFIG_SPI_RENESAS_RZ_DMA
		if (data->ctx.rx_len < data->data_len && data->fsp_config->p_transfer_tx != NULL &&
		    data->fsp_config->p_transfer_rx != NULL) {
			data->ctx.rx_len = 0;
			spi_context_update_rx(&data->ctx, data->dfs,
					      spi_context_total_rx_len(&data->ctx));
		} else {
			spi_context_update_rx(&data->ctx, data->dfs, data->data_len);
		}
#else
		spi_context_update_rx(&data->ctx, data->dfs, data->data_len);
#endif /* CONFIG_SPI_RENESAS_RZ_DMA */
	}
	if (data->fsp_ctrl->tx_count == data->fsp_ctrl->count) {
#ifdef CONFIG_SPI_RENESAS_RZ_DMA
		if (data->ctx.tx_len < data->data_len && data->fsp_config->p_transfer_tx != NULL &&
		    data->fsp_config->p_transfer_rx != NULL) {
			data->ctx.tx_len = 0;
		}
#endif /* CONFIG_SPI_RENESAS_RZ_DMA */
		spi_context_update_tx(&data->ctx, data->dfs, data->data_len);
	}

	if (spi_rz_transfer_ongoing(data)) {
		data->fsp_ctrl->p_regs->SPCR_b.SPE = 0U;
		if (data->ctx.rx_len == 0) {
			data->data_len = data->ctx.tx_len;
			ret = config->fsp_api->write(data->fsp_ctrl, data->ctx.tx_buf,
						     data->data_len, data->fsp_ctrl->bit_width);
		} else if (data->ctx.tx_len == 0) {
			data->data_len = data->ctx.rx_len;
			ret = config->fsp_api->read(data->fsp_ctrl, data->ctx.rx_buf,
						    data->data_len, data->fsp_ctrl->bit_width);
		} else {
			data->data_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
			ret = config->fsp_api->writeRead(data->fsp_ctrl, data->ctx.tx_buf,
							 data->ctx.rx_buf, data->data_len,
							 data->fsp_ctrl->bit_width);
		}
		if (ret) {
			LOG_ERR("Async transmit fail: %d", ret);
		}
	} else {
		spi_tei_isr();
	}
#else
	spi_tei_isr();
#endif /* CONFIG_SPI_RTIO */
}

static void spi_rz_eri_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	spi_eri_isr();
}
#endif /* CONFIG_SPI_RENESAS_RZ_INTERRUPT */

static int spi_rz_init(const struct device *dev)
{
	const struct spi_rz_config *config = dev->config;
	struct spi_rz_data *data = dev->data;
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
#ifdef CONFIG_SPI_RTIO
	spi_rtio_init(data->rtio_ctx, dev);
#endif /* CONFIG_SPI_RTIO */
	spi_context_unlock_unconditionally(&data->ctx);

#if defined(CONFIG_SPI_RENESAS_RZ_DMA) && defined(CONFIG_CPU_CORTEX_R52)
	if (data->fsp_config->p_transfer_tx != NULL && data->fsp_config->p_transfer_rx != NULL) {
		uint8_t region_num = BSP_FEATURE_BSP_MASTER_MPU_REGION_TYPE == 1 ? 8 : 16;
		const dmac_extended_cfg_t *dmac_tx_ext =
			(const dmac_extended_cfg_t *)
				data->fsp_config->p_transfer_tx->p_cfg->p_extend;
		const dmac_extended_cfg_t *dmac_rx_ext =
			(const dmac_extended_cfg_t *)
				data->fsp_config->p_transfer_tx->p_cfg->p_extend;

		/* Disable register protection for Master-MPU related registers. */
		R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SYSTEM);

		if (dmac_tx_ext->unit == 0 || dmac_rx_ext->unit == 0) {
			for (uint8_t i = 0; i < region_num; i++) {
				R_MPU0->RGN[i].STADD = RZ_MASTER_MPU_STADD_DISABLE_RW_PROTECTION;
				R_MPU0->RGN[i].ENDADD = RZ_MASTER_MPU_ENDADD_DISABLE_RW_PROTECTION;
			}
		}
		if (dmac_tx_ext->unit == 1 || dmac_rx_ext->unit == 1) {
			for (uint8_t i = 0; i < region_num; i++) {
				R_MPU1->RGN[i].STADD = RZ_MASTER_MPU_STADD_DISABLE_RW_PROTECTION;
				R_MPU1->RGN[i].ENDADD = RZ_MASTER_MPU_ENDADD_DISABLE_RW_PROTECTION;
			}
		}

		/* Enable register protection for Master-MPU related registers. */
		R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SYSTEM);
	}
#endif /* CONFIG_SPI_RENESAS_RZ_DMA && CONFIG_CPU_CORTEX_R52 */

	return 0;
}

#ifdef CONFIG_SPI_RENESAS_RZ_INTERRUPT
#define RZ_SPI_CONNECT_IRQ(n, irq_name, irq_handler)                                               \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, irq_name, irq),                                         \
		    DT_INST_IRQ_BY_NAME(n, irq_name, priority), irq_handler,                       \
		    DEVICE_DT_INST_GET(n), DT_INST_IRQ_BY_NAME(n, irq_name, flags));               \
	irq_enable(DT_INST_IRQ_BY_NAME(n, irq_name, irq));

#define RZ_SPI_IRQ_INIT_DEFAULT(n)                                                                 \
	do {                                                                                       \
		RZ_SPI_CONNECT_IRQ(n, rxi, spi_rz_rxi_isr);                                        \
		RZ_SPI_CONNECT_IRQ(n, txi, spi_rz_txi_isr);                                        \
		RZ_SPI_CONNECT_IRQ(n, tei, spi_rz_tei_isr);                                        \
		RZ_SPI_CONNECT_IRQ(n, eri, spi_rz_eri_isr);                                        \
	} while (0)

#ifdef CONFIG_SPI_RENESAS_RZ_DMA
#define SPI_RZ_DMA_CHANNEL_CONFIG(id, dir) DT_INST_DMAS_CELL_BY_NAME(id, dir, config)

#define SPI_RZ_DMA_MODE(config)           ((config) & 0x1)
#define SPI_RZ_DMA_SRC_DATA_SIZE(config)  ((config >> 1) & 0x7)
#define SPI_RZ_DMA_DEST_DATA_SIZE(config) ((config >> 4) & 0x7)
#define SPI_RZ_DMA_SRC_ADDR_MODE(config)  ((config >> 7) & 0x1)
#define SPI_RZ_DMA_DEST_ADDR_MODE(config) ((config >> 8) & 0x1)

#define SPI_RZ_DMA_DEFINE(n, dir, TRIG, spi_channel)                                               \
	static dmac_instance_ctrl_t g_transfer##n##_##dir##_ctrl;                                  \
	static void g_spi##n##_##dir##_transfer_callback(transfer_callback_args_t *p_args)         \
	{                                                                                          \
		FSP_PARAMETER_NOT_USED(p_args);                                                    \
		spi_##dir##_dmac_callback(&g_spi##n##_ctrl);                                       \
	}                                                                                          \
	transfer_info_t g_transfer##n##_##dir##_info = {                                           \
		.dest_addr_mode = SPI_RZ_DMA_DEST_ADDR_MODE(SPI_RZ_DMA_CHANNEL_CONFIG(n, dir)),    \
		.src_addr_mode = SPI_RZ_DMA_SRC_ADDR_MODE(SPI_RZ_DMA_CHANNEL_CONFIG(n, dir)),      \
		.mode = SPI_RZ_DMA_MODE(SPI_RZ_DMA_CHANNEL_CONFIG(n, dir)),                        \
		.p_dest = (void *)NULL,                                                            \
		.p_src = (void const *)NULL,                                                       \
		.length = 0,                                                                       \
		.src_size = SPI_RZ_DMA_SRC_DATA_SIZE(SPI_RZ_DMA_CHANNEL_CONFIG(n, dir)),           \
		.dest_size = SPI_RZ_DMA_DEST_DATA_SIZE(SPI_RZ_DMA_CHANNEL_CONFIG(n, dir)),         \
		.p_next1_src = NULL,                                                               \
		.p_next1_dest = NULL,                                                              \
		.next1_length = 1,                                                                 \
	};                                                                                         \
	const dmac_extended_cfg_t g_transfer##n##_##dir##_extend = {                               \
		.unit = DT_PROP(DT_INST_DMAS_CTLR_BY_NAME(n, dir), dma_unit),                      \
		.channel = DT_INST_DMAS_CELL_BY_NAME(n, dir, channel),                             \
		.dmac_int_irq = DT_IRQ_BY_NAME(                                                    \
			DT_INST_DMAS_CTLR_BY_NAME(n, dir),                                         \
			UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)), irq),            \
		.dmac_int_ipl = DT_IRQ_BY_NAME(                                                    \
			DT_INST_DMAS_CTLR_BY_NAME(n, dir),                                         \
			UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)), priority),       \
		.dmac_int_irq_detect_type = 0,                                                     \
		.activation_source =                                                               \
			UTIL_CAT(UTIL_CAT(ELC_EVENT_SPI, spi_channel), UTIL_CAT(_SP, TRIG)),       \
		.ack_mode = DMAC_ACK_MODE_BUS_CYCLE_MODE,                                          \
		.detection_mode = (dmac_detection_t)((0) << 2 | (1) << 1 | (0) << 0),              \
		.activation_request_source_select = DMAC_REQUEST_DIRECTION_SOURCE_MODULE,          \
		.dmac_mode = DMAC_MODE_SELECT_REGISTER,                                            \
		.p_descriptor = NULL,                                                              \
		.transfer_interval = 0,                                                            \
		.channel_scheduling = DMAC_CHANNEL_SCHEDULING_FIXED,                               \
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
		.p_api = &g_transfer_on_dmac,                                                      \
	};

#define RZ_SPI_CONNECT_DMA_IRQ(n, dir)                                                             \
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, dir),                              \
				   UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)), irq), \
		    DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, dir),                              \
				   UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)),       \
				   priority),                                                      \
		    dmac_int_isr, NULL,                                                            \
		    DT_IRQ_BY_NAME(DT_INST_DMAS_CTLR_BY_NAME(n, dir),                              \
				   UTIL_CAT(ch, DT_INST_DMAS_CELL_BY_NAME(n, dir, channel)),       \
				   flags));
#define RZ_SPI_DMA_IRQ_INIT(n)                                                                     \
	do {                                                                                       \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                                      \
		(                                                                                  \
			RZ_SPI_CONNECT_DMA_IRQ(n, rx);                                             \
			RZ_SPI_CONNECT_DMA_IRQ(n, tx);                                             \
			RZ_SPI_CONNECT_IRQ(n, eri, spi_rz_eri_isr);                                \
			RZ_SPI_CONNECT_IRQ(n, tei, spi_rz_tei_isr);                                \
		),                                                                                 \
		(                                                                                  \
			RZ_SPI_CONNECT_IRQ(n, rxi, spi_rz_rxi_isr);                                \
			RZ_SPI_CONNECT_IRQ(n, txi, spi_rz_txi_isr);                                \
			RZ_SPI_CONNECT_IRQ(n, eri, spi_rz_eri_isr);                                \
			RZ_SPI_CONNECT_IRQ(n, tei, spi_rz_tei_isr);                                \
		)) \
	} while (0)
#endif /* CONFIG_SPI_RENESAS_RZ_DMA */
#endif /* CONFIG_SPI_RENESAS_RZ_INTERRUPT */

#ifdef CONFIG_SPI_RENESAS_RZ_INTERRUPT
#ifdef CONFIG_SPI_RENESAS_RZ_DMA
#define RZ_SPI_IRQ_INIT(n) RZ_SPI_DMA_IRQ_INIT(n)
#else
#define RZ_SPI_IRQ_INIT(n) RZ_SPI_IRQ_INIT_DEFAULT(n)
#endif /* CONFIG_SPI_RENESAS_RZ_DMA */
#else
#define RZ_SPI_IRQ_INIT(n)
#endif /* CONFIG_SPI_RENESAS_RZ_INTERRUPT */

#if defined(CONFIG_SPI_RTIO)
#define SPI_RZ_RTIO_DEFINE(n)                                                                      \
	SPI_RTIO_DEFINE(spi_rz_rtio_##n, CONFIG_SPI_RTIO_SQ_SIZE, CONFIG_SPI_RTIO_CQ_SIZE)
#else
#define SPI_RZ_RTIO_DEFINE(n)
#endif /* CONFIG_SPI_RTIO */

#define SPI_RZ_INIT(n)                                                                             \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SPI_RZ_RTIO_DEFINE(n);                                                                     \
	static spi_instance_ctrl_t g_spi##n##_ctrl;                                                \
	static spi_extended_cfg_t g_spi_##n##_cfg_extend = {                                       \
		.spi_clksyn = SPI_SSL_MODE_SPI,                                                    \
		.spi_comm = SPI_COMMUNICATION_FULL_DUPLEX,                                         \
		.ssl_polarity = SPI_SSLP_LOW,                                                      \
		.ssl_select = SPI_SSL_SELECT_SSL0,                                                 \
		.mosi_idle = SPI_MOSI_IDLE_VALUE_FIXING_DISABLE,                                   \
		.parity = SPI_PARITY_MODE_DISABLE,                                                 \
		.byte_swap = SPI_BYTE_SWAP_DISABLE,                                                \
		.clock_source = SPI_CLOCK_SOURCE_SPI0ASYNCCLK,                                     \
		.spck_div =                                                                        \
			{                                                                          \
				.spbr = 4,                                                         \
				.brdv = 0,                                                         \
			},                                                                         \
		.spck_delay = SPI_DELAY_COUNT_1,                                                   \
		.ssl_negation_delay = SPI_DELAY_COUNT_1,                                           \
		.next_access_delay = SPI_DELAY_COUNT_1,                                            \
		.transmit_fifo_threshold = 0,                                                      \
		.receive_fifo_threshold = 0,                                                       \
		.receive_data_ready_detect_adjustment = 0,                                         \
		.master_receive_clock = SPI_MASTER_RECEIVE_CLOCK_MRIOCLK,                          \
		.mrioclk_analog_delay = SPI_MRIOCLK_ANALOG_DELAY_NODELAY,                          \
		.mrclk_digital_delay = SPI_MRCLK_DIGITAL_DELAY_CLOCK_0,                            \
	};                                                                                         \
	IF_ENABLED(CONFIG_SPI_RENESAS_RZ_DMA, (IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(n), dmas),\
				    (SPI_RZ_DMA_DEFINE(n, tx, TI, DT_INST_PROP(n, channel)))))) \
	IF_ENABLED(CONFIG_SPI_RENESAS_RZ_DMA, (IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(n), dmas),\
				    (SPI_RZ_DMA_DEFINE(n, rx, RI, DT_INST_PROP(n, channel)))))) \
	static spi_cfg_t g_spi_##n##_config = {                                                    \
		.channel = DT_INST_PROP(n, channel),                                               \
		.eri_irq = DT_INST_IRQ_BY_NAME(n, eri, irq),                                       \
		.rxi_ipl = DT_INST_IRQ_BY_NAME(n, rxi, priority),                                  \
		.txi_ipl = DT_INST_IRQ_BY_NAME(n, txi, priority),                                  \
		.eri_ipl = DT_INST_IRQ_BY_NAME(n, eri, priority),                                  \
		.operating_mode = SPI_MODE_MASTER,                                                 \
		.clk_phase = SPI_CLK_PHASE_EDGE_ODD,                                               \
		.clk_polarity = SPI_CLK_POLARITY_LOW,                                              \
		.mode_fault = SPI_MODE_FAULT_ERROR_ENABLE,                                         \
		.bit_order = SPI_BIT_ORDER_MSB_FIRST,                                              \
		.p_callback = spi_callback,                                                        \
		.p_context = DEVICE_DT_INST_GET(n),                                                \
		.p_extend = &g_spi_##n##_cfg_extend,                                               \
		.tei_irq = DT_INST_IRQ_BY_NAME(n, tei, irq),                                       \
		COND_CODE_1(CONFIG_SPI_RENESAS_RZ_DMA,                                             \
			(                                                                          \
				COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), dmas),                \
					(                                                          \
						.rxi_irq = FSP_INVALID_VECTOR,                     \
						.txi_irq = FSP_INVALID_VECTOR,                     \
						.p_transfer_tx = &g_transfer##n##_tx,              \
						.p_transfer_rx = &g_transfer##n##_rx,              \
						),                                                 \
					(                                                          \
						.rxi_irq = DT_INST_IRQ_BY_NAME(n, rxi, irq),       \
						.txi_irq = DT_INST_IRQ_BY_NAME(n, txi, irq),       \
						.p_transfer_tx = NULL,                             \
						.p_transfer_rx = NULL,                             \
					)                                                          \
				)                                                                  \
			),                                                                         \
			(                                                                          \
				.rxi_irq = DT_INST_IRQ_BY_NAME(n, rxi, irq),                       \
				.txi_irq = DT_INST_IRQ_BY_NAME(n, txi, irq),                       \
				.p_transfer_tx = NULL,                                             \
				.p_transfer_rx = NULL,                                             \
			)) };                   \
	static const struct spi_rz_config spi_rz_config_##n = {                                    \
		.pinctrl_dev = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                  \
		.fsp_api = &g_spi_on_spi,                                                          \
		.clock_source = (spi_clock_source_t)DT_INST_PROP(n, clk_src),                      \
	};                                                                                         \
                                                                                                   \
	static struct spi_rz_data spi_rz_data_##n = {                                              \
		SPI_CONTEXT_INIT_LOCK(spi_rz_data_##n, ctx),                                       \
		SPI_CONTEXT_INIT_SYNC(spi_rz_data_##n, ctx),                                       \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx).fsp_ctrl = &g_spi##n##_ctrl,  \
		.fsp_config = &g_spi_##n##_config,                                                 \
		IF_ENABLED(CONFIG_SPI_RTIO,                                                        \
					 (.rtio_ctx = &spi_rz_rtio_##n,)) };                       \
                                                                                                   \
	static int spi_rz_init_##n(const struct device *dev)                                       \
	{                                                                                          \
		int err = spi_rz_init(dev);                                                        \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		RZ_SPI_IRQ_INIT(n);                                                                \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, &spi_rz_init_##n, NULL, &spi_rz_data_##n, &spi_rz_config_##n,     \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_rz_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_RZ_INIT)
