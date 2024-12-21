/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_scif_uart

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include "r_scif_uart.h"

LOG_MODULE_REGISTER(rz_scif_uart);

struct uart_rz_scif_config {
	const struct pinctrl_dev_config *pin_config;
	const uart_api_t *fsp_api;
};

struct uart_rz_scif_data {
	struct uart_config uart_config;
	uart_cfg_t *fsp_cfg;
	scif_uart_instance_ctrl_t *fsp_ctrl;
};

static int uart_rz_scif_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_rz_scif_data *data = dev->data;
	R_SCIFA0_Type *reg = data->fsp_ctrl->p_reg;

	if (reg->FDR_b.R == 0U) {
		/* There are no characters available to read. */
		return -1;
	}
	*c = reg->FRDR;

	return 0;
}

static void uart_rz_scif_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_rz_scif_data *data = dev->data;
	R_SCIFA0_Type *reg = data->fsp_ctrl->p_reg;

	while (!reg->FSR_b.TDFE) {
	}

	reg->FTDR = c;

	while (!reg->FSR_b.TEND) {
	}
}

static int uart_rz_scif_err_check(const struct device *dev)
{
	struct uart_rz_scif_data *data = dev->data;
	R_SCIFA0_Type *reg = data->fsp_ctrl->p_reg;

	const uint32_t fsr = reg->FSR;
	const uint32_t lsr = reg->LSR;
	int errors = 0;

	if ((lsr & R_SCIFA0_LSR_ORER_Msk) != 0) {
		errors |= UART_ERROR_OVERRUN;
	}
	if ((fsr & R_SCIFA0_FSR_PER_Msk) != 0) {
		errors |= UART_ERROR_PARITY;
	}
	if ((fsr & R_SCIFA0_FSR_FER_Pos) != 0) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int uart_rz_scif_apply_config(const struct device *dev)
{
	struct uart_rz_scif_data *data = dev->data;

	struct uart_config *uart_config = &data->uart_config;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;

	scif_baud_setting_t baud_setting;
	scif_uart_extended_cfg_t config_extend;
	const scif_uart_extended_cfg_t *fsp_config_extend = fsp_cfg->p_extend;

	fsp_err_t fsp_err;

	fsp_err = R_SCIF_UART_BaudCalculate(data->fsp_ctrl, uart_config->baudrate, false, 5000,
					    &baud_setting);
	if (fsp_err) {
		return -EIO;
	}

	memcpy(fsp_config_extend->p_baud_setting, &baud_setting, sizeof(scif_baud_setting_t));

	switch (uart_config->data_bits) {
	case UART_CFG_DATA_BITS_7:
		fsp_cfg->data_bits = UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		fsp_cfg->data_bits = UART_DATA_BITS_8;
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

	memcpy(&config_extend, fsp_config_extend->p_baud_setting, sizeof(scif_baud_setting_t));

	switch (uart_config->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		config_extend.flow_control = SCIF_UART_FLOW_CONTROL_NONE;
		config_extend.uart_mode = SCIF_UART_MODE_RS232;
		config_extend.rs485_setting.enable = SCI_UART_RS485_DISABLE;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		config_extend.flow_control = SCIF_UART_FLOW_CONTROL_AUTO;
		config_extend.uart_mode = SCIF_UART_MODE_RS232;
		config_extend.rs485_setting.enable = SCI_UART_RS485_DISABLE;
		break;
	default:
		return -ENOTSUP;
	}

	memcpy(fsp_config_extend->p_baud_setting, &config_extend, sizeof(scif_baud_setting_t));

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_rz_scif_configure(const struct device *dev, const struct uart_config *cfg)
{
	int err;
	fsp_err_t fsp_err;
	const struct uart_rz_scif_config *config = dev->config;
	struct uart_rz_scif_data *data = dev->data;

	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	err = uart_rz_scif_apply_config(dev);

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

	R_SCIFA0_Type *reg = data->fsp_ctrl->p_reg;
	/* Temporarily disable the DRI interrupt caused by receive data ready */
	/* TODO: support interrupt-driven api */
	reg->SCR_b.RIE = 0;

	return err;
}

static int uart_rz_scif_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rz_scif_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(struct uart_config));
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static DEVICE_API(uart, uart_rz_scif_driver_api) = {
	.poll_in = uart_rz_scif_poll_in,
	.poll_out = uart_rz_scif_poll_out,
	.err_check = uart_rz_scif_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rz_scif_configure,
	.config_get = uart_rz_scif_config_get,
#endif
};

static int uart_rz_scif_init(const struct device *dev)
{
	const struct uart_rz_scif_config *config = dev->config;
	struct uart_rz_scif_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* uart_rz_scif_apply_config must be called first before open api */
	ret = uart_rz_scif_apply_config(dev);
	if (ret < 0) {
		return ret;
	}

	config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	R_SCIFA0_Type *reg = data->fsp_ctrl->p_reg;
	/* Temporarily disable the DRI interrupt caused by receive data ready */
	/* TODO: support interrupt-driven api */
	reg->SCR_b.RIE = 0;

	return 0;
}

#define UART_RZG_INIT(n)                                                                           \
	static scif_uart_instance_ctrl_t g_uart##n##_ctrl;                                         \
	static scif_baud_setting_t g_uart##n##_baud_setting;                                       \
	static scif_uart_extended_cfg_t g_uart##n##_cfg_extend = {                                 \
		.bri_ipl = DT_INST_IRQ_BY_NAME(n, bri, priority),                                  \
		.bri_irq = DT_INST_IRQ_BY_NAME(n, bri, irq),                                       \
		.clock = SCIF_UART_CLOCK_INT,                                                      \
		.noise_cancel = SCIF_UART_NOISE_CANCELLATION_ENABLE,                               \
		.p_baud_setting = &g_uart##n##_baud_setting,                                       \
		.rx_fifo_trigger = SCIF_UART_RECEIVE_TRIGGER_MAX,                                  \
		.rts_fifo_trigger = SCIF_UART_RTS_TRIGGER_14,                                      \
		.uart_mode = SCIF_UART_MODE_RS232,                                                 \
		.flow_control = SCIF_UART_FLOW_CONTROL_NONE,                                       \
		.rs485_setting =                                                                   \
			{                                                                          \
				.enable = (sci_uart_rs485_enable_t)NULL,                           \
				.polarity = SCI_UART_RS485_DE_POLARITY_HIGH,                       \
				.de_control_pin =                                                  \
					(bsp_io_port_pin_t)SCIF_UART_INVALID_16BIT_PARAM,          \
			},                                                                         \
	};                                                                                         \
	static uart_cfg_t g_uart##n##_cfg = {                                                      \
		.channel = DT_INST_PROP(n, channel),                                               \
		.p_callback = NULL,                                                                \
		.p_context = NULL,                                                                 \
		.p_extend = &g_uart##n##_cfg_extend,                                               \
		.p_transfer_tx = NULL,                                                             \
		.p_transfer_rx = NULL,                                                             \
		.rxi_ipl = DT_INST_IRQ_BY_NAME(n, rxi, priority),                                  \
		.txi_ipl = DT_INST_IRQ_BY_NAME(n, txi, priority),                                  \
		.tei_ipl = DT_INST_IRQ_BY_NAME(n, tei, priority),                                  \
		.eri_ipl = DT_INST_IRQ_BY_NAME(n, eri, priority),                                  \
		.rxi_irq = DT_INST_IRQ_BY_NAME(n, rxi, irq),                                       \
		.txi_irq = DT_INST_IRQ_BY_NAME(n, txi, irq),                                       \
		.tei_irq = DT_INST_IRQ_BY_NAME(n, tei, irq),                                       \
		.eri_irq = DT_INST_IRQ_BY_NAME(n, eri, irq),                                       \
	};                                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct uart_rz_scif_config uart_rz_scif_config_##n = {                        \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(n), .fsp_api = &g_uart_on_scif};      \
	static struct uart_rz_scif_data uart_rz_scif_data_##n = {                                  \
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
		.fsp_cfg = &g_uart##n##_cfg,                                                       \
		.fsp_ctrl = &g_uart##n##_ctrl,                                                     \
	};                                                                                         \
	static int uart_rz_scif_init_##n(const struct device *dev)                                 \
	{                                                                                          \
		return uart_rz_scif_init(dev);                                                     \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, &uart_rz_scif_init_##n, NULL, &uart_rz_scif_data_##n,             \
			      &uart_rz_scif_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_rz_scif_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RZG_INIT)
