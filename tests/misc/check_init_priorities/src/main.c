/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

static int device_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_DEFINE(DT_INST(0, vnd_gpio_device), device_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);
DEVICE_DT_DEFINE(DT_INST(0, vnd_i2c), device_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);

DEVICE_DT_DEFINE(DT_INST(0, vnd_i2c_device), device_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, 49, NULL);
DEVICE_DT_DEFINE(DT_INST(1, vnd_i2c_device), device_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);
DEVICE_DT_DEFINE(DT_INST(2, vnd_i2c_device), device_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, 51, NULL);
