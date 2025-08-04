/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_uart_sci_b

#include <r_sci_b_uart.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_rx_uart_sci_b, CONFIG_UART_LOG_LEVEL);

struct uart_renesas_rx_sci_b_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	struct clock_control_rx_subsys_cfg clock_subsys;
	/* clang-format off */
	R_SCI_B0_Type * const regs;
	/* clang-format on */
};

struct uart_renesas_rx_sci_b_data {
	const struct device *dev;
	struct st_sci_b_uart_instance_ctrl sci;
	struct uart_config uart_config;
	struct st_uart_cfg fsp_config;
	struct st_sci_b_uart_extended_cfg fsp_config_extend;
	struct st_sci_b_baud_setting_t fsp_baud_setting;
};

static int uart_renesas_rx_sci_b_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_renesas_rx_sci_b_config *cfg = dev->config;

	if (cfg->regs->SSR_b.RDRF == 0U) {
		/* There are no characters available to read. */
		return -1;
	}

	/* got a character */
	*c = (unsigned char)cfg->regs->RDR;

	return 0;
}

static void uart_renesas_rx_sci_b_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_renesas_rx_sci_b_config *cfg = dev->config;

	while (cfg->regs->SSR_b.TEND == 0U) {
	}

	cfg->regs->TDR_BY = c;

	while (cfg->regs->SSR_b.TEND == 0U) {
	}
}

static int uart_rx_sci_b_apply_config(const struct uart_config *config,
				      struct st_uart_cfg *fsp_config,
				      struct st_sci_b_uart_extended_cfg *fsp_config_extend,
				      struct st_sci_b_baud_setting_t *fsp_baud_setting)
{
	fsp_err_t fsp_err;

	fsp_err = R_SCI_B_UART_BaudCalculate(config->baudrate, false, 5000, fsp_baud_setting);
	if (fsp_err != FSP_SUCCESS) {
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

	fsp_config_extend->clock = SCI_B_UART_CLOCK_INT;
	fsp_config_extend->rx_edge_start = SCI_B_UART_START_BIT_FALLING_EDGE;
	fsp_config_extend->noise_cancel = SCI_B_UART_NOISE_CANCELLATION_DISABLE;
	fsp_config_extend->flow_control_pin = UINT16_MAX;

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

static DEVICE_API(uart, uart_renesas_rx_sci_b_driver_api) = {
	.poll_in = uart_renesas_rx_sci_b_poll_in,
	.poll_out = uart_renesas_rx_sci_b_poll_out,
};

static int uart_renesas_rx_sci_b_init(const struct device *dev)
{
	const struct uart_renesas_rx_sci_b_config *config = dev->config;
	struct uart_renesas_rx_sci_b_data *data = dev->data;
	int ret;
	fsp_err_t fsp_err;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	/* Setup fsp sci_uart setting */
	ret = uart_rx_sci_b_apply_config(&data->uart_config, &data->fsp_config,
					 &data->fsp_config_extend, &data->fsp_baud_setting);
	if (ret != 0) {
		return ret;
	}

	data->fsp_config_extend.p_baud_setting = &data->fsp_baud_setting;
	data->fsp_config.p_extend = &data->fsp_config_extend;

	fsp_err = R_SCI_B_UART_Open(&data->sci, &data->fsp_config);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to open SCI UART: %d", fsp_err);
		return -EINVAL;
	}

	return 0;
}

#define UART_RX_SCI_B_INIT(index)                                                                  \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
	static const struct uart_renesas_rx_sci_b_config uart_renesas_rx_sci_b_config_##index = {  \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(index))),                 \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL(DT_INST_PARENT(index), mstp),               \
				.stop_bit = DT_CLOCKS_CELL(DT_INST_PARENT(index), stop_bit),       \
			},                                                                         \
		.regs = (R_SCI_B0_Type *)DT_REG_ADDR(DT_INST_PARENT(index)),                       \
	};                                                                                         \
	static struct uart_renesas_rx_sci_b_data uart_renesas_rx_sci_b_data_##index = {            \
		.dev = DEVICE_DT_GET(DT_DRV_INST(index)),                                          \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_PROP(DT_INST_PARENT(index), channel),                \
			},                                                                         \
		.fsp_config_extend = {},                                                           \
		.fsp_baud_setting = {},                                                            \
	};                                                                                         \
	static int uart_renesas_rx_sci_b_init_##index(const struct device *dev)                    \
	{                                                                                          \
		return uart_renesas_rx_sci_b_init(dev);                                            \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, uart_renesas_rx_sci_b_init_##index, NULL,                     \
			      &uart_renesas_rx_sci_b_data_##index,                                 \
			      &uart_renesas_rx_sci_b_config_##index, PRE_KERNEL_1,                 \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_renesas_rx_sci_b_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RX_SCI_B_INIT)
