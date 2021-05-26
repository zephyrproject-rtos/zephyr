/*
 * Copyright (c) 2021, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>

struct uart_rpi_config {
	uart_inst_t *const uart_dev;
	uint32_t baudrate;
	uint32_t tx_pin;
	uint32_t rx_pin;
};

struct uart_rpi_data {

};

static int uart_rpi_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_rpi_config *config = dev->config;

	if (!uart_is_readable(config->uart_dev))
		return -1;

	*c = (unsigned char)uart_get_hw(config->uart_dev)->dr;
	return 0;
}

static void uart_rpi_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_rpi_config *config = dev->config;

	uart_putc_raw(config->uart_dev, c);
}

static int uart_rpi_init(const struct device *dev)
{
	const struct uart_rpi_config *config = dev->config;

	gpio_init(config->tx_pin);
	gpio_init(config->rx_pin);
	gpio_set_function(config->tx_pin, GPIO_FUNC_UART);
	gpio_set_function(config->rx_pin, GPIO_FUNC_UART);

	return !uart_init(config->uart_dev, config->baudrate);
}

static const struct uart_driver_api uart_rpi_driver_api = {
	.poll_in = uart_rpi_poll_in,
	.poll_out = uart_rpi_poll_out,
};

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT raspberrypi_rp2_uart

#define RPI_UART_INIT(idx)						\
	static const struct uart_rpi_config uart_rpi_cfg_##idx = {	\
		.uart_dev = (uart_inst_t *)DT_INST_REG_ADDR(idx),	\
		.baudrate = DT_INST_PROP(idx, current_speed),		\
		.tx_pin = DT_INST_PROP(idx, tx_pin),			\
		.rx_pin = DT_INST_PROP(idx, rx_pin),			\
	};								\
									\
	static struct uart_rpi_data uart_rpi_data_##idx;		\
									\
	DEVICE_DT_INST_DEFINE(idx, &uart_rpi_init,			\
				device_pm_control_nop,			\
				&uart_rpi_data_##idx, 			\
				&uart_rpi_cfg_##idx, PRE_KERNEL_1,	\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
				&uart_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_UART_INIT)
