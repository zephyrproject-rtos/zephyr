/*
 * Copyright (c) 2021, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <drivers/pinctrl.h>

/* pico-sdk includes */
#include <hardware/uart.h>

#define DT_DRV_COMPAT raspberrypi_pico_uart

struct uart_rpi_config {
	uart_inst_t *const uart_dev;
	uint32_t baudrate;
	const struct pinctrl_dev_config *pcfg;
};

static int uart_rpi_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_rpi_config *config = dev->config;

	if (!uart_is_readable(config->uart_dev)) {
		return -1;
	}

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
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	uart_init(config->uart_dev, config->baudrate);

	return 0;
}

static const struct uart_driver_api uart_rpi_driver_api = {
	.poll_in = uart_rpi_poll_in,
	.poll_out = uart_rpi_poll_out,
};

#define RPI_UART_INIT(idx)						\
	PINCTRL_DT_INST_DEFINE(idx);					\
	static const struct uart_rpi_config uart_rpi_cfg_##idx = {	\
		.uart_dev = (uart_inst_t *)DT_INST_REG_ADDR(idx),	\
		.baudrate = DT_INST_PROP(idx, current_speed),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(idx, &uart_rpi_init,			\
				NULL,					\
				NULL,					\
				&uart_rpi_cfg_##idx, PRE_KERNEL_1,	\
				CONFIG_SERIAL_INIT_PRIORITY,		\
				&uart_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_UART_INIT)
