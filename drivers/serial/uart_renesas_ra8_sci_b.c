/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra8_uart_sci_b

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "r_sci_b_uart.h"
#include "r_dtc.h"
#include "r_transfer_api.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ra8_uart_sci_b);

#if defined(CONFIG_UART_ASYNC_API)
void sci_b_uart_rxi_isr(void);
void sci_b_uart_txi_isr(void);
void sci_b_uart_tei_isr(void);
void sci_b_uart_eri_isr(void);
#endif

struct uart_ra_sci_b_config {
	R_SCI_B0_Type * const regs;
	const struct pinctrl_dev_config *pcfg;
};

struct uart_ra_sci_b_data {
	const struct device *dev;
	struct st_sci_b_uart_instance_ctrl sci;
	struct uart_config uart_config;
	struct st_uart_cfg fsp_config;
	struct st_sci_b_uart_extended_cfg fsp_config_extend;
	struct st_sci_b_baud_setting_t fsp_baud_setting;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_callback_user_data_t user_cb;
	void *user_cb_data;
	uint32_t csr;
#endif
#if defined(CONFIG_UART_ASYNC_API)
	/* RX */
	struct st_transfer_instance rx_transfer;
	struct st_dtc_instance_ctrl rx_transfer_ctrl;
	struct st_transfer_info rx_transfer_info;
	struct st_transfer_cfg rx_transfer_cfg;
	struct st_dtc_extended_cfg rx_transfer_cfg_extend;
	struct k_work_delayable rx_timeout_work;
	size_t rx_timeout;
	uint8_t *rx_buffer;
	size_t rx_buffer_len;
	size_t rx_buffer_cap;
	size_t rx_buffer_offset;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_cap;

	/* TX */
	struct st_transfer_instance tx_transfer;
	struct st_dtc_instance_ctrl tx_transfer_ctrl;
	struct st_transfer_info tx_transfer_info;
	struct st_transfer_cfg tx_transfer_cfg;
	struct st_dtc_extended_cfg tx_transfer_cfg_extend;
	struct k_work_delayable tx_timeout_work;
	size_t tx_timeout;
	uint8_t *tx_buffer;
	size_t tx_buffer_len;
	size_t tx_buffer_cap;

	uart_callback_t async_user_cb;
	void *async_user_cb_data;
#endif
};

static int uart_ra_sci_b_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	/* Check if async reception was enabled */
	if (IS_ENABLED(CONFIG_UART_ASYNC_API) && cfg->regs->CCR0_b.RIE) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE) ? cfg->regs->FRSR_b.R == 0U
							      : cfg->regs->CSR_b.RDRF == 0U) {
		/* There are no characters available to read. */
		return -1;
	}

	/* got a character */
	*c = (unsigned char)cfg->regs->RDR;

	return 0;
}

static void uart_ra_sci_b_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	while (cfg->regs->CSR_b.TEND == 0U) {
	}

	cfg->regs->TDR_BY = c;
}

static int uart_ra_sci_b_err_check(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	const uint32_t status = cfg->regs->CSR;
	int errors = 0;

	if ((status & BIT(R_SCI_B0_CSR_ORER_Pos)) != 0) {
		errors |= UART_ERROR_OVERRUN;
	}
	if ((status & BIT(R_SCI_B0_CSR_PER_Pos)) != 0) {
		errors |= UART_ERROR_PARITY;
	}
	if ((status & BIT(R_SCI_B0_CSR_FER_Pos)) != 0) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int uart_ra_sci_b_apply_config(const struct uart_config *config,
				      struct st_uart_cfg *fsp_config,
				      struct st_sci_b_uart_extended_cfg *fsp_config_extend,
				      struct st_sci_b_baud_setting_t *fsp_baud_setting)
{
	fsp_err_t fsp_err;

	fsp_err = R_SCI_B_UART_BaudCalculate(config->baudrate, false, 5000, fsp_baud_setting);
	__ASSERT(fsp_err == 0, "sci_uart: baud calculate error");

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

	fsp_config_extend->clock = SCI_B_UART_CLOCK_INT;
	fsp_config_extend->rx_edge_start = SCI_B_UART_START_BIT_FALLING_EDGE;
	fsp_config_extend->noise_cancel = SCI_B_UART_NOISE_CANCELLATION_DISABLE;
	fsp_config_extend->flow_control_pin = UINT16_MAX;
#if CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE
	fsp_config_extend->rx_fifo_trigger = 0x8;
#endif /* CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE */

	switch (config->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		fsp_config_extend->flow_control = 0;
		fsp_config_extend->rs485_setting.enable = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		fsp_config_extend->flow_control = SCI_B_UART_FLOW_CONTROL_HARDWARE_CTSRTS;
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

static int uart_ra_sci_b_configure(const struct device *dev, const struct uart_config *cfg)
{
	int err;
	fsp_err_t fsp_err;
	struct uart_ra_sci_b_data *data = dev->data;

	err = uart_ra_sci_b_apply_config(cfg, &data->fsp_config, &data->fsp_config_extend,
					 &data->fsp_baud_setting);
	if (err) {
		return err;
	}

	fsp_err = R_SCI_B_UART_Close(&data->sci);
	__ASSERT(fsp_err == 0, "sci_uart: configure: fsp close failed");

	fsp_err = R_SCI_B_UART_Open(&data->sci, &data->fsp_config);
	__ASSERT(fsp_err == 0, "sci_uart: configure: fsp open failed");
	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	return err;
}

static int uart_ra_sci_b_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ra_sci_b_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(*cfg));
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_ra_sci_b_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;
	int num_tx = 0U;

	if (IS_ENABLED(CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE) && data->sci.fifo_depth > 0) {
		while ((size - num_tx > 0) && cfg->regs->FTSR != 0x10U) {
			/* FTSR flag will be cleared with byte write to TDR register */

			/* Send a character (8bit , parity none) */
			cfg->regs->TDR_BY = tx_data[num_tx++];
		}
	} else {
		if (size > 0 && cfg->regs->CSR_b.TDRE) {
			/* TEND flag will be cleared with byte write to TDR register */

			/* Send a character (8bit , parity none) */
			cfg->regs->TDR_BY = tx_data[num_tx++];
		}
	}

	return num_tx;
}

static int uart_ra_sci_b_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;
	int num_rx = 0U;

	if (IS_ENABLED(CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE) && data->sci.fifo_depth > 0) {
		while ((size - num_rx > 0) && cfg->regs->FRSR_b.R > 0U) {
			/* FRSR.DR flag will be cleared with byte write to RDR register */

			/* Receive a character (8bit , parity none) */
			rx_data[num_rx++] = cfg->regs->RDR;
		}
		if (cfg->regs->FRSR_b.R == 0U) {
			cfg->regs->CFCLR_b.RDRFC = 1U;
			cfg->regs->FFCLR_b.DRC = 1U;
		}
	} else {
		if (size > 0 && cfg->regs->CSR_b.RDRF) {
			/* Receive a character (8bit , parity none) */
			rx_data[num_rx++] = cfg->regs->RDR;
		}
	}

	/* Clear overrun error flag */
	cfg->regs->CFCLR_b.ORERC = 0U;

	return num_rx;
}

static void uart_ra_sci_b_irq_tx_enable(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	cfg->regs->CCR0 |= (BIT(R_SCI_B0_CCR0_TIE_Pos) | BIT(R_SCI_B0_CCR0_TEIE_Pos));
}

static void uart_ra_sci_b_irq_tx_disable(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	cfg->regs->CCR0 &= ~(BIT(R_SCI_B0_CCR0_TIE_Pos) | BIT(R_SCI_B0_CCR0_TEIE_Pos));
}

static int uart_ra_sci_b_irq_tx_ready(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;

	return (cfg->regs->CCR0_b.TIE == 1U) &&
	       (data->csr & (BIT(R_SCI_B0_CSR_TDRE_Pos) | BIT(R_SCI_B0_CSR_TEND_Pos)));
}

static int uart_ra_sci_b_irq_tx_complete(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;

	return (cfg->regs->CCR0_b.TEIE == 1U) && (data->csr & BIT(R_SCI_B0_CSR_TEND_Pos));
}

static void uart_ra_sci_b_irq_rx_enable(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	cfg->regs->CCR0_b.RIE = 1U;
}

static void uart_ra_sci_b_irq_rx_disable(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	cfg->regs->CCR0_b.RIE = 0U;
}

static int uart_ra_sci_b_irq_rx_ready(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;

	return (cfg->regs->CCR0_b.RIE == 1U) &&
	       ((data->csr & BIT(R_SCI_B0_CSR_RDRF_Pos)) ||
		(IS_ENABLED(CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE) && cfg->regs->FRSR_b.DR == 1U));
}

static void uart_ra_sci_b_irq_err_enable(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	NVIC_EnableIRQ(data->fsp_config.eri_irq);
}

static void uart_ra_sci_b_irq_err_disable(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	NVIC_DisableIRQ(data->fsp_config.eri_irq);
}

static int uart_ra_sci_b_irq_is_pending(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	const uint32_t ccr0 = cfg->regs->CCR0;
	const uint32_t csr = cfg->regs->CSR;

	const bool tx_pending = ((ccr0 & BIT(R_SCI_B0_CCR0_TIE_Pos)) &&
				 (csr & (BIT(R_SCI_B0_CSR_TEND_Pos) | BIT(R_SCI_B0_CSR_TDRE_Pos))));
	const bool rx_pending =
		((ccr0 & BIT(R_SCI_B0_CCR0_RIE_Pos)) &&
		 ((csr & (BIT(R_SCI_B0_CSR_RDRF_Pos) | BIT(R_SCI_B0_CSR_PER_Pos) |
			  BIT(R_SCI_B0_CSR_FER_Pos) | BIT(R_SCI_B0_CSR_ORER_Pos))) ||
		  (IS_ENABLED(CONFIG_UART_RA_SCI_B_UART_FIFO_ENABLE) &&
		   cfg->regs->FRSR_b.DR == 1U)));

	return tx_pending || rx_pending;
}

static int uart_ra_sci_b_irq_update(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;
	uint32_t cfclr = 0;

	data->csr = cfg->regs->CSR;

	if (data->csr & BIT(R_SCI_B0_CSR_PER_Pos)) {
		cfclr |= BIT(R_SCI_B0_CFCLR_PERC_Pos);
	}
	if (data->csr & BIT(R_SCI_B0_CSR_FER_Pos)) {
		cfclr |= BIT(R_SCI_B0_CFCLR_FERC_Pos);
	}
	if (data->csr & BIT(R_SCI_B0_CSR_ORER_Pos)) {
		cfclr |= BIT(R_SCI_B0_CFCLR_ORERC_Pos);
	}

	cfg->regs->CFCLR = cfclr;

	return 1;
}

static void uart_ra_sci_b_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_ra_sci_b_data *data = dev->data;

	data->user_cb = cb;
	data->user_cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static inline void async_user_callback(const struct device *dev, struct uart_event *event)
{
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->async_user_cb) {
		data->async_user_cb(dev, event, data->async_user_cb_data);
	}
}

static inline void async_rx_error(const struct device *dev, enum uart_rx_stop_reason reason)
{
	struct uart_ra_sci_b_data *data = dev->data;
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = reason,
		.data.rx_stop.data.buf = (uint8_t *)data->rx_buffer,
		.data.rx_stop.data.offset = data->rx_buffer_offset,
		.data.rx_stop.data.len = data->rx_buffer_len,
	};
	async_user_callback(dev, &event);
}

static inline void async_rx_disabled(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_DISABLED,
	};
	async_user_callback(dev, &event);
}

static inline void async_request_rx_buffer(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_BUF_REQUEST,
	};
	async_user_callback(dev, &event);
}

static inline void async_rx_ready(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->rx_buffer_len == 0) {
		return;
	}

	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = (uint8_t *)data->rx_buffer,
		.data.rx.offset = data->rx_buffer_offset,
		.data.rx.len = data->rx_buffer_len,
	};
	async_user_callback(dev, &event);

	data->rx_buffer_offset += data->rx_buffer_len;
	data->rx_buffer_len = 0;
}

static inline void async_replace_rx_buffer(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->rx_next_buffer != NULL) {
		data->rx_buffer = data->rx_next_buffer;
		data->rx_buffer_cap = data->rx_next_buffer_cap;

		R_SCI_B_UART_Read(&data->sci, data->rx_buffer, data->rx_buffer_cap);

		data->rx_next_buffer = NULL;
		data->rx_next_buffer_cap = 0;
		async_request_rx_buffer(dev);
	} else {
		async_rx_disabled(dev);
	}
}

static inline void async_release_rx_buffer(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->rx_buffer == NULL) {
		return;
	}

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx.buf = (uint8_t *)data->rx_buffer,
	};
	async_user_callback(dev, &event);

	data->rx_buffer = NULL;
	data->rx_buffer_cap = 0;
	data->rx_buffer_len = 0;
	data->rx_buffer_offset = 0;
}

static inline void async_release_rx_next_buffer(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->rx_next_buffer == NULL) {
		return;
	}

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx.buf = (uint8_t *)data->rx_next_buffer,
	};
	async_user_callback(dev, &event);

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_cap = 0;
}

static inline void async_update_tx_buffer(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = (uint8_t *)data->tx_buffer,
		.data.tx.len = data->tx_buffer_cap,
	};
	async_user_callback(dev, &event);

	data->tx_buffer = NULL;
	data->tx_buffer_cap = 0;
}

static inline void async_tx_abort(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->tx_buffer_len < data->tx_buffer_cap) {
		struct uart_event event = {
			.type = UART_TX_ABORTED,
			.data.tx.buf = (uint8_t *)data->tx_buffer,
			.data.tx.len = data->tx_buffer_len,
		};
		async_user_callback(dev, &event);
	}

	data->tx_buffer = NULL;
	data->tx_buffer_cap = 0;
}

static inline void uart_ra_sci_b_async_timer_start(struct k_work_delayable *work, size_t timeout)
{
	if (timeout != SYS_FOREVER_US && timeout != 0) {
		LOG_DBG("Async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static inline int fsp_err_to_errno(fsp_err_t fsp_err)
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

static int uart_ra_sci_b_async_callback_set(const struct device *dev, uart_callback_t cb,
					    void *cb_data)

{
	struct uart_ra_sci_b_data *data = dev->data;
	unsigned int key = irq_lock();

	data->async_user_cb = cb;
	data->async_user_cb_data = cb_data;

	irq_unlock(key);
	return 0;
}

static int uart_ra_sci_b_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
				  int32_t timeout)
{
	struct uart_ra_sci_b_data *data = dev->data;
	int err = 0;

	unsigned int key = irq_lock();

	if (data->tx_buffer_len < data->tx_buffer_cap) {
		err = -EBUSY;
		goto unlock;
	}

	err = fsp_err_to_errno(R_SCI_B_UART_Write(&data->sci, buf, len));
	if (err != 0) {
		goto unlock;
	}

	data->tx_buffer = (uint8_t *)buf;
	data->tx_buffer_cap = len;

	uart_ra_sci_b_async_timer_start(&data->tx_timeout_work, timeout);

unlock:
	irq_unlock(key);
	return err;
}

static inline void disable_tx(const struct device *dev)
{
	const struct uart_ra_sci_b_config *cfg = dev->config;

	/* Transmit interrupts must be disabled to start with. */
	cfg->regs->CCR0 &= (uint32_t) ~(R_SCI_B0_CCR0_TIE_Msk | R_SCI_B0_CCR0_TEIE_Msk);

	/*
	 * Make sure no transmission is in progress. Setting CCR0_b.TE to 0 when CSR_b.TEND
	 * is 0 causes SCI peripheral to work abnormally.
	 */
	while (cfg->regs->CSR_b.TEND != 1U) {
	}

	cfg->regs->CCR0 &= (uint32_t) ~(R_SCI_B0_CCR0_TE_Msk);
	while (cfg->regs->CESR_b.TIST != 0U) {
	}
}

static int uart_ra_sci_b_async_tx_abort(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	int err = 0;

	disable_tx(dev);
	k_work_cancel_delayable(&data->tx_timeout_work);

	if (data->fsp_config.p_transfer_tx) {
		transfer_properties_t transfer_info;

		err = fsp_err_to_errno(R_DTC_InfoGet(&data->tx_transfer_ctrl, &transfer_info));
		if (err != 0) {
			return err;
		}
		data->tx_buffer_len = data->tx_buffer_cap - transfer_info.transfer_length_remaining;
	} else {
		data->tx_buffer_len = data->tx_buffer_cap - data->sci.tx_src_bytes;
	}

	R_SCI_B_UART_Abort(&data->sci, UART_DIR_TX);

	async_tx_abort(dev);

	return 0;
}

static void uart_ra_sci_b_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_ra_sci_b_data *data =
		CONTAINER_OF(dwork, struct uart_ra_sci_b_data, tx_timeout_work);

	uart_ra_sci_b_async_tx_abort(data->dev);
}

static int uart_ra_sci_b_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
					 int32_t timeout)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;
	int err = 0;

	k_work_cancel_delayable(&data->rx_timeout_work);

	unsigned int key = irq_lock();

	if (data->rx_buffer) {
		err = -EBUSY;
		goto unlock;
	}

	err = fsp_err_to_errno(R_SCI_B_UART_Read(&data->sci, buf, len));
	if (err != 0) {
		goto unlock;
	}

	data->rx_timeout = timeout;
	data->rx_buffer = buf;
	data->rx_buffer_cap = len;
	data->rx_buffer_len = 0;
	data->rx_buffer_offset = 0;

	cfg->regs->CCR0_b.RIE = 1U;

	async_request_rx_buffer(dev);

unlock:
	irq_unlock(key);
	return err;
}

static int uart_ra_sci_b_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_ra_sci_b_data *data = dev->data;

	data->rx_next_buffer = buf;
	data->rx_next_buffer_cap = len;

	return 0;
}

static int uart_ra_sci_b_async_rx_disable(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;
	const struct uart_ra_sci_b_config *cfg = dev->config;
	uint32_t remaining_byte = 0;
	int err = 0;
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->rx_timeout_work);

	err = fsp_err_to_errno(R_SCI_B_UART_ReadStop(&data->sci, &remaining_byte));
	if (err != 0) {
		goto unlock;
	}

	if (!data->fsp_config.p_transfer_rx) {
		data->rx_buffer_len = data->rx_buffer_cap - data->rx_buffer_offset - remaining_byte;
	}
	async_rx_ready(dev);
	async_release_rx_buffer(dev);
	async_release_rx_next_buffer(dev);
	async_rx_disabled(dev);

	/* Clear the RDRF bit so that the next reception can be raised correctly */
	cfg->regs->CFCLR_b.RDRFC = 1U;

unlock:
	irq_unlock(key);
	return err;
}

static void uart_ra_sci_b_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_ra_sci_b_data *data =
		CONTAINER_OF(dwork, struct uart_ra_sci_b_data, rx_timeout_work);
	const struct device *dev = data->dev;

	unsigned int key = irq_lock();

	if (!data->fsp_config.p_transfer_rx) {
		data->rx_buffer_len =
			data->rx_buffer_cap - data->rx_buffer_offset - data->sci.rx_dest_bytes;
	}
	async_rx_ready(dev);

	irq_unlock(key);
}

static void uart_ra_sci_b_callback_adapter(struct st_uart_callback_arg *fsp_args)
{
	const struct device *dev = fsp_args->p_context;
	struct uart_ra_sci_b_data *data = dev->data;

	switch (fsp_args->event) {
	case UART_EVENT_TX_COMPLETE: {
		data->tx_buffer_len = data->tx_buffer_cap;
		async_update_tx_buffer(dev);
		break;
	}
	case UART_EVENT_RX_COMPLETE: {
		data->rx_buffer_len =
			data->rx_buffer_cap - data->rx_buffer_offset - data->sci.rx_dest_bytes;
		async_rx_ready(dev);
		async_release_rx_buffer(dev);
		async_replace_rx_buffer(dev);
		break;
	}
	case UART_EVENT_ERR_PARITY:
		async_rx_error(dev, UART_ERROR_PARITY);
		break;
	case UART_EVENT_ERR_FRAMING:
		async_rx_error(dev, UART_ERROR_FRAMING);
		break;
	case UART_EVENT_ERR_OVERFLOW:
		async_rx_error(dev, UART_ERROR_OVERRUN);
		break;
	case UART_EVENT_BREAK_DETECT:
		async_rx_error(dev, UART_BREAK);
		break;
	case UART_EVENT_TX_DATA_EMPTY:
	case UART_EVENT_RX_CHAR:
		break;
	}
}

#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_ra_sci_b_driver_api = {
	.poll_in = uart_ra_sci_b_poll_in,
	.poll_out = uart_ra_sci_b_poll_out,
	.err_check = uart_ra_sci_b_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ra_sci_b_configure,
	.config_get = uart_ra_sci_b_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ra_sci_b_fifo_fill,
	.fifo_read = uart_ra_sci_b_fifo_read,
	.irq_tx_enable = uart_ra_sci_b_irq_tx_enable,
	.irq_tx_disable = uart_ra_sci_b_irq_tx_disable,
	.irq_tx_ready = uart_ra_sci_b_irq_tx_ready,
	.irq_rx_enable = uart_ra_sci_b_irq_rx_enable,
	.irq_rx_disable = uart_ra_sci_b_irq_rx_disable,
	.irq_tx_complete = uart_ra_sci_b_irq_tx_complete,
	.irq_rx_ready = uart_ra_sci_b_irq_rx_ready,
	.irq_err_enable = uart_ra_sci_b_irq_err_enable,
	.irq_err_disable = uart_ra_sci_b_irq_err_disable,
	.irq_is_pending = uart_ra_sci_b_irq_is_pending,
	.irq_update = uart_ra_sci_b_irq_update,
	.irq_callback_set = uart_ra_sci_b_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#if CONFIG_UART_ASYNC_API
	.callback_set = uart_ra_sci_b_async_callback_set,
	.tx = uart_ra_sci_b_async_tx,
	.tx_abort = uart_ra_sci_b_async_tx_abort,
	.rx_enable = uart_ra_sci_b_async_rx_enable,
	.rx_buf_rsp = uart_ra_sci_b_async_rx_buf_rsp,
	.rx_disable = uart_ra_sci_b_async_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

static int uart_ra_sci_b_init(const struct device *dev)
{
	const struct uart_ra_sci_b_config *config = dev->config;
	struct uart_ra_sci_b_data *data = dev->data;
	int ret;
	fsp_err_t fsp_err;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Setup fsp sci_uart setting */
	ret = uart_ra_sci_b_apply_config(&data->uart_config, &data->fsp_config,
					 &data->fsp_config_extend, &data->fsp_baud_setting);
	if (ret != 0) {
		return ret;
	}

	data->fsp_config_extend.p_baud_setting = &data->fsp_baud_setting;
	data->fsp_config.p_extend = &data->fsp_config_extend;

#if defined(CONFIG_UART_ASYNC_API)
	data->fsp_config.p_callback = uart_ra_sci_b_callback_adapter;
	data->fsp_config.p_context = dev;

	k_work_init_delayable(&data->tx_timeout_work, uart_ra_sci_b_async_tx_timeout);
	k_work_init_delayable(&data->rx_timeout_work, uart_ra_sci_b_async_rx_timeout);
#endif /* defined(CONFIG_UART_ASYNC_API) */

	fsp_err = R_SCI_B_UART_Open(&data->sci, &data->fsp_config);
	__ASSERT(fsp_err == 0, "sci_uart: initialization: open failed");

	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

static void uart_ra_sci_b_rxi_isr(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	uart_ra_sci_b_async_timer_start(&data->rx_timeout_work, data->rx_timeout);

	if (data->fsp_config.p_transfer_rx) {
		/*
		 * The RX DTC is set to TRANSFER_IRQ_EACH, triggering an interrupt for each received
		 * byte. However, the sci_b_uart_rxi_isr function currently only handles the
		 * TRANSFER_IRQ_END case, which assumes the transfer is complete. To address this,
		 * we need to add some code to simulate the TRANSFER_IRQ_END case by counting the
		 * received length.
		 */
		data->rx_buffer_len++;
		if (data->rx_buffer_offset + data->rx_buffer_len == data->rx_buffer_cap) {
			sci_b_uart_rxi_isr();
		} else {
			R_ICU->IELSR_b[data->fsp_config.rxi_irq].IR = 0U;
		}
	} else {
		sci_b_uart_rxi_isr();
	}
#else
	R_ICU->IELSR_b[data->fsp_config.rxi_irq].IR = 0U;
#endif
}

static void uart_ra_sci_b_txi_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	sci_b_uart_txi_isr();
#else
	R_ICU->IELSR_b[data->fsp_config.txi_irq].IR = 0U;
#endif
}

static void uart_ra_sci_b_tei_isr(const struct device *dev)
{
	struct uart_ra_sci_b_data *data = dev->data;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	k_work_cancel_delayable(&data->tx_timeout_work);
	sci_b_uart_tei_isr();
#else
	R_ICU->IELSR_b[data->fsp_config.tei_irq].IR = 0U;
#endif
}

static void uart_ra_sci_b_eri_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_ra_sci_b_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	sci_b_uart_eri_isr();
#else
	R_ICU->IELSR_b[data->fsp_config.eri_irq].IR = 0U;
#endif
}

#endif /* defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) */

#define _ELC_EVENT_SCI_RXI(channel) ELC_EVENT_SCI##channel##_RXI
#define _ELC_EVENT_SCI_TXI(channel) ELC_EVENT_SCI##channel##_TXI
#define _ELC_EVENT_SCI_TEI(channel) ELC_EVENT_SCI##channel##_TEI
#define _ELC_EVENT_SCI_ERI(channel) ELC_EVENT_SCI##channel##_ERI

#define ELC_EVENT_SCI_RXI(channel) _ELC_EVENT_SCI_RXI(channel)
#define ELC_EVENT_SCI_TXI(channel) _ELC_EVENT_SCI_TXI(channel)
#define ELC_EVENT_SCI_TEI(channel) _ELC_EVENT_SCI_TEI(channel)
#define ELC_EVENT_SCI_ERI(channel) _ELC_EVENT_SCI_ERI(channel)

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

#define UART_RA_SCI_B_IRQ_CONFIG_INIT(index)                                                       \
	do {                                                                                       \
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
			    uart_ra_sci_b_rxi_isr, DEVICE_DT_INST_GET(index), 0);                  \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),                  \
			    uart_ra_sci_b_txi_isr, DEVICE_DT_INST_GET(index), 0);                  \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority),                  \
			    uart_ra_sci_b_tei_isr, DEVICE_DT_INST_GET(index), 0);                  \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, priority),                  \
			    uart_ra_sci_b_eri_isr, DEVICE_DT_INST_GET(index), 0);                  \
	} while (0)

#else

#define UART_RA_SCI_B_IRQ_CONFIG_INIT(index)

#endif

#if defined(CONFIG_UART_ASYNC_API)

#define UART_RA_SCI_B_DTC_INIT(index)                                                              \
	do {                                                                                       \
		uart_ra_sci_b_data_##index.fsp_config.p_transfer_rx =                              \
			&uart_ra_sci_b_data_##index.rx_transfer;                                   \
		uart_ra_sci_b_data_##index.fsp_config.p_transfer_tx =                              \
			&uart_ra_sci_b_data_##index.tx_transfer;                                   \
	} while (0)

#define UART_RA_SCI_B_ASYNC_INIT(index)                                                            \
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
			.p_info = &uart_ra_sci_b_data_##index.rx_transfer_info,                    \
			.p_extend = &uart_ra_sci_b_data_##index.rx_transfer_cfg_extend,            \
	},                                                                                         \
	.rx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = &uart_ra_sci_b_data_##index.rx_transfer_ctrl,                    \
			.p_cfg = &uart_ra_sci_b_data_##index.rx_transfer_cfg,                      \
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
			.p_info = &uart_ra_sci_b_data_##index.tx_transfer_info,                    \
			.p_extend = &uart_ra_sci_b_data_##index.tx_transfer_cfg_extend,            \
	},                                                                                         \
	.tx_transfer = {                                                                           \
		.p_ctrl = &uart_ra_sci_b_data_##index.tx_transfer_ctrl,                            \
		.p_cfg = &uart_ra_sci_b_data_##index.tx_transfer_cfg,                              \
		.p_api = &g_transfer_on_dtc,                                                       \
	},

#else
#define UART_RA_SCI_B_ASYNC_INIT(index)
#define UART_RA_SCI_B_DTC_INIT(index)
#endif

#define UART_RA_SCI_B_INIT(index)                                                                  \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
                                                                                                   \
	static const struct uart_ra_sci_b_config uart_ra_sci_b_config_##index = {                  \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.regs = (R_SCI_B0_Type *)DT_REG_ADDR(DT_INST_PARENT(index)),                       \
	};                                                                                         \
                                                                                                   \
	static struct uart_ra_sci_b_data uart_ra_sci_b_data_##index = {                            \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = COND_CODE_1(DT_NODE_HAS_PROP(idx, hw_flow_control),   \
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
		.dev = DEVICE_DT_GET(DT_DRV_INST(index)),                                          \
		UART_RA_SCI_B_ASYNC_INIT(index)};                                                  \
                                                                                                   \
	static int uart_ra_sci_b_init_##index(const struct device *dev)                            \
	{                                                                                          \
		UART_RA_SCI_B_DTC_INIT(index);                                                     \
		UART_RA_SCI_B_IRQ_CONFIG_INIT(index);                                              \
		int err = uart_ra_sci_b_init(dev);                                                 \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, uart_ra_sci_b_init_##index, NULL,                             \
			      &uart_ra_sci_b_data_##index, &uart_ra_sci_b_config_##index,          \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,                           \
			      &uart_ra_sci_b_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RA_SCI_B_INIT)
