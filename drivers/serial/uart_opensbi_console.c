/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT opensbi_console_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <opensbi.h>

struct opensbi_console_config {
	uint32_t instance;
};

struct opensbi_console_data {
	uint32_t dummy;
};

static int opensbi_console_init(const struct device *dev)
{
	return 0;
}

/*
 * it should be avoided to read single characters only
 */
static int opensbi_console_poll_in(const struct device *dev, unsigned char *c)
{
	return -1;
}

/*
 * it should be avoided to write single characters only
 */
static void opensbi_console_poll_out(const struct device *dev, unsigned char c)
{
	/* workaround: openSBI does handle '\r' a little strange */
	if (c == '\r') {
		return;
	}
	sbi_console_put_char(c);
}

static int opensbi_console_err_check(const struct device *dev)
{
	return 0;
}

static const struct uart_driver_api opensbi_console_driver_api = {

	.poll_in = opensbi_console_poll_in,
	.poll_out = opensbi_console_poll_out,
	.err_check = opensbi_console_err_check,

};

#define OPENSBI_CONSOLE_INIT(n)                                                                    \
                                                                                                   \
	static const struct opensbi_console_config opensbi_console_##n##_cfg = {};                 \
                                                                                                   \
	static struct opensbi_console_data opensbi_console_##n##_data = {};                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &opensbi_console_init, NULL, &opensbi_console_##n##_data,         \
			      &opensbi_console_##n##_cfg, PRE_KERNEL_1,                            \
			      CONFIG_SERIAL_INIT_PRIORITY, &opensbi_console_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OPENSBI_CONSOLE_INIT)
