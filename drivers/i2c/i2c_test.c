/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real I2C driver. It is used to instantiate struct
 * devices for the "vnd,i2c" devicetree compatible used in test code.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT vnd_i2c

static int vnd_i2c_configure(const struct device *dev,
			     uint32_t dev_config)
{
	return -ENOTSUP;
}

static int vnd_i2c_transfer(const struct device *dev,
			    struct i2c_msg *msgs,
			    uint8_t num_msgs, uint16_t addr)
{
	return -ENOTSUP;
}

static const struct i2c_driver_api vnd_i2c_api = {
	.configure = vnd_i2c_configure,
	.transfer  = vnd_i2c_transfer,
};

#define VND_I2C_INIT(n)							       \
	I2C_DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,      \
				  CONFIG_I2C_INIT_PRIORITY, &vnd_i2c_api);

DT_INST_FOREACH_STATUS_OKAY(VND_I2C_INIT)
