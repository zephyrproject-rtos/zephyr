/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ftdi_ft201x

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>

struct ftdi_ft201x_config {
	struct i2c_dt_spec i2c_spec;
};

static int ftdi_ft201x_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct ftdi_ft201x_config *config = (struct ftdi_ft201x_config *)dev->config;

	return i2c_read_dt(&config->i2c_spec, p_char, 1) == 0 ? 0 : -1;
}

static void ftdi_ft201x_poll_out(const struct device *dev, unsigned char c)
{
	struct ftdi_ft201x_config *config = (struct ftdi_ft201x_config *)dev->config;

	i2c_write_dt(&config->i2c_spec, &c, 1);
}

static const struct uart_driver_api ftdi_ft201x_api = {
	.poll_in = ftdi_ft201x_poll_in,
	.poll_out = ftdi_ft201x_poll_out,
};

static int ftdi_ft201x_init(const struct device *dev)
{
	struct ftdi_ft201x_config *config = (struct ftdi_ft201x_config *)dev->config;

	if (!device_is_ready(config->i2c_spec.bus)) {
		return -ENODEV;
	}

	return 0;
}

#define FTDI_FT201X_DEVICE(inst)						\
	struct ftdi_ft201x_config ftdi_ft201x_config##inst = {		\
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, ftdi_ft201x_init, NULL, NULL,		\
			      &ftdi_ft201x_config##inst, POST_KERNEL,		\
			      CONFIG_SERIAL_INIT_PRIORITY, &ftdi_ft201x_api);

DT_INST_FOREACH_STATUS_OKAY(FTDI_FT201X_DEVICE)
