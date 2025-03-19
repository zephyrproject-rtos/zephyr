/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_sci_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include "r_sci_uart.h"

LOG_MODULE_REGISTER(rz_sci_uart);

struct uart_rz_sci_config {
	const struct pinctrl_dev_config *pin_config;
	const uart_api_t *fsp_api;
};

struct uart_rz_sci_int {
	bool rx_fifo_busy;
	bool irq_rx_enable;
	bool irq_tx_enable;
	uint8_t rx_byte;
	uint8_t tx_byte;
	uart_event_t event;
};

struct uart_rz_sci_data {
	struct uart_config uart_config;
	uart_cfg_t *fsp_cfg;
	struct uart_rz_sci_int int_data;
	sci_uart_instance_ctrl_t *fsp_ctrl;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *callback_data;
#endif
};

#define SCI_UART_ERROR_RATE_x1000 (5000)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void sci_uart_rxi_isr(void);
void sci_uart_txi_isr(void);
void sci_uart_tei_isr(void);
void sci_uart_eri_isr(void);
#endif

static int uart_rz_sci_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_rz_sci_data *data = dev->data;
	R_SCI0_Type *reg = data->fsp_ctrl->p_reg;

	if (reg->FRSR_b.R == 0U) {
		/* There are no characters available to read. */
		return -1;
	}

	*c = (unsigned char)reg->RDR_b.RDAT;
	return 0;
}

static void uart_rz_sci_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_rz_sci_data *data = dev->data;
	R_SCI0_Type *reg = data->fsp_ctrl->p_reg;
	uint8_t key;

	key = irq_lock();
	while (!reg->CSR_b.TDRE) {
		;
	}

	reg->TDR_b.TDAT = c;
	reg->CFCLR_b.TDREC = 1;

	while (!reg->CSR_b.TEND) {
		;
	}
	irq_unlock(key);
}

static int uart_rz_sci_err_check(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;
	uart_event_t event = data->int_data.event;
	int err = 0;

	if (event & UART_EVENT_ERR_OVERFLOW) {
		err |= UART_ERROR_OVERRUN;
	}
	if (event & UART_EVENT_ERR_FRAMING) {
		err |= UART_ERROR_FRAMING;
	}
	if (event & UART_EVENT_ERR_PARITY) {
		err |= UART_ERROR_PARITY;
	}

	return err;
}

static int uart_rz_sci_apply_config(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	struct uart_config *uart_config = &data->uart_config;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;

	sci_baud_setting_t baud_setting;
	sci_uart_extended_cfg_t *fsp_config_extend = (sci_uart_extended_cfg_t *)fsp_cfg->p_extend;

	fsp_err_t fsp_err;
	sci_uart_baud_calculation_t baud_target;

	baud_target.baudrate = uart_config->baudrate;
	baud_target.bitrate_modulation = false;
	baud_target.baud_rate_error_x_1000 = SCI_UART_ERROR_RATE_x1000;

	fsp_err = R_SCI_UART_BaudCalculate(&baud_target, fsp_config_extend->clock_source,
					   &baud_setting);
	if (fsp_err) {
		return -EIO;
	}

	memcpy(fsp_config_extend->p_baud_setting, &baud_setting, sizeof(sci_baud_setting_t));

	switch (uart_config->data_bits) {
	case UART_CFG_DATA_BITS_7:
		fsp_cfg->data_bits = UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		fsp_cfg->data_bits = UART_DATA_BITS_8;
		break;
	case UART_CFG_DATA_BITS_9:
		fsp_cfg->data_bits = UART_DATA_BITS_9;
		break;
	default:
		return -ENOTSUP;
	}

	switch (uart_config->parity) {
	case UART_CFG_PARITY_NONE:
		fsp_cfg->parity = UART_PARITY_OFF;
		break;
	case UART_CFG_PARITY_ODD:
		fsp_cfg->parity = UART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		fsp_cfg->parity = UART_PARITY_EVEN;
		break;
	default:
		return -ENOTSUP;
	}

	switch (uart_config->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		fsp_cfg->stop_bits = UART_STOP_BITS_1;
		break;
	case UART_CFG_STOP_BITS_2:
		fsp_cfg->stop_bits = UART_STOP_BITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	switch (uart_config->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		fsp_config_extend->flow_control = 0;
		fsp_config_extend->rs485_setting.enable = SCI_UART_RS485_DISABLE;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		fsp_config_extend->flow_control = SCI_UART_FLOW_CONTROL_HARDWARE_CTSRTS;
		fsp_config_extend->rs485_setting.enable = SCI_UART_RS485_DISABLE;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_rz_sci_configure(const struct device *dev, const struct uart_config *cfg)
{
	int err;
	fsp_err_t fsp_err;
	const struct uart_rz_sci_config *config = dev->config;
	struct uart_rz_sci_data *data = dev->data;

	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	err = uart_rz_sci_apply_config(dev);
	if (err) {
		return err;
	}

	fsp_err = config->fsp_api->close(data->fsp_ctrl);
	if (fsp_err) {
		return -EIO;
	}

	fsp_err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (fsp_err) {
		return -EIO;
	}

	return err;
}

static int uart_rz_sci_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rz_sci_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(struct uart_config));
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_rz_sci_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct uart_rz_sci_data *data = dev->data;
	sci_uart_instance_ctrl_t *fsp_ctrl = data->fsp_ctrl;

	fsp_ctrl->tx_src_bytes = size;
	fsp_ctrl->p_tx_src = tx_data;

	sci_uart_txi_isr();

	return (size - fsp_ctrl->tx_src_bytes);
}

static int uart_rz_sci_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_rz_sci_data *data = dev->data;
	sci_uart_instance_ctrl_t *fsp_ctrl = data->fsp_ctrl;

	fsp_ctrl->rx_dest_bytes = size;
	fsp_ctrl->p_rx_dest = rx_data;

	sci_uart_rxi_isr();

	data->int_data.rx_fifo_busy = false;

	return (size - fsp_ctrl->rx_dest_bytes);
}

static void uart_rz_sci_irq_rx_enable(const struct device *dev)
{
	const struct uart_rz_sci_config *config = dev->config;
	struct uart_rz_sci_data *data = dev->data;

	data->int_data.irq_rx_enable = true;

	/* Prepare 1-byte buffer to receive, it will be overwritten by fifo read */
	config->fsp_api->read(data->fsp_ctrl, &(data->int_data.rx_byte), 1);
}

static void uart_rz_sci_irq_rx_disable(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	data->int_data.irq_rx_enable = false;
}

static void uart_rz_sci_irq_tx_enable(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;
	const struct uart_rz_sci_config *config = dev->config;

	data->int_data.irq_tx_enable = true;

	/* Trigger TX with a NULL frame */
	/* It is expected not to be sent, and will be overwritten by the fifo fill */
	data->int_data.tx_byte = '\0';
	config->fsp_api->write(data->fsp_ctrl, &data->int_data.tx_byte, 1);
}

static void uart_rz_sci_irq_tx_disable(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	data->int_data.irq_tx_enable = false;
}

static int uart_rz_sci_irq_tx_ready(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	return data->int_data.irq_tx_enable;
}

static int uart_rz_sci_irq_rx_ready(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	return data->int_data.rx_fifo_busy && data->int_data.irq_rx_enable;
}

static int uart_rz_sci_irq_is_pending(const struct device *dev)
{
	return (uart_rz_sci_irq_tx_ready(dev)) || (uart_rz_sci_irq_rx_ready(dev));
}

static void uart_rz_sci_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_rz_sci_data *data = dev->data;

	data->callback = cb;
	data->callback_data = cb_data;
}

static int uart_rz_sci_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_rz_sci_driver_api) = {
	.poll_in = uart_rz_sci_poll_in,
	.poll_out = uart_rz_sci_poll_out,
	.err_check = uart_rz_sci_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rz_sci_configure,
	.config_get = uart_rz_sci_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rz_sci_fifo_fill,
	.fifo_read = uart_rz_sci_fifo_read,
	.irq_rx_enable = uart_rz_sci_irq_rx_enable,
	.irq_rx_disable = uart_rz_sci_irq_rx_disable,
	.irq_tx_enable = uart_rz_sci_irq_tx_enable,
	.irq_tx_disable = uart_rz_sci_irq_tx_disable,
	.irq_tx_ready = uart_rz_sci_irq_tx_ready,
	.irq_rx_ready = uart_rz_sci_irq_rx_ready,
	.irq_is_pending = uart_rz_sci_irq_is_pending,
	.irq_callback_set = uart_rz_sci_irq_callback_set,
	.irq_update = uart_rz_sci_irq_update,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_rz_init(const struct device *dev)
{
	const struct uart_rz_sci_config *config = dev->config;
	struct uart_rz_sci_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* uart_rz_sci_apply_config must be called first before open api */
	ret = uart_rz_sci_apply_config(dev);
	if (ret < 0) {
		return ret;
	}

	config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	return 0;
}

static void uart_rz_sci_rxi_isr(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	data->int_data.rx_fifo_busy = true;
	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}

static void uart_rz_sci_txi_isr(const struct device *dev)
{
	struct uart_rz_sci_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}

static void uart_rz_sci_tei_isr(const struct device *dev)
{
	sci_uart_tei_isr();
}

static void uart_rz_sci_eri_isr(const struct device *dev)
{
	sci_uart_eri_isr();
}

static void uart_rz_sci_event_handler(uart_callback_args_t *p_args)
{
	const struct device *dev = (const struct device *)p_args->p_context;
	struct uart_rz_sci_data *data = dev->data;

	data->int_data.event = p_args->event;
	switch (p_args->event) {
	case UART_EVENT_RX_CHAR:
		data->int_data.rx_byte = p_args->data;
		break;
	case UART_EVENT_RX_COMPLETE:
		break;
	case UART_EVENT_TX_DATA_EMPTY:
		break;
	case UART_EVENT_TX_COMPLETE:
		break;
	default:
		break;
	}
}

#define SCI_NODE(idx) DT_INST_PARENT(idx)
#define UART_RZ_IRQ_CONNECT(n, irq_name, isr)                                                      \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(SCI_NODE(n), irq_name, irq),                            \
			    DT_IRQ_BY_NAME(SCI_NODE(n), irq_name, priority), isr,                  \
			    DEVICE_DT_INST_GET(n), DT_IRQ_BY_NAME(SCI_NODE(n), irq_name, flags));  \
		irq_enable(DT_IRQ_BY_NAME(SCI_NODE(n), irq_name, irq));                            \
	} while (0)

#define UART_RZ_CONFIG_FUNC(n)                                                                     \
	UART_RZ_IRQ_CONNECT(n, eri, uart_rz_sci_eri_isr);                                          \
	UART_RZ_IRQ_CONNECT(n, rxi, uart_rz_sci_rxi_isr);                                          \
	UART_RZ_IRQ_CONNECT(n, txi, uart_rz_sci_txi_isr);                                          \
	UART_RZ_IRQ_CONNECT(n, tei, uart_rz_sci_tei_isr);

#define UART_RZ_INIT(n)                                                                            \
	static sci_baud_setting_t g_uart##n##_baud_setting;                                        \
	static sci_uart_extended_cfg_t g_uart##n##_cfg_extend = {                                  \
		.clock = SCI_UART_CLOCK_INT,                                                       \
		.rx_edge_start = SCI_UART_START_BIT_FALLING_EDGE,                                  \
		.noise_cancel = SCI_UART_NOISE_CANCELLATION_ENABLE,                                \
		.rx_fifo_trigger = SCI_UART_RX_FIFO_TRIGGER_MAX,                                   \
		.p_baud_setting = &g_uart##n##_baud_setting,                                       \
		.clock_source = SCI_UART_CLOCK_SOURCE_SCI0ASYNCCLK,                                \
		.flow_control = SCI_UART_FLOW_CONTROL_RTS,                                         \
		.flow_control_pin = 0xFF,                                                          \
		.rs485_setting =                                                                   \
			{                                                                          \
				.enable = SCI_UART_RS485_DISABLE,                                  \
				.polarity = SCI_UART_RS485_DE_POLARITY_HIGH,                       \
				.assertion_time = 1,                                               \
				.negation_time = 1,                                                \
			},                                                                         \
	};                                                                                         \
	static uart_cfg_t g_uart##n##_cfg = {                                                      \
		.channel = DT_PROP(SCI_NODE(n), channel),                                          \
		.p_extend = &g_uart##n##_cfg_extend,                                               \
		.p_transfer_tx = NULL,                                                             \
		.p_transfer_rx = NULL,                                                             \
		.rxi_ipl = DT_IRQ_BY_NAME(SCI_NODE(n), rxi, priority),                             \
		.rxi_irq = DT_IRQ_BY_NAME(SCI_NODE(n), rxi, irq),                                  \
		.txi_ipl = DT_IRQ_BY_NAME(SCI_NODE(n), txi, priority),                             \
		.txi_irq = DT_IRQ_BY_NAME(SCI_NODE(n), txi, irq),                                  \
		.tei_ipl = DT_IRQ_BY_NAME(SCI_NODE(n), tei, priority),                             \
		.tei_irq = DT_IRQ_BY_NAME(SCI_NODE(n), tei, irq),                                  \
		.eri_ipl = DT_IRQ_BY_NAME(SCI_NODE(n), eri, priority),                             \
		.eri_irq = DT_IRQ_BY_NAME(SCI_NODE(n), eri, irq),                                  \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (                                         \
			 .p_callback = uart_rz_sci_event_handler,                                  \
			 .p_context = (void *)DEVICE_DT_INST_GET(n),)) };                          \
	PINCTRL_DT_DEFINE(SCI_NODE(n));                                                            \
	static struct uart_rz_sci_config uart_rz_config_##n = {                                    \
		.pin_config = PINCTRL_DT_DEV_CONFIG_GET(SCI_NODE(n)),                              \
		.fsp_api = &g_uart_on_sci,                                                         \
	};                                                                                         \
	static sci_uart_instance_ctrl_t g_uart##n##_ctrl;                                          \
	static struct uart_rz_sci_data uart_rz_sci_data_##n = {                                    \
		.fsp_ctrl = &g_uart##n##_ctrl,                                                     \
		.fsp_cfg = &g_uart##n##_cfg,                                                       \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP_OR(n, current_speed, 115200),             \
				.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),    \
				.stop_bits =                                                       \
					DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),   \
				.data_bits =                                                       \
					DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),   \
				.flow_ctrl = DT_INST_PROP_OR(n, hw_flow_control,                   \
							     UART_CFG_FLOW_CTRL_NONE),             \
			},                                                                         \
	};                                                                                         \
	static int uart_rz_init_##n(const struct device *dev)                                      \
	{                                                                                          \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                  \
			(UART_RZ_CONFIG_FUNC(n);))                        \
		return uart_rz_init(dev);                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, &uart_rz_init_##n, NULL, &uart_rz_sci_data_##n,                   \
			      &uart_rz_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,      \
			      &uart_rz_sci_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RZ_INIT)
