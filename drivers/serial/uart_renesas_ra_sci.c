/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_sci_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "r_sci_uart.h"
#include "r_dtc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ra_sci_uart);

#define SCI_UART_SSR_FIFO_DR_RDF    (R_SCI0_SSR_FIFO_DR_Msk | R_SCI0_SSR_FIFO_RDF_Msk)
#define SCI_UART_SSR_FIFO_TDFE_TEND (R_SCI0_SSR_FIFO_TDFE_Msk | R_SCI0_SSR_FIFO_TEND_Msk)
#define SCI_UART_SSR_TDRE_TEND      (R_SCI0_SSR_TDRE_Msk | R_SCI0_SSR_TEND_Msk)
#define SCI_UART_SSR_ERR_MSK        (R_SCI0_SSR_ORER_Msk | R_SCI0_SSR_FER_Msk | R_SCI0_SSR_PER_Msk)
#define SCI_UART_SSR_FIFO_ERR_MSK                                                                  \
	(R_SCI0_SSR_FIFO_ORER_Msk | R_SCI0_SSR_FIFO_FER_Msk | R_SCI0_SSR_FIFO_PER_Msk)

#if defined(CONFIG_UART_ASYNC_API)
void sci_uart_rxi_isr(void);
void sci_uart_txi_isr(void);
void sci_uart_tei_isr(void);
void sci_uart_eri_isr(void);
#endif

struct uart_ra_sci_config {
	const struct pinctrl_dev_config *pcfg;

	R_SCI0_Type * const regs;
};

struct uart_ra_sci_data {
	const struct device *dev;
	struct st_sci_uart_instance_ctrl sci;
	struct uart_config uart_config;
	struct st_uart_cfg fsp_config;
	struct st_sci_uart_extended_cfg fsp_config_extend;
	struct st_baud_setting_t fsp_baud_setting;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_callback_user_data_t user_cb;
	void *user_cb_data;
	uint32_t ssr;
#endif
#if defined(CONFIG_UART_ASYNC_API)
	uart_callback_t async_user_cb;
	void *async_user_cb_data;

	struct k_work_delayable rx_timeout_work;
	size_t rx_timeout;
	size_t rx_buf_len;
	size_t rx_buf_offset;
	size_t rx_buf_cap;
	uint8_t *rx_buffer;
	size_t rx_next_buf_cap;
	uint8_t *rx_next_buf;

	struct st_transfer_instance rx_transfer;
	struct st_dtc_instance_ctrl rx_transfer_ctrl;
	struct st_transfer_info rx_transfer_info;
	struct st_transfer_cfg rx_transfer_cfg;
	struct st_dtc_extended_cfg rx_transfer_cfg_extend;

	struct k_work_delayable tx_timeout;
	size_t tx_buf_cap;

	struct st_transfer_instance tx_transfer;
	struct st_dtc_instance_ctrl tx_transfer_ctrl;
	struct st_transfer_info tx_transfer_info;
	struct st_transfer_cfg tx_transfer_cfg;
	struct st_dtc_extended_cfg tx_transfer_cfg_extend;
#endif
};

static int uart_ra_sci_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;

	if (IS_ENABLED(CONFIG_UART_ASYNC_API) && cfg->regs->SCR_b.RIE) {
		/* This function cannot be used if async reception was enabled */
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_UART_RA_SCI_UART_FIFO_ENABLE) && data->sci.fifo_depth > 0
		    ? cfg->regs->FDR_b.R == 0U
		    : cfg->regs->SSR_b.RDRF == 0U) {
		/* There are no characters available to read. */
		return -1;
	}

	/* got a character */
	*c = IS_ENABLED(CONFIG_UART_RA_SCI_UART_FIFO_ENABLE) && data->sci.fifo_depth > 0
		     ? cfg->regs->FRDRL
		     : cfg->regs->RDR;

	return 0;
}

static void uart_ra_sci_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth > 0) {
		while (cfg->regs->FDR_b.T > 0x8) {
		}
		cfg->regs->FTDRL = c;
	} else
#endif
	{
		while (cfg->regs->SSR_b.TDRE == 0U) {
		}
		cfg->regs->TDR = c;
	}
}

static int uart_ra_sci_err_check(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	int errors = 0;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth > 0) {
		const uint8_t status = cfg->regs->SSR_FIFO;
		uint8_t ssr_fifo = 0;

		if (status & R_SCI0_SSR_FIFO_ORER_Msk) {
			errors |= UART_ERROR_OVERRUN;
			ssr_fifo |= R_SCI0_SSR_FIFO_ORER_Msk;
		}
		if (status & R_SCI0_SSR_FIFO_PER_Msk) {
			errors |= UART_ERROR_PARITY;
			ssr_fifo |= R_SCI0_SSR_FIFO_PER_Msk;
		}
		if (status & R_SCI0_SSR_FIFO_FER_Msk) {
			errors |= UART_ERROR_FRAMING;
			ssr_fifo |= R_SCI0_SSR_FIFO_FER_Msk;
		}
		cfg->regs->SSR_FIFO &= ~ssr_fifo;
	} else
#endif
	{
		const uint8_t status = cfg->regs->SSR;
		uint8_t ssr = 0;

		if (status & R_SCI0_SSR_ORER_Msk) {
			errors |= UART_ERROR_OVERRUN;
			ssr |= R_SCI0_SSR_ORER_Msk;
		}
		if (status & R_SCI0_SSR_PER_Msk) {
			errors |= UART_ERROR_PARITY;
			ssr |= R_SCI0_SSR_PER_Msk;
		}
		if (status & R_SCI0_SSR_FER_Msk) {
			errors |= UART_ERROR_FRAMING;
			ssr |= R_SCI0_SSR_FER_Msk;
		}
		cfg->regs->SSR &= ~ssr;
	}

	return errors;
}

static int uart_ra_sci_apply_config(const struct uart_config *config,
				    struct st_uart_cfg *fsp_config,
				    struct st_sci_uart_extended_cfg *fsp_config_extend,
				    struct st_baud_setting_t *fsp_baud_setting)
{
	fsp_err_t fsp_err;

	fsp_err = R_SCI_UART_BaudCalculate(config->baudrate, true, 5000, fsp_baud_setting);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: uart: baud calculate error");
		return -EINVAL;
	}

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		fsp_config->parity = UART_PARITY_OFF;
		break;
	case UART_CFG_PARITY_ODD:
		fsp_config->parity = UART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		fsp_config->parity = UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		return -ENOTSUP;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_1:
		fsp_config->stop_bits = UART_STOP_BITS_1;
		break;
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_2:
		fsp_config->stop_bits = UART_STOP_BITS_2;
		break;
	default:
		return -EINVAL;
	}

	switch (config->data_bits) {
	case UART_CFG_DATA_BITS_5:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_6:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_7:
		fsp_config->data_bits = UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		fsp_config->data_bits = UART_DATA_BITS_8;
		break;
	case UART_CFG_DATA_BITS_9:
		fsp_config->data_bits = UART_DATA_BITS_9;
		break;
	default:
		return -EINVAL;
	}

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	fsp_config_extend->rx_fifo_trigger = 0x8;
#endif

	switch (config->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		fsp_config_extend->flow_control = 0;
		fsp_config_extend->rs485_setting.enable = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		fsp_config_extend->flow_control = SCI_UART_FLOW_CONTROL_HARDWARE_CTSRTS;
		fsp_config_extend->rs485_setting.enable = false;
		break;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
		return -ENOTSUP;
	case UART_CFG_FLOW_CTRL_RS485:
		/* TODO: implement this config */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_ra_sci_configure(const struct device *dev, const struct uart_config *config)
{
	int err;
	fsp_err_t fsp_err;
	struct uart_ra_sci_data *data = dev->data;

	err = uart_ra_sci_apply_config(config, &data->fsp_config, &data->fsp_config_extend,
				       &data->fsp_baud_setting);
	if (err) {
		return err;
	}

	fsp_err = R_SCI_UART_Close(&data->sci);
	fsp_err |= R_SCI_UART_Open(&data->sci, &data->fsp_config);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: serial: uart configure failed");
		return -EIO;
	}
	memcpy(&data->uart_config, config, sizeof(*config));

	return 0;
}

static int uart_ra_sci_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ra_sci_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(*cfg));
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_ra_sci_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	uint8_t num_tx = 0U;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		while ((size - num_tx > 0) && cfg->regs->FDR_b.T < data->sci.fifo_depth) {
			/* Send a character (8bit , parity none) */
			cfg->regs->FTDRL = tx_data[num_tx++];
		}
		cfg->regs->SSR_FIFO &= (uint8_t)~SCI_UART_SSR_FIFO_TDFE_TEND;
	} else
#endif
	{
		if (size > 0 && cfg->regs->SSR_b.TDRE) {
			/* Send a character (8bit , parity none) */
			cfg->regs->TDR = tx_data[num_tx++];
		}
	};

	return num_tx;
}

static int uart_ra_sci_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	uint8_t num_rx = 0U;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		while ((size - num_rx > 0) && cfg->regs->FDR_b.R > 0) {
			/* Receive a character (8bit , parity none) */
			rx_data[num_rx++] = cfg->regs->FRDRL;
		}
		cfg->regs->SSR_FIFO &= (uint8_t)~SCI_UART_SSR_FIFO_DR_RDF;
	} else
#endif
	{
		if (size > 0 && cfg->regs->SSR_b.RDRF) {
			/* Receive a character (8bit , parity none) */
			rx_data[num_rx++] = cfg->regs->RDR;
		}
		cfg->regs->SSR &= (uint8_t)~R_SCI0_SSR_RDRF_Msk;
	}

	return num_rx;
}

static void uart_ra_sci_irq_tx_enable(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		cfg->regs->SSR_FIFO &= (uint8_t)~SCI_UART_SSR_FIFO_TDFE_TEND;
	} else
#endif
	{
		cfg->regs->SSR = (uint8_t)~SCI_UART_SSR_TDRE_TEND;
	}

	cfg->regs->SCR |= (R_SCI0_SCR_TIE_Msk | R_SCI0_SCR_TEIE_Msk);
}

static void uart_ra_sci_irq_tx_disable(const struct device *dev)
{
	const struct uart_ra_sci_config *cfg = dev->config;

	cfg->regs->SCR &= ~(R_SCI0_SCR_TIE_Msk | R_SCI0_SCR_TEIE_Msk);
}

static int uart_ra_sci_irq_tx_ready(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	int ret;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		ret = (cfg->regs->SCR_b.TIE == 1U) && (data->ssr & R_SCI0_SSR_FIFO_TDFE_Msk);
	} else
#endif
	{
		ret = (cfg->regs->SCR_b.TIE == 1U) && (data->ssr & R_SCI0_SSR_TDRE_Msk);
	}

	return ret;
}

static int uart_ra_sci_irq_tx_complete(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;

	return (cfg->regs->SCR_b.TEIE == 1U) && (data->ssr & BIT(R_SCI0_SSR_TEND_Pos));
}

static void uart_ra_sci_irq_rx_enable(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		cfg->regs->SSR_FIFO &= (uint8_t) ~(SCI_UART_SSR_FIFO_DR_RDF);
	} else
#endif
	{
		cfg->regs->SSR_b.RDRF = 0U;
	}
	cfg->regs->SCR_b.RIE = 1U;
}

static void uart_ra_sci_irq_rx_disable(const struct device *dev)
{
	const struct uart_ra_sci_config *cfg = dev->config;

	cfg->regs->SCR_b.RIE = 0U;
}

static int uart_ra_sci_irq_rx_ready(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	int ret;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		ret = (cfg->regs->SCR_b.RIE == 1U) && (data->ssr & SCI_UART_SSR_FIFO_DR_RDF);
	} else
#endif
	{
		ret = (cfg->regs->SCR_b.RIE == 1U) && (data->ssr & R_SCI0_SSR_RDRF_Msk);
	}

	return ret;
}

static void uart_ra_sci_irq_err_enable(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;

	NVIC_EnableIRQ(data->fsp_config.eri_irq);
}

static void uart_ra_sci_irq_err_disable(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;

	NVIC_DisableIRQ(data->fsp_config.eri_irq);
}

static int uart_ra_sci_irq_is_pending(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	uint8_t scr;
	uint8_t ssr;
	int ret;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		scr = cfg->regs->SCR;
		ssr = cfg->regs->SSR_FIFO;
		ret = ((scr & R_SCI0_SCR_TIE_Msk) &&
		       (ssr & (R_SCI0_SSR_FIFO_TEND_Msk | R_SCI0_SSR_FIFO_TDFE_Msk))) ||
		      ((scr & R_SCI0_SCR_RIE_Msk) &&
		       ((ssr & (R_SCI0_SSR_FIFO_RDF_Msk | R_SCI0_SSR_FIFO_DR_Msk |
				R_SCI0_SSR_FIFO_FER_Msk | R_SCI0_SSR_FIFO_ORER_Msk |
				R_SCI0_SSR_FIFO_PER_Msk))));
	} else
#endif
	{
		scr = cfg->regs->SCR;
		ssr = cfg->regs->SSR;
		ret = ((scr & R_SCI0_SCR_TIE_Msk) &&
		       (ssr & (R_SCI0_SSR_TEND_Msk | R_SCI0_SSR_TDRE_Msk))) ||
		      ((scr & R_SCI0_SCR_RIE_Msk) &&
		       (ssr & (R_SCI0_SSR_RDRF_Msk | R_SCI0_SSR_PER_Msk | R_SCI0_SSR_FER_Msk |
			       R_SCI0_SSR_ORER_Msk)));
	}

	return ret;
}

static int uart_ra_sci_irq_update(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		data->ssr = cfg->regs->SSR_FIFO;
		uint8_t ssr = data->ssr ^ (R_SCI0_SSR_FIFO_ORER_Msk | R_SCI0_SSR_FIFO_FER_Msk |
					   R_SCI0_SSR_FIFO_PER_Msk);
		cfg->regs->SSR_FIFO &= ssr;
	} else
#endif
	{
		data->ssr = cfg->regs->SSR;
		uint8_t ssr =
			data->ssr ^ (R_SCI0_SSR_ORER_Msk | R_SCI0_SSR_FER_Msk | R_SCI0_SSR_PER_Msk);
		cfg->regs->SSR_FIFO &= ssr;
	}

	return 1;
}

static void uart_ra_sci_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_ra_sci_data *data = dev->data;

	data->user_cb = cb;
	data->user_cb_data = cb_data;

#if CONFIG_UART_EXCLUSIVE_API_CALLBACKS
	data->async_user_cb = NULL;
	data->async_user_cb_data = NULL;
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static int fsp_err_to_errno(fsp_err_t fsp_err)
{
	switch (fsp_err) {
	case FSP_ERR_INVALID_ARGUMENT:
		return -EINVAL;
	case FSP_ERR_NOT_OPEN:
		return -EIO;
	case FSP_ERR_IN_USE:
		return -EBUSY;
	case FSP_ERR_UNSUPPORTED:
		return -ENOTSUP;
	case 0:
		return 0;
	default:
		return -EINVAL;
	}
}

static int uart_ra_sci_async_callback_set(const struct device *dev, uart_callback_t cb,
					  void *cb_data)
{
	struct uart_ra_sci_data *data = dev->data;

	data->async_user_cb = cb;
	data->async_user_cb_data = cb_data;

#if CONFIG_UART_EXCLUSIVE_API_CALLBACKS
	data->user_cb = NULL;
	data->user_cb_data = NULL;
#endif
	return 0;
}

static int uart_ra_sci_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
				int32_t timeout)
{
	struct uart_ra_sci_data *data = dev->data;
	int err;

	err = fsp_err_to_errno(R_SCI_UART_Write(&data->sci, buf, len));
	if (err) {
		return err;
	}
	data->tx_buf_cap = len;
	if (timeout != SYS_FOREVER_US && timeout != 0) {
		k_work_reschedule(&data->tx_timeout, Z_TIMEOUT_US(timeout));
	}

	return 0;
}

static inline void async_user_callback(const struct device *dev, struct uart_event *event)
{
	struct uart_ra_sci_data *data = dev->data;

	if (data->async_user_cb) {
		data->async_user_cb(dev, event, data->async_user_cb_data);
	}
}

static inline void async_rx_release_buf(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx.buf = (uint8_t *)data->rx_buffer,
	};
	async_user_callback(dev, &event);
	data->rx_buffer = NULL;
	data->rx_buf_offset = 0;
	data->rx_buf_len = 0;
	data->rx_buf_cap = 0;
}

static inline void async_rx_release_next_buf(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx.buf = (uint8_t *)data->rx_next_buf,
	};
	async_user_callback(dev, &event);
	data->rx_next_buf = NULL;
}

static inline void async_rx_req_buf(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &event);
}

static inline void async_rx_disable(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	struct uart_event event = {
		.type = UART_RX_DISABLED,
	};
	async_user_callback(dev, &event);

	/* Disable the RXI request and clear the status flag to be ready for the next reception */
	cfg->regs->SCR_b.RIE = 0;
#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth != 0) {
		cfg->regs->SSR_FIFO &= (uint8_t)~SCI_UART_SSR_FIFO_DR_RDF;
	} else
#endif
	{
		cfg->regs->SSR_b.RDRF = 0;
	}
}

static inline void async_rx_ready(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;

	if (!data->rx_buf_len) {
		return;
	}

	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = (uint8_t *)data->rx_buffer,
		.data.rx.offset = data->rx_buf_offset,
		.data.rx.len = data->rx_buf_len,
	};
	async_user_callback(data->dev, &event);
	data->rx_buf_offset += data->rx_buf_len;
	data->rx_buf_len = 0;
}

static inline void disable_tx(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;

	/* Transmit interrupts must be disabled to start with. */
	cfg->regs->SCR &= (uint8_t) ~(R_SCI0_SCR_TIE_Msk | R_SCI0_SCR_TEIE_Msk);

	/*
	 * Make sure no transmission is in progress. Setting CCR0_b.TE to 0 when CSR_b.TEND
	 * is 0 causes SCI peripheral to work abnormally.
	 */
	while (IS_ENABLED(CONFIG_UART_RA_SCI_UART_FIFO_ENABLE) && data->sci.fifo_depth
		       ? cfg->regs->SSR_FIFO_b.TEND != 1U
		       : cfg->regs->SSR_b.TEND != 1U) {
	}

	cfg->regs->SCR_b.TE = 0;
}

static inline void enable_tx(const struct device *dev)
{
	const struct uart_ra_sci_config *cfg = dev->config;

	cfg->regs->SCR_b.TE = 1;
}

static int uart_ra_sci_async_tx_abort(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	int err = 0;

	if (!data->sci.p_tx_src) {
		return -EFAULT;
	}

	disable_tx(dev);

	if (FSP_SUCCESS != R_SCI_UART_Abort(&data->sci, UART_DIR_TX)) {
		LOG_DBG("drivers: serial: uart abort tx failed");
		err = -EIO;
		goto unlock;
	}
	transfer_properties_t tx_properties = {0};

	if (FSP_SUCCESS != R_DTC_InfoGet(data->tx_transfer.p_ctrl, &tx_properties)) {
		LOG_DBG("drivers: serial: uart abort tx failed");
		err = -EIO;
		goto unlock;
	}
	struct uart_event event = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = (uint8_t *)data->sci.p_tx_src,
		.data.tx.len = data->tx_buf_cap - tx_properties.transfer_length_remaining,
	};
	async_user_callback(dev, &event);
	k_work_cancel_delayable(&data->tx_timeout);

unlock:
	enable_tx(dev);
	return err;
}

static int uart_ra_sci_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				       int32_t timeout)
{
	struct uart_ra_sci_data *data = dev->data;
	const struct uart_ra_sci_config *cfg = dev->config;
	int err = 0;
	unsigned int key = irq_lock();

	if (data->rx_buffer) {
		err = -EAGAIN;
		goto unlock;
	}

#if CONFIG_UART_RA_SCI_UART_FIFO_ENABLE
	if (data->sci.fifo_depth) {
		cfg->regs->SSR_FIFO &= (uint8_t) ~(SCI_UART_SSR_FIFO_ERR_MSK);
	} else
#endif
	{
		cfg->regs->SSR = (uint8_t)~SCI_UART_SSR_ERR_MSK;
	}

	err = fsp_err_to_errno(R_SCI_UART_Read(&data->sci, buf, len));
	if (err) {
		goto unlock;
	}

	data->rx_timeout = timeout;
	data->rx_buffer = buf;
	data->rx_buf_cap = len;
	data->rx_buf_len = 0;
	data->rx_buf_offset = 0;

	/* Call buffer request user callback */
	async_rx_req_buf(dev);
	cfg->regs->SCR_b.RIE = 1;

unlock:
	irq_unlock(key);
	return err;
}

static int uart_ra_sci_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_ra_sci_data *data = dev->data;

	data->rx_next_buf = buf;
	data->rx_next_buf_cap = len;

	return 0;
}

static int uart_ra_sci_async_rx_disable(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	uint32_t remaining_byte = 0;
	int err = 0;
	unsigned int key = irq_lock();

	if (!data->rx_buffer) {
		err = -EAGAIN;
		goto unlock;
	}

	k_work_cancel_delayable(&data->rx_timeout_work);
	if (FSP_SUCCESS != R_SCI_UART_ReadStop(&data->sci, &remaining_byte)) {
		LOG_DBG("drivers: serial: uart stop reading failed");
		err = -EIO;
		goto unlock;
	}

	async_rx_ready(dev);
	async_rx_release_buf(dev);
	async_rx_release_next_buf(dev);
	async_rx_disable(dev);

unlock:
	irq_unlock(key);
	return err;
}

static inline void async_evt_rx_err(const struct device *dev, enum uart_rx_stop_reason reason)
{
	struct uart_ra_sci_data *data = dev->data;

	k_work_cancel_delayable(&data->rx_timeout_work);
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = reason,
		.data.rx_stop.data.buf = (uint8_t *)data->sci.p_rx_dest,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.len =
			data->rx_buf_cap - data->rx_buf_offset - data->sci.rx_dest_bytes,
	};
	async_user_callback(dev, &event);
}

static inline void async_evt_rx_complete(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
	unsigned int key = irq_lock();

	async_rx_ready(dev);
	async_rx_release_buf(dev);
	if (data->rx_next_buf) {
		data->rx_buffer = data->rx_next_buf;
		data->rx_buf_offset = 0;
		data->rx_buf_cap = data->rx_next_buf_cap;
		data->rx_next_buf = NULL;
		R_SCI_UART_Read(&data->sci, data->rx_buffer, data->rx_buf_cap);
		async_rx_req_buf(dev);
	} else {
		async_rx_disable(dev);
	}
	irq_unlock(key);
}

static inline void async_evt_tx_done(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;

	k_work_cancel_delayable(&data->tx_timeout);
	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = (uint8_t *)data->sci.p_tx_src,
		.data.tx.len = data->tx_buf_cap,
	};
	async_user_callback(dev, &event);
}

static void uart_ra_sci_callback_adapter(struct st_uart_callback_arg *fsp_args)
{
	const struct device *dev = fsp_args->p_context;

	switch (fsp_args->event) {
	case UART_EVENT_TX_COMPLETE:
		async_evt_tx_done(dev);
		break;
	case UART_EVENT_RX_COMPLETE:
		async_evt_rx_complete(dev);
		break;
	case UART_EVENT_ERR_PARITY:
		async_evt_rx_err(dev, UART_ERROR_PARITY);
		break;
	case UART_EVENT_ERR_FRAMING:
		async_evt_rx_err(dev, UART_ERROR_FRAMING);
		break;
	case UART_EVENT_ERR_OVERFLOW:
		async_evt_rx_err(dev, UART_ERROR_OVERRUN);
		break;
	case UART_EVENT_BREAK_DETECT:
		async_evt_rx_err(dev, UART_BREAK);
		break;
	case UART_EVENT_TX_DATA_EMPTY:
	case UART_EVENT_RX_CHAR:
		break;
	}
}

static void uart_ra_sci_rx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_ra_sci_data *data =
		CONTAINER_OF(dwork, struct uart_ra_sci_data, rx_timeout_work);
	unsigned int key = irq_lock();

	async_rx_ready(data->dev);
	irq_unlock(key);
}

static void uart_ra_sci_tx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_ra_sci_data *data = CONTAINER_OF(dwork, struct uart_ra_sci_data, tx_timeout);

	uart_ra_sci_async_tx_abort(data->dev);
}

#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_ra_sci_driver_api = {
	.poll_in = uart_ra_sci_poll_in,
	.poll_out = uart_ra_sci_poll_out,
	.err_check = uart_ra_sci_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ra_sci_configure,
	.config_get = uart_ra_sci_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ra_sci_fifo_fill,
	.fifo_read = uart_ra_sci_fifo_read,
	.irq_tx_enable = uart_ra_sci_irq_tx_enable,
	.irq_tx_disable = uart_ra_sci_irq_tx_disable,
	.irq_tx_ready = uart_ra_sci_irq_tx_ready,
	.irq_rx_enable = uart_ra_sci_irq_rx_enable,
	.irq_rx_disable = uart_ra_sci_irq_rx_disable,
	.irq_tx_complete = uart_ra_sci_irq_tx_complete,
	.irq_rx_ready = uart_ra_sci_irq_rx_ready,
	.irq_err_enable = uart_ra_sci_irq_err_enable,
	.irq_err_disable = uart_ra_sci_irq_err_disable,
	.irq_is_pending = uart_ra_sci_irq_is_pending,
	.irq_update = uart_ra_sci_irq_update,
	.irq_callback_set = uart_ra_sci_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#if CONFIG_UART_ASYNC_API
	.callback_set = uart_ra_sci_async_callback_set,
	.tx = uart_ra_sci_async_tx,
	.tx_abort = uart_ra_sci_async_tx_abort,
	.rx_enable = uart_ra_sci_async_rx_enable,
	.rx_buf_rsp = uart_ra_sci_async_rx_buf_rsp,
	.rx_disable = uart_ra_sci_async_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

static int uart_ra_sci_init(const struct device *dev)
{
	const struct uart_ra_sci_config *config = dev->config;
	struct uart_ra_sci_data *data = dev->data;
	int ret;
	fsp_err_t fsp_err;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Setup fsp sci_uart setting */
	ret = uart_ra_sci_apply_config(&data->uart_config, &data->fsp_config,
				       &data->fsp_config_extend, &data->fsp_baud_setting);
	if (ret != 0) {
		return ret;
	}

	data->fsp_config_extend.p_baud_setting = &data->fsp_baud_setting;
#if defined(CONFIG_UART_ASYNC_API)
	data->fsp_config.p_callback = uart_ra_sci_callback_adapter;
	data->fsp_config.p_context = dev;
	k_work_init_delayable(&data->tx_timeout, uart_ra_sci_tx_timeout_handler);
	k_work_init_delayable(&data->rx_timeout_work, uart_ra_sci_rx_timeout_handler);
#endif /* defined(CONFIG_UART_ASYNC_API) */
	data->fsp_config.p_extend = &data->fsp_config_extend;

	fsp_err = R_SCI_UART_Open(&data->sci, &data->fsp_config);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: uart: initialize failed");
		return -EIO;
	}
	irq_disable(data->fsp_config.eri_irq);
	return 0;
}

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
static void uart_ra_sci_rxi_isr(const struct device *dev)
{
	struct uart_ra_sci_data *data = dev->data;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
		goto out;
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	if (data->rx_timeout != SYS_FOREVER_US && data->rx_timeout != 0) {
		k_work_reschedule(&data->rx_timeout_work, Z_TIMEOUT_US(data->rx_timeout));
	}
	data->rx_buf_len++;
	if (data->rx_buf_len + data->rx_buf_offset == data->rx_buf_cap) {
		sci_uart_rxi_isr();
	} else {
		goto out;
	}
#endif
out:
	R_ICU->IELSR_b[data->fsp_config.rxi_irq].IR = 0U;
}

static void uart_ra_sci_txi_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_ra_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
		R_ICU->IELSR_b[data->fsp_config.txi_irq].IR = 0U;
		return;
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	sci_uart_txi_isr();
#endif
}

static void uart_ra_sci_tei_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_ra_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
		R_ICU->IELSR_b[data->fsp_config.tei_irq].IR = 0U;
		return;
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	sci_uart_tei_isr();
#endif
}

static void uart_ra_sci_eri_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_ra_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
		R_ICU->IELSR_b[data->fsp_config.eri_irq].IR = 0U;
		return;
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	sci_uart_eri_isr();
#endif
}
#endif

#define _ELC_EVENT_SCI_RXI(channel) ELC_EVENT_SCI##channel##_RXI
#define _ELC_EVENT_SCI_TXI(channel) ELC_EVENT_SCI##channel##_TXI
#define _ELC_EVENT_SCI_TEI(channel) ELC_EVENT_SCI##channel##_TEI
#define _ELC_EVENT_SCI_ERI(channel) ELC_EVENT_SCI##channel##_ERI

#define ELC_EVENT_SCI_RXI(channel) _ELC_EVENT_SCI_RXI(channel)
#define ELC_EVENT_SCI_TXI(channel) _ELC_EVENT_SCI_TXI(channel)
#define ELC_EVENT_SCI_TEI(channel) _ELC_EVENT_SCI_TEI(channel)
#define ELC_EVENT_SCI_ERI(channel) _ELC_EVENT_SCI_ERI(channel)

#if CONFIG_UART_ASYNC_API
#define UART_RA_SCI_ASYNC_INIT(index)                                                              \
	.rx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED, \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,  \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_EACH,                         \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,        \
			.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.rx_transfer_cfg_extend = {.activation_source =                                            \
					   DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq)},       \
	.rx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &uart_ra_sci_data_##index.rx_transfer_info,                      \
			.p_extend = &uart_ra_sci_data_##index.rx_transfer_cfg_extend,              \
	},                                                                                         \
	.rx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = &uart_ra_sci_data_##index.rx_transfer_ctrl,                      \
			.p_cfg = &uart_ra_sci_data_##index.rx_transfer_cfg,                        \
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
	.tx_transfer_cfg_extend = {.activation_source =                                            \
					   DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq)},       \
	.tx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &uart_ra_sci_data_##index.tx_transfer_info,                      \
			.p_extend = &uart_ra_sci_data_##index.tx_transfer_cfg_extend,              \
	},                                                                                         \
	.tx_transfer = {                                                                           \
		.p_ctrl = &uart_ra_sci_data_##index.tx_transfer_ctrl,                              \
		.p_cfg = &uart_ra_sci_data_##index.tx_transfer_cfg,                                \
		.p_api = &g_transfer_on_dtc,                                                       \
	},

#define UART_RA_SCI_DTC_INIT(index)                                                                \
	{                                                                                          \
		uart_ra_sci_data_##index.fsp_config.p_transfer_rx =                                \
			&uart_ra_sci_data_##index.rx_transfer;                                     \
		uart_ra_sci_data_##index.fsp_config.p_transfer_tx =                                \
			&uart_ra_sci_data_##index.tx_transfer;                                     \
	}

#else
#define UART_RA_SCI_ASYNC_INIT(index)
#define UART_RA_SCI_DTC_INIT(index)
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
#define UART_RA_SCI_IRQ_INIT(index)                                                                \
	{                                                                                          \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq)] =                    \
			ELC_EVENT_SCI_RXI(DT_INST_PROP(index, channel));                           \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq)] =                    \
			ELC_EVENT_SCI_TXI(DT_INST_PROP(index, channel));                           \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq)] =                    \
			ELC_EVENT_SCI_TEI(DT_INST_PROP(index, channel));                           \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq)] =                    \
			ELC_EVENT_SCI_ERI(DT_INST_PROP(index, channel));                           \
                                                                                                   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, priority),                  \
			    uart_ra_sci_rxi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),                  \
			    uart_ra_sci_txi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority),                  \
			    uart_ra_sci_tei_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, priority),                  \
			    uart_ra_sci_eri_isr, DEVICE_DT_INST_GET(index), 0);                    \
                                                                                                   \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq));                       \
	}
#else
#define UART_RA_SCI_IRQ_INIT(index)
#endif

#define UART_RA_SCI_INIT(index)                                                                    \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
	static const struct uart_ra_sci_config uart_ra_sci_config_##index = {                      \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.regs = (R_SCI0_Type *)DT_REG_ADDR(DT_INST_PARENT(index)),                         \
	};                                                                                         \
                                                                                                   \
	static struct uart_ra_sci_data uart_ra_sci_data_##index = {                                \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = COND_CODE_1(DT_INST_PROP(index, hw_flow_control),     \
							 (UART_CFG_FLOW_CTRL_RTS_CTS),             \
							 (UART_CFG_FLOW_CTRL_NONE)),               \
			},                                                                         \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.rxi_ipl = DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, priority),   \
				.rxi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),        \
				.txi_ipl = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),   \
				.txi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),        \
				.tei_ipl = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority),   \
				.tei_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),        \
				.eri_ipl = DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, priority),   \
				.eri_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),        \
			},                                                                         \
		.fsp_config_extend = {},                                                           \
		.fsp_baud_setting = {},                                                            \
		.dev = DEVICE_DT_INST_GET(index),                                                  \
		UART_RA_SCI_ASYNC_INIT(index)};                                                    \
                                                                                                   \
	static int uart_ra_sci_init##index(const struct device *dev)                               \
	{                                                                                          \
		UART_RA_SCI_IRQ_INIT(index);                                                       \
		UART_RA_SCI_DTC_INIT(index);                                                       \
		int err = uart_ra_sci_init(dev);                                                   \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, uart_ra_sci_init##index, NULL, &uart_ra_sci_data_##index,     \
			      &uart_ra_sci_config_##index, PRE_KERNEL_1,                           \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_ra_sci_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RA_SCI_INIT)
