/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_spi_b

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>
#include <zephyr/irq.h>

#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
#include <zephyr/drivers/interrupt_controller/intc_renesas_rx_icum_grp_irq.h>
#endif

#include "r_spi_b.h"
#include "bsp_icu.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rx_spi_b);

#include "spi_context.h"

#define ICU_EVENT_RSPI_SPRI(channel) CONCAT(ICU_EVENT_RSPI, channel, _SPRI)
#define ICU_EVENT_RSPI_SPTI(channel) CONCAT(ICU_EVENT_RSPI, channel, _SPTI)
#define ICU_EVENT_RSPI_SPCI(channel) CONCAT(ICU_EVENT_RSPI, channel, _SPCI)

#define SPI_CMD_FULL_DUPLEX (0x0)
#define SPI_CMD_TX_ONLY     (0x1)
#define SPI_CMD_RX_ONLY     (0x2)

struct rx_spi_b_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_rx_subsys_cfg clock_subsys;
	const struct device *spei_ctrl;
	uint8_t spei_num;
#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
	void (*irq_config_func)(const struct device *dev);
	void (*spti_isr)(const struct device *dev);
	void (*spri_isr)(const struct device *dev);
	void (*spci_isr)(const struct device *dev);
	void (*spei_isr)(const struct device *dev);
#endif
};

struct rx_spi_b_data {
	struct spi_context context;
	struct st_spi_b_instance_ctrl spi;
	uint8_t dfs;
	struct st_spi_cfg fsp_config;
	struct st_spi_b_extended_cfg fsp_config_extend;
#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
	uint32_t data_len;
#endif
};

#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
void spi_b_rxi_isr(void);
void spi_b_txi_isr(void);
void spi_b_tei_isr(void);
void spi_b_eri_isr(void);
#endif

#define SPI_B_RENESAS_RX_IRQ_GET(id, name, cell)                                                   \
	COND_CODE_1(DT_IRQ_HAS_NAME(id, name), (DT_IRQ_BY_NAME(id, name, cell)),                   \
		    ((IRQn_Type) FSP_INVALID_VECTOR))

#define SPI_B_GROUP_IRQ(id, name) DT_IRQ(DT_IRQ_INTC_BY_NAME(id, name), irq)

static bool rx_spi_b_transfer_ongoing(struct rx_spi_b_data *data)
{
#if defined(CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT)
	return (spi_context_tx_on(&data->context) || spi_context_rx_on(&data->context));
#else
	if (spi_context_total_tx_len(&data->context) < spi_context_total_rx_len(&data->context)) {
		return (spi_context_tx_on(&data->context) || spi_context_rx_on(&data->context));
	} else {
		return (spi_context_tx_on(&data->context) && spi_context_rx_on(&data->context));
	}
#endif
}

#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
static void rx_spi_b_retransmit(struct rx_spi_b_data *data)
{
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->context.config->operation) - 1);

	if (data->context.rx_len == 0) {
		data->data_len = data->context.tx_len;
		data->spi.p_tx_data = data->context.tx_buf;
		data->spi.p_rx_data = NULL;
	} else if (data->context.tx_len == 0) {
		data->data_len = data->context.rx_len;
		data->spi.p_tx_data = NULL;
		data->spi.p_rx_data = data->context.rx_buf;
	} else {
		data->data_len = MIN(data->context.tx_len, data->context.rx_len);
		data->spi.p_tx_data = data->context.tx_buf;
		data->spi.p_rx_data = data->context.rx_buf;
	}

	data->spi.bit_width = spi_width;
	data->spi.rx_count = 0;
	data->spi.tx_count = 0;
	data->spi.count = data->data_len;

	/*
	 * RSPIA only leaves the idle state (SPSR.IDLNF) once new data is written into
	 * SPDR while SPE is still set; clearing SPTEFC alone does not resume clocking.
	 * SPE is intentionally left enabled across segments here, so prime the shift
	 * register directly, mirroring what R_SPI_B_Write/Read/WriteRead does on the
	 * initial kick-off, otherwise the transfer silently stalls at the segment
	 * boundary and the caller eventually times out.
	 */
	if (data->spi.p_tx_data == NULL) {
		data->spi.p_regs->SPDR = 0;
	} else if (data->spi.bit_width > SPI_BIT_WIDTH_16_BITS) {
		data->spi.p_regs->SPDR = ((uint32_t *)data->spi.p_tx_data)[0];
	} else if (data->spi.bit_width > SPI_BIT_WIDTH_8_BITS) {
		data->spi.p_regs->SPDR = ((uint16_t *)data->spi.p_tx_data)[0];
	} else {
		data->spi.p_regs->SPDR = ((uint8_t *)data->spi.p_tx_data)[0];
	}
	data->spi.tx_count = 1;

	data->spi.p_regs->SPSCLR = R_SPI_B0_SPSCLR_SPTEFC_Msk;
}

static void rx_spi_b_spri_isr(const struct device *dev)
{

	const struct rx_spi_b_config *config = dev->config;
#ifndef CONFIG_SPI_PERIPHERAL
	config->spri_isr(dev);
#else
	struct rx_spi_b_data *data = dev->data;

	config->spri_isr(dev);
	if (spi_context_is_peripheral(&data->context) && data->spi.rx_count == data->spi.count) {

		if (data->context.rx_buf != NULL && data->context.tx_buf != NULL) {
			data->context.recv_frames = MIN(spi_context_total_tx_len(&data->context),
							spi_context_total_rx_len(&data->context));
		} else if (data->context.tx_buf == NULL) {
			data->context.recv_frames = data->data_len;
		} else {
			/* Do nothing */
		}

		R_BSP_IrqDisable(data->fsp_config.tei_irq);

		R_BSP_IrqDisable(data->fsp_config.txi_irq);

		/* Disable the SPI Transfer. */
		data->spi.p_regs->SPCR_b.SPE = 0;

		/* Re-enable the TXI IRQ and clear the pending IRQ. */
		R_BSP_IrqEnable(data->fsp_config.txi_irq);

		spi_context_cs_control(&data->context, false);
		spi_context_complete(&data->context, dev, 0);
	}
#endif
}

static void rx_spi_b_spci_isr(const struct device *dev)
{
	struct rx_spi_b_data *data = dev->data;
	const struct rx_spi_b_config *config = dev->config;

	if (data->spi.rx_count == data->spi.count) {
		spi_context_update_rx(&data->context, 1, data->data_len);
	}
	if (data->spi.tx_count == data->spi.count) {
		spi_context_update_tx(&data->context, 1, data->data_len);
	}
	if (rx_spi_b_transfer_ongoing(data)) {
		rx_spi_b_retransmit(data);
	} else {
		config->spci_isr(dev);
	}
}

static void rx_spi_b_spti_isr(const struct device *dev)
{
	const struct rx_spi_b_config *config = dev->config;

	config->spti_isr(dev);
}

static void rx_spi_b_spei_isr(const struct device *dev)
{
	const struct rx_spi_b_config *config = dev->config;

	config->spei_isr(dev);
}

static void rx_spi_b_callback(spi_callback_args_t *p_args)
{
	struct device *dev = (struct device *)p_args->p_context;
	struct rx_spi_b_data *data = dev->data;

	switch (p_args->event) {
	case SPI_EVENT_TRANSFER_COMPLETE:
		spi_context_cs_control(&data->context, false);
		spi_context_complete(&data->context, dev, 0);
		break;
	case SPI_EVENT_ERR_MODE_FAULT:    /* Mode fault error */
	case SPI_EVENT_ERR_READ_OVERFLOW: /* Read overflow error */
	case SPI_EVENT_ERR_PARITY:        /* Parity error */
	case SPI_EVENT_ERR_OVERRUN:       /* Overrun error */
	case SPI_EVENT_ERR_FRAMING:       /* Framing error */
	case SPI_EVENT_ERR_MODE_UNDERRUN: /* Underrun error */
		spi_context_cs_control(&data->context, false);
		spi_context_complete(&data->context, dev, -EIO);
		break;
	default:
		break;
	}
}
#endif

#ifndef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
static int rx_spi_b_transceive_slave(struct rx_spi_b_data *data)
{
	R_SPI_B0_Type *p_spi_reg = data->spi.p_regs;

	if (p_spi_reg->SPSR_b.SPTEF && spi_context_tx_on(&data->context)) {
		uint32_t tx;

		if (data->context.tx_buf != NULL) {
			if (data->dfs > 2) {
				tx = *(uint32_t *)(data->context.tx_buf);
			} else if (data->dfs > 1) {
				tx = *(uint16_t *)(data->context.tx_buf);
			} else {
				tx = *(uint8_t *)(data->context.tx_buf);
			}
		} else {
			tx = 0;
		}
		/* Clear Transmit Empty flag */
		p_spi_reg->SPSCLR = R_SPI_B0_SPSCLR_SPTEFC_Msk;

		p_spi_reg->SPDR = tx;

		spi_context_update_tx(&data->context, data->dfs, 1);
	} else {
		p_spi_reg->SPCR_b.SPTIE = 0;
	}

	if (p_spi_reg->SPSR_b.SPRF && spi_context_rx_buf_on(&data->context)) {
		uint32_t rx;

		rx = p_spi_reg->SPDR;
		/* Clear Receive Full flag */
		p_spi_reg->SPSCLR = R_SPI_B0_SPSCLR_SPRFC_Msk;
		if (data->dfs > 2) {
			UNALIGNED_PUT(rx, (uint32_t *)data->context.rx_buf);
		} else if (data->dfs > 1) {
			UNALIGNED_PUT(rx, (uint16_t *)data->context.rx_buf);
		} else {
			UNALIGNED_PUT(rx, (uint8_t *)data->context.rx_buf);
		}
		spi_context_update_rx(&data->context, data->dfs, 1);
	}

	return 0;
}

static int rx_spi_b_transceive_master(struct rx_spi_b_data *data)
{
	R_SPI_B0_Type *p_spi_reg = data->spi.p_regs;
	uint32_t tx;
	uint32_t rx;

	/* Tx transfer*/
	if (spi_context_tx_buf_on(&data->context)) {
		if (data->dfs > 2) {
			tx = *(uint32_t *)(data->context.tx_buf);
		} else if (data->dfs > 1) {
			tx = *(uint16_t *)(data->context.tx_buf);
		} else {
			tx = *(uint8_t *)(data->context.tx_buf);
		}
	} else {
		tx = 0U;
	}

	while (!p_spi_reg->SPSR_b.SPTEF) {
	}
	p_spi_reg->SPDR = tx;

	/* Clear Transmit Empty flag */
	p_spi_reg->SPSCLR = R_SPI_B0_SPSCLR_SPTEFC_Msk;
	spi_context_update_tx(&data->context, data->dfs, 1);
	/* Rx receive */

	if (spi_context_rx_on(&data->context)) {
		while (!p_spi_reg->SPSR_b.SPRF) {
		}
		rx = p_spi_reg->SPDR;
		/* Clear Receive Full flag */
		p_spi_reg->SPSCLR = R_SPI_B0_SPSCLR_SPRFC_Msk;
		if (data->dfs > 2) {
			UNALIGNED_PUT(rx, (uint32_t *)data->context.rx_buf);
		} else if (data->dfs > 1) {
			UNALIGNED_PUT(rx, (uint16_t *)data->context.rx_buf);
		} else {
			UNALIGNED_PUT(rx, (uint8_t *)data->context.rx_buf);
		}
		spi_context_update_rx(&data->context, data->dfs, 1);
	}

	return 0;
}

static int rx_spi_b_transceive_data(struct rx_spi_b_data *data)
{
	uint16_t operation = data->context.config->operation;

	if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_CONTROLLER) {
		rx_spi_b_transceive_master(data);
	} else {
		rx_spi_b_transceive_slave(data);
	}

	return 0;
}
#endif

static spi_b_clock_source_t rx_spi_b_clock_name(const struct device *clock_dev)
{
	const char *clock_dev_name = clock_dev->name;

	if (strcmp(clock_dev_name, "spiclk") == 0 || strcmp(clock_dev_name, "scispiclk") == 0) {
		return SPI_B_CLOCK_SOURCE_SCISPICLK;
	}

	return SPI_B_CLOCK_SOURCE_PCLK;
}

static int rx_spi_b_configure(const struct device *dev, const struct spi_config *config)
{
	struct rx_spi_b_data *data = dev->data;
	fsp_err_t err;
	uint8_t word_size = SPI_WORD_SIZE_GET(config->operation);

	if (spi_context_configured(&data->context, config)) {
		/* Nothing to do */
		return 0;
	}

	err = R_SPI_B_Close(&data->spi);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to close SPI channel: %d", err);
		return -EIO;
	}

	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
		return -ENOTSUP;
	}

	if (word_size < 4 || word_size > 32) {
		LOG_ERR("Unsupported SPI word size: %u", word_size);
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_PERIPHERAL) {
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
		err = R_SPI_B_CalculateBitrate(config->frequency,
					       data->fsp_config_extend.clock_source,
					       &data->fsp_config_extend.spck_div);
		if (err != FSP_SUCCESS) {
			LOG_ERR("Failed to calculate bitrate: %d", err);
			return -EINVAL;
		}
	}

	data->fsp_config_extend.spi_comm = SPI_B_COMMUNICATION_FULL_DUPLEX;

	if (spi_cs_is_gpio(config) || !IS_ENABLED(CONFIG_SPI_RENESAS_RX_SPI_B_USE_HW_SS)) {
		data->fsp_config_extend.spi_clksyn = SPI_B_SSL_MODE_CLK_SYN;
	} else {
		data->fsp_config_extend.spi_clksyn = SPI_B_SSL_MODE_SPI;
		data->fsp_config_extend.ssl_select = SPI_B_SSL_SELECT_SSL0;
	}

	data->fsp_config.p_extend = &data->fsp_config_extend;

	err = R_SPI_B_Open(&data->spi, &data->fsp_config);
	if (err != FSP_SUCCESS) {
		LOG_ERR("R_SPI_B_Open error: %d", err);
		return -EINVAL;
	}

	data->context.config = config;

	return 0;
}

static int rx_spi_b_transceive_common(const struct device *dev, const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs, bool asynchronous,
				      spi_callback_t cb, void *userdata)
{
	struct rx_spi_b_data *data = dev->data;
	R_SPI_B0_Type *p_spi_reg;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->context, asynchronous, cb, userdata, config);
	ret = rx_spi_b_configure(dev, config);
	if (ret) {
		goto end_transceive;
	}

	data->dfs = ((SPI_WORD_SIZE_GET(config->operation) - 1) / 8) + 1;
	p_spi_reg = data->spi.p_regs;
	/* Set buffers info */
	spi_context_buffers_setup(&data->context, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(&data->context, true);
	if ((!spi_context_tx_buf_on(&data->context)) && (!spi_context_rx_buf_on(&data->context))) {
		/* If current buffer has no data, do nothing */
		goto end_transceive;
	}
#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
	spi_bit_width_t spi_width =
		(spi_bit_width_t)(SPI_WORD_SIZE_GET(data->context.config->operation) - 1);

	if (data->context.rx_len == 0) {
		data->data_len = spi_context_is_peripheral(&data->context)
					 ? spi_context_total_tx_len(&data->context)
					 : data->context.tx_len;
	} else if (data->context.tx_len == 0) {
		data->data_len = spi_context_is_peripheral(&data->context)
					 ? spi_context_total_rx_len(&data->context)
					 : data->context.rx_len;
	} else {
		data->data_len = spi_context_is_peripheral(&data->context)
					 ? MAX(spi_context_total_tx_len(&data->context),
					       spi_context_total_rx_len(&data->context))
					 : MIN(data->context.tx_len, data->context.rx_len);
	}

	if (data->context.rx_buf == NULL) {
		R_SPI_B_Write(&data->spi, data->context.tx_buf, data->data_len, spi_width);
	} else if (data->context.tx_buf == NULL) {
		R_SPI_B_Read(&data->spi, data->context.rx_buf, data->data_len, spi_width);
	} else {
		R_SPI_B_WriteRead(&data->spi, data->context.tx_buf, data->context.rx_buf,
				  data->data_len, spi_width);
	}
	ret = spi_context_wait_for_completion(&data->context);

#else
	p_spi_reg->SPCR_b.CMMD = SPI_CMD_FULL_DUPLEX; /* tx - rx*/
	if (!spi_context_rx_on(&data->context)) {
		p_spi_reg->SPCR_b.CMMD = SPI_CMD_TX_ONLY; /* tx only */
	}

	/* Clear FIFOs */
	p_spi_reg->SPFCLR = 1;

	p_spi_reg->SPCR_b.SPE = 1;
	p_spi_reg->SPCMD0 |= (uint32_t)(SPI_WORD_SIZE_GET(data->context.config->operation) - 1)
			     << R_SPI_B0_SPCMD0_SPB_Pos;
	do {
		rx_spi_b_transceive_data(data);
	} while (rx_spi_b_transfer_ongoing(data));

	/* Wait for transmision complete */
	while (p_spi_reg->SPSR_b.IDLNF) {
	}

	/* Disable the SPI Transfer. */
	p_spi_reg->SPCR_b.SPE = 0;
#endif
#ifdef CONFIG_SPI_PERIPHERAL
	if (spi_context_is_peripheral(&data->context) && !ret) {
		ret = data->context.recv_frames;
	}
#endif /* CONFIG_SPI_PERIPHERAL */

end_transceive:
	spi_context_release(&data->context, ret);
	return ret;
}

static int rx_spi_b_transceive(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return rx_spi_b_transceive_common(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int rx_spi_b_transceive_async(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return rx_spi_b_transceive_common(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int rx_spi_b_release(const struct device *dev, const struct spi_config *config)
{
	struct rx_spi_b_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->context);

	return 0;
}

static int rx_spi_b_init(const struct device *dev)
{
	const struct rx_spi_b_config *config = dev->config;
	struct rx_spi_b_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	data->fsp_config_extend.clock_source = rx_spi_b_clock_name(config->clock_dev);

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->context);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->context);
#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
	config->irq_config_func(dev);
#endif

	return 0;
}

static DEVICE_API(spi, rx_spi_b_driver_api) = {
	.transceive = rx_spi_b_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = rx_spi_b_transceive_async,
#endif
	.release = rx_spi_b_release,
};

#ifdef CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT
#define RX_SPI_B_IRQ_CONFIG_INIT(index)                                                            \
                                                                                                   \
	static void rx_spi_b_spti_isr_##index(const struct device *dev)                            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		ISR_CALL(DT_INST_IRQ_BY_NAME(index, spti, irq), spi_b_txi_isr);                    \
	}                                                                                          \
                                                                                                   \
	static void rx_spi_b_spri_isr_##index(const struct device *dev)                            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		ISR_CALL(DT_INST_IRQ_BY_NAME(index, spri, irq), spi_b_rxi_isr);                    \
	}                                                                                          \
                                                                                                   \
	static void rx_spi_b_spci_isr_##index(const struct device *dev)                            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		ISR_CALL(DT_INST_IRQ_BY_NAME(index, spci, irq), spi_b_tei_isr);                    \
	}                                                                                          \
                                                                                                   \
	static void rx_spi_b_spei_isr_##index(const struct device *dev)                            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		spi_b_eri_isr();                                                                   \
	}                                                                                          \
                                                                                                   \
	static void rx_spi_b_irq_config_func##index(const struct device *dev)                      \
	{                                                                                          \
		const struct rx_spi_b_config *cfg = dev->config;                                   \
		struct rx_spi_b_data *data = dev->data;                                            \
                                                                                                   \
		int num_irq = DT_INST_IRQ_BY_NAME(index, spri, irq);                               \
		if (128 <= num_irq && num_irq < 144) {                                             \
			R_ICU->SLIXR[num_irq - 128].SLIXR =                                        \
				ICU_EVENT_RSPI_SPRI(DT_INST_PROP(index, channel));                 \
		} else if (144 <= num_irq && num_irq <= 255) {                                     \
			R_ICU->SLIR[num_irq - 144].SLIR =                                          \
				ICU_EVENT_RSPI_SPRI(DT_INST_PROP(index, channel));                 \
		} else {                                                                           \
			LOG_ERR("Invalid IRQ number for SPRI: %d", num_irq);                       \
		}                                                                                  \
                                                                                                   \
		num_irq = DT_INST_IRQ_BY_NAME(index, spti, irq);                                   \
		if (128 <= num_irq && num_irq < 144) {                                             \
			R_ICU->SLIXR[num_irq - 128].SLIXR =                                        \
				ICU_EVENT_RSPI_SPTI(DT_INST_PROP(index, channel));                 \
		} else if (144 <= num_irq && num_irq <= 255) {                                     \
			R_ICU->SLIR[num_irq - 144].SLIR =                                          \
				ICU_EVENT_RSPI_SPTI(DT_INST_PROP(index, channel));                 \
		} else {                                                                           \
			LOG_ERR("Invalid IRQ number for SPTI: %d", num_irq);                       \
		}                                                                                  \
                                                                                                   \
		num_irq = DT_INST_IRQ_BY_NAME(index, spci, irq);                                   \
		if (128 <= num_irq && num_irq < 144) {                                             \
			R_ICU->SLIXR[num_irq - 128].SLIXR =                                        \
				ICU_EVENT_RSPI_SPCI(DT_INST_PROP(index, channel));                 \
		} else if (144 <= num_irq) {                                                       \
			R_ICU->SLIR[num_irq - 144].SLIR =                                          \
				ICU_EVENT_RSPI_SPCI(DT_INST_PROP(index, channel));                 \
		} else {                                                                           \
			LOG_ERR("Invalid IRQ number for SPCI: %d", num_irq);                       \
		}                                                                                  \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, spri, irq),                                 \
			    DT_INST_IRQ_BY_NAME(index, spri, priority), rx_spi_b_spri_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, spti, irq),                                 \
			    DT_INST_IRQ_BY_NAME(index, spti, priority), rx_spi_b_spti_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, spci, irq),                                 \
			    DT_INST_IRQ_BY_NAME(index, spci, priority), rx_spi_b_spci_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, spri, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(index, spti, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(index, spci, irq));                                 \
                                                                                                   \
		int err = rx_icum_grp_irq_callback_set(cfg->spei_ctrl, cfg->spei_num,              \
						       (void (*)(void))rx_spi_b_spei_isr,          \
						       (void *)&data->spi);                        \
		if (err != 0) {                                                                    \
			LOG_ERR("Failed to set callback for group interrupt SPEI: %d", err);       \
		}                                                                                  \
		err = rx_icum_grp_irq_enable(cfg->spei_ctrl, cfg->spei_num);                       \
		if (err != 0) {                                                                    \
			LOG_ERR("Failed to allow interrupt request for SPEI: %d", err);            \
		}                                                                                  \
	}
#else
#define RX_SPI_B_IRQ_CONFIG_INIT(index)
#endif

#define ISR_CALL(num, isr) BSP_ICU_IRQ_HANDLER(num, isr)

#if defined(CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT)
#define RX_SPI_B_IRQ_CONFIG_FUNC(index)                                                            \
	.irq_config_func = rx_spi_b_irq_config_func##index, .spti_isr = rx_spi_b_spti_isr_##index, \
	.spri_isr = rx_spi_b_spri_isr_##index, .spci_isr = rx_spi_b_spci_isr_##index,              \
	.spei_isr = rx_spi_b_spei_isr_##index,                                                     \
	.spei_ctrl = DEVICE_DT_GET(DT_IRQ_INTC_BY_NAME(DT_DRV_INST(index), spei)),                 \
	.spei_num = DT_IRQ_BY_NAME(DT_DRV_INST(index), spei, irq),
#else
#define RX_SPI_B_IRQ_CONFIG_FUNC(index)
#endif

#if defined(CONFIG_SPI_RENESAS_RX_SPI_B_INTERRUPT)
#define RX_SPI_B_FSP_CONFIG_IRQ(index)                                                             \
	.rxi_irq = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spri, irq),                        \
	.txi_irq = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spti, irq),                        \
	.tei_irq = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spci, irq),                        \
	.eri_irq = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spei, irq) << 8 |                  \
		   SPI_B_GROUP_IRQ(DT_DRV_INST(index), spei),                                      \
	.rxi_ipl = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spri, priority),                   \
	.txi_ipl = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spti, priority),                   \
	.tei_ipl = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spci, priority),                   \
	.eri_ipl = SPI_B_RENESAS_RX_IRQ_GET(DT_DRV_INST(index), spei, priority),                   \
	.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(index)), .p_callback = rx_spi_b_callback,
#else
#define RX_SPI_B_FSP_CONFIG_IRQ(index)
#endif

#define RX_SPI_B_INIT(index)                                                                       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	RX_SPI_B_IRQ_CONFIG_INIT(index)                                                            \
                                                                                                   \
	static const struct rx_spi_b_config rx_spi_b_config_##index = {                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, spiclk,       \
									      mstp),               \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, spiclk, stop_bit),  \
			},                                                                         \
		RX_SPI_B_IRQ_CONFIG_FUNC(index)};                                                  \
                                                                                                   \
	static struct rx_spi_b_data rx_spi_b_data_##index = {                                      \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), context)                       \
			SPI_CONTEXT_INIT_LOCK(rx_spi_b_data_##index, context),                     \
		SPI_CONTEXT_INIT_SYNC(rx_spi_b_data_##index, context),                             \
		.fsp_config = {.channel = DT_INST_PROP(index, channel),                            \
			       RX_SPI_B_FSP_CONFIG_IRQ(index)},                                    \
	};                                                                                         \
                                                                                                   \
	static int rx_spi_b_init##index(const struct device *dev)                                  \
	{                                                                                          \
		int err = rx_spi_b_init(dev);                                                      \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(index, rx_spi_b_init##index, NULL, &rx_spi_b_data_##index,       \
				  &rx_spi_b_config_##index, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, \
				  &rx_spi_b_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RX_SPI_B_INIT)
