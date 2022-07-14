/*
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	infineon_xmc4xxx_uart

#include <xmc_uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>

struct uart_xmc4xx_config {
	XMC_USIC_CH_t *uart;
	const struct pinctrl_dev_config *pcfg;
	uint8_t input_src;
};

struct uart_xmc4xxx_data {
	XMC_UART_CH_CONFIG_t config;
};

static int uart_xmc4xxx_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_xmc4xx_config *config = dev->config;

	if (!XMC_USIC_CH_GetReceiveBufferStatus(config->uart)) {
		return -1;
	}

	*c = (unsigned char)XMC_UART_CH_GetReceivedData(config->uart);

	return 0;
}

static void uart_xmc4xxx_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_xmc4xx_config *config = dev->config;

	XMC_UART_CH_Transmit(config->uart, c);
}

static int uart_xmc4xxx_init(const struct device *dev)
{
	int ret;
	const struct uart_xmc4xx_config *config = dev->config;
	struct uart_xmc4xxx_data *data = dev->data;

	data->config.data_bits = 8U;
	data->config.stop_bits = 1U;

	XMC_UART_CH_Init(config->uart, &(data->config));

	/* Connect UART RX to logical 1. It is connected to proper pin after pinctrl is applied */
	XMC_UART_CH_SetInputSource(config->uart, XMC_UART_CH_INPUT_RXD, 0x7);

	/* Start the UART before pinctrl, because the USIC is driving the TX line */
	/* low in off state */
	XMC_UART_CH_Start(config->uart);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	/* Connect UART RX to the target pin */
	XMC_UART_CH_SetInputSource(config->uart, XMC_UART_CH_INPUT_RXD,
				   config->input_src);

	return 0;
}

static const struct uart_driver_api uart_xmc4xxx_driver_api = {
	.poll_in = uart_xmc4xxx_poll_in,
	.poll_out = uart_xmc4xxx_poll_out,
};

#define XMC4XXX_INIT(index)						\
PINCTRL_DT_INST_DEFINE(index);						\
static struct uart_xmc4xxx_data xmc4xxx_data_##index = {		\
	.config.baudrate = DT_INST_PROP(index, current_speed)		\
};									\
									\
static const struct uart_xmc4xx_config xmc4xxx_config_##index = {	\
	.uart = (XMC_USIC_CH_t *)DT_INST_REG_ADDR(index),		\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.input_src = DT_INST_ENUM_IDX(index, input_src),		\
};									\
									\
	DEVICE_DT_INST_DEFINE(index, &uart_xmc4xxx_init,		\
			    NULL,					\
			    &xmc4xxx_data_##index,			\
			    &xmc4xxx_config_##index, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_INIT)
