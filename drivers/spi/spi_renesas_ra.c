/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_spi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <instances/r_dtc.h>
#include <instances/r_spi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ra_spi);

#include "spi_context.h"

#if defined(CONFIG_SPI_INTERRUPT)
void spi_rxi_isr(void);
void spi_txi_isr(void);
void spi_tei_isr(void);
void spi_eri_isr(void);
#endif

struct ra_spi_config {
	const struct pinctrl_dev_config *pcfg;
};

struct ra_spi_data {
	struct spi_context ctx;
	uint8_t dfs;
	struct st_spi_instance_ctrl spi;
	struct st_spi_cfg fsp_config;
	struct st_spi_extended_cfg fsp_config_extend;
#if CONFIG_SPI_INTERRUPT
	uint32_t data_len;
#endif
#if defined(CONFIG_SPI_RA_DTC)
	/* RX */
	struct st_transfer_instance rx_transfer;
	struct st_dtc_instance_ctrl rx_transfer_ctrl;
	struct st_transfer_info rx_transfer_info DTC_TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg rx_transfer_cfg;
	struct st_dtc_extended_cfg rx_transfer_cfg_extend;

	/* TX */
	struct st_transfer_instance tx_transfer;
	struct st_dtc_instance_ctrl tx_transfer_ctrl;
	struct st_transfer_info tx_transfer_info DTC_TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg tx_transfer_cfg;
	struct st_dtc_extended_cfg tx_transfer_cfg_extend;
#endif
};

static void spi_cb(spi_callback_args_t *p_args)
{
	struct device *dev = (struct device *)p_args->p_context;
	struct ra_spi_data *data = dev->data;

	switch (p_args->event) {
	case SPI_EVENT_TRANSFER_COMPLETE:
		spi_context_cs_control(&data->ctx, false);
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

static int ra_spi_configure(const struct device *dev, const struct spi_config *config)
{
	struct ra_spi_data *data = dev->data;
	fsp_err_t fsp_err;
	uint8_t word_size = SPI_WORD_SIZE_GET(config->operation);

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (data->spi.open != 0) {
		R_SPI_Close(&data->spi);
	}

	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
		return -ENOTSUP;
	}

	if (!((word_size >= 8 && word_size <= 16) || word_size == 20 || word_size == 24 ||
	      word_size == 32)) {
		LOG_ERR("Unsupported SPI word size: %u", word_size);
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		data->fsp_config.operating_mode = SPI_MODE_SLAVE;
	} else {
		data->fsp_config.operating_mode = SPI_MODE_MASTER;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		data->fsp_config.clk_polarity = SPI_CLK_POLARITY_HIGH;
	} else {
		data->fsp_config.clk_polarity = SPI_CLK_POLARITY_LOW;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		data->fsp_config.clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
	} else {
		if (data->fsp_config.operating_mode == SPI_MODE_MASTER) {
			data->fsp_config.clk_phase = SPI_CLK_PHASE_EDGE_ODD;
		} else {
			LOG_ERR("Invalid clock phase");
			return -EINVAL;
		}
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		data->fsp_config.bit_order = SPI_BIT_ORDER_LSB_FIRST;
	} else {
		data->fsp_config.bit_order = SPI_BIT_ORDER_MSB_FIRST;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		data->fsp_config_extend.ssl_polarity = SPI_SSLP_HIGH;
	} else {
		data->fsp_config_extend.ssl_polarity = SPI_SSLP_LOW;
	}

	if (!(config->operation & SPI_OP_MODE_SLAVE)) {
		LOG_INF("frequency: %d", config->frequency);
		fsp_err = R_SPI_CalculateBitrate(config->frequency,
						 &data->fsp_config_extend.spck_div);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("spi frequency calculate error %d", fsp_err);
			return -EIO;
		}
	}

	data->fsp_config_extend.spi_comm = SPI_COMMUNICATION_FULL_DUPLEX;
	if (spi_cs_is_gpio(config) || !IS_ENABLED(CONFIG_SPI_USE_HW_SS)) {
		data->fsp_config_extend.spi_clksyn = SPI_SSL_MODE_CLK_SYN;
	} else {
		data->fsp_config_extend.spi_clksyn = SPI_SSL_MODE_SPI;
		switch (config->slave) {
		case 0:
			data->fsp_config_extend.ssl_select = SPI_SSL_SELECT_SSL0;
			break;
		case 1:
			data->fsp_config_extend.ssl_select = SPI_SSL_SELECT_SSL1;
			break;
		case 2:
			data->fsp_config_extend.ssl_select = SPI_SSL_SELECT_SSL2;
			break;
		case 3:
			data->fsp_config_extend.ssl_select = SPI_SSL_SELECT_SSL3;
			break;
		default:
			LOG_ERR("Invalid SSL");
			return -EINVAL;
		}
	}

	data->fsp_config.p_extend = &data->fsp_config_extend;

	data->fsp_config.p_callback = spi_cb;
	data->fsp_config.p_context = (void *)dev;
	fsp_err = R_SPI_Open(&data->spi, &data->fsp_config);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SPI_Open error: %d", fsp_err);
		return -EIO;
	}
	data->ctx.config = config;

	return 0;
}

static bool ra_spi_transfer_ongoing(struct ra_spi_data *data)
{
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
}

#ifndef CONFIG_SPI_INTERRUPT
static int ra_spi_transceive_slave(struct ra_spi_data *data)
{
	R_SPI0_Type *p_spi_reg = data->spi.p_regs;
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->ctx.config->operation) - 1);

	if (p_spi_reg->SPSR_b.SPTEF && spi_context_tx_buf_on(&data->ctx)) {
		uint32_t tx;

		if (data->ctx.tx_buf != NULL) {
			if (data->dfs > 2) {
				tx = *(uint32_t *)(data->ctx.tx_buf);
			} else if (data->dfs > 1) {
				tx = *(uint16_t *)(data->ctx.tx_buf);
			} else {
				tx = *(uint8_t *)(data->ctx.tx_buf);
			}
		} else {
			tx = 0;
		}

		/* Update data register */
		if (SPI_BIT_WIDTH_16_BITS < spi_width) {
			p_spi_reg->SPDR = (uint32_t)tx;
		} else if (SPI_BIT_WIDTH_8_BITS >= spi_width) {
			p_spi_reg->SPDR_BY = (uint8_t)tx;
		} else {
			p_spi_reg->SPDR_HA = (uint16_t)tx;
		}

		spi_context_update_tx(&data->ctx, data->dfs, 1);
	} else {
		p_spi_reg->SPCR_b.SPTIE = 0;
	}

	if (p_spi_reg->SPSR_b.SPRF && spi_context_rx_buf_on(&data->ctx)) {
		uint32_t rx;

		/* Update RX data */
		if (SPI_BIT_WIDTH_16_BITS < spi_width) {
			rx = (uint32_t)p_spi_reg->SPDR;
		} else if (SPI_BIT_WIDTH_8_BITS >= spi_width) {
			rx = (uint8_t)p_spi_reg->SPDR_BY;
		} else {
			rx = (uint16_t)p_spi_reg->SPDR_HA;
		}

		if (data->dfs > 2) {
			UNALIGNED_PUT(rx, (uint32_t *)data->ctx.rx_buf);
		} else if (data->dfs > 1) {
			UNALIGNED_PUT(rx, (uint16_t *)data->ctx.rx_buf);
		} else {
			UNALIGNED_PUT(rx, (uint8_t *)data->ctx.rx_buf);
		}
		spi_context_update_rx(&data->ctx, data->dfs, 1);
	}

	return 0;
}

static int ra_spi_transceive_master(struct ra_spi_data *data)
{
	R_SPI0_Type *p_spi_reg = data->spi.p_regs;
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->ctx.config->operation) - 1);
	uint32_t tx;
	uint32_t rx;

	/* Tx transfer */
	if (spi_context_tx_buf_on(&data->ctx)) {
		if (data->dfs > 2) {
			tx = *(uint32_t *)(data->ctx.tx_buf);
		} else if (data->dfs > 1) {
			tx = *(uint16_t *)(data->ctx.tx_buf);
		} else {
			tx = *(uint8_t *)(data->ctx.tx_buf);
		}
	} else {
		tx = 0U;
	}

	while (!p_spi_reg->SPSR_b.SPTEF) {
	}

	/* Update data register */
	if (SPI_BIT_WIDTH_16_BITS < spi_width) {
		p_spi_reg->SPDR = (uint32_t)tx;
	} else if (SPI_BIT_WIDTH_8_BITS >= spi_width) {
		p_spi_reg->SPDR_BY = (uint8_t)tx;
	} else {
		p_spi_reg->SPDR_HA = (uint16_t)tx;
	}

	spi_context_update_tx(&data->ctx, data->dfs, 1);

	if (p_spi_reg->SPCR_b.TXMD == 0x0) {
		while (!p_spi_reg->SPSR_b.SPRF) {
		}

		/* Update RX data */
		if (SPI_BIT_WIDTH_16_BITS < spi_width) {
			rx = (uint32_t)p_spi_reg->SPDR;
		} else if (SPI_BIT_WIDTH_8_BITS >= spi_width) {
			rx = (uint8_t)p_spi_reg->SPDR_BY;
		} else {
			rx = (uint16_t)p_spi_reg->SPDR_HA;
		}

		if (spi_context_rx_buf_on(&data->ctx)) {
			if (data->dfs > 2) {
				UNALIGNED_PUT(rx, (uint32_t *)data->ctx.rx_buf);
			} else if (data->dfs > 1) {
				UNALIGNED_PUT(rx, (uint16_t *)data->ctx.rx_buf);
			} else {
				UNALIGNED_PUT(rx, (uint8_t *)data->ctx.rx_buf);
			}
		}
		spi_context_update_rx(&data->ctx, data->dfs, 1);
	}

	return 0;
}

static int ra_spi_transceive_data(struct ra_spi_data *data)
{

	uint16_t operation = data->ctx.config->operation;

	if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_MASTER) {
		ra_spi_transceive_master(data);
	} else {
		ra_spi_transceive_slave(data);
	}

	return 0;
}
#endif

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct ra_spi_data *data = dev->data;
	R_SPI0_Type *p_spi_reg;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	ret = ra_spi_configure(dev, config);
	if (ret) {
		goto end;
	}
	data->dfs = ((SPI_WORD_SIZE_GET(config->operation) - 1) / 8) + 1;
	p_spi_reg = data->spi.p_regs;
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->ctx.config->operation) - 1);
	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	spi_context_cs_control(&data->ctx, true);

	if ((!spi_context_tx_buf_on(&data->ctx)) && (!spi_context_rx_buf_on(&data->ctx))) {
		/* If current buffer has no data, do nothing */
		goto end;
	}

#ifdef CONFIG_SPI_INTERRUPT
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

	if (data->ctx.rx_buf == NULL) {
		R_SPI_Write(&data->spi, data->ctx.tx_buf, data->data_len, spi_width);
	} else if (data->ctx.tx_buf == NULL) {
		R_SPI_Read(&data->spi, data->ctx.rx_buf, data->data_len, spi_width);
	} else {
		R_SPI_WriteRead(&data->spi, data->ctx.tx_buf, data->ctx.rx_buf, data->data_len,
				spi_width);
	}

	ret = spi_context_wait_for_completion(&data->ctx);

#else
	p_spi_reg->SPCR_b.TXMD = (0x0);
	if (!spi_context_tx_on(&data->ctx)) {
		p_spi_reg->SPCR_b.TXMD = 0x0;
	}
	if (!spi_context_rx_on(&data->ctx)) {
		p_spi_reg->SPCR_b.TXMD = 0x1; /* tx only */
	}

	uint32_t spdcr = p_spi_reg->SPDCR;

	if (SPI_BIT_WIDTH_16_BITS < spi_width) {
		/* Configure Word access to data register. */
		spdcr &= ~R_SPI0_SPDCR_SPBYT_Msk;
		spdcr |= R_SPI0_SPDCR_SPLW_Msk;
	} else if (SPI_BIT_WIDTH_8_BITS >= spi_width) {
		/* Set SPBYT so 8bit transfer works with the DTC/DMAC. */
		spdcr |= R_SPI0_SPDCR_SPBYT_Msk;
	} else {
		/* Configure Half-Word access to data register. */
		spdcr &= ~(R_SPI0_SPDCR_SPBYT_Msk | R_SPI0_SPDCR_SPLW_Msk);
	}

	/* Configure data length based on the selected bit width . */
	uint32_t bit_width = spi_width;

	if (bit_width > SPI_BIT_WIDTH_16_BITS) {
		bit_width = ((bit_width + 1) >> 2) - 5;
	}
	p_spi_reg->SPDCR = (uint8_t)spdcr;
	p_spi_reg->SPCMD[0] |= bit_width << 8;

	/* Enable the SPI Transfer. */
	p_spi_reg->SPCR |= R_SPI0_SPCR_SPE_Msk;

	do {
		ra_spi_transceive_data(data);
	} while (ra_spi_transfer_ongoing(data));

	/* Wait for transmision complete */
	while (p_spi_reg->SPSR_b.IDLNF) {
	}
	/* Disable the SPI Transfer. */
	p_spi_reg->SPCR_b.SPE = 0;

	spi_context_cs_control(&data->ctx, false);
	spi_context_complete(&data->ctx, dev, 0);
#endif
#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

end:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int ra_spi_transceive(const struct device *dev, const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int ra_spi_transceive_async(const struct device *dev, const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				   void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int ra_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct ra_spi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, ra_spi_driver_api) = {.transceive = ra_spi_transceive,
#ifdef CONFIG_SPI_ASYNC
					     .transceive_async = ra_spi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
					     .release = ra_spi_release};

static int spi_ra_init(const struct device *dev)
{
	const struct ra_spi_config *config = dev->config;
	struct ra_spi_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#if defined(CONFIG_SPI_INTERRUPT)

static void ra_spi_retransmit(struct ra_spi_data *data)
{
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->ctx.config->operation) - 1);

	if (data->ctx.rx_len == 0) {
		data->data_len = data->ctx.tx_len;
		data->spi.p_tx_data = data->ctx.tx_buf;
		data->spi.p_rx_data = NULL;
	} else if (data->ctx.tx_len == 0) {
		data->data_len = data->ctx.rx_len;
		data->spi.p_tx_data = NULL;
		data->spi.p_rx_data = data->ctx.rx_buf;
	} else {
		data->data_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
		data->spi.p_tx_data = data->ctx.tx_buf;
		data->spi.p_rx_data = data->ctx.rx_buf;
	}

	data->spi.bit_width = spi_width;
	data->spi.rx_count = 0;
	data->spi.tx_count = 0;
	data->spi.count = data->data_len;

#ifdef CONFIG_SPI_RA_DTC
	/* Determine DTC transfer size */
	transfer_size_t size;

	if (SPI_BIT_WIDTH_16_BITS < spi_width) {
		size = TRANSFER_SIZE_4_BYTE;
	} else if (SPI_BIT_WIDTH_8_BITS >= spi_width) {
		size = TRANSFER_SIZE_1_BYTE;
	} else {
		size = TRANSFER_SIZE_2_BYTE;
	}

	if (data->spi.p_cfg->p_transfer_rx) {
		/* When the rxi interrupt is called, all transfers will be finished. */
		data->spi.rx_count = data->data_len;

		transfer_instance_t *p_transfer_rx =
			(transfer_instance_t *)data->spi.p_cfg->p_transfer_rx;
		transfer_info_t *p_info = p_transfer_rx->p_cfg->p_info;

		/* Configure the receive DMA instance. */
		p_info->transfer_settings_word_b.size = size;
		p_info->length = (uint16_t)data->data_len;
		p_info->transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		p_info->p_dest = data->ctx.rx_buf;

		if (NULL == data->ctx.rx_buf) {
			static uint32_t dummy_rx;

			p_info->transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
			p_info->p_dest = &dummy_rx;
		}

		p_transfer_rx->p_api->reconfigure(p_transfer_rx->p_ctrl, p_info);
	}

	if (data->spi.p_cfg->p_transfer_tx) {
		/* When the txi interrupt is called, all transfers will be finished. */
		data->spi.tx_count = data->data_len;

		transfer_instance_t *p_transfer_tx =
			(transfer_instance_t *)data->spi.p_cfg->p_transfer_tx;
		transfer_info_t *p_info = p_transfer_tx->p_cfg->p_info;

		/* Configure the transmit DMA instance. */
		p_info->transfer_settings_word_b.size = size;
		p_info->length = (uint16_t)data->data_len;
		p_info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		p_info->p_src = data->ctx.tx_buf;

		if (NULL == data->ctx.tx_buf) {
			static uint32_t dummy_tx;

			p_info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED;
			p_info->p_src = &dummy_tx;
		}
		R_SPI0_Type *p_spi_reg = data->spi.p_regs;

		p_transfer_tx->p_api->reconfigure(p_transfer_tx->p_ctrl, p_info);

		/* Enable the SPI Transfer */
		p_spi_reg->SPCR |= R_SPI0_SPCR_SPE_Msk;
	}

#endif
}

static void ra_spi_rxi_isr(const struct device *dev)
{
#ifndef CONFIG_SPI_SLAVE
	ARG_UNUSED(dev);
	spi_rxi_isr();
#else
	struct ra_spi_data *data = dev->data;

	spi_rxi_isr();

	if (spi_context_is_slave(&data->ctx) && data->spi.rx_count == data->spi.count) {
		if (data->ctx.rx_buf != NULL && data->ctx.tx_buf != NULL) {
			data->ctx.recv_frames = MIN(spi_context_total_tx_len(&data->ctx),
						    spi_context_total_rx_len(&data->ctx));
		} else if (data->ctx.tx_buf == NULL) {
			data->ctx.recv_frames = data->data_len;
		}
		R_BSP_IrqDisable(data->fsp_config.tei_irq);

		/* Writing 0 to SPE generatates a TXI IRQ. Disable the TXI IRQ.
		 * (See Section 38.2.1 SPI Control Register in the RA6T2 manual R01UH0886EJ0100).
		 */
		R_BSP_IrqDisable(data->fsp_config.txi_irq);

		/* Disable the SPI Transfer. */
		data->spi.p_regs->SPCR_b.SPE = 0;

		/* Re-enable the TXI IRQ and clear the pending IRQ. */
		R_BSP_IrqEnable(data->fsp_config.txi_irq);

		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
	}

#endif
}

static void ra_spi_txi_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	spi_txi_isr();
}

static void ra_spi_tei_isr(const struct device *dev)
{
	struct ra_spi_data *data = dev->data;
	R_SPI0_Type *p_spi_reg = data->spi.p_regs;

	if (data->spi.rx_count == data->spi.count) {
		spi_context_update_rx(&data->ctx, 1, data->data_len);
	}
	if (data->spi.tx_count == data->spi.count) {
		spi_context_update_tx(&data->ctx, 1, data->data_len);
	}

	if (ra_spi_transfer_ongoing(data)) {
		R_BSP_IrqDisable(data->spi.p_cfg->txi_irq);
		/* Disable the SPI Transfer. */
		p_spi_reg->SPCR_b.SPE = 0U;
		/* Clear the status register. */
		p_spi_reg->SPSR;
		p_spi_reg->SPSR = 0;
		R_BSP_IrqEnable(data->spi.p_cfg->txi_irq);

#ifndef CONFIG_SPI_RA_DTC
		/* Enable the SPI Transfer */
		p_spi_reg->SPCR |= R_SPI0_SPCR_SPE_Msk;
#endif

		R_ICU->IELSR_b[data->fsp_config.tei_irq].IR = 0U;
		ra_spi_retransmit(data);
	} else {
		spi_tei_isr();
	}
}

static void ra_spi_eri_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	spi_eri_isr();
}
#endif

#define EVENT_SPI_RXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _RXI))
#define EVENT_SPI_TXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _TXI))
#define EVENT_SPI_TEI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _TEI))
#define EVENT_SPI_ERI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _ERI))

#if defined(CONFIG_SPI_INTERRUPT)

#define RA_SPI_IRQ_CONFIG_INIT(index)                                                              \
	do {                                                                                       \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, rxi, irq)] =                               \
			EVENT_SPI_RXI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, txi, irq)] =                               \
			EVENT_SPI_TXI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, tei, irq)] =                               \
			EVENT_SPI_TEI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, eri, irq)] =                               \
			EVENT_SPI_ERI(DT_INST_PROP(index, channel));                               \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rxi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, rxi, priority), ra_spi_rxi_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, txi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, txi, priority), ra_spi_txi_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, tei, priority), ra_spi_tei_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, eri, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, eri, priority), ra_spi_eri_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rxi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, txi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, eri, irq));                                  \
	} while (0)

#else

#define RA_SPI_IRQ_CONFIG_INIT(index)

#endif

#ifndef CONFIG_SPI_RA_DTC
#define RA_SPI_DTC_STRUCT_INIT(index)
#define RA_SPI_DTC_INIT(index)
#else
#define RA_SPI_DTC_INIT(index)                                                                     \
	do {                                                                                       \
		if (DT_INST_PROP_OR(index, rx_dtc, false)) {                                       \
			ra_spi_data_##index.fsp_config.p_transfer_rx =                             \
				&ra_spi_data_##index.rx_transfer;                                  \
		}                                                                                  \
		if (DT_INST_PROP_OR(index, tx_dtc, false)) {                                       \
			ra_spi_data_##index.fsp_config.p_transfer_tx =                             \
				&ra_spi_data_##index.tx_transfer;                                  \
		}                                                                                  \
	} while (0)
#define RA_SPI_DTC_STRUCT_INIT(index)                                                              \
	.rx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED, \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,  \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,        \
			.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.rx_transfer_cfg_extend = {.activation_source = DT_INST_IRQ_BY_NAME(index, rxi, irq)},     \
	.rx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &ra_spi_data_##index.rx_transfer_info,                           \
			.p_extend = &ra_spi_data_##index.rx_transfer_cfg_extend,                   \
	},                                                                                         \
	.rx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = &ra_spi_data_##index.rx_transfer_ctrl,                           \
			.p_cfg = &ra_spi_data_##index.rx_transfer_cfg,                             \
			.p_api = &g_transfer_on_dtc,                                               \
	},                                                                                         \
	.tx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,       \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,       \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,  \
			.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.tx_transfer_cfg_extend = {.activation_source = DT_INST_IRQ_BY_NAME(index, txi, irq)},     \
	.tx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &ra_spi_data_##index.tx_transfer_info,                           \
			.p_extend = &ra_spi_data_##index.tx_transfer_cfg_extend,                   \
	},                                                                                         \
	.tx_transfer = {                                                                           \
		.p_ctrl = &ra_spi_data_##index.tx_transfer_ctrl,                                   \
		.p_cfg = &ra_spi_data_##index.tx_transfer_cfg,                                     \
		.p_api = &g_transfer_on_dtc,                                                       \
	},
#endif

#define RA_SPI_INIT(index)                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct ra_spi_config ra_spi_config_##index = {                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
	};                                                                                         \
                                                                                                   \
	static struct ra_spi_data ra_spi_data_##index = {                                          \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)                           \
			SPI_CONTEXT_INIT_LOCK(ra_spi_data_##index, ctx),                           \
		SPI_CONTEXT_INIT_SYNC(ra_spi_data_##index, ctx),                                   \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.rxi_ipl = DT_INST_IRQ_BY_NAME(index, rxi, priority),              \
				.rxi_irq = DT_INST_IRQ_BY_NAME(index, rxi, irq),                   \
				.txi_ipl = DT_INST_IRQ_BY_NAME(index, txi, priority),              \
				.txi_irq = DT_INST_IRQ_BY_NAME(index, txi, irq),                   \
				.tei_ipl = DT_INST_IRQ_BY_NAME(index, tei, priority),              \
				.tei_irq = DT_INST_IRQ_BY_NAME(index, tei, irq),                   \
				.eri_ipl = DT_INST_IRQ_BY_NAME(index, eri, priority),              \
				.eri_irq = DT_INST_IRQ_BY_NAME(index, eri, irq),                   \
			},                                                                         \
		RA_SPI_DTC_STRUCT_INIT(index)};                                                    \
                                                                                                   \
	static int spi_ra_init##index(const struct device *dev)                                    \
	{                                                                                          \
		RA_SPI_DTC_INIT(index);                                                            \
		int err = spi_ra_init(dev);                                                        \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		RA_SPI_IRQ_CONFIG_INIT(index);                                                     \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, spi_ra_init##index, NULL,                                     \
			      &ra_spi_data_##index, &ra_spi_config_##index, PRE_KERNEL_1,          \
			      CONFIG_SPI_INIT_PRIORITY, &ra_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RA_SPI_INIT)
