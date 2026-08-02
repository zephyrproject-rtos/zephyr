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

/* Automatically ordered device: populates the automatic-ordering slot of the
 * level, which runs after the whole manual priority range.
 */
DEVICE_DT_DEFINE_AUTO(DT_INST(2, vnd_i2c_device), NULL, NULL, NULL, NULL,
		      PRE_KERNEL, NULL);

/* Initialization function ordered after a device, which lands in the same
 * automatic-ordering slot.
 */
static int init_fn_2(void)
{
	return 0;
}

SYS_INIT_DEPENDS(init_fn_2, PRE_KERNEL, DT_INST(0, vnd_i2c));

/* Deliberate violation: the device this function is ordered after is
 * initialized at a later level, so the linker sort cannot honour the
 * dependency. The build-time check must report it.
 */
DEVICE_DT_DEFINE(DT_INST(3, vnd_i2c_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_2, 50, NULL);

static int init_fn_3(void)
{
	return 0;
}

SYS_INIT_DEPENDS(init_fn_3, PRE_KERNEL, DT_INST(3, vnd_i2c_device));
