/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

static int init_fn_0(const struct device *dev)
{
	return 0;
}

static int init_fn_1(const struct device *dev)
{
	return 0;
}

DEVICE_DT_DEFINE(DT_INST(0, vnd_gpio_device), init_fn_0, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);
DEVICE_DT_DEFINE(DT_INST(0, vnd_i2c), init_fn_1, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);

DEVICE_DT_DEFINE(DT_INST(0, vnd_i2c_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, 49, NULL);
DEVICE_DT_DEFINE(DT_INST(1, vnd_i2c_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);
