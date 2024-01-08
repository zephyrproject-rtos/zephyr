/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

DEVICE_DT_DEFINE(DT_INST(0, vnd_gpio_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);
DEVICE_DT_DEFINE(DT_INST(0, vnd_i2c), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);

DEVICE_DT_DEFINE(DT_INST(0, vnd_i2c_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, 49, NULL);
DEVICE_DT_DEFINE(DT_INST(1, vnd_i2c_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, 50, NULL);
