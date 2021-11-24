/*
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	infineon_xmc_uart

#include <xmc_gpio.h>
#include <xmc_uart.h>
#include <drivers/uart.h>

struct uart_xmc_data {
	XMC_UART_CH_CONFIG_t config;
};

#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct uart_xmc_data * const)(dev)->data)

static int uart_xmc_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	*(uint16_t *)c =
		XMC_UART_CH_GetReceivedData((XMC_USIC_CH_t *)config->base);

	return 0;
}

static void uart_xmc_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	XMC_UART_CH_Transmit((XMC_USIC_CH_t *)config->base, (uint16_t)c);
}

static int uart_xmc_init(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	struct uart_xmc_data *data = DEV_DATA(dev);
	XMC_USIC_CH_t *uart = (XMC_USIC_CH_t *)config->base;

	data->config.data_bits = 8U;
	data->config.stop_bits = 1U;

	/* configure PIN 0.0 and 0.1 as UART */
	XMC_UART_CH_Init(uart, &(data->config));
	XMC_GPIO_SetMode(P0_0, XMC_GPIO_MODE_INPUT_TRISTATE);
	XMC_UART_CH_SetInputSource(uart, XMC_UART_CH_INPUT_RXD,
				   USIC1_C1_DX0_P0_0);
	XMC_UART_CH_Start(uart);

	XMC_GPIO_SetMode(P0_1,
			 XMC_GPIO_MODE_OUTPUT_PUSH_PULL | P0_1_AF_U1C1_DOUT0);

	return 0;
}

static const struct uart_driver_api uart_xmc_driver_api = {
	.poll_in = uart_xmc_poll_in,
	.poll_out = uart_xmc_poll_out,
};

#define XMC_INIT(index)						\
static struct uart_xmc_data xmc_data_##index = {		\
	.config.baudrate = DT_INST_PROP(index, current_speed)		\
};									\
									\
static const struct uart_device_config xmc_config_##index = {	\
	.base = (void *)DT_INST_REG_ADDR(index),			\
};									\
									\
	DEVICE_DT_INST_DEFINE(index, &uart_xmc_init,		\
			    NULL,					\
			    &xmc_data_##index,			\
			    &xmc_config_##index, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_xmc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC_INIT)
