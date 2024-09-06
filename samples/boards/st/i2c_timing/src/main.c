/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>


#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_0)
#else
#error "Please set the correct I2C device"
#endif

int main(void)
{
	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
	uint32_t i2c_cfg;

	if (!device_is_ready(i2c_dev)) {
		printk("I2C device is not ready\n");
		return -1;
	}

	/* Verify i2c_get_config() */
	if (i2c_get_config(i2c_dev, (uint32_t *)&i2c_cfg)) {
		printk("I2C get_config failed\n");
		return -1;
	}

	/* I2C BIT RATE */
	printk("\nI2C freq.");

	if ((I2C_SPEED_GET(i2c_cfg) == 1) &&
		(DT_PROP(I2C_DEV_NODE, clock_frequency) == 100000)) {
		printk(" I2C_BITRATE_STANDARD\n");
	} else if ((I2C_SPEED_GET(i2c_cfg) == 2) &&
		(DT_PROP(I2C_DEV_NODE, clock_frequency) == 400000)) {
		printk(" I2C_BITRATE_FAST\n");
	} else if ((I2C_SPEED_GET(i2c_cfg) == 3) &&
		(DT_PROP(I2C_DEV_NODE, clock_frequency) == 1000000)) {
		printk(" I2C_SPEED_FAST_PLUS\n");
	} else {
		printk(" I2C speed mismatch (%d)\n", I2C_SPEED_GET(i2c_cfg));
	}

	return 0;
}
