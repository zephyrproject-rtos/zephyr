/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra8_spi_b

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <instances/r_spi_b.h>

#ifdef CONFIG_SPI_B_RA_DTC
#include <instances/r_dtc.h>
#endif /* CONFIG_SPI_B_RA_DTC */

#ifdef CONFIG_SPI_B_DMAC
#include <instances/r_dmac.h>
#endif /* CONFIG_SPI_B_DMAC */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ra8_spi_b);

#include "spi_context.h"
#ifdef CONFIG_SPI_RTIO
#include "spi_rtio.h"
#endif

#ifdef CONFIG_SPI_B_RA_DTC
#define TRANSFER_INFO_ALIGNMENT DTC_TRANSFER_INFO_ALIGNMENT
#else
#define TRANSFER_INFO_ALIGNMENT
#endif

#if defined(CONFIG_SPI_B_INTERRUPT)
void spi_b_rxi_isr(void);
void spi_b_txi_isr(void);
void spi_b_tei_isr(void);
void spi_b_eri_isr(void);
#endif

#ifdef CONFIG_SPI_B_DMAC
extern void dmac_int_isr(void);
extern void spi_b_tx_dmac_callback(spi_b_instance_ctrl_t const *const p_ctrl);
extern void spi_b_rx_dmac_callback(spi_b_instance_ctrl_t const *const p_ctrl);
#endif

struct ra_spi_config {
	R_SPI_B0_Type *regs;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
};

struct ra_spi_data {
	struct spi_context ctx;
	uint8_t dfs;
	struct st_spi_b_instance_ctrl spi;
	struct st_spi_cfg fsp_config;
	struct st_spi_b_extended_cfg fsp_config_extend;
#if CONFIG_SPI_B_INTERRUPT
	uint32_t data_len;
#endif
#if defined(CONFIG_SPI_B_RA_DTC) || defined(CONFIG_SPI_B_DMAC)
	/* RX */
	struct st_transfer_instance rx_transfer;
	struct st_transfer_info rx_transfer_info TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg rx_transfer_cfg;

	/* TX */
	struct st_transfer_instance tx_transfer;
	struct st_transfer_info tx_transfer_info TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg tx_transfer_cfg;
#endif /* CONFIG_SPI_B_RA_DTC || CONFIG_SPI_B_DMAC */
#if defined(CONFIG_SPI_B_RA_DTC)
	/* DTC RX */
	struct st_dtc_instance_ctrl rx_dtc_ctrl;
	struct st_dtc_extended_cfg rx_dtc_cfg_extend;

	/* DTC TX */
	struct st_dtc_instance_ctrl tx_dtc_ctrl;
	struct st_dtc_extended_cfg tx_dtc_cfg_extend;
#endif
#if defined(CONFIG_SPI_B_DMAC)
	/* DMAC RX */
	struct st_dmac_instance_ctrl rx_dmac_ctrl;
	struct st_dmac_extended_cfg rx_dmac_cfg_extend;

	/* DMAC TX */
	struct st_dmac_instance_ctrl tx_dmac_ctrl;
	struct st_dmac_extended_cfg tx_dmac_cfg_extend;
#endif
};

#ifdef CONFIG_SPI_B_INTERRUPT
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
#endif /* CONFIG_SPI_B_INTERRUPT */

#if defined(CONFIG_SPI_B_INTERRUPT) && defined(CONFIG_SPI_SLAVE)
static void ra_spi_slave_transfer_done(const struct device *dev)
{
	struct ra_spi_data *data = dev->data;

	if (data->ctx.rx_buf != NULL && data->ctx.tx_buf != NULL) {
		data->ctx.recv_frames = MIN(spi_context_total_tx_len(&data->ctx),
					    spi_context_total_rx_len(&data->ctx));
	} else if (data->ctx.tx_buf == NULL) {
		/* RX-only */
		data->ctx.recv_frames = data->data_len;
	} else {
		/* TX-only */
		data->ctx.recv_frames = 0;
	}

	/*
	 * Writing 0 to SPE generates a TXI IRQ. Disable TXI first,
	 * clear SPE, then re-enable TXI and clear any pending IRQ.
	 * (See Section 38.2.1 in the RA6T2 manual R01UH0886EJ0100).
	 */
	R_BSP_IrqDisable(data->fsp_config.tei_irq);
	R_BSP_IrqDisable(data->fsp_config.txi_irq);

	data->spi.p_regs->SPCR_b.SPE = 0;

	R_BSP_IrqEnable(data->fsp_config.txi_irq);

	spi_context_cs_control(&data->ctx, false);
	spi_context_complete(&data->ctx, dev, data->ctx.recv_frames);
}
#endif /* CONFIG_SPI_B_INTERRUPT && CONFIG_SPI_SLAVE */

static int ra_spi_b_configure(const struct device *dev, const struct spi_config *config)
{
	struct ra_spi_data *data = dev->data;
	fsp_err_t fsp_err;
	uint8_t word_size = SPI_WORD_SIZE_GET(config->operation);

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (data->spi.open != 0) {
		R_SPI_B_Close(&data->spi);
	}

	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
		return -ENOTSUP;
	}

	if (word_size < 4 || word_size > 32) {
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
		data->fsp_config.clk_phase = SPI_CLK_PHASE_EDGE_ODD;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		data->fsp_config.bit_order = SPI_BIT_ORDER_LSB_FIRST;
	} else {
		data->fsp_config.bit_order = SPI_BIT_ORDER_MSB_FIRST;
	}

	if (config->frequency > 0) {
		fsp_err = R_SPI_B_CalculateBitrate(config->frequency,
						   data->fsp_config_extend.clock_source,
						   &data->fsp_config_extend.spck_div);
		__ASSERT(fsp_err == 0, "spi_b: spi frequency calculate error: %d", fsp_err);
	}

	data->fsp_config_extend.spi_comm = SPI_B_COMMUNICATION_FULL_DUPLEX;
	if (spi_cs_is_gpio(config) || !IS_ENABLED(CONFIG_SPI_B_USE_HW_SS)) {
		data->fsp_config_extend.spi_clksyn = SPI_B_SSL_MODE_CLK_SYN;
	} else {
		data->fsp_config_extend.spi_clksyn = SPI_B_SSL_MODE_SPI;
		data->fsp_config_extend.ssl_select = SPI_B_SSL_SELECT_SSL0;
	}

	data->fsp_config.p_extend = &data->fsp_config_extend;
#ifdef CONFIG_SPI_B_INTERRUPT
	data->fsp_config.p_callback = spi_cb;
#else
	data->fsp_config.p_callback = NULL;
#endif /* CONFIG_SPI_B_INTERRUPT */
	data->fsp_config.p_context = (void *)dev;
	fsp_err = R_SPI_B_Open(&data->spi, &data->fsp_config);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SPI_B_Open error: %d", fsp_err);
		return -EINVAL;
	}
	data->ctx.config = config;

	return 0;
}

static bool ra_spi_b_transfer_ongoing(struct ra_spi_data *data)
{
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
}

#ifndef CONFIG_SPI_B_INTERRUPT
static int ra_spi_b_transceive_slave(struct ra_spi_data *data)
{
	R_SPI_B0_Type *p_spi_reg = data->spi.p_regs;

	if (p_spi_reg->SPSR_b.SPTEF && spi_context_tx_on(&data->ctx)) {
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
		/* Clear Transmit Empty flag */
		p_spi_reg->SPSRC = R_SPI_B0_SPSRC_SPTEFC_Msk;

		p_spi_reg->SPDR = tx;

		spi_context_update_tx(&data->ctx, data->dfs, 1);
	} else {
		p_spi_reg->SPCR_b.SPTIE = 0;
	}

	if (p_spi_reg->SPSR_b.SPRF && spi_context_rx_buf_on(&data->ctx)) {
		uint32_t rx;

		rx = p_spi_reg->SPDR;
		/* Clear Receive Full flag */
		p_spi_reg->SPSRC = R_SPI_B0_SPSRC_SPRFC_Msk;
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

static int ra_spi_b_transceive_master(struct ra_spi_data *data)
{
	R_SPI_B0_Type *p_spi_reg = data->spi.p_regs;
	uint32_t tx;
	uint32_t rx;

	/* Tx transfer*/
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
	p_spi_reg->SPDR = tx;

	/* Clear Transmit Empty flag */
	p_spi_reg->SPSRC = R_SPI_B0_SPSRC_SPTEFC_Msk;
	spi_context_update_tx(&data->ctx, data->dfs, 1);

	if (p_spi_reg->SPCR_b.TXMD == 0x0) {
		while (!p_spi_reg->SPSR_b.SPRF) {
		}
		rx = p_spi_reg->SPDR;
		/* Clear Receive Full flag */
		p_spi_reg->SPSRC = R_SPI_B0_SPSRC_SPRFC_Msk;

		/* Rx receive */
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

static int ra_spi_b_transceive_data(struct ra_spi_data *data)
{
	uint16_t operation = data->ctx.config->operation;

	if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_MASTER) {
		ra_spi_b_transceive_master(data);
	} else {
		ra_spi_b_transceive_slave(data);
	}

	return 0;
}
#endif

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct ra_spi_data *data = dev->data;
	R_SPI_B0_Type *p_spi_reg;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_B_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	ret = ra_spi_b_configure(dev, config);
	if (ret) {
		goto end;
	}
	data->dfs = ((SPI_WORD_SIZE_GET(config->operation) - 1) / 8) + 1;
	p_spi_reg = data->spi.p_regs;
	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	spi_context_cs_control(&data->ctx, true);

	if ((!spi_context_tx_buf_on(&data->ctx)) && (!spi_context_rx_buf_on(&data->ctx))) {
		/* If current buffer has no data, do nothing */
		goto end;
	}

#ifdef CONFIG_SPI_B_INTERRUPT
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->ctx.config->operation) - 1);

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
		R_SPI_B_Write(&data->spi, data->ctx.tx_buf, data->data_len, spi_width);
	} else if (data->ctx.tx_buf == NULL) {
		R_SPI_B_Read(&data->spi, data->ctx.rx_buf, data->data_len, spi_width);
	} else {
		R_SPI_B_WriteRead(&data->spi, data->ctx.tx_buf, data->ctx.rx_buf,
			data->data_len, spi_width);
	}
	ret = spi_context_wait_for_completion(&data->ctx);

#else
	p_spi_reg->SPCR_b.TXMD = 0x0; /* tx - rx*/
	if (!spi_context_rx_on(&data->ctx)) {
		p_spi_reg->SPCR_b.TXMD = 0x1; /* tx only */
	}

	/* Clear FIFOs */
	p_spi_reg->SPFCR = 1;

	/* Enable the SPI Transfer. */
	p_spi_reg->SPCR_b.SPE = 1;
	p_spi_reg->SPCMD0 |= (uint32_t)(SPI_WORD_SIZE_GET(data->ctx.config->operation) - 1)
			     << R_SPI_B0_SPCMD0_SPB_Pos;
	do {
		ra_spi_b_transceive_data(data);
	} while (ra_spi_b_transfer_ongoing(data));

	/* Wait for transmission complete */
	while (p_spi_reg->SPSR_b.IDLNF) {
	}

	/* Disable the SPI Transfer. */
	p_spi_reg->SPCR_b.SPE = 0;

	spi_context_cs_control(&data->ctx, false);
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

static int ra_spi_b_transceive(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int ra_spi_b_transceive_async(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int ra_spi_b_release(const struct device *dev, const struct spi_config *config)
{
	struct ra_spi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, ra_spi_driver_api) = {.transceive = ra_spi_b_transceive,
#ifdef CONFIG_SPI_ASYNC
					     .transceive_async = ra_spi_b_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
					     .iodev_submit = spi_rtio_iodev_default_submit,
#endif /* CONFIG_SPI_RTIO */
					     .release = ra_spi_b_release};

static spi_b_clock_source_t ra_spi_b_clock_name(const struct device *clock_dev)
{
	const char *clock_dev_name = clock_dev->name;

	if (strcmp(clock_dev_name, "spiclk") == 0 || strcmp(clock_dev_name, "scispiclk") == 0) {
		return SPI_B_CLOCK_SOURCE_SCISPICLK;
	}

	return SPI_B_CLOCK_SOURCE_PCLK;
}

static int spi_b_ra_init(const struct device *dev)
{
	const struct ra_spi_config *config = dev->config;
	struct ra_spi_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	data->fsp_config_extend.clock_source = ra_spi_b_clock_name(config->clock_dev);

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

#if defined(CONFIG_SPI_B_INTERRUPT)

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
#ifdef CONFIG_SPI_B_RA_DTC
	/* Determine DTC transfer size */
	transfer_size_t size;

	if (spi_width > SPI_BIT_WIDTH_16_BITS) { /* Bit Widths of 17-32 bits */
		size = TRANSFER_SIZE_4_BYTE;
	} else if (spi_width > SPI_BIT_WIDTH_8_BITS) { /* Bit Widths of 9-16 bits*/
		size = TRANSFER_SIZE_2_BYTE;
	} else { /* Bit Widths of 4-8 bits */
		size = TRANSFER_SIZE_1_BYTE;
	}

	if (data->spi.p_cfg->p_transfer_rx) {
		/* When the rxi interrupt is called, all transfers will be finished. */
		data->spi.rx_count = data->data_len;

		transfer_instance_t *p_transfer_rx =
			(transfer_instance_t *)data->spi.p_cfg->p_transfer_rx;
		transfer_info_t *p_info = p_transfer_rx->p_cfg->p_info;

		/* Configure the receive transfer instance. */
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

		/* Configure the transmit transfer instance. */
		p_info->transfer_settings_word_b.size = size;
		p_info->length = (uint16_t)data->data_len;
		p_info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		p_info->p_src = data->ctx.tx_buf;

		if (NULL == data->ctx.tx_buf) {
			static uint32_t dummy_tx;

			p_info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED;
			p_info->p_src = &dummy_tx;
		}

		p_transfer_tx->p_api->reconfigure(p_transfer_tx->p_ctrl, p_info);
	}
#endif
	data->spi.p_regs->SPSRC = R_SPI_B0_SPSRC_SPTEFC_Msk;
}

static __maybe_unused void ra_spi_rxi_isr(const struct device *dev)
{
#ifndef CONFIG_SPI_SLAVE
	ARG_UNUSED(dev);
	spi_b_rxi_isr();
#else
	struct ra_spi_data *data = dev->data;

	spi_b_rxi_isr();
	if (spi_context_is_slave(&data->ctx) && data->spi.rx_count == data->spi.count) {
		ra_spi_slave_transfer_done(dev);
	}
#endif
}

static __maybe_unused void ra_spi_txi_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	spi_b_txi_isr();
}

static void ra_spi_tei_isr(const struct device *dev)
{
	struct ra_spi_data *data = dev->data;

	if (data->spi.rx_count == data->spi.count) {
		spi_context_update_rx(&data->ctx, 1, data->data_len);
	}
	if (data->spi.tx_count == data->spi.count) {
		spi_context_update_tx(&data->ctx, 1, data->data_len);
	}
	if (ra_spi_b_transfer_ongoing(data)) {
		R_ICU->IELSR_b[data->fsp_config.tei_irq].IR = 0U;
		ra_spi_retransmit(data);
	} else {
		spi_b_tei_isr();
	}
}

static void ra_spi_eri_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	spi_b_eri_isr();
}
#endif

#define EVENT_SPI_RXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _RXI))
#define EVENT_SPI_TXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _TXI))
#define EVENT_SPI_TEI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _TEI))
#define EVENT_SPI_ERI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SPI, channel, _ERI))
#define EVENT_DMAC_INT(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_DMAC, channel, _INT))

#if defined(CONFIG_SPI_B_INTERRUPT)

#define SPI_B_DMAC_IRQ(index, dir)                                                                 \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                                             \
		    (DT_IRQ_BY_IDX(DT_INST_DMAS_CTLR_BY_NAME(index, dir),                          \
				   DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                 \
				   irq)),                                                          \
		    (FSP_INVALID_VECTOR))

#define SPI_B_DMAC_IPL(index, dir)                                                                 \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                                             \
		    (DT_IRQ_BY_IDX(DT_INST_DMAS_CTLR_BY_NAME(index, dir),                          \
				   DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                 \
				   priority)),                                                     \
		    (BSP_IRQ_DISABLED))

#define RA_SPI_B_IRQ_CONFIG_INIT(index)                                                            \
	do {                                                                                       \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		/* Enable DMAC interrupt instead of TXI/RXI for DMA transfers */                   \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, rx),                                      \
		(                                                                                  \
			R_ICU->IELSR[SPI_B_DMAC_IRQ(index, rx)] = EVENT_DMAC_INT(                  \
					DT_INST_DMAS_CELL_BY_NAME(index, rx, channel));            \
			IRQ_CONNECT(SPI_B_DMAC_IRQ(index, rx),                                     \
				    SPI_B_DMAC_IPL(index, rx),                                     \
				    dmac_int_isr, NULL, 0);                                        \
			irq_enable(SPI_B_DMAC_IRQ(index, rx));                                     \
		),                                                                                 \
		(                                                                                  \
			R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, rxi, irq)] =                       \
				EVENT_SPI_RXI(DT_INST_PROP(index, channel));                       \
                                                                                                   \
			IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rxi, irq),                          \
				    DT_INST_IRQ_BY_NAME(index, rxi, priority),                     \
				    ra_spi_rxi_isr, DEVICE_DT_INST_GET(index), 0);                 \
			irq_enable(DT_INST_IRQ_BY_NAME(index, rxi, irq));                          \
		))                                                                                 \
                                                                                                   \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),                                      \
		(                                                                                  \
			R_ICU->IELSR[SPI_B_DMAC_IRQ(index, tx)] = EVENT_DMAC_INT(                  \
					DT_INST_DMAS_CELL_BY_NAME(index, tx, channel));            \
			IRQ_CONNECT(SPI_B_DMAC_IRQ(index, tx),                                     \
				    SPI_B_DMAC_IPL(index, tx),                                     \
				    dmac_int_isr, NULL, 0);                                        \
			irq_enable(SPI_B_DMAC_IRQ(index, tx));                                     \
		),                                                                                 \
		(                                                                                  \
			R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, txi, irq)] =                       \
				EVENT_SPI_TXI(DT_INST_PROP(index, channel));                       \
                                                                                                   \
			IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, txi, irq),                          \
				    DT_INST_IRQ_BY_NAME(index, txi, priority),                     \
				    ra_spi_txi_isr, DEVICE_DT_INST_GET(index), 0);                 \
			irq_enable(DT_INST_IRQ_BY_NAME(index, txi, irq));                          \
		))                                                                                 \
                                                                                                   \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, tei, irq)] =                               \
			EVENT_SPI_TEI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, eri, irq)] =                               \
			EVENT_SPI_ERI(DT_INST_PROP(index, channel));                               \
                                                                                                   \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_SPI_RXI(DT_INST_PROP(index, channel)));     \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_SPI_TXI(DT_INST_PROP(index, channel)));     \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_SPI_TEI(DT_INST_PROP(index, channel)));     \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_SPI_ERI(DT_INST_PROP(index, channel)));     \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, tei, priority), ra_spi_tei_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, eri, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, eri, priority), ra_spi_eri_isr,             \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, eri, irq));                                  \
	} while (0)

#else

#define RA_SPI_B_IRQ_CONFIG_INIT(index)

#endif

#ifndef CONFIG_SPI_B_DMAC
#define SPI_B_DMAC_CFG_EXTEND(index)
#define SPI_B_DMAC_CALLBACKS(index)
#else
#define SPI_B_DMAC_CFG_EXTEND(index)                                                               \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, tx), (                                             \
		.tx_dmac_cfg_extend = {                                                            \
			.activation_source = EVENT_SPI_TXI(DT_INST_PROP(index, channel)),          \
			.channel = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),                   \
					      (DT_INST_DMAS_CELL_BY_NAME(index, tx, channel)),     \
					      (0)),                                                \
			.irq = SPI_B_DMAC_IRQ(index, tx),                                          \
			.ipl = SPI_B_DMAC_IPL(index, tx),                                          \
			.offset = 1, .src_buffer_size = 1},                                        \
	))                                                                                         \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, rx), (                                             \
		.rx_dmac_cfg_extend = {                                                            \
			.activation_source = EVENT_SPI_RXI(DT_INST_PROP(index, channel)),          \
			.channel = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, rx),                   \
					      (DT_INST_DMAS_CELL_BY_NAME(index, rx, channel)),     \
					      (0)),                                                \
			.irq = SPI_B_DMAC_IRQ(index, rx),                                          \
			.ipl = SPI_B_DMAC_IPL(index, rx),                                          \
			.offset = 1, .src_buffer_size = 1},                                        \
	))

#define SPI_B_DMAC_CALLBACKS(index)                                                                \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, tx), (                                             \
		static void ra_spi_b_dmac_tx_cb_##index(dmac_callback_args_t *p_args)              \
		{                                                                                  \
			FSP_PARAMETER_NOT_USED(p_args);                                            \
			spi_b_tx_dmac_callback(&ra_spi_data_##index.spi);                          \
		}                                                                                  \
	))                                                                                         \
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, rx), (                                             \
		static void ra_spi_b_dmac_rx_cb_##index(dmac_callback_args_t *p_args)              \
		{                                                                                  \
			FSP_PARAMETER_NOT_USED(p_args);                                            \
			IF_ENABLED(CONFIG_SPI_SLAVE, (                                             \
			if (spi_context_is_slave(&ra_spi_data_##index.ctx)) {                      \
				ra_spi_slave_transfer_done(DEVICE_DT_INST_GET(index));             \
				return;                                                            \
			}))                                                                        \
			spi_b_rx_dmac_callback(&ra_spi_data_##index.spi);                          \
		}                                                                                  \
	))
#endif /* CONFIG_SPI_B_DMAC */

#ifndef CONFIG_SPI_B_RA_DTC
#define SPI_B_DTC_CFG_EXTEND(index)
#else
#define SPI_B_DTC_CFG_EXTEND(index)                                                                \
	IF_ENABLED(DT_INST_PROP_OR(index, rx_dtc, false),                                          \
		   (.rx_dtc_cfg_extend = {.activation_source =                                     \
				      DT_INST_IRQ_BY_NAME(index, rxi, irq)},))                     \
	IF_ENABLED(DT_INST_PROP_OR(index, tx_dtc, false),                                          \
		   (.tx_dtc_cfg_extend = {.activation_source =                                     \
					   DT_INST_IRQ_BY_NAME(index, txi, irq)},))
#endif

#define SPI_B_ASSERT_NO_CONFLICT(index, dir)                                                       \
	BUILD_ASSERT(!(DT_INST_DMAS_HAS_NAME(index, dir) &&                                        \
		       DT_INST_PROP_OR(index, dir##_dtc, false)),                                  \
		      "Cannot use both DMAS and " #dir "-dtc for the same direction")

#if defined(CONFIG_SPI_B_RA_DTC) || defined(CONFIG_SPI_B_DMAC)
/*
 * Select a transfer value for a given direction (@p dir: rx or tx).
 * - If instance has DMAS for @p dir: use @p dmac_val
 * - Else if instance has DTC property for @p dir: use @p dtc_val
 * - Else: NULL
 */
#define SPI_B_TRANSFER_DMAC_OR_DTC(index, dir, dmac_val, dtc_val)                                  \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                                             \
		    (dmac_val),                                                                    \
		    (COND_CODE_1(DT_INST_PROP_OR(index, dir##_dtc, false),                         \
				 (dtc_val),                                                        \
				 (NULL))))

#define SPI_B_TRANSFER_INIT(index)                                                                 \
	do {                                                                                       \
		if (DT_INST_PROP_OR(index, rx_dtc, false) || DT_INST_DMAS_HAS_NAME(index, rx)) {   \
			ra_spi_data_##index.fsp_config.p_transfer_rx =                             \
				&ra_spi_data_##index.rx_transfer;                                  \
		}                                                                                  \
		if (DT_INST_PROP_OR(index, tx_dtc, false) || DT_INST_DMAS_HAS_NAME(index, tx)) {   \
			ra_spi_data_##index.fsp_config.p_transfer_tx =                             \
				&ra_spi_data_##index.tx_transfer;                                  \
		}                                                                                  \
		IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, rx),                                       \
			(ra_spi_data_##index.rx_dmac_cfg_extend.p_callback =                       \
				ra_spi_b_dmac_rx_cb_##index;))                                     \
		IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, tx),                                       \
			(ra_spi_data_##index.tx_dmac_cfg_extend.p_callback =                       \
				ra_spi_b_dmac_tx_cb_##index;))                                     \
	} while (0)

#define SPI_B_TRANSFER_CONFIGURE(index)                                                            \
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
	.rx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &ra_spi_data_##index.rx_transfer_info,                           \
			.p_extend = SPI_B_TRANSFER_DMAC_OR_DTC(index, rx,                          \
					(&ra_spi_data_##index.rx_dmac_cfg_extend),                 \
					(&ra_spi_data_##index.rx_dtc_cfg_extend)),                 \
	},                                                                                         \
	.rx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = SPI_B_TRANSFER_DMAC_OR_DTC(index, rx,                            \
						(&ra_spi_data_##index.rx_dmac_ctrl),               \
						(&ra_spi_data_##index.rx_dtc_ctrl)),               \
			.p_cfg = &ra_spi_data_##index.rx_transfer_cfg,                             \
			.p_api = SPI_B_TRANSFER_DMAC_OR_DTC(index, rx,                             \
						(&g_transfer_on_dmac),                             \
						(&g_transfer_on_dtc)),                             \
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
	.tx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &ra_spi_data_##index.tx_transfer_info,                           \
			.p_extend = SPI_B_TRANSFER_DMAC_OR_DTC(index, tx,                          \
					(&ra_spi_data_##index.tx_dmac_cfg_extend),                 \
					(&ra_spi_data_##index.tx_dtc_cfg_extend)),                 \
	},                                                                                         \
	.tx_transfer = {                                                                           \
		.p_ctrl = SPI_B_TRANSFER_DMAC_OR_DTC(index, tx,                                    \
					(&ra_spi_data_##index.tx_dmac_ctrl),                       \
					(&ra_spi_data_##index.tx_dtc_ctrl)),                       \
		.p_cfg = &ra_spi_data_##index.tx_transfer_cfg,                                     \
		.p_api = SPI_B_TRANSFER_DMAC_OR_DTC(index, tx,                                     \
					(&g_transfer_on_dmac),                                     \
					(&g_transfer_on_dtc)),                                     \
	},                                                                                         \
	SPI_B_DTC_CFG_EXTEND(index)                                                                \
	SPI_B_DMAC_CFG_EXTEND(index)
#else
#define SPI_B_TRANSFER_INIT(index)
#define SPI_B_TRANSFER_CONFIGURE(index)
#endif /* CONFIG_SPI_B_RA_DTC || CONFIG_SPI_B_DMAC */

#define SPI_B_IRQ_GET(index, name, cell)                                                           \
	COND_CODE_1(DT_IRQ_HAS_NAME(index, name),                                                  \
		    (DT_IRQ_BY_NAME(index, name, cell)),                                           \
		    ((IRQn_Type)FSP_INVALID_VECTOR))

#define RA_SPI_INIT(index)                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct ra_spi_config ra_spi_config_##index = {                                \
		.regs = (R_SPI_B0_Type *)DT_INST_REG_ADDR(index),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, spiclk,       \
									      mstp),               \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, spiclk, stop_bit),  \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct ra_spi_data ra_spi_data_##index = {                                          \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)                           \
			SPI_CONTEXT_INIT_LOCK(ra_spi_data_##index, ctx),                           \
		SPI_CONTEXT_INIT_SYNC(ra_spi_data_##index, ctx),                                   \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.rxi_ipl = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, rx),           \
					   (BSP_IRQ_DISABLED),                                     \
					   (SPI_B_IRQ_GET(DT_DRV_INST(index), rxi, priority))),    \
				.rxi_irq = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, rx),           \
					   (FSP_INVALID_VECTOR),                                   \
					   (SPI_B_IRQ_GET(DT_DRV_INST(index), rxi, irq))),         \
				.txi_ipl = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),           \
					   (BSP_IRQ_DISABLED),                                     \
					   (SPI_B_IRQ_GET(DT_DRV_INST(index), txi, priority))),    \
				.txi_irq = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),           \
					   (FSP_INVALID_VECTOR),                                   \
					   (SPI_B_IRQ_GET(DT_DRV_INST(index), txi, irq))),         \
				.tei_ipl = SPI_B_IRQ_GET(DT_DRV_INST(index), tei, priority),       \
				.tei_irq = SPI_B_IRQ_GET(DT_DRV_INST(index), tei, irq),            \
				.eri_ipl = SPI_B_IRQ_GET(DT_DRV_INST(index), eri, priority),       \
				.eri_irq = SPI_B_IRQ_GET(DT_DRV_INST(index), eri, irq),            \
				.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(index)),            \
			},                                                                         \
		SPI_B_TRANSFER_CONFIGURE(index)};                                                  \
                                                                                                   \
	SPI_B_DMAC_CALLBACKS(index)                                                                \
                                                                                                   \
	static int spi_b_ra_init##index(const struct device *dev)                                  \
	{                                                                                          \
		SPI_B_ASSERT_NO_CONFLICT(index, tx);                                               \
		SPI_B_ASSERT_NO_CONFLICT(index, rx);                                               \
		SPI_B_TRANSFER_INIT(index);                                                        \
		int err = spi_b_ra_init(dev);                                                      \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		RA_SPI_B_IRQ_CONFIG_INIT(index);                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(index, spi_b_ra_init##index, PM_DEVICE_DT_INST_GET(index),       \
				  &ra_spi_data_##index, &ra_spi_config_##index, POST_KERNEL,       \
				  CONFIG_SPI_INIT_PRIORITY, &ra_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RA_SPI_INIT)
