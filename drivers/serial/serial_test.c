/*
 * Copyright (c) 2021, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real serial driver. It is used to instantiate struct
 * devices for the "vnd,serial" devicetree compatible used in test code.
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/uart.h>

#define DT_DRV_COMPAT vnd_serial

static int serial_vnd_poll_in(const struct device *dev, unsigned char *c)
{
	return -ENOTSUP;
}

static void serial_vnd_poll_out(const struct device *dev, unsigned char c)
{
}

static int serial_vnd_err_check(const struct device *dev)
{
	return -ENOTSUP;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int serial_vnd_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	return -ENOTSUP;
}

static int serial_vnd_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	return -ENOTSUP;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */


static const struct uart_driver_api serial_vnd_api = {
	.poll_in = serial_vnd_poll_in,
	.poll_out = serial_vnd_poll_out,
	.err_check = serial_vnd_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = serial_vnd_configure,
	.config_get = serial_vnd_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
};

static int serial_vnd_init(const struct device *dev)
{
	return 0;
}

#define VND_SERIAL_INIT(n)						\
	DEVICE_DT_INST_DEFINE(n, &serial_vnd_init, NULL,		\
			      NULL, NULL, POST_KERNEL,			\
			      CONFIG_SERIAL_INIT_PRIORITY,		\
			      &serial_vnd_api);

DT_INST_FOREACH_STATUS_OKAY(VND_SERIAL_INIT)
